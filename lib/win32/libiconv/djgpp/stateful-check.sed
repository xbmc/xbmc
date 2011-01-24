# Sed script for tests/stateful-check editing.

/charsetf=/ a\
\
# For systems with severe filename restrictions allow for\
# an alternate filename.\
UNAME=${UNAME-`uname 2>/dev/null`}\
case X$UNAME in\
  *-DOS) filename=`echo "$charsetf" | sed "s|ISO-|ISO/|;s|2022-|2022|;s|BIG5-HKSCS-|BIG5-HKSCS/|"` ;;\
  *)     filename="$charsetf" ;;\
esac
s/\$charsetf"-snippet/$filename"-snippet/g
