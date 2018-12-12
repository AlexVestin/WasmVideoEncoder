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
    while(ret > 0) {
        uint8_t* left;
        uint8_t* right;
        uint8_t* img;
        ret = get_next(&left, &right, &img, &frame_size, &type);

        if(ret == -11) {
            ret = 1;
        }else {
             if(type == 0) {
                free(img);
            }else {
                free(left);
                free(right);
            }
        }

        printf("vczxcxcsf\n");
    }

    close_muxer();

    return 0;
}