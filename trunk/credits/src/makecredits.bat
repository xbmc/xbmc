@echo off

echo Assembling Shaders...
xsasm /P /nologo credits.vtx
if %errorlevel% neq 0 goto error
xsasm /P /nologo credits.pxl
if %errorlevel% neq 0 goto error

echo Converting Mesh...
conv3ds -Mmf xbmc.3ds
meshconv xbmc.x credits.rdf
if %errorlevel% neq 0 goto error

echo Bundling...
bundler credits.rdf
if %errorlevel% neq 0 goto error
XprPack ..\credits.xpr
if %errorlevel% neq 0 goto error

echo Success!
goto exit

:error

echo ERROR!

:exit