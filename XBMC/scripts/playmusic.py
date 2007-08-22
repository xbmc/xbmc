import xbmc, xbmcgui, os

dir = '/mnt/audio/misc/' #sample music dir, do not forget trailing /
list = os.listdir(dir)

dialog = xbmcgui.Dialog()
selected = dialog.select('select a song to play, no directory!!!', list)
filename = list[selected]

xbmc.Player().play(dir + filename)
