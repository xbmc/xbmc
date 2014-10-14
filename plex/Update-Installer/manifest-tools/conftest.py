import pytest
from hashlib import sha1
import os, subprocess, tempfile, shutil


class Environment:
  VERSIONS = ("v1.0", "v2.0", "v3.0")
  PLATFORM = "unknown"

  def __init__(self):
    print "Creating environment"
    self.workdir = tempfile.mkdtemp()
    for v in self.VERSIONS:
      os.makedirs(os.path.join(self.workdir, v))

    self.olddir = os.getcwd()
    os.chdir(self.workdir)

    for version in self.VERSIONS:
      file(os.path.join(self.workdir, version, "not_changed"), "w").write("This file should not change")
      file(os.path.join(self.workdir, version, "contains_version_number"), "w").write("This file changes with versions %s" % version)
      file(os.path.join(self.workdir, version, version + "txt"), "w").write("This is unique to this version")

      subprocess.call([os.path.join(self.olddir, "create_manifest.py"), "-p", self.PLATFORM, "-v", version, "-r", "test", os.path.join(self.workdir, version)])

    # create delta from the latest version
    self.latestVersion = self.VERSIONS[-1]
    bsdiffpath = os.path.join(self.olddir, "..", "build", "external", "bsdiff", "bsdiff-endsley")

    print "Creating deltas"
    subprocess.call([os.path.join(self.olddir, "create_delta.py"), "-b", bsdiffpath, self.get_manifestPaths()[-1]] + self.get_manifestPaths()[:-1])
    os.chdir(self.olddir)

    print "Environment created in %s" % self.workdir 

  def get_manifestPaths(self):
    return [os.path.join(self.workdir, "test-%s-%s-manifest.xml.bz2") % (v, self.PLATFORM) for v in self.VERSIONS]

  def get_zipPaths(self):
    return [os.path.join(self.workdir, "test-%s-%s-full.zip") % (v, self.PLATFORM) for v in self.VERSIONS]

  def get_deltaZipPaths(self):
    return [os.path.join(self.workdir, "test-%s-%s-delta-from-%s.zip" % (self.latestVersion, self.PLATFORM, v)) for v in self.VERSIONS[:-1]]

  def get_deltaManifestPaths(self):
    return [os.path.join(self.workdir, "test-%s-%s-delta-from-%s-manifest.xml.bz2" % (self.latestVersion, self.PLATFORM, v)) for v in self.VERSIONS[:-1]]

  def get_sha1(self, path):
    fp = file(path, "r")

    s = sha1()
    s.update(fp.read())
    return s.hexdigest()


@pytest.fixture(scope="session")
def env(request):
  env = Environment()

  def fin():
    print "Destroying environment"
    shutil.rmtree(env.workdir)

  #request.addfinalizer(fin)

  return env

