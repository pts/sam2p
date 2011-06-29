#! /bin/bash --

# Imp:
# by pts@fazekas.hu at Fri May 23 23:33:19 CEST 2008

set -ex

export  CC='/usr/src/uclibc-compiler/bin/i686-gcc -static'
export CXX='/usr/src/uclibc-compiler/bin/i686-g++ -static'

if test -f bts2.tth; then :; else
  ./configure --enable-lzw --enable-gif
  make  # generate some files
fi

SAM2P_VERSION="$(bash ./mkdist.sh --getversion)"
test "$SAM2P_VERSION"

# vvv either -mwindows or -mconsole
$CXX -s -DNDEBUG -O3 \
    -DHAVE_CONFIG2_H -DUSE_CONFIG_UCLIBC_H -DSAM2P_VERSION=\""$SAM2P_VERSION"\" \
    -fsigned-char -fno-rtti -fno-exceptions -ansi -pedantic -Wall -W \
    sam2p_main.cpp appliers.cpp crc32.c c_lgcc.cpp in_ps.cpp in_tga.cpp in_pnm.cpp in_bmp.cpp in_gif.cpp in_lbm.cpp in_xpm.cpp mapping.cpp in_pcx.cpp in_jai.cpp in_png.cpp in_jpeg.cpp in_tiff.cpp rule.cpp minips.cpp encoder.cpp pts_lzw.c pts_fax.c pts_defl.c error.cpp image.cpp gensio.cpp snprintf.c gensi.cpp out_gif.cpp \
    -o sam2p
upx.pts --brute sam2p
elfosfix.pl sam2p

