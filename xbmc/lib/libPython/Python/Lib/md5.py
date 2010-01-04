# $Id: md5.py 58064 2007-09-09 20:25:00Z gregory.p.smith $
#
#  Copyright (C) 2005   Gregory P. Smith (greg@krypto.org)
#  Licensed to PSF under a Contributor Agreement.

import warnings
warnings.warn("the md5 module is deprecated; use hashlib instead",
                DeprecationWarning, 2)

from hashlib import md5
new = md5

blocksize = 1        # legacy value (wrong in any useful sense)
digest_size = 16
