import xbmcgui

# get actioncodes from keymap.xml, if you leave these away and still use something like
# ACTION_PREVIOUS_MENU, python wil use it as if it was 0 

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

# NewDialog class with xbmcgui.WindowDialog as it's base class
# note xbmcgui.WindowDialog has xbmcgui.Window as it's baseclass so we can use this
# dialog the same as a window

class NewDialog(xbmcgui.WindowDialog):
	def __init__(self):
		
		# create result variable and add some images + buttons to our dialog
		self.result = 0
		self.addControl(xbmcgui.ControlImage(177,225,406,140, 'dialog-popup.png'))
		
		self.buttonOK = xbmcgui.ControlButton(290, 330, 80, 32, 'OK')
		self.buttonCancel = xbmcgui.ControlButton(380, 330, 80, 32, 'Cancel')
		self.addControl(self.buttonOK)
		self.addControl(self.buttonCancel)
		
		# setting up navigation and focus
		self.setFocus(self.buttonOK)
		self.buttonOK.controlRight(self.buttonCancel)
		self.buttonOK.controlLeft(self.buttonCancel)
		self.buttonCancel.controlRight(self.buttonOK)
		self.buttonCancel.controlLeft(self.buttonOK)
		
		# labels
		self.lblHeading = xbmcgui.ControlLabel(190, 226, 200, 20, '')
		
		self.lblLine = []
		self.lblLine.append(xbmcgui.ControlLabel(190, 260, 200, 20, ''))
		self.lblLine.append(xbmcgui.ControlLabel(190, 280, 200, 20, ''))
		self.lblLine.append(xbmcgui.ControlLabel(190, 300, 200, 20, ''))
		self.addControl(self.lblHeading)
		self.addControl(self.lblLine[0])
		self.addControl(self.lblLine[1])
		self.addControl(self.lblLine[2])
		
	def setHeading(self, text):
		self.lblHeading.setLabel(text)
 
 	def setLine(self, line, text):
		self.lblLine[line].setLabel(text)

	def onAction(self, action):
		if action == ACTION_PREVIOUS_MENU:
			# previous menu action recieved, set result to 0 (cancel / aborted) and close the window
			self.result = 0
			self.close()
	
	def onControl(self, control):
		if control == self.buttonOK:
			# ok butten pressed, set result to 1 and close the dialog
			self.result = 1
			self.close()
		elif control == self.buttonCancel:
			# ok butten pressed, set result to 0 and close the dialog
			self.result = 0
			self.close()

	def ask(self):
		# show this dialog and wait until it's closed
		self.doModal()
		return self.result

dialog = NewDialog()
dialog.setHeading('Python OK / Cancel Dialog')
dialog.setLine(0, 'line 0')
dialog.setLine(1, 'line 1')
dialog.setLine(2, 'line 2')

print(dialog.ask())
# print('dialog returned' + str(dialog.ask())

