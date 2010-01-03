#! /bin/bash
grep -v '³' cp1251.base | ../xlt ../maps/cp1251.map ../maps/ibm866.map\
  >ibm866.base
../normalize.pl cp1251.base <ibm866.base >ibm866.xbase
rm ibm866.base
../doit.sh cp1251 ibm866 iso88595 koi8uni maccyr ibm855 koi8u
