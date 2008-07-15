Prerequisites for building an XBMC for Windows installer:

1) A working Visual C++ 2008 Express XBMC environment (See also http://xbmc.org/wiki/?title=HOW-TO_compile_XBMC_for_Windows_from_source_code#Compiling_XBMC_using_Visual_C.2B.2B_2008_Express_Edition)
2) Nullsoft sciptable install system (http://nsis.sourceforge.net/Download)


Usage:
1) Run BuildSetup.bat in project\Win32BuildSetup
2) watch the screen, maybe you're asked for input
3) wait... wait... wait...
You should now have XBMCSetup-Rev<svnrevisionnr>.exe file.


Adding additional skins:
1) Copy or rename config.ini.template to config.ini if not already done.
2) Enter a path for "skinpath" in config.ini to where the skins are located.
3) The buildscript will try to find build.bat or build_skin.bat for each of the subdirectories of <skinpath> and build the skins.
   Only skins that output the build to BUILD\<skinname> will be included in the setup at this point.
   I build.bat is not found it will look for skin.xml and copy the directory if found (prebuilt skin).

Example:
Skins are located in E:\Data\XBMC\Skins and it contains the Basics-101 and xTV skin.
You edit config.ini to skinpath=E:\Data\XBMC\Skins
Build of Basics-101 is copied to E:\Data\XBMC\Skins\Basics-101\BUILD\Basics-101, ok, so it will be include in the installer.
Build of xTV is copied to E:\Data\XBMC\Skins\xTV\BUILD\xTV, ok, so it will be include in the installer.
When running the installer it will now provide Basics-101 and xTV as optional skins to install.

Adding scripts:
1) Copy or rename config.ini.template to config.ini if not already done.
2) Enter a path for "scriptpath" in config.ini to where the scripts are located.
3) The buildscript will try to find build.bat for each of the subdirectories of <scriptpath> and build the scripts.
   Only scripts that output the build to BUILD\<scriptname> will be included in the setup at this point.
   I build.bat is not found it will look for default.py and copy the directory if found (prebuilt script).

Adding plugins:
1) Copy or rename config.ini.template to config.ini if not already done.
2) Enter a path for "pluginpath" in config.ini to where the plugins are located.
   A directorystructure of <pluginpath>\video\, <pluginpath>\music\, <pluginpath>\programs\, <pluginpath>\pictures\ is expected.
3) The buildscript will try to find build.bat for each of the subdirectories in <pluginpath>\video,music,programs,pictures and build the plugins.
Only plugins that output the build to BUILD\<video/music/programs/picture>\<plugins> will be included in the setup at this point.
I build.bat is not found it will look for default.py and copy the directory if found (prebuilt plugin).


TODO:
-Add skin/script/plugin credits/revision to installoption description (if possible)
-When uninstalling ask user if profiles should be deleted as well (currently they are not uninstalled)
-Let user choose between install for all users or current user (at this point it installs for the  current user).
-Multilingual
-Webinstaller?
-...