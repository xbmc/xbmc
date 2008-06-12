#!/bin/sh

cd "$1"
test -f "$2" || exit 0

for I in $3 $4 $5 $6 $7 $8
do 
	echo "Linking $I to $2"
	ln -s $2 $I
done

exit 0
