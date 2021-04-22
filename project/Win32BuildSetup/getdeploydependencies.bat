@echo off

set DOWNLOAD_URL=https://aka.ms/vs/16/release/vc_redist.%TARGET_ARCHITECTURE%.exe
set DOWNLOAD_FOLDER=..\BuildDependencies\downloads\vcredist\2017-2019
set DOWNLOAD_FILE=vcredist_%TARGET_ARCHITECTURE%.exe

:: Following commands expect this script's parent directory to be the current directory, so make sure that's so
PUSHD %~dp0

if not exist %DOWNLOAD_FOLDER% mkdir %DOWNLOAD_FOLDER%

if not exist %DOWNLOAD_FOLDER%\%DOWNLOAD_FILE% (
  echo Downloading vc142 redist...
  ..\BuildDependencies\bin\wget --tries=5 --retry-connrefused --waitretry=2 -nv -O %DOWNLOAD_FOLDER%\%DOWNLOAD_FILE% %DOWNLOAD_URL%
)
:: Restore the previous current directory
POPD
