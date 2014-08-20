#!/usr/bin/env python
import sys, os
from update import Update, PatchElement, PackageElement
from optparse import OptionParser
from multiprocessing import Pool
import itertools, tempfile, zipfile, shutil
from hashlib import sha1
from subprocess import call

bsdiff_exe = None
 
class UpdateBundle:
  updateElement = None
  manifestPath = None
  zipfilePath = None
  directory = None

  def __init__(self, manifestPath):
    self.manifestPath = manifestPath
    self.updateElement = Update.parse(file(manifestPath).read())

    rootdir = os.path.dirname(self.manifestPath)
    basename = os.path.basename(self.manifestPath)
    basename = basename.replace("-manifest.xml", "-full.zip")
    self.zipfilePath = os.path.join(rootdir, basename)
    print "zipfilePath = %s" % self.zipfilePath

  def unpack(self):
    # create temporary directory
    self.directory = tempfile.mkdtemp()
    zf = zipfile.ZipFile(self.zipfilePath)
    print "Unpacking %s to %s" % (self.zipfilePath, self.directory)
    zf.extractall(self.directory)

  def cleanup(self):
    shutil.rmtree(self.directory)

def sizefromfile(fp):
  info = os.stat(fp)
  return info.st_size

def sha1fromfile(fp):
  sh = sha1()
  sh.update(file(fp).read())
  return sh.hexdigest()

def create_delta(data):
  oBundle = data[0]
  tBundle = data[1]
  oElement = data[0].updateElement
  tElement = data[1].updateElement

  print "Diffing %s->%s" % (oElement.targetVersion, tElement.targetVersion)
  oldmap = oElement.get_filemap()
  targetmap = tElement.get_filemap()

  print "Unpacking old version"
  oBundle.unpack()

  newUpdate = Update()
  newUpdate.targetVersion = tElement.targetVersion
  newUpdate.platform = tElement.platform
  newUpdate.version = 4

  zipname = os.path.splitext(os.path.basename(tBundle.zipfilePath))[0].replace("-full", "")
  newUpdateFileName = "%s-delta-from-%s.zip" % (zipname, oElement.targetVersion)

  zf = zipfile.ZipFile(newUpdateFileName, "w")

  diffdir = tempfile.mkdtemp()

  for tpath in targetmap.keys():
    tfile = targetmap[tpath]
    if tpath in oldmap:
      ofile = oldmap[tpath]

      if not ofile == tfile:
        diffpath = os.path.join(diffdir, tfile.name + ".bsdiff")
        print "Diffing file %s" % diffpath

        try: os.makedirs(os.path.dirname(os.path.join(diffdir, tfile.name)))
        except: pass

        arguments = [bsdiff_exe, os.path.join(oBundle.directory, tfile.name),
                     os.path.join(tBundle.directory, tfile.name), diffpath]
        print arguments
        if call(arguments) == 1:
          print "Failed to create patch file"
          sys.exit(1)

        zf.write(diffpath, tfile.name + ".bsdiff")

        pe = PatchElement()
        pe.name = tfile.name
        pe.patchHash = sha1fromfile(diffpath)
        pe.patchName = tfile.name + ".bsdiff"
        pe.targetHash = tfile.fileHash
        pe.targetSize = tfile.size
        pe.targetPerm = tfile.permissions
        pe.sourceHash = ofile.fileHash
        pe.package = newUpdateFileName.replace(".zip", "")

        newUpdate.patches.append(pe)

    else:
      zf.write(os.path.join(tBundle.directory, tfile.name), tfile.name)
      tfile.package = newUpdateFileName.replace(".zip", "")
      newUpdate.install.append(tfile)

    tfile.package = newUpdateFileName.replace(".zip", "")
    newUpdate.manifest.append(tfile)

  oBundle.cleanup()
  zf.close()
  print "%s created!" % newUpdateFileName

  package = PackageElement()
  package.fileHash = sha1fromfile(newUpdateFileName)
  package.name = newUpdateFileName.replace(".zip", "")
  package.size = sizefromfile(newUpdateFileName)

  newUpdate.packages.append(package)

  updateManifestPath = newUpdateFileName.replace(".zip", "-manifest.xml")
  fp = file(updateManifestPath, "w")
  fp.write(newUpdate.render(pretty=True))
  fp.close()

  print "%s created!" % updateManifestPath

  return newUpdate

if __name__ == "__main__":
  o = OptionParser()
  o.add_option("-b", dest="bsdiff", default="bsdiff-endsley", type="string")
  options, args = o.parse_args()

  bsdiff_exe = options.bsdiff

  if len(args) < 2:
    print "You need to specify at least two manifests"
    sys.exit(1)

  # use first manifest as the target version
  target = args[0]

  if not os.path.exists(target):
    print "The target manifest is not available."
    sys.exit(1)

  targetUpdate = UpdateBundle(target)
  print "Target version is %s" % targetUpdate.updateElement.targetVersion

  versions = []
  for version in args[1:]:
    #try:
      versionupdate = UpdateBundle(version)
      print "Old version: %s" % versionupdate.updateElement.targetVersion
      versions.append(versionupdate)
    #except:
    #  print "Failed to read manifest %s" % version
    #  continue

  if len(versions) < 1:
    print "No versions to delta against..."
    sys.exit(1)

  targetUpdate.unpack()

  pool = Pool(2)
  res = pool.map(create_delta, itertools.izip(versions, itertools.repeat(targetUpdate)))

  targetUpdate.cleanup()


