/*
 * all.cc: Helper file for compiler benchmarking.
 * by pts@fazekas.hu  at Sat Dec  7 22:56:41 CET 2013
 *
 * time g++ -c -DNDEBUG -DHAVE_CONFIG2_H   -fsigned-char -fno-rtti -fno-exceptions -ansi -pedantic -Wall -W allcc.cc
 */

/* Needed to prevent POSIX_C_SOURCE from being redefined in
 * /usr/include/features.h
 */
#define USE_GNU_SOURCE_INSTEAD_OF_POSIX_SOURCE 1

#ifndef OBJDEP  /* Make `make Makedep' work. */
#include "sam2p_main.cpp"
#include "appliers.cpp"
#include "c_lgcc.cpp"
#include "out_gif.cpp"
#include "in_ps.cpp"
#include "in_tga.cpp"
#include "in_pnm.cpp"
#include "in_bmp.cpp"
#include "in_gif.cpp"
#include "in_lbm.cpp"
#include "in_xpm.cpp"
#include "mapping.cpp"
#include "in_pcx.cpp"
#include "in_jai.cpp"
#include "in_png.cpp"
#include "in_jpeg.cpp"
#include "in_tiff.cpp"
#include "rule.cpp"
#include "minips.cpp"
#include "encoder.cpp"
#include "error.cpp"
#include "image.cpp"
#include "gensio.cpp"
#include "gensi.cpp"
#endif
