# This 2-line script brings up the Wikipedia entry for the artist of the
# currently playing song

import xbmc
xbmc.executebuiltin('XBMC.BrowseURL(http://en.wikipedia.org/wiki/Special:Search/'+xbmc.Player().getMusicInfoTag().getArtist()+')')

