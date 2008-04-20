#!/usr/bin/env python

#
# A python module that maps printerdata to a dictionary.  We define
# two classes.  The printerdata class maps to Get/Set/Enum/DeletePrinterData
# and the printerdata_ex class maps to Get/Set/Enum/DeletePrinterDataEx
#

#
# TODO:
#
#   - Implement __delitem__
#

from samba import spoolss

class printerdata:
    def __init__(self, host, creds = {}, access = 0x02000000):
	# For read access, use MAXIMUM_ALLOWED_ACCESS = 0x02000000
	# For write access, use PRINTER_ACCESS_ADMINISTER = 0x00000004
        self.hnd = spoolss.openprinter(host, creds = creds, access = access)

    def keys(self):
        return self.hnd.enumprinterdata().keys()

    def __getitem__(self, key):
        return self.hnd.getprinterdata(key)['data']

    def __setitem__(self, key, value):
        # Store as REG_BINARY for now
        self.hnd.setprinterdata({"key": "", "value": key, "type": 3,
                                 "data": value})
        
class printerdata_ex:
    def __init__(self, host, creds = {}, access = 0x02000000):
	# For read access, use MAXIMUM_ALLOWED_ACCESS = 0x02000000
	# For write access, use PRINTER_ACCESS_ADMINISTER = 0x00000004
        self.host = host
        self.top_level_keys = ["PrinterDriverData", "DsSpooler", "DsDriver",
                               "DsUser"]
	self.creds = creds
	self.access = access

    def keys(self):
        return self.top_level_keys

    def has_key(self, key):
        for k in self.top_level_keys:
            if k == key:
                return 1
        return 0

    class printerdata_ex_subkey:
        def __init__(self, host, key, creds, access):
            self.hnd = spoolss.openprinter(host, creds, access)
            self.key = key

        def keys(self):
            return self.hnd.enumprinterdataex(self.key).keys()

        def __getitem__(self, key):
            return self.hnd.getprinterdataex(self.key, key)['data']

    def __getitem__(self, key):
        return self.printerdata_ex_subkey(
            self.host, key, self.creds, self.access)
