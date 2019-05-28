#!/usr/bin/env python

import sys
import struct

if len(sys.argv) != 3:
	print "Usage: %s appname dest_file.xcent" % sys.argv[0]
	sys.exit(-1)

APPNAME = sys.argv[1]
DEST = sys.argv[2]

if not DEST.endswith('.xml') and not DEST.endswith('.xcent'):
	print "Dest must be .xml (for ldid) or .xcent (for codesign)"
	sys.exit(-1)

entitlements = """
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>application-identifier</key>
    <string>%s</string>
    <key>get-task-allow</key>
    <true/>
</dict>
</plist>
""" % APPNAME

f = open(DEST,'w')
if DEST.endswith('.xcent'):
	f.write("\xfa\xde\x71\x71")
	f.write(struct.pack('>L', len(entitlements) + 8))
f.write(entitlements)
f.close()

