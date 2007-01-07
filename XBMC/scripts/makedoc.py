import pydoc, os, xbmc, xbmcgui

def makeDocDir():
	try:
		os.mkdir('q:\\doc')
		os.mkdir('q:\\doc\\python')
	except:
		pass

makeDocDir()		
doc = pydoc.HTMLDoc()

f = open('Q:\\doc\\python\\xbmc.html', 'w')
f.write(doc.document(xbmc))
f.close()

f = open('Q:\\doc\\python\\xbmcgui.html', 'w')
f.write(doc.document(xbmcgui))
f.close()
