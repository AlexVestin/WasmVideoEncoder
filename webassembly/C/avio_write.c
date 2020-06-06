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

static struct SwsContext *audio_swr_ctx = NULL;
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

const int BUFFER_ADD_SIZE = 32*1048576;

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
        bd->buf = av_realloc_f(bd->buf, 1, bd->size + BUFFER_ADD_SIZE);
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
    
    free(&buf);
    
    return buf_size;
}

static void encode(AVFrame *frame, AVCodecContext* cod, AVStream* out, AVPacket* p) {    
    ret = avcodec_send_frame(cod, frame);

    if (ret < 0) {
        //printf(stderr, "Error sending a frame for encoding\n");
        exit(1);
    }
    while (ret >= 0) {
        ret = avcodec_receive_packet(cod, p);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
            av_packet_unref(p);
            return;
        }
        else if (ret < 0) {
            //printf(stderr, "Error during encoding\n");
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
        printf("frame nr: %d buf size: %d room: %d \n", count, bd.size, bd.room);
    }
    
    free(yuv_buffer);
}


void write_header() {
    ret = avformat_write_header(ofmt_ctx, NULL);
    if (ret < 0) {
        printf("Error occurred when opening output file\n");
        exit(1);
    } 
}

void open_video(int w, int h, int fps, int br, int preset_idx, int codec_idx, int format_idx){
    const char* formats[] = {"webm",  "mp4", "mp3", "aac", "ogg" };
    const char* codecs[] = { "libvpx", "libx264" };
    AVOutputFormat* of = av_guess_format(formats[1], 0, 0);
    bd.ptr  = bd.buf = av_malloc(BUFFER_ADD_SIZE * 4);
    
    if (!bd.buf) {
        printf("BUF ERROR\n");
        ret = AVERROR(ENOMEM);
    }
    bd.size = bd.room = BUFFER_ADD_SIZE * 4;
    
    avio_ctx_buffer = av_malloc(avio_ctx_buffer_size);
    printf("Allocating buffer...\n");
    if (!avio_ctx_buffer) {
        ret = AVERROR(ENOMEM);   
        exit(1);
    }
    printf("Allocating avio context...\n");
    avio_ctx = avio_alloc_context(avio_ctx_buffer, avio_ctx_buffer_size, 1, &bd, NULL, &write_packet, &seek);
    if (!avio_ctx) {
        ret = AVERROR(ENOMEM);
        exit(1);
    }
    printf("Allocating output context...\n");
    ret = avformat_alloc_output_context2(&ofmt_ctx, of, NULL, NULL);
    if (ret < 0) {
        printf("Could not create output context\n");
        exit(1);
    }
    printf("Finding encoder %s...\n", codecs[codec_idx]);
    AVCodec* video_codec = avcodec_find_encoder_by_name(codecs[codec_idx]);
    printf("Gotten... %p\n", video_codec);
    if (!video_codec) {
        printf("Codec '%s' not found\n", codec_name);
        exit(1);
    }
    
    printf("Allocating codec context...\n");
    video_ctx = avcodec_alloc_context3(video_codec);
    video_ctx->width = w;
    video_ctx->height = h;
    video_ctx->time_base.num = 1;
    video_ctx->time_base.den = fps;
    video_ctx->bit_rate = br; 
    
    video_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    video_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    const char *presets[] = { "ultrafast", "veryfast", "fast", "medium", "slow", "veryslow" };
    
    av_opt_set(video_ctx->priv_data, "preset", presets[preset_idx], 0);
    printf("Opening codec...\n");
    if(avcodec_open2(video_ctx, video_codec, NULL) < 0) {
        printf("couldnt open codec\n");
        exit(1);
    }

    // Frame initalization
    video_frame = av_frame_alloc();
    video_frame->format = video_ctx->pix_fmt;
    video_frame->width  = w;
    video_frame->height = h;
    ret = av_frame_get_buffer(video_frame, 0);

    printf("Allocating packet...\n");
    pkt = av_packet_alloc();
    if(!pkt){
        printf("errror packer\n");
        exit(1);
    }
    printf("Allocating stream...\n");
    video_stream = avformat_new_stream(ofmt_ctx, NULL);  
    if(!video_stream){
        printf(stderr, "error making stream\n");
        exit(1);
    }
    
    ofmt_ctx->pb = avio_ctx;
    ofmt_ctx->flags |= AVFMT_FLAG_CUSTOM_IO;

    ofmt_ctx->oformat = of;
    video_stream->codec->codec_tag = 0;

    video_stream->time_base = video_ctx->time_base;
    video_stream->id = ofmt_ctx->nb_streams-1;
    printf("Setting parameters...\n");
    ret = avcodec_parameters_from_context(video_stream->codecpar, video_ctx);

    frame_idx = 0;
    have_video = 1;
    printf("Done opening video...\n");
} 

uint8_t* close_stream(int* size) {
    printf("Flushing video...\n");
    if(have_video)encode(NULL, video_ctx, video_stream, pkt);
    printf("Flushing audio...\n");
    if(have_audio)encode(NULL, audio_ctx, audio_stream, pkt);

    printf("Writing trailer...\n");
    av_write_trailer(ofmt_ctx);
    printf("Freeing context...\n");
    avformat_free_context(ofmt_ctx);
    
    printf("Closing video...\n");

    if (have_video) {
        avcodec_free_context(&video_ctx);    
        av_frame_free(&video_frame);
    }
    printf("Closing audio...\n");
    if (have_audio) {    
        avcodec_free_context(&audio_ctx);
        av_frame_free(&audio_frame);
        swr_free(&audio_swr_ctx);
    }

    printf("Freeing buffers...\n");
    av_freep(&avio_ctx->buffer);
    av_free(avio_ctx);
    printf("Returning...\n");
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
        printf(stderr, "Error allocating an audio frame\n");
        exit(1);
    }

    audio_frame->format           = audio_ctx->sample_fmt;
    audio_frame->channel_layout   = audio_ctx->channel_layout;
    audio_frame->sample_rate      = audio_ctx->sample_rate;
    audio_frame->nb_samples       = audio_ctx->frame_size;

    ret = av_frame_get_buffer(audio_frame, 4);
    if (ret < 0) {
        printf(stderr, "Error allocating an audio buffer\n");
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
        printf("Haven't initialized the audio");
        exit(1);
    }
    return audio_frame->nb_samples;
}

void add_audio_frame(float* left, float* right, int size) {

    ret = av_frame_make_writable(audio_frame);
    if (ret < 0) {
        printf("error\n");
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
        printf("Error in swr_convert, ret: %d", ret);
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

void test(){
    printf("TESTED\n");
}

void open_audio(int sample_rate, int nr_channels, int bit_rate, int codec_idx) {
    src_sample_rate     = sample_rate;
    src_bit_rate        = bit_rate;
    src_nr_channels     = nr_channels;

    const int codecs[] = { AV_CODEC_ID_OPUS, AV_CODEC_ID_AAC, AV_CODEC_ID_MP3 };    
    
    AVCodec* ac = avcodec_find_encoder(codecs[2]);
    if(!ac) {
        printf("Error creating audio codec context\n");
        exit(-1);
    }

    audio_stream = avformat_new_stream(ofmt_ctx, NULL);
    audio_stream->id = ofmt_ctx->nb_streams-1;
    audio_ctx = avcodec_alloc_context3(ac);

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
        printf(stderr, "Could not open audio codec: %s\n", av_err2str(ret));
        exit(1);
    }

    audio_frame = alloc_audio_frame();

    ret = avcodec_parameters_from_context(audio_stream->codecpar, audio_ctx);
    if (ret < 0) {
        printf(stderr, "Could not copy the stream parameters\n");
        exit(1);
    }

    audio_swr_ctx = swr_alloc();
    if (!audio_swr_ctx) {
        printf(stderr, "Could not allocate resampler context\n");
        exit(1);
    }

    av_opt_set_int       (audio_swr_ctx, "in_channel_count",   nr_channels,               0);
    av_opt_set_int       (audio_swr_ctx, "in_sample_rate",     sample_rate,               0);
    av_opt_set_sample_fmt(audio_swr_ctx, "in_sample_fmt",      AV_SAMPLE_FMT_FLTP,        0);
    av_opt_set_int       (audio_swr_ctx, "out_channel_count",  audio_ctx->channels,       0);
    av_opt_set_int       (audio_swr_ctx, "out_sample_rate",    audio_ctx->sample_rate,    0);
    av_opt_set_sample_fmt(audio_swr_ctx, "out_sample_fmt",     audio_ctx->sample_fmt,     0);

    if ((ret = swr_init(audio_swr_ctx)) < 0) {
        printf(stderr, "Failed to initialize the resampling context\n");
        exit(1);
    }
    if(!pkt)pkt = av_packet_alloc();

    if(!pkt){
        exit(1);
    }


    have_audio = 1;
    printf("%d \n", getAudioFrameSize());
}


void open_audio_pre(float* left, float* right, int size){
    //printf("size: %d sample_rate: %d channels: %d bitrate: %d \n", size,sample_rate, nr_channels,bit_rate);
    src_size = size * sizeof(float);
    src_buf_left = (uint8_t*)left;
    src_buf_right = (uint8_t*)right;
}