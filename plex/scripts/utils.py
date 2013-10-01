import hashlib

def make_shasum(fpath):
	sha1 = hashlib.sha1()
	f = open(fpath, "rb")
	try:
		sha1.update(f.read())
	finally:
		f.close()

	return sha1.hexdigest()
