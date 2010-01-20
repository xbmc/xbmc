# Sed script for tests/translit-check editing.

/\.\./ i\
# For systems with severe filename restrictions allow for\
# an alternate filename.\
UNAME=${UNAME-`uname 2>/dev/null`}\
case X$UNAME in\
  *-DOS) file=`echo "$file" | sed "s|TranslitFail1|_Translit/Fail1|; \\\
                                   s|Translit1|_Translit/1|"`;;\
  *)     file="$file" ;;\
esac
