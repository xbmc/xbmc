rem Use Astyle to fix style in a file
fixlines -p %1%
astyle --style=ansi -c -o --convert-tabs --indent-preprocessor %1%
del %1%.orig
@rem convert line terminators to Unix style LFs
fixlines -u %1%
del %1%.bak
