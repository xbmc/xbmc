from multiprocessing import Pool
import os, sys, stat
import hashlib
from update import FileElement

def get_file(fpath):
  sha_hash = hashlib.sha1()
  fp = open(fpath, "rb")
  try:
    sha_hash.update(fp.read())
  except:
    print "Failed to process %s" % fpath
    return None

  fe = FileElement()
  fe.name = fpath
  fe.fileHash = sha_hash.hexdigest()

  info = os.stat(fpath)
  fe.size = info.st_size
  fe.permissions = oct(stat.S_IMODE(info.st_mode))
  fe.included = "true"

  #print "%s = %s" % (fe.name, fe.fileHash)

  return fe

def get_files(directory):
  filestohash = []
  for root, dirs, files in os.walk(directory):
    for f in files:
      fpath = os.path.join(root, f)
      filestohash.append(fpath)

  print "Going to hash %d files" % len(filestohash)
  pool = Pool(processes=4)
  res = pool.map(get_file, filestohash)

  return res