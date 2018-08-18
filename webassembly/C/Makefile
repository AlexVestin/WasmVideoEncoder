
FLAGS = `pkg-config --libs libavcodec libavformat libswscale libswresample vpx opus x264` \

FILES = avio_write.c avio_write.h \
	-I../build/reg/include \
	-L../build/reg/lib \

main:
	gcc -g maintest.c $(FILES) $(FLAGS) 

mult:
	gcc -g testmultiple.c $(FILES) $(FLAGS) 