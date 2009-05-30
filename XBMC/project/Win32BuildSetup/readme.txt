Prerequisites for building an XBMC for Windows installer:

1) A working Visual C++ 2008 Express XBMC environment (See also http://xbmc.org/wiki/?title=HOW-TO_compile_XBMC_for_Windows_from_source_code#Compiling_XBMC_using_Visual_C.2B.2B_2008_Express_Edition)
2) Nullsoft sciptable install system (http://nsis.sourceforge.net/Download)


Usage:
1) Copy the microsoft runtime dlls msvcp71.dll and msvcr71.dll into dependencies\
2) Copy all plugins, skins and scripts you wish to include into the Add_* folders
   Currently the following items are included:
   - The Apple Movie Trailers script
   - The Apple Movie Trailers Lite video plugin
   - The SVN Repo Installer program plugin
   - A favourites.xml pointing to the SVN Repo Installer
   - The MediaStream skin

3) Run BuildSetup.bat in project\Win32BuildSetup
4) Watch the screen, maybe you're asked for input
5) Wait... Wait... Wait...
You should now have XBMCSetup-Rev<svnrevisionnr>.exe file.


Adding additional skins:
1) Copy the desired skins into Add_skins\
2) The buildscript will try to find build.bat or build_skin.bat for each of the subdirectories of <skinpath> and build the skins.
   Only skins that output the build to BUILD\<skinname> will be included in the setup at this point.
   If build.bat is not found it will look for skin.xml and copy the directory if found (prebuilt skin).

Adding scripts:
1) Copy the desired scripts into Add_scripts\
2) The buildscript will try to find build.bat for each of the subdirectories of <scriptpath> and build the scripts.
   Only scripts that output the build to BUILD\<scriptname> will be included in the setup at this point.
   If build.bat is not found it will look for default.py and copy the directory if found (prebuilt script).

Adding plugins:
1) Copy the desired plugins into Add_plugins\video, Add_plugins\music, Add_plugins\programs or Add_plugins\pictures
2) The buildscript will try to find build.bat for each of the subdirectories in <pluginpath>\video,music,programs,pictures and build the plugins.
   If build.bat is not found it will look for default.py and copy the directory if found (prebuilt plugin).


TODO:
-Add skin/script/plugin credits/revision to installoption description (if possible)
-When uninstalling ask user if profiles should be deleted as well (currently they are not uninstalled)
-Let user choose between install for all users or current user (at this point it installs for the  current user).
-Multilingual
-Webinstaller?
-...