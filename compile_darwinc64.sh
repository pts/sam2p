#! /bin/bash --
# by pts@fazekas.hu at Mon Jul 24 17:33:42 CEST 2017

set -ex

# $ docker image ls --digests multiarch/crossbuild
# The image ID is also a digest, and is a computed SHA256 hash of the image configuration object, which contains the digests of the layers that contribute to the image's filesystem definition.
# REPOSITORY             TAG                 DIGEST                                                                    IMAGE ID            CREATED             SIZE
# multiarch/crossbuild   latest              sha256:84a53371f554a3b3d321c9d1dfd485b8748ad6f378ab1ebed603fe1ff01f7b4d   846ea4d99d1a        5 months ago        2.99 GB
  CCC="docker run -v $PWD:/workdir multiarch/crossbuild /usr/osxcross/bin/o64-clang -mmacosx-version-min=10.5 -c"
 CXXC="docker run -v $PWD:/workdir multiarch/crossbuild /usr/osxcross/bin/o64-clang++ -mmacosx-version-min=10.5 -c"
 CCLD="docker run -v $PWD:/workdir multiarch/crossbuild /usr/osxcross/bin/o64-clang -mmacosx-version-min=10.5 -Ldarwin_libgcc/x86_64-apple-darwin10/4.9.4 -lSystem -lgcc -lcrt1.10.5.o -nostdlib"
STRIP="docker run -v $PWD:/workdir multiarch/crossbuild /usr/osxcross/bin/x86_64-apple-darwin14-strip"
test -f darwin_libgcc/x86_64-apple-darwin10/4.9.4/libgcc.a

if test -f bts2.tth; then :; else
  CC=gcc ./gen_bts2_tth.sh  # Not cross-compiled.
fi

SAM2P_VERSION="$(bash ./mkdist.sh --getversion)"
test "$SAM2P_VERSION"

$CCLD -DNDEBUG -O3 -DHAVE_CONFIG2_H -DUSE_CONFIG_UCLIBC_H -DUSE_ATTRIBUTE_ALIAS=0 -DSAM2P_VERSION=\""$SAM2P_VERSION"\" \
    -fsigned-char -fno-rtti -fno-exceptions -nostdinc++ -ansi -pedantic -Wall -W -Wextra \
    sam2p_main.cpp appliers.cpp crc32.c out_gif.cpp in_ps.cpp in_tga.cpp \
    in_pnm.cpp in_bmp.cpp in_gif.cpp in_lbm.cpp in_xpm.cpp mapping.cpp \
    in_pcx.cpp in_jai.cpp in_png.cpp in_jpeg.cpp in_tiff.cpp rule.cpp \
    minips.cpp encoder.cpp pts_lzw.c pts_fax.c pts_defl.c error.cpp \
    image.cpp gensio.cpp snprintf.c gensi.cpp c_lgcc.cpp \
    -o sam2p.darwinc64
$STRIP sam2p.darwinc64

: OK.
