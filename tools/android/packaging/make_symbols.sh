FN=$(basename "$1")
dump_syms "$1" > $FN.sym
HASH=`head -n 1 $FN.sym | cut -d " " -f 4`
mkdir -p symbols/$FN/$HASH
mv $FN.sym symbols/$FN/$HASH
echo "$FN.sym generated"
