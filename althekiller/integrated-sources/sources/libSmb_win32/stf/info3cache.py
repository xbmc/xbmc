#!/usr/bin/python
#
# Upon a winbindd authentication, test that an info3 record is cached in
# netsamlogon_cache.tdb and cache records are removed from winbindd_cache.tdb
#

import comfychair, stf
from samba import tdb, winbind

#
# We want to implement the following test on a win2k native mode domain.
#
# 1. trash netsamlogon_cache.tdb
# 2. wbinfo -r DOMAIN\Administrator                    [FAIL]
# 3. wbinfo --auth-crap DOMAIN\Administrator%password  [PASS]
# 4. wbinfo -r DOMAIN\Administrator                    [PASS]
#
# Also for step 3 we want to try 'wbinfo --auth-smbd' and
# 'wbinfo --auth-plaintext'
#

#
# TODO: To implement this test we need to be able to
#
#  - pass username%password combination for an invidivual winbindd request
#    (so we can get the administrator SID so we can clear the info3 cache)
#
#  - start/restart winbindd (to trash the winbind cache)
#
#  - from samba import dynconfig (to find location of info3 cache)
#
#  - be able to modify the winbindd cache (to set/reset individual winbind
#    cache entries)
#
#  - have --auth-crap present in HEAD
#

class WinbindAuthCrap(comfychair.TestCase):
    def runtest(self):
        raise comfychair.NotRunError, "not implemented"
        
class WinbindAuthSmbd(comfychair.TestCase):
    def runtest(self):
        # Grr - winbindd in HEAD doesn't contain the auth_smbd function
        raise comfychair.NotRunError, "no auth_smbd in HEAD"

class WinbindAuthPlaintext(comfychair.TestCase):
    def runtest(self):
        raise comfychair.NotRunError, "not implemented"

tests = [WinbindAuthCrap, WinbindAuthSmbd, WinbindAuthPlaintext]

if __name__ == "__main__":
    comfychair.main(tests)
