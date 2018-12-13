#include "stdio.h"
#include "avio_read.h"
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>


int main(int argc, char** argv) {    

    FILE* f = fopen("./assets/big2.mp4", "rb");
    fseek(f, 0, SEEK_END);
    int size = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t* buf = malloc(size);
    int bytes_read = fread(buf, sizeof(uint8_t), size, f);
    fclose(f);
    int* info = malloc(10 * sizeof(int));
    int a = init_muxer(buf, size, info);


    int frame_size = 0, type = 0;
   
    int ret = 1;
    FILE* audio_f = fopen("audio.raw", "wb");
    FILE* video_f = fopen("video.raw", "wb");

    int count = 0;
    while(1) {
        count++;
        if(count > 600)
            break;
        uint8_t* left;
        uint8_t* right;
        uint8_t* img;
        ret = get_next(&left, &right, &img, &frame_size, &type);
        if(ret < 0) {
            break;
        }

        if(type == 0) {
            fwrite(img, 1, frame_size, video_f);
            free(img);
        }else {
            fwrite(left, 1, frame_size, audio_f);
            free(left);
            free(right);
        }
    }
    fclose(audio_f);
    fclose(video_f);
    close_muxer();

    return 0;
}