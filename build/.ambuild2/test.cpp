
#include <stdio.h>
#include <stdlib.h>

int main()
{
#if defined __ICC
  printf("icc %d\n", __ICC);
#elif defined __clang__
# if defined(__clang_major__) && defined(__clang_minor__)
#  if defined(__apple_build_version__)
    printf("apple-clang %d.%d\n", __clang_major__, __clang_minor__);
#  else   
    printf("clang %d.%d\n", __clang_major__, __clang_minor__);
#  endif
# else
  printf("clang 1.%d\n", __GNUC_MINOR__);
# endif
#elif defined __GNUC__
  printf("gcc %d.%d\n", __GNUC__, __GNUC_MINOR__);
#elif defined _MSC_VER
  printf("msvc %d\n", _MSC_VER);
#elif defined __TenDRA__
  printf("tendra 0\n");
#elif defined __SUNPRO_C
  printf("sun %x\n", __SUNPRO_C);
#elif defined __SUNPRO_CC
  printf("sun %x\n", __SUNPRO_CC);
#else
#error "Unrecognized compiler!"
#endif
#if defined __cplusplus
  printf("CXX\n");
#else
  printf("CC\n");
#endif
  exit(0);
}
