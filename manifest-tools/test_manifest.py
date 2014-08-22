from update import Update
from hashlib import sha1
import zipfile, os

def test_exists(env):
  for manifest in env.get_manifestPaths():
    assert os.path.exists(manifest)

  for z in env.get_zipPaths():
    assert os.path.exists(z)

def test_parse_manifest(env):
  for manifest in env.get_manifestPaths():
    u = Update.parse(file(manifest, "r").read())
    assert u.platform == env.PLATFORM

    filemap = u.get_filemap()

    for f in env.files[u.targetVersion]:
      assert f in filemap, ("File: %s was not in manifest?" % f)

    for f in u.manifest:
      filePath = os.path.join(os.path.dirname(manifest), u.targetVersion, f.name)
      assert os.path.exists(filePath)

      # compare sha sum.
      assert env.get_sha1(filePath) == f.fileHash

def test_check_zipfiles(env):
  for manifest in env.get_manifestPaths():
    u = Update.parse(file(manifest, "r").read())
    zfile = os.path.join(env.workdir, u.packages[0].name + ".zip")
    assert u.packages[0].fileHash == env.get_sha1(zfile)

    filemap = u.get_filemap()
    zf = zipfile.ZipFile(zfile)
    for fp in zf.namelist():
      assert fp in filemap
      fe = filemap[fp]

      data = zf.read(fp)
      s = sha1()
      s.update(data)
      assert fe.fileHash == s.hexdigest()

