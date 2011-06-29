#! /bin/bash --
#
# mkdist.sh by pts@fazekas.hu at Wed Mar  6 09:09:01 CET 2002
# added debian/changelog support at Fri Mar  5 19:37:45 CET 2004
# added libevhdns Makefile.in support at Sun Apr 25 12:24:43 CEST 2010
# based on mkdist.sh of autotrace
#

if test "$1" = --cd; then
  MYDIR="${0%/*}"
  test "$MYDIR" || MYDIR=.
  cd "$MYDIR"
fi

# Get the release version number.
if test -f debian/changelog; then
  # PRODUCT_AND_VERSION=$PRODUCT-$VERSION
  PRODUCT_AND_VERSION="`<debian/changelog perl -ne 'print"$1-$2"if/^(\S+) +[(]([-.\w]+?)(?:-\d+)?[)] +\w+;/;last'`"
  if test "$PRODUCT_AND_VERSION"; then :; else
    echo "$0: couldn't determine version from debian/changelog" >&2
    exit 4
  fi
elif test -f Makefile.in && grep '^product-and-version:' <Makefile.in >/dev/null; then
  PRODUCT_AND_VERSION="`make SHELL=/bin/bash -f Makefile.in product-and-version`"
  if test "$?" != 0 || test -z "$PRODUCT_AND_VERSION"; then
    echo "$0: couldn't determine version from Makefile.in" >&2
    exit 7
  fi
else
  echo "$0: missing: debian/changelog or RELEASE= in Makefile.in" >&2
  exit 2
fi

if test "$1" = --getversion; then
  echo "${PRODUCT_AND_VERSION##*-}"
  exit
fi

echo "Creating distfile in $PWD"

# Get the list of files.
if test -f files; then
  FILES="`cat files`"
elif test -d CVS; then
  FILES=$( IFS='
'
    find -type d -name CVS | while read D; do
      F="$D/Entries"
      export E="${D%/CVS}/"
      E="${E#./}"
      perl -ne 'print"$ENV{E}$1\n"if m@^/([^/]+)/[1-9]@' <"$F"
    done)
else
  echo "$0: missing: files or **/CVS/Entries" >&2
  exit 3
fi


if test -e "$PRODUCT_AND_VERSION"; then
  echo "$0: directory $PRODUCT_AND_VERSION already exists, remove it first" >&2
  exit 5
fi

if test $# -gt 0; then
  TGZ_NAME="$1.tar.gz"; shift
else
  TGZ_NAME="$PRODUCT_AND_VERSION.tar.gz"
fi

set -e # exit on error
rm -f "../$TGZ_NAME"
mkdir "$PRODUCT_AND_VERSION"
(IFS='
'; exec tar -c -- $FILES "$@") |
(cd "$PRODUCT_AND_VERSION" && exec tar -xv)
# ^^^ tar(1) magically calls mkdir(2) etc.

# vvv Dat: don't include sam2p-.../ in the filenames of the .tar.gz
#(IFS='
#'; cd "$PRODUCT_AND_VERSION" && exec tar -czf "../../$TGZ_NAME" -- $FILES "$@")

# vvv Dat: do include sam2p-.../ in the filenames of the .tar.gz
(IFS='
'; export PRODUCT_AND_VERSION; exec tar -czf "../$TGZ_NAME" -- `echo "$FILES" | perl -pe '$_="$ENV{PRODUCT_AND_VERSION}/$_"'` "$@")

rm -rf "$PRODUCT_AND_VERSION"
set +e

if test -s "../$TGZ_NAME"; then :; else
  echo "$0: failed to create distfile: ../$TGZ_NAME" >&2
  exit 6
fi

FULL_TGZ_NAME="`cd ..;echo "$PWD/$TGZ_NAME"`"
echo "Created distfile: $FULL_TGZ_NAME"
if type -p pts-xclip >/dev/null; then
  echo -n "$FULL_TGZ_NAME" | pts-xclip -i
  echo -n "$FULL_TGZ_NAME" | pts-xclip -i -selection clipboard
  echo "Name of distfile added to the X11 primary + clipboard."
fi

# __EOF__
