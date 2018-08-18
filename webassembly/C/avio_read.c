/*
 * Copyright (c) 2012 Stefano Sabatini
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 * @file
 * Demuxing and decoding example.
 *
 * Show how to use the libavformat and libavcodec API to demux and
 * decode audio and video data.
 * @example demuxing_decoding.c
 */

#include "bonus.c"

#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>

#include <libswscale/swscale.h>

//audio
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>


struct SwrContext *swr_ctx;

static AVFormatContext *fmt_ctx = NULL;
static AVCodecContext *video_dec_ctx = NULL, *audio_dec_ctx;
static int width, height;
static enum AVPixelFormat pix_fmt;
static AVStream *video_stream = NULL, *audio_stream = NULL;
static struct SwsContext *sws_context = NULL;

static uint8_t *video_dst_data[4] = {NULL};
static int      video_dst_linesize[4];
static int      video_dst_bufsize;

static int video_stream_idx = -1, audio_stream_idx = -1;
static AVFrame *frame = NULL;
static AVPacket pkt;
static int video_frame_count = 0;
static int audio_frame_count = 0;

/* Enable or disable frame reference counting. You are not supposed to support
 * both paths in your application but pick the one most appropriate to your
 * needs. Look for the use of refcount in this example to see what are the
 * differences of API usage between them. */

static int refcount = 0;
AVIOContext *avio_ctx = NULL;


//AUDIO
int src_nb_channels, src_rate, src_fmt, src_nb_samples;
int dst_nb_samples, max_dst_nb_samples, dst_linesize, dst_sample_fmt = AV_SAMPLE_FMT_FLTP, dst_rate = 48000;
int dst_bufsize, dst_nb_channels, dst_ch_layout = 2;
uint8_t** dst_data;
// AVIO
struct buffer_data {
    uint8_t* buf;
    uint8_t* ptr;
    size_t size;
    size_t total_size;
};

struct buffer_data bd = { 0 }; 
int time_base = 0;

static int64_t seek (void *opaque, int64_t offset, int whence) {
    struct buffer_data *bd = (struct buffer_data *)opaque;
    switch(whence){
        case SEEK_SET:
            bd->size = bd->total_size - offset;
            bd->ptr = bd->buf + offset;
            break;
        case SEEK_CUR:
            bd->ptr += offset;
            bd->size -= offset;
            break;
        case SEEK_END:
            bd->size = 0;
            bd->ptr = (bd->buf + bd->total_size);
            break;
        case AVSEEK_SIZE:
            return bd->total_size;
        default:
           return -1;
    }
    return 0;
}

int image_inited = 0;


static int decode_audio(uint8_t** left, uint8_t** right, int* frame_size) {
    int ret = 0, dec_ret = 0;
    int decoded = pkt.size;

    uint8_t* tmp_right = NULL;
    uint8_t* tmp_left = NULL;
    
    dec_ret = avcodec_send_packet(audio_dec_ctx, &pkt);
    if (dec_ret < 0) {
        fprintf(stderr, "Error submitting audio packet (%s)\n", av_err2str(dec_ret));
        return dec_ret;
    }

    uint8_t* buf;

    int tsize = 0;
    while(dec_ret >= 0) {
        dec_ret = avcodec_receive_frame(audio_dec_ctx, frame);            
        if(dec_ret == AVERROR(EAGAIN) || dec_ret == AVERROR_EOF) {
            return decoded;
        }
        else if (dec_ret < 0) {
            printf("ERROR IN DECODING \n");
            exit(0);
        }


        if(!swr_ctx) {
            src_nb_channels =audio_dec_ctx->channels;
            src_rate = audio_dec_ctx->sample_rate;
            src_fmt = audio_dec_ctx->sample_fmt;
            src_nb_samples = frame->nb_samples;
            dst_sample_fmt = AV_SAMPLE_FMT_FLTP;
            dst_rate = 48000;
            dst_ch_layout = AV_CH_LAYOUT_STEREO;

            swr_ctx = swr_alloc();
            if (!swr_ctx) {
                fprintf(stderr, "Could not allocate resampler context\n");
                ret = AVERROR(ENOMEM);
                exit(1);
            }

            /* set options */
            av_opt_set_int(swr_ctx, "in_channel_layout",  src_nb_channels, 0);
            av_opt_set_int(swr_ctx, "in_sample_rate",       src_rate, 0);
            av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", src_fmt, 0);

            av_opt_set_int(swr_ctx, "out_channel_layout",    dst_ch_layout, 0);
            av_opt_set_int(swr_ctx, "out_sample_rate",       dst_rate, 0);
            av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", dst_sample_fmt, 0);

            if ((ret = swr_init(swr_ctx)) < 0) {
                fprintf(stderr, "Failed to initialize the resampling context\n");
                exit(1);
            }

            printf("frame samples: %d\n", frame->nb_samples);

            max_dst_nb_samples = dst_nb_samples = av_rescale_rnd( frame->nb_samples, dst_rate, src_rate, AV_ROUND_UP);
            dst_nb_channels = av_get_channel_layout_nb_channels(dst_ch_layout);
            ret = av_samples_alloc_array_and_samples(&dst_data, &dst_linesize, 2, dst_nb_samples, dst_sample_fmt, 0);

            if(ret < 0 ) {
                printf("couldnt alloc dest samples: %s \n", av_err2str(ret));
                exit(1);
            }

        }
        
        dst_nb_samples = av_rescale_rnd(swr_get_delay(swr_ctx, src_rate) +src_nb_samples, dst_rate, src_rate, AV_ROUND_UP);
        
        if (dst_nb_samples > max_dst_nb_samples) {
            av_freep(&dst_data[0]);
            ret = av_samples_alloc(dst_data, &dst_linesize, dst_nb_channels, dst_nb_samples, dst_sample_fmt, 1);
            if (ret < 0)
                break;
            
            max_dst_nb_samples = dst_nb_samples;
        }
        ret = swr_convert(swr_ctx, dst_data, dst_nb_samples, (const uint8_t **)frame->data, frame->nb_samples);
        if (ret < 0) {
            fprintf(stderr, "Error while converting\n");
            exit(1);
        }

        dst_bufsize = av_samples_get_buffer_size(&dst_linesize, dst_nb_channels, ret, dst_sample_fmt, 1);
        if (dst_bufsize < 0) {
            fprintf(stderr, "Could not get sample buffer size: %s\n", av_err2str(dst_bufsize));
            exit(1);
        }

        const int split = dst_bufsize / 2;
        tmp_left = realloc(tmp_left, split + tsize);
        tmp_right = realloc(tmp_right, split + tsize);
        memcpy(tmp_left + tsize, dst_data[0], split);
        memcpy(tmp_right + tsize, dst_data[1], split);            
        tsize += split;

        *frame_size = tsize;
        *left = tmp_left;
        *right = tmp_right;
    }
}


static int decode_packet(uint8_t** data, int* frame_size)
{
    int ret = 0, dec_ret = 0;
    int decoded = pkt.size;


    if (pkt.stream_index == video_stream_idx) {
        /* decode video frame */
        dec_ret = avcodec_send_packet(video_dec_ctx, &pkt);
        if (dec_ret < 0) {
            fprintf(stderr, "Error decoding video frame (%s)\n", av_err2str(ret));
            return dec_ret;
        }

        while(dec_ret >= 0) {
            dec_ret = avcodec_receive_frame(video_dec_ctx, frame);            
            if(dec_ret == AVERROR(EAGAIN) || dec_ret == AVERROR_EOF) {
                return decoded;
            }
            else if (dec_ret < 0) {
                printf("Error in video decoding \n");
                exit(0);
            }

            if(!image_inited && frame->format != -1) {
                video_dec_ctx->pix_fmt = frame->format;
                pix_fmt = frame->format;
                //ret = av_image_alloc(video_dst_data, video_dst_linesize,width, height, pix_fmt, 1);
                if (ret < 0) {
                    fprintf(stderr, "Could not allocate raw video buffer\n");
                    exit(1);
                }
                video_dst_bufsize = ret;
                image_inited = 1;
                time_base = ((int)(video_dec_ctx->framerate.den) *AV_TIME_BASE) / ( int)(video_dec_ctx->framerate.den);
                
                sws_context = sws_getCachedContext(
                    sws_context,
                    video_dec_ctx->width, video_dec_ctx->height, 
                    pix_fmt,
                    video_dec_ctx->width, video_dec_ctx->height, 
                    AV_PIX_FMT_RGB24,
                    0, NULL, NULL, NULL
                );
            }
           
            if (frame->width != width || frame->height != height || frame->format != pix_fmt) {
                /* To handle this change, one could call av_image_alloc again and
                 * decode the following frames into another rawvideo file. */
                fprintf(stderr, "Error: Width, height and pixel format have to be "
                        "constant in a rawvideo file, but the width, height or "
                        "pixel format of the input video changed:\n"
                        "old: width = %d, height = %d, format = %s\n"
                        "new: width = %d, height = %d, format = %s\n",
                        width, height, av_get_pix_fmt_name(pix_fmt),
                        frame->width, frame->height,
                        av_get_pix_fmt_name(frame->format));
                return -1;
            }

            printf("video_frame n:%d coded_n:%d\n", video_frame_count++, frame->coded_picture_number);



            /* copy decoded frame to destination buffer:
             * this is required since rawvideo expects non aligned data */
            //av_image_copy(video_dst_data, video_dst_linesize,(const uint8_t **)(frame->data), frame->linesize,pix_fmt, width, height);
            const int size = video_dec_ctx->width*video_dec_ctx->height*3;
            uint8_t* buffer = malloc(size);

            const int out_linesize[1] = { 3 * video_dec_ctx->width };
            sws_scale(
                sws_context, 
                frame->data, 
                frame->linesize, 
                0, 
                video_dec_ctx->height, 
                (const uint8_t * const *)&buffer, 
                out_linesize
            );
        
            *data = buffer;
            decoded =  size;
            //fwrite(*data, 1, decoded, video_dst_file);
            //fwrite(video_dst_data[0], 1, video_dst_bufsize, video_dst_file);
        }
    } 

    return decoded;
}

static int open_codec_context(int *stream_idx,
                              AVCodecContext **dec_ctx, AVFormatContext *fmt_ctx, enum AVMediaType type)
{
    int ret, stream_index;
    AVStream *st;
    AVCodec *dec = NULL;
    AVDictionary *opts = NULL;

    ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not find %s stream in input file '%s'\n", av_get_media_type_string(type), 0);
        return ret;
    } else {
        stream_index = ret;
        st = fmt_ctx->streams[stream_index];

        /* find decoder for the stream */
        dec = avcodec_find_decoder(st->codecpar->codec_id);
        if (!dec) {
            fprintf(stderr, "Failed to find %s codec\n",
                    av_get_media_type_string(type));
            return AVERROR(EINVAL);
        }

        /* Allocate a codec context for the decoder */
        *dec_ctx = avcodec_alloc_context3(dec);
        if (!*dec_ctx) {
            fprintf(stderr, "Failed to allocate the %s codec context\n",
                    av_get_media_type_string(type));
            return AVERROR(ENOMEM);
        }

        /* Copy codec parameters from input stream to output codec context */
        if ((ret = avcodec_parameters_to_context(*dec_ctx, st->codecpar)) < 0) {
            fprintf(stderr, "Failed to copy %s codec parameters to decoder context\n",
                    av_get_media_type_string(type));
            return ret;
        }

        /* Init the decoders, with or without reference counting */
        av_dict_set(&opts, "refcounted_frames", refcount ? "1" : "0", 0);
        if ((ret = avcodec_open2(*dec_ctx, dec, &opts)) < 0) {
            fprintf(stderr, "Failed to open %s codec\n",
                    av_get_media_type_string(type));
            return ret;
        }
        *stream_idx = stream_index;
    }

    return 0;
}

static int get_format_from_sample_fmt(const char **fmt,enum AVSampleFormat sample_fmt)
{
    int i;
    struct sample_fmt_entry {
        enum AVSampleFormat sample_fmt; const char *fmt_be, *fmt_le;
    } sample_fmt_entries[] = {
        { AV_SAMPLE_FMT_U8,  "u8",    "u8"    },
        { AV_SAMPLE_FMT_S16, "s16be", "s16le" },
        { AV_SAMPLE_FMT_S32, "s32be", "s32le" },
        { AV_SAMPLE_FMT_FLT, "f32be", "f32le" },
        { AV_SAMPLE_FMT_DBL, "f64be", "f64le" },
    };
    *fmt = NULL;

    for (i = 0; i < FF_ARRAY_ELEMS(sample_fmt_entries); i++) {
        struct sample_fmt_entry *entry = &sample_fmt_entries[i];
        if (sample_fmt == entry->sample_fmt) {
            *fmt = AV_NE(entry->fmt_be, entry->fmt_le);
            return 0;
        }
    }

    fprintf(stderr,
            "sample format %s is not supported as output format\n",
            av_get_sample_fmt_name(sample_fmt));
    return -1;
}

static int read_packet(void *opaque, uint8_t *buf, int buf_size)
{
    struct buffer_data *bd = (struct buffer_data *)opaque;
    buf_size = FFMIN(buf_size, bd->size);

    //fprintf(stderr, "reading: buf->size: %d, buf_size: ", bd->size, buf_size);
    if (!buf_size){
        printf("read the thing\n");
        return AVERROR_EOF;
    }
    
    /* copy internal buffer data to buf */
    memcpy(buf, bd->ptr, buf_size);
    bd->ptr  += buf_size;
    bd->size -= buf_size;


    return buf_size;
}

int set_frame(int t) {
    int flgs = AVSEEK_FLAG_ANY;
    int den = video_dec_ctx->framerate.den * AV_TIME_BASE;
    int num = video_dec_ctx->framerate.num;

    int target = (den/num) * t;

    if(av_seek_frame(fmt_ctx, -1, target, flgs) < 0) {
       return -1;
    }
    
    return 0;
}

uint8_t* get_next_frame(int* size) {
    int ret = 0, idummy;
    do {
        ret = av_read_frame(fmt_ctx, &pkt);
    }while(pkt.stream_index != video_stream_idx && ret >= 0);
    
    if(ret < 0){
        *size = -1;
        return -1;
    }

    uint8_t* data;
    AVPacket orig_pkt = pkt;
    int s = 0;
    do {
        ret = decode_packet(&data, &idummy);
        if (ret < 0)
            break;

        s += ret;
        pkt.data += ret;
        pkt.size -= ret;
    } while (pkt.size > 0);
    av_packet_unref(&orig_pkt);

    *size = video_dec_ctx->width * video_dec_ctx->height *3;
    //*size = s;
    return data;
}

int init_muxer(uint8_t* data, int size, int* video_info) {
    int ret;
    bd.buf = bd.ptr = data;
    bd.total_size = bd.size = size;

    /* open input file, and allocate format context */
    fmt_ctx = avformat_alloc_context();
    if(!fmt_ctx)
        fprintf(stderr, "%d\n", ret);


    const int avio_ctx_buffer_size = 4096;
    uint8_t* avio_ctx_buffer = av_malloc(avio_ctx_buffer_size);
    if (!avio_ctx_buffer) {
        ret = AVERROR(ENOMEM);
        exit(0);
    }

    avio_ctx = avio_alloc_context(avio_ctx_buffer, avio_ctx_buffer_size, 0, &bd, &read_packet, 0, &seek);
    if(!avio_ctx){
        fprintf(stderr, "failed oto init avio_ctx\n");
        exit(1);
    }

    fmt_ctx->pb = avio_ctx;
    fmt_ctx->flags = AVFMT_FLAG_CUSTOM_IO;

    ret = avformat_open_input(&fmt_ctx, NULL, NULL, NULL);
    if ( ret < 0) {
        char buf[128];
        av_strerror(ret, buf, sizeof(buf));
        fprintf(stderr, "Could not open source file %s errorcode: %s\n", 0, buf);
        exit(1);
    }

    av_dump_format(fmt_ctx, 0, "dfgh", 0);
    /* retrieve stream information */
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        exit(1);
    }

    avio_seek(avio_ctx, 0, SEEK_SET);

    if (open_codec_context(&video_stream_idx, &video_dec_ctx, fmt_ctx, AVMEDIA_TYPE_VIDEO) >= 0) {
        video_stream = fmt_ctx->streams[video_stream_idx];

        /* allocate image where the decoded image will be put */
        width = video_dec_ctx->width;
        height = video_dec_ctx->height;
        pix_fmt = video_dec_ctx->pix_fmt;       
    }

    if (open_codec_context(&audio_stream_idx, &audio_dec_ctx, fmt_ctx, AVMEDIA_TYPE_AUDIO) >= 0) {
        audio_stream = fmt_ctx->streams[audio_stream_idx];
    }

    /* dump input information to stderr */
    av_dump_format(fmt_ctx, 0, "-", 0);

    if (!audio_stream && !video_stream) {
        fprintf(stderr, "Could not find audio or video stream in the input, aborting\n");
        ret = 1;
        exit(0);
    }

    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate frame\n");
        ret = AVERROR(ENOMEM);
        exit(0);
    }

    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;

    int s;
    get_next_frame(&s);
    if(set_frame(0) < 0) {
        printf("error reseting frame to 0 \n");
        exit(1);
    }

    video_info[0] = video_dec_ctx->framerate.num;
    video_info[1] = video_dec_ctx->framerate.den;
    
    video_info[2] = video_dec_ctx->width;
    video_info[3] = video_dec_ctx->height;
    video_info[4] = video_dec_ctx->pix_fmt;
    video_info[5] = video_dec_ctx->bit_rate;
} 


uint8_t* extract_audio(int *out_size, int* bitrate) {
    if(!audio_stream) {
        *out_size = -1;
        return -1;
    }
    int ret = 0;
    if(av_seek_frame(fmt_ctx, audio_stream_idx, 0, AVSEEK_FLAG_ANY) < 0) {
        printf("could not set audio frame to 0\n");
        exit(1);
    }
    uint8_t * left_sound_buf = NULL;
    uint8_t * right_sound_buf = NULL;

    int size = 0, total_size = 0;
    while(1) {
        do {
            ret = av_read_frame(fmt_ctx, &pkt);
        }while(pkt.stream_index != audio_stream_idx && ret >= 0);
    
        if(ret < 0){
            break;
        }

        uint8_t* left = NULL;
        uint8_t* right = NULL;
        int s = 0, ts = 0;

        ret = decode_audio(&left, &right, &size);
        if (ret < 0)
            break;

        if(total_size + size > 0 ) {
            left_sound_buf = realloc(left_sound_buf, total_size + size);
            right_sound_buf = realloc(right_sound_buf, total_size + size);

            memcpy(left_sound_buf + total_size, left, size);
            memcpy(right_sound_buf + total_size, right, size);
            total_size += size;

            free(left);
            free(right);
        }
       
        if(size < 0)
            break;

    }

    *bitrate = dst_rate;
    *out_size = total_size;
    return left_sound_buf;
}


void close_muxer() {
    pkt.data = NULL;
    pkt.size = 0;
    int dummy;
    uint8_t* data;

    // TODO GET THESE FRAME TO WHERE THEY BELONG
    decode_packet( &data, &dummy);
    decode_audio(&data, &data, &dummy);

    if (video_stream) {
        printf("Play the output video file with the command:\n"
               "ffplay -f rawvideo -pixel_format rgb24 -video_size %dx%d %s\n",
                video_dec_ctx->width, video_dec_ctx->height,
               "video");
    }

     if (audio_stream) {
        enum AVSampleFormat sfmt = audio_dec_ctx->sample_fmt;
        int n_channels = audio_dec_ctx->channels;
        const char *fmt;

        if (av_sample_fmt_is_planar(dst_sample_fmt)) {
            const char *packed = av_get_sample_fmt_name(sfmt);
            printf("Warning: the sample format the decoder produced is planar "
                   "(%s). This example will output the first channel only.\n",
                   packed ? packed : "?");
            dst_sample_fmt = av_get_packed_sample_fmt(dst_sample_fmt);
            n_channels = 1;
        }

        if ((get_format_from_sample_fmt(&fmt, dst_sample_fmt)) < 0) {
            printf("couldnt retrieve audio format \n");
            exit(1);
        }

        printf("Play the output audio file with the command:\n"
               "ffplay -f %s -ac %d -ar %d %s\n",
               fmt, n_channels, dst_rate,
               "audio");
    }
    avcodec_free_context(&video_dec_ctx);
    avcodec_free_context(&audio_dec_ctx);
    avformat_close_input(&fmt_ctx);

    av_frame_free(&frame);
    av_free(video_dst_data[0]);
    av_free(bd.buf);
    free(dst_data);
}

/*

int main (int argc, char **argv) {
    int ret = 0;
    
    FILE *f = fopen("box.mp4", "rb");
    if(!f) {
        printf("file not found\n");
        exit(1);
    }
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t *buffer = malloc(fsize);
    fread(buffer, fsize, 1, f); 
    fclose(f);

    int *video_info = malloc(5 *sizeof(int));
    init_muxer(buffer, fsize, 1, video_info);
    printf("%d %d %d %d %d %d\n", video_info[0], video_info[1], video_info[2] , video_info[3] ,video_info[4], video_info[5]);
    

    int get_video = 1 , get_audio = 1;

    if(get_video) {
        FILE* vf = fopen("video", "wb");
        // get video frames
        uint8_t* fr;
        int size;
        while(1) {
            fr = get_next_frame(&size);
            if(size < 0)
                break;

            fwrite(fr, 1, size, vf);
            free(fr);
        }

         fclose(vf);
    }
    if(get_audio) {
        FILE* af = fopen("audio", "wb");
        // get audio
        uint8_t* sbuf;
        int s, br; 
        sbuf = extract_audio(&s, &br);   

        printf("size: %d br: %d\n", s, br);
        fwrite(sbuf, 1, s, af);
        fclose(af);
    }
    
    close_muxer();
   
    return ret < 0;
}
*/