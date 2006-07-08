import xbmcgui
"""
Usage Explained


xbmcgui.Dialog().numeric(type,heading)

"""
print xbmcgui.Dialog().numeric(0,"Enter a Number")
print xbmcgui.Dialog().numeric(1,"Enter a Date")
print xbmcgui.Dialog().numeric(2,"Enter a Time")
print xbmcgui.Dialog().numeric(3,"Enter a Ip")