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

tar czvf "$MYDIR.tar.gz" `<"$MYDIR"/files perl -pi -e '$_="$ENV{MYDIR}/$_"'`

[ -s "$UPDIR/$MYDIR.tar.gz" ] || {
  echo "$0: zero size: $UPDIR/$MYDIR.tar.gz"
  exit 4
}

echo "Created $UPDIR/$MYDIR.tar.gz" >&2
