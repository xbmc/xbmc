## MediaInfoDLL - All info about media files
# This software is provided 'as-is', without any express or implied
# warranty.  In no event will the authors be held liable for any damages
# arising from the use of this software.
#
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely, subject to the following restrictions:
#
# 1. The origin of this software must not be misrepresented; you must not
#    claim that you wrote the original software. If you use this software
#    in a product, an acknowledgment in the product documentation would be
#    appreciated but is not required.
# 2. Altered source versions must be plainly marked as such, and must not be
#    misrepresented as being the original software.
# 3. This notice may not be removed or altered from any source distribution.
#
#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#
# Python (Windows)  example
#
# To make this example working, you must put MediaInfo.Dll and test.avi
# in the same folder
#
#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#
# Should be "import MediaInfoDLL" but does not work, why?
# How to import MediaInfoDLL.py correctly?
# Example following
#
#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

from MediaInfoDLL import *

MI = MediaInfo()

Version=MI.Option_Static("Info_Version", "0.7.7.0;MediaInfoDLL_Example_Python;0.7.7.0")
if Version=="":
	print "MediaInfo.Dll: this version of the DLL is not compatible"
	exit


#Information about MediaInfo
print "Info_Parameters"
print MI.Option_Static(u"Info_Parameters")

print
print "Info_Capacities"
print MI.Option_Static(u"Info_Capacities")

print
print "Info_Codecs"
print MI.Option_Static(u"Info_Codecs")


#An example of how to use the library
print
print "Open"
MI.Open(u"Example.ogg")

print
print "Inform with Complete=false"
MI.Option_Static("Complete")
print MI.Inform()

print
print "Inform with Complete=true"
MI.Option_Static(u"Complete", u"1")
print MI.Inform()

print
print "Custom Inform"
MI.Option_Static(u"Inform", u"General;Example : FileSize=%FileSize%")
print MI.Inform()

print
print "Get with Stream=General and Parameter='FileSize'"
print MI.Get(Stream.General, 0, u"FileSize")

print
print "GetI with Stream=General and Parameter=46"
print MI.GetI(Stream.General, 0, 46)

print
print "Count_Get with StreamKind=Stream_Audio"
print MI.Count_Get(Stream.Audio)

print
print "Get with Stream=General and Parameter='AudioCount'"
print MI.Get(Stream.General, 0, u"AudioCount")

print
print "Get with Stream=Audio and Parameter='StreamCount'"
print MI.Get(Stream.Audio, 0, u"StreamCount")

print
print "Close"
MI.Close()
