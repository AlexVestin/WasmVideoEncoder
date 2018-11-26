
ENCODE_FLAGS = `pkg-config --libs libavcodec libavformat libswscale libswresample vpx opus x264`
DECODE_FLAGS = `pkg-config --libs libavcodec libavformat libswscale libswresample` 


ENCODE_FILES = avio_write.c avio_write.h \
	-I../build/reg/include \
	-L../build/reg/lib \

DECODE_FILES = avio_read.c avio_read.h \
	-I../build/reg/include \
	-L../build/reg/lib \


encode:
	gcc -g encodetest.c $(ENCODE_FILES) $(ENCODE_FLAGS) 

decode:
	gcc -g decodetest.c $(DECODE_FILES) $(DECODE_FLAGS) 