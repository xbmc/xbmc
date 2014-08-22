import os, shutil, subprocess, time
from update import Update
from verify_app import verify_directory

def test_updater_delta(env):
  targetdir = os.path.join(env.workdir, "target_updater_delta")
  targetversion = env.VERSIONS[-1]

  for v in env.VERSIONS[:-1]:
    shutil.copytree(os.path.join(env.workdir, v), targetdir)

    manifestPath = os.path.join(env.workdir, "test-%s-%s-delta-from-%s-manifest.xml" % (targetversion, env.PLATFORM, v))
    print "trying %s" % manifestPath

    um = Update.parse(file(manifestPath).read())

    cmd = ["../build/src/updater", "--install-dir=%s" % targetdir, "--package-dir=%s" % env.workdir, "--auto-close", "--script=%s" % manifestPath]
    print "running: %s" % (" ".join(cmd))
    res = subprocess.call(cmd)
    assert res == 0

    assert verify_directory(manifestPath, targetdir)

    shutil.rmtree(targetdir)

def test_updater_full(env):
  targetdir = os.path.join(env.workdir, "target_updater_full")
  targetversion = env.VERSIONS[-1]
  source = env.VERSIONS[0]
  sourcedir = os.path.join(env.workdir, source)
  shutil.copytree(sourcedir, targetdir)
  manifestPath = env.get_manifestPaths()[-1]

  cmd = ["../build/src/updater", "--install-dir=%s" % targetdir, "--package-dir=%s" % env.workdir, "--auto-close", "--script=%s" % manifestPath]
  res = subprocess.call(cmd)
  assert res == 0
  
  assert verify_directory(manifestPath, targetdir)
  shutil.rmtree(targetdir)

# def test_updater_full_fail(env):
#   targetdir = os.path.join(env.workdir, "target_updater_full_fail")
#   targetversion = env.VERSIONS[-1]
#   source = env.VERSIONS[0]
#   sourcedir = os.path.join(env.workdir, source)
#   shutil.copytree(sourcedir, targetdir)
#   manifestPath = env.get_manifestPaths()[-1]
#   oldmanifestPath = env.get_manifestPaths()[0]

#   cmd = ["../build/src/updater", "--install-dir=%s" % targetdir, "--package-dir=%s" % env.workdir, "--auto-close", "--script=%s" % manifestPath]
#   po = subprocess.Popen(cmd)
#   time.sleep(0.05)
#   po.kill()
#   assert po.wait() == -9
  
#   # since we fail verifying against the new mainfest should fail but the old should succeed!
#   assert not verify_directory(manifestPath, targetdir)
#   assert verify_directory(oldmanifestPath, targetdir)

#   shutil.rmtree(targetdir)

