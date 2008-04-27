rem Use Astyle to fix style in 'C' files
cd %1%

fixlines -p *.c
fixlines -p *.cpp
fixlines -p *.cc

astyle --style=ansi -c -o --convert-tabs --indent-preprocessor *.c
astyle --style=ansi -c -o --convert-tabs --indent-preprocessor *.cpp
astyle --style=ansi -c -o --convert-tabs --indent-preprocessor *.cc
del *.orig
@rem convert line terminators to Unix style LFs
fixlines -u *.c
fixlines -u *.cpp
fixlines -u *.cc
fixlines -u *.h
del *.bak

cd ..\
