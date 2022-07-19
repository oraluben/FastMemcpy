#ifndef INLINE
#ifdef __GNUC__
#if (__GNUC__ <= 3)
#error "Require gcc >= 4"
#else
#define INLINE __inline__ __attribute__((always_inline))
#endif
#elif defined(_MSC_VER)
#define INLINE __forceinline
#elif (defined(__BORLANDC__) || defined(__WATCOMC__))
#define INLINE __inline
#else
#define INLINE
#endif
#endif
