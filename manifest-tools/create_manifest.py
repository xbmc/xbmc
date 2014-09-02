#!/usr/bin/env python

import os, sys, platform
from update import Update, FileElement, PackageElement
from hashutils import get_files, get_file
import optparse
import zipfile

if __name__ == "__main__":
  o = optparse.OptionParser()
  o.add_option("-r", dest="product", default="PlexHomeTheater", type="string")
  o.add_option('-v', dest="version", default="NOVERSION", type="string")
  o.add_option("-o", dest="output", default=".", type="string")
  o.add_option("-p", dest="platform", default=platform.system().lower(), type="string")
  o.add_option("-f", dest="fromversion", default="", type="string")
  o.add_option("-m", dest="mainbinary", default="Contents/MacOS/Plex Home Theater", type="string")

  (options, args) = o.parse_args()
  package = "%s-%s-%s" % (options.product, options.version, options.platform)

  if len(args) < 1:
    print "Need directory argument"
    sys.exit(1)

  if not os.path.isdir(args[0]):
    print "First argument must be a directory!"
    sys.exit(1)

  directory = args[0]
  if not directory[len(directory)-1] == "/":
    directory = directory + "/"

  hashes = get_files(directory)
  for h in hashes:
    h.name = h.name.replace(directory, "")
    if h.name == options.mainbinary:
      h.is_main_binary = "true"

    h.package = package
  
  update = Update()
  update.manifest = hashes
  update.version = 4
  update.targetVersion = options.version
  update.platform = options.platform

  print "Creating package..."

  try: os.makedirs(options.output)
  except: pass

  packagepath = os.path.join(options.output, package + "-full.zip")

  zfile = zipfile.ZipFile(packagepath, "w", zipfile.ZIP_STORED)
  for h in hashes:
    zfile.write(os.path.join(directory, h.name), h.name)

  zfile.close()
  print "%s created" % packagepath

  fpe = get_file(packagepath)
  pe = PackageElement()
  pe.fileHash = fpe.fileHash
  pe.name = package + "-full.zip"
  pe.size = fpe.size

  update.packages.append(pe)

  manifestpath = os.path.join(options.output, package + "-manifest.xml")

  manifestfp = file(manifestpath, "w")
  manifestfp.write(update.render(pretty=True))
  manifestfp.close()

  print "%s created" % (manifestpath)
