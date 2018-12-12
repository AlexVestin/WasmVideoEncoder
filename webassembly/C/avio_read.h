
#include <stdint.h>

int init_muxer(uint8_t* data, int size, int* video_info);
void close_muxer();
uint8_t* extract_audio(int *out_size, int* bitrate);
int get_next(uint8_t** left, uint8_t** right, uint8_t** img, int* size, int* type);
