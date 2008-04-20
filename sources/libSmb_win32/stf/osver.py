#!/usr/bin/python
#
# Utilities for determining the Windows operating system version remotely.
#

from samba import srvsvc

# Constants

PLATFORM_UNKNOWN = 0
PLATFORM_WIN9X = 1
PLATFORM_NT4 = 2
PLATFORM_NT5 = 3                        # Windows 2000

def platform_name(platform_type):

    platform_names = { PLATFORM_UNKNOWN: "Unknown",
                       PLATFORM_WIN9X: "Windows 9x",
                       PLATFORM_NT4: "Windows NT",
                       PLATFORM_NT5: "Windows 2000" }

    if platform_names.has_key(platform_type):
        return platform_names[platform_type]

    return "Unknown"

def platform_type(info101):
    """Determine the operating system type from a SRV_INFO_101."""

    if info101['major_version'] == 4 and info101['minor_version'] == 0:
        return PLATFORM_NT4

    if info101['major_version'] == 5 and info101['minor_version'] == 0:
        return PLATFORM_NT5

    return PLATFORM_UNKNOWN

def is_domain_controller(info101):
    """Return true if the server_type field from a  SRV_INFO_101
    indicates a domain controller."""
    return info101['server_type'] & srvsvc.SV_TYPE_DOMAIN_CTRL

def os_version(name):
    info = srvsvc.netservergetinfo("\\\\%s" % name, 101)
    return platform_type(info)

if __name__ == "__main__":
    import sys
    if len(sys.argv) != 2:
        print "Usage: osver.py server"
        sys.exit(0)
    info = srvsvc.netservergetinfo("\\\\%s" % sys.argv[1], 101)
    print "platform type = %d" % platform_type(info)
    if is_domain_controller(info):
        print "%s is a domain controller" % sys.argv[1]
