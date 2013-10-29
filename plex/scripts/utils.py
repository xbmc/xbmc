#!/usr/bin/env python

import hashlib
import sys

def make_shasum(fpath):
	sha1 = hashlib.sha1()
	try:
		sha1.update(fpath.read())
	finally:
		fpath.close()

	return sha1.hexdigest()

if __name__ == "__main__":
	for f in sys.argv[1:]:
		fp=open(f, "r")
		print f, make_shasum(fp)