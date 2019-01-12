/*
 * minigxx_nortti.cc: minimal C++ library for gcc -fno-rtti -fno-exceptions
 * by pts@fazekas.hu at Sat Jan 12 18:03:57 CET 2019
 *
 * To use this library, add it to your `gcc -fno-rtti -fno-exceptions'
 * command-line. See also README.txt for better options: with support for
 * -frtti, and making smaller executables by omitting unneeded features.
 *
 * Tested with gcc-4.4 and gcc-4.8.
 */
#include <stdlib.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

static void* emulate_cc_new(size_t len) {
  void *p = malloc(len);
  if (p == 0) {
    /* Don't use stdio (e.g. fputs), because that may want to allocate more
     * memory.
     */
    (void)!write(2, "out of memory\n", 14);
    abort();
  }
  return p;
}

static void emulate_cc_delete(void* p) {
  free(p);
}

void* _Znwm /*operator new   amd64*/(size_t len) __attribute__((alias("emulate_cc_new")));
void* _Znam /*operator new[] amd64*/(size_t len) __attribute__((alias("emulate_cc_new")));
void* _Znwj /*operator new   i386*/(size_t len) __attribute__((alias("emulate_cc_new")));
void* _Znaj /*operator new[] i386*/(size_t len) __attribute__((alias("emulate_cc_new")));
void  _ZdlPv/*operator delete  */(void* p) __attribute__((alias("emulate_cc_delete")));
void  _ZdaPv/*operator delete[]*/(void* p) __attribute__((alias("emulate_cc_delete")));
void  _ZdlPvm/*operator delete   amd64*/(void* p, unsigned long) __attribute__((alias("emulate_cc_delete")));
void  _ZdaPvm/*operator delete[] amd64*/(void* p, unsigned long) __attribute__((alias("emulate_cc_delete")));
void  _ZdlPvj/*operator delete   i386*/(void* p, unsigned int) __attribute__((alias("emulate_cc_delete")));
void  _ZdaPvj/*operator delete[] i386*/(void* p, unsigned int) __attribute__((alias("emulate_cc_delete")));

/* See https://libcxxabi.llvm.org/spec.html */
__attribute__((noreturn)) void __cxa_pure_virtual(void) {
  (void)!write(2, "pure virtual method called\n", 27);
  abort();
}

/* No need to implement __cxa_atexit, it's part of glibc and xstatic uClibc as
 * well.
 */

#ifdef __cplusplus
}  /* extern "C" */
#endif
