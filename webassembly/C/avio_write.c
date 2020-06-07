#include "avio_write.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include <libavformat/avio.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>

struct buffer_data {
    uint8_t *buf;
    int size;
    uint8_t *ptr;
    size_t room; ///< size left in the buffer
};

AVFormatContext *ofmt_ctx = NULL;
AVIOContext *avio_ctx = NULL;
uint8_t *avio_ctx_buffer = NULL;
size_t avio_ctx_buffer_size = 4096;
int i, ret = 0;
struct buffer_data bd = { 0 };
const size_t bd_buf_size = 1024;
const char* codec_name = "libvpx";

//VIDEO
AVFrame *video_frame, *audio_frame;
int frame_idx;
AVStream *video_stream = NULL;
AVStream *audio_stream = NULL;

AVPacket *pkt;

static struct SwrContext *audio_swr_ctx = NULL;
AVCodecContext *video_ctx, *audio_ctx;


const int NR_COLORS = 4;
int have_audio = 0, have_video = 0;

//AUDIO
int frame_bytes, audio_idx, dst_nb_samples; 
int src_sample_rate, src_bit_rate, src_nr_channels, src_size;
uint8_t* src_buf_left;
uint8_t* src_buf_right;


float* audio_buffer_left;
float* audio_buffer_right;
int audio_buffer_size, max_audio_buffer_size;

const int BUFFER_ADD_SIZE = 64*1048576;
int supposedSize = 0;

char* log_str;
// https://stackoverflow.com/questions/5901181/c-string-append
void _log(char* str2) {
  char * new_str ;
  printf(str2);
  if((new_str = malloc(strlen(log_str)+strlen(str2)+1)) != NULL){
      new_str[0] = '\0';   // ensures the memory is an empty string
      strcat(new_str, log_str);
      strcat(new_str, str2);
      free(log_str);
      log_str = new_str;
  } else {
      printf("malloc for log string failed!\n");
      exit(1);
  }
}

// https://stackoverflow.com/questions/5901181/c-string-append
int _log_append(char **json, const char *format, ...)
{
    char *str = NULL;
    char *old_json = NULL, *new_json = NULL;
  
    va_list arg_ptr;
    va_start(arg_ptr, format);
    vasprintf(&str, format, arg_ptr);

    // log to console
    printf(str);

    // save old json
    asprintf(&old_json, "%s", (*json == NULL ? "" : *json));

    // calloc new json memory
    new_json = (char *)calloc(strlen(old_json) + strlen(str) + 1, sizeof(char));

    strcat(new_json, old_json);
    strcat(new_json, str);

    if (*json) free(*json);
    *json = new_json;

    free(old_json);
    free(str);
  
    return 0;
}

int get_log_size() {
  return strlen(log_str);
}

char* get_log() {
  return log_str;
}

static int64_t seek (void *opaque, int64_t offset, int whence) {
    struct buffer_data *bd = (struct buffer_data *)opaque;
    switch(whence){
        case SEEK_SET:
            bd->ptr = bd->buf + offset;
            return bd->ptr;
            break;
        case SEEK_CUR:
            bd->ptr += offset;
            break;
        case SEEK_END:
            bd->ptr = (bd->buf + bd->size) + offset;
            return bd->ptr;
            break;
        case AVSEEK_SIZE:
            return bd->size;
            break;
        default:
           return -1;
    }
    return 1;
}

static int write_packet(void *opaque, uint8_t *buf, int buf_size) {
    struct buffer_data *bd = (struct buffer_data *)opaque;
    
    while (buf_size > bd->room) {
        int64_t offset = bd->ptr - bd->buf;
        bd->buf = realloc(bd->buf, bd->size + BUFFER_ADD_SIZE);//(av_realloc_f(bd->buf, 1, bd->size + BUFFER_ADD_SIZE);
        if (!bd->buf)
            return AVERROR(ENOMEM);
        bd->size += BUFFER_ADD_SIZE;
        bd->ptr = bd->buf + offset;
        bd->room = bd->size - offset;
    }
    
    //printf("write packet pkt_size:%d used_buf_size:%zu buf_size:%zu buf_room:%zu\n", buf_size, bd->ptr-bd->buf, bd->size, bd->room);

    memcpy(bd->ptr, buf, buf_size);
    bd->ptr  += buf_size;
    bd->room -= buf_size;
      
    supposedSize += buf_size;
    return buf_size;
}

static void encode(AVFrame *frame, AVCodecContext* cod, AVStream* out, AVPacket* p) {    
    ret = avcodec_send_frame(cod, frame);

    if (ret < 0) {
        _log_append(&log_str, "Error: Sending a frame for encoding failed: %s\n", av_err2str(ret));
        exit(1);
    }
    while (ret >= 0) {
        ret = avcodec_receive_packet(cod, p);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
            av_packet_unref(p);
            return;
        }
        else if (ret < 0) {
            _log_append(&log_str, "Error: Encoding failed with: %s\n", av_err2str(ret));
            exit(1);
        }

        //log_packet(ofmt_ctx, pkt, "write");
        p->stream_index = out->index;      
        av_packet_rescale_ts(p, cod->time_base, out->time_base);
        av_write_frame(ofmt_ctx, p);
        av_packet_unref(p);
        
    }
}

void flip_vertically(uint8_t *pixels) {
    const size_t width = video_ctx->width;
    const size_t height = video_ctx->height;
    
    const size_t stride = width * NR_COLORS;
    uint8_t *row = malloc(stride);
    uint8_t *low = pixels;
    uint8_t *high = &pixels[(height - 1) * stride];

    for (; low < high; low += stride, high -= stride) {
        memcpy(row, low, stride);
        memcpy(low, high, stride);
        memcpy(high, row, stride);
    }
    free(row);
}

void rgb2yuv420p(uint8_t *destination, uint8_t *rgb, size_t width, size_t height)
{
    size_t image_size = width * height;
    size_t upos = image_size;
    size_t vpos = upos + upos / 4;
    size_t i = 0;
    uint8_t r, g, b;

    size_t idx;

    for( size_t line = 0; line < height; ++line ) {
        if( !(line % 2) ) {
            for( size_t x = 0; x < width; x += 2 )
            {
                r = rgb[NR_COLORS * i];
                g = rgb[NR_COLORS * i + 1];
                b = rgb[NR_COLORS * i + 2];

        
                destination[i++] = ((66*r + 129*g + 25*b) >> 8) + 16;

                destination[upos++] = ((-38*r + -74*g + 112*b) >> 8) + 128;
                destination[vpos++] = ((112*r + -94*g + -18*b) >> 8) + 128;

                r = rgb[NR_COLORS * i];
                g = rgb[NR_COLORS * i + 1];
                b = rgb[NR_COLORS * i + 2];

                destination[i++] = ((66*r + 129*g + 25*b) >> 8) + 16;
            }
        }
        else
        {
            for( size_t x = 0; x < width; x += 1 )
            {
                r = rgb[NR_COLORS * i];
                g = rgb[NR_COLORS * i + 1];
                b = rgb[NR_COLORS * i + 2];

                destination[i++] = ((66*r + 129*g + 25*b) >> 8) + 16;
            }
        }
    }  
}

int count = 0;
void add_video_frame(uint8_t* frame){ 

   
    flip_vertically(frame);
    ret = av_frame_make_writable(video_frame);

    // ~15% faster than sws_scale
    int size = (video_ctx->width * video_ctx->height * 3) / 2;
    uint8_t* yuv_buffer = malloc(size);

    rgb2yuv420p(yuv_buffer, frame, video_ctx->width, video_ctx->height);

    av_image_fill_arrays (
        (AVPicture*)video_frame->data,
        video_frame->linesize, 
        yuv_buffer, 
        video_frame->format, 
        video_frame->width, 
        video_frame->height, 
        1
    );

    video_frame->pts = frame_idx++;

    encode(video_frame, video_ctx, video_stream, pkt);
    //av_frame_free(video_frame);
    if( !(count++ % 300)) {
        _log_append(&log_str, "Frame nr: %d buf size: %d room: %d  counter: %d\n", count, bd.size, bd.room, supposedSize);
    }
    
    free(yuv_buffer);
}


void write_header() {
    _log("Writing header\n");
    ret = avformat_write_header(ofmt_ctx, NULL);

    if (ret < 0) {
        _log_append(&log_str, "Error occurred when opening output file %s\n", av_err2str(ret));
        exit(1);
    } 
    _log("Header written\n");
}

void open_video(int w, int h, int fps, int br, int preset_idx, int codec_idx, int format_idx, int duration){
    _log("Opening video\n");

    const char* formats[] = {"webm",  "mp4", "mp3", "aac", "ogg" };
    const char* codecs[] = { "libvpx", "libx264" };
    AVOutputFormat* of = av_guess_format(formats[1], 0, 0);

    // TODO fix memory fragmentations
    int assumed_br = br;
    int mult = w == 1920 ? 12 : 6;
    if (br == 0) {
      assumed_br = w == 1280 ? mult * 1024 * 1024 : 3 * 1024 * 1024;
    }

    _log_append(&log_str, "Using w: %d h: %d fps: %d br: %d preset: %d duration: %d \n", w, h, fps, br, preset_idx, duration);

    // we want to malloc more than the video size
    int padded_buf_size = 1.2 * ((assumed_br/8) * duration); 
    const int TEST_BUF_SIZE = padded_buf_size;//BUFFER_ADD_SIZE*4;//617843505*2;
    bd.ptr = bd.buf = av_malloc(TEST_BUF_SIZE);
    
    _log_append(&log_str, "padded buf size: %d\n", padded_buf_size);  
    _log_append(&log_str, "bd.ptr=%p, bd.buf=%p\n", bd.ptr, bd.buf);

    if (!bd.buf) {
        _log("Warning: Failed to create internal buffer using av_malloc\n");
        // Probably there's no difference using malloc instead of av_malloc, but worth a shot 
        bd.ptr = bd.buf = malloc(TEST_BUF_SIZE);
        if (!bd.buf) {
          bd.buf = bd.ptr = malloc(10);
          

          if (!bd.buf) {
            _log("Can't malloc at all\n");
            exit(1);
          }

          bd.buf = bd.ptr = malloc(16*1000*1000);
          if (bd.buf) {

          }

          bd.buf = bd.ptr = malloc(BUFFER_ADD_SIZE);
          _log("Can malloc, trying buffer size\n");
          if(!bd.buf) {
            _log("Warning: Failed to create internal buffer\n");
            exit(1);
          }
        }
    }
    bd.size = bd.room = TEST_BUF_SIZE;
    
    avio_ctx_buffer = av_malloc(avio_ctx_buffer_size);
    _log("Allocating buffer\n");
    if (!avio_ctx_buffer) {
        _log("Error: Failed to allocate memory for temporary avio buffer\n");
        exit(1);
    }

    _log("Allocating avio context\n");
    avio_ctx = avio_alloc_context(avio_ctx_buffer, avio_ctx_buffer_size, 1, &bd, NULL, &write_packet, &seek);
    if (!avio_ctx) {
        _log("Error: Failed to allocate avio_ctx\n");
        exit(1);
    }
    _log("Allocating output context \n");
    ret = avformat_alloc_output_context2(&ofmt_ctx, of, NULL, NULL);
    if (ret < 0) {
        _log_append(&log_str, "Error: Could not create output context: %s\n", av_err2str(ret));
        exit(1);
    }
    _log_append(&log_str, "Finding encoder %s \n", codecs[codec_idx]);
    AVCodec* video_codec = avcodec_find_encoder_by_name(codecs[codec_idx]);
    _log("Gotten encoder \n");
    if (!video_codec) {
        _log_append(&log_str, "Error: Codec '%s' not found\n", codec_name);
        exit(1);
    }
    
    _log("Allocating codec context\n");
    video_ctx = avcodec_alloc_context3(video_codec);
    if (!video_ctx) {
      _log("Failed to allocate video context\n");
      exit(1);
    }
    video_ctx->width = w;
    video_ctx->height = h;
    video_ctx->time_base.num = 1;
    video_ctx->time_base.den = fps;
    video_ctx->bit_rate = br; 
    
    video_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    video_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    const char *presets[] = { "ultrafast", "veryfast", "fast", "medium", "slow", "veryslow" };
    
    av_opt_set(video_ctx->priv_data, "preset", presets[preset_idx], 0);
    _log("Opening codec...\n");
    ret = avcodec_open2(video_ctx, video_codec, NULL);
    if(ret < 0) {
        _log_append(&log_str, "Error: couldn't open codec: %s \n", av_err2str(ret));
        exit(1);
    }

    // Frame initalization
    video_frame = av_frame_alloc();
    video_frame->format = video_ctx->pix_fmt;
    video_frame->width  = w;
    video_frame->height = h;
    ret = av_frame_get_buffer(video_frame, 0);

    _log("Allocating packet\n");
    pkt = av_packet_alloc();
    if(!pkt){
        _log("Error: failed to allocate packet\n");
        exit(1);
    }
    _log("Allocating stream\n");
    video_stream = avformat_new_stream(ofmt_ctx, NULL);  
    if(!video_stream){
        _log("Error: Failed createing video stream\n");
        exit(1);
    }
    
    ofmt_ctx->pb = avio_ctx;
    ofmt_ctx->flags |= AVFMT_FLAG_CUSTOM_IO;

    ofmt_ctx->oformat = of;

    video_stream->time_base = video_ctx->time_base;
    video_stream->id = ofmt_ctx->nb_streams-1;
    _log("Setting parameters...\n");
    ret = avcodec_parameters_from_context(video_stream->codecpar, video_ctx);

    frame_idx = 0;
    have_video = 1;
    _log("Done opening video...\n");
} 

uint8_t* close_stream(int* size) {
    int ret;

    _log("Flushing video...\n");
    if(have_video){
      encode(NULL, video_ctx, video_stream, pkt);
    }

    _log("Flushing audio...\n");
    if(have_audio) {
      encode(NULL, audio_ctx, audio_stream, pkt);
    }
    
    _log("Writing trailer...\n");
    ret = av_write_trailer(ofmt_ctx);
    if (ret < 0) {
      _log_append(&log_str, "Error: writing trailer %s\n", av_err2str(ret));
    }
    _log("Freeing context...\n");
    avformat_free_context(ofmt_ctx);
    
    _log("Closing video...\n");

    if (have_video) {
        avcodec_free_context(&video_ctx);    
        av_frame_free(&video_frame);
    }
    _log("Closing audio...\n");
    if (have_audio) {    
        avcodec_free_context(&audio_ctx);
        av_frame_free(&audio_frame);
        swr_free(&audio_swr_ctx);
    }

    _log("Freeing buffers...\n");
    av_freep(&avio_ctx->buffer);
    av_free(avio_ctx);
    _log("Returning...\n");
    *size = bd.size - bd.room;
    return bd.buf; 
}   

void free_buffer(){
    av_free(bd.buf);
}

static AVFrame *alloc_audio_frame() {
    AVFrame *audio_frame = av_frame_alloc();
    int ret;

    if (!audio_frame) {
        _log("Error: Failed to allocate an audio frame\n");
        exit(1);
    }

    audio_frame->format           = audio_ctx->sample_fmt;
    audio_frame->channel_layout   = audio_ctx->channel_layout;
    audio_frame->sample_rate      = audio_ctx->sample_rate;
    audio_frame->nb_samples       = audio_ctx->frame_size;

    ret = av_frame_get_buffer(audio_frame, 4);
    if (ret < 0) {
        _log_append(&log_str, "Error: Failed to get an audio buffer: %s\n", av_err2str(ret));
        exit(1);
    }
    
    return audio_frame;
}

static int check_sample_fmt(const AVCodec *codec, enum AVSampleFormat sample_fmt){
    const enum AVSampleFormat *p = codec->sample_fmts;
    while (*p != AV_SAMPLE_FMT_NONE) {
        if (*p == sample_fmt)
            return 1;
        p++;
    }
    return 0;
}


int getAudioFrameSize() {
    if(!have_audio || !audio_frame) {
        _log("Error: Haven't initialized the audio");
        exit(1);
    }
    return audio_frame->nb_samples;
}

void add_audio_frame(float* left, float* right, int size) {

    ret = av_frame_make_writable(audio_frame);
    if (ret < 0) {
        _log_append(&log_str,"Error: making frame writable %s \n", av_err2str(ret));
        exit(1);
    }

    audio_frame->data[0] = (uint8_t*)left;
    audio_frame->data[1] = (uint8_t*)right;
    dst_nb_samples =
        av_rescale_rnd(swr_get_delay(audio_swr_ctx, audio_ctx->sample_rate) +
                            audio_frame->nb_samples,
                        src_sample_rate, audio_ctx->sample_rate, AV_ROUND_UP);

    ret =
        swr_convert(audio_swr_ctx, audio_frame->data, dst_nb_samples,
                    (const uint8_t**)audio_frame->data, audio_frame->nb_samples);

    if (ret < 0) {
        _log_append(&log_str,"Error: Failed to create swr convert, ret %s \n", av_err2str(ret));
        exit(1);
    }

    audio_frame->pts =
        av_rescale_q(frame_bytes, (AVRational){1, audio_ctx->sample_rate},
                    audio_ctx->time_base);

    frame_bytes += dst_nb_samples;

    AVPacket audio_pkt;
    av_init_packet(&audio_pkt);
    encode(audio_frame, audio_ctx, audio_stream, &audio_pkt);
}


void open_audio(int sample_rate, int nr_channels, int bit_rate, int codec_idx) {
    src_sample_rate     = sample_rate;
    src_bit_rate        = bit_rate;
    src_nr_channels     = nr_channels;

    _log_append(&log_str, "Opening audio with src_sample_rate: %d src_bit_rate: %d src_nr_channels: %d \n",src_sample_rate, bit_rate, src_nr_channels );

    const int codecs[] = { AV_CODEC_ID_OPUS, AV_CODEC_ID_AAC, AV_CODEC_ID_MP3 };    
    
    AVCodec* ac = avcodec_find_encoder(codecs[2]);
    if (!ac) {
        _log("Error: Failed to create audio codec context\n");
        exit(-1);
    }

    audio_stream = avformat_new_stream(ofmt_ctx, NULL);
    if (!audio_stream) {
      _log("Error: Failed to create stream for audio codec context\n");
      exit(1);
    }
    audio_stream->id = ofmt_ctx->nb_streams-1;
    audio_ctx = avcodec_alloc_context3(ac);
    if (!audio_ctx) {
      _log("Error: Failed to allocate context for audio\n");
      exit(1);
    }

    audio_ctx->bit_rate = bit_rate;
    audio_ctx->sample_rate = sample_rate;
    if (ac->supported_samplerates) {
        audio_ctx->sample_rate = ac->supported_samplerates[0];
        for (i = 0; ac->supported_samplerates[i]; i++) {
            if (ac->supported_samplerates[i] == sample_rate)
                audio_ctx->sample_rate = sample_rate;
        }
    }
    audio_ctx->channels = nr_channels;
    audio_ctx->channel_layout = AV_CH_LAYOUT_STEREO;
    if (ac->channel_layouts) {
        audio_ctx->channel_layout = ac->channel_layouts[0];
        for (i = 0; ac->channel_layouts[i]; i++) {
            if (ac->channel_layouts[i] == AV_CH_LAYOUT_STEREO)
                audio_ctx->channel_layout = AV_CH_LAYOUT_STEREO;
        }
    }

    audio_ctx->channels  = av_get_channel_layout_nb_channels(audio_ctx->channel_layout);    
    audio_ctx->sample_fmt = ac->sample_fmts[0];
    audio_stream->time_base = (AVRational){1, audio_ctx->sample_rate};

    ret = avcodec_open2(audio_ctx, ac, NULL);
    if (ret < 0) {
        _log_append(&log_str, "Error: Could not open audio codec: %s\n", av_err2str(ret));
        exit(1);
    }

    audio_frame = alloc_audio_frame();

    ret = avcodec_parameters_from_context(audio_stream->codecpar, audio_ctx);
    if (ret < 0) {
        _log_append(&log_str,"Error: Could not set parameter from codec: %s\n", av_err2str(ret));
        exit(1);
    }

    _log("Audio stream, codec & frame set up\n");

    audio_swr_ctx = swr_alloc();
    if (!audio_swr_ctx) {
        _log_append(&log_str, "Error: Could not allocate swr context\n");
        exit(1);
    }

    av_opt_set_int       (audio_swr_ctx, "in_channel_count",   nr_channels,               0);
    av_opt_set_int       (audio_swr_ctx, "in_sample_rate",     sample_rate,               0);
    av_opt_set_sample_fmt(audio_swr_ctx, "in_sample_fmt",      AV_SAMPLE_FMT_FLTP,        0);
    av_opt_set_int       (audio_swr_ctx, "out_channel_count",  audio_ctx->channels,       0);
    av_opt_set_int       (audio_swr_ctx, "out_sample_rate",    audio_ctx->sample_rate,    0);
    av_opt_set_sample_fmt(audio_swr_ctx, "out_sample_fmt",     audio_ctx->sample_fmt,     0);

    if ((ret = swr_init(audio_swr_ctx)) < 0) {
        _log_append(&log_str, "Could not initialize swr context: %s\n", av_err2str(ret));
        exit(1);
    }

    _log("Swr context set up\n");
    if(!pkt) {
      pkt = av_packet_alloc();
    }
    if(!pkt){
      _log("Error: Could not initialize video packet\n");
      exit(1);
    }

    have_audio = 1;
    _log_append(&log_str, "Audio set up, frame size: %d \n", getAudioFrameSize());
}
