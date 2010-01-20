# Sed script for tests/stateless-check editing.

/charsetf=/ a\
\
# For systems with severe filename restrictions\
# allow for an alternate filename.\
UNAME=${UNAME-`uname 2>/dev/null`}\
case X$UNAME in\
  *-DOS) filename=`echo "$charsetf" | sed "s|ISO-|ISO/|; \\\
                                           s|Mac|Mac/|; \\\
                                           s|BIG5-HKSCS-|BIG5-HKSCS/|; \\\
                                           s|Georgian-|Georgian/|"`\
         tmp_filename=`echo "$filename" | sed "s|/|/tmp-|"`\
         tmp_orig_filename=`echo "$filename" | sed "s|/|/tmp-orig-|"` ;;\
  *)     filename="$charsetf"\
         tmp_filename="$charsetf"\
         tmp_orig_filename="$charsetf" ;;\
esac
s|/"\$charsetf"|/"$filename"|g
s|tmp-"\$charsetf"|"${srcdir}"/"$tmp_filename"|g
s|tmp-orig"\$charsetf"|"${srcdir}"/"$tmp_orig_filename"|g
s|\.INVERSE\.|.INVERSE-|g
s|\.IRREVERSIBLE\.|.IRREVERSIBLE-|g
