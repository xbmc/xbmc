import hashlib

def make_shasum(fpath):
	sha1 = hashlib.sha1()
	try:
		sha1.update(fpath.read())
	finally:
		fpath.close()

	return sha1.hexdigest()
