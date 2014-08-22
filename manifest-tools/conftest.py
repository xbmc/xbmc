import pytest
from hashlib import sha1
import os, subprocess, tempfile, shutil
from string import Template

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

    files = [
      ('not_changed', "This File should not change"),
      ('contains_version_nubmer', "This file is changes with versions $version"),
      ('unique_to_version-$version', "unique"),
      ('subdir/subdir.txt', "subdir file"),
      ('subdir/a/subdir_changes.txt', "subdir file that changes $version"),
      ('subdir/a/b/subdir.txt', "subdir b file")
    ]

    self.files = {}

    for version in self.VERSIONS:
      fileList = []
      for fp, cdata in files:
        fname = Template(fp).substitute(version=version)
        content = Template(cdata).substitute(version=version)
        
        fpath = os.path.join(self.workdir, version, fname)
        fileList.append(fname)

        if os.path.dirname(fpath) != "" and not os.path.exists(os.path.dirname(fpath)):
          os.makedirs(os.path.dirname(fpath))

        f = file(fpath, "w")
        f.write(content)
        f.close()

      self.files[version] = fileList
      subprocess.call([os.path.join(self.olddir, "create_manifest.py"), "-p", self.PLATFORM, "-v", version, "-r", "test", os.path.join(self.workdir, version)])

    # create delta from the latest version
    self.latestVersion = self.VERSIONS[-1]
    bsdiffpath = os.path.join(self.olddir, "..", "build", "external", "bsdiff", "bsdiff-endsley")

    print "Creating deltas"
    subprocess.call([os.path.join(self.olddir, "create_delta.py"), "-b", bsdiffpath, self.get_manifestPaths()[-1]] + self.get_manifestPaths()[:-1])
    os.chdir(self.olddir)

    print "Environment created in %s" % self.workdir 

  def get_manifestPaths(self):
    return [os.path.join(self.workdir, "test-%s-%s-manifest.xml") % (v, self.PLATFORM) for v in self.VERSIONS]

  def get_zipPaths(self):
    return [os.path.join(self.workdir, "test-%s-%s-full.zip") % (v, self.PLATFORM) for v in self.VERSIONS]

  def get_deltaZipPaths(self):
    return [os.path.join(self.workdir, "test-%s-%s-delta-from-%s.zip" % (self.latestVersion, self.PLATFORM, v)) for v in self.VERSIONS[:-1]]

  def get_deltaManifestPaths(self):
    return [os.path.join(self.workdir, "test-%s-%s-delta-from-%s-manifest.xml" % (self.latestVersion, self.PLATFORM, v)) for v in self.VERSIONS[:-1]]

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

