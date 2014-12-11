#!/usr/bin/python

import yaml, optparse
import os, sys, re

RELEASENOTEFILE="plex/ReleaseNotes/%s.txt"

def get_pht_version():
  if not os.path.exists("CMakeLists.txt"):
    return None

  rm = re.compile("set\(PLEX_VERSION_(\w+)\s(\d+)\)")
  with open("CMakeLists.txt", "r") as cfile:
    versionmap = {}
    for line in cfile:
      match = rm.match(line)
      if match is not None:
        versionmap[match.group(1)] = match.group(2)

    if len(versionmap) == 3:
      version = "%s.%s.%s" % (versionmap["MAJOR"], versionmap["MINOR"], versionmap["PATCH"])
      return version

  return None

def print_forum_node(node, inlist=False):
  if isinstance(node, dict):
    for heading in node:
      if inlist:
        print "[*]%s[/*]" % heading
      else:
        print "[b]%s[/b]" % heading
      print_forum_node(node[heading])
      print ""
  elif isinstance(node, list):
    print "[LIST]"
    for value in node:
      print_forum_node(value, True)
    print "[/LIST]"
  elif inlist:
    print "[*]%s[/*]" % node
  else:
    print node

if __name__ == '__main__':
  parser = optparse.OptionParser()
  parser.add_option("-v", "--version", action="store", type="string", dest="version", default=None)

  (options, argv) = parser.parse_args(sys.argv)

  version = options.version
  if version is None:
    version = get_pht_version()
    if version is None:
      print "No version found"
      sys.exit(1)

  if os.path.exists(RELEASENOTEFILE % version):
    rnotes = yaml.load(file(RELEASENOTEFILE % version, "r"))
  else:
    rnotes = {"New":[], "Fixed":[]}

  try:
    command = argv[1]
  except:
    print "Need a command.."
    sys.exit(1)
  text = " ".join(argv[2:])

  if command == "new":
    rnotes["New"].append(text)
    print yaml.dump(rnotes, default_flow_style=False, indent=2)
  elif command == "fixed":
    rnotes["Fixed"].append(text)
    print yaml.dump(rnotes, default_flow_style=False, indent=2)
  elif command == "print":
    style="forum"
    if len(argv) > 2:
      style = argv[2]

    if style == "forum":
      print_forum_node(rnotes)

  else:
    print "You need to supply a command."
    sys.exit(1)

  


