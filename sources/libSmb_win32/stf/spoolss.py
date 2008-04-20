#!/usr/bin/python

import re
import comfychair, stf
from samba import spoolss

class PrintServerTest(comfychair.TestCase):
    """An abstract class requiring a print server."""
    def setUp(self):
        # TODO: create a test printer
        self.server = stf.get_server(platform = "nt")
        self.require(self.server != None, "print server required")
        # TODO: remove hardcoded printer name
        self.printername = "p"
        self.uncname = "\\\\%s\\%s" % \
                           (self.server["hostname"], self.printername)

class W2kPrintServerTest(comfychair.TestCase):
    """An abstract class requiring a print server."""
    def setUp(self):
        # TODO: create a test printer
        self.server = stf.get_server(platform = "nt5")
        self.require(self.server != None, "print server required")
        # TODO: remove hardcoded printer name
        self.printername = "p"
        self.uncname = "\\\\%s\\%s" % \
                           (self.server["hostname"], self.printername)

class CredentialTest(PrintServerTest):
    """An class that calls a function with various sets of credentials."""
    def runTest(self):

        bad_user_creds = {"username": "spotty",
                          "domain": "dog",
                          "password": "bone"}
                        
        cases = ((self.server["administrator"], "Admin credentials", 1),
                 (bad_user_creds,               "Bad credentials",   0))

        # TODO: add unpriv user case

        for creds, testname, result in cases:
            try:
                self.runTestArg(creds)
            except:
                if result:
                    import traceback
                    traceback.print_exc()
                    self.fail("rpc with creds %s failed when it "
                              "should have suceeded" % creds)
                return

            if not result:
                self.fail("rpc with creds %s suceeded when it should "
                          "have failed" % creds)
        
class ArgTestServer(PrintServerTest):
    """Test a RPC that takes a UNC print server name."""
    def runTest(self):

        # List of test cases, %s substituted for server name

        cases = (("",             "No server name",          0),
                 ("\\\\%s",       "Valid server name",       1),
                 ("\\%s",         "Invalid unc server name", 0),
                 ("\\\\%s__",     "Invalid unc server name", 0))

        for unc, testname, result in cases:
            unc = re.sub("%s", self.server["hostname"], unc)
            try:
                self.runTestArg(unc)
            except:
                if result:
                    self.fail("rpc(\"%s\") failed when it should have "
                              "suceeded" % unc)
                return

            if not result:
                # Suceeded when we should have failed
                self.fail("rpc(\"%s\") suceeded when it should have "
                          "failed" % unc)

class ArgTestServerAndPrinter(ArgTestServer):
    """Test a RPC that takes a UNC print server or UNC printer name."""
    def runTest(self):

        ArgTestServer.runTest(self)
        
        # List of test cases, %s substituted for server name, %p substituted
        # for printer name.

        cases = (("\\\\%s\\%p",   "Valid server and printer name",      1),
                 ("\\\\%s\\%p__", "Valid server, invalid printer name", 0),
                 ("\\\\%s__\\%p", "Invalid server, valid printer name", 0))

        for unc, testname, result in cases:
            unc = re.sub("%s", self.server["hostname"], unc)
            unc = re.sub("%p", self.printername, unc)
            try:
                self.runTestArg(unc)
            except:
                if result:
                    self.fail("openprinter(\"%s\") failed when it should have "
                              "suceeded" % unc)
                return

            if not result:
                # Suceeded when we should have failed
                self.fail("openprinter(\"%s\") suceeded when it should have "
                          "failed" % unc)

class OpenPrinterArg(ArgTestServerAndPrinter):
    """Test the OpenPrinter RPC with combinations of valid and invalid
    server and printer names."""
    def runTestArg(self, unc):
        spoolss.openprinter(unc)

class OpenPrinterCred(CredentialTest):
    """Test opening printer with good and bad credentials."""
    def runTestArg(self, creds):
        spoolss.openprinter(self.uncname, creds = creds)

class ClosePrinter(PrintServerTest):
    """Test the ClosePrinter RPC on a printer handle."""
    def runTest(self):
        hnd = spoolss.openprinter(self.uncname)
        spoolss.closeprinter(hnd)
        
class ClosePrinterServer(PrintServerTest):
    """Test the ClosePrinter RPC on a print server handle."""
    def runTest(self):
        hnd = spoolss.openprinter("\\\\%s" % self.server["hostname"])
        spoolss.closeprinter(hnd)
        
class GetPrinterInfo(PrintServerTest):
    """Retrieve printer info at various levels."""

    # Sample printer data

    sample_info = {
        0: {'printer_errors': 0, 'unknown18': 0, 'unknown13': 0, 'unknown26': 0, 'cjobs': 0, 'unknown11': 0, 'server_name': '\\\\win2kdc1', 'total_pages': 0, 'unknown15': 586, 'unknown16': 0, 'month': 2, 'unknown20': 0, 'second': 23, 'unknown22': 983040, 'unknown25': 0, 'total_bytes': 0, 'unknown27': 0, 'year': 2003, 'build_version': 2195, 'unknown28': 0, 'global_counter': 4, 'day': 13, 'minute': 53, 'total_jobs': 0, 'unknown29': 1114112, 'name': '\\\\win2kdc1\\p', 'hour': 2, 'level': 0, 'c_setprinter': 0, 'change_id': 522454169, 'major_version': 5, 'unknown23': 15, 'day_of_week': 4, 'unknown14': 1, 'session_counter': 2, 'status': 1, 'unknown7': 1, 'unknown8': 0, 'unknown9': 0, 'milliseconds': 421, 'unknown24': 0},
        1: {'comment': "I'm a teapot!", 'level': 1, 'flags': 8388608, 'name': '\\\\win2kdc1\\p', 'description': '\\\\win2kdc1\\p,HP LaserJet 4,Canberra office'},
        2: {'comment': "I'm a teapot!", 'status': 1, 'print_processor': 'WinPrint', 'until_time': 0, 'share_name': 'p', 'start_time': 0, 'device_mode': {'icm_method': 1, 'bits_per_pel': 0, 'log_pixels': 0, 'orientation': 1, 'panning_width': 0, 'color': 2, 'pels_width': 0, 'print_quality': 600, 'driver_version': 24, 'display_flags': 0, 'y_resolution': 600, 'media_type': 0, 'display_frequency': 0, 'icm_intent': 0, 'pels_height': 0, 'reserved1': 0, 'size': 220, 'scale': 100, 'dither_type': 0, 'panning_height': 0, 'default_source': 7, 'duplex': 1, 'fields': 16131, 'spec_version': 1025, 'copies': 1, 'device_name': '\\\\win2kdc1\\p', 'paper_size': 1, 'paper_length': 0, 'private': 'private', 'collate': 0, 'paper_width': 0, 'form_name': 'Letter', 'reserved2': 0, 'tt_option': 0}, 'port_name': 'LPT1:', 'sepfile': '', 'parameters': '', 'security_descriptor': {'group_sid': 'S-1-5-21-1606980848-1677128483-854245398-513', 'sacl': None, 'dacl': {'ace_list': [{'flags': 0, 'type': 0, 'mask': 983052, 'trustee': 'S-1-5-32-544'}, {'flags': 9, 'type': 0, 'mask': 983056, 'trustee': 'S-1-5-32-544'}, {'flags': 0, 'type': 0, 'mask': 131080, 'trustee': 'S-1-5-21-1606980848-1677128483-854245398-1121'}, {'flags': 10, 'type': 0, 'mask': 131072, 'trustee': 'S-1-3-0'}, {'flags': 9, 'type': 0, 'mask': 983056, 'trustee': 'S-1-3-0'}, {'flags': 0, 'type': 0, 'mask': 131080, 'trustee': 'S-1-5-21-1606980848-1677128483-854245398-1124'}, {'flags': 0, 'type': 0, 'mask': 131080, 'trustee': 'S-1-1-0'}, {'flags': 0, 'type': 0, 'mask': 983052, 'trustee': 'S-1-5-32-550'}, {'flags': 9, 'type': 0, 'mask': 983056, 'trustee': 'S-1-5-32-550'}, {'flags': 0, 'type': 0, 'mask': 983052, 'trustee': 'S-1-5-32-549'}, {'flags': 9, 'type': 0, 'mask': 983056, 'trustee': 'S-1-5-32-549'}, {'flags': 0, 'type': 0, 'mask': 983052, 'trustee': 'S-1-5-21-1606980848-1677128483-854245398-1106'}], 'revision': 2}, 'owner_sid': 'S-1-5-32-544', 'revision': 1}, 'name': '\\\\win2kdc1\\p', 'server_name': '\\\\win2kdc1', 'level': 2, 'datatype': 'RAW', 'cjobs': 0, 'average_ppm': 0, 'priority': 1, 'driver_name': 'HP LaserJet 4', 'location': 'Canberra office', 'attributes': 8776, 'default_priority': 0},
        3: {'flags': 4, 'security_descriptor': {'group_sid': 'S-1-5-21-1606980848-1677128483-854245398-513', 'sacl': None, 'dacl': {'ace_list': [{'flags': 0, 'type': 0, 'mask': 983052, 'trustee': 'S-1-5-32-544'}, {'flags': 9, 'type': 0, 'mask': 983056, 'trustee': 'S-1-5-32-544'}, {'flags': 0, 'type': 0, 'mask': 131080, 'trustee': 'S-1-5-21-1606980848-1677128483-854245398-1121'}, {'flags': 10, 'type': 0, 'mask': 131072, 'trustee': 'S-1-3-0'}, {'flags': 9, 'type': 0, 'mask': 983056, 'trustee': 'S-1-3-0'}, {'flags': 0, 'type': 0, 'mask': 131080, 'trustee': 'S-1-5-21-1606980848-1677128483-854245398-1124'}, {'flags': 0, 'type': 0, 'mask': 131080, 'trustee': 'S-1-1-0'}, {'flags': 0, 'type': 0, 'mask': 983052, 'trustee': 'S-1-5-32-550'}, {'flags': 9, 'type': 0, 'mask': 983056, 'trustee': 'S-1-5-32-550'}, {'flags': 0, 'type': 0, 'mask': 983052, 'trustee': 'S-1-5-32-549'}, {'flags': 9, 'type': 0, 'mask': 983056, 'trustee': 'S-1-5-32-549'}, {'flags': 0, 'type': 0, 'mask': 983052, 'trustee': 'S-1-5-21-1606980848-1677128483-854245398-1106'}], 'revision': 2}, 'owner_sid': 'S-1-5-32-544', 'revision': 1}, 'level': 3}
        }

    def runTest(self):
        self.hnd = spoolss.openprinter(self.uncname)

        # Everyone should have getprinter levels 0-3

        for i in (0, 1, 2, 3):
            info = self.hnd.getprinter(level = i)
            try:
                stf.dict_check(self.sample_info[i], info)
            except ValueError, msg:
                raise "info%d: %s" % (i, msg)

class EnumPrinters(PrintServerTest):
    """Enumerate print info at various levels."""

    sample_info = {

        0: {'q': {'printer_errors': 0, 'unknown18': 0, 'unknown13': 0, 'unknown26': 0, 'cjobs': 0, 'unknown11': 0, 'server_name': '', 'total_pages': 0, 'unknown15': 586, 'unknown16': 0, 'month': 2, 'unknown20': 0, 'second': 23, 'unknown22': 983040, 'unknown25': 0, 'total_bytes': 0, 'unknown27': 0, 'year': 2003, 'build_version': 2195, 'unknown28': 0, 'global_counter': 4, 'day': 13, 'minute': 53, 'total_jobs': 0, 'unknown29': -1833435136, 'name': 'q', 'hour': 2, 'level': 0, 'c_setprinter': 0, 'change_id': 522454169, 'major_version': 5, 'unknown23': 15, 'day_of_week': 4, 'unknown14': 1, 'session_counter': 1, 'status': 0, 'unknown7': 1, 'unknown8': 0, 'unknown9': 0, 'milliseconds': 421, 'unknown24': 0}, 'p': {'printer_errors': 0, 'unknown18': 0, 'unknown13': 0, 'unknown26': 0, 'cjobs': 0, 'unknown11': 0, 'server_name': '', 'total_pages': 0, 'unknown15': 586, 'unknown16': 0, 'month': 2, 'unknown20': 0, 'second': 23, 'unknown22': 983040, 'unknown25': 0, 'total_bytes': 0, 'unknown27': 0, 'year': 2003, 'build_version': 2195, 'unknown28': 0, 'global_counter': 4, 'day': 13, 'minute': 53, 'total_jobs': 0, 'unknown29': -1831337984, 'name': 'p', 'hour': 2, 'level': 0, 'c_setprinter': 0, 'change_id': 522454169, 'major_version': 5, 'unknown23': 15, 'day_of_week': 4, 'unknown14': 1, 'session_counter': 1, 'status': 1, 'unknown7': 1, 'unknown8': 0, 'unknown9': 0, 'milliseconds': 421, 'unknown24': 0}, 'magpie': {'printer_errors': 0, 'unknown18': 0, 'unknown13': 0, 'unknown26': 0, 'cjobs': 0, 'unknown11': 0, 'server_name': '', 'total_pages': 0, 'unknown15': 586, 'unknown16': 0, 'month': 2, 'unknown20': 0, 'second': 23, 'unknown22': 983040, 'unknown25': 0, 'total_bytes': 0, 'unknown27': 0, 'year': 2003, 'build_version': 2195, 'unknown28': 0, 'global_counter': 4, 'day': 13, 'minute': 53, 'total_jobs': 0, 'unknown29': 1114112, 'name': 'magpie', 'hour': 2, 'level': 0, 'c_setprinter': 0, 'change_id': 522454169, 'major_version': 5, 'unknown23': 15, 'day_of_week': 4, 'unknown14': 1, 'session_counter': 1, 'status': 0, 'unknown7': 1, 'unknown8': 0, 'unknown9': 0, 'milliseconds': 421, 'unknown24': 0}},

        1: {'q': {'comment': 'cheepy birds', 'level': 1, 'flags': 8388608, 'name': 'q', 'description': 'q,HP LaserJet 4,'}, 'p': {'comment': "I'm a teapot!", 'level': 1, 'flags': 8388608, 'name': 'p', 'description': 'p,HP LaserJet 4,Canberra office'}, 'magpie': {'comment': '', 'level': 1, 'flags': 8388608, 'name': 'magpie', 'description': 'magpie,Generic / Text Only,'}}
        }

    def runTest(self):
        for i in (0, 1):
            info = spoolss.enumprinters(
                "\\\\%s" % self.server["hostname"], level = i)
            try:
                stf.dict_check(self.sample_info[i], info)
            except ValueError, msg:
                raise "info%d: %s" % (i, msg)

class EnumPrintersArg(ArgTestServer):
    def runTestArg(self, unc):
        spoolss.enumprinters(unc)

class EnumPrintersCred(CredentialTest):
    """Test opening printer with good and bad credentials."""
    def runTestArg(self, creds):
        spoolss.enumprinters(
            "\\\\%s" % self.server["hostname"], creds = creds)

class EnumPrinterdrivers(PrintServerTest):

    sample_info = {
        1: {'Okipage 10ex (PCL5E) : STANDARD': {'name': 'Okipage 10ex (PCL5E) : STANDARD', 'level': 1}, 'Generic / Text Only': {'name': 'Generic / Text Only', 'level': 1}, 'Brother HL-1030 series': {'name': 'Brother HL-1030 series', 'level': 1}, 'Brother HL-1240 series': {'name': 'Brother HL-1240 series', 'level': 1}, 'HP DeskJet 1220C Printer': {'name': 'HP DeskJet 1220C Printer', 'level': 1}, 'HP LaserJet 4100 PCL 6': {'name': 'HP LaserJet 4100 PCL 6', 'level': 1}, 'HP LaserJet 4': {'name': 'HP LaserJet 4', 'level': 1}},
        2: {'Okipage 10ex (PCL5E) : STANDARD': {'version': 2, 'config_file': '\\\\WIN2KDC1\\print$\\W32X86\\2\\RASDDUI.DLL', 'name': 'Okipage 10ex (PCL5E) : STANDARD', 'driver_path': '\\\\WIN2KDC1\\print$\\W32X86\\2\\RASDD.DLL', 'data_file': '\\\\WIN2KDC1\\print$\\W32X86\\2\\OKIPAGE.DLL', 'level': 2, 'architecture': 'Windows NT x86'}, 'Generic / Text Only': {'version': 3, 'config_file': '\\\\WIN2KDC1\\print$\\W32X86\\3\\UNIDRVUI.DLL', 'name': 'Generic / Text Only', 'driver_path': '\\\\WIN2KDC1\\print$\\W32X86\\3\\UNIDRV.DLL', 'data_file': '\\\\WIN2KDC1\\print$\\W32X86\\3\\TTY.GPD', 'level': 2, 'architecture': 'Windows NT x86'}, 'Brother HL-1030 series': {'version': 3, 'config_file': '\\\\WIN2KDC1\\print$\\W32X86\\3\\BRUHL99A.DLL', 'name': 'Brother HL-1030 series', 'driver_path': '\\\\WIN2KDC1\\print$\\W32X86\\3\\BROHL99A.DLL', 'data_file': '\\\\WIN2KDC1\\print$\\W32X86\\3\\BROHL103.PPD', 'level': 2, 'architecture': 'Windows NT x86'}, 'Brother HL-1240 series': {'version': 3, 'config_file': '\\\\WIN2KDC1\\print$\\W32X86\\3\\BRUHL99A.DLL', 'name': 'Brother HL-1240 series', 'driver_path': '\\\\WIN2KDC1\\print$\\W32X86\\3\\BROHL99A.DLL', 'data_file': '\\\\WIN2KDC1\\print$\\W32X86\\3\\BROHL124.PPD', 'level': 2, 'architecture': 'Windows NT x86'}, 'HP DeskJet 1220C Printer': {'version': 3, 'config_file': '\\\\WIN2KDC1\\print$\\W32X86\\3\\HPW8KMD.DLL', 'name': 'HP DeskJet 1220C Printer', 'driver_path': '\\\\WIN2KDC1\\print$\\W32X86\\3\\HPW8KMD.DLL', 'data_file': '\\\\WIN2KDC1\\print$\\W32X86\\3\\HPW8KMD.DLL', 'level': 2, 'architecture': 'Windows NT x86'}, 'HP LaserJet 4100 PCL 6': {'version': 3, 'config_file': '\\\\WIN2KDC1\\print$\\W32X86\\3\\HPBF042E.DLL', 'name': 'HP LaserJet 4100 PCL 6', 'driver_path': '\\\\WIN2KDC1\\print$\\W32X86\\3\\HPBF042G.DLL', 'data_file': '\\\\WIN2KDC1\\print$\\W32X86\\3\\HPBF042I.PMD', 'level': 2, 'architecture': 'Windows NT x86'}, 'HP LaserJet 4': {'version': 2, 'config_file': '\\\\WIN2KDC1\\print$\\W32X86\\2\\hpblff0.dll', 'name': 'HP LaserJet 4', 'driver_path': '\\\\WIN2KDC1\\print$\\W32X86\\2\\hpblff2.dll', 'data_file': '\\\\WIN2KDC1\\print$\\W32X86\\2\\hpblff39.pmd', 'level': 2, 'architecture': 'Windows NT x86'}}
        }

    def runTest(self):
        for i in (1, 2):
            info = spoolss.enumprinterdrivers(
                "\\\\%s" % self.server["hostname"], level = i)
            try:
                if not self.sample_info.has_key(i):
                    self.log("%s" % info)
                    self.fail()
                stf.dict_check(self.sample_info[i], info)
            except ValueError, msg:
                raise "info%d: %s" % (i, msg)

class EnumPrinterdriversArg(ArgTestServer):
    def runTestArg(self, unc):
        spoolss.enumprinterdrivers(unc)

class EnumPrinterdriversCred(CredentialTest):
    """Test opening printer with good and bad credentials."""
    def runTestArg(self, creds):
        spoolss.enumprinterdrivers(
            "\\\\%s" % self.server["hostname"], creds = creds)

def usage():
    print "Usage: spoolss.py [options] [test1[,test2...]]"
    print "\t -v/--verbose     Display debugging information"
    print "\t -l/--list-tests  List available tests"
    print
    print "A list of comma separated test names or regular expressions"
    print "can be used to filter the tests performed."
            
def test_match(subtest_list, test_name):
    """Return true if a test matches a comma separated list of regular
    expression of test names."""
    # re.match does an implicit ^ at the start of the pattern.
    # Explicitly anchor to end to avoid matching substrings.
    for s in string.split(subtest_list, ","):
        if re.match(s + "$", test_name):
            return 1
    return 0
           
if __name__ == "__main__":
    import os, sys, string
    import getopt
    
    try:
        opts, args = getopt.getopt(sys.argv[1:], "vl", \
                                   ["verbose", "list-tests"])
    except getopt.GetoptError:
        usage()
        sys.exit(0)

    verbose = 0
    list_tests = 0

    for opt, arg in opts:
        if opt in ("-v", "--verbose"):
            verbose = 1
        if opt in ("-l", "--list-tests"):
            list_tests = 1

    if len(args) > 1:
        usage()
        sys.exit(0)

    test_list = [
        OpenPrinterArg,
        OpenPrinterCred,
        ClosePrinter,
        ClosePrinterServer,
        GetPrinterInfo,
        EnumPrinters,
        EnumPrintersCred,
        EnumPrintersArg,
        EnumPrinterdrivers,
        EnumPrinterdriversCred,
        EnumPrinterdriversArg,
        ]

    if len(args):
        t = []
        for test in test_list:
            if test_match(args[0], test.__name__):
                t.append(test)
        test_list = t

    if os.environ.has_key("SAMBA_DEBUG"):
        spoolss.setup_logging(interactive = 1)
        spoolss.set_debuglevel(10)

    if list_tests:
        for test in test_list:
            print test.__name__
    else:
        comfychair.runtests(test_list, verbose = verbose)
