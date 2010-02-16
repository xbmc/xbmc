OVERVIEW:

Plugin to browse and install XBMC Addons (Scripts/Plugins) from SVN Repositories. 
Can also check for updates of your installed Addons.

SETUP:
 *Requires XBMC revision 19001 or newer to run.
 Install to Plugins\Programs\SVN Repo Installer  (IMPORTANT! keep sub folder structure intact)


USAGE: 
 1) From XBMC Home Screen choose "Programs"   (you may need to first enable this in your chosen XBMC skin)
 2) Select "Program Plugins"
 3) Select "SVN Repo Installer"
    Plugin will run and present you with its root categories:

To Browse and Install an Addon:
 1) Select any of the Repositories
    XBMC-ADDONS Contains both Plugins and Scripts
    XBMC-SCRIPTING Contains just scripts

 2) Select Addon
 3) Select INSTALL
 4) Context menu to view readme(if available) or changelog

To Check For Updates:
  1) Select "Check For Updates"
     Your installed Addon versions will be checked against those in SVN Repositories.
  2) Version Status list will be shown.

  Version Status List explain:
    OK                    = The Addon is upto date.
    New                   = A newer version is available.  Select to install.
    Incompatible          = Your XBMC build needs updating inorder to run this Addon.
    Not in SVN or SVN ?   = Not found in any SVN Repo.
    Unknown Version or v? = Addon does not contain __version__ tag.
 
CHANGELOGS:
 Selecting "View Changelog" from Plugin settings will show installers full changelog.

 Selecting "View Changelog" whilst in root folder will show the selected repositories changelog.

 Selecting "View Changelog" whilst browsing addons or updates will show the selected addon changelog.

README:
 Selecting "View Readme" from Plugin settings will show installers /resources/readme.txt.

 Selecting "View Readme" whilst in root folder will show installers /resources/readme.txt.

 Selecting "View Readme" whilst browsing addons or updates (if avaliable) will show the selected addon /resources/readme.txt.

DELETING AN ADDON:
  From "Check for Updates" in each addons Context Menu, there is a "Delete" option.

  When deleting an installed Addon, it first makes a backup copy to <category/.backups/<addonname>, then deletes Addon.

  When deleting a Backup of an Addon, it delete it from <category/.backups/<addonname>.


CREDITS:
 Written By Nuka1195/BigBellyBilly

 Thanks to others who've contributed.

 Project Homepage: http://code.google.com/p/xbmc-addons/

 Forum: http://forum.xbmc.org

 XBMC Homepage: http://xbmc.org

enjoy!
