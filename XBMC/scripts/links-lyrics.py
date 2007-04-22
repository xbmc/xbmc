# This 2-line script brings up the Lyrc.com.ar lyrics page for the
# currently playing song

import xbmc
xbmc.executebuiltin('XBMC.BrowseURL(http://lyrc.com.ar/en/tema1en.php?artist='+xbmc.Player().getMusicInfoTag().getArtist()+'&songname='+xbmc.Player().getMusicInfoTag().getTitle()+')'))

