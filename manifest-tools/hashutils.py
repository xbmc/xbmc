from multiprocessing import Pool
import os, sys, stat
import hashlib
from update import FileElement
import itertools

def get_file(args):
  fpath, prefix = args[0], args[1]

  sha_hash = hashlib.sha1()
  fp = open(fpath, "rb")
  try:
    sha_hash.update(fp.read())
  except:
    print "Failed to process %s" % fpath
    return None

  fe = FileElement()
  fe.name = fpath.replace(prefix, "")
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
  res = pool.map(get_file, itertools.izip(filestohash, itertools.repeat(directory)))

  return res