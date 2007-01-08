import xbmc, xbmcgui, re, os

thumbDir = "Q:\\thumbs"
files = os.listdir(thumbDir)
test = re.compile("\.tbn$", re.IGNORECASE)
files = filter(test.search, files)

for file in files:
	srcPath = thumbDir + "\\" + file
	size = len(file)
	diff = 12 - size
	dest = ("0" * diff) + file.lower()
	subDir = dest[0]
	destPath = thumbDir + "\\" + subDir + "\\" + dest
	if not os.path.exists(destPath):
		os.rename(srcPath,destPath)
