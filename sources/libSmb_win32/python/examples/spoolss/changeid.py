#!/usr/bin/python
#
# Display the changeid for a list of printers given on the command line
#
# Sample usage:
#
#     changeid.py '\\win2kdc1\magpie'
#

import sys
from samba import spoolss

if len(sys.argv) == 1:
    print "Usage: changeid.py <printername>"
    sys.exit(1)

for printer in sys.argv[1:]:

    # Open printer handle

    try:
        hnd = spoolss.openprinter(printer)
    except:
        print "error opening printer %s" % printer
        sys.exit(1)

    # Fetch and display changeid

    info = hnd.getprinter(level = 0)
    print info["change_id"]

    # Clean up

    spoolss.closeprinter(hnd)
