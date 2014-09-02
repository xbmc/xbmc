import os, shutil
from update import Update
import subprocess, zipfile
from bz2 import BZ2File

def test_delta_exists(env):
  for de in env.get_deltaZipPaths():
    assert os.path.exists(de)

  for dm in env.get_deltaManifestPaths():
    assert os.path.exists(dm)

def test_delta_manifest_parse(env):
  for dm in env.get_deltaManifestPaths():
    u = Update.parse(BZ2File(dm).read())

def test_patch_delta(env):
  # first make a copy of the version we want to patch
  targetdir = os.path.join(env.workdir, "target")
  patchdir = os.path.join(env.workdir, "patches")
  shutil.copytree(os.path.join(env.workdir, "v1.0"), targetdir)

  os.makedirs(patchdir)
  zf = zipfile.ZipFile(env.get_deltaZipPaths()[0])
  zf.extractall(patchdir)
  zf.close()

  deltamanifest = Update.parse(BZ2File(env.get_deltaManifestPaths()[0], "r").read())
  for p in deltamanifest.patches:
    cmd = ["../build/external/bsdiff/bspatch-endsley",
            os.path.join(targetdir, p.name),
            os.path.join(targetdir, p.name + ".new"),
            os.path.join(patchdir, p.patchName)]
    print cmd
    res = subprocess.call(cmd)
    assert res == 0
    assert file(os.path.join(targetdir, p.name + ".new")).read() == "This file changes with versions v3.0"

  shutil.rmtree(targetdir)
  shutil.rmtree(patchdir)