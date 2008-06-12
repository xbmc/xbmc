#! /usr/bin/python     

# Comfychair test cases for Samba string functions.

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

"""Tests for Samba library functions."""

import sys, re, comfychair
from unicodenames import *

class snprintf_Test(comfychair.TestCase):
    def runtest(self):
        # Everything is built in to the test
        out, err = self.runcmd('t_snprintf')
        
# Define the tests exported by this module
tests = [snprintf_Test]

# Handle execution of this file as a main program
if __name__ == '__main__':
    comfychair.main(tests)

# Local variables:
# coding: utf-8
# End:
