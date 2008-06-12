#!/bin/sh
grep '{".*P_[GL]' param/loadparm.c | sed -e 's/&.*$//g' -e 's/",.*P_LOCAL.*$/  S/' -e 's/",.*P_GLOBAL.*$/  G/' -e 's/^ .*{"//g' | sort -f
