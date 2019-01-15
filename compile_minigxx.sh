#! /bin/sh --
# by pts@fazekas.hu at Sat Jan 12 19:08:59 CET 2019

set -ex

export CC="${CC:-gcc}"

if test -f bts2.tth; then :; else
  (. ./gen_bts2_tth.sh) || exit "$?"
fi

$CC -O2 -ansi -Wall -W -Wextra print_sizeofs.c -o print_sizeofs
./print_sizeofs >sizeofs.h

SAM2P_VERSION="$(set -- --getversion; . ./mkdist.sh)"
test "$SAM2P_VERSION"

# It also compiles (with some warnings about -nostdc++ for cc1) without -x c++
# You can also use `-nodefaultlibs -lc', but it's not needed.
# With or without -fno-use-cxa-atexit, doesn't make a difference.
$CC -s -O2 \
    -DHAVE_CONFIG2_H -DUSE_CONFIG_STDC_H -DSAM2P_VERSION=\""$SAM2P_VERSION"\" \
    -fsigned-char -fno-rtti -fno-exceptions -nostdinc++ -ansi -pedantic -Wall -W -Wextra \
    -x c++ minigxx_nortti.cc \
    sam2p_main.cpp appliers.cpp crc32.c in_ps.cpp in_tga.cpp in_pnm.cpp in_bmp.cpp in_gif.cpp in_lbm.cpp in_xpm.cpp mapping.cpp in_pcx.cpp in_jai.cpp in_png.cpp in_jpeg.cpp in_tiff.cpp rule.cpp minips.cpp encoder.cpp pts_lzw.c pts_fax.c pts_defl.c error.cpp image.cpp gensio.cpp snprintf.c gensi.cpp out_gif.cpp \
    -o sam2p
