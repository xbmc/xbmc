import os, shutil, subprocess
from update import Update
from bz2 import BZ2File

def test_updater(env):
  targetdir = os.path.join(env.workdir, "target_updater")
  targetversion = env.VERSIONS[-1]

  for v in env.VERSIONS[:-1]:
    shutil.copytree(os.path.join(env.workdir, v), targetdir)

    manifestPath = os.path.join(env.workdir, "test-%s-%s-delta-from-%s-manifest.xml.bz2" % (targetversion, env.PLATFORM, v))
    print "trying %s" % manifestPath

    um = Update.parse(BZ2File(manifestPath).read())

    cmd = ["../build/src/updater", "--install-dir=%s" % targetdir, "--package-dir=%s" % env.workdir, "--auto-close", "--script=%s" % manifestPath]
    print "running: %s" % (" ".join(cmd))
    res = subprocess.call(cmd)
    assert res == 0

    for f in um.manifest:
      fpath = os.path.join(targetdir, f.name)
      assert os.path.exists(fpath)
      assert env.get_sha1(fpath) == f.fileHash

    fmap = um.get_filemap()

    for root, dirs, files in os.walk(targetdir):
      for f in files:
        fname = os.path.join(root, f)
        fname = fname.replace(targetdir + "/", "")

        assert fname in fmap

    shutil.rmtree(targetdir)

