import xbmc, xbmcgui, nt

dir = 'f:\\music\\mp3\\'
list = nt.listdir(dir)

dialog = xbmcgui.Dialog()
selected = dialog.select('select a song to play, no directory!!!', list)
filename = list[selected]

xbmc.mediaplay(dir + filename)
