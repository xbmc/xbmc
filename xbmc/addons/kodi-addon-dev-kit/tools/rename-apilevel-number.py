#!/usr/bin/python

#
#      Copyright (C) 2016 Team KODI
#      http://kodi.tv
#
#  This Program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2, or (at your option)
#  any later version.
#
#  This Program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with KODI; see the file COPYING.  If not, see
#  <http://www.gnu.org/licenses/>.
#
#

from optparse import OptionParser
import os, sys

# cannot be loaded as a module
if __name__ != "__main__":
    sys.stderr.write('This file cannot be loaded as a module!')
    sys.exit()

# parse command-line options
disc = """
This utility rename the string 'api*' with new defined level number on all
files with path defined header source.

Is to have on a API level change all doxygen related documentation comments
with right number.

Example:
./rename-apilevel-number.py --source=../include/kodi/api3 --old_level=2 --new_level=3
"""

parser = OptionParser(description=disc)
parser.add_option('--source', dest='source', metavar='DIR',
                  help='the source folder where changes becomes done [required]')
parser.add_option('--old_level', dest='old_level', metavar='NUMBER',
                  help='old level number which becomes changes [required]')
parser.add_option('--new_level', dest='new_level', metavar='NUMBER',
                  help='new level number which becomes used [required]')
(options, args) = parser.parse_args()

# the header option is required
if options.source is None or options.old_level is None or options.new_level is None:
    parser.print_help(sys.stdout)
    sys.exit()

def replace_word(infile,old_word,new_word):
    if not os.path.isfile(infile):
        print ("Error on replace_word, not a regular file: " + infile)
        sys.exit(1)

    f1=open(infile,'r').read()
    f2=open(infile,'w')
    m=f1.replace(old_word,new_word)
    f2.write(m)

print 'Renaming on all files in "' + options.source + '" the text from "api' + options.old_level + '" to "api' + options.new_level + '"'

from os.path import join, getsize
for root, dirs, files in os.walk(options.source):
    for file in files:
        print root + '/' + file
        replace_word(root + '/' + file, 'api' + options.old_level, 'api' + options.new_level)
