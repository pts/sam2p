#! /bin/bash --
#
# mkdist.sh by pts@fazekas.hu at Wed Mar  6 09:09:01 CET 2002
# based on mkdist.sh of autotrace
#
#


WD="`pwd`"
[ "$WD" ] || {
  echo "$0: cannot find current directory"
  exit 2
}

UPDIR="${WD%/*}"
MYDIR="${WD##*/}"
export MYDIR
cd "$UPDIR"

#[ -n "$NEED" -a ! -f "$MYDIR/$NEED" ] || {
#  echo "$0: cannot find file $NEED"
#  exit 3
#}

if [ "$1" ]; then
  TAR_BASENAME="$1"
  shift
  EXTRA_FILES=""
  for F in "$@"; do EXTRA_FILES="$EXTRA_FILES$F
"; done
else
  TAR_BASENAME="$MY_DIR"
  EXTRA_FILES=""
fi

tar czvf "$TAR_BASENAME.tar.gz" `{ cat "$MYDIR"/files; echo -n "$EXTRA_FILES"; } | perl -pi -e '$_="$ENV{MYDIR}/$_"'`

[ -s "$UPDIR/$TAR_BASENAME.tar.gz" ] || {
  echo "$0: zero size: $UPDIR/$TAR_BASENAME.tar.gz"
  exit 4
}

echo "Created $UPDIR/$TAR_BASENAME.tar.gz" >&2
