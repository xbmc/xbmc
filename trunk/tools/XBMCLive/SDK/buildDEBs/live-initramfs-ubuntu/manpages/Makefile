all: doc

doc: doc_man doc_html

doc_html: html-stamp

html-stamp: live-initramfs.en.7.txt live-snapshot.en.1.txt
	asciidoc -b xhtml11 live-initramfs.en.7.txt
	asciidoc -b xhtml11 live-snapshot.en.1.txt
	touch html-stamp

doc_man: man-stamp

man-stamp: live-initramfs.en.7.txt live-snapshot.en.1.txt
	# live-initramfs:
	asciidoc -d manpage -b docbook live-initramfs.en.7.txt
	xsltproc /usr/share/xml/docbook/stylesheet/nwalsh/manpages/docbook.xsl live-initramfs.en.7.xml
	mv live-initramfs.7 live-initramfs.en.7
	# live-snapshot:
	asciidoc -d manpage -b docbook live-snapshot.en.1.txt
	xsltproc /usr/share/xml/docbook/stylesheet/nwalsh/manpages/docbook.xsl live-snapshot.en.1.xml
	mv live-snapshot.1 live-snapshot.en.1
	touch man-stamp

clean:
	rm -f live-initramfs.en.7.html live-initramfs.en.7.xml live-initramfs.en.7
	rm -f live-snapshot.en.1.html live-snapshot.en.1.xml live-snapshot.en.1
	rm -f html-stamp man-stamp
