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

# XXX: All this code assumes that the Unix character set is UTF-8,
# which is the most common setting.  I guess it would be better to
# force it to that value while running the tests.  I'm not sure of the
# best way to do that yet.
# 
# Note that this is NOT the case in C code until the loadparm table is
# intialized -- the default seems to be ASCII, which rather lets Samba
# off the hook. :-) The best way seems to be to put this in the test
# harnesses:
#
#       lp_load("/dev/null", True, False, False);
#
# -- mbp

import sys, re, comfychair
from unicodenames import *

def signum(a):
    if a < 0:
        return -1
    elif a > 0:
        return +1
    else:
        return 0


class PushUCS2_Tests(comfychair.TestCase):
    """Conversion to/from UCS2"""
    def runtest(self):
        OE = LATIN_CAPITAL_LETTER_O_WITH_DIARESIS
        oe = LATIN_CAPITAL_LETTER_O_WITH_DIARESIS
        cases = ['hello',
                 'hello world',
                 'g' + OE + OE + 'gomobile', 
                 'g' + OE + oe + 'gomobile', 
                 u'foo\u0100',
                 KATAKANA_LETTER_A * 20,
                 ]
        for u8str in cases:
            out, err = self.runcmd("t_push_ucs2 \"%s\"" % u8str.encode('utf-8'))
            self.assert_equal(out, "0\n")
    

class StrCaseCmp(comfychair.TestCase):
    """String comparisons in simple ASCII""" 
    def run_strcmp(self, a, b, expect):
        out, err = self.runcmd('t_strcmp \"%s\" \"%s\"' % (a.encode('utf-8'), b.encode('utf-8')))
        if signum(int(out)) != expect:
            self.fail("comparison failed:\n"
                      "  a=%s\n"
                      "  b=%s\n"
                      "  expected=%s\n"
                      "  result=%s\n" % (`a`, `b`, `expect`, `out`))

    def runtest(self):
        # A, B, strcasecmp(A, B)
        cases = [('hello', 'hello', 0),
                 ('hello', 'goodbye', +1),
                 ('goodbye', 'hello', -1),
                 ('hell', 'hello', -1),
                 ('', '', 0),
                 ('a', '', +1),
                 ('', 'a', -1),
                 ('a', 'A', 0),
                 ('aa', 'aA', 0),
                 ('Aa', 'aa', 0),
                 ('longstring ' * 100, 'longstring ' * 100, 0),
                 ('longstring ' * 100, 'longstring ' * 100 + 'a', -1),
                 ('longstring ' * 100 + 'a', 'longstring ' * 100, +1),
                 (KATAKANA_LETTER_A, KATAKANA_LETTER_A, 0),
                 (KATAKANA_LETTER_A, 'a', 1),
                 ]
        for a, b, expect in cases:
            self.run_strcmp(a, b, expect)
        
class strstr_m(comfychair.TestCase):
    """String comparisons in simple ASCII""" 
    def run_strstr(self, a, b, expect):
        out, err = self.runcmd('t_strstr \"%s\" \"%s\"' % (a.encode('utf-8'), b.encode('utf-8')))
        if (out != (expect + '\n').encode('utf-8')):
            self.fail("comparison failed:\n"
                      "  a=%s\n"
                      "  b=%s\n"
                      "  expected=%s\n"
                      "  result=%s\n" % (`a`, `b`, `expect+'\n'`, `out`))

    def runtest(self):
        # A, B, strstr_m(A, B)
        cases = [('hello', 'hello', 'hello'),
                 ('hello', 'goodbye', '(null)'),
                 ('goodbye', 'hello', '(null)'),
                 ('hell', 'hello', '(null)'),
                 ('hello', 'hell', 'hello'),
                 ('', '', ''),
                 ('a', '', 'a'),
                 ('', 'a', '(null)'),
                 ('a', 'A', '(null)'),
                 ('aa', 'aA', '(null)'),
                 ('Aa', 'aa', '(null)'),
                 ('%v foo', '%v', '%v foo'),
                 ('foo %v foo', '%v', '%v foo'),
                 ('foo %v', '%v', '%v'),
                 ('longstring ' * 100, 'longstring ' * 99, 'longstring ' * 100),
                 ('longstring ' * 99, 'longstring ' * 100, '(null)'),
                 ('longstring a' * 99, 'longstring ' * 100 + 'a', '(null)'),
                 ('longstring ' * 100 + 'a', 'longstring ' * 100, 'longstring ' * 100 + 'a'),
                 (KATAKANA_LETTER_A, KATAKANA_LETTER_A + 'bcd', '(null)'),
                 (KATAKANA_LETTER_A + 'bcde', KATAKANA_LETTER_A + 'bcd', KATAKANA_LETTER_A + 'bcde'),
                 ('d'+KATAKANA_LETTER_A + 'bcd', KATAKANA_LETTER_A + 'bcd', KATAKANA_LETTER_A + 'bcd'),
                 ('d'+KATAKANA_LETTER_A + 'bd', KATAKANA_LETTER_A + 'bcd', '(null)'),
                 
                 ('e'+KATAKANA_LETTER_A + 'bcdf', KATAKANA_LETTER_A + 'bcd', KATAKANA_LETTER_A + 'bcdf'),
                 (KATAKANA_LETTER_A, KATAKANA_LETTER_A + 'bcd', '(null)'),
                 (KATAKANA_LETTER_A*3, 'a', '(null)'),
                 ]
        for a, b, expect in cases:
            self.run_strstr(a, b, expect)
        
# Define the tests exported by this module
tests = [StrCaseCmp,
         strstr_m,
         PushUCS2_Tests]

# Handle execution of this file as a main program
if __name__ == '__main__':
    comfychair.main(tests)

# Local variables:
# coding: utf-8
# End:
