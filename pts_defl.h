/*
 * pts_defl.h -- a compact deflate compressor (not uncompressor) implementation
 * compiled by pts@fazekas.hu at Sun Mar  3 23:23:14 CET 2002
 *
 * algorithm ripped from Info-ZIP 2.2, implementation and (C):
 *
 * Copyright (C) 1990-1997 Mark Adler, Richard B. Wales, Jean-loup Gailly,
 * Kai Uwe Rommel, Onno van der Linden and Igor Mandrichenko.
 * Permission is granted to any individual or institution to use, copy, or
 * redistribute this software so long as all of the original files are included,
 * that it is not sold for profit, and that this copyright notice is retained.
 *
 * Howto
 * ~~~~~
 * Note that the interface has been changed (pts_deflate_init -> pts_defl_new)
 * between sam2p versions 0.36 and 0.37. This howto describes the old interface,
 * and that is no longer relevant.
 *
 * 1. The user should #include <pts_defl.h>
 * 2. The user should define a callback function (of type pts_defl_zpfwrite_t) that
 *    will be called for each compressed data chunk that is about to be
 *    written. A non-zero return value should indicate error. Example:
 *
 *      static int my_fwrite(char *block, unsigned len, void *zfile) {
 *        fwrite(block, 1, len, (FILE*)zfile);
 *        return ferror((FILE*)zfile) ? -1 : 0;
 *      }
 *
 * 3. The user should choose a compression level: 0..9 (9==strongest,slowest)
 * 4. The user should initialize a pts_defl_interface structure. Example:
 *
 *      struct pts_defl_interface fs;
 *      fs.zpfwrite=my_fwrite;
 *      fs.zpfmalloc=my_fmalloc;
 *      fs.zpffree=my_free;
 *      fs.pack_level=5;
 *      fs.zfile=(void*)stdout;
 *
 * 5. The user should call pts_deflate_init() which fills fs.deflate2 and
 *    initializes several other fields:
 *
 *      pts_deflate_init(&fs);
 *
 * 6. The user should supply the uncompressed input. Example:
 *
 *      char inbuf[999];
 *      int size;
 *      while (0<(size=fread(inbuf, 1, sizeof(inbuf), fin)))
 *        fs.deflate2(inbuf,size,fs);
 *
 * 7. The user should signal the end-of-input, so the library would flush
 *    output and free resources:
 *
 *      fs.deflate2(0,0,&fs);
 *
 * 8. The current library implementation (but not the interface!) supports
 *    only one (1) open compressor in the same time.
 * 9. Error handling is not customizable yet.
 */

#ifndef PTS_DEFL_H
#define PTS_DEFL_H 1

#ifdef __GNUC__
#pragma interface
#endif

/**** pts ****/
#ifndef ___
#if (defined(__PROTOTYPES__) || defined(__STDC__) || defined(__cplusplus)) && !defined(NO_PROTO)
# define _(args) args
# define OF(args) args
# define ___(arg2s,arg1s,argafter) arg2s /* Dat: no direct comma allowed in args :-( */
#else
# define _(args) ()
# define OF(args) ()
# define ___(arg2s,arg1s,argafter) arg1s argafter /* Dat: no direct comma allowed in args :-( */
#endif
#endif
struct pts_defl_interface;
/* @return -1 on error, 0 on success */
typedef int  (*pts_defl_zpfwrite_t) OF((char *block, unsigned len, void *zfile));
typedef void (*pts_defl_deflate2_t) OF((char *,unsigned,struct pts_defl_interface*));
/** Maximum memory chunk allocated by pts_defl.c is 65535 bytes */
typedef void* (*pts_defl_malloc_t) OF((unsigned));
typedef void (*pts_defl_free_t) OF((void*));
typedef void (*pts_defl_delete2_t) OF((struct pts_defl_interface*));
struct pts_defl_internal_state;
struct pts_defl_interface {
  /* Parameters initialized before pts_deflate_init() */
  pts_defl_zpfwrite_t zpfwrite;
  pts_defl_malloc_t   zpfmalloc;
  pts_defl_free_t     zpffree;
  int pack_level; /* 1..9 */
  void *zfile;

  /* Parameters initialized by pts_deflate_init() */
  /* void *other; */
  pts_defl_deflate2_t deflate2;
  pts_defl_delete2_t  delete2;
  struct pts_defl_internal_state *ds;
  unsigned err;
};

/* old interface: void pts_deflate_init OF((struct pts_defl_interface*)); */

/** @return NULL on fatal out-of-memory; a valid interface otherwise */
#ifdef __cplusplus
extern "C"
#endif
struct pts_defl_interface* pts_defl_new OF((
  pts_defl_zpfwrite_t zpfwrite_,
  pts_defl_malloc_t   zpfmalloc_,
  pts_defl_free_t     zpffree_,
  int pack_level_,
  void *zfile_
));

/* EXTERN_C void pts_defl_delete OF((pts_defl_interface*)); */

#endif
