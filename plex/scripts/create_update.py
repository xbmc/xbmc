#!/usr/bin/env python
import os, optparse, sys, hashlib, stat
import lxml.etree as et
import platform
import zipfile
from utils import make_shasum

def get_file_element(root, frelpath, size, perms, shasum, tarfilename, ismain):
	fileEl = et.SubElement(root, "file")

	name = et.SubElement(fileEl, "name")
	name.text = frelpath

	sizeel = et.SubElement(fileEl, "size")
	sizeel.text = str(size)

	permissions = et.SubElement(fileEl, "permissions")
	permissions.text = str(perms).replace("L", "")

	hashel = et.SubElement(fileEl, "hash")
	hashel.text = shasum

	package = et.SubElement(fileEl, "package")
	package.text = os.path.basename(tarfilename.replace(".zip", ""))

	if ismain:
		ismainel = et.SubElement(fileEl, "is-main-binary")
		ismainel.text = "true"

	return fileEl

def mangle_file_path(path):
	ps = path.split(os.sep)
	if ps[0].endswith(".app"):
		return (os.path.join(*ps[1:]), ps[0])
	return (path, None)

def create_update(product, version, output, platform, input, delta, fversion, mainbinary):
	if not os.path.isdir(input) and not input.endswith(".zip"):
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

	inputiszip = False

	if input.endswith(".zip"):
		tarfilename = input
		inputiszip = True
	else:
		prodstr = "%s-%s-%s" % (product, version, platform)

		tarfilename = prodstr + "-update_full.zip"
		if delta:
			tarfilename = prodstr + "-update_delta-%s.zip" % fversion

	archive = None
	pathprefix = None
	if inputiszip:
		archive = zipfile.ZipFile(tarfilename, "r")
		for info in archive.infolist():
			frelpath, pathprefix = mangle_file_path(info.filename)
			size = info.file_size
			perms = oct((info.external_attr >> 16) & 0777)

			zfile = archive.open(info, "r")
			shasum = make_shasum(zfile)

			ismain = False;
			if frelpath == mainbinary: ismain = True

			fileEl = get_file_element(install, frelpath, size, perms, shasum, tarfilename, ismain)

	else:
		archive = zipfile.ZipFile(tarfilename, "w")
		for root,dirs,files in os.walk(input):
			for f in files:
				fpath = os.path.join(root, f)
				frelpath, pathprefix = mangle_file_path(fpath.replace(input + "/", ""))
				shasum = make_shasum(open(fpath, "r"))
				size = os.path.getsize(fpath)
				perms = oct(stat.S_IMODE(os.lstat(fpath).st_mode))

				archive.write(fpath, frelpath, zipfile.ZIP_DEFLATED)

				ismain = False
				if frelpath == mainbinary: ismain = True

				fileEl = get_file_element(install, frelpath, size, perms, shasum, tarfilename, ismain)


	archive.close()

	if pathprefix is not None and len(pathprefix) > 0:
		ppEl = et.SubElement(rootel, "pathprefix")
		ppEl.text = pathprefix

	shasum = make_shasum(file(tarfilename, "rb"))

	packages = et.SubElement(rootel, "packages")
	package = et.SubElement(packages, "package")

	name = et.SubElement(package, "name")
	name.text = os.path.basename(tarfilename.replace(".zip", ""))

	hashel = et.SubElement(package, "hash")
	hashel.text = shasum

	sizeel = et.SubElement(package, "size")
	sizeel.text = str(os.path.getsize(tarfilename))
	tree = et.ElementTree(rootel)
	tree.write(tarfilename.replace(".zip", "-manifest.xml"), pretty_print=True)

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
	o.add_option("-m", dest="mainbinary", default="Contents/MacOS/Plex Home Theater", type="string")

	(options, args) = o.parse_args()

	if not create_update(options.product, options.version, options.output, options.platform, options.input, options.delta, options.fromversion, options.mainbinary):
		sys.exit(1)
