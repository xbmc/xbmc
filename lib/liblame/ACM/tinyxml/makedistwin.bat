del .\tinyxml_win\*.*
del .\docs\*.*

doxygen dox
mkdir tinyxml_win

copy readme.txt tinyxml_win
copy changes.txt tinyxml_win

copy *.cpp tinyxml_win
copy *.h tinyxml_win
copy *.dsp tinyxml_win
copy test0.xml tinyxml_win
copy test1.xml tinyxml_win
copy test2.xml tinyxml_win

mkdir .\tinyxml_win\docs
copy docs .\tinyxml_win\docs

