#! /bin/bash --
# by pts@fazekas.hu at Wed Oct  4 18:02:10 CEST 2017
#
# This tests that sam2p can load various PNG files correctly, especially
# with all kinds of predictors. Please note that it's not a comprehensive
# test, for example it doesn't contain interlaced PDF. It is also checked
# that sam2p is able to load its own output,
#

function cleanup() {
  rm -f -- png_test.tmp.pbm png_test.tmp.pgm png_test.tmp.ppm png_test.tmp.png
}

function do_png_test() {
  local INPUT_PNG="$1" TMP_PNM="$2" EXPECTED_PNM="$3" TMP_PNG=png_test.tmp.png

  "$SAM2P" -j:quiet -- "$EXPECTED_PNM" "$TMP_PNM"
  # Remove comment from "$TMP_PNM".
  perl -pi -0777 -e 's@\A(P\d\n)#.*\n@$1@' "$TMP_PNM"
  cmp "$EXPECTED_PNM" "$TMP_PNM"

  "$SAM2P" -j:quiet -- "$INPUT_PNG" "$TMP_PNM"
  perl -pi -0777 -e 's@\A(P\d\n)#.*\n@$1@' "$TMP_PNM"
  cmp "$EXPECTED_PNM" "$TMP_PNM"

  # -c:zip:15 makes a difference, it makes sam2p choose per-row predictors
  # differently.
  "$SAM2P" -j:quiet -c:zip:15 -- "$INPUT_PNG" "$TMP_PNG"
  "$SAM2P" -j:quiet -- "$TMP_PNG" "$TMP_PNM"
  perl -pi -0777 -e 's@\A(P\d\n)#.*\n@$1@' "$TMP_PNM"
  cmp "$EXPECTED_PNM" "$TMP_PNM"

  "$SAM2P" -j:quiet -- "$INPUT_PNG" "$TMP_PNG"
  "$SAM2P" -j:quiet -- "$TMP_PNG" "$TMP_PNM"
  perl -pi -0777 -e 's@\A(P\d\n)#.*\n@$1@' "$TMP_PNM"
  cmp "$EXPECTED_PNM" "$TMP_PNM"

  rm -f -- "$TMP_PNG" "$TMP_PNM"
}

set -ex
cd "${0%/*}"
if test "$1"; then
  SAM2P="$1"
else
  SAM2P="../sam2p"
fi
if test -d "${SAM2P%/*}/pppdir"; then
  export PATH="${SAM2P%/*}/pppdir"
fi

cleanup
do_png_test hello.gray1allpreds.png png_test.tmp.pbm hello.gray1.pbm
# It's important not to have png22pnm on the $PATH, because it's buggy here.
# It emits "P5 203 81 255\n", but it should emit "P5 203 81 3\n".
# pngtopnm is fine, it emits the latter.
do_png_test hello.gray2allpreds.png png_test.tmp.pgm hello.gray2.pgm
do_png_test hello.indexed4allpreds.png png_test.tmp.ppm hello.rgb8.ppm
do_png_test hello.indexed4orig.png png_test.tmp.ppm hello.rgb8.ppm
do_png_test hello.indexed4pngout.png png_test.tmp.ppm hello.rgb8.ppm
do_png_test hello.rgb8allpreds.png png_test.tmp.ppm hello.rgb8.ppm
cleanup  # Clean up only on success.

: png_test.sh OK.
