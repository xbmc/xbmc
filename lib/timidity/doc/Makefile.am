# TiMidity++ -- MIDI to WAVE converter and player
# Copyright (C) 1999-2004 Masanao Izumo <iz@onicos.co.jp>
# Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

include $(top_srcdir)/common.makefile.in
SUFFIXES = .1 .5 .txt .ps

SUBDIRS = C ja_JP.eucJP
lang_target = C

man_MANS = \
	timidity.1 \
	timidity.cfg.5

EXTRA_DIST =

.1.txt:
	groff -man -Tlatin1 -P-b -P-u $< >$@
.1.ps:
	groff -man $< >$@
.5.txt:
	groff -man -Tlatin1 -P-b -P-u $< >$@
.5.ps:
	groff -man $< >$@

timidity.1:
	rm -f timidity.1; $(LN_S) $(srcdir)/$(lang_target)/timidity.1 .

timidity.cfg.5:
	rm -f timidity.cfg.5; $(LN_S) $(srcdir)/$(lang_target)/timidity.cfg.5 .

remake-mans:
	rm -f timidity.1; $(LN_S) $(srcdir)/$(lang_target)/timidity.1 .
	rm -f timidity.cfg.5; $(LN_S) $(srcdir)/$(lang_target)/timidity.cfg.5 .

install.man:
	$(MAKE) remake-mans lang_target=C; $(MAKE) install-man

install.manj:
	$(MAKE) remake-mans lang_target=ja_JP.eucJP; $(MAKE) install-man

clean:
	rm -f timidity.1
	rm -f timidity.cfg.5
