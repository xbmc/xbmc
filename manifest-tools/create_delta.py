#!/usr/bin/env python
import sys, os
from update import Update, PatchElement, PackageElement
from optparse import OptionParser
from multiprocessing import Pool
import itertools, tempfile, zipfile, shutil
from hashlib import sha1
from subprocess import call
import bz2

bsdiff_exe = None
output_path = None
 
class UpdateBundle:
  updateElement = None
  manifestPath = None
  zipfilePath = None
  directory = None

  def __init__(self, manifestPath):
    self.manifestPath = manifestPath
    self.updateElement = Update.parse(bz2.BZ2File(manifestPath).read())

    rootdir = os.path.dirname(self.manifestPath)
    basename = os.path.basename(self.manifestPath)
    basename = basename.replace("-manifest.xml.bz2", "-full.zip")
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

def run_bsdiff(data):
  tfile, ofile, diffdir, oBundle, tBundle = data

  diffpath = os.path.join(diffdir, tfile.name + ".bsdiff")

  try: os.makedirs(os.path.dirname(os.path.join(diffdir, tfile.name)))
  except: pass

  arguments = [bsdiff_exe, os.path.join(oBundle.directory, tfile.name),
               os.path.join(tBundle.directory, tfile.name), diffpath]
  if call(arguments) == 1:
    print "Failed to create patch file from %s" % tfile.name
    raise IOError

  pe = PatchElement()
  pe.name = tfile.name
  pe.patchHash = sha1fromfile(diffpath)
  pe.patchName = tfile.name + ".bsdiff"
  pe.targetHash = tfile.fileHash
  pe.targetSize = tfile.size
  pe.targetPerm = tfile.permissions
  pe.sourceHash = ofile.fileHash

  return pe

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

  zf = zipfile.ZipFile(os.path.join(output_path, newUpdateFileName), "w")

  diffdir = tempfile.mkdtemp()

  filestodiff = []
  filestoadd = []

  # first iterate all the files and
  # find the ones that needs to be
  # diffed, the ones that needs to be installd
  # and the ones that just needs to be documented
  for tpath in targetmap.keys():
    tfile = targetmap[tpath]
    if tpath in oldmap and tfile.targetLink is None:
      ofile = oldmap[tpath]
      if not ofile == tfile:
        filestodiff.append((tfile, ofile, diffdir, oBundle, tBundle))
    else:
      filestoadd.append(tfile)

    tfile.package = newUpdateFileName.replace(".zip", "")
    newUpdate.manifest.append(tfile)

  # run bsdiff in parallel, we can't add files to the
  # zip in parallel because it's not thread safe
  print "Going to run delta on %d files" % len(filestodiff)
  pool = Pool(4)
  newUpdate.patches = pool.map(run_bsdiff, filestodiff)

  # now add the bsdiff fragments to the zip file
  for pe in newUpdate.patches:
    zf.write(os.path.join(diffdir, pe.name + ".bsdiff"), pe.name + ".bsdiff", zipfile.ZIP_STORED)
    pe.package = newUpdateFileName.replace(".zip", "")

  # and the rest of the files
  for fa in filestoadd:
    zf.write(os.path.join(tBundle.directory, fa.name), fa.name, zipfile.ZIP_DEFLATED)
    fa.package = newUpdateFileName.replace(".zip", "")
    newUpdate.install.append(fa)

  oBundle.cleanup()
  zf.close()
  print "%s created!" % newUpdateFileName

  package = PackageElement()
  package.fileHash = sha1fromfile(os.path.join(output_path, newUpdateFileName))
  package.name = newUpdateFileName.replace(".zip", "")
  package.size = sizefromfile(os.path.join(output_path, newUpdateFileName))

  newUpdate.packages.append(package)

  updateManifestPath = newUpdateFileName.replace(".zip", "-manifest.xml.bz2")
  bz = bz2.BZ2File(os.path.join(output_path, updateManifestPath), "w")
  bz.write(newUpdate.render(pretty=True))
  bz.close()

  print "%s created!" % updateManifestPath

  return newUpdate

if __name__ == "__main__":
  o = OptionParser()
  o.add_option("-b", dest="bsdiff", default="bsdiff-endsley", type="string")
  o.add_option("-o", dest="output", default=".", type="string")
  options, args = o.parse_args()

  bsdiff_exe = options.bsdiff
  output_path = options.output

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
    versionupdate = UpdateBundle(version)
    if versionupdate.updateElement.platform == targetUpdate.updateElement.platform:
      print "Old version: %s" % versionupdate.updateElement.targetVersion
      versions.append(versionupdate)
    else:
      print "! Ignoring %s because it was another platform" % version

  if len(versions) < 1:
    print "No versions to delta against..."
    sys.exit(1)

  targetUpdate.unpack()

  map(create_delta, itertools.izip(versions, itertools.repeat(targetUpdate)))

  targetUpdate.cleanup()


