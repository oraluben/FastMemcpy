raw:
	gcc -O3 FastMemcpy.c -o out/FastMemcpy
	./out/FastMemcpy

sse2:
	gcc -O3 -msse2 -DHAS_SSE2 FastMemcpy.c -o out/FastMemcpy_Sse2
	./out/FastMemcpy_Sse2

avx:
	gcc -O3 -mavx -DHAS_AVX FastMemcpy.c -o out/FastMemcpy_Avx
	./out/FastMemcpy_Avx
