@echo off

echo Assembling Shaders...
xsasm /P /nologo credits.vtx
if %errorlevel% neq 0 goto exit
xsasm /P /nologo credits.pxl
if %errorlevel% neq 0 goto exit

echo Converting Mesh...
conv3ds -Mmf xbmc.3ds
meshconv xbmc.x credits.rdf
if %errorlevel% neq 0 goto exit

echo Bundling...
bundler credits.rdf
if %errorlevel% neq 0 goto exit

echo Success!

:exit