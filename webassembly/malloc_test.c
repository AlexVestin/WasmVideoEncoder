
#include <stdio.h>
#include <stdlib.h>
int test_malloc() {
  void* buf0 = malloc(1000*1000* 10);
  if(buf0) {
    printf("10MB\n");
    free(buf0);
  } else {
    return 0;
  }
  void* buf1 = malloc(1000*1000* 100);
  if(buf1) {
    printf("100MB\n");
    free(buf1);
  } else {
    return 1;
  }

  void* buf2 = malloc(1000*1000* 500);
  if(buf2) {
    printf("500MB\n");
    free(buf2);
  } else {
    return 2;
  }

  void* buf3 = malloc(1000*1000* 500);
  if(buf3) {
    printf("500MB\n");
    free(buf3);
  } else {
    return 3;
  }
  printf("done\n");
  return 4;
}