import xbmc
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
	
		self.addControl(xbmcgui.ControlImage(0,0,720,576, 'background.png'))
		self.list = xbmcgui.ControlList(200, 100, 400, 400)
		self.strAction = xbmcgui.ControlLabel(50, 100, 100, 20, 'action', 'font13', '0xFFFF3300')
		self.strButton = xbmcgui.ControlLabel(50, 150, 100, 20, 'button', 'font13', '0xFFFFFFFF')
		
		self.addControl(self.list)
		self.addControl(self.strAction)
		self.addControl(self.strButton)
				
		self.button1 = xbmcgui.ControlButton(50, 200, 90, 30, "Button 1")
		self.button2 = xbmcgui.ControlButton(50, 240, 90, 30, "Button 2")
		self.addControl(self.button1)
		self.addControl(self.button2)
		
		self.button1.controlDown(self.button2)
		self.button1.controlRight(self.list)
		self.button2.controlUp(self.button1)
		self.button2.controlRight(self.list)
		self.list.controlLeft(self.button1)
		
		# add a few items to the list
		xbmcgui.lock()
		for i in range(50):
			self.list.addItem('item' + str(i))
		xbmcgui.unlock()
		self.setFocus(self.button1)
	
	def onAction(self, action):
		if action == ACTION_PREVIOUS_MENU:
			print('action recieved: previous')
			self.close()
		if action == ACTION_SHOW_INFO:
			self.strAction.setLabel('action recieved: show info')
		if action == ACTION_STOP:
			self.strAction.setLabel('action recieved: stop')
		if action == ACTION_PAUSE:
			print('pause')
			dialog = xbmcgui.Dialog()
			dialog.ok('action recieved','ACTION_PAUSE')
	
	def onControl(self, control):
		if control == self.button1:
			self.strButton.setLabel('button 1 clicked')
		elif control == self.button2:
			self.strButton.setLabel('button 2 clicked')
		elif control == self.list:
			item = self.list.getSelectedItem()
			self.strButton.setLabel('selected : ' + self.list.getSelectedItem().getLabel())
			item.setLabel(item.getLabel() + '1')

w = Window()
w.doModal()

del w
