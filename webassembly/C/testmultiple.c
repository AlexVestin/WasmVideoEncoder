#include "avio_write.h"
#include "stdio.h"
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>

const int SECONDS = 4;

//DUMMY VIDEO
const int FPS = 25;
const int WIDTH = 720;
const int HEIGHT = 480;
const int BIT_RATE = 400000; 
const int NR_CLS = 4;
const int PRESET = 3;

//DUMMY AUDIO
const int SAMPLE_RATE = 44100;
const int CHANNELS = 2;

const int fmt_type = 1;

float* get_audio_buf(const char* filename, int *fs){
    FILE* f = fopen(filename, "rb");
    
    fseek(f, 0, SEEK_END);
    int size = ftell(f) / sizeof(float);
    fseek(f, 0, SEEK_SET);

    float* buf = malloc(sizeof(float) * size);
    int bytes_read = fread(buf, sizeof(float), size, f);
    if(bytes_read != size){
        printf("ERROR READING FILE\n");
        exit(1);
    }
    *fs = size;
    fclose(f);
    return buf;   
}

int main(int argc, char** argv) {
    int i, j, audio_size;

    open_video(WIDTH,HEIGHT,FPS,BIT_RATE, PRESET, fmt_type, fmt_type);

    int leftSize, rightSize;
    float* left = get_audio_buf("./assets/right1.raw", &leftSize);
    float* right = get_audio_buf("./assets/left1.raw", &rightSize);
    const int audio_frame_size = (1.0 / (float)FPS) * SAMPLE_RATE;
    open_audio( 44100, 2, 128000, fmt_type );
        
    write_header();

    int added_audio = 0;

    clock_t start = clock(), diff;
    int k, frame_size = WIDTH*HEIGHT*NR_CLS;
    for(i = 0;i < SECONDS*FPS; i++){
        uint8_t* buffer = malloc(frame_size);
        for(j = 0; j < frame_size; j++){
            buffer[j] = 0;

            //red at half the screen
            if(j < (frame_size / 2))
                buffer[j] = !(j % NR_CLS) ? 255 : 0;
        }
        

        add_video_frame(buffer);
        //add_audio_frame(left + added_audio, right + added_audio, audio_frame_size );
        //added_audio += audio_frame_size;
    }

    add_audio_frame(left + added_audio, right + added_audio, leftSize );
    diff = clock() - start;
    int msec = diff * 1000 / CLOCKS_PER_SEC;
    printf("--\n");
    printf("Time taken %d seconds %d milliseconds\n", msec/1000, msec%1000);
    printf("--\n");

    int size;
    uint8_t* out = close_stream(&size);
    
    FILE* out_file = fopen( fmt_type == 0 ? "fil.webm" : "fil.mp4", "wb");
    fwrite(out, size, 1, out_file);
    fclose(out_file);
    free_buffer();
    
    free(left);
    free(right);
    return 0;
}