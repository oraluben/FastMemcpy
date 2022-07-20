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
#elif HAS_NEON
  #include "FastMemcpy_Base.h"
void memcpy_fast(void* dstin, void* src, size_t count) {
    __asm__(
"	add	x4, %[src], %[count]\n"
"	add	x5, %[dstin], %[count]\n"
"	cmp	%[count], 128\n"
"	b.hi	.Lcopy_long\n"
"	cmp	%[count], 32\n"
"	b.hi	.Lcopy32_128\n"

    /* Small copies: 0..32 bytes.  */
"	cmp	%[count], 16\n"
"	b.hi	.Lcopy16\n"
"	ldr	q0, [%[src]]\n"
" 	ldr	q1, [x4, -16]\n"
" 	str	q0, [%[dstin]]\n"
" 	str	q1, [x5, -16]\n"
" 	ret\n"

	/* Copy 8-15 bytes.  */
" .Lcopy16:\n"
" 	tbz	%[count], 3, .Lcopy8\n"
" 	ldr	x6, [%[src]]\n"
" 	ldr	x7, [x4, -8]\n"
" 	str	x6, [%[dstin]]\n"
" 	str	x7, [x5, -8]\n"
" 	ret\n"

	/* Copy 4-7 bytes.  */
" .Lcopy8:\n"
" 	tbz	%[count], 2, .Lcopy4\n"
" 	ldr	w6, [%[src]]\n"
" 	ldr	w8, [x4, -4]\n"
" 	str	w6, [%[dstin]]\n"
" 	str	w8, [x5, -4]\n"
" 	ret\n"

	/* Copy 0..3 bytes using a branchless sequence.  */
" .Lcopy4:\n"
" 	cbz	%[count], .Lcopy0\n"
" 	lsr	x14, %[count], 1\n"
" 	ldrb	w6, [%[src]]\n"
" 	ldrb	w10, [x4, -1]\n"
" 	ldrb	w8, [%[src], x14]\n"
" 	strb	w6, [%[dstin]]\n"
" 	strb	w8, [%[dstin], x14]\n"
" 	strb	w10, [x5, -1]\n"
" .Lcopy0:\n"
" 	ret\n"

" 	.p2align 4\n"
	/* Medium copies: 33..128 bytes.  */
" .Lcopy32_128:\n"
" 	ldp	q0, q1, [%[src]]\n"
" 	ldp	q2, q3, [x4, -32]\n"
" 	cmp	%[count], 64\n"
" 	b.hi	.Lcopy128\n"
" 	stp	q0, q1, [%[dstin]]\n"
" 	stp	q2, q3, [x5, -32]\n"
" 	ret\n"

" 	.p2align 4\n"
	/* Copy 65..128 bytes.  */
" .Lcopy128:\n"
" 	ldp	q4, q5, [%[src], 32]\n"
" 	cmp	%[count], 96\n"
" 	b.ls	.Lcopy96\n"
" 	ldp	q6, q7, [x4, -64]\n"
" 	stp	q6, q7, [x5, -64]\n"
" .Lcopy96:\n"
" 	stp	q0, q1, [%[dstin]]\n"
" 	stp	q4, q5, [%[dstin], 32]\n"
" 	stp	q2, q3, [x5, -32]\n"
" 	ret\n"

	/* Align loop64 below to 16 bytes.  */
" 	nop\n"

	/* Copy more than 128 bytes.  */
" .Lcopy_long:\n"
	/* Copy 16 bytes and then align src to 16-byte alignment.  */
" 	ldr	q3, [%[src]]\n"
" 	and	x14, %[src], 15\n"
" 	bic	%[src], %[src], 15\n"
" 	sub	x3, %[dstin], x14\n"
" 	add	%[count], %[count], x14	/* %[count] is now 16 too large.  */\n"
" 	ldp	q0, q1, [%[src], 16]\n"
" 	str	q3, [%[dstin]]\n"
" 	ldp	q2, q3, [%[src], 48]\n"
" 	subs	%[count], %[count], 128 + 16	/* Test and readjust %[count].  */\n"
" 	b.ls	.Lcopy64_from_end\n"
" .Lloop64:\n"
" 	stp	q0, q1, [x3, 16]\n"
" 	ldp	q0, q1, [%[src], 80]\n"
" 	stp	q2, q3, [x3, 48]\n"
" 	ldp	q2, q3, [%[src], 112]\n"
" 	add	%[src], %[src], 64\n"
" 	add	x3, x3, 64\n"
" 	subs	%[count], %[count], 64\n"
" 	b.hi	.Lloop64\n"

	/* Write the last iteration and copy 64 bytes from the end.  */
" .Lcopy64_from_end:\n"
" 	ldp	q4, q5, [x4, -64]\n"
" 	stp	q0, q1, [x3, 16]\n"
" 	ldp	q0, q1, [x4, -32]\n"
" 	stp	q2, q3, [x3, 48]\n"
" 	stp	q4, q5, [x5, -64]\n"
" 	stp	q0, q1, [x5, -32]\n"
" 	ret"
:
: [dstin] "r" (dstin), [src] "r" (src), [count] "r" (count)
: "%x4", "%x5", "%x6", "%x7", "%x8", "%x9", "%x14",
  "%w6", "%w8", "%w10",
  "%q0", "%q1", "%q2", "%q3", "%q4", "%q5", "%q6", "%q7"
    );
}
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
