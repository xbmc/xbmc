## Process this with automake to create Makefile.in

SUBDIRS = vorbisfile vorbisenc

docdir = $(datadir)/doc/$(PACKAGE)-$(VERSION)

### all of the static docs, commited to SVN and included as is
static_docs = \
	draft-ietf-avt-rtp-vorbis-06.xml \
	draft-ietf-avt-rtp-vorbis-06.txt \
	eightphase.png \
	evenlsp.png \
	floor1_inverse_dB_table.html \
	floorval.png \
	fourphase.png \
	framing.html \
	helper.html \
	index.html \
	lspmap.png \
	oddlsp.png \
	oggstream.html \
	programming.html \
	squarepolar.png \
	stereo.html \
	stream.png \
	v-comment.html \
	vorbis-clip.txt \
	vorbis-errors.txt \
	vorbis-fidelity.html \
	vorbis.html \
	vorbisword2.png \
	wait.png \
	white-ogg.png \
	white-xifish.png

### FIXME: bits needed by the spec which lives in xml
SPEC_PNG = \
	components.png \
	floor1-1.png \
	floor1-2.png \
	floor1-3.png \
	floor1-4.png \
	hufftree.png \
	hufftree-under.png \
	residue-pack.png \
	residue2.png \
	white-xifish.png \
	window1.png \
	window2.png
SPEC_PDF = xifish.pdf

# FIXME: also needed here
#	white-xifish.png 

built_docs = Vorbis_I_spec.html Vorbis_I_spec.pdf

# conditionally make the generated documentation
if BUILD_DOCS
doc_DATA = $(static_docs) $(SPEC_PNG) $(built_docs)
else
doc_DATA = $(static_docs) $(SPEC_PNG)
endif

EXTRA_DIST = $(static_docs) $(doc_sources) \
	$(SPEC_PNG) $(SPEC_PDF) $(built_docs)

doc_sources = xml/Vorbis_I_spec.xml \
	xml/01-introduction.xml \
	xml/02-bitpacking.xml \
	xml/03-codebook.xml \
	xml/04-codec.xml \
	xml/05-comment.xml \
	xml/06-floor0.xml \
	xml/07-floor1.xml \
	xml/08-residue.xml \
	xml/09-helper.xml \
	xml/10-tables.xml \
	xml/a1-encapsulation_ogg.xml \
	xml/a2-encapsulation_rtp.xml \
	xml/footer.xml \
	xml/spec-common.xsl \
	xml/spec-fo.xsl \
	xml/spec-html.xsl

# these are expensive; only remove if we have to
DISTCLEANFILES = $(built_docs)
CLEANFILES = Vorbis_I_spec.fo spec.aux spec.log spec.out

# rules for temporary build dir for spec
BUILDDIR = build
# how to copy images to build dir
SPEC_PNG_BUILD = $(foreach file, $(SPEC_PNG), $(BUILDDIR)/$(file))
SPEC_PDF_BUILD = $(foreach file, $(SPEC_PDF), $(BUILDDIR)/$(file))
$(BUILDDIR)/%: $(srcdir)/%
	@echo "Copying $< to $@"
	@mkdir -p $(BUILDDIR)
	@cp $< $@

# how to copy the xml to build dir; cheat by only targeting the main one
$(BUILDDIR)/Vorbis_I_spec.xml: $(doc_sources)
	@mkdir -p $(BUILDDIR)
	@for file in $(doc_sources); do cp $(srcdir)/$$file $(BUILDDIR); done
# translate the draft rtp.txt to .xml in the BUILDDIR
$(BUILDDIR)/draft-rtp.xml: draft-ietf-avt-rtp-vorbis-05.txt
	cat $< | sed 's///g' > $@
# cat $< | sed 's///g' | sed 's/</\&lt;/g' | sed 's/>/\&gt;/g' > $@

# explicit rules for generating docs
if BUILD_DOCS
Vorbis_I_spec.html: $(SPEC_PNG_BUILD) $(BUILDDIR)/Vorbis_I_spec.xml
	cd build; xsltproc --xinclude --output Vorbis_I_spec.html spec-html.xsl Vorbis_I_spec.xml && mv $@ ..

Vorbis_I_spec.fo: $(BUILDDIR)/Vorbis_I_spec.xml
	cd build; xsltproc --xinclude --output $@ spec-fo.xsl Vorbis_I_spec.xml && mv $@ ..

# we add the two newlines to pdfxmltex because this entire XML
# toolchain is built from wishful thinking and bonghits and the
# default supplied passivetex config files are buggy.
Vorbis_I_spec.pdf: Vorbis_I_spec.fo $(SPEC_PNG_BUILD) $(SPEC_PDF_BUILD)
	cp $< build/spec.fo # work around a passivetex bug
	-cd build; echo -e -n "\n\n" | pdfxmltex spec.fo
	-cd build; echo -e -n "\n\n" | pdfxmltex spec.fo
	rm build/spec.fo
	mv build/spec.pdf $@
else
Vorbis_I_spec.html: NO_DOCS_ERROR
Vorbis_I_spec.pdf: NO_DOCS_ERROR
NO_DOCS_ERROR:
	@echo
	@echo "*** Documentation has not been built! ***"
	@echo "Try re-running after passing --enable-docs to configure."
	@echo
endif

# clean up build dir
clean-local:
	-$(RM) -r $(BUILDDIR)
