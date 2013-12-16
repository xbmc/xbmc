#!/usr/bin/env python

import sys, os, shutil

lang_map = {
	"af-ZA": "Afrikaans",
	"cs-CZ": "Czech",
	"da": "Danish",
	"de": "German",
	"en": "English (US)",
	"es": "Spanish",
	"es-419" : "Spanish (Argentina)",
	"fi": "Finnish",
	"fr": "French",
	"grk": "Greek",
	"he": "Hebrew",
	"hr-HR": "Croatian",
	"is-IS": "Icelandic",
	"it": "Italian",
	"ko": "Korean",
	"lt": "Latvian",
	"nl": "Dutch",
	"no": "Norwegian",
	"pl-PL": "Polish",
	"pt-BR": "Portuguese (Brazil)",
	"pt-PT": "Portuguese",
	"ru": "Russian",
	"sr": "Serbian",
	"sv": "Swedish",
	"zh-CN": "Chinese (Simple)"
}

if __name__ == "__main__":

	if len(sys.argv) != 3:
		print "Need two arguments"
		sys.exit(1)

	d = sys.argv[1]
	dest = sys.argv[2]

	if not os.path.isdir(d):
		print "%s is not a dir!" % d
		sys.exit(1)

	if not os.path.isdir(dest):
		print "%s is not a xbmc lang dir" % dest
		sys.exit(1)

	langdir = os.path.join(dest, "language")
	skinlangdir = os.path.join(dest, "addons", "skin.plex", "language")

	if not os.path.isdir(langdir) or not os.path.isdir(skinlangdir):
		print "Can't find %s and %s" % (langdir, skinlangdir)
		sys.exit(1)

	for l in os.listdir(d):
		if not l in lang_map:
			print "Can't find mapping for %s" % l
			continue

		xlang = lang_map[l]
		xlang += "_plex"
		
		xlangfile = os.path.join(langdir, xlang, "strings.po")
		xskinlangfile = os.path.join(skinlangdir, xlang, "strings.po")
		ld = os.path.join(d, l)

		pofile = os.path.join(ld, "strings_%s.po" % l)
		spofile = os.path.join(ld, "string_skin_%s.po" % l)

		if os.path.exists(pofile):
			if not os.path.isdir(os.path.join(langdir, xlang)):
				print "Can't find dir %s" % os.path.join(langdir, xlang)
			else:
				print "%s->%s" % (pofile, xlangfile)
				shutil.copyfile(pofile, xlangfile)

		if os.path.exists(spofile):
			if not os.path.isdir(os.path.join(skilangdir, xlang)):
				print "Can't find dir %s" % os.path.join(skinlangdir, xlang)
			else:
				print "%s->%s" % (spofile, xskinlangfile)
				shutil.copyfile(spofile, xskinlangfile)

