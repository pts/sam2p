#! /bin/bash --
# by pts@fazekas.hu at Wed Nov 15 00:36:26 CET 2017

if test "$1"; then
  SAM2P="$1"
else
  SAM2P="../sam2p"
fi

set -ex

"$SAM2P" -j:quiet -s:indexed4:stop           hello.rgb8.ppm pi4d.png      # Uses predictor 10 (PngNone). Default is the same as -c:zip:25. Default here is the same as -c:zip:1.
"$SAM2P" -j:quiet -s:indexed4:stop -c:zip    hello.rgb8.ppm pi4zip.png    # Uses predictor 10 (PngNone). -c:zip is the same as -c:zip:1.
"$SAM2P" -j:quiet -s:indexed4:stop -c:zip:1  hello.rgb8.ppm pi4zip1.png   # Uses predictor 10 (PngNone).
"$SAM2P" -j:quiet -s:indexed4:stop -c:zip:10 hello.rgb8.ppm pi4zip10.png  # Uses predictor 10 (PngNone).
"$SAM2P" -j:quiet -s:indexed4:stop -c:zip:15 hello.rgb8.ppm pi4zip15.png  # Uses various PNG predictors (10...14).
"$SAM2P" -j:quiet -s:indexed4:stop -c:zip:25 hello.rgb8.ppm pi4zip25.png  # Uses predictor 10 (PngNone). -c:zip is the same as -c:zip:1.

"$SAM2P" -j:quiet -s:rgb8:stop               hello.rgb8.ppm pr8d.png      # Uses various PNG predictors (10..14). Default is the same as -c:zip:25. Default here is the same as -c:zip:15.
"$SAM2P" -j:quiet -s:rgb8:stop     -c:zip    hello.rgb8.ppm pr8zip.png    # Uses predictor 10 (PngNone). -c:zip is the same as -c:zip:1.
"$SAM2P" -j:quiet -s:rgb8:stop     -c:zip:1  hello.rgb8.ppm pr8zip1.png   # Uses predictor 10 (PngNone).
"$SAM2P" -j:quiet -s:rgb8:stop     -c:zip:10 hello.rgb8.ppm pr8zip10.png  # Uses predictor 10 (PngNone).
"$SAM2P" -j:quiet -s:rgb8:stop     -c:zip:15 hello.rgb8.ppm pr8zip15.png  # Uses various PNG predictors (10...14).
"$SAM2P" -j:quiet -s:rgb8:stop     -c:zip:25 hello.rgb8.ppm pr8zip25.png  # Uses various PNG predictors (10...14). -c:zip:25 here is the same as -c:zip:15.

"$SAM2P" -j:quiet -s:indexed4:stop           hello.rgb8.ppm pi4d.pdf      # No predictor. Default is the same as -c:zip:25. Default here is the same as -c:zip:1.
"$SAM2P" -j:quiet -s:indexed4:stop -c:zip    hello.rgb8.ppm pi4zip.pdf    # No predictor. -c:zip is the same as -c:zip:1.
"$SAM2P" -j:quiet -s:indexed4:stop -c:zip:1  hello.rgb8.ppm pi4zip1.pdf   # No predictor.
"$SAM2P" -j:quiet -s:indexed4:stop -c:zip:10 hello.rgb8.ppm pi4zip10.pdf  # /Predictor 10. Uses predictor 10 (PngNone).
"$SAM2P" -j:quiet -s:indexed4:stop -c:zip:15 hello.rgb8.ppm pi4zip15.pdf  # /Predictor 10. Uses various PNG predictors (10..14).
"$SAM2P" -j:quiet -s:indexed4:stop -c:zip:25 hello.rgb8.ppm pi4zip25.pdf  # No predictor. -c:zip:25 here is the same as -c:zip:1.

"$SAM2P" -j:quiet -s:rgb8:stop               hello.rgb8.ppm pr8d.pdf      # /Predictor 10. Default is the same as -c:zip:25. Default here is the same as -c:zip:15.
"$SAM2P" -j:quiet -s:rgb8:stop     -c:zip    hello.rgb8.ppm pr8zip.pdf    # No predictor. -c:zip is the same as -c:zip:1.
"$SAM2P" -j:quiet -s:rgb8:stop     -c:zip:1  hello.rgb8.ppm pr8zip1.pdf   # No predictor.
"$SAM2P" -j:quiet -s:rgb8:stop     -c:zip:10 hello.rgb8.ppm pr8zip10.pdf  # /Predictor 10. Uses predictor 10 (PngNone).
"$SAM2P" -j:quiet -s:rgb8:stop     -c:zip:15 hello.rgb8.ppm pr8zip15.pdf  # /Predictor 10. Uses various PNG predictors (10..14).
"$SAM2P" -j:quiet -s:rgb8:stop     -c:zip:25 hello.rgb8.ppm pr8zip25.pdf  # /Predictor 10. -c:zip:25 here is the same as -c:zip:15.

cmp pi4zip1.png pi4d.png
cmp pi4zip1.png pi4zip.png
cmp pi4zip1.png pi4zip10.png
cmp pi4zip1.png pi4zip25.png

cmp pr8zip15.png pr8d.png
cmp pr8zip1.png pr8zip.png
cmp pr8zip1.png pr8zip10.png
cmp pr8zip15.png pr8zip25.png

cmp pi4zip1.pdf pi4d.pdf
cmp pi4zip1.pdf pi4zip.pdf
cmp pi4zip1.pdf pi4zip25.pdf

cmp pr8zip1.pdf pr8zip.pdf
cmp pr8zip15.pdf pr8d.pdf
cmp pr8zip15.pdf pr8zip25.pdf

: predictors.sh OK.
