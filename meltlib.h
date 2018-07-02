#ifndef MELTLIB_H_
#define MELTLIB_H_

// TSX support

#define _RTM_H 1

#define _XBEGIN_STARTED		(~0u)

#define __rtm_force_inline __attribute__((__always_inline__)) inline

static __rtm_force_inline int _xbegin(void)
{// reference: https://github.com/defuse/flush-reload-attacks
	int ret = _XBEGIN_STARTED;
	asm volatile(".byte 0xc7,0xf8 ; .long 0" : "+a" (ret) :: "memory");
	return ret;
}

static __rtm_force_inline void _xend(void)
{// reference: https://github.com/defuse/flush-reload-attacks
	 asm volatile(".byte 0x0f,0x01,0xd5" ::: "memory");
}


#define _xabort(status) \
	asm volatile(".byte 0xc6,0xf8,%P0" :: "i" (status) : "memory")

static __rtm_force_inline int _xtest(void)
{
	unsigned char out;
	asm volatile(".byte 0x0f,0x01,0xd6 ; setnz %0" : "=r" (out) :: "memory");
	return out;
}


__attribute__((always_inline))
inline void flush(const char *adrs)
{
  asm __volatile__ (
     "mfence         \n"
     "clflush 0(%0)  \n"
     :
     : "r" (adrs)
     :
  );
}

__attribute__((always_inline))
inline unsigned long measure_time(const char *addr)
{// reference: https://github.com/defuse/flush-reload-attacks
  volatile unsigned long time;

  asm __volatile__ (
    "mfence             \n"
    "lfence             \n"
    "rdtsc              \n"
    "lfence             \n"
    "movl %%eax, %%esi  \n"
    "movl (%1), %%eax   \n"
    "lfence             \n"
    "rdtsc              \n"
    "subl %%esi, %%eax  \n"
    "clflush 0(%1)      \n"
    : "=a" (time)
    : "c" (addr)
    :  "%esi", "%edx");

  return time;
}

#endif