#! /bin/sh

LANG=C; export LANG
LC_ALL=C; export LC_ALL
LC_COLLATE=C; export LC_COLLATE

if [ $# -lt 3 ]
then
  echo "Usage: $0 awk [-h headerdefine] outputheader proto_obj"
  exit 1
fi

awk="$1"
shift

if [ x"$1" = x-h ]
then
  headeropt="-v headername=$2"
  shift; shift;
else
  headeropt=""
fi

header="$1"
shift
headertmp="$header.$$.tmp~"

proto_src="`echo $@ | tr ' ' '\n' | sed -e 's/\.o/\.c/g' | sort | uniq | egrep -v 'ubiqx/|wrapped|modules/getdate'`"

echo creating $header

mkdir -p `dirname $header`

${awk} $headeropt \
  -f script/mkproto.awk $proto_src > $headertmp

if cmp -s $header $headertmp 2>/dev/null
then
  echo "$header unchanged"
  rm $headertmp
else
  mv $headertmp $header
fi
