/*
 * pts_defl.c -- a compact deflate compressor (not uncompressor) implementation
 * compiled by pts@fazekas.hu at Sun Mar  3 23:23:14 CET 2002
 * reentrant library version by pts@fazekas.hu
 *   started at Thu Jul  4 19:04:39 CEST 2002
 *   milestone at Sat Jul  6 17:26:43 CEST 2002
 *   finished at Sat Jul  6 18:25:58 CEST 2002
 *
 * algorithm ripped from Info-ZIP 2.2, implementation and (C):
 *
 * Copyright (C) 1990-1997 Mark Adler, Richard B. Wales, Jean-loup Gailly,
 * Kai Uwe Rommel, Onno van der Linden and Igor Mandrichenko.
 * Permission is granted to any individual or institution to use, copy, or
 * redistribute this software so long as all of the original files are included,
 * that it is not sold for profit, and that this copyright notice is retained.
 *
 */

#ifdef __GNUC__
#pragma implementation
#endif

#if _MSC_VER > 1000
# undef  __PROTOTYPES__
# define __PROTOTYPES__ 1
# pragma warning(disable: 4127) /* conditional expression is constant */
# pragma warning(disable: 4244) /* =' : conversion from 'int ' to 'unsigned char ', possible loss of data */
#endif

/* ---- zip.h */

/*

 Copyright (C) 1990-1997 Mark Adler, Richard B. Wales, Jean-loup Gailly,
 Kai Uwe Rommel, Onno van der Linden and Igor Mandrichenko.
 Permission is granted to any individual or institution to use, copy, or
 redistribute this software so long as all of the original files are included,
 that it is not sold for profit, and that this copyright notice is retained.

*/

/*
 *  zip.h by Mark Adler.
 */

#ifndef __zip_h
#define __zip_h 1

/**** pts ****/
/* Imp: long, ulg */
/* OK : parametric memory allocation */
/* OK : error handling code into caller */
/* OK : make it a library */
/* Symbols imported: __assert_fail memcpy
 * Symbols exported: T pts_defl_new
 * Hidden global const variables: bl_order bl_desc d_desc l_desc
 *   extra_blbits extra_dbits extra_lbits configuration table
 * Hidden global variables (the initial value assigned by the 1st call to
 *   pts_defl_new() is retained forever): base_dist base_length dist_code
 *   length_code static_dtree static_ltree
 * Thread synchronization variables: need_ct_init
 */

#if OBJDEP
#  warning PROVIDES: pts_defl
#endif
#define asmv_local local
#define DYN_ALLOC 1
#undef HUFFMAN_ONLY
#undef FILTERED /* Imp: ?? */
#undef FULL_SEARCH
#undef DEBUG
#undef DEBUGNAMES
#undef ASMV
#undef RISCOS
#undef MEMORY16
#define PTS_ZIP 1 /**** pts ****/
#define NO_MKTEMP 1
#undef  NULL
#define NULL ((void*)0)
#undef  NULLL
#define NULLL 0
#define zzz static /* local to the merged pts_defl.c */
#include <assert.h>

/* Possible values for zip_pts_error */
#define ERROR_DONE     0 /* compression finished without errors */
#define ERROR_WORKING  1 /* compression in progress */
#define ERROR_PASTEOF  2 /* data arrived after EOF */
#define ERROR_BADPL    3 /* invalid pack_level for pts_deflate_init() */
#define ERROR_ZPFWRITE 4 /* zpfwrite() failed */
/* vvv ERROR_* error codes are never set! */
#define ERROR_MEM      5 /* out of memory */
#define ERROR_MEM_CT   6 /* out of memory in ct_init() */
#define ERROR_MEM_WIN  7 /* out of memory for window */
#define ERROR_MEM_HT   8 /* out of memory for hashtable */
#define ERROR_MEM_DS   9 /* out of memory for pts_defl_internal_state */

/* Set up portability */
/* #include "tailor.h" */

/* ---- tailor.h */

/* When "void" is an alias for "int", prototypes cannot be used. */
#if (defined(NO_VOID) && !defined(NO_PROTO))
#  define NO_PROTO
#endif

#include "pts_defl.h"
#include <string.h> /* memcpy */

/* Avoid using const if compiler does not support it */
#if (!defined(ZCONST) && (!defined(NO_CONST) || defined(USE_CONST)))
#  define ZCONST const
#endif

#ifndef ZCONST
#  define ZCONST
#endif

#ifndef PTS_ZIP
/*
 * case mapping functions. case_map is used to ignore case in comparisons,
 * to_up is used to force upper case even on Unix (for dosify option).
 */
#ifdef USE_CASE_MAP
#  define case_map(c) upper[(c) & 0xff]
#  define to_up(c)    upper[(c) & 0xff]
#else
#  define case_map(c) (c)
#  define to_up(c)    ((c) >= 'a' && (c) <= 'z' ? (c)-'a'+'A' : (c))
#endif /* USE_CASE_MAP */
#endif

/* Define void, zvoid, and extent (size_t) */

#ifdef NO_VOID
#  define void int
   typedef char zvoid;
#else /* !NO_VOID */
# ifdef NO_TYPEDEF_VOID
#  define zvoid void
# else
   typedef void zvoid;
# endif
#endif /* ?NO_VOID */

#ifndef PTS_ZIP
/*
 * A couple of forward declarations that are needed on systems that do
 * not supply C runtime library prototypes.
 */
#ifdef NO_PROTO
char *strcpy();
char *strcat();
char *strrchr();
/* XXX use !defined(ZMEM) && !defined(__hpux__) ? */
#if !defined(ZMEM) && defined(NO_STRING_H)
char *memset();
char *memcpy();
#endif /* !ZMEM && NO_STRING_H */

/* XXX use !defined(__hpux__) ? */
#ifdef NO_STDLIB_H
char *calloc();
char *malloc();
char *getenv();
long atol();
#endif /* NO_STDLIB_H */

#endif /* NO_PROTO */
#endif /* PTS_ZIP */

/* Define this symbol if your target allows access to unaligned data.
 * This is not mandatory, just a speed optimization. The compressed
 * output is strictly identical.
 */
#if (defined(MSDOS) && !defined(WIN32)) || defined(i386)
#    define UNALIGNED_OK
#endif
#if defined(mc68020) || defined(vax)
#    define UNALIGNED_OK
#endif

#ifdef SMALL_MEM
#   define CBSZ 2048 /* buffer size for copying files */
#   define ZBSZ 2048 /* buffer size for temporary zip file */
#endif

#ifdef MEDIUM_MEM
#  define CBSZ 8192
#  define ZBSZ 8192
#endif

#ifndef CBSZ
#  define CBSZ 16384
#  define ZBSZ 16384
#endif

#ifndef MEMORY16
#  ifdef __WATCOMC__
#    undef huge
#    undef far
#    undef near
#  endif
#  ifndef __IBMC__
#    define huge
#    define far
#    define near
#  endif
#  define nearmalloc malloc
#  define nearfree free
#  define farmalloc malloc
#  define farfree free
#endif /* !MEMORY16 */

/* end of tailor.h */


#define MIN_MATCH  3
#define MAX_MATCH  258
/* The minimum and maximum match lengths */

#ifndef WSIZE
#  define WSIZE  (0x8000)
#endif
/* Maximum window size = 32K. If you are really short of memory, compile
 * with a smaller WSIZE but this reduces the compression ratio for files
 * of size > WSIZE. WSIZE must be a power of two in the current implementation.
 */

#define MIN_LOOKAHEAD (MAX_MATCH+MIN_MATCH+1)
/* Minimum amount of lookahead, except at the end of the input file.
 * See deflate.c for comments about the MIN_MATCH+1.
 */

#define MAX_DIST  (WSIZE-MIN_LOOKAHEAD)
/* In order to simplify the code, particularly on 16 bit machines, match
 * distances are limited to MAX_DIST instead of WSIZE.
 */

/* Types centralized here for easy modification */
#define local static            /* More meaningful outside functions */
typedef unsigned char uch;      /* unsigned 8-bit value */
typedef unsigned short ush;     /* unsigned 16-bit value */
typedef unsigned long u32;      /* unsigned 32-bit value */

/* Diagnostic functions */
#ifdef DEBUG
# ifdef MSDOS
#  undef  stderr
#  define stderr stdout
# endif
#  define diag(where) fprintf(stderr, "zip diagnostic: %s\n", where)
#  define Assert(cond,msg) {if(!(cond)) error(msg);}
#  define Trace(x) fprintf x
#  define Tracev(x) {if (verbose) fprintf x ;}
#  define Tracevv(x) {if (verbose>1) fprintf x ;}
#  define Tracec(c,x) {if (verbose && (c)) fprintf x ;}
#  define Tracecv(c,x) {if (verbose>1 && (c)) fprintf x ;}
#else
#  define diag(where)
#  define Assert(cond,msg) /*assert(cond)*/ /**** pts ****/
#  define Trace(x)
#  define Tracev(x)
#  define Tracevv(x)
#  define Tracec(c,x)
#  define Tracecv(c,x)
#endif

#ifdef DEBUGNAMES
#  define free(x) { int *v;Free(x); v=x;*v=0xdeadbeef;x=(void *)0xdeadbeef; }
#endif

/* ===========================================================================
 * Constants
 */

#define MAX_BITS 15
/* All codes must not exceed MAX_BITS bits */

#define MAX_BL_BITS 7
/* Bit length codes must not exceed MAX_BL_BITS bits */

#define LENGTH_CODES 29
/* number of length codes, not counting the special END_BLOCK code */

#define LITERALS  256
/* number of literal bytes 0..255 */

#define END_BLOCK 256
/* end of block literal code */

#define L_CODES (LITERALS+1+LENGTH_CODES)
/* number of Literal or Length codes, including the END_BLOCK code */

#define D_CODES   30
/* number of distance codes */

#define BL_CODES  19
/* number of codes used to transfer the bit lengths */

#define HEAP_SIZE (2*L_CODES+1)
/* maximum heap size */

#ifndef LIT_BUFSIZE
#  ifdef SMALL_MEM
#    define LIT_BUFSIZE  0x2000
#  else
#  ifdef MEDIUM_MEM
#    define LIT_BUFSIZE  0x4000
#  else
#    define LIT_BUFSIZE  0x8000
#  endif
#  endif
#endif
#define DIST_BUFSIZE  LIT_BUFSIZE
/* Sizes of match buffers for literals/lengths and distances.  There are
 * 4 reasons for limiting LIT_BUFSIZE to 64K:
 *   - frequencies can be kept in 16 bit counters
 *   - if compression is not successful for the first block, all input data is
 *     still in the window so we can still emit a stored block even when input
 *     comes from standard input.  (This can also be done for all blocks if
 *     LIT_BUFSIZE is not greater than 32K.)
 *   - if compression is not successful for a file smaller than 64K, we can
 *     even emit a stored file instead of a stored block (saving 5 bytes).
 *   - creating new Huffman trees less frequently may not provide fast
 *     adaptation to changes in the input data statistics. (Take for
 *     example a binary file with poorly compressible code followed by
 *     a highly compressible string table.) Smaller buffer sizes give
 *     fast adaptation but have of course the overhead of transmitting trees
 *     more frequently.
 *   - I can't count above 4
 * The current code is general and allows DIST_BUFSIZE < LIT_BUFSIZE (to save
 * memory at the expense of compression). Some optimizations would be possible
 * if we rely on DIST_BUFSIZE == LIT_BUFSIZE.
 */

#define REP_3_6      16
/* repeat previous bit length 3-6 times (2 bits of repeat count) */

#define REPZ_3_10    17
/* repeat a zero length 3-10 times  (3 bits of repeat count) */

#define REPZ_11_138  18
/* repeat a zero length 11-138 times  (7 bits of repeat count) */

/* --- GLOBAL-independent functions */

#define SMALLEST 1
/* Index within the heap array of least frequent node in the Huffman tree */


/* ===========================================================================
 * Compares to subtrees, using the tree depth as tie breaker when
 * the subtrees have equal frequency. This minimizes the worst case length.
 */
#define smaller(tree, n, m) \
   (tree[n].Freq < tree[m].Freq || \
   (tree[n].Freq == tree[m].Freq && depth[n] <= depth[m]))

#define Freq fc.freq
#define Code fc.code
#define Dad  dl.dad
#define Len  dl.len

/* Data structure describing a single value and its code string. */
typedef struct ct_data {
    union {
        ush  freq;       /* frequency count */
        ush  code;       /* bit string */
    } fc;
    union {
        ush  dad;        /* father node in Huffman tree */
        ush  len;        /* length of bit string */
    } dl;
} ct_data;

#if defined(BIG_MEM) || defined(MMAP)
  typedef unsigned Pos; /* must be at least 32 bits */
#else
  typedef ush Pos;
#endif
typedef unsigned IPos;
/* A Pos is an index in the character window. We use short instead of int to
 * save space in the various tables. IPos is used only for parameter passing.
 */

/* ===========================================================================
 * Generate the codes for a given tree and bit counts (which need not be
 * optimal).
 * IN assertion: the array bl_count contains the bit length statistics for
 * the given tree and the field len is set for all tree elements.
 * OUT assertion: the field code is set for all tree elements of non
 *     zero code length.
 */
local void gen_codes ___((ct_data near *tree, int max_code, ush near*bl_count),(tree, max_code, bl_count),(
    ct_data near *tree;        /* the tree to decorate */
    int max_code;              /* largest code with non zero frequency */
    ush near *bl_count;
)) {
    ush next_code[MAX_BITS+1]; /* next code value for each bit length */
    ush code = 0;              /* running code value */
    int bits;                  /* bit index */
    int n;                     /* code index */

    /* The distribution counts are first used to generate the code values
     * without bit reversal.
     */
    for (bits = 1; bits <= MAX_BITS; bits++) {
        next_code[bits] = code = (code + bl_count[bits-1]) << 1;
    }
    /* Check that the bit counts in bl_count are consistent. The last code
     * must be all ones.
     */
    Assert (code + bl_count[MAX_BITS]-1 == (1<< ((ush) MAX_BITS)) - 1,
            "inconsistent bit counts");
    Tracev((stderr,"\ngen_codes: max_code %d ", max_code));

    for (n = 0;  n <= max_code; n++) {
        int len = tree[n].Len;
        if (len == 0) continue;
        /* Now reverse the bits */

        /**** pts ****/
        /* tree[n].Code = bi_reverse(next_code[len]++, len); */
        /* ===========================================================================
         * Reverse the first len bits of a code, using straightforward code (a faster
         * method would use a table)
         * IN assertion: 1 <= len <= 15
         */
        #if 0
        unsigned bi_reverse ___((unsigned code, int len),(code, len),(
            unsigned code; /* the value to invert */
            int len;       /* its bit length */
        ))
        #endif
        {
            unsigned code=next_code[len]++;
            register unsigned res = 0;
            do {
                res |= code & 1;
                code >>= 1, res <<= 1;
            } while (--len > 0);
            /* return res >> 1; */
            tree[n].Code=res>>1;
        }

        Tracec(tree != static_ltree, (stderr,"\nn %3d %c l %2d c %4x (%x) ",
             n, (isgraph(n) ? n : ' '), len, tree[n].Code, next_code[len]-1));
    }
}

/* ===========================================================================
 * Restore the heap property by moving down the tree starting at node k,
 * exchanging a node with the smallest of its two sons if necessary, stopping
 * when the heap property is re-established (each father smaller than its
 * two sons).
 */
local void pqdownheap ___((ct_data near *tree,int k, int *heap, int heap_len, uch *depth),(tree, k, heap, heap_len, depth),(
    ct_data near *tree;  /* the tree to restore */
    int k;               /* node to move down */
    int *heap;
    int heap_len;
    uch *depth;
)) {
    int v = heap[k];
    int j = k << 1;  /* left son of k */
    int htemp;       /* required because of bug in SASC compiler */

    while (j <= heap_len) {
        /* Set j to the smallest of the two sons: */
        if (j < heap_len && smaller(tree, heap[j+1], heap[j])) j++;

        /* Exit if v is smaller than both sons */
        htemp = heap[j];
        if (smaller(tree, v, htemp)) break;

        /* Exchange v with the smallest son */
        heap[k] = htemp;
        k = j;

        /* And continue down the tree, setting j to the left son of k */
        j <<= 1;
    }
    heap[k] = v;
}

/* --- */

#if 0
local pts_defl_free_t zip_pts_zpffree; /*:GLOBAL*/
local unsigned zip_pts_error; /*:GLOBAL*/
local unsigned zip_pts_state; /*:GLOBAL*/
local int (*zip_zpfwrite)(char *block, unsigned len, void *zfile); /*:GLOBAL*/
zzz /*extern*/ int zip_pack_level; /*:GLOBAL*/      /* Compression level */
local char *avail_buf; /*:GLOBAL*/
local char *avail_bufend; /*:GLOBAL*/
/** Have we read EOF? */
local int avail_eof; /*:GLOBAL*/
/** How many bytes are we expecting? */
local int saved_match_available; /*:GLOBAL*/ /* set if previous match exists */
local unsigned saved_match_length; /*:GLOBAL*/ /* length of best match */
zzz /*extern*/ u32 window_size; /*:GLOBAL*/ /* size of sliding window */
local void *zfile; /*:GLOBAL*/ /* output zip file */
asmv_local unsigned short bi_buf; /*:GLOBAL*/
/* Output buffer. bits are inserted starting at the bottom (least significant
 * bits).
 */
asmv_local int bi_valid; /*:GLOBAL*/
/* Number of valid bits in bi_buf.  All bits above the last valid bit
 * are always zero.
 */
zzz char file_outbuf[1024]; /*:GLOBAL*/ /* static! */
/* Output buffer for compression to file */
local char *out_buf;
local char *out_bufp, *out_buflast; /*:GLOBAL*/
local int l_desc_max_code; /*:GLOBAL*/
local int d_desc_max_code; /*:GLOBAL*/
local ct_data near dyn_ltree[HEAP_SIZE];   /*:GLOBAL*/ /* literal and length tree */
local ct_data near dyn_dtree[2*D_CODES+1]; /*:GLOBAL*/ /* distance tree */
local ct_data near bl_tree[2*BL_CODES+1]; /*:GLOBAL*/
/* Huffman tree for the bit lengths */
local ush near bl_count[MAX_BITS+1]; /*:GLOBAL*/
local int near heap[2*L_CODES+1]; /*:GLOBAL*/ /* heap used to build the Huffman trees */
/* number of codes at each bit length for an optimal tree */
local int heap_len;               /*:GLOBAL*/ /* number of elements in the heap */
local int heap_max;               /*:GLOBAL*/ /* element of largest frequency */
/* The sons of heap[n] are heap[2*n] and heap[2*n+1]. heap[0] is not used.
 * The same heap array is used to build all trees.
 */
local uch near depth[2*L_CODES+1]; /*:GLOBAL*/
/* Depth of each subtree used as tie breaker for trees of equal frequency */
local uch far *l_buf; /*:GLOBAL*/
local ush far *d_buf; /*:GLOBAL*/
local uch near flag_buf[(LIT_BUFSIZE/8)]; /*:GLOBAL*/
/* flag_buf is a bit array distinguishing literals from lengths in
 * l_buf, and thus indicating the presence or absence of a distance.
 */
local unsigned last_lit;    /*:GLOBAL*/ /* running index in l_buf */
local unsigned last_dist;   /*:GLOBAL*/ /* running index in d_buf */
local unsigned last_flags;  /*:GLOBAL*/ /* running index in flag_buf */
local uch flags;            /*:GLOBAL*/ /* current flags not yet saved in flag_buf */
local uch flag_bit;         /*:GLOBAL*/ /* current bit used in flags */
/* bits are filled in flags starting at bit 0 (least significant).
 * Note: these flags are overkill in the current code since we don't
 * take advantage of DIST_BUFSIZE == LIT_BUFSIZE.
 */
local u32 opt_len;        /*:GLOBAL*/ /* bit length of current block with optimal trees */
local u32 static_len;     /*:GLOBAL*/ /* bit length of current block with static trees */
zzz /*extern*/ long block_start; /*:GLOBAL*/       /* window offset of current block */
zzz /*extern*/ unsigned near strstart; /*:GLOBAL*/ /* window offset of current string to insert */
zzz uch far * near window; /*:GLOBAL*/
zzz Pos far * near prev  ; /*:GLOBAL*/
zzz unsigned int near prev_length; /*:GLOBAL*/
/* Length of the best match at previous step. Matches not greater than this
 * are discarded. This is used in the lazy match evaluation.
 */
zzz Pos far * near head; /*:GLOBAL*/
local unsigned      lookahead; /*:GLOBAL*/     /* number of valid bytes ahead in window */
local int sliding; /*:GLOBAL*/
/* Set to false when the input file is already in memory */
local unsigned ins_h; /*:GLOBAL*/  /* hash index of string to be inserted */
zzz      unsigned near match_start; /*:GLOBAL*/   /* start of matching string */
local int           eofile; /*:GLOBAL*/        /* flag set at end of input file */
zzz unsigned near max_chain_length; /*:GLOBAL*/
/* To speed up deflation, hash chains are never searched beyond this length.
 * A higher limit improves compression ratio but degrades the speed.
 */
local unsigned int max_lazy_match; /*:GLOBAL*/
/* Attempt to find a better match only when the current match is strictly
 * smaller than this value. This mechanism is used only for compression
 * levels >= 4.
 */
zzz unsigned near good_match; /*:GLOBAL*/
/* Use a faster search when the previous match is longer than this */
zzz int near nice_match; /*:GLOBAL*/ /* Stop searching when current match exceeds this */
#endif

/**** pts ****/
#define DS0 struct pts_defl_internal_state* ds
#define DS1 , struct pts_defl_internal_state* ds
#define DS2 ds
#define DS3 struct pts_defl_internal_state* ds;

struct pts_defl_internal_state { /* Thu Jul  4 23:16:41 CEST 2002 */
  struct pts_defl_interface interf;
  /** Have we read EOF? */
  int avail_eof; /*:GLOBAL*/
  /* pts_defl_free_t zip_pts_zpffree; */ /*:GLOBAL*/
  /* unsigned zip_pts_error; */ /*:GLOBAL*/
  unsigned zip_pts_state; /*:GLOBAL*/
  /** How many bytes are we expecting? */
  int saved_match_available; /*:GLOBAL*/ /* set if previous match exists */
  unsigned saved_match_length; /*:GLOBAL*/ /* length of best match */
  /* int zip_pack_level; */ /*:GLOBAL*/      /* Compression level */
  u32 window_size; /*:GLOBAL*/ /* size of sliding window */
  void *zfile; /*:GLOBAL*/ /* output zip file */
  unsigned short bi_buf; /*:GLOBAL*/
  /* Output buffer. bits are inserted starting at the bottom (least significant
   * bits).
   */
  int bi_valid; /*:GLOBAL*/
  /* Number of valid bits in bi_buf.  All bits above the last valid bit
   * are always zero.
   */
  #if 0
    char file_outbuf[1024]; /*:GLOBAL*/ /* separate alloc*/
    /* Output buffer for compression to file */
    char *out_buf;
  #else
    char out_buf[1024];
  #endif
  char *out_bufp, *out_buflast; /*:GLOBAL*/
  int l_desc_max_code; /*:GLOBAL*/
  int d_desc_max_code; /*:GLOBAL*/
  /* int (*zip_zpfwrite)(char *block, unsigned len, void *zfile); */ /*:GLOBAL*/
  ct_data near dyn_ltree[HEAP_SIZE];   /*:GLOBAL*/ /* literal and length tree */
  ct_data near dyn_dtree[2*D_CODES+1]; /*:GLOBAL*/ /* distance tree */
  ct_data near bl_tree[2*BL_CODES+1]; /*:GLOBAL*/
  /* Huffman tree for the bit lengths */
  ush near bl_count[MAX_BITS+1]; /*:GLOBAL*/
  /* number of codes at each bit length for an optimal tree */
  int near heap[2*L_CODES+1]; /*:GLOBAL*/ /* heap used to build the Huffman trees */
  int heap_len;               /*:GLOBAL*/ /* number of elements in the heap */
  int heap_max;               /*:GLOBAL*/ /* element of largest frequency */
  /* The sons of heap[n] are heap[2*n] and heap[2*n+1]. heap[0] is not used.
   * The same heap array is used to build all trees.
   */
  uch near depth[2*L_CODES+1]; /*:GLOBAL*/
  /* Depth of each subtree used as tie breaker for trees of equal frequency */
  uch far *l_buf; /*:GLOBAL*/
  ush far *d_buf; /*:GLOBAL*/
  uch near flag_buf[(LIT_BUFSIZE/8)]; /*:GLOBAL*/
  /* flag_buf is a bit array distinguishing literals from lengths in
   * l_buf, and thus indicating the presence or absence of a distance.
   */
  unsigned last_lit;    /*:GLOBAL*/ /* running index in l_buf */
  unsigned last_dist;   /*:GLOBAL*/ /* running index in d_buf */
  unsigned last_flags;  /*:GLOBAL*/ /* running index in flag_buf */
  uch flags;            /*:GLOBAL*/ /* current flags not yet saved in flag_buf */
  uch flag_bit;         /*:GLOBAL*/ /* current bit used in flags */
  /* bits are filled in flags starting at bit 0 (least significant).
   * Note: these flags are overkill in the current code since we don't
   * take advantage of DIST_BUFSIZE == LIT_BUFSIZE.
   */
  u32 opt_len;        /*:GLOBAL*/ /* bit length of current block with optimal trees */
  u32 static_len;     /*:GLOBAL*/ /* bit length of current block with static trees */
  /*extern*/ long block_start; /*:GLOBAL*/       /* window offset of current block */
  /*extern*/ unsigned near strstart; /*:GLOBAL*/ /* window offset of current string to insert */
  uch far * near window; /*:GLOBAL*/
  Pos far * near prev  ; /*:GLOBAL*/
  Pos far * near head; /*:GLOBAL*/
  int sliding; /*:GLOBAL*/
  /* Set to false when the input file is already in memory */
  unsigned ins_h; /*:GLOBAL*/  /* hash index of string to be inserted */
  unsigned int near prev_length; /*:GLOBAL*/
  /* Length of the best match at previous step. Matches not greater than this
   * are discarded. This is used in the lazy match evaluation.
   */
  unsigned near match_start; /*:GLOBAL*/   /* start of matching string */
  int           eofile; /*:GLOBAL*/        /* flag set at end of input file */
  unsigned      lookahead; /*:GLOBAL*/     /* number of valid bytes ahead in window */
  unsigned near max_chain_length; /*:GLOBAL*/
  /* To speed up deflation, hash chains are never searched beyond this length.
   * A higher limit improves compression ratio but degrades the speed.
   */
  unsigned int max_lazy_match; /*:GLOBAL*/
  /* Attempt to find a better match only when the current match is strictly
   * smaller than this value. This mechanism is used only for compression
   * levels >= 4.
   */
  unsigned near good_match; /*:GLOBAL*/
  /* Use a faster search when the previous match is longer than this */
  int near nice_match; /*:GLOBAL*/ /* Stop searching when current match exceeds this */
};

/* ===========================================================================
 * Remove the smallest element from the heap and recreate the heap with
 * one less element. Updates heap and heap_len.
 */
#define pqremove(tree, top) \
{\
    top = ds->heap[SMALLEST]; \
    ds->heap[SMALLEST] = ds->heap[ds->heap_len--]; \
    pqdownheap(tree, SMALLEST, ds->heap, ds->heap_len, ds->depth); \
}

/* internal file attribute */
#define UNKNOWN (-1)
#define BINARY  0
#define ASCII   1
#define __EBCDIC 2

/* Error return codes and PERR macro */

#define BEST -1                 /* Use best method (deflation or store) */
#define STORE 0                 /* Store method */
#define DEFLATE 8               /* Deflation method*/
extern int method;              /* Restriction on compression method */

#ifdef PTS_ZIP
#define translate_eol 0
#else
extern int translate_eol;       /* Translate end-of-line LF -> CR LF */
#endif


#ifndef PTS_ZIP
#if 0            /* Optimization: use the (const) result of crc32(0L,NULL,0) */
#  define CRCVAL_INITIAL  crc32(0L, (uch *)NULL, 0)
#else
#  define CRCVAL_INITIAL  0L
#endif
#ifdef UTIL
#  define error(msg)    ziperr(ZE_LOGIC, msg)
#else
   void error OF((char *h));
#  ifdef VMSCLI
     void help OF((void));
#  endif
   int encr_passwd OF((int modeflag, char *pwbuf, int size, ZCONST char *zfn));
#endif
#endif /* PTS_ZIP */

/*zzz void easy_zerror2 OF((char*));*/

#ifndef USE_ZLIB
#ifndef UTIL
#ifndef PTS_ZIP
        /* in crc32.c */
_unused_ ulg  crc32         OF((ulg, ZCONST uch *, extent));
#endif
#endif /* !UTIL */

        /* in deflate.c */
#ifdef PTS_ZIP
/*zzz void lm_init OF((void));*/
#else
zzz void lm_init OF((int pack_level, ush *flags));
#endif
/* zzz void lm_free OF((void)); */
/* ulg  deflate OF((void)); */

        /* in trees.c */
/* zzz void ct_init     OF((ush *attr, int *method)); */
zzz int  ct_tally    OF((int dist, int lc DS1));
zzz void flush_block OF((char far *buf, u32 stored_len, int eof DS1));

        /* in bits.c */
/* zzz void     bi_init     OF((void *zipfile)); */
zzz void     send_bits   OF((int value, int length DS1));
/*zzz unsigned bi_reverse  OF((unsigned value, int length));*/
/*zzz void     bi_windup   OF((void));*/
/* zzz void     copy_block  OF((char *block, unsigned len, int header)); */

#ifndef PTS_ZIP
int      seekable    OF((void));
extern   int (*read_buf) OF((char *buf, unsigned size));
_unused_ ulg      memcompress OF((char *tgt, ulg tgtsize, char *src, ulg srcsize));
#endif

#endif /* !UTIL */

zzz void deflate2 OF((char *,unsigned,struct pts_defl_interface*));

#endif /* !__zip_h */
/* end of zip.h */

/* ---- bits.c */

/*

 Copyright (C) 1990-1997 Mark Adler, Richard B. Wales, Jean-loup Gailly,
 Kai Uwe Rommel, Onno van der Linden and Igor Mandrichenko.
 Permission is granted to any individual or institution to use, copy, or
 redistribute this software so long as all of the original files are included,
 that it is not sold for profit, and that this copyright notice is retained.

*/

/*
 *  bits.c by Jean-loup Gailly and Kai Uwe Rommel.
 *
 *  This is a new version of im_bits.c originally written by Richard B. Wales
 *
 *  PURPOSE
 *
 *      Output variable-length bit strings. Compression can be done
 *      to a file or to memory.
 *
 *  DISCUSSION
 *
 *      The PKZIP "deflate" file format interprets compressed file data
 *      as a sequence of bits.  Multi-bit strings in the file may cross
 *      byte boundaries without restriction.
 *
 *      The first bit of each byte is the low-order bit.
 *
 *      The routines in this file allow a variable-length bit value to
 *      be output right-to-left (useful for literal values). For
 *      left-to-right output (useful for code strings from the tree routines),
 *      the bits must have been reversed first with bi_reverse().
 *
 *      For in-memory compression, the compressed bit stream goes directly
 *      into the requested output buffer. The input data is read in blocks
 *      by the mem_read() function. The buffer is limited to 64K on 16 bit
 *      machines.
 *
 *  INTERFACE
 *
 *      void bi_init (FILE *zipfile)
 *          Initialize the bit string routines.
 *
 *      void send_bits (int value, int length)
 *          Write out a bit string, taking the source bits right to
 *          left.
 *
 *      int bi_reverse (int value, int length)
 *          Reverse the bits of a bit string, taking the source bits left to
 *          right and emitting them right to left.
 *
 *      void bi_windup (void)
 *          Write out any remaining bits in an incomplete byte.
 *
 *      void copy_block(char far *buf, unsigned len, int header)
 *          Copy a stored block to the zip file, storing first the length and
 *          its one's complement if requested.
 *
 *      int seekable(void)
 *          Return true if the zip file can be seeked.
 *
 *      ulg memcompress (char *tgt, ulg tgtsize, char *src, ulg srcsize);
 *          Compress the source buffer src into the target buffer tgt.
 */

/* #include "zip.h" */
/* #include "crypt.h" */


/* ===========================================================================
 * Local data used by the "bit string" routines.
 */


/**** pts ****/
#if 0
zzz void easy_zerror2 ___((char *s),(s),(char *s;)) {
  fprintf(stderr,"zerror: %s\n",s);
  abort();
}
#endif


#define Buf_size (8 * 2*sizeof(char))
/* Number of bits used within bi_buf. (bi_buf might be implemented on
 * more than 16 bits on some systems.)
 */


#ifndef PTS_ZIP
asmv_local unsigned in_offset, out_offset;
/* Current offset in input and output buffers. in_offset is used only for
 * in-memory compression. On 16 bit machines, the buffer is limited to 64K.
 */
asmv_local unsigned in_size, out_size;
/* Size of current input and output buffers */
int (*read_buf) OF((char *buf, unsigned size)) = file_read;
/* Current input function. Set to mem_read for in-memory compression */
#endif

#ifdef DEBUG
ulg bits_sent;   /* bit length of the compressed data */
#endif

#ifndef PTS_ZIP
/* Output a 16 bit value to the bit stream, lower (oldest) byte first */
#define PUTSHORT(w) \
{ if (out_offset < out_size-1) { \
    out_buf[out_offset++] = (char) ((w) & 0xff); \
    out_buf[out_offset++] = (char) ((ush)(w) >> 8); \
  } else { \
    flush_outbuf((w),2); \
  } \
}

#define PUTBYTE(b) \
{ if (out_offset < out_size) { \
    out_buf[out_offset++] = (char) (b); \
  } else { \
    flush_outbuf((b),1); \
  } \
}
#endif

/* ===========================================================================
 *  Prototypes for local functions
 */
#ifndef PTS_ZIP
local int  mem_read     OF((char *b,    unsigned bsize));
#endif
/*#if !defined(RISCOS) || (defined(RISCOS) && !defined(ASMV))*/
local void flush_outbuf OF((unsigned w, unsigned bytes DS1));
/*#endif*/


/*#if !defined(ASMV) || !defined(RISCOS)*/
/* ===========================================================================
 * Send a value on a given number of bits.
 * IN assertion: length <= 16 and value fits in length bits.
 */
static void send_bits ___((int value, int length DS1),(value, length DS2),(
    int value;  /* value to send */
    int length; /* number of bits */
DS3)) {
#ifdef DEBUG
    Tracevv((stderr," l %2d v %4x ", length, value));
    Assert(length > 0 && length <= 15, "invalid length");
    bits_sent += (ulg)length;
#endif
    /* If not enough room in bi_buf, use (valid) bits from bi_buf and
     * (16 - ds->bi_valid) bits from value, leaving (width - (16-ds->bi_valid))
     * unused bits in value.
     */
    if (ds->bi_valid > (int)Buf_size - length) {
        ds->bi_buf |= (value << ds->bi_valid);
        if (ds->out_bufp < ds->out_buflast) {
          *ds->out_bufp++ = (char) (ds->bi_buf & 0xff);
          *ds->out_bufp++ = (char) ((ush)ds->bi_buf >> 8);
        } else flush_outbuf(ds->bi_buf, 2, ds);
        ds->bi_buf = (ush)value >> (Buf_size - ds->bi_valid);
        ds->bi_valid += length - Buf_size;
    } else {
        ds->bi_buf |= value << ds->bi_valid;
        ds->bi_valid += length;
    }
}

/*#endif*/

/* ===========================================================================
 * Flush the current output buffer.
 */
asmv_local
void flush_outbuf ___((unsigned w,unsigned bytes DS1),(w, bytes DS2),(
    unsigned w;     /* value to flush */
    unsigned bytes; /* number of bytes to flush (0, 1 or 2) */
DS3 )) {
#ifndef PTS_ZIP
    if (ds->zfile == NULL) {
        error("output buffer too small for in-memory compression");
    }
#endif
    /* Encrypt and write the output buffer: */
    if (ds->out_bufp != ds->out_buf) {
        if (ds->interf.err<2 && 0!=(ds->interf.zpfwrite)(ds->out_buf, ds->out_bufp-ds->out_buf, ds->zfile))
          ds->interf.err=ERROR_ZPFWRITE;
          /* easy_zerror2("write error on zip file"); */ /*:ERROR*/
    }
    /* if (bytes!=0) { */
      ds->out_buf[0] = (char) ((w) & 0xff);
      ds->out_bufp = ds->out_buf+1;
      if (bytes == 2) {
         *ds->out_bufp++ = (char) ((ush)(w) >> 8);
      } else assert(bytes == 1);
    /* } else out_offset = 0; */
}

#ifndef PTS_ZIP
/* ===========================================================================
 * Return true if the zip file can be seeked. This is used to check if
 * the local header can be re-rewritten. This function always returns
 * true for in-memory compression.
 * IN assertion: the local header has already been written (ftell() > 0).
 */
int seekable()
{
    return fseekable(ds->zfile);
}
#endif

#ifndef PTS_ZIP
asmv_local char *in_buf;
/* Current input and output buffers. in_buf is used only for in-memory
 * compression.
 */
/* ===========================================================================
 * In-memory compression. This version can be used only if the entire input
 * fits in one memory buffer. The compression is then done in a single
 * call of memcompress(). (An extension to allow repeated calls would be
 * possible but is not needed here.)
 * The first two bytes of the compressed output are set to a short with the
 * method used (DEFLATE or STORE). The following four bytes contain the CRC.
 * The values are stored in little-endian order on all machines.
 * This function returns the byte size of the compressed output, including
 * the first six bytes (method and crc).
 */

ulg memcompress(tgt, tgtsize, src, srcsize)
    char *tgt, *src;       /* target and source buffers */
    ulg tgtsize, srcsize;  /* target and source sizes */
{
    ush att      = (ush)UNKNOWN;
    ush flags    = 0;
    ulg crc;
    int method   = DEFLATE;

    if (tgtsize <= 6L) error("target buffer too small");

    crc = CRCVAL_INITIAL;
    crc = crc32(crc, (uch *)src, (extent)srcsize);

    read_buf  = mem_read;
    in_buf    = src;
    in_size   = (unsigned)srcsize;
    in_offset = 0;

    out_buf    = tgt;
    out_size   = (unsigned)tgtsize;
    out_offset = 2 + 4;
    window_size = 0L;

    bi_init((FILE *)NULL);
    ct_init(&att, &method);
    lm_init((level != 0 ? level : 1), &flags);
    deflate();
    window_size = 0L; /* was updated by lm_init() */

    /* For portability, force little-endian order on all machines: */
    tgt[0] = (char)(method & 0xff);
    tgt[1] = (char)((method >> 8) & 0xff);
    tgt[2] = (char)(crc & 0xff);
    tgt[3] = (char)((crc >> 8) & 0xff);
    tgt[4] = (char)((crc >> 16) & 0xff);
    tgt[5] = (char)((crc >> 24) & 0xff);

    return (ulg)out_offset;
}

/* ===========================================================================
 * In-memory read function. As opposed to file_read(), this function
 * does not perform end-of-line translation, and does not update the
 * crc and input size.
 *    Note that the size of the entire input buffer is an unsigned long,
 * but the size used in mem_read() is only an unsigned int. This makes a
 * difference on 16 bit machines. mem_read() may be called several
 * times for an in-memory compression.
 */
local int mem_read ___((char *b,unsigned bsize),(b, bsize),(
     char *b;
     unsigned bsize;
)) {
    if (in_offset < in_size) {
        ulg block_size = in_size - in_offset;
        if (block_size > (ulg)bsize) block_size = (ulg)bsize;
        memcpy(b, in_buf + in_offset, (unsigned)block_size);
        in_offset += (unsigned)block_size;
        return (int)block_size;
    } else {
        return 0; /* end of input */
    }
}
#endif


/* ---- trees.c */

/*

 Copyright (C) 1990-1997 Mark Adler, Richard B. Wales, Jean-loup Gailly,
 Kai Uwe Rommel, Onno van der Linden and Igor Mandrichenko.
 Permission is granted to any individual or institution to use, copy, or
 redistribute this software so long as all of the original files are included,
 that it is not sold for profit, and that this copyright notice is retained.

*/

/*
 *  trees.c by Jean-loup Gailly
 *
 *  This is a new version of im_ctree.c originally written by Richard B. Wales
 *  for the defunct implosion method.
 *
 *  PURPOSE
 *
 *      Encode various sets of source values using variable-length
 *      binary code trees.
 *
 *  DISCUSSION
 *
 *      The PKZIP "deflation" process uses several Huffman trees. The more
 *      common source values are represented by shorter bit sequences.
 *
 *      Each code tree is stored in the ZIP file in a compressed form
 *      which is itself a Huffman encoding of the lengths of
 *      all the code strings (in ascending order by source values).
 *      The actual code strings are reconstructed from the lengths in
 *      the UNZIP process, as described in the "application note"
 *      (APPNOTE.TXT) distributed as part of PKWARE's PKZIP program.
 *
 *  REFERENCES
 *
 *      Lynch, Thomas J.
 *          Data Compression:  Techniques and Applications, pp. 53-55.
 *          Lifetime Learning Publications, 1985.  ISBN 0-534-03418-7.
 *
 *      Storer, James A.
 *          Data Compression:  Methods and Theory, pp. 49-50.
 *          Computer Science Press, 1988.  ISBN 0-7167-8156-5.
 *
 *      Sedgewick, R.
 *          Algorithms, p290.
 *          Addison-Wesley, 1983. ISBN 0-201-06672-6.
 *
 *  INTERFACE
 *
 *      void ct_init (ush *attr, int *method)
 *          Allocate the match buffer, initialize the various tables and save
 *          the location of the internal file attribute (ascii/binary) and
 *          method (DEFLATE/STORE)
 *
 *      void ct_tally (int dist, int lc);
 *          Save the match info and tally the frequency counts.
 *
 *      long flush_block (char *buf, ulg stored_len, int eof)
 *          Determine the best encoding for the current block: dynamic trees,
 *          static trees or store, and output the encoded block to the zip
 *          file. Returns the total compressed length for the file so far.
 *
 */

/* #include <ctype.h> */
/* #include "zip.h" */



local int near ZCONST extra_lbits[LENGTH_CODES] /*:SHARED*/ /* extra bits for each length code */
   = {0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0};

local int near ZCONST extra_dbits[D_CODES] /*:SHARED*/ /* extra bits for each distance code */
   = {0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13};

local int near ZCONST extra_blbits[BL_CODES] /*:SHARED*/  /* extra bits for each bit length code */
   = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,3,7};

#define STORED_BLOCK 0
#define STATIC_TREES 1
#define DYN_TREES    2
/* The three kinds of block type */


/* ===========================================================================
 * Local data
 */





local ct_data near static_ltree[L_CODES+2]; /*:SHARED :INIT*/
/* The static literal tree. Since the bit lengths are imposed, there is no
 * need for the L_CODES extra codes used during heap construction. However
 * The codes 286 and 287 are needed to build a canonical tree (see ct_init
 * below).
 */

local ct_data near static_dtree[D_CODES]; /*:SHARED :INIT*/
/* The static distance tree. (Actually a trivial tree since all codes use
 * 5 bits.)
 */


typedef struct tree_desc {
    /* ct_data near *dyn_tree; */      /* the dynamic tree */
    ct_data near *static_tree;   /* corresponding static tree or NULL */
    int     near ZCONST*extra_bits;    /* extra bits for each code or NULL */
    int     extra_base;          /* base index for extra_bits */
    int     elems;               /* max number of elements in the tree */
    int     max_length;          /* max bit length for the codes */
    /* int     max_code; */            /* largest code with non zero frequency */
} tree_desc;

local tree_desc near ZCONST l_desc = /*:SHARED*/
{/*dyn_ltree,*/ static_ltree, extra_lbits, LITERALS+1, L_CODES, MAX_BITS/*, 0*/};

local tree_desc near ZCONST d_desc = /*:SHARED*/
{/*dyn_dtree,*/ static_dtree, extra_dbits, 0,          D_CODES, MAX_BITS/*, 0*/};

local tree_desc near ZCONST bl_desc = /*:SHARED*/
{/*bl_tree,*/ NULLL,       extra_blbits, 0,         BL_CODES, MAX_BL_BITS/*, 0*/};



local uch near ZCONST bl_order[BL_CODES] /*:SHARED*/
   = {16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15};
/* The lengths of the bit length codes are sent in order of decreasing
 * probability, to avoid transmitting the lengths for unused bit length codes.
 */


local uch length_code[MAX_MATCH-MIN_MATCH+1]; /*:SHARED :INIT*/
/* length code for each normalized match length (0 == MIN_MATCH) */

local uch dist_code[512]; /*:SHARED :INIT*/
/* distance codes. The first 256 values correspond to the distances
 * 3 .. 258, the last 256 values correspond to the top 8 bits of
 * the 15 bit distances.
 */

local int near base_length[LENGTH_CODES]; /*:SHARED :INIT*/
/* First normalized length for each code (0 = MIN_MATCH) */

local int near base_dist[D_CODES]; /*:SHARED :INIT*/
/* First normalized distance for each code (0 = distance of 1) */

#ifndef DYN_ALLOC
  error ::
  local uch far l_buf[LIT_BUFSIZE];  /*:NONEED_GLOBAL*/ /* buffer for literals/lengths */
  local ush far d_buf[DIST_BUFSIZE]; /*:NONEED_GLOBAL*/ /* buffer for distances */
#endif




#ifndef PTS_ZIP
local u32 compressed_len; /*:NONEED_GLOBAL*/ /* total bit length of compressed file */
ush *file_type;        /* pointer to UNKNOWN, BINARY or ASCII */
int *file_method;      /* pointer to DEFLATE or STORE */
local u32 input_len;      /*:NONEED_GLOBAL*/ /* total byte length of input file */
/* input_len is for debugging only since we can get it by other means. */
#endif

#ifdef DEBUG
extern ulg bits_sent;  /* bit length of the compressed data */
extern ulg isize;      /* byte length of input file */
#endif


/* ===========================================================================
 * Local (static) routines in this file.
 */

/* local void init_block     OF((void)); */
/* local void pqdownheap     OF((ct_data near *tree, int k));*/
/* local void gen_bitlen     OF((tree_desc near *desc)); */
local void gen_codes      OF((ct_data near *tree, int max_code, ush near *bl_count));
local int build_tree     OF((tree_desc near ZCONST*desc, ct_data near *tree DS1));
/* local void scan_tree      OF((ct_data near *tree, int max_code)); */
/* local void send_tree      OF((ct_data near *tree, int max_code)); */
/*local int  build_bl_tree  OF((void));*/
/*local void send_all_trees OF((int lcodes, int dcodes, int blcodes));*/
/*local void compress_block OF((ct_data near *ltree, ct_data near *dtree));*/
#ifndef PTS_ZIP
local void set_file_type  OF((void));
#endif

#ifndef DEBUG
#  define send_code(c, tree) send_bits(tree[c].Code, tree[c].Len, ds)
   /* Send a code of the given tree. c and tree must not have side effects */

#else /* DEBUG */
#  define send_code(c, tree) \
     { if (verbose>1) fprintf(stderr,"\ncd %3d ",(c)); \
       send_bits(tree[c].Code, tree[c].Len); }
#endif

#define d_code(dist) \
   ((dist) < 256 ? dist_code[dist] : dist_code[256+((dist)>>7)])
/* Mapping from a distance to a distance code. dist is the distance - 1 and
 * must not have side effects. dist_code[256] and dist_code[257] are never
 * used.
 */





/* ===========================================================================
 * Construct one Huffman tree and assigns the code bit strings and lengths.
 * Update the total bit length for the current block.
 * IN assertion: the field freq is set for all tree elements.
 * OUT assertions: the fields len and code are set to the optimal bit length
 *     and corresponding code. The length opt_len is updated; static_len is
 *     also updated if stree is not null. The field max_code is set.
 */
local int build_tree ___((tree_desc near ZCONST*desc, ct_data near *tree DS1),(desc, tree DS2),(
    tree_desc near *desc; /* the tree descriptor */
    ct_data near *tree
DS3 )) {
    /* ct_data near *tree   = desc->dyn_tree; */ /**** pts ****/
    ct_data near *stree  = desc->static_tree;
    int elems            = desc->elems;
    int n, m;          /* iterate over heap elements */
    int max_code = -1; /* largest code with non zero frequency */
    int node = elems;  /* next internal node of the tree */

    /* Construct the initial heap, with least frequent element in
     * heap[SMALLEST]. The sons of heap[n] are heap[2*n] and heap[2*n+1].
     * heap[0] is not used.
     */
    ds->heap_len = 0, ds->heap_max = HEAP_SIZE;

    for (n = 0; n < elems; n++) {
        if (tree[n].Freq != 0) {
            ds->heap[++ds->heap_len] = max_code = n;
            ds->depth[n] = 0;
        } else {
            tree[n].Len = 0;
        }
    }

    /* The pkzip format requires that at least one distance code exists,
     * and that at least one bit should be sent even if there is only one
     * possible code. So to avoid special checks later on we force at least
     * two codes of non zero frequency.
     */
    while (ds->heap_len < 2) {
        int new_ = ds->heap[++ds->heap_len] = (max_code < 2 ? ++max_code : 0);
        tree[new_].Freq = 1;
        ds->depth[new_] = 0;
        ds->opt_len--; if (stree) ds->static_len -= stree[new_].Len;
        /* new_ is 0 or 1 so it does not have extra bits */
    }
    /* desc->max_code = max_code; */ /**** pts ****/

    /* The elements heap[heap_len/2+1 .. heap_len] are leaves of the tree,
     * establish sub-heaps of increasing lengths:
     */
    for (n = ds->heap_len/2; n >= 1; n--) pqdownheap(tree, n, ds->heap,ds->heap_len,ds->depth);

    /* Construct the Huffman tree by repeatedly combining the least two
     * frequent nodes.
     */
    do {
        pqremove(tree, n);   /* n = node of least frequency */
        m = ds->heap[SMALLEST];  /* m = node of next least frequency */

        ds->heap[--ds->heap_max] = n; /* keep the nodes sorted by frequency */
        ds->heap[--ds->heap_max] = m;

        /* Create a new node father of n and m */
        tree[node].Freq = tree[n].Freq + tree[m].Freq;
        
        /**** pts ****/ /* Eliminated Max(...) */
        ds->depth[node] = (uch) ((ds->depth[n] > ds->depth[m] ? ds->depth[n] : ds->depth[m]) + 1);
        
        tree[n].Dad = tree[m].Dad = node;
#ifdef DUMP_BL_TREE
        if (tree == ds->bl_tree) {
            fprintf(stderr,"\nnode %d(%d), sons %d(%d) %d(%d)",
                    node, tree[node].Freq, n, tree[n].Freq, m, tree[m].Freq);
        }
#endif
        /* and insert the new node in the heap */
        ds->heap[SMALLEST] = node++;
        pqdownheap(tree, SMALLEST, ds->heap,ds->heap_len,ds->depth);

    } while (ds->heap_len >= 2);

    ds->heap[--ds->heap_max] = ds->heap[SMALLEST];

    /* At this point, the fields freq and dad are set. We can now
     * generate the bit lengths.
     */
    /**** pts ****/
    /* ===========================================================================
     * Compute the optimal bit lengths for a tree and update the total bit length
     * for the current block.
     * IN assertion: the fields freq and dad are set, heap[heap_max] and
     *    above are the tree nodes sorted by increasing frequency.
     * OUT assertions: the field len is set to the optimal bit length, the
     *     array bl_count contains the frequencies for each bit length.
     *     The length opt_len is updated; static_len is also updated if stree is
     *     not null.
     */
    #if 0
    local void gen_bitlen ___((tree_desc near *desc),(desc),(
        tree_desc near *desc; /* the tree descriptor */
    ))
    #endif
    {
        /* ct_data near *tree  = desc->dyn_tree; */ /**** pts ****/
        int near ZCONST*extra     = desc->extra_bits;
        int base            = desc->extra_base;
        /* int max_code        = desc->max_code; */ /**** pts ****/
        int max_length      = desc->max_length;
        ct_data near *stree = desc->static_tree;
        int h;              /* heap index */
        int n, m;           /* iterate over the tree elements */
        int bits;           /* bit length */
        int xbits;          /* extra bits */
        ush f;              /* frequency */
        int overflow = 0;   /* number of elements with bit length too large */

        for (bits = 0; bits <= MAX_BITS; bits++) ds->bl_count[bits] = 0;

        /* In a first pass, compute the optimal bit lengths (which may
         * overflow in the case of the bit length tree).
         */
        tree[ds->heap[ds->heap_max]].Len = 0; /* root of the heap */

        for (h = ds->heap_max+1; h < HEAP_SIZE; h++) {
            n = ds->heap[h];
            bits = tree[tree[n].Dad].Len + 1;
            if (bits > max_length) bits = max_length, overflow++;
            tree[n].Len = bits;
            /* We overwrite tree[n].Dad which is no longer needed */

            if (n > max_code) continue; /* not a leaf node */

            ds->bl_count[bits]++;
            xbits = 0;
            if (n >= base) xbits = extra[n-base];
            f = tree[n].Freq;
            ds->opt_len += (u32)f * (bits + xbits);
            if (stree) ds->static_len += (u32)f * (stree[n].Len + xbits);
        }
        if (overflow != 0) {
          Trace((stderr,"\nbit length overflow\n"));
          /* This happens for example on obj2 and pic of the Calgary corpus */

          /* Find the first bit length which could increase: */
          do {
              bits = max_length-1;
              while (ds->bl_count[bits] == 0) bits--;
              ds->bl_count[bits]--;      /* move one leaf down the tree */
              ds->bl_count[bits+1] += 2; /* move one overflow item as its brother */
              ds->bl_count[max_length]--;
              /* The brother of the overflow item also moves one step up,
               * but this does not affect ds->bl_count[max_length]
               */
              overflow -= 2;
          } while (overflow > 0);

          /* Now recompute all bit lengths, scanning in increasing frequency.
           * h is still equal to HEAP_SIZE. (It is simpler to reconstruct all
           * lengths instead of fixing only the wrong ones. This idea is taken
           * from 'ar' written by Haruhiko Okumura.)
           */
          for (bits = max_length; bits != 0; bits--) {
              n = ds->bl_count[bits];
              while (n != 0) {
                  m = ds->heap[--h];
                  if (m > max_code) continue;
                  if (tree[m].Len != (unsigned) bits) {
                      Trace((stderr,"code %d bits %d->%d\n", m, tree[m].Len, bits));
                      ds->opt_len += ((long)bits-(long)tree[m].Len)*(long)tree[m].Freq;
                      tree[m].Len = bits;
                  }
                  n--;
              }
          }
        }
    }

    /* The field len is now set, we can generate the bit codes */
    gen_codes ((ct_data near *)tree, max_code, ds->bl_count);
    
    return max_code;
} /* build_tree */





/* ===========================================================================
 * Determine the best encoding for the current block: dynamic trees, static
 * trees or store, and output the encoded block to the zip file. This function
 * returns the total compressed length for the file so far.
 */
static void flush_block ___((char *buf, u32 stored_len, int eof DS1), (buf, stored_len, eof DS2),(
    char *buf;        /* input block, or NULL if too old */
    u32 stored_len;   /* length of input block */
    int eof;          /* true if this is the last block for a file */
DS3 )) {
    u32 opt_lenb, static_lenb; /* opt_len and static_len in bytes */
    int max_blindex;  /* index of last bit length code of non zero freq */

    ds->flag_buf[ds->last_flags] = ds->flags; /* Save the flags for the last 8 items */

     /* Check if the file is ascii or binary */
#ifndef PTS_ZIP
    if (*file_type == (ush)UNKNOWN) set_file_type();
#endif

    /* Construct the literal and distance trees */
    ds->l_desc_max_code=build_tree((tree_desc near ZCONST*)(&l_desc), ds->dyn_ltree, ds);
    Tracev((stderr, "\nlit data: dyn %ld, stat %ld", opt_len, static_len));

    ds->d_desc_max_code=build_tree((tree_desc near ZCONST*)(&d_desc), ds->dyn_dtree, ds);
    Tracev((stderr, "\ndist data: dyn %ld, stat %ld", opt_len, static_len));
    /* At this point, opt_len and static_len are the total bit lengths of
     * the compressed block data, excluding the tree representations.
     */

    /* Build the bit length tree for the above two trees, and get the index
     * in bl_order of the last bit length code to send.
     */
    /**** pts ****/
    /* max_blindex = build_bl_tree(); */
    /* ===========================================================================
     * Construct the Huffman tree for the bit lengths and return the index in
     * bl_order of the last bit length code to send.
     */
    #if 0
    local int build_bl_tree(void)
    #endif
    {
        /*int max_blindex;*/  /* index of last bit length code of non zero freq */

        /* Determine the bit length frequencies for literal and distance trees */
        /**** pts ****/
        /* scan_tree((ct_data near *)dyn_ltree, l_desc.max_code);
         * scan_tree((ct_data near *)dyn_dtree, d_desc.max_code);
         */
        ct_data near *tree=ds->dyn_ltree;
        int max_code=ds->l_desc_max_code;
        /* ===========================================================================
         * Scan a literal or distance tree to determine the frequencies of the codes
         * in the bit length tree. Updates opt_len to take into account the repeat
         * counts. (The contribution of the bit length codes will be added later
         * during the construction of bl_tree.)
         */
        #if 0
        local void scan_tree ___((ct_data near *tree, int max_code),(tree, max_code),(
            ct_data near *tree; /* the tree to be scanned */
            int max_code;       /* and its largest code of non zero frequency */
        ))
        #endif
        while (1) {
            int n;                     /* iterates over all tree elements */
            int prevlen = -1;          /* last emitted length */
            int curlen;                /* length of current code */
            int nextlen = tree[0].Len; /* length of next code */
            int count = 0;             /* repeat count of the current code */
            int max_count = 7;         /* max repeat count */
            int min_count = 4;         /* min repeat count */
            
            

            if (nextlen == 0) max_count = 138, min_count = 3;
            tree[max_code+1].Len = (ush)-1; /* guard */

            for (n = 0; n <= max_code; n++) {
                curlen = nextlen; nextlen = tree[n+1].Len;
                if (++count < max_count && curlen == nextlen) {
                    continue;
                } else if (count < min_count) {
                    ds->bl_tree[curlen].Freq += count;
                } else if (curlen != 0) {
                    if (curlen != prevlen) ds->bl_tree[curlen].Freq++;
                    ds->bl_tree[REP_3_6].Freq++;
                } else if (count <= 10) {
                    ds->bl_tree[REPZ_3_10].Freq++;
                } else {
                    ds->bl_tree[REPZ_11_138].Freq++;
                }
                count = 0; prevlen = curlen;
                if (nextlen == 0) {
                    max_count = 138, min_count = 3;
                } else if (curlen == nextlen) {
                    max_count = 6, min_count = 3;
                } else {
                    max_count = 7, min_count = 4;
                }
            }
            if (tree==ds->dyn_dtree) break;
            tree=ds->dyn_dtree; max_code=ds->d_desc_max_code;
        }

        /* Build the bit length tree: */
        build_tree((tree_desc near ZCONST*)(&bl_desc), ds->bl_tree, ds);
        /* opt_len now includes the length of the tree representations, except
         * the lengths of the bit lengths codes and the 5+5+4 bits for the counts.
         */

        /* Determine the number of bit length codes to send. The pkzip format
         * requires that at least 4 bit length codes be sent. (appnote.txt says
         * 3 but the actual value used is 4.)
         */
        for (max_blindex = BL_CODES-1; max_blindex >= 3; max_blindex--) {
            if (ds->bl_tree[bl_order[max_blindex]].Len != 0) break;
        }
        /* Update opt_len to include the bit length tree and counts */
        ds->opt_len += 3*(max_blindex+1) + 5+5+4;
        Tracev((stderr, "\ndyn trees: dyn %ld, stat %ld", opt_len, static_len));

        /*return max_blindex;*/
    }

    /* Determine the best encoding. Compute first the block length in bytes */
    opt_lenb = (ds->opt_len+3+7)>>3;
    static_lenb = (ds->static_len+3+7)>>3;
    #ifndef PTS_ZIP
      input_len += stored_len; /* for debugging only */
    #endif

    Trace((stderr, "\nopt %lu(%lu) stat %lu(%lu) stored %lu lit %u dist %u ",
            opt_lenb, opt_len, static_lenb, static_len, stored_len,
            last_lit, last_dist));

    if (static_lenb <= opt_lenb) opt_lenb = static_lenb;

#ifndef PGP /* PGP can't handle stored blocks */
#ifndef PTS_ZIP /**** pts ****/
    /* If compression failed and this is the first and last block,
     * the whole file is transformed into a stored file:
     */
#ifdef FORCE_METHOD
    if (zip_pack_level == 1 && eof && compressed_len == 0L) { /* force stored file */
#else
    if (stored_len <= opt_lenb && eof && compressed_len == 0L && seekable()) { /* } */
#endif
        /* Since LIT_BUFSIZE <= 2*WSIZE, the input data must be there: */
        if (buf == NULL) error ("block vanished");

        copy_block(buf, (unsigned)stored_len, 0); /* without header */
        compressed_len = stored_len << 3;
        *file_method = STORE;
    } else
#endif    
#endif /* PGP */

#ifdef FORCE_METHOD
    if (zip_pack_level == 2 && buf != (char*)NULL) { /* force stored block */
#else
    if (stored_len+4 <= opt_lenb && buf != (char*)NULL) { /* } */
                       /* 4: two words for the lengths */
#endif
        /* The test buf != NULL is only necessary if LIT_BUFSIZE > WSIZE.
         * Otherwise we can't have processed more than WSIZE input bytes since
         * the last block flush, because compression would have been
         * successful. If LIT_BUFSIZE <= WSIZE, it is never too late to
         * transform a block into a stored block.
         */
        send_bits((STORED_BLOCK<<1)+eof, 3, ds);  /* send block type */
        #ifndef PTS_ZIP
          compressed_len = (compressed_len + 3 + 7) & ~7L;
          compressed_len += (stored_len + 4) << 3;
        #endif

        /**** pts ****/
        /* copy_block(buf, (unsigned)stored_len, 1); */ /* with header */
        /* ===========================================================================
         * Copy a stored block to the zip file, storing first the length and its
         * one's complement if requested.
         */
        #if 0
        void copy_block ___((char *block, unsigned len, int header),(block, len, header),(
            char *block;  /* the input data */
            unsigned len; /* its length */
            int header;   /* true if block header must be written */
        ))
        #endif
        {
            unsigned len=(unsigned)stored_len; /* Dat: unsigned conversion is OK here, stored_len is never too large */
            
            /**** pts ****/
            /* bi_windup(); */              /* align on byte boundary */
            /* ===========================================================================
             * Write out any remaining bits in an incomplete byte.
             */
            #if 0
            void bi_windup(void)
            #endif
            {
              if (ds->bi_valid!=0) {
                unsigned g=ds->bi_valid>8;
                if (ds->out_bufp<ds->out_buflast-g-3) {
                  *ds->out_bufp++ = (char) (ds->bi_buf & 0xff);
                  if (g) *ds->out_bufp++ = (char) ((ush)ds->bi_buf >> 8);
                } else flush_outbuf(ds->bi_buf, g+1, ds);
              }
              ds->bi_buf = 0;
              ds->bi_valid = 0;
              #ifdef DEBUG
                bits_sent = (bits_sent+7) & ~7;
              #endif
            }

            if (1 /*header*/) { /* Imp: test this */
              /**** pts ****/
              /* PUTSHORT((ush)len); PUTSHORT((ush)~len); */
              *ds->out_bufp++ = (char) (len & 0xff);
              *ds->out_bufp++ = (char) ((ush)(len) >> 8);
              *ds->out_bufp++ = (char) (~len & 0xff);
              *ds->out_bufp++ = (char) ((ush)~len >> 8);
              #ifdef DEBUG
                bits_sent += 2*16;
              #endif
            }
            if (1 /**** pts ds->zfile ****/) {
                if (ds->out_bufp != ds->out_buf && ds->interf.err<2 && 0!=(ds->interf.zpfwrite)(ds->out_buf, ds->out_bufp-ds->out_buf, ds->zfile))
                      ds->interf.err=ERROR_ZPFWRITE;
                      /* easy_zerror2("write error on zip file"); */ /*:ERROR*/
                if (ds->interf.err<2 && 0!=(ds->interf.zpfwrite)(buf /*block*/, len, ds->zfile))
                  ds->interf.err=ERROR_ZPFWRITE;
                  /* easy_zerror2("write error on zip file"); */ /*:ERROR*/
                ds->out_bufp=ds->out_buf;
            }
            #ifndef PTS_ZIP
            else if (out_offset + len > out_size) {
                error("output buffer too small for in-memory compression");
            } else {
                memcpy(ds->out_buf + out_offset, buf /*block*/, len);
                out_offset += len;
            }
            #endif
        #ifdef DEBUG
            bits_sent += (ulg)len<<3;
        #endif
        }
    
    } else { /**** pts ****/    
      ct_data near *ltree; /* literal tree */
      ct_data near *dtree; /* distance tree */

  #ifdef FORCE_METHOD
      if (zip_pack_level == 3) { /* force static trees } */
  #else
      if (static_lenb == opt_lenb) {
  #endif
          send_bits((STATIC_TREES<<1)+eof, 3, ds);
          #ifndef PTS_ZIP
            compressed_len += 3 + static_len;
          #endif
          /* compress_block((ct_data near *)static_ltree, (ct_data near *)static_dtree); */
          ltree=(ct_data near*)static_ltree; dtree=(ct_data near *)static_dtree;
          /* assert(0); */
      } else {
          send_bits((DYN_TREES<<1)+eof, 3, ds);
          
          /**** pts ***/
          /* send_all_trees(l_desc.max_code+1, d_desc.max_code+1, max_blindex+1); */
          /* ===========================================================================
           * Send the header for a block using dynamic Huffman trees: the counts, the
           * lengths of the bit length codes, the literal tree and the distance tree.
           * IN assertion: lcodes >= 257, dcodes >= 1, blcodes >= 4.
           */
          #if 0
          local void send_all_trees ___((int lcodes, int dcodes, int blcodes),(lcodes, dcodes, blcodes),(
              int lcodes, dcodes, blcodes; /* number of codes for each tree */
          ))
          #endif
          {
              int lcodes=ds->l_desc_max_code+1, dcodes=ds->d_desc_max_code+1, blcodes=max_blindex+1;
              
              int rank;                    /* index in bl_order */

              Assert (lcodes >= 257 && dcodes >= 1 && blcodes >= 4, "not enough codes");
              Assert (lcodes <= L_CODES && dcodes <= D_CODES && blcodes <= BL_CODES,
                      "too many codes");
              Tracev((stderr, "\nbl counts: "));
              send_bits(lcodes-257, 5, ds);
              /* not +255 as stated in appnote.txt 1.93a or -256 in 2.04c */
              send_bits(dcodes-1,   5, ds);
              send_bits(blcodes-4,  4, ds); /* not -3 as stated in appnote.txt */
              for (rank = 0; rank < blcodes; rank++) {
                  Tracev((stderr, "\nbl code %2d ", bl_order[rank]));
                  send_bits(ds->bl_tree[bl_order[rank]].Len, 3, ds);
              }
              Tracev((stderr, "\nbl tree: sent %ld", bits_sent));

              /**** pts ****/
              /* send_tree((ct_data near *)dyn_ltree, lcodes-1); */ /* send the literal tree */
              /* Tracev((stderr, "\nlit tree: sent %ld", bits_sent)); */
              /* send_tree((ct_data near *)dyn_dtree, dcodes-1); */ /* send the distance tree */
              /* Tracev((stderr, "\ndist tree: sent %ld", bits_sent)); */
              { ct_data near *tree=ds->dyn_ltree;
                int max_code=lcodes-1;

                /* ===========================================================================
                 * Send a literal or distance tree in compressed form, using the codes in
                 * bl_tree.
                 */
                #if 0
                local void send_tree ___((ct_data near *tree, int max_code),(tree, max_code),(
                    ct_data near *tree; /* the tree to be scanned */
                    int max_code;       /* and its largest code of non zero frequency */
                ))
                #endif
                while (1) {
                  int n;                     /* iterates over all tree elements */
                  int prevlen = -1;          /* last emitted length */
                  int curlen;                /* length of current code */
                  int nextlen = tree[0].Len; /* length of next code */
                  int count = 0;             /* repeat count of the current code */
                  int max_count = 7;         /* max repeat count */
                  int min_count = 4;         /* min repeat count */

                  /* tree[max_code+1].Len = -1; */  /* guard already set */
                  if (nextlen == 0) max_count = 138, min_count = 3;

                  for (n = 0; n <= max_code; n++) {
                      curlen = nextlen; nextlen = tree[n+1].Len;
                      if (++count < max_count && curlen == nextlen) {
                          continue;
                      } else if (count < min_count) {
                          do { send_code(curlen, ds->bl_tree); } while (--count != 0);

                      } else if (curlen != 0) {
                          if (curlen != prevlen) {
                              send_code(curlen, ds->bl_tree); count--;
                          }
                          Assert(count >= 3 && count <= 6, " 3_6?");
                          send_code(REP_3_6, ds->bl_tree); send_bits(count-3, 2, ds);

                      } else if (count <= 10) {
                          send_code(REPZ_3_10, ds->bl_tree); send_bits(count-3, 3, ds);

                      } else {
                          send_code(REPZ_11_138, ds->bl_tree); send_bits(count-11, 7, ds);
                      }
                      count = 0; prevlen = curlen;
                      if (nextlen == 0) {
                          max_count = 138, min_count = 3;
                      } else if (curlen == nextlen) {
                          max_count = 6, min_count = 3;
                      } else {
                          max_count = 7, min_count = 4;
                      }
                  }
                  if (tree==ds->dyn_dtree) break;
                  tree=ds->dyn_dtree; max_code=dcodes-1;
                }
              }
          }
          #ifndef PTS_ZIP
            compressed_len += 3 + opt_len;
          #endif
          
          /* compress_block((ct_data near *)dyn_ltree, (ct_data near *)dyn_dtree); */
          ltree=(ct_data near*)ds->dyn_ltree; dtree=(ct_data near *)ds->dyn_dtree;
      }

      /* ===========================================================================
       * Send the block data compressed using the given Huffman trees
       */
      #if 0
      local void compress_block ___((ct_data near *ltree, ct_data near *dtree), (ltree, dtree), (
          ct_data near *ltree; /* literal tree */
          ct_data near *dtree; /* distance tree */
      ))
      #endif
      {
          unsigned dist;      /* distance of matched string */
          int lc;             /* match length or unmatched char (if dist == 0) */
          unsigned lx = 0;    /* running index in l_buf */
          unsigned dx = 0;    /* running index in d_buf */
          unsigned fx = 0;    /* running index in flag_buf */
          uch flag = 0;       /* current flags */
          unsigned code;      /* the code to send */
          int extra;          /* number of extra bits to send */

          if (ds->last_lit != 0) do {
              if ((lx & 7) == 0) flag = ds->flag_buf[fx++];
              lc = ds->l_buf[lx++];
              if ((flag & 1) == 0) {
                  send_code(lc, ltree); /* send a literal byte */
                  Tracecv(isgraph(lc), (stderr," '%c' ", lc));
              } else {
                  /* Here, lc is the match length - MIN_MATCH */
                  code = length_code[lc];
                  send_code(code+LITERALS+1, ltree); /* send the length code */
                  extra = extra_lbits[code];
                  if (extra != 0) {
                      lc -= base_length[code];
                      send_bits(lc, extra, ds);        /* send the extra length bits */
                  }
                  dist = ds->d_buf[dx++];
                  /* Here, dist is the match distance - 1 */
                  code = d_code(dist);
                  Assert (code < D_CODES, "bad d_code");

                  send_code(code, dtree);       /* send the distance code */
                  extra = extra_dbits[code];
                  if (extra != 0) {
                      dist -= base_dist[code];
                      send_bits(dist, extra, ds);   /* send the extra distance bits */
                  }
              } /* literal or match pair ? */
              flag >>= 1;
          } while (lx < ds->last_lit);

          send_code(END_BLOCK, ltree);
      } /* compress_block() */

    } /**** pts ****/

/*    Assert (compressed_len == bits_sent, "bad compressed size"); */
    /* init_block(); */
    /* ===========================================================================
     * Initialize a new block.
     */
    #if 0
    local void init_block(void)
    #endif
    {
        int n; /* iterates over tree elements */

        /* Initialize the trees. */
        for (n = 0; n < L_CODES;  n++) ds->dyn_ltree[n].Freq = 0;
        for (n = 0; n < D_CODES;  n++) ds->dyn_dtree[n].Freq = 0;
        for (n = 0; n < BL_CODES; n++) ds->bl_tree[n].Freq = 0;

        ds->dyn_ltree[END_BLOCK].Freq = 1;
        ds->opt_len = ds->static_len = 0L;
        ds->last_lit = ds->last_dist = ds->last_flags = 0;
        ds->flags = 0; ds->flag_bit = 1;
    }

    if (eof) {
#ifndef PTS_ZIP
#if defined(PGP) && !defined(MMAP)
        /* Wipe out sensitive data for pgp */
# ifdef DYN_ALLOC
        extern uch *window;
# else
        extern uch window[];
# endif
        memset(window, 0, (unsigned)(2*WSIZE-1)); /* -1 needed if WSIZE=32K */
#else /* !PGP */
/*        Assert (input_len == isize, "bad input size"); */
#endif
#endif

        /**** pts ****/
        /* bi_windup(); */
        /* ===========================================================================
         * Write out any remaining bits in an incomplete byte.
         */
        #if 0
        void bi_windup(void)
        #endif
        {
          #if 0
            if (ds->bi_valid > 8) {
                PUTSHORT(bi_buf);
            } else if (ds->bi_valid > 0) {
                PUTBYTE(bi_buf);
            }
            #if 0
              if (1 /**** pts: ds->zfile != NULL ****/) {
                  if (out_offset != 0 && ds->interf.err<2 && 0!=(ds->interf.zpfwrite)(ds->out_buf, out_offset, ds->zfile))
                        ds->interf.err=ERROR_ZPFWRITE;
                        /* easy_zerror2("write error on zip file"); */ /*:ERROR*/
                  out_offset=0;
              }
            #endif
          #else /**** pts ****/
            if (ds->bi_valid!=0) {
              unsigned g=ds->bi_valid>8; /* Imp: test this (>8 and >30 seem to be equivalent) */
              if (ds->out_bufp<=ds->out_buflast-g) {
                *ds->out_bufp++ = (char) (ds->bi_buf & 0xff);
                if (g) *ds->out_bufp++ = (char) ((ush)ds->bi_buf >> 8);
              } else flush_outbuf(ds->bi_buf, g+1, ds);
            }
          #endif
          ds->bi_buf = 0;
          ds->bi_valid = 0;
          #ifdef DEBUG
            bits_sent = (bits_sent+7) & ~7;
          #endif
        }

        #ifndef PTS_ZIP
          compressed_len += 7;  /* align on byte boundary */
        #endif
    }
    Tracev((stderr,"\ncomprlen %lu(%lu) ", compressed_len>>3,
           compressed_len-7*eof));
    Trace((stderr, "\n"));

    #ifndef PTS_ZIP
      return compressed_len >> 3;
    #endif
}

/* ===========================================================================
 * Save the match info and tally the frequency counts. Return true if
 * the current block must be flushed.
 */
static int ct_tally ___((int dist, int lc DS1),(dist, lc DS2),(
    int dist;  /* distance of matched string */
    int lc;    /* match length-MIN_MATCH or unmatched char (if dist==0) */
DS3 )) {
    ds->l_buf[ds->last_lit++] = (uch)lc;
    if (dist == 0) {
        /* lc is the unmatched char */
        ds->dyn_ltree[lc].Freq++;
    } else {
        /* Here, lc is the match length - MIN_MATCH */
        dist--;             /* dist = match distance - 1 */
        Assert((ush)dist < (ush)MAX_DIST &&
               (ush)lc <= (ush)(MAX_MATCH-MIN_MATCH) &&
               (ush)d_code(dist) < (ush)D_CODES,  "ct_tally: bad match");

        ds->dyn_ltree[length_code[lc]+LITERALS+1].Freq++;
        ds->dyn_dtree[d_code(dist)].Freq++;

        ds->d_buf[ds->last_dist++] = dist;
        ds->flags |= ds->flag_bit;
    }
    ds->flag_bit <<= 1;

    /* Output the flags if they fill a byte: */
    if ((ds->last_lit & 7) == 0) {
        ds->flag_buf[ds->last_flags++] = ds->flags;
        ds->flags = 0, ds->flag_bit = 1;
    }
    /* Try to guess if it is profitable to stop the current block here */
    if (ds->interf.pack_level > 2 && (ds->last_lit & 0xfff) == 0) {
        /* Compute an upper bound for the compressed length */
        u32 out_length = (u32)ds->last_lit*8L;
        u32 in_length = (u32)ds->strstart-ds->block_start;
        int dcode;
        for (dcode = 0; dcode < D_CODES; dcode++) {
            out_length += (u32)ds->dyn_dtree[dcode].Freq*(5L+extra_dbits[dcode]);
        }
        out_length >>= 3;
        Trace((stderr,"\nlast_lit %u, last_dist %u, in %ld, out ~%ld(%ld%%) ",
               last_lit, last_dist, in_length, out_length,
               100L - out_length*100L/in_length));
        if (ds->last_dist < ds->last_lit/2 && out_length < in_length/2) return 1;
    }
    return (ds->last_lit == LIT_BUFSIZE-1 || ds->last_dist == DIST_BUFSIZE);
    /* We avoid equality with LIT_BUFSIZE because of wraparound at 64K
     * on 16 bit machines and because stored blocks are restricted to
     * 64K-1 bytes.
     */
}


#ifndef PTS_ZIP
/* ===========================================================================
 * Set the file type to ASCII or BINARY, using a crude approximation:
 * binary if more than 20% of the bytes are <= 6 or >= 128, ascii otherwise.
 * IN assertion: the fields freq of dyn_ltree are set and the total of all
 * frequencies does not exceed 64K (to fit in an int on 16 bit machines).
 */
local void set_file_type()
{
    int n = 0;
    unsigned ascii_freq = 0;
    unsigned bin_freq = 0;
    while (n < 7)        bin_freq += dyn_ltree[n++].Freq;
    while (n < 128)    ascii_freq += dyn_ltree[n++].Freq;
    while (n < LITERALS) bin_freq += dyn_ltree[n++].Freq;
    *file_type = bin_freq > (ascii_freq >> 2) ? BINARY : ASCII;
#ifndef PGP
#ifndef PTS_ZIP
    if (*file_type == BINARY && translate_eol) {
        zipwarn("-l used on binary file", "");
    }
#endif    
#endif
}
#endif

/* end of trees.c */

/* ---- deflate.c */

/*

 Copyright (C) 1990-1997 Mark Adler, Richard B. Wales, Jean-loup Gailly,
 Kai Uwe Rommel, Onno van der Linden and Igor Mandrichenko.
 Permission is granted to any individual or institution to use, copy, or
 redistribute this software so long as all of the original files are included,
 that it is not sold for profit, and that this copyright notice is retained.

*/

/*
 *  deflate.c by Jean-loup Gailly.
 *
 *  PURPOSE
 *
 *      Identify new text as repetitions of old text within a fixed-
 *      length sliding window trailing behind the new text.
 *
 *  DISCUSSION
 *
 *      The "deflation" process depends on being able to identify portions
 *      of the input text which are identical to earlier input (within a
 *      sliding window trailing behind the input currently being processed).
 *
 *      The most straightforward technique turns out to be the fastest for
 *      most input files: try all possible matches and select the longest.
 *      The key feature of this algorithm is that insertions into the string
 *      dictionary are very simple and thus fast, and deletions are avoided
 *      completely. Insertions are performed at each input character, whereas
 *      string matches are performed only when the previous match ends. So it
 *      is preferable to spend more time in matches to allow very fast string
 *      insertions and avoid deletions. The matching algorithm for small
 *      strings is inspired from that of Rabin & Karp. A brute force approach
 *      is used to find longer strings when a small match has been found.
 *      A similar algorithm is used in comic (by Jan-Mark Wams) and freeze
 *      (by Leonid Broukhis).
 *         A previous version of this file used a more sophisticated algorithm
 *      (by Fiala and Greene) which is guaranteed to run in linear amortized
 *      time, but has a larger average cost, uses more memory and is patented.
 *      However the F&G algorithm may be faster for some highly redundant
 *      files if the parameter max_chain_length (described below) is too large.
 *
 *  ACKNOWLEDGEMENTS
 *
 *      The idea of lazy evaluation of matches is due to Jan-Mark Wams, and
 *      I found it in 'freeze' written by Leonid Broukhis.
 *      Thanks to many info-zippers for bug reports and testing.
 *
 *  REFERENCES
 *
 *      APPNOTE.TXT documentation file in PKZIP 1.93a distribution.
 *
 *      A description of the Rabin and Karp algorithm is given in the book
 *         "Algorithms" by R. Sedgewick, Addison-Wesley, p252.
 *
 *      Fiala,E.R., and Greene,D.H.
 *         Data Compression with Finite Windows, Comm.ACM, 32,4 (1989) 490-595
 *
 *  INTERFACE
 *
 *      void lm_init (int pack_level, ush *flags)
 *          Initialize the "longest match" routines for a new file
 *
 *      ulg deflate (void)
 *          Processes a new input file and return its compressed length. Sets
 *          the compressed length, crc, deflate flags and internal file
 *          attributes.
 */

/* #include "zip.h" */


/* ===========================================================================
 * Configuration parameters
 */

/* Compile with MEDIUM_MEM to reduce the memory requirements or
 * with SMALL_MEM to use as little memory as possible. Use BIG_MEM if the
 * entire input file can be held in memory (not possible on 16 bit systems).
 * Warning: defining these symbols affects HASH_BITS (see below) and thus
 * affects the compression ratio. The compressed output
 * is still correct, and might even be smaller in some cases.
 */

#ifdef SMALL_MEM
#   define HASH_BITS  13  /* Number of bits used to hash strings */
#endif
#ifdef MEDIUM_MEM
#   define HASH_BITS  14
#endif
#ifndef HASH_BITS
#   define HASH_BITS  15
   /* For portability to 16 bit machines, do not use values above 15. */
#endif

#define HASH_SIZE (unsigned)(1<<HASH_BITS)
#define HASH_MASK (HASH_SIZE-1)
#define WMASK     (WSIZE-1)
/* HASH_SIZE and WSIZE must be powers of two */

#define NIL 0
/* Tail of hash chains */

#define FAST 4
#define SLOW 2
/* speed options for the general purpose bit flag */

#ifndef TOO_FAR
#  define TOO_FAR 4096
#endif
/* Matches of length 3 are discarded if their distance exceeds TOO_FAR */

#if 0 && defined(ASMV) && !defined(MSDOS16) && defined(DYN_ALLOC)
   error: DYN_ALLOC not yet supported in match.S or match32.asm
#endif

#ifdef MEMORY16
#  define MAXSEG_64K
#endif

/* ===========================================================================
 * Local data used by the "longest match" routines.
 */


#ifndef DYN_ALLOC
  error::
  zzz uch    window[2L*WSIZE]; /*:NONEED_GLOBAL*/
  /* Sliding window. Input bytes are read into the second half of the window,
   * and move to the first half later to keep a dictionary of at least WSIZE
   * bytes. With this organization, matches are limited to a distance of
   * WSIZE-MAX_MATCH bytes, but this ensures that IO is always
   * performed with a length multiple of the block size. Also, it limits
   * the window size to 64K, which is quite useful on MSDOS.
   * To do: limit the window size to WSIZE+CBSZ if SMALL_MEM (the code would
   * be less efficient since the data would have to be copied WSIZE/CBSZ times)
   */
  zzz Pos    prev[WSIZE]; /*:NONEED_GLOBAL*/
  /* Link to older string with same hash index. To limit the size of this
   * array to 64K, this link is maintained only for the last 32K strings.
   * An index in this array is thus a window index modulo 32K.
   */
  zzz Pos    head[HASH_SIZE]; /*:NONEED_GLOBAL*/
  /* Heads of the hash chains or NIL. If your compiler thinks that
   * HASH_SIZE is a dynamic value, recompile with -DDYN_ALLOC.
   */
#endif


/* zzz ulg window_size; --C++ */
/* window size, 2*WSIZE except for MMAP or BIG_MEM, where it is the
 * input file length plus MIN_LOOKAHEAD.
 */

/* zzz long block_start; --C++ */
/* window position at the beginning of the current output block. Gets
 * negative when the window is moved backwards.
 */


#define H_SHIFT  ((HASH_BITS+MIN_MATCH-1)/MIN_MATCH)
/* Number of bits by which ins_h and del_h must be shifted at each
 * input step. It must be such that after MIN_MATCH steps, the oldest
 * byte no longer takes part in the hash key, that is:
 *   H_SHIFT * MIN_MATCH >= HASH_BITS
 */


#define max_insert_length  ds->max_lazy_match
/* Insert new strings in the hash table only if the match length
 * is not greater than this length. This saves time but degrades compression.
 * max_insert_length is used only for compression levels <= 3.
 */



/* Values for max_lazy_match, good_match and max_chain_length, depending on
 * the desired pack level (0..9). The values given below have been tuned to
 * exclude worst case performance for pathological files. Better values may be
 * found for specific files.
 */

typedef struct config {
   ush good_length; /* reduce lazy search above this match length */
   ush max_lazy;    /* do not perform lazy search above this match length */
   ush nice_length; /* quit search above this match length */
   ush max_chain;
} config;

#ifdef  FULL_SEARCH
  error::
# define nice_match MAX_MATCH
#endif


local config configuration_table[10] = { /*:SHARED*/
/*      good lazy nice chain */
/* 0 */ {0,    0,  0,    0},  /* store only */
/* 1 */ {4,    4,  8,    4},  /* maximum speed, no lazy matches */
/* 2 */ {4,    5, 16,    8},
/* 3 */ {4,    6, 32,   32},

/* 4 */ {4,    4, 16,   16},  /* lazy matches */
/* 5 */ {8,   16, 32,   32},
/* 6 */ {8,   16, 128, 128},
/* 7 */ {8,   32, 128, 256},
/* 8 */ {32, 128, 258, 1024},
/* 9 */ {32, 258, 258, 4096}}; /* maximum compression */

/* Note: the deflate() code requires max_lazy >= MIN_MATCH and max_chain >= 4
 * For deflate_fast() (levels <= 3) good is ignored and lazy has a different
 * meaning.
 */

#define EQUAL 0
/* result of memcmp for equal strings */

/* ===========================================================================
 *  Prototypes for local functions.
 */

local int fill_window   OF((char**,char* DS1));

zzz      int  longest_match OF((IPos cur_match DS1));
#if 0 && defined(ASMV) && !defined(RISCOS)
      void match_init OF((void)); /* asm code initialization */
#endif

#ifdef DEBUG
local  void check_match OF((IPos start, IPos match, int length));
#endif

/* ===========================================================================
 * Update a hash value with the given input byte
 * IN  assertion: all calls to to UPDATE_HASH are made with consecutive
 *    input characters, so that a running hash key can be computed from the
 *    previous key instead of complete recalculation each time.
 */
#define UPDATE_HASH(h,c) (h = (((h)<<H_SHIFT) ^ (c)) & HASH_MASK)

/* ===========================================================================
 * Insert string s in the dictionary and set match_head to the previous head
 * of the hash chain (the most recent string with same hash key). Return
 * the previous length of the hash chain.
 * IN  assertion: all calls to to INSERT_STRING are made with consecutive
 *    input characters and the first MIN_MATCH bytes of s are valid
 *    (except for the last MIN_MATCH-1 bytes of the input file).
 */
#define INSERT_STRING(s, match_head) \
   (UPDATE_HASH(ds->ins_h, ds->window[(s) + (MIN_MATCH-1)]), \
    ds->prev[(s) & WMASK] = match_head = ds->head[ds->ins_h], \
    ds->head[ds->ins_h] = (s))


/* ===========================================================================
 * Set match_start to the longest match starting at the given string and
 * return its length. Matches shorter or equal to prev_length are discarded,
 * in which case the result is equal to prev_length and match_start is
 * garbage.
 * IN assertions: cur_match is the head of the hash chain for the current
 *   string (strstart) and its distance is <= MAX_DIST, and prev_length >= 1
 */
/*#ifndef ASMV*/
/* For 80x86 and 680x0 and ARM, an optimized version is in match.asm or
 * match.S. The code is functionally equivalent, so you can use the C version
 * if desired.
 */
zzz int longest_match ___((IPos cur_match DS1),(cur_match DS2),(
    IPos cur_match;                             /* current match */
DS3 )) {
    unsigned chain_length = ds->max_chain_length;   /* max hash chain length */
    register uch far *scan = ds->window + ds->strstart; /* current string */
    register uch far *match;                    /* matched string */
    register int len;                           /* length of current match */
    int best_len = ds->prev_length;                 /* best match length so far */
    IPos limit = ds->strstart > (IPos)MAX_DIST ? ds->strstart - (IPos)MAX_DIST : NIL;
    /* Stop when cur_match becomes <= limit. To simplify the code,
     * we prevent matches with the string of window index 0.
     */

/* The code is optimized for HASH_BITS >= 8 and MAX_MATCH-2 multiple of 16.
 * It is easy to get rid of this optimization if necessary.
 */
#if HASH_BITS < 8 || MAX_MATCH != 258
   error: Code too clever
#endif

#ifdef UNALIGNED_OK
    /* Compare two bytes at a time. Note: this is not always beneficial.
     * Try with and without -DUNALIGNED_OK to check.
     */
    register uch far *strend = ds->window + ds->strstart + MAX_MATCH - 1;
    register ush scan_start = *(ush far *)scan;
    register ush scan_end   = *(ush far *)(scan+best_len-1);
#else
    register uch far *strend = ds->window + ds->strstart + MAX_MATCH;
    register uch scan_end1  = scan[best_len-1];
    register uch scan_end   = scan[best_len];
#endif

    /* Do not waste too much time if we already have a good match: */
    if (ds->prev_length >= ds->good_match) {
        chain_length >>= 2;
    }
    Assert(ds->strstart <= ds->window_size-MIN_LOOKAHEAD, "insufficient lookahead");

    do {
        Assert(cur_match < ds->strstart, "no future");
        match = ds->window + cur_match;

        /* Skip to next match if the match length cannot increase
         * or if the match length is less than 2:
         */
#if (defined(UNALIGNED_OK) && MAX_MATCH == 258)
        /* This code assumes sizeof(unsigned short) == 2. Do not use
         * UNALIGNED_OK if your compiler uses a different size.
         */
        if (*(ush far *)(match+best_len-1) != scan_end ||
            *(ush far *)match != scan_start) continue;

        /* It is not necessary to compare scan[2] and match[2] since they are
         * always equal when the other bytes match, given that the hash keys
         * are equal and that HASH_BITS >= 8. Compare 2 bytes at a time at
         * strstart+3, +5, ... up to strstart+257. We check for insufficient
         * lookahead only every 4th comparison; the 128th check will be made
         * at strstart+257. If MAX_MATCH-2 is not a multiple of 8, it is
         * necessary to put more guard bytes at the end of the window, or
         * to check more often for insufficient lookahead.
         */
        scan++, match++;
        do {
        } while (*(ush far *)(scan+=2) == *(ush far *)(match+=2) &&
                 *(ush far *)(scan+=2) == *(ush far *)(match+=2) &&
                 *(ush far *)(scan+=2) == *(ush far *)(match+=2) &&
                 *(ush far *)(scan+=2) == *(ush far *)(match+=2) &&
                 scan < strend);
        /* The funny "do {}" generates better code on most compilers */

        /* Here, scan <= window+strstart+257 */
        Assert(scan <= ds->window+(unsigned)(ds->ds->window_size-1), "wild scan");
        if (*scan == *match) scan++;

        len = (MAX_MATCH - 1) - (int)(strend-scan);
        scan = strend - (MAX_MATCH-1);

#else /* UNALIGNED_OK */

        if (match[best_len]   != scan_end  ||
            match[best_len-1] != scan_end1 ||
            *match            != *scan     ||
            *++match          != scan[1])      continue;

        /* The check at best_len-1 can be removed because it will be made
         * again later. (This heuristic is not always a win.)
         * It is not necessary to compare scan[2] and match[2] since they
         * are always equal when the other bytes match, given that
         * the hash keys are equal and that HASH_BITS >= 8.
         */
        scan += 2, match++;

        /* We check for insufficient lookahead only every 8th comparison;
         * the 256th check will be made at strstart+258.
         */
        do {
        } while (*++scan == *++match && *++scan == *++match &&
                 *++scan == *++match && *++scan == *++match &&
                 *++scan == *++match && *++scan == *++match &&
                 *++scan == *++match && *++scan == *++match &&
                 scan < strend);

        Assert(scan <= ds->window+(unsigned)(ds->window_size-1), "wild scan");

        len = MAX_MATCH - (int)(strend - scan);
        scan = strend - MAX_MATCH;

#endif /* UNALIGNED_OK */

        if (len > best_len) {
            ds->match_start = cur_match;
            best_len = len;
            if (len >= ds->nice_match) break;
#ifdef UNALIGNED_OK
            scan_end = *(ush far *)(scan+best_len-1);
#else
            scan_end1  = scan[best_len-1];
            scan_end   = scan[best_len];
#endif
        }
    } while ((cur_match = ds->prev[cur_match & WMASK]) > limit
             && --chain_length != 0);

    return best_len;
}
/*#endif*/ /* ASMV */

#ifdef DEBUG
/* ===========================================================================
 * Check that the match at match_start is indeed a match.
 */
local void check_match ___(IPos start, IPos match, int length),(start, match, length),(
    IPos start, match;
    int length;
)) {
    /* check that the match is indeed a match */
    if (memcmp((char*)window + match,
                (char*)window + start, length) != EQUAL) {
        fprintf(stderr,
            " start %d, match %d, length %d\n",
            start, match, length);
        error("invalid match");
    }
    if (verbose > 1) {
        fprintf(stderr,"\\[%d,%d]", start-match, length);
#ifndef WINDLL
        do { putc(window[start++], stderr); } while (--length != 0);
#else
        do { fprintf(stdout,"%c",window[start++]); } while (--length != 0);
#endif
    }
}
#else
#  define check_match(start, match, length)
#endif



/* ===========================================================================
 * Fill the window when the lookahead becomes insufficient.
 * Updates strstart and lookahead, and sets eofile if end of input file.
 *
 * IN assertion: lookahead < MIN_LOOKAHEAD && strstart + lookahead > 0
 * OUT assertions: strstart <= ds->window_size-MIN_LOOKAHEAD
 *    At least one byte has been read, or eofile is set; file reads are
 *    performed for at least two bytes (required for the translate_eol option).
 *
 * @return true iff window cannot be filled because we're waiting for new data
 */
local int fill_window(char **avail_bufp, char *avail_bufend DS1)
{
    register unsigned n, m;
    unsigned more;    /* Amount of free space at the end of the window. */
    unsigned avail_len=avail_bufend-*avail_bufp;

    if (ds->zip_pts_state>=16) {
      assert(!ds->eofile);
      ds->zip_pts_state&=15;
      assert(ds->lookahead < MIN_LOOKAHEAD);
      /* goto continue_pending; */
    }

    do {
        more = (unsigned)(ds->window_size - (u32)ds->lookahead - (u32)ds->strstart);

        /* If the window is almost full and there is insufficient ds->lookahead,
         * move the upper half to the lower one to make room in the upper half.
         */
        if (more == (unsigned)-1 /*EOF*/) {
            /* Very unlikely, but possible on 16 bit machine if strstart == 0
             * and ds->lookahead == 1 (input done one byte at time)
             */
            more--;

        /* For MMAP or BIG_MEM, the whole input file is already in memory
         * so we must not perform sliding. We must however call file_read in
         * order to compute the crc, update ds->lookahead and possibly set eofile.
         */
        } else if (ds->strstart >= WSIZE+MAX_DIST && ds->sliding) {

            /* By the IN assertion, the window is not empty so we can't confuse
             * more == 0 with more == 64K on a 16 bit machine.
             */
            memcpy((char*)ds->window, (char*)ds->window+WSIZE, (unsigned)WSIZE);
            ds->match_start -= WSIZE;
            ds->strstart    -= WSIZE; /* we now have strstart >= MAX_DIST: */

            ds->block_start -= (long) WSIZE;

            for (n = 0; n < HASH_SIZE; n++) {
                m = ds->head[n];
                ds->head[n] = (Pos)(m >= WSIZE ? m-WSIZE : NIL);
            }
            for (n = 0; n < WSIZE; n++) {
                m = ds->prev[n];
                ds->prev[n] = (Pos)(m >= WSIZE ? m-WSIZE : NIL);
                /* If n is not on any hash chain, prev[n] is garbage but
                 * its value will never be used.
                 */
            }
            more += WSIZE;
#ifndef PTS_ZIP
#ifndef WINDLL
            if (verbose) putc('.', stderr);
#else
            if (verbose) fprintf(stdout,"%c",'.');
#endif
#endif
        }
        if (ds->eofile) return 0;

        /* If there was no sliding:
         *    strstart <= WSIZE+MAX_DIST-1 && ds->lookahead <= MIN_LOOKAHEAD - 1 &&
         *    more == ds->window_size - ds->lookahead - strstart
         * => more >= ds->window_size - (MIN_LOOKAHEAD-1 + WSIZE + MAX_DIST-1)
         * => more >= ds->window_size - 2*WSIZE + 2
         * In the BIG_MEM or MMAP case (not yet supported in gzip),
         *   ds->window_size == input_size + MIN_LOOKAHEAD  &&
         *   strstart + ds->lookahead <= input_size => more >= MIN_LOOKAHEAD.
         * Otherwise, ds->window_size == 2*WSIZE so more >= 2.
         * If there was sliding, more >= WSIZE. So in all cases, more >= 2.
         */
        Assert(more >= 2, "more < 2");
        
        if (avail_len==0) {
          if (ds->avail_eof) { ds->eofile=1; return 0; } /* Forced EOF. */
                    else { ds->zip_pts_state|=16; return 1; } /* May arrive more. */
        }
        assert(!ds->avail_eof);
        if (avail_len>more) {
          memcpy((char*)ds->window+ds->strstart+ds->lookahead, *avail_bufp, more);
          *avail_bufp+=more;
          ds->lookahead+=more;
          avail_len-=more;
        } else {
          memcpy((char*)ds->window+ds->strstart+ds->lookahead, *avail_bufp, avail_len);
          *avail_bufp+=avail_len;
          ds->lookahead+=avail_len;
          avail_len=0;
        }
#ifndef PTS_ZIP
        n = (*read_buf)((char*)ds->window+strstart+ds->lookahead, more);
        if (n == 0 || n == (unsigned)EOF) {
            ds->eofile = 1;
        } else {
            ds->lookahead += n;
        }
#endif
    } while (ds->lookahead < MIN_LOOKAHEAD && !ds->eofile);
    return 0;
}

/* ===========================================================================
 * Flush the current block, with given end-of-file flag.
 * IN assertion: strstart is set to the end of the current match.
 */
#define FLUSH_BLOCK(eof) \
   flush_block(ds->block_start >= 0L ? (char*)&ds->window[(unsigned)ds->block_start] : \
                (char*)NULL, (long)ds->strstart - ds->block_start, (eof), ds)

#ifndef PTS_ZIP
local ulg deflate_fast   OF((void));
/* ===========================================================================
 * Processes a new input file and return its compressed length. This
 * function does not perform lazy evaluationof matches and inserts
 * new strings in the dictionary only for unmatched strings or for short
 * matches. It is used only for the fast compression options.
 */
local ulg deflate_fast()
{
    IPos hash_head; /* head of the hash chain */
    int flush;      /* set if current block must be flushed */
    unsigned match_length = 0;  /* length of best match */

}
#endif

zzz void mem_cleanup(pts_defl_free_t zpffree DS1) {
  if (ds->d_buf !=NULL) { zpffree(ds->d_buf); ds->d_buf=NULLL; }
  if (ds->l_buf !=NULL) { zpffree(ds->l_buf); ds->l_buf=NULLL; }
  if (ds->window!=NULL) { zpffree(ds->window);ds->window=NULLL;}
  if (ds->prev  !=NULL) { zpffree(ds->prev);  ds->prev=NULLL;  }
  if (ds->head  !=NULL) { zpffree(ds->head);  ds->head=NULLL;  }
  #if 0 /* we don't signal EOF, deliberately */
  if (ds->interf.zpfwrite!=NULL) {
    ds->interf.zpfwrite("",0,ds->interf.zfile); /* signal EOF to subsequent levels */
    ds->interf.zpfwrite=NULLL;
    if (ds->zip_pts_state==ERROR_WORKING) ds->zip_pts_state=ERROR_DONE;
  }
  #endif
}
zzz void delete2(struct pts_defl_interface*fs) {
  /* struct pts_defl_internal_state *ds=fs->ds; */
  mem_cleanup(fs->zpffree, fs->ds);
  fs->zpffree(fs->ds); /* deallocates both ds and fs */
}

/* ===========================================================================
 * Same as above, but achieves better compression. We use a lazy
 * evaluation for matches: a match is finally adopted only if there is
 * no better match at the next window position.
 */
zzz void deflate2 ___((char *avail_buf, unsigned avail_len__, struct pts_defl_interface*fs),
  (avail_buf, avail_len__, fs DS2),(
  char *avail_buf;
  unsigned avail_len__;
  struct pts_defl_interface *fs;
)) {
    char *avail_bufend=avail_buf+avail_len__;
    IPos hash_head;          /* head of hash chain */
    IPos prev_match;         /* previous match */
    int flush;               /* set if current block must be flushed */
    int match_available=0/*gcc*/;   /* set if previous match exists */
    register unsigned match_length=0/*gcc*/; /* length of best match */
#ifdef DEBUG
    extern ulg isize;        /* byte length of input file, for debug only */
#endif
    DS3
 
    ds=fs->ds;
    /* assert(ds->interf.err==fs->err); */
    if (ds->interf.err!=ERROR_WORKING) {
      if (ds->interf.err==0) ds->interf.err=ERROR_PASTEOF;
      return;
    }

    /* avail_buf=avail_buf_; avail_len=avail_len_; */
    if (avail_len__==0) ds->avail_eof=1;
    if (ds->zip_pts_state>=16 && fill_window(&avail_buf, avail_bufend, ds)) goto ret_;

    if (ds->zip_pts_state==0) {
      /* lm_init(); should have been called before! */
      /* ds->lookahead = (*read_buf)((char*)ds->window, more); */
      register unsigned more = WSIZE;
      #ifndef MAXSEG_64K
          if (sizeof(int) > 2) more <<= 1; /* Can read 64K in one step */
      #endif
      if (avail_bufend==avail_buf) {
        if (ds->avail_eof) { ds->eofile=1; ds->lookahead=0; } /* Forced EOF. */
                  else assert(0); /* May arrive more. */
      } else {
        ds->eofile=0;
        if (1U*(avail_bufend-avail_buf)>more) {
          memcpy((char*)ds->window, avail_buf, more);
          avail_buf+=more;
          ds->lookahead=more;
          /* avail_len-=more; */
        } else {
          memcpy((char*)ds->window, avail_buf, avail_bufend-avail_buf);
          ds->lookahead=avail_bufend-avail_buf;
          avail_buf=avail_bufend;
          /* avail_len=0; */
        }
      }
      assert(ds->zip_pts_state==0);
      ds->zip_pts_state=1;
    }
    if (ds->zip_pts_state==1) {
      /* Make sure that we always have enough ds->lookahead. This is important
       * if input comes from a device such as a tty.
       */
      if (ds->lookahead < MIN_LOOKAHEAD && fill_window(&avail_buf, avail_bufend, ds)) goto ret_;
      ds->zip_pts_state=2;
    }
    if (ds->zip_pts_state==2) {
      unsigned j;
      ds->ins_h = 0;
      for (j=0; j<MIN_MATCH-1; j++) UPDATE_HASH(ds->ins_h, ds->window[j]);
      /* If ds->lookahead < MIN_MATCH, ins_h is garbage, but this is
       * not important since only literal bytes will be emitted.
       */
      ds->prev_length = MIN_MATCH-1; /* for deflate_fast() */
      ds->zip_pts_state=3;
    }
    if (ds->zip_pts_state==3) {
      if (ds->interf.pack_level<=3) { /* deflate_fast() */
        while (ds->lookahead != 0) {
            /* Insert the string window[strstart .. strstart+2] in the
             * dictionary, and set hash_head to the head of the hash chain:
             */
            INSERT_STRING(ds->strstart, hash_head);

            /* Find the longest match, discarding those <= prev_length.
             * At this point we have always match_length < MIN_MATCH
             */
            if (hash_head != NIL && ds->strstart - hash_head <= MAX_DIST) {
                /* To simplify the code, we prevent matches with the string
                 * of window index 0 (in particular we have to avoid a match
                 * of the string with itself at the start of the input file).
                 */
    #ifndef HUFFMAN_ONLY
                match_length = longest_match (hash_head, ds);
    #endif
                /* longest_match() sets match_start */
                if (match_length > ds->lookahead) match_length = ds->lookahead;
            }
            if (match_length >= MIN_MATCH) {
                check_match(ds->strstart, ds->match_start, match_length);

                flush = ct_tally(ds->strstart-ds->match_start, match_length - MIN_MATCH, ds);

                ds->lookahead -= match_length;

                /* Insert new strings in the hash table only if the match length
                 * is not too large. This saves time but degrades compression.
                 */
                if (match_length <= max_insert_length) {
                    match_length--; /* string at strstart already in hash table */
                    do {
                        ds->strstart++;
                        INSERT_STRING(ds->strstart, hash_head);
                        /* strstart never exceeds WSIZE-MAX_MATCH, so there are
                         * always MIN_MATCH bytes ahead. If ds->lookahead < MIN_MATCH
                         * these bytes are garbage, but it does not matter since
                         * the next ds->lookahead bytes will be emitted as literals.
                         */
                    } while (--match_length != 0);
                    ds->strstart++;
                } else {
                    ds->strstart += match_length;
                    match_length = 0;
                    ds->ins_h = ds->window[ds->strstart];
                    UPDATE_HASH(ds->ins_h, ds->window[ds->strstart+1]);
    #if MIN_MATCH != 3
                    Call UPDATE_HASH() MIN_MATCH-3 more times
    #endif
                }
            } else {
                /* No match, output a literal byte */
                Tracevv((stderr,"%c",window[ds->strstart]));
                flush = ct_tally (0, ds->window[ds->strstart], ds);
                ds->lookahead--;
                ds->strstart++;
            }
            if (flush) FLUSH_BLOCK(0), ds->block_start = ds->strstart;

            /* Make sure that we always have enough ds->lookahead, except
             * at the end of the input file. We need MAX_MATCH bytes
             * for the next match, plus MIN_MATCH bytes to insert the
             * string following the next match.
             */
            if (ds->lookahead < MIN_LOOKAHEAD && fill_window(&avail_buf, avail_bufend, ds)) goto ret_;
        }
      } else { /* normal, non-fast deflate */
        match_available=ds->saved_match_available;
        match_length=ds->saved_match_length;
        /* Process the input block. */
        while (ds->lookahead != 0) {
            /* Insert the string window[strstart .. strstart+2] in the
             * dictionary, and set hash_head to the head of the hash chain:
             */
            INSERT_STRING(ds->strstart, hash_head);

            /* Find the longest match, discarding those <= prev_length.
             */
            ds->prev_length = match_length, prev_match = ds->match_start;
            match_length = MIN_MATCH-1;

            if (hash_head != NIL && ds->prev_length < ds->max_lazy_match &&
                ds->strstart - hash_head <= MAX_DIST) {
                /* To simplify the code, we prevent matches with the string
                 * of window index 0 (in particular we have to avoid a match
                 * of the string with itself at the start of the input file).
                 */
    #ifndef HUFFMAN_ONLY
                match_length = longest_match (hash_head, ds);
    #endif
                /* longest_match() sets match_start */
                if (match_length > ds->lookahead) match_length = ds->lookahead;

    #ifdef FILTERED
                /* Ignore matches of length <= 5 */
                if (match_length <= 5) {
    #else
                /* Ignore a length 3 match if it is too distant: */
                if (match_length == MIN_MATCH && ds->strstart-ds->match_start > TOO_FAR){ /* } */
    #endif
                    /* If prev_match is also MIN_MATCH, match_start is garbage
                     * but we will ignore the current match anyway.
                     */
                    match_length = MIN_MATCH-1;
                }
            }
            /* If there was a match at the previous step and the current
             * match is not better, output the previous match:
             */
            if (ds->prev_length >= MIN_MATCH && match_length <= ds->prev_length) {

                check_match(ds->strstart-1, prev_match, ds->prev_length);

                flush = ct_tally(ds->strstart-1-prev_match, ds->prev_length - MIN_MATCH, ds);

                /* Insert in hash table all strings up to the end of the match.
                 * strstart-1 and strstart are already inserted.
                 */
                ds->lookahead -= ds->prev_length-1;
                ds->prev_length -= 2;
                do {
                    ds->strstart++;
                    INSERT_STRING(ds->strstart, hash_head);
                    /* strstart never exceeds WSIZE-MAX_MATCH, so there are
                     * always MIN_MATCH bytes ahead. If ds->lookahead < MIN_MATCH
                     * these bytes are garbage, but it does not matter since the
                     * next ds->lookahead bytes will always be emitted as literals.
                     */
                } while (--ds->prev_length != 0);
                match_available = 0;
                match_length = MIN_MATCH-1;
                ds->strstart++;

                if (flush) FLUSH_BLOCK(0), ds->block_start = ds->strstart;

            } else if (match_available) {
                /* If there was no match at the previous position, output a
                 * single literal. If there was a match but the current match
                 * is longer, truncate the previous match to a single literal.
                 */
                Tracevv((stderr,"%c",window[strstart-1]));
                if (ct_tally (0, ds->window[ds->strstart-1], ds)) {
                    FLUSH_BLOCK(0), ds->block_start = ds->strstart;
                }
                ds->strstart++;
                ds->lookahead--;
            } else {
                /* There is no previous match to compare with, wait for
                 * the next step to decide.
                 */
                match_available = 1;
                ds->strstart++;
                ds->lookahead--;
            }
    /*        Assert (ds->strstart <= isize && ds->lookahead <= isize, "a bit too far"); */ /**** pts ****/

            /* Make sure that we always have enough ds->lookahead, except
             * at the end of the input file. We need MAX_MATCH bytes
             * for the next match, plus MIN_MATCH bytes to insert the
             * string following the next match.
             */
            if (ds->lookahead < MIN_LOOKAHEAD && fill_window(&avail_buf, avail_bufend, ds)) {
              ds->saved_match_available=match_available;
              ds->saved_match_length=match_length;
              goto ret_;
            }
        }
        if (match_available) ct_tally (0, ds->window[ds->strstart-1], ds);
      } /* IF fast/normal deflate */
      assert(ds->zip_pts_state==3);
      /* fprintf(stderr, "HI!\n"); */
      FLUSH_BLOCK(1); /* BUGFIX01 at Sun Mar  3 17:27:59 CET 2002 */
      if (ds->out_bufp != ds->out_buf && ds->interf.err<2 && 0!=(ds->interf.zpfwrite)(ds->out_buf, ds->out_bufp-ds->out_buf, ds->zfile))
            ds->interf.err=ERROR_ZPFWRITE;
            /* easy_zerror2("write error on zip file"); */ /*:ERROR*/
      ds->zip_pts_state=4;
      if (ds->interf.err<2) ds->interf.err=ERROR_DONE;
      /* fs->err=ds->interf.err; */
      goto free_;
    }
    
    /*fprintf(stderr,"st=%u\n", ds->zip_pts_state);*/ assert(0);
   ret_:
    if (/*(fs->err=ds->zip_pts_error)*/ ds->interf.err !=ERROR_WORKING) {
     free_: /* Done. De-allocate structures. */
      assert(ds->zip_pts_state==4 || ds->interf.err>=2);
      mem_cleanup(fs->zpffree, ds); /* may change ds->interf.err? */

      /**** pts ****/
      /* lm_free(); */
      /* ===========================================================================
       * Free the window and hash table; nothing to do.
       */
      /* pts_defl_delete() does its task */ 
    }
}

/* --- */

static char need_ct_init=1;

/**** pts ****/
/* void pts_deflate_init ___((struct pts_defl_interface *fs),
  (fs),
  (struct pts_defl_interface *fs;)) { */
struct pts_defl_interface* pts_defl_new(
  pts_defl_zpfwrite_t zpfwrite_,
  pts_defl_malloc_t   zpfmalloc_,
  pts_defl_free_t     zpffree_,
  int pack_level_,
  void *zfile_) {
  struct pts_defl_interface* fs;
  DS3

  if (pack_level_ < 1 || pack_level_ > 9) {
    /* fs->err=ERROR_BADPL; */
    /*easy_zerror2("bad pack level");*/ /*:ERROR*/
    return NULLL; /* Imp: better return indication */
  }
  
  /* Allocate. */
# define zcalloc(a,b) zpfmalloc_(0U+(a)*(b)) /* Dat: always allocate <=65535 bytes */
  if (NULL==(ds=(struct pts_defl_internal_state*)zcalloc(1, sizeof*ds))) {
    /* fs->err=ERROR_MEM_DS; pts_defl_delete(fs); return; */
    return NULLL;
  }
  fs=&ds->interf;
  fs->ds=ds;
  fs->zpfwrite=zpfwrite_;
  fs->zpfmalloc=zpfmalloc_;
  fs->zpffree=zpffree_;
  fs->pack_level=pack_level_;
  fs->zfile=zfile_;
  fs->deflate2=deflate2;
  fs->delete2=delete2;
  fs->err=/*ds->interf.err=*/ERROR_WORKING;
  /* fs->other=0; */

  /* clear dynalloc pointers */
  ds->d_buf=NULLL;
  ds->l_buf=NULLL;
  ds->window=NULLL;
  ds->prev=ds->head=NULLL;
  /* vvv Dat: malloc() is enough, calloc() is not required */

/* ===========================================================================
 * Initialize the bit string routines.
 */
  /* bi_init(zipfile:fs->zfile); */
    ds->zfile  = fs->zfile;
    ds->bi_buf = 0;
    ds->bi_valid = 0;
#ifdef DEBUG
    bits_sent = 0L;
#endif
    /* Set the defaults for file compression. They are set by memcompress
     * for in-memory compression.
     */
    if (1 /**** pts ds->zfile != NULL ****/) {
      #if 0
        out_bufp=out_buf = file_outbuf;
        out_buflast = out_buf+sizeof(file_outbuf)-1;
      #else
        ds->out_bufp=ds->out_buf;
        ds->out_buflast = ds->out_buf+sizeof(ds->out_buf)-1;
      #endif
        assert(ds->out_buflast-ds->out_buf+1>=6);
#ifndef PTS_ZIP
        read_buf  = file_read;
#endif
    }

  /* printf("pl=%u\n", fs->pack_level); */

  /**** pts ****/
  /* ct_init(0,0); */
  /* ===========================================================================
   * Allocate the match buffer, initialize the various tables and save the
   * location of the internal file attribute (ascii/binary) and method
   * (DEFLATE/STORE).
   */
  #if 0
  void ct_init ___((ush *attr, int *method),(attr, method),(
      ush  *attr;   /* pointer to internal file attribute */
      int  *method; /* pointer to compression method */
  ))
  #endif

  /* Reentrancy note: this library (pts_defl.c) is reentrant and thread-safe.
   * Thread-safety is automatically achieved if there is no need for
   * synchronization, threads are required not to use objects allocated by
   * the library concurrently, _and_ _any_ of the following holds:
   *
   * -- there are no global (process-wide) variables (in the library)
   * -- global variables are ``const'', i.e their initial value is determined
   *    at process load time, before the very first thread begins running
   * -- global variables that are not ``const'' are assigned by _each_ thread
   *    to the same value (reentrantly!), before using any of them
   * -- global variables that are not ``const'' are assigned by _each_ thread
   *    to the same value, before using any of them, unless a flag indicates
   *    that _all_ initial assignments have been already made,
   *    _and_ the initial assigner function is reentrant. Note it is still
   *    possible that multiple threads are executing the initial assigner
   *    function concurrently.
   *
   * For pts_defl.c, the last one is the case. need_ct_init is the flag, and
   * the initial assigner function is inside the just following `if'.
   */
   
  if (need_ct_init) {
      int n;        /* iterates over tree elements */
      int bits;     /* bit counter */
      int length;   /* length value */
      int code;     /* code value */
      int dist;     /* distance index */

  #if 0
    #ifdef PTS_ZIP
      (void)attr;
      (void)method;
    #else
      file_type = attr;
      file_method = method;
      compressed_len = 0L;
      input_len = 0L;
    #endif
      if (static_dtree[0].Len != 0) return; /* ct_init already called */
    #else
  #endif

  #ifdef DYN_ALLOC
      if (NULL==(ds->d_buf = (ush far *) zcalloc(DIST_BUFSIZE, sizeof(ush)))
       || NULL==(ds->l_buf = (uch far *) zcalloc(LIT_BUFSIZE/2, 2))) {
          /* Avoid using the value 64K on 16 bit machines */
        /* fs->err=ERROR_MEM_CT; pts_defl_delete(fs); return; */
        delete2(fs); return NULLL;

        /*easy_zerror2("ct_init: out of memory");*/ /*:ERROR*/
      }
  #endif

      /* Initialize the mapping length (0..255) -> length code (0..28) */
      length = 0;
      for (code = 0; code < LENGTH_CODES-1; code++) {
          base_length[code] = length;
          for (n = 0; n < (1<<extra_lbits[code]); n++) {
              length_code[length++] = (uch)code;
          }
      }
      Assert (length == 256, "ct_init: length != 256");
      /* Note that the length 255 (match length 258) can be represented
       * in two different ways: code 284 + 5 bits or code 285, so we
       * overwrite length_code[255] to use the best encoding:
       */
      length_code[length-1] = (uch)code;

      /* Initialize the mapping dist (0..32K) -> dist code (0..29) */
      dist = 0;
      for (code = 0 ; code < 16; code++) {
          base_dist[code] = dist;
          for (n = 0; n < (1<<extra_dbits[code]); n++) {
              dist_code[dist++] = (uch)code;
          }
      }
      Assert (dist == 256, "ct_init: dist != 256");
      dist >>= 7; /* from now on, all distances are divided by 128 */
      for ( ; code < D_CODES; code++) {
          base_dist[code] = dist << 7;
          for (n = 0; n < (1<<(extra_dbits[code]-7)); n++) {
              dist_code[256 + dist++] = (uch)code;
          }
      }
      Assert (dist == 256, "ct_init: 256+dist != 512");

      /* Construct the codes of the static literal tree */
      for (bits = 0; bits <= MAX_BITS; bits++) ds->bl_count[bits] = 0;
      n = 0;
      while (n <= 143) static_ltree[n++].Len = 8, ds->bl_count[8]++;
      while (n <= 255) static_ltree[n++].Len = 9, ds->bl_count[9]++;
      while (n <= 279) static_ltree[n++].Len = 7, ds->bl_count[7]++;
      while (n <= 287) static_ltree[n++].Len = 8, ds->bl_count[8]++;
      /* Codes 286 and 287 do not exist, but we must include them in the
       * tree construction to get a canonical Huffman tree (longest code
       * all ones)
       */
      gen_codes((ct_data near *)static_ltree, L_CODES+1, ds->bl_count);
      /* ^^^ this call to gen_codes() is reentrant (verified) */

      /* The static distance tree is trivial: */
      for (n = 0; n < D_CODES; n++) {
          static_dtree[n].Len = 5;

          /**** pts ****/
          /*static_dtree[n].Code = bi_reverse(n, 5);*/
          static_dtree[n].Code=((n>>4)&1) | ((n>>2)&2) | (n&4) | ((n<<2)&8) | ((n<<4)&16);
          /* ^^^ BUGFIX discovered by gcc at Wed Aug 21 17:53:29 CEST 2002 */
      }

      /* Initialize the first block of the first file: */
      /* init_block(); */
      /* ===========================================================================
       * Initialize a new block.
       */
      #if 0
      local void init_block(void)
      #endif
      {
          int n; /* iterates over tree elements */

          /* Initialize the trees. */
          for (n = 0; n < L_CODES;  n++) ds->dyn_ltree[n].Freq = 0;
          for (n = 0; n < D_CODES;  n++) ds->dyn_dtree[n].Freq = 0;
          for (n = 0; n < BL_CODES; n++) ds->bl_tree[n].Freq = 0;

          ds->dyn_ltree[END_BLOCK].Freq = 1;
          ds->opt_len = ds->static_len = 0L;
          ds->last_lit = ds->last_dist = ds->last_flags = 0;
          ds->flags = 0; ds->flag_bit = 1;
      }
      need_ct_init=0; /* Clear the flag _after_ doing all assignments */
  }

  
  /**** pts ****/
  /* lm_init(); */
  /* ===========================================================================
   * Initialize the "longest match" routines for a new file
   *
   * IN assertion: ds->window_size is > 0 if the input file is already read or
   *    mmap'ed in the window[] array, 0 otherwise. In the first case,
   *    ds->window_size is sufficient to contain the whole input file plus
   *    MIN_LOOKAHEAD bytes (to avoid referencing memory beyond the end
   *    of window[] when looking for matches towards the end).
   */
  #if 0
  #if PTS_ZIP
  zzz void lm_init(void) {
  #else
  zzz void lm_init ___((int pack_level, ush *flags),(pack_level, flags),(
      int pack_level; /* 0: store, 1: best speed, 9: best compression */
      ush *flags;     /* general purpose bit flag */
  ))
  #endif
  #endif
  {
      /* assert(ds->zip_pts_state==0 || ds->zip_pts_state==4); ds->zip_pts_state=0; */
      ds->window_size=0; /* BUGFIX found by __CHECKER__ at Sat Jul  6 16:57:07 CEST 2002 */


      /* Do not slide the window if the whole input is already in memory
       * (ds->window_size > 0)
       */
      ds->sliding = 0;
      if (ds->window_size == 0L) {
          ds->sliding = 1;
          ds->window_size = (u32)2*WSIZE;
      }

      /* Use dynamic allocation if compiler does not like big static arrays: */
  #ifdef DYN_ALLOC
      if (1 /*window == NULL*/) {
          ds->window = (uch far *) zcalloc(WSIZE,   2*sizeof(uch));
          if (ds->window == NULL) {
            /*ziperr(ZE_MEM,*/ /*easy_zerror2("out of memory for window allocation");*/ /*:ERROR*/
            /* fs->err=ERROR_MEM_WIN; delete2(fs); return; */
            delete2(fs); return NULLL;
          }
      }
      if (1 /*ds->prev == NULL*/) {
          ds->prev   = (Pos far *) zcalloc(WSIZE,     sizeof(Pos));
          ds->head   = (Pos far *) zcalloc(HASH_SIZE, sizeof(Pos));
          if (ds->prev == NULL || ds->head == NULL) {
            /*ziperr(ZE_MEM,*/ /*easy_zerror2("out of memory for hash table allocation");*/ /*:ERROR*/
            /* fs->err=ERROR_MEM_HT; delete2(fs); return; */
            delete2(fs); return NULLL;
          }
      }
  #endif /* DYN_ALLOC */

      /* Initialize the hash table (avoiding 64K overflow for 16 bit systems).
       * prev[] will be initialized on the fly.
       */
      ds->head[HASH_SIZE-1] = NIL;
      
      /**** pts ****/
      /*memset((char*)ds->head, NIL, (unsigned)(HASH_SIZE-1)*sizeof(*ds->head));*/
      { unsigned len=(unsigned)(HASH_SIZE-1);
        while (len--!=0) ds->head[len]=NIL;
      }

      /* Set the default configuration parameters:
       */
      ds->max_lazy_match   = configuration_table[ds->interf.pack_level].max_lazy;
      ds->good_match       = configuration_table[ds->interf.pack_level].good_length;
  #ifndef FULL_SEARCH
      ds->nice_match       = configuration_table[ds->interf.pack_level].nice_length;
  #endif
      ds->max_chain_length = configuration_table[ds->interf.pack_level].max_chain;
  #if PTS_ZIP    
  #else
      (void)flags;
      if (ds->interf.pack_level <= 2) {
         *flags |= FAST;
      } else if (ds->interf.pack_level >= 8) {
         *flags |= SLOW;
      }
  #endif    
      /* ?? ? reduce max_chain_length for binary files */

      ds->strstart = 0;
      ds->block_start = 0L;
  #if 0 && defined(ASMV) && !defined(RISCOS)
      match_init(); /* initialize the asm code */
  #endif
  }

  /* ds->zip_pts_zpffree=fs->zpffree; */
  ds->zip_pts_state=0;
  ds->saved_match_available = 0;
  ds->saved_match_length = MIN_MATCH-1;
  ds->avail_eof=0; /* BUGFIX at Sat Jul  6 15:58:47 CEST 2002 */

  return fs;
}

/* ---- __END__ */
