@echo off

set DOWNLOAD_URL=https://aka.ms/vc14/vc_redist.%TARGET_ARCHITECTURE%.exe
set DOWNLOAD_FOLDER=..\BuildDependencies\downloads\vcredist\2017-2026
set DOWNLOAD_FILE=vcredist_%TARGET_ARCHITECTURE%.exe

:: Following commands expect this script's parent directory to be the current directory, so make sure that's so
PUSHD %~dp0

if not exist %DOWNLOAD_FOLDER% mkdir %DOWNLOAD_FOLDER%

if not exist %DOWNLOAD_FOLDER%\%DOWNLOAD_FILE% (
  echo Downloading vc14 redist...
  curl --retry 5 --retry-all-errors --retry-connrefused --retry-delay 5 --location --output %DOWNLOAD_FOLDER%\%DOWNLOAD_FILE% %DOWNLOAD_URL%
)
:: Restore the previous current directory
POPD
