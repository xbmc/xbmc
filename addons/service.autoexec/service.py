import xbmc
import xbmcaddon
__addon__=xbmcaddon.Addon()

if __addon__.getSettingBool('enable_autoexec'):
    xbmc.executebuiltin('RunScript(special://profile/autoexec.py)')
