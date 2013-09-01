#!/bin/sh -e
# gendocs.sh -- generate a GNU manual in many formats.  This script is
#   mentioned in maintain.texi.  See the help message below for usage details.

scriptversion=2010-09-17.07

# Copyright 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010
# Free Software Foundation, Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Original author: Mohit Agarwal.
# Send bug reports and any other correspondence to bug-texinfo@gnu.org.

prog=`basename "$0"`
srcdir=`pwd`

scripturl="http://savannah.gnu.org/cgi-bin/viewcvs/~checkout~/texinfo/texinfo/util/gendocs.sh"
templateurl="http://savannah.gnu.org/cgi-bin/viewcvs/~checkout~/texinfo/texinfo/util/gendocs_template"

: ${SETLANG="env LANG= LC_MESSAGES= LC_ALL= LANGUAGE="}
: ${MAKEINFO="makeinfo"}
: ${TEXI2DVI="texi2dvi -t @finalout"}
: ${DVIPS="dvips"}
: ${DOCBOOK2HTML="docbook2html"}
: ${DOCBOOK2PDF="docbook2pdf"}
: ${DOCBOOK2PS="docbook2ps"}
: ${DOCBOOK2TXT="docbook2txt"}
: ${GENDOCS_TEMPLATE_DIR="."}
: ${TEXI2HTML="texi2html"}
unset CDPATH
unset use_texi2html

version="gendocs.sh $scriptversion

Copyright 2010 Free Software Foundation, Inc.
There is NO warranty.  You may redistribute this software
under the terms of the GNU General Public License.
For more information about these matters, see the files named COPYING."

usage="Usage: $prog [OPTION]... PACKAGE MANUAL-TITLE

Generate various output formats from PACKAGE.texinfo (or .texi or .txi) source.
See the GNU Maintainers document for a more extensive discussion:
  http://www.gnu.org/prep/maintain_toc.html

Options:
  -s SRCFILE  read Texinfo from SRCFILE, instead of PACKAGE.{texinfo|texi|txi}
  -o OUTDIR   write files into OUTDIR, instead of manual/.
  --email ADR use ADR as contact in generated web pages.
  --docbook   convert to DocBook too (xml, txt, html, pdf and ps).
  --html ARG  pass indicated ARG to makeinfo or texi2html for HTML targets.
  --texi2html use texi2html to generate HTML targets.
  --help      display this help and exit successfully.
  --version   display version information and exit successfully.

Simple example: $prog --email bug-gnu-emacs@gnu.org emacs \"GNU Emacs Manual\"

Typical sequence:
  cd PACKAGESOURCE/doc
  wget \"$scripturl\"
  wget \"$templateurl\"
  $prog --email BUGLIST MANUAL \"GNU MANUAL - One-line description\"

Output will be in a new subdirectory \"manual\" (by default, use -o OUTDIR
to override).  Move all the new files into your web CVS tree, as
explained in the Web Pages node of maintain.texi.

Please use the --email ADDRESS option to specify your bug-reporting
address in the generated HTML pages.

MANUAL-TITLE is included as part of the HTML <title> of the overall
manual/index.html file.  It should include the name of the package being
documented.  manual/index.html is created by substitution from the file
$GENDOCS_TEMPLATE_DIR/gendocs_template.  (Feel free to modify the
generic template for your own purposes.)

If you have several manuals, you'll need to run this script several
times with different MANUAL values, specifying a different output
directory with -o each time.  Then write (by hand) an overall index.html
with links to them all.

If a manual's Texinfo sources are spread across several directories,
first copy or symlink all Texinfo sources into a single directory.
(Part of the script's work is to make a tar.gz of the sources.)

You can set the environment variables MAKEINFO, TEXI2DVI, TEXI2HTML, and
DVIPS to control the programs that get executed, and
GENDOCS_TEMPLATE_DIR to control where the gendocs_template file is
looked for.  With --docbook, the environment variables DOCBOOK2HTML,
DOCBOOK2PDF, DOCBOOK2PS, and DOCBOOK2TXT are also respected.

By default, makeinfo and texi2dvi are run in the default (English)
locale, since that's the language of most Texinfo manuals.  If you
happen to have a non-English manual and non-English web site, see the
SETLANG setting in the source.

Email bug reports or enhancement requests to bug-texinfo@gnu.org.
"

calcsize()
{
  size=`ls -ksl $1 | awk '{print $1}'`
  echo $size
}

MANUAL_TITLE=
PACKAGE=
EMAIL=webmasters@gnu.org  # please override with --email
htmlarg=
outdir=manual
srcfile=

while test $# -gt 0; do
  case $1 in
    --email) shift; EMAIL=$1;;
    --help) echo "$usage"; exit 0;;
    --version) echo "$version"; exit 0;;
    -s) shift; srcfile=$1;;
    -o) shift; outdir=$1;;
    --docbook) docbook=yes;;
    --html) shift; htmlarg=$1;;
    --texi2html) use_texi2html=1;;
    -*)
      echo "$0: Unknown option \`$1'." >&2
      echo "$0: Try \`--help' for more information." >&2
      exit 1;;
    *)
      if test -z "$PACKAGE"; then
        PACKAGE=$1
      elif test -z "$MANUAL_TITLE"; then
        MANUAL_TITLE=$1
      else
        echo "$0: extra non-option argument \`$1'." >&2
        exit 1
      fi;;
  esac
  shift
done

# For most of the following, the base name is just $PACKAGE
base=$PACKAGE

if test -n "$srcfile"; then
  # but here, we use the basename of $srcfile
  base=`basename "$srcfile"`
  case $base in
    *.txi|*.texi|*.texinfo) base=`echo "$base"|sed 's/\.[texinfo]*$//'`;;
  esac
  PACKAGE=$base
elif test -s "$srcdir/$PACKAGE.texinfo"; then
  srcfile=$srcdir/$PACKAGE.texinfo
elif test -s "$srcdir/$PACKAGE.texi"; then
  srcfile=$srcdir/$PACKAGE.texi
elif test -s "$srcdir/$PACKAGE.txi"; then
  srcfile=$srcdir/$PACKAGE.txi
else
  echo "$0: cannot find .texinfo or .texi or .txi for $PACKAGE in $srcdir." >&2
  exit 1
fi

if test ! -r $GENDOCS_TEMPLATE_DIR/gendocs_template; then
  echo "$0: cannot read $GENDOCS_TEMPLATE_DIR/gendocs_template." >&2
  echo "$0: it is available from $templateurl." >&2
  exit 1
fi

case $outdir in
  /*) abs_outdir=$outdir;;
  *)  abs_outdir=$srcdir/$outdir;;
esac

echo Generating output formats for $srcfile

cmd="$SETLANG $MAKEINFO -o $PACKAGE.info \"$srcfile\""
echo "Generating info files... ($cmd)"
eval "$cmd"
mkdir -p "$outdir/"
tar czf "$outdir/$PACKAGE.info.tar.gz" $PACKAGE.info*
info_tgz_size=`calcsize "$outdir/$PACKAGE.info.tar.gz"`
# do not mv the info files, there's no point in having them available
# separately on the web.

cmd="$SETLANG ${TEXI2DVI} \"$srcfile\""
echo "Generating dvi ... ($cmd)"
eval "$cmd"

# now, before we compress dvi:
echo Generating postscript...
${DVIPS} $PACKAGE -o
gzip -f -9 $PACKAGE.ps
ps_gz_size=`calcsize $PACKAGE.ps.gz`
mv $PACKAGE.ps.gz "$outdir/"

# compress/finish dvi:
gzip -f -9 $PACKAGE.dvi
dvi_gz_size=`calcsize $PACKAGE.dvi.gz`
mv $PACKAGE.dvi.gz "$outdir/"

cmd="$SETLANG ${TEXI2DVI} --pdf \"$srcfile\""
echo "Generating pdf ... ($cmd)"
eval "$cmd"
pdf_size=`calcsize $PACKAGE.pdf`
mv $PACKAGE.pdf "$outdir/"

cmd="$SETLANG $MAKEINFO -o $PACKAGE.txt --no-split --no-headers \"$srcfile\""
echo "Generating ASCII... ($cmd)"
eval "$cmd"
ascii_size=`calcsize $PACKAGE.txt`
gzip -f -9 -c $PACKAGE.txt >"$outdir/$PACKAGE.txt.gz"
ascii_gz_size=`calcsize "$outdir/$PACKAGE.txt.gz"`
mv $PACKAGE.txt "$outdir/"

html_split()
{
  opt="--split=$1 $htmlarg --node-files"
  cmd="$SETLANG $TEXI2HTML --output $PACKAGE.html $opt \"$srcfile\""
  echo "Generating html by $1... ($cmd)"
  eval "$cmd"
  split_html_dir=$PACKAGE.html
  (
    cd ${split_html_dir} || exit 1
    ln -sf ${PACKAGE}.html index.html
    tar -czf "$abs_outdir/${PACKAGE}.html_$1.tar.gz" -- *.html
  )
  eval html_$1_tgz_size=`calcsize "$outdir/${PACKAGE}.html_$1.tar.gz"`
  rm -f "$outdir"/html_$1/*.html
  mkdir -p "$outdir/html_$1/"
  mv ${split_html_dir}/*.html "$outdir/html_$1/"
  rmdir ${split_html_dir}
}

if test -z "$use_texi2html"; then
  opt="--no-split --html -o $PACKAGE.html $htmlarg"
  cmd="$SETLANG $MAKEINFO $opt \"$srcfile\""
  echo "Generating monolithic html... ($cmd)"
  rm -rf $PACKAGE.html  # in case a directory is left over
  eval "$cmd"
  html_mono_size=`calcsize $PACKAGE.html`
  gzip -f -9 -c $PACKAGE.html >"$outdir/$PACKAGE.html.gz"
  html_mono_gz_size=`calcsize "$outdir/$PACKAGE.html.gz"`
  mv $PACKAGE.html "$outdir/"

  cmd="$SETLANG $MAKEINFO --html -o $PACKAGE.html $htmlarg \"$srcfile\""
  echo "Generating html by node... ($cmd)"
  eval "$cmd"
  split_html_dir=$PACKAGE.html
  (
   cd ${split_html_dir} || exit 1
   tar -czf "$abs_outdir/${PACKAGE}.html_node.tar.gz" -- *.html
  )
  html_node_tgz_size=`calcsize "$outdir/${PACKAGE}.html_node.tar.gz"`
  rm -f "$outdir"/html_node/*.html
  mkdir -p "$outdir/html_node/"
  mv ${split_html_dir}/*.html "$outdir/html_node/"
  rmdir ${split_html_dir}
else
  cmd="$SETLANG $TEXI2HTML --output $PACKAGE.html $htmlarg \"$srcfile\""
  echo "Generating monolithic html... ($cmd)"
  rm -rf $PACKAGE.html  # in case a directory is left over
  eval "$cmd"
  html_mono_size=`calcsize $PACKAGE.html`
  gzip -f -9 -c $PACKAGE.html >"$outdir/$PACKAGE.html.gz"
  html_mono_gz_size=`calcsize "$outdir/$PACKAGE.html.gz"`
  mv $PACKAGE.html "$outdir/"

  html_split node
  html_split chapter
  html_split section
fi

echo Making .tar.gz for sources...
d=`dirname $srcfile`
(
  cd "$d"
  srcfiles=`ls *.texinfo *.texi *.txi *.eps 2>/dev/null` || true
  tar cvzfh "$abs_outdir/$PACKAGE.texi.tar.gz" $srcfiles
)
texi_tgz_size=`calcsize "$outdir/$PACKAGE.texi.tar.gz"`

if test -n "$docbook"; then
  cmd="$SETLANG $MAKEINFO -o - --docbook \"$srcfile\" > ${srcdir}/$PACKAGE-db.xml"
  echo "Generating docbook XML... ($cmd)"
  eval "$cmd"
  docbook_xml_size=`calcsize $PACKAGE-db.xml`
  gzip -f -9 -c $PACKAGE-db.xml >"$outdir/$PACKAGE-db.xml.gz"
  docbook_xml_gz_size=`calcsize "$outdir/$PACKAGE-db.xml.gz"`
  mv $PACKAGE-db.xml "$outdir/"

  cmd="${DOCBOOK2HTML} -o $split_html_db_dir \"${outdir}/$PACKAGE-db.xml\""
  echo "Generating docbook HTML... ($cmd)"
  eval "$cmd"
  split_html_db_dir=html_node_db
  (
    cd ${split_html_db_dir} || exit 1
    tar -czf "$abs_outdir/${PACKAGE}.html_node_db.tar.gz" -- *.html
  )
  html_node_db_tgz_size=`calcsize "$outdir/${PACKAGE}.html_node_db.tar.gz"`
  rm -f "$outdir"/html_node_db/*.html
  mkdir -p "$outdir/html_node_db"
  mv ${split_html_db_dir}/*.html "$outdir/html_node_db/"
  rmdir ${split_html_db_dir}

  cmd="${DOCBOOK2TXT} \"${outdir}/$PACKAGE-db.xml\""
  echo "Generating docbook ASCII... ($cmd)"
  eval "$cmd"
  docbook_ascii_size=`calcsize $PACKAGE-db.txt`
  mv $PACKAGE-db.txt "$outdir/"

  cmd="${DOCBOOK2PS} \"${outdir}/$PACKAGE-db.xml\""
  echo "Generating docbook PS... ($cmd)"
  eval "$cmd"
  gzip -f -9 -c $PACKAGE-db.ps >"$outdir/$PACKAGE-db.ps.gz"
  docbook_ps_gz_size=`calcsize "$outdir/$PACKAGE-db.ps.gz"`
  mv $PACKAGE-db.ps "$outdir/"

  cmd="${DOCBOOK2PDF} \"${outdir}/$PACKAGE-db.xml\""
  echo "Generating docbook PDF... ($cmd)"
  eval "$cmd"
  docbook_pdf_size=`calcsize $PACKAGE-db.pdf`
  mv $PACKAGE-db.pdf "$outdir/"
fi

echo "Writing index file..."
if test -z "$use_texi2html"; then
   CONDS="/%%IF  *HTML_SECTION%%/,/%%ENDIF  *HTML_SECTION%%/d;\
          /%%IF  *HTML_CHAPTER%%/,/%%ENDIF  *HTML_CHAPTER%%/d"
else
   CONDS="/%%ENDIF.*%%/d;/%%IF  *HTML_SECTION%%/d;/%%IF  *HTML_CHAPTER%%/d"
fi
curdate=`$SETLANG date '+%B %d, %Y'`
sed \
   -e "s!%%TITLE%%!$MANUAL_TITLE!g" \
   -e "s!%%EMAIL%%!$EMAIL!g" \
   -e "s!%%PACKAGE%%!$PACKAGE!g" \
   -e "s!%%DATE%%!$curdate!g" \
   -e "s!%%HTML_MONO_SIZE%%!$html_mono_size!g" \
   -e "s!%%HTML_MONO_GZ_SIZE%%!$html_mono_gz_size!g" \
   -e "s!%%HTML_NODE_TGZ_SIZE%%!$html_node_tgz_size!g" \
   -e "s!%%HTML_SECTION_TGZ_SIZE%%!$html_section_tgz_size!g" \
   -e "s!%%HTML_CHAPTER_TGZ_SIZE%%!$html_chapter_tgz_size!g" \
   -e "s!%%INFO_TGZ_SIZE%%!$info_tgz_size!g" \
   -e "s!%%DVI_GZ_SIZE%%!$dvi_gz_size!g" \
   -e "s!%%PDF_SIZE%%!$pdf_size!g" \
   -e "s!%%PS_GZ_SIZE%%!$ps_gz_size!g" \
   -e "s!%%ASCII_SIZE%%!$ascii_size!g" \
   -e "s!%%ASCII_GZ_SIZE%%!$ascii_gz_size!g" \
   -e "s!%%TEXI_TGZ_SIZE%%!$texi_tgz_size!g" \
   -e "s!%%DOCBOOK_HTML_NODE_TGZ_SIZE%%!$html_node_db_tgz_size!g" \
   -e "s!%%DOCBOOK_ASCII_SIZE%%!$docbook_ascii_size!g" \
   -e "s!%%DOCBOOK_PS_GZ_SIZE%%!$docbook_ps_gz_size!g" \
   -e "s!%%DOCBOOK_PDF_SIZE%%!$docbook_pdf_size!g" \
   -e "s!%%DOCBOOK_XML_SIZE%%!$docbook_xml_size!g" \
   -e "s!%%DOCBOOK_XML_GZ_SIZE%%!$docbook_xml_gz_size!g" \
   -e "s,%%SCRIPTURL%%,$scripturl,g" \
   -e "s!%%SCRIPTNAME%%!$prog!g" \
   -e "$CONDS" \
$GENDOCS_TEMPLATE_DIR/gendocs_template >"$outdir/index.html"

echo "Done, see $outdir/ subdirectory for new files."

# Local variables:
# eval: (add-hook 'write-file-hooks 'time-stamp)
# time-stamp-start: "scriptversion="
# time-stamp-format: "%:y-%02m-%02d.%02H"
# time-stamp-end: "$"
# End:
