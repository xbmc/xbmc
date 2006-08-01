'''
This module allows you to skin your python scripts with a standard XBMC skinfile.xml.
You may pass a dialog and a percent finished as a continuation for your scripts initialization.

It creates a dictionary of your controls, with the controls <id> as the key (self.controls{id : control}). It also
creates a dictionary of visibilty conditions, with the controls <id> as the key (self.visibility{id : condition}).
e.g.  self.controls[300].setVisible(xbmc.getCondVisibility(self.visibility[300]))

It creates a dictionary of your controls positions in a tuple, with the controls <id> as the key (self.positions{id : (x, y)}).
It also creates a list variable for a coordinates based window (self.coordinates[x, y]).
e.g. self.controls[300].setPosition(self.positions[300][0] + self.coordinates[0], self.positions[300][1] + self.coordinates[1])

Post a message at http://www.xbmc.xbox-scene.com/forum/ with any suggestions.

The Includes & references functions were translated from Xbox media center's source, thanks Developers.
A special thanks to elupus for shaming me into doing it the right way. :)

Nuka1195
'''

import xbmc, xbmcgui
import xml.dom.minidom

class GUIBuilder:
	def __init__(self, win, skinXML, ImagePath = '', title = 'GUI Builder', line1 = '', dlg = None, pct = 0):
		try:
			self.win 							= win
			self.win.SUCCEEDED	 			= True
			if (not skinXML): raise
			self.skinXML 						= skinXML[skinXML.rfind('\\') + 1:]
			self.pct 							= pct
			self.lineno 							= line1 != ''
			self.lines 							= [''] * 3
			self.lines[0] 						= line1
			self.lines[self.lineno] 			= 'Importing controls from %s.' % (self.skinXML,)
			if (dlg): self.dlg = dlg
			else: 
				self.dlg = xbmcgui.DialogProgress()
				self.dlg.create(title, self.lines[0], self.lines[1], self.lines[2])
			self.dlg.update(self.pct, self.lines[0], self.lines[1], self.lines[2])
			self.initVariables()
			self.pct1 = int((100 - pct) * 0.333)
			self.includesExist = self.LoadIncludes()
			self.referencesExist = self.LoadReferences()
			self.pct1 = int((100 - self.pct) * 0.5)
			self.parseSkinFile(ImagePath, skinXML)
			if (self.win.SUCCEEDED): self.setNav()
			if (not self.win.SUCCEEDED): raise
			else:
				if (self.defaultControl and self.win.controls.has_key(self.defaultControl)):
					self.win.setFocus(self.win.controls[self.defaultControl])
				if (self.includesExist): self.incdoc.unlink()
				self.dlg.close()
		except:
			self.win.SUCCEEDED = False
			if (dlg): dlg.close()
			dlg = xbmcgui.Dialog()
			dlg.ok(title, 'There was an error setting up controls.', 'Check your skin file:', skinXML)
			self.dlg.close()
		
	def initVariables(self):
		self.lines[self.lineno + 1]	= 'initializing variables...'
		self.dlg.update(self.pct, self.lines[0], self.lines[1], self.lines[2])
		self.win.SUCCEEDED	 				= True
		self.win.controls 						= {}
		self.win.visibility 						= {}
		self.win.positions						= {}
		#self.win.onclick 						= {}
		#self.win.onfocus 						= {}
		self.navigation						= {}


	def GetConditionalVisibility(self, conditions):
		if (len(conditions) == 0): return 'true'
		if (len(conditions) == 1): return conditions[0]
		else:
			#multiple conditions should be anded together
			conditionString = "["
			for i in range(len(conditions) - 1):
				conditionString += conditions[i] + "] + ["
			conditionString += conditions[len(conditions) - 1] + "]"
		return conditionString

	def addCtl(self, control):
		try:
			if (control['type'] == 'image'):
				if (control.has_key('info')): 
					if (control['info'][0] != ''): control['texture'] = xbmc.getInfoImage(control['info'][0])
				self.win.controls[int(control['id'])] = (xbmcgui.ControlImage(x=int(control['posx']) + self.posx,\
					y=int(control['posy']) + self.posy, width=int(control['width']), height=int(control['height']),\
					filename=control['texture'], colorKey=control['colorkey'], aspectRatio=int(control['aspectratio'])))#,\
					#colorDiffuse=control['colordiffuse']))
				self.win.addControl(self.win.controls[int(control['id'])])
			elif (control['type'] == 'label'):
				if (control.has_key('info')):
					if (control['info'][0] != ''): control['label'][0] = xbmc.getInfoLabel(control['info'][0])
				self.win.controls[int(control['id'])] = (xbmcgui.ControlLabel(x=int(control['posx']) + self.posx,\
					y=int(control['posy']) + self.posy, width=int(control['width']), height=int(control['height']), label=control['label'][0],\
					font=control['font'], textColor=control['textcolor'], disabledColor=control['disabledcolor'], alignment=control['align'],\
					hasPath=control['haspath'], angle=int(control['angle'])))#, shadowColor=control['shadowcolor']))
				self.win.addControl(self.win.controls[int(control['id'])])
			elif (control['type'] == 'button'):
				if (control.has_key('info')):
					if (control['info'][0] != ''): control['label'][0] = xbmc.getInfoLabel(control['info'][0])
				self.win.controls[int(control['id'])] = (xbmcgui.ControlButton(x=int(control['posx']) + self.posx,\
					y=int(control['posy']) + self.posy, width=int(control['width']), height=int(control['height']), label=control['label'][0],\
					font=control['font'], textColor=control['textcolor'], disabledColor=control['disabledcolor'], alignment=control['align'],\
					angle=int(control['angle']), shadowColor=control['shadowcolor'], focusTexture=control['texturefocus'],\
					noFocusTexture=control['texturenofocus'], textXOffset=int(control['textoffsetx']), textYOffset=int(control['textoffsety'])))
				self.win.addControl(self.win.controls[int(control['id'])])
			elif (control['type'] == 'checkmark'):
				if (control.has_key('info')):
					if (control['info'][0] != ''): control['label'][0] = xbmc.getInfoLabel(control['info'][0])
				self.win.controls[int(control['id'])] = (xbmcgui.ControlCheckMark(x=int(control['posx']) + self.posx,\
					y=int(control['posy']) + self.posy, width=int(control['width']), height=int(control['height']), label=control['label'][0],\
					font=control['font'], textColor=control['textcolor'], disabledColor=control['disabledcolor'], alignment=control['align'],\
					focusTexture=control['texturecheckmark'], noFocusTexture=control['texturecheckmarknofocus'],\
					checkWidth=int(control['markwidth']), checkHeight=int(control['markheight'])))
				self.win.addControl(self.win.controls[int(control['id'])])
			elif (control['type'] == 'textbox'):
				self.win.controls[int(control['id'])] = (xbmcgui.ControlTextBox(x=int(control['posx']) + self.posx,\
					y=int(control['posy']) + self.posy, width=int(control['width']), height=int(control['height']), font=control['font'],\
					textColor=control['textcolor']))
				self.win.addControl(self.win.controls[int(control['id'])])
				if (control.has_key('label')): self.win.controls[int(control['id'])].setText(control['label'][0])
			elif (control['type'] == 'fadelabel' or control['type'] == 'listcontrol'):
				if (control['type'] == 'fadelabel'):
					self.win.controls[int(control['id'])] = (xbmcgui.ControlFadeLabel(x=int(control['posx']) + self.posx,\
						y=int(control['posy']) + self.posy, width=int(control['width']), height=int(control['height']), font=control['font'],\
						textColor=control['textcolor'], alignment=control['align']))#, shadowColor=control['shadowcolor']))
					self.win.addControl(self.win.controls[int(control['id'])])
					if (control.has_key('info')):
						for item in control['info']:
							if (item != ''): self.win.controls[int(control['id'])].addLabel(xbmc.getInfoLabel(item))
					if (control.has_key('label')):
						for item in control['label']:
							if (item != ''): self.win.controls[int(control['id'])].addLabel(item)
				elif (control['type'] == 'listcontrol'):
					self.win.controls[int(control['id'])] = (xbmcgui.ControlList(x=int(control['posx']) + self.posx,\
						y=int(control['posy']) + self.posy, width=int(control['width']), height=int(control['height']), font=control['font'],\
						textColor=control['textcolor'], alignmentY=control['aligny'], buttonTexture=control['texturenofocus'],\
						buttonFocusTexture=control['texturefocus'], selectedColor=control['selectedcolor'], imageWidth=int(control['itemwidth']),\
						imageHeight=int(control['itemheight']), itemTextXOffset=int(control['textxoff']), itemTextYOffset=int(control['textyoff']),\
						itemHeight=int(control['textureheight']), space=int(control['spacebetweenitems'])))#, shadowColor=control['shadowcolor']))
					self.win.addControl(self.win.controls[int(control['id'])])
					if (control.has_key('label')):
						for cnt, item in enumerate(control['label']):
							if (item != ''): 
								if (cnt < len(control['label2'])): tmp = control['label2'][cnt]
								else: tmp = ''
								l = xbmcgui.ListItem(item, tmp, '', control['image'])
								self.win.controls[int(control['id'])].addItem(l)
			try: 
				self.win.controls[int(control['id'])].setVisible(xbmc.getCondVisibility(control['visible']))
				self.win.visibility[int(control['id'])] = control['visible']
				self.win.positions[int(control['id'])] = (int(control['posx']), int(control['posy']))
				self.navigation[int(control['id'])] = (int(control['onup']), int(control['ondown']), int(control['onleft']), int(control['onright']))
			except: pass
		except:
			self.dlg.close()
			self.win.SUCCEEDED = False


	def parseSkinFile(self, ImagePath, filename):
		try:
			self.lines[self.lineno + 1]	= 'parsing %s file...' % (self.skinXML,)
			# load and parse skin.xml file
			skindoc = xml.dom.minidom.parse(filename)
			self.posx = 0
			self.posy = 0
			try:
				default = skindoc.getElementsByTagName('defaultcontrol')
				if (len(default)):
					if (default[0].hasChildNodes()): self.defaultControl = int(default[0].firstChild.nodeValue)
					else: self.defaultControl = None
				coordinates = skindoc.getElementsByTagName('coordinates')
				if (len(coordinates)):
					systemBase = self.FirstChildElement(coordinates[0], 'system')
					if (systemBase and systemBase.firstChild): 
						system = int(systemBase.firstChild.nodeValue)
						if (system == 1):
							posx = self.FirstChildElement(coordinates[0], 'posx')
							if (posx and posx.firstChild): self.posx = int(posx.firstChild.nodeValue)
							posy = self.FirstChildElement(coordinates[0], 'posy')
							if (posy and posy.firstChild): self.posy = int(posy.firstChild.nodeValue)
			except: pass
			self.win.coordinates = [self.posx, self.posy]
			data = skindoc.getElementsByTagName('control')
			t = len(data)
			if (not data): raise
			for cnt, control in enumerate(data):
				self.dlg.update(int((float(self.pct1) / float(t) * (cnt + 1)) + self.pct), self.lines[0], self.lines[1], self.lines[2])
				if (self.includesExist): tmp = self.ResolveInclude(control)
				else: tmp = control
				ctl = {}
				node 		= self.FirstChildElement(tmp, None)
				lbl1 		= []
				lbl2 		= []
				ifo 		= []
				vis 		= []
				ctype 	= ''

				while (node):
					# key node so save to the dictionary
					if (node.tagName.lower() == 'label'): 
						try: 
							ls = xbmc.getLocalizedifong(int(node.firstChild.nodeValue))
							if (ls): lbl1.append(ls)
							else: raise
						except: 
							if (node.hasChildNodes()): lbl1.append(node.firstChild.nodeValue)
					elif (node.tagName.lower() == 'label2'):
						try: 
							ls = xbmc.getLocalizedifong(int(node.firstChild.nodeValue))
							if (ls): lbl2.append(ls)
							else: raise
						except:
							if (node.hasChildNodes()): lbl2.append(node.firstChild.nodeValue)
					elif (node.tagName.lower() == 'info'):
						if (node.hasChildNodes()): ifo.append(node.firstChild.nodeValue)
					elif (node.tagName.lower() == 'visible'):
						if (node.hasChildNodes()): vis.append(node.firstChild.nodeValue)
					elif (node.hasChildNodes()): 
						if (node.tagName.lower() == 'type'): ctype = node.firstChild.nodeValue
						ctl[node.tagName.lower()] = node.firstChild.nodeValue
					node = self.NextSiblingElement(node, None)

				if (ctype):
					# The following apply to all controls
					if (not ctl.has_key('id')): ctl['id'] = self.m_references.get(ctype + '_id', '0')
					if (not ctl.has_key('posx')): ctl['posx'] = self.m_references.get(ctype + '_posx', '0')
					if (not ctl.has_key('posy')): ctl['posy'] = self.m_references.get(ctype + '_posy', '0')
					if (not ctl.has_key('width')): ctl['width'] = self.m_references.get(ctype + '_width', '100')
					if (not ctl.has_key('height')): ctl['height'] = self.m_references.get(ctype + '_height', '100')
					if (not ctl.has_key('onup')): ctl['onup'] = self.m_references.get(ctype + '_onup', ctl['id'])
					if (not ctl.has_key('ondown')): ctl['ondown'] = self.m_references.get(ctype + '_ondown', ctl['id'])
					if (not ctl.has_key('onleft')): ctl['onleft'] = self.m_references.get(ctype + '_onleft', ctl['id'])
					if (not ctl.has_key('onright')): ctl['onright'] = self.m_references.get(ctype + '_onright', ctl['id'])
					if (vis): ctl['visible'] = self.GetConditionalVisibility(vis)
					else: ctl['visible'] = self.m_references.get(ctype + '_visible', 'true')
					#if (not ctl.has_key('onclick')): ctl['onclick'] = self.m_references.get(ctype + '_onclick', '')
					#self.onClick.append(ctl['onclick'])
					#if (not ctl.has_key('onfocus')): ctl['onfocus'] = self.m_references.get(ctype + '_onfocus', '')
					#self.onFocus.append(ctl['onfocus'])

					if (ctype == 'image' or ctype == 'label' or ctype == 'fadelabel' or ctype == 'button' or ctype == 'checkmark' or ctype == 'textbox'):
						if (ifo): ctl['info'] = ifo
						else: ctl['info'] = [self.m_references.get(ctype + '_info', '')]
					if (ctype == 'label' or ctype == 'fadelabel' or ctype == 'button' or ctype == 'checkmark' or ctype == 'textbox' or ctype == 'listcontrol'):
						if (lbl1): ctl['label'] = lbl1
						else: ctl['label'] = [self.m_references.get(ctype + '_label', '')]
						if (not ctl.has_key('shadowcolor')): ctl['shadowcolor'] = self.m_references.get(ctype + '_shadowcolor', '')
						if (not ctl.has_key('font')): ctl['font'] = self.m_references.get(ctype + '_font', 'font12')
						if (not ctl.has_key('textcolor')): ctl['textcolor'] = self.m_references.get(ctype + '_textcolor', '0xFF000000')

					if (ctype == 'label' or ctype == 'fadelabel' or ctype == 'button' or ctype == 'checkmark' or ctype == 'listcontrol'):
						if (not ctl.has_key('align')): ctl['align'] = self.m_references.get(ctype + '_align', 0)
						if (ctl['align'] == 'left'): ctl['align'] = 0
						elif (ctl['align'] == 'right'): ctl['align'] = 1
						elif (ctl['align'] == 'center'): ctl['align'] = 2
						else: ctl['align'] = 0
						if (not ctl.has_key('aligny')): ctl['aligny'] = self.m_references.get(ctype + '_aligny', 0)
						if (ctl['aligny'] == 'center'): ctl['aligny'] = 4
						else: ctl['aligny'] = 0
						ctl['align'] += ctl['aligny']
							
					if (ctype == 'label' or ctype == 'button' or ctype == 'checkmark'):
						if (not ctl.has_key('disabledcolor')): ctl['disabledcolor'] = self.m_references.get(ctype + '_disabledcolor', '0x60000000')

					if (ctype == 'label' or ctype == 'button'):
						if (not ctl.has_key('angle')): ctl['angle'] = self.m_references.get(ctype + '_angle', '0')

					if (ctype == 'listcontrol' or ctype == 'button'):
						if (not ctl.has_key('texturefocus')): ctl['texturefocus'] = self.m_references.get(ctype + '_texturefocus', '')
						elif (ctl['texturefocus'][0] == '\\'): ctl['texturefocus'] = ImagePath + ctl['texturefocus']
						if (not ctl.has_key('texturenofocus')): ctl['texturenofocus'] = self.m_references.get(ctype + '_texturenofocus', '')
						elif (ctl['texturenofocus'][0] == '\\'): ctl['texturenofocus'] = ImagePath + ctl['texturenofocus']
						
					if (ctype == 'image'):
						if (not ctl.has_key('aspectratio')): ctl['aspectratio'] = self.m_references.get(ctype + '_aspectratio', 0)
						if (ctl['aspectratio'] == 'stretch'): ctl['aspectratio'] = 0
						elif (ctl['aspectratio'] == 'scale'): ctl['aspectratio'] = 1
						elif (ctl['aspectratio'] == 'keep'): ctl['aspectratio'] = 2
						else: ctl['aspectratio'] = 0
						if (not ctl.has_key('colorkey')): ctl['colorkey'] = self.m_references.get(ctype + '_colorkey', '')
						if (not ctl.has_key('colordiffuse')): ctl['colordiffuse'] = self.m_references.get(ctype + '_colordiffuse', '0xFFFFFFFF')
						if (not ctl.has_key('texture')): ctl['texture'] = self.m_references.get(ctype + '_texture', '')
						elif (ctl['texture'][0] == '\\'): ctl['texture'] = ImagePath + ctl['texture']

					if (ctype == 'label'):
						if (not ctl.has_key('haspath')): ctl['haspath'] = self.m_references.get(ctype + '_haspath', 0)
						if (ctl['haspath'] == 'false' or ctl['haspath'] == 'no'): ctl['haspath'] = 0
						elif (ctl['haspath'] == 'true' or ctl['haspath'] == 'yes'): ctl['haspath'] = 1
						else: ctl['haspath'] = 0
						if (ctl.has_key('number')): ctl['label'][0] = [ctl['number']]

					if (ctype == 'button'):
						if (not ctl.has_key('textoffsetx')): ctl['textoffsetx'] = self.m_references.get(ctype + '_textoffsetx', '0')
						if (not ctl.has_key('textoffsety')): ctl['textoffsety'] = self.m_references.get(ctype + '_textoffsety', '0')
							
					if (ctype == 'checkmark'):
						if (not ctl.has_key('texturecheckmark')): ctl['texturecheckmark'] = self.m_references.get(ctype + '_texturecheckmark', '')
						elif (ctl['texturecheckmark'][0] == '\\'): ctl['texturecheckmark'] = ImagePath + ctl['texturecheckmark']
						if (not ctl.has_key('texturecheckmarknofocus')): ctl['texturecheckmarknofocus'] = self.m_references.get(ctype + '_texturecheckmarknofocus', '')
						elif (ctl['texturecheckmarknofocus'][0] == '\\'): ctl['texturecheckmarknofocus'] = ImagePath + ctl['texturecheckmarknofocus']
						if (not ctl.has_key('markwidth')): ctl['markwidth'] = self.m_references.get(ctype + '_markwidth', '20')
						if (not ctl.has_key('markheight')): ctl['markheight'] = self.m_references.get(ctype + '_markheight', '20')

					if (ctype == 'listcontrol'):
						if (lbl2): ctl['label2'] = lbl2
						else: ctl['label2'] = ['']
						if (not ctl.has_key('selectedcolor')): ctl['selectedcolor'] = self.m_references.get(ctype + '_selectedcolor', '0xFFFFFFFF')
						if (not ctl.has_key('itemwidth')): ctl['itemwidth'] = self.m_references.get(ctype + '_itemwidth', '20')
						if (not ctl.has_key('itemheight')): ctl['itemheight'] = self.m_references.get(ctype + '_itemheight', '20')
						if (not ctl.has_key('textureheight')): ctl['textureheight'] = self.m_references.get(ctype + '_textureheight', '20')
						if (not ctl.has_key('textxoff')): ctl['textxoff'] = self.m_references.get(ctype + '_textxoff', '0')
						if (not ctl.has_key('textyoff')): ctl['textyoff'] = self.m_references.get(ctype + '_textyoff', '0')
						if (not ctl.has_key('spacebetweenitems')): ctl['spacebetweenitems'] = self.m_references.get(ctype + '_spacebetweenitems', '0')
						if (not ctl.has_key('image')): ctl['image'] = self.m_references.get(ctype + '_image', '')
						if (not ctl.has_key('image')): ctl['image'] = self.m_references.get(ctype + '_image', '')
						elif (ctl['image'][0] == '\\'): ctl['image'] = ImagePath + ctl['image']

				self.addCtl(ctl)
			self.pct += self.pct1
		except:
			self.dlg.close()
			self.win.SUCCEEDED = False
		else:	skindoc.unlink()

	def setNav(self):
		try:
			self.lines[self.lineno + 1]	= 'setting up navigation...'
			t = len(self.navigation)
			for cnt, item in enumerate(self.navigation):
				self.dlg.update(int((float(self.pct1) / float(t) * (cnt + 1)) + self.pct), self.lines[0], self.lines[1], self.lines[2])
				if (self.win.controls.has_key(item)):
					if (self.win.controls.has_key(self.navigation[item][0])):
						self.win.controls[item].controlUp(self.win.controls[self.navigation[item][0]])
					if (self.win.controls.has_key(self.navigation[item][1])):
						self.win.controls[item].controlDown(self.win.controls[self.navigation[item][1]])
					if (self.win.controls.has_key(self.navigation[item][2])):
						self.win.controls[item].controlLeft(self.win.controls[self.navigation[item][2]])
					if (self.win.controls.has_key(self.navigation[item][3])):
						self.win.controls[item].controlRight(self.win.controls[self.navigation[item][3]])
		except:
			self.dlg.close()
			self.win.SUCCEEDED = False


	def GetSkinPath(self, filename):
		from os import path
		paths = {'1080i' : 0, '720p' : 1, '480p' : 2, '480p16x9' : 3, 'ntsc' : 4, 'ntsc16x9' : 5, 'pal' : 6, 'pal16x9' : 7, 'pal60' : 8, 'pal6016x9' : 9}
		paths2 = ('1080i', '720p', '480p', '480p16x9', 'ntsc', 'ntsc16x9', 'pal', 'pal16x9', 'pal60', 'pal6016x9')
		res = self.win.getResolution()
		default = 6
		defaultwide = 7
		try:
			fname = 'Q:\\skin\\' + xbmc.getSkinDir() + '\\skin.xml'
			skindoc = xml.dom.minidom.parse(fname)
			root = skindoc.documentElement
			if (not root or root.tagName != 'skin'): raise
			strDefault = skindoc.getElementsByTagName('defaultresolution')
			strDefaultWide = skindoc.getElementsByTagName('defaultresolutionwide')
			default = paths.get(strDefault[0].firstChild.nodeValue.lower(), default)
			defaultwide = paths.get(strDefaultWide[0].firstChild.nodeValue.lower(), defaultwide)
			skindoc.unlink()
		except: pass
		fname = 'Q:\\skin\\' + xbmc.getSkinDir() + '\\' + paths2[res] + '\\' + filename
		if (path.exists(fname)):
			if (filename == 'includes.xml'): self.win.setCoordinateResolution(res)
			return fname
		# if we're in 1080i mode, try 720p next
		if (res == 0):
			fname = 'Q:\\skin\\' + xbmc.getSkinDir() + '\\' + paths2[1] + '\\' + filename
			if (path.exists(fname)):
				if (filename == 'includes.xml'): self.win.setCoordinateResolution(1)
				return fname
		# that failed - drop to the default widescreen resolution if we're in a widemode
		if (res == 9 or res == 7 or res == 5 or res == 3 or res == 1):
			fname = 'Q:\\skin\\' + xbmc.getSkinDir() + '\\' + paths2[defaultwide] + '\\' + filename
			if (path.exists(fname)):
				if (filename == 'includes.xml'): self.win.setCoordinateResolution(defaultwide)
				return fname
		# that failed - drop to the default resolution
		fname = 'Q:\\skin\\' + xbmc.getSkinDir() + '\\' + paths2[default] + '\\' + filename
		if (path.exists(fname)): res = default
		if (path.exists(fname)):
			if (filename == 'includes.xml'): self.win.setCoordinateResolution(default)
			return fname
		else: return None

	
	def LoadReferences(self):
		self.lines[self.lineno + 1]	= 'loading references.xml file...'
		self.dlg.update(self.pct, self.lines[0], self.lines[1], self.lines[2])
		self.pct += self.pct1
		# make sure our reference map is cleared.
		self.m_references = {}
		# get the references.xml file location if it exists
		referenceFile = self.GetSkinPath('references.xml')
		# load and parse references.xml file
		try: refdoc = xml.dom.minidom.parse(referenceFile)
		except: return False
		root = refdoc.documentElement
		if (not root or root.tagName != 'controls'): return False
		data = refdoc.getElementsByTagName('control')
		for control in data:
			if (self.includesExist): tmp = self.ResolveInclude(control)
			else: tmp = control
			t = tmp.getElementsByTagName('type')
			if (t[0].hasChildNodes()): 
				tagName = t[0].firstChild.nodeValue
				node = self.FirstChildElement(tmp, None)
				while (node):
					# key node so save to the dictionary
					if (node.tagName.lower() != 'type'):
						if (node.hasChildNodes()): 
							self.m_references[tagName + '_' + node.tagName.lower()] = node.firstChild.nodeValue
					node = self.NextSiblingElement(node, None)
		refdoc.unlink()
		return True


	def FirstChildElement(self, root, value = 'include'):
		node = root.firstChild
		while (node):
			if (node and node.nodeType == 1):
				if (node.tagName == value or not value):
					return node
			node = node.nextSibling
		return None


	def NextSiblingElement(self, node, value = 'include'):
		while (node):
			node = node.nextSibling
			if (node and node.nodeType == 1):
				if (node.tagName == value or not value):
					return node
		return None


	def LoadIncludes(self):
		self.lines[self.lineno + 1]	= 'loading includes.xml file...'
		self.dlg.update(self.pct, self.lines[0], self.lines[1], self.lines[2])
		self.pct += self.pct1
		# make sure our include map is cleared.
		self.m_includes = {}
		# get the includes.xml file location if it exists
		includeFile = self.GetSkinPath('includes.xml')
		# load and parse includes.xml file
		try: self.incdoc = xml.dom.minidom.parse(includeFile)
		except: return False
		root = self.incdoc.documentElement
		if (not root or root.tagName != 'includes'):
			self.incdoc.unlink()
			return False
		node = self.FirstChildElement(root)
		while (node):
			if (node.attributes["name"] and node.firstChild):
				# key node so save to the dictionary
				tagName = node.attributes["name"].value
				self.m_includes[tagName] = node
			node = self.NextSiblingElement(node)
		return True


	def ResolveInclude(self, node):
		# we have a node, find any <include>tagName</include> tags and replace
		# recursively with their real includes
		if (not node): return None
		include = self.FirstChildElement(node)
		while (include and include.firstChild):
			# have an include tag - grab it's tag name and replace it with the real tag contents
			tagName = include.firstChild.toxml()
			try: 
				element = self.m_includes[tagName].cloneNode(True)
				#found the tag(s) to include - let's replace it
				for tag in element.childNodes:
					# we insert before the <include> element to keep the correct
					# order (we render in the order given in the xml file)
					if (tag.nodeType == 1): result = node.insertBefore(tag, include)
			except: pass# invalid include
			#try:
			result = node.removeChild(include)
			include.unlink()
			#except: pass
			include = self.FirstChildElement(node)
		return node

