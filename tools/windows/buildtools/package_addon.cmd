@echo off

for /f "delims=\" %%a in ("%cd%") do set addon_id=%%~nxa

if "%kodi_src%" EQU "" (
  set kodi_src=%1%
)

if not "%2%" EQU "" (
set addon_install=%2%
) else (
set addon_install=%kodi_src%\addons
)

set addon_folder=%cd%

echo using %kodi_src% to package add-on: %addon_id%

if not defined DevEnvDir (
  call "%VS140COMNTOOLS%..\..\VC\bin\vcvars32.bat"
)

if not exist %addon_folder%\package\build mkdir %addon_folder%\package\build
cd %addon_folder%\package\build

:: use / delimiter for paths to make CMake happy
set cmake_addon_folder=%addon_folder:\=/%
set cmake_kodi_src=%kodi_src:\=/%
set cmake_addon_install=%addon_install:\=/%

cmake -G "NMake Makefiles" -DADDONS_TO_BUILD="%addon_id%" -DADDON_SRC_PREFIX="%cmake_addon_folder%/.." -DADDONS_DEFINITION_DIR="%cmake_addon_folder%/../repo-binary-addons" -DCMAKE_INSTALL_PREFIX="%cmake_addon_install%" -DCMAKE_VERBOSE_MAKEFILE=ON -DPACKAGE_ZIP=ON -DPACKAGE_DIR=%cmake_addon_folder%/package -DCMAKE_BUILD_TYPE=Release "%cmake_kodi_src%/project/cmake/addons"

%cmake_kodi_src%/tools/windows/buildtools/jom.exe -j8 %addon_id%
%cmake_kodi_src%/tools/windows/buildtools/jom.exe -j8 package-%addon_id%

:ERROR
::TODO implement error handling

cd %addon_folder%

