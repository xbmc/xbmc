import xbmcgui
"""
Usage Explained

xbmcgui.Dialog().browse(type,prompt,shares)
type can be
    0 for directory
    1  for files
    2 for images (eg it will show thumbnails if skin shows it)
    any other number will do directories

prompt can be text/string

shares can be
- files
- videos
- music
- programs
- pictures
"""
print xbmcgui.Dialog().browse(0,"a Directory","files")
print xbmcgui.Dialog().browse(1,"a file","files")
print xbmcgui.Dialog().browse(1,"a music file","music")
print xbmcgui.Dialog().browse(1,"a image","files")


