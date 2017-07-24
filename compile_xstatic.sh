#! /bin/bash --
# by pts@fazekas.hu at Tue Jan 24 19:35:49 CET 2017

set -ex

if test -f bts2.tth; then :; else
  CC="xstatic gcc" ./gen_bts2_tth.sh
fi

SAM2P_VERSION="$(bash ./mkdist.sh --getversion)"
test "$SAM2P_VERSION"

# Don't use `-nostdlib -lc', it prevents linking crtbeginT.o or causes segfault.
# With or without -fno-use-cxa-atexit, doesn't make a difference.
xstatic gcc -x c++ -s -DNDEBUG -O3 \
    -DHAVE_CONFIG2_H -DUSE_CONFIG_UCLIBC_H -DSAM2P_VERSION=\""$SAM2P_VERSION"\" \
    -fsigned-char -fno-rtti -fno-exceptions -nostdinc++ -ansi -pedantic -Wall -W -Wextra \
    sam2p_main.cpp appliers.cpp crc32.c c_lgcc.cpp in_ps.cpp in_tga.cpp in_pnm.cpp in_bmp.cpp in_gif.cpp in_lbm.cpp in_xpm.cpp mapping.cpp in_pcx.cpp in_jai.cpp in_png.cpp in_jpeg.cpp in_tiff.cpp rule.cpp minips.cpp encoder.cpp pts_lzw.c pts_fax.c pts_defl.c error.cpp image.cpp gensio.cpp snprintf.c gensi.cpp out_gif.cpp \
    -o sam2p.xstatic.uncompressed
elfosfix.pl sam2p.xstatic.uncompressed
cp -a sam2p.xstatic.uncompressed sam2p.xstatic
upx.pts --brute sam2p.xstatic
elfosfix.pl sam2p.xstatic
