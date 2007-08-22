#!/usr/bin/env python

import re, os, sys

def usage ():
  print " Changelog.py [option]"
  print "  -h       : display this message"
  print "  -r <REV> : get log messages up until REV"
  print "  -d <DIR> : place Changelog.txt in DIR"
  sys.exit()


header =  "*************************************************************************************************************\n" \
          "*************************************************************************************************************\n" \
          "                                     Xbox Media Center CHANGELOG\n" \
          "*************************************************************************************************************\n" \
          "*************************************************************************************************************" \
          "\nDate        Rev   Message\n" \
          "=============================================================================================================\n"

xmlre = re.compile("^<logentry.*?revision=\"([0-9]{4,})\".*?<date>([0-9]{4}-[0-9]{2}-[0-9]{2})T.*?<msg>(.*?)</msg>.*?</logentry>$", re.MULTILINE | re.DOTALL)
txtre = re.compile("([0-9]{4}-[0-9]{2}-[0-9]{2})  ([0-9]{4,5}) {1,2}(.*)")

old = None
lastrev = 8638
rev = 1000000
dir = "."

nargs = len(sys.argv)
args = sys.argv

i = 1
while i < nargs:
  if args[i] == "-r":
    i += 1
    try:
      rev = int(sys.argv[i])
    except:
      rev=1000000
  elif args[i] == "-d":
    i+=1
    dir = args[i].replace(' ', '\ ')
  elif args[i] == "-h" or args[i] == "--help":
    usage()
  i+=1

# print dir
# print rev

try:
  old = openi("%s/Changelog.txt" % (dir))
except:
  old = None

if old != None:
  olddoc = old.read()
  old.close()
  oldmsgs = txtre.findall(olddoc)
  del olddoc
  if len(oldmsgs) > 0:
    lastrev = int(oldmsgs[0][1])
try:
  output = open("%s/Changelog.txt" % (dir),"w")
except:
  print "Can't open %s/Changelog.txt for writing." % (dir)
  sys.exit()

output.write(header)

if rev <= lastrev:
  for msg in oldmsgs:
    if int(msg[1]) <= rev:
      s = "%-11.11s %-5.5s %s\n" % (msg[0], msg[1], msg[2])
      output.write(s)
  sys.exit()

svncmd = "svn log --xml -r %s:HEAD" % (lastrev)
newlog = os.popen(svncmd)

newlogdoc = newlog.read()
newlog.close()

newmsgs = xmlre.findall(newlogdoc)

newmsgs.reverse()

for msg in newmsgs:
  s = "%-11.11s %-5.5s %s\n" % (msg[1], msg[0], msg[2].replace('\n',''))
  output.write(s)

skip = 0
if old != None:
  for msg in oldmsgs:
    if skip == 1:
      s = "%-11.11s %-5.5s %s\n" % (msg[0], msg[1], msg[2])
      output.write(s)
    else:
      skip = 1

output.close()

