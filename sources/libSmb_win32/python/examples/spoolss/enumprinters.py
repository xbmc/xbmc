#!/usr/bin/env python
#
# Display information on all printers on a print server.  Defaults to
# printer info level 1.
#
# Example: enumprinters.py win2kdc1
#

import sys
from samba import spoolss

if len(sys.argv) < 2 or len(sys.argv) > 3:
    print "Usage: enumprinters.py <servername> [infolevel]"
    sys.exit(1)

printserver = sys.argv[1]

level = 1
if len(sys.argv) == 3:
    level = int(sys.argv[2])
        
# Get list of printers

try:
    printer_list = spoolss.enumprinters("\\\\%s" % printserver)
except:
    print "error enumerating printers on %s" % printserver
    sys.exit(1)

# Display basic info

for printer in printer_list:
    h = spoolss.openprinter("\\\\%s\\%s" % (printserver, printer))
    info = h.getprinter(level = level)
    print "Printer info %d for %s: %s" % (level, printer, info)
    print
