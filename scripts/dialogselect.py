import xbmc

dialog = xbmc.Dialog()

list = ['yes','no','cancel','movies','f:\\music\\test','test','67','true']
list.append('last item')
selected = dialog.select('select an item', list)
dialog.ok('user selected', 'item nr : ' + str(selected), list[selected])

selected = dialog.select('select an action', ['Cancel', 'Reboot', 'Shut Down'])

if selected == 1:
	xbmc.restart()
elif selected == 2:
	xbmc.shutdown()