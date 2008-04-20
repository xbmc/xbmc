#!/usr/bin/env python
#
# Get or set the security descriptor on a printer
#

import sys, re, string
from samba import spoolss

if len(sys.argv) != 3:
    print "Usage: psec.py getsec|setsec printername"
    sys.exit(1)

op = sys.argv[1]
printername = sys.argv[2]

# Display security descriptor

if op == "getsec":

    try:
        hnd = spoolss.openprinter(printername)
    except:
        print "error opening printer %s" % printername
        sys.exit(1)

    secdesc = hnd.getprinter(level = 3)["security_descriptor"]

    print secdesc["owner_sid"]
    print secdesc["group_sid"]

    for acl in secdesc["dacl"]["ace_list"]:
        print "%d %d 0x%08x %s" % (acl["type"], acl["flags"],
                                   acl["mask"], acl["trustee"])

    spoolss.closeprinter(hnd)

    sys.exit(0)

# Set security descriptor

if op == "setsec":

    # Open printer

    try:
        hnd = spoolss.openprinter(printername,
                                  creds = {"domain": "NPSD-TEST2",
                                           "username": "Administrator",
                                           "password": "penguin"})
    except:
        print "error opening printer %s" % printername
        sys.exit(1)

    # Read lines from standard input and build security descriptor

    lines = sys.stdin.readlines()

    secdesc = {}

    secdesc["owner_sid"] = lines[0]
    secdesc["group_sid"] = lines[1]

    secdesc["revision"] = 1
    secdesc["dacl"] = {}
    secdesc["dacl"]["revision"] = 2
    secdesc["dacl"]["ace_list"] = []

    for acl in lines[2:]:
        match = re.match("(\d+) (\d+) (0[xX][\dA-Fa-f]+) (\S+)", acl)
        secdesc["dacl"]["ace_list"].append(
            {"type": int(match.group(1)), "flags": int(match.group(2)),
             "mask": string.atoi(match.group(3), 0), "trustee": match.group(4)})

    # Build info3 structure

    info3 = {}

    info3["flags"] = 0x8004             # self-relative, dacl present
    info3["level"] = 3
    info3["security_descriptor"] = secdesc

    hnd.setprinter(info3)

    spoolss.closeprinter(hnd)
    sys.exit(0)

print "invalid operation %s" % op
sys.exit(1)
