@echo off
setlocal

set ds=%1
set zip=%2
set src=%3
set target=%4

%ds% %src% > %target%
%zip% a "%target%.7z" "%target%"
