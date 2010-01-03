# @(#) $Id: setup.sh,v 1.8 2005/11/24 11:42:47 yeti Exp $
ENCA=$top_builddir/src/enca
TEST_LANGUAGES="be bg cs et hr hu lt lv pl ru sk sl uk zh"
ALL_TEST_LANGUAGES="$TEST_LANGUAGES none"
TEST_PAIR_be="koi8uni cp1251"
TEST_PAIR_bg="ibm855 cp1251"
TEST_PAIR_cs="keybcs2 ibm852"
TEST_PAIR_et="iso885913 baltic"
TEST_PAIR_hr="ibm852 cp1250"
TEST_PAIR_hu="cp1250 ibm852"
TEST_PAIR_lt="iso88594 baltic"
TEST_PAIR_lv="ibm775 cp1257"
TEST_PAIR_pl="iso88592 iso885916"
TEST_PAIR_ru="koi8r cp866"
TEST_PAIR_sk="keybcs2 koi8cs2"
TEST_PAIR_sl="cp1250 ibm852"
TEST_PAIR_uk="koi8u cp1125"
# Only lucky people have basename
TESTNAME=`echo $0 | sed -e 's/.*\///' -e 's/.sh$//'`
DIE=
E77=
cat </dev/null >$TESTNAME.actual
rm -f core* 2>/dev/null
