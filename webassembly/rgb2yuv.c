#include <inttypes.h>
#include <stddef.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#define NR_COLORS 3
#define NR_THREADS 4


void rgb2yuv420p_seq(uint8_t *destination, uint8_t *rgb, size_t width, size_t height)
{

    size_t image_size = width * height;
    size_t upos = image_size;
    size_t vpos = upos + upos / 4;
    size_t i = 0;
    uint8_t r, g, b;

    size_t idx;

    for( size_t line = 0; line < height; ++line ) {
        if(!(line % 180)) {
            //printf("sequential line: %d, i: %d upos %d vpos %d\n", line, i, upos, vpos);
        }
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



typedef struct _thread_info {
    uint8_t* dst;
    uint8_t* src;   
    int rank;
    int width;
    int height;
} thread_info;


void* rgb2yuv420p(void* tt)
{
    thread_info* ti = (thread_info*)tt;
    size_t max_vpos_increment = (ti->height*ti->width)/4;
    size_t max_i_increment = (ti->height*ti->width);
    
    size_t part_vpos = max_vpos_increment / NR_THREADS;
    size_t part_i = max_i_increment / NR_THREADS;
    size_t part_height = ti->height / NR_THREADS;



    size_t image_size = ti->width * ti->height;
    size_t upos = image_size + ti->rank*part_vpos;
    size_t vpos = image_size + image_size / 4 + ti->rank*part_vpos;
    size_t i = part_i * ti->rank;
    uint8_t r, g, b;

    size_t idx;

    //printf("parallel line %d i: %d upos: %d, vpos: %d \n", part_height*ti->rank, i, upos, vpos);
    //printf("part_height: %d, start: %d end: %d\n", part_height, part_height*rank, part_height*(rank+1));

    for( size_t line = part_height*ti->rank; line < part_height*(ti->rank+1); ++line ) {
        if( !(line % 2) ) {
            for( size_t x = 0; x < ti->width; x += 2 )
            {
                r = ti->src[NR_COLORS * i];
                g = ti->src[NR_COLORS * i + 1];
                b = ti->src[NR_COLORS * i + 2];

        
                ti->dst[i++] = ((66*r + 129*g + 25*b) >> 8) + 16;

                ti->dst[upos++] = ((-38*r + -74*g + 112*b) >> 8) + 128;
                ti->dst[vpos++] = ((112*r + -94*g + -18*b) >> 8) + 128;

                r = ti->src[NR_COLORS * i];
                g = ti->src[NR_COLORS * i + 1];
                b = ti->src[NR_COLORS * i + 2];

                ti->dst[i++] = ((66*r + 129*g + 25*b) >> 8) + 16;
            }
        }
        else
        {
            for( size_t x = 0; x < ti->width; x += 1 )
            {
                r = ti->src[NR_COLORS * i];
                g = ti->src[NR_COLORS * i + 1];
                b = ti->src[NR_COLORS * i + 2];

                ti->dst[i++] = ((66*r + 129*g + 25*b) >> 8) + 16;
            }
        }
    }  
}

pthread_t* threads;


void test_thing() {
    struct timespec stime, etime;

    int width = 1280;
    int height = 720;

    pthread_t threads[NR_THREADS];
    thread_info infos[NR_THREADS];

    
    uint8_t* src = (uint8_t*)malloc(width*height*NR_COLORS);
    int size = (width * height * NR_COLORS) / 2;
    uint8_t* dst = (uint8_t*)malloc(size);
    uint8_t* dst_seq = (uint8_t*)malloc(size);

    for (int i = 0; i < NR_THREADS; i++) {
        infos[i] = (thread_info){dst, src, i, width, height};
    }

    for(int i = 0; i < width*height*NR_COLORS; i++) {
        src[i] = i % 255;
    }

    clock_gettime(CLOCK_REALTIME, &stime);
    
    for(int i = 0; i < NR_THREADS; i++) {
        pthread_t t;
        pthread_create(&t, NULL, rgb2yuv420p, &infos[i]);
        threads[i] = t;
    }


    for(int i = 0; i < NR_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

  

    clock_gettime(CLOCK_REALTIME, &etime);


    printf("time parallel: %g\n", (etime.tv_sec  - stime.tv_sec) + 1e-9*(etime.tv_nsec  - stime.tv_nsec));
    
	clock_gettime(CLOCK_REALTIME, &stime);
    for(int j = 0; j < 1; j++) {
        rgb2yuv420p_seq(dst_seq, src, width, height);
    }
    clock_gettime(CLOCK_REALTIME, &etime);
    printf("time sequential: %g\n", (etime.tv_sec  - stime.tv_sec) + 1e-9*(etime.tv_nsec  - stime.tv_nsec));


    printf("Done: \n");
    for(int i = 0; i < size; i++) {
        if(dst_seq[i] != dst[i]) {
            printf("mismatch: seq %d parallel: %d i: %d\n",dst_seq[i], dst[i], i);
        }
        
    }
    

}