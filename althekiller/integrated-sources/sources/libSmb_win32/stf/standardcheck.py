#! /usr/bin/python

# Comfychair test cases for Samba

# Copyright (C) 2003 by Martin Pool <mbp@samba.org>
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

import strings, comfychair
import smbcontrol, sambalib

# There should not be any actual tests in here: this file just serves
# to define the ones run by default.  They're imported from other
# modules.

tests = strings.tests + smbcontrol.tests + sambalib.tests

if __name__ == '__main__':
    comfychair.main(tests)
