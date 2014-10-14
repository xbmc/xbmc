#!/usr/bin/env python

import hashlib, hashutils
import sys, os, shutil
from update import Update

def verify_directory(manifestPath, directory):
  u = None
  try:
    u = Update.parse(file(manifestPath).read())
  except:
    print "Failed to parse manifest: " + manifestPath
    return False

  if not os.path.isdir(directory):
    print "%s is not a directory" % directory
    return False

  filemap = u.get_filemap()
  for root, dirs, files in os.walk(directory):
    for f in files:
      fullpath = os.path.join(root, f)
      fpath = fullpath.replace(directory + "/", "")
      if not fpath in filemap:
        print "Could not find file %s in manifest" % fpath
        return False

      ufe = filemap[fpath]

      fe = hashutils.get_file((fullpath, directory + "/"))
      if not fe == ufe:
        print "Could not match %s to the manifest: %s" % (fpath, ",".join(fe.whyneq(ufe)))
        return False

  return True

def test_verify_dir(env):
  appdir = os.path.join(env.workdir, env.VERSIONS[0])
  manifest = env.get_manifestPaths()[0]
  assert verify_directory(manifest, appdir)

def test_verify_dir_wrong_hash(env):
  tdir = os.path.join(env.workdir, "verify_target")
  shutil.copytree(os.path.join(env.workdir, env.VERSIONS[0]), tdir)
  manifest = env.get_manifestPaths()[0]
  assert verify_directory(manifest, tdir)

  # now change a hash.
  fp = file(os.path.join(tdir, "not_changed"), "w")
  fp.write("hah, I changed it up!")
  fp.close()

  assert not verify_directory(manifest, tdir)

# this should also fail this test
def test_add_file_to_app(env):
  tdir = os.path.join(env.workdir, "verify_target_new_file")
  shutil.copytree(os.path.join(env.workdir, env.VERSIONS[0]), tdir)
  manifest = env.get_manifestPaths()[0]
  assert verify_directory(manifest, tdir)

  # add a file
  fp = file(os.path.join(tdir, "hello_new_file"), "w")
  fp.write("I should not be here")
  fp.close()

  assert not verify_directory(manifest, tdir)

if __name__ == '__main__':
  if len(sys.argv) < 2:
    print "Need manifest and directory arguments"
    sys.exit(1)

  verify_directory(sys.argv[1], sys.argv[2])


