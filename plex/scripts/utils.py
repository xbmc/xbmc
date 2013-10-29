#!/usr/bin/env python

import hashlib
import sys
import glob

def make_shasum(fpath):
	sha1 = hashlib.sha1()
	try:
		sha1.update(fpath.read())
	finally:
		fpath.close()

	return sha1.hexdigest()

if __name__ == "__main__":
	for f in sys.argv[1:]:
		for f2 in glob.glob(f):
			fp=open(f2, "rb")
			print f2, make_shasum(fp)