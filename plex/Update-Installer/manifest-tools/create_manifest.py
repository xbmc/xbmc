#!/usr/bin/env python

import os, sys, platform, re
from update import Update, FileElement, PackageElement
from hashutils import get_files, get_file
import optparse
import zipfile
from bz2 import BZ2File

if __name__ == "__main__":
  o = optparse.OptionParser()
  o.add_option("-r", dest="product", default="PlexHomeTheater", type="string")
  o.add_option('-v', dest="version", default="NOVERSION", type="string")
  o.add_option("-o", dest="output", default=".", type="string")
  o.add_option("-p", dest="platform", default=platform.system().lower(), type="string")
  o.add_option("-f", dest="fromversion", default="", type="string")
  o.add_option("-m", dest="mainbinary", default="Contents/MacOS/Plex Home Theater", type="string")
  o.add_option("-e", dest="exclude", default="", type="string")

  (options, args) = o.parse_args()
  package = "%s-%s-%s-full" % (options.product, options.version, options.platform)

  if len(args) < 1:
    print "Need directory argument"
    sys.exit(1)

  if not os.path.isdir(args[0]):
    print "First argument must be a directory!"
    sys.exit(1)

  directory = args[0]
  if not directory[len(directory)-1] == "/":
    directory = directory + "/"

  excludere = None
  if len(options.exclude) > 0:
    excludere = re.compile(options.exclude)

  hashes = get_files(directory)
  ihashes = []
  for h in hashes:
    h.name = h.name.replace(directory, "")
    if h.name == options.mainbinary:
      h.is_main_binary = "true"

    h.package = package
    if excludere is None or excludere.match(h.name) is None:
      ihashes.append(h)
    else:
      print "Excluding %s" % h.name
  
  update = Update()
  update.install = ihashes
  update.manifest = ihashes
  update.version = 4
  update.targetVersion = options.version
  update.platform = options.platform

  print "Creating package..."

  try: os.makedirs(options.output)
  except: pass

  packagepath = os.path.join(options.output, package + ".zip")

  zfile = zipfile.ZipFile(packagepath, "w", zipfile.ZIP_DEFLATED)
  for h in ihashes:
    zfile.write(os.path.join(directory, h.name), h.name)

  zfile.close()
  print "%s created" % packagepath

  fpe = get_file(packagepath)
  pe = PackageElement()
  pe.fileHash = fpe.fileHash
  pe.name = package + ".zip"
  pe.size = fpe.size

  update.packages.append(pe)

  manifestpath = os.path.join(options.output, package.replace("-full", "-manifest.xml.bz2"))

  manifestfp = BZ2File(manifestpath, "w")
  manifestfp.write(update.render(pretty=True))
  manifestfp.close()

  print "%s created" % (manifestpath)
