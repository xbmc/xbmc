#! /usr/bin/python

# Comfychair test cases for Samba python extensions

# Copyright (C) 2003 by Tim Potter <tpot@samba.org>
# 
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of the
# License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
# USA

"""These tests are run by Samba's "make check"."""

import sys, comfychair

class ImportTest(comfychair.TestCase):
    """Check that all modules can be imported without error."""
    def runtest(self):
        python_modules = ['spoolss', 'lsa', 'samr', 'winbind', 'winreg',
                          'srvsvc', 'tdb', 'smb', 'tdbpack']
        for m in python_modules:
            try:
                __import__('samba.%s' % m)
            except ImportError, msg:
                self.log(str(msg))
                self.fail('error importing %s module' % m)

tests = [ImportTest]

if __name__ == '__main__':
    # Some magic to repend build directory to python path so we see the
    # objects we have built and not previously installed stuff.
    from distutils.util import get_platform
    from os import getcwd
    sys.path.insert(0, '%s/build/lib.%s-%s' %
                    (getcwd(), get_platform(), sys.version[0:3]))

    comfychair.main(tests)
