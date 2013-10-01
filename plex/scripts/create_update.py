#!/usr/bin/env python
import os, optparse, sys, hashlib, stat
import lxml.etree as et
import platform
import zipfile
from utils import make_shasum

def create_update(product, version, output, platform, input, delta, fversion):
	if not os.path.isdir(input):
		print "Input directory %s can't be read." % input
		return False

	if not os.path.isdir(output):
		print "Output directory %s can't be read." % output
		return False

	if version == "NOVERSION":
		print "Consider adding -v to specify a version..."

	rootel = et.Element("update")
	rootel.set("version", "3")

	targetVersion = et.SubElement(rootel, "targetVersion")
	targetVersion.text = version

	if delta:
		deltael = et.SubElement(rootel, "delta")
		deltael.text = fversion

	platformel = et.SubElement(rootel, "platform")
	platformel.text = platform

	dependencies = et.SubElement(rootel, "dependencies")
	updatefile = et.SubElement(dependencies, "file")
	updatefile.text = "updater"

	install = et.SubElement(rootel, "install")

	prodstr = "%s-%s-%s" % (product, version, platform)

	tarfilename = prodstr + "-update_full.zip"
	if delta:
		tarfilename = prodstr + "-update_delta-%s.zip" % fversion

	archive = zipfile.ZipFile(tarfilename, "w")

	for root,dirs,files in os.walk(input):
		for f in files:
			fpath = os.path.join(root, f)
			frelpath = fpath.replace(input + "/", "Plex Home Theater.app/")
			shasum = make_shasum(fpath)
			size = os.path.getsize(fpath)
			prems = oct(stat.S_IMODE(os.lstat(fpath).st_mode))

			archive.write(fpath, frelpath, zipfile.ZIP_DEFLATED)

			fileEl = et.SubElement(install, "file")

			name = et.SubElement(fileEl, "name")
			name.text = frelpath

			sizeel = et.SubElement(fileEl, "size")
			sizeel.text = str(size)

			permissions = et.SubElement(fileEl, "permissions")
			permissions.text = str(prems)

			hashel = et.SubElement(fileEl, "hash")
			hashel.text = shasum

			package = et.SubElement(fileEl, "package")
			package.text = tarfilename.replace(".zip", "")


	archive.close()

	shasum = make_shasum(tarfilename)

	packages = et.SubElement(rootel, "packages")
	package = et.SubElement(packages, "package")

	name = et.SubElement(package, "name")
	name.text = tarfilename.replace(".zip", "")

	hashel = et.SubElement(package, "hash")
	hashel.text = shasum

	sizeel = et.SubElement(package, "size")
	sizeel.text = str(os.path.getsize(tarfilename))
	tree = et.ElementTree(rootel)
	tree.write(tarfilename.replace(".zip", ".xml"), pretty_print=True)

	return True

if __name__ == "__main__":
	o = optparse.OptionParser()
	o.add_option("-r", dest="product", default="PlexHomeTheater", type="string")
	o.add_option('-v', dest="version", default="NOVERSION", type="string")
	o.add_option("-o", dest="output", default=".", type="string")
	o.add_option("-p", dest="platform", default=platform.system().lower(), type="string")
	o.add_option("-i", dest="input", default="output/Plex Home Theater.app", type="string")
	o.add_option("-d", dest="delta", default=False, action="store_true")
	o.add_option("-f", dest="fromversion", default="", type="string")

	(options, args) = o.parse_args()

	if not create_update(options.product, options.version, options.output, options.platform, options.input, options.delta, options.fromversion):
		sys.exit(1)
