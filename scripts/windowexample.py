from xbmcgui import *
w = Window()
c1 = ControlLabel(100, 100, 200, 200, 'justsometext', 'font13')
c2 = ControlLabel(100, 150, 200, 200, u'text', 'font14')
p = ControlImage(0,0,400,400, 'q:\\skin\\mediacenter\\media\\background.png')
w.addControl(p)
w.addControl(c1)
w.addControl(c2)
w.show()
c2.setText(u'texttesttttttt')
counter = 1
while 1:
	c2.setText(str(counter))
	counter = counter + 1
