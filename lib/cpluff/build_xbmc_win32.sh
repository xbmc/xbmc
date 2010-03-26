#!/bin/bash 

make distclean
rm config.h_bak

./configure &&

# uncomment if you want another path seperator (XBMC uses url style)
#cp config.h config.h_bak &&
#cat config.h | grep -v CP_FNAMESEP_CHAR |grep -v CP_FNAMESEP_STR > config.h &&
#echo "#define CP_FNAMESEP_CHAR '/'" >> config.h &&
#echo "#define CP_FNAMESEP_STR \"/\"" >> config.h &&*/
 
make -j3

