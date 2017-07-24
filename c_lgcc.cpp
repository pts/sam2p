/* c_lgcc.cpp -- make C++ programs linkable with gcc
 * by pts@fazekas.hu at Tue Sep  3 18:31:58 CEST 2002
 *
 * This file is not sam2p-specific. See also AC_PTS_GCC_LINKS_CXX in file
 * aclocal.m4.
 */

#include <stdlib.h>
#include <unistd.h>  /* for write(), also available on Windows */
#include <sys/types.h>  /* size_t */

/* Sat Jul  6 16:39:19 CEST 2002
 * empirical checkerg++ helper routines for gcc version 2.95.2 20000220 (Debian GNU/Linux)
 */
#ifdef __CHECKER__
void* __builtin_vec_new XMALLOC_CODE()
void  __builtin_vec_delete XFREE_CODE()
void* __builtin_new XMALLOC_CODE()
void  __builtin_delete XFREE_CODE()
void  __rtti_user() { abort(); }
void  __rtti_si() { abort(); }
void  terminate() { abort(); }
/* void* __pure_virtual=0; -- doesn't work */
extern "C" void __pure_virtual(); void __pure_virtual() { abort(); }

#else

/* at Tue Sep  3 18:24:26 CEST 2002:
 *   empirical g++-3.2 helper routines for gcc version 3.2.1 20020830 (Debian prerelease)
 * at Sun Dec 19 19:25:31 CET 2010:
 *   works for g++-4.2.1 and g++-4.4.1 as well
 *   removed dependency on stdio, so we get more reliable OOM reporting
 */
static void* emulate_cc_new(unsigned len) { \
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
  if (p!=0) free(p);
}
#if USE_ATTRIBUTE_ALIAS
void* operator new  (size_t len) __attribute__((alias("emulate_cc_new")));
void* operator new[](size_t len) __attribute__((alias("emulate_cc_new")));
void  operator delete  (void* p)   __attribute__((alias("emulate_cc_delete")));
void  operator delete[](void* p)   __attribute__((alias("emulate_cc_delete")));
#else  /* Darwin o32-clang doesn't have it. */
void* operator new  (size_t len) { return emulate_cc_new(len); }
void* operator new[](size_t len) { return emulate_cc_new(len); }
void  operator delete  (void* p) { return emulate_cc_delete(p); }
void  operator delete[](void* p) { return emulate_cc_delete(p); }
#endif

void* __cxa_pure_virtual = 0;
#if 0  /* Needed only if this is not used: -fno-use-cxa-atexit */
extern "C" void __dso_handle();
void __dso_handle() {}  /* Defined in crtbeginT.o for xstatic. */
#endif

#endif
