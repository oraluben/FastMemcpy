raw:
	gcc $(CFLAGS) -O3 FastMemcpy.c -o out/FastMemcpy
	./out/FastMemcpy

sse2:
	gcc $(CFLAGS) -O3 -msse2 -DHAS_SSE2 FastMemcpy.c -o out/FastMemcpy_Sse2
	./out/FastMemcpy_Sse2

avx:
	gcc $(CFLAGS) -O3 -mavx -DHAS_AVX FastMemcpy.c -o out/FastMemcpy_Avx
	./out/FastMemcpy_Avx

neon:
	gcc $(CFLAGS) -O3 -DHAS_NEON FastMemcpy.c -o out/FastMemcpy_Neon
	./out/FastMemcpy_Neon
