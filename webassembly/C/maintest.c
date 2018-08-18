#include "avio_write.h"
#include "stdio.h"
#include <stdlib.h>
#include <inttypes.h>

#include <time.h>

const int SECONDS = 2;

//DUMMY VIDEO
const int FPS = 30;
const int WIDTH = 1280;
const int HEIGHT = 720;
const int BIT_RATE = 400000; 
const int NR_CLS = 4;

//DUMMY AUDIO
const int SAMPLE_RATE = 44100;
const int CHANNELS = 2;

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
    double seconds = 10;

    open_video(WIDTH,HEIGHT,FPS,BIT_RATE, 4);

    /*
    int leftSize, rightSize;
    float* left = get_audio_buf("right1.raw", &leftSize);
    float* right = get_audio_buf("left1.raw", &rightSize);
    open_audio( left, right, leftSize, 44100, 2, 320000 );
    seconds = (leftSize+rightSize) / (double) (44100 * 2);
    */

    write_header();

    clock_t start = clock(), diff;
    for(i = 0;i < (int)20*FPS; i++){
        uint8_t* buffer = malloc(WIDTH*HEIGHT*NR_CLS * 10);
        for(j = 0; j < WIDTH*HEIGHT*NR_CLS; j++){
        buffer[j] = 0;
        //Spoof red at half the screen
        
        if(j < (WIDTH*HEIGHT*NR_CLS)/2)
            buffer[j] = !(j % NR_CLS) ? 255 : 0;
        }
        add_frame(buffer, 1);
    }
    diff = clock() - start;
    int msec = diff * 1000 / CLOCKS_PER_SEC;
    printf("Time taken %d seconds %d milliseconds\n", msec/1000, msec%1000);

    //write_audio_frame();

    int size = close_stream();
    uint8_t* out = get_buffer();
    
    FILE* out_file = fopen("fi1.mp4", "w");
    fwrite(out, size, 1, out_file);
    fclose(out_file);
    free_buffer();
    return 0;
}