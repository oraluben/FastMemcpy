//=====================================================================
//
// FastMemcpy.c - skywind3000@163.com, 2015
//
// feature:
// 50% speed up in avg. vs standard memcpy (tested in vc2012/gcc4.9)
//
//=====================================================================
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


#ifndef SRC_ALIGNED
#define SRC_ALIGNED 1
#endif
#ifndef DST_ALIGNED
#define DST_ALIGNED 1
#endif

#ifndef BATCH
#define BATCH 1024
#endif

#ifndef TIMES
#define TIMES 0x10000000 / BATCH * 64
#endif

#if (defined(_WIN32) || defined(WIN32))
#include <windows.h>
#include <mmsystem.h>
#ifdef _MSC_VER
#pragma comment(lib, "winmm.lib")
#endif
#elif defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
#include <sys/time.h>
#include <unistd.h>
#else
#error it can only be compiled under windows or unix
#endif

#ifdef HAS_SSE2
  #include "FastMemcpy_Sse2.h"
#elif HAS_AVX
  #include "FastMemcpy_Avx.h"
#else
  #define memcpy_fast memcpy
#endif


#define FETCH(dst, n)                          \
    do                                         \
    {                                          \
        for (size_t __i = 0; __i < (n); __i++) \
        {                                      \
            (dst)[__i] += 0;                   \
        }                                      \
    } while (0)


unsigned int gettime()
{
	#if (defined(_WIN32) || defined(WIN32))
	return timeGetTime();
	#else
	static struct timezone tz={ 0,0 };
	struct timeval time;
	gettimeofday(&time,&tz);
	return (time.tv_sec * 1000 + time.tv_usec / 1000);
	#endif
}

void sleepms(unsigned int millisec)
{
#if defined(_WIN32) || defined(WIN32)
	Sleep(millisec);
#else
	usleep(millisec * 1000);
#endif
}


void benchmark(int dstalign, int srcalign, size_t size, int times)
{
	char *DATA1 = (char*)malloc(size + 64);
	char *DATA2 = (char*)malloc(size + 64);
	size_t LINEAR1 = ((size_t)DATA1);
	size_t LINEAR2 = ((size_t)DATA2);
	char *ALIGN1 = (char*)(((64 - (LINEAR1 & 63)) & 63) + LINEAR1);
	char *ALIGN2 = (char*)(((64 - (LINEAR2 & 63)) & 63) + LINEAR2);
	char *dst = (dstalign)? ALIGN1 : (ALIGN1 + 1);
	char *src = (srcalign)? ALIGN2 : (ALIGN2 + 3);
	unsigned int t1;
	int k;
	
	sleepms(100);
	t1 = gettime();
	for (k = times; k > 0; k--) {
		memcpy_fast(dst, src, size);
	}
	t1 = gettime() - t1;

	FETCH(dst, size);

	free(DATA1);
	free(DATA2);

	printf("dst %s, src %s, %dms\n",  
		dstalign? "aligned" : "unalign", 
		srcalign? "aligned" : "unalign", (int)t1);
}


void bench(int copysize, int times)
{
	printf("benchmark(size=%d bytes, times=%d):\n", copysize, times);
	benchmark(DST_ALIGNED, SRC_ALIGNED, copysize, times);
	printf("\n");
}


void random_bench(int maxsize, int times)
{
	static char A[11 * 1024 * 1024 + 2];
	static char B[11 * 1024 * 1024 + 2];
	static int random_offsets[0x10000];
	static int random_sizes[0x8000];
	unsigned int i, p1, p2;
	unsigned int t1;
	for (i = 0; i < 0x10000; i++) {	// generate random offsets
		random_offsets[i] = rand() % (10 * 1024 * 1024 + 1);
	}
	for (i = 0; i < 0x8000; i++) {	// generate random sizes
		random_sizes[i] = 1 + rand() % maxsize;
	}
	sleepms(100);
	t1 = gettime();
	for (p1 = 0, p2 = 0, i = 0; i < times; i++) {
		int offset1 = random_offsets[(p1++) & 0xffff];
		int offset2 = random_offsets[(p1++) & 0xffff];
		int size = random_sizes[(p2++) & 0x7fff];
		memcpy_fast(A + offset1, B + offset2, size);
	}
	t1 = gettime() - t1;
	printf("benchmark random access:\n");
	printf("%d", (int)t1);
}


#ifdef _MSC_VER
#pragma comment(lib, "winmm.lib")
#endif

int main(void)
{
	bench(BATCH, TIMES);

	// random_bench(2048, 8000000);

	return 0;
}
