import xbmc

dialog = xbmc.Dialog()
selected = dialog.yesno("header", "doit", "line2", "line3")

dialog.ok("error", "user selected" + str(selected))

if dialog.yesno("restart", "sure?"):
	xbmc.restart()

