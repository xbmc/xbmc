import xbmcgui

#get actioncodes from keymap.xml

ACTION_MOVE_LEFT 							= 1	
ACTION_MOVE_RIGHT							= 2
ACTION_MOVE_UP								= 3
ACTION_MOVE_DOWN							= 4
ACTION_PAGE_UP								= 5
ACTION_PAGE_DOWN							= 6
ACTION_SELECT_ITEM						= 7
ACTION_HIGHLIGHT_ITEM					= 8
ACTION_PARENT_DIR							= 9
ACTION_PREVIOUS_MENU					= 10
ACTION_SHOW_INFO							= 11

ACTION_PAUSE									= 12
ACTION_STOP										= 13
ACTION_NEXT_ITEM							= 14
ACTION_PREV_ITEM							= 15

class Window(xbmcgui.Window):
	def __init__(self):
		self.addControl(xbmcgui.ControlImage(0,0,400,400, 'q:\\skin\\mediacenter\\media\\background.png'))
		self.strAction = xbmcgui.ControlLabel(100, 100, 200, 200, '', 'font13')
		self.addControl(self.strAction)
	
	def onAction(self, action):
		if action == ACTION_PREVIOUS_MENU:
			print('action recieved: previous')
			self.close()
		if action == ACTION_SHOW_INFO:
			self.strAction.setText('action recieved: show info')
		if action == ACTION_STOP:
			self.strAction.setText('action recieved: stop')
		if action == ACTION_PAUSE:
			print('pause')
			dialog = xbmcgui.Dialog()
			dialog.ok('action recieved','ACTION_PAUSE')

w = Window()

c2 = xbmcgui.ControlLabel(100, 150, 200, 200, u'text', 'font14')
c2.setText('just some text')
w.addControl(c2)


w.doModal()

del w
del c2
