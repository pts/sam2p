#! /bin/bash --
# by pts@fazkeas.hu at Mon Jul 24 18:21:28 CEST 2017
set -ex

CC="${CC:-gcc}"

# Omit -pedantic instead of -Wno-overlength-strings.
$CC -O2 -DNO_CONFIG -fsigned-char -ansi -Wall -W -Wextra ps_tiny.c -o ps_tiny.gen

# --- Based on Makefile.

<l1zip.psm >l1g8z.pst.tmp.h perl -pe0
$CC -E -DCFG_FMT_ZLIB_ONLY=1 -DNDEBUG=1 -DCFG_NO_VAR_S=1 -DUSE_NO_BIND=1 -DUSE_SHORT_NAMES=1 -DUSE_CURRENTFILE=1 -DUSE_NO_EOF=1 -DUSE_UNITLENGTH_8 -DUSE_EARLYCHANGE_1 -DUSE_LOWBITFIRST_FALSE -DUSE_NO_NULLDEF=1 -DUSE_PIN=1 -DUSE_A85D=1 l1g8z.pst.tmp.h >l1g8z.pst.tmp.i
<l1g8z.pst.tmp.i >l1g8z.pst.tmp.pin perl -ne's@/\s+(?=\w)@/@g;print if!/^#/&&!/^\s*\Z/'
<l1g8z.pst.tmp.pin >l1g8z.pst.tmp.ps0 ./ps_tiny.gen
<l1g8z.pst.tmp.ps0 >l1g8z.pst.tmp.pst perl -e '$s=$_=join"",<STDIN>; $s=~s@([()\\])@\\$1@g; die if $ARGV[0]!~/^(\w+)/; print "\n% TTT_QUOTE\n/$1 ($s)\n\n"' -- l1g8z.pst
mv -f l1g8z.pst.tmp.pst l1g8z.pst

<l1zip.psm >l1ghz.pst.tmp.h perl -pe0
$CC -E -DCFG_FMT_ZLIB_ONLY=1 -DNDEBUG=1 -DCFG_NO_VAR_S=1 -DUSE_NO_BIND=1 -DUSE_SHORT_NAMES=1 -DUSE_CURRENTFILE=1 -DUSE_NO_EOF=1 -DUSE_UNITLENGTH_8 -DUSE_EARLYCHANGE_1 -DUSE_LOWBITFIRST_FALSE -DUSE_NO_NULLDEF=1 -DUSE_PIN=1 -DUSE_HEXD=1 l1ghz.pst.tmp.h >l1ghz.pst.tmp.i
<l1ghz.pst.tmp.i >l1ghz.pst.tmp.pin perl -ne's@/\s+(?=\w)@/@g;print if!/^#/&&!/^\s*\Z/'
<l1ghz.pst.tmp.pin >l1ghz.pst.tmp.ps0 ./ps_tiny.gen
<l1ghz.pst.tmp.ps0 >l1ghz.pst.tmp.pst perl -e '$s=$_=join"",<STDIN>; $s=~s@([()\\])@\\$1@g; die if $ARGV[0]!~/^(\w+)/; print "\n% TTT_QUOTE\n/$1 ($s)\n\n"' -- l1ghz.pst
mv -f l1ghz.pst.tmp.pst l1ghz.pst

<l1zip.psm >l1gbz.pst.tmp.h perl -pe0
$CC -E -DCFG_FMT_ZLIB_ONLY=1 -DNDEBUG=1 -DCFG_NO_VAR_S=1 -DUSE_NO_BIND=1 -DUSE_SHORT_NAMES=1 -DUSE_CURRENTFILE=1 -DUSE_NO_EOF=1 -DUSE_UNITLENGTH_8 -DUSE_EARLYCHANGE_1 -DUSE_LOWBITFIRST_FALSE -DUSE_NO_NULLDEF=1 -DUSE_PIN=1 -DUSE_BINARY=1 l1gbz.pst.tmp.h >l1gbz.pst.tmp.i
<l1gbz.pst.tmp.i >l1gbz.pst.tmp.pin perl -ne's@/\s+(?=\w)@/@g;print if!/^#/&&!/^\s*\Z/'
<l1gbz.pst.tmp.pin >l1gbz.pst.tmp.ps0 ./ps_tiny.gen
<l1gbz.pst.tmp.ps0 >l1gbz.pst.tmp.pst perl -e '$s=$_=join"",<STDIN>; $s=~s@([()\\])@\\$1@g; die if $ARGV[0]!~/^(\w+)/; print "\n% TTT_QUOTE\n/$1 ($s)\n\n"' -- l1gbz.pst
mv -f l1gbz.pst.tmp.pst l1gbz.pst

<l1lzw.psm >l1g8l.pst.tmp.h perl -pe0
$CC -E -DCFG_FMT_ZLIB_ONLY=1 -DNDEBUG=1 -DCFG_NO_VAR_S=1 -DUSE_NO_BIND=1 -DUSE_SHORT_NAMES=1 -DUSE_CURRENTFILE=1 -DUSE_NO_EOF=1 -DUSE_UNITLENGTH_8 -DUSE_EARLYCHANGE_1 -DUSE_LOWBITFIRST_FALSE -DUSE_NO_NULLDEF=1 -DUSE_PIN=1 -DUSE_A85D=1 l1g8l.pst.tmp.h >l1g8l.pst.tmp.i
<l1g8l.pst.tmp.i >l1g8l.pst.tmp.pin perl -ne's@/\s+(?=\w)@/@g;print if!/^#/&&!/^\s*\Z/'
<l1g8l.pst.tmp.pin >l1g8l.pst.tmp.ps0 ./ps_tiny.gen
<l1g8l.pst.tmp.ps0 >l1g8l.pst.tmp.pst perl -e '$s=$_=join"",<STDIN>; $s=~s@([()\\])@\\$1@g; die if $ARGV[0]!~/^(\w+)/; print "\n% TTT_QUOTE\n/$1 ($s)\n\n"' -- l1g8l.pst
mv -f l1g8l.pst.tmp.pst l1g8l.pst

<l1lzw.psm >l1ghl.pst.tmp.h perl -pe0
$CC -E -DCFG_FMT_ZLIB_ONLY=1 -DNDEBUG=1 -DCFG_NO_VAR_S=1 -DUSE_NO_BIND=1 -DUSE_SHORT_NAMES=1 -DUSE_CURRENTFILE=1 -DUSE_NO_EOF=1 -DUSE_UNITLENGTH_8 -DUSE_EARLYCHANGE_1 -DUSE_LOWBITFIRST_FALSE -DUSE_NO_NULLDEF=1 -DUSE_PIN=1 -DUSE_HEXD=1 l1ghl.pst.tmp.h >l1ghl.pst.tmp.i
<l1ghl.pst.tmp.i >l1ghl.pst.tmp.pin perl -ne's@/\s+(?=\w)@/@g;print if!/^#/&&!/^\s*\Z/'
<l1ghl.pst.tmp.pin >l1ghl.pst.tmp.ps0 ./ps_tiny.gen
<l1ghl.pst.tmp.ps0 >l1ghl.pst.tmp.pst perl -e '$s=$_=join"",<STDIN>; $s=~s@([()\\])@\\$1@g; die if $ARGV[0]!~/^(\w+)/; print "\n% TTT_QUOTE\n/$1 ($s)\n\n"' -- l1ghl.pst
mv -f l1ghl.pst.tmp.pst l1ghl.pst

<l1lzw.psm >l1gbl.pst.tmp.h perl -pe0
$CC -E -DCFG_FMT_ZLIB_ONLY=1 -DNDEBUG=1 -DCFG_NO_VAR_S=1 -DUSE_NO_BIND=1 -DUSE_SHORT_NAMES=1 -DUSE_CURRENTFILE=1 -DUSE_NO_EOF=1 -DUSE_UNITLENGTH_8 -DUSE_EARLYCHANGE_1 -DUSE_LOWBITFIRST_FALSE -DUSE_NO_NULLDEF=1 -DUSE_PIN=1 -DUSE_BINARY=1 l1gbl.pst.tmp.h >l1gbl.pst.tmp.i
<l1gbl.pst.tmp.i >l1gbl.pst.tmp.pin perl -ne's@/\s+(?=\w)@/@g;print if!/^#/&&!/^\s*\Z/'
<l1gbl.pst.tmp.pin >l1gbl.pst.tmp.ps0 ./ps_tiny.gen
<l1gbl.pst.tmp.ps0 >l1gbl.pst.tmp.pst perl -e '$s=$_=join"",<STDIN>; $s=~s@([()\\])@\\$1@g; die if $ARGV[0]!~/^(\w+)/; print "\n% TTT_QUOTE\n/$1 ($s)\n\n"' -- l1gbl.pst
mv -f l1gbl.pst.tmp.pst l1gbl.pst

# ---

perl -pe0 bts.ttt l1g8z.pst l1ghz.pst l1gbz.pst l1g8l.pst l1ghl.pst l1gbl.pst >bts1.ttt
./ps_tiny.gen --copy <bts1.ttt >bts2.ttt
perl -x ./hq.pl <bts2.ttt >bts2.tth
rm -f *.pst.tmp.* bts1.ttt bts2.ttt *.pst ps_tiny.gen

: gen_bts2_tth.sh OK.
