
#include <stdint.h>

void write_audio_frame();
void free_buffer();
void open_video(int, int, int, int, int, int, int); 
void add_audio_frame(float*, float*, int);
void open_audio_pre(float*, float*, int);
void open_audio( int, int, int, int);

void write_header();
uint8_t* close_stream();
void add_video_frame(uint8_t*);
