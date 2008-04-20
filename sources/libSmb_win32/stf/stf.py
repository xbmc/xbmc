#!/usr/bin/python
#
# Samba Testing Framework for Unit-testing
#

import os, string, re
import osver

def get_server_list_from_string(s):

    server_list = []
    
    # Format is a list of server:domain\username%password separated
    # by commas.

    for entry in string.split(s, ","):

        # Parse entry 

        m = re.match("(.*):(.*)(\\\\|/)(.*)%(.*)", entry)
        if not m:
            raise "badly formed server list entry '%s'" % entry

        server = m.group(1)
        domain = m.group(2)
        username = m.group(4)
        password = m.group(5)

        # Categorise servers

        server_list.append({"platform": osver.os_version(server),
                            "hostname": server,
                            "administrator": {"username": username,
                                              "domain": domain,
                                              "password" : password}})

    return server_list

def get_server_list():
    """Iterate through all sources of server info and append them all
    in one big list."""
    
    server_list = []

    # The $STF_SERVERS environment variable

    if os.environ.has_key("STF_SERVERS"):
        server_list = server_list + \
                      get_server_list_from_string(os.environ["STF_SERVERS"])

    return server_list

def get_server(platform = None):
    """Return configuration information for a server.  The platform
    argument can be a string either 'nt4' or 'nt5' for Windows NT or
    Windows 2000 servers, or just 'nt' for Windows NT and higher."""
    
    server_list = get_server_list()

    for server in server_list:
        if platform:
            p = server["platform"]
            if platform == "nt":
                if (p == osver.PLATFORM_NT4 or p == osver.PLATFORM_NT5):
                    return server
            if platform == "nt4" and p == osver.PLATFORM_NT4:
                return server
            if platform == "nt5" and p == osver.PLATFORM_NT5:
                return server
        else:
            # No filter defined, return first in list
            return server
        
    return None

def dict_check(sample_dict, real_dict):
    """Check that real_dict contains all the keys present in sample_dict
    and no extras.  Also check that common keys are of them same type."""
    tmp = real_dict.copy()
    for key in sample_dict.keys():
        # Check existing key and type
        if not real_dict.has_key(key):
            raise ValueError, "dict does not contain key '%s'" % key
        if type(sample_dict[key]) != type(real_dict[key]):
            raise ValueError, "dict has differing types (%s vs %s) for key " \
                  "'%s'" % (type(sample_dict[key]), type(real_dict[key]), key)
        # Check dictionaries recursively
        if type(sample_dict[key]) == dict:
            dict_check(sample_dict[key], real_dict[key])
        # Delete visited keys from copy
        del(tmp[key])
    # Any keys leftover are present in the real dict but not the sample
    if len(tmp) == 0:
        return
    result = "dict has extra keys: "
    for key in tmp.keys():
        result = result + key + " "
    raise ValueError, result

if __name__ == "__main__":
    print get_server(platform = "nt")
