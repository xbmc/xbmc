#!/bin/sh

# Copyright (C) 2004 Erik de Castro Lopo <erikd@mega-nerd.com>
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
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.

#=======================================================================
# This short test script compiles the file src_sinc.c into assembler
# output and the greps the assembler output for the fldcw (FPU Load
# Control Word) instruction which is very detrimental to the perfromance
# of floating point code.

echo -n "    Checking assembler output for bad code : "

if [ $# -ne 1 ]; then
	echo "Error : Need the name of the input file."
	exit 1
	fi

filename=$1

if [ ! -f $filename ];then
	echo "Error : Can't find file $filename."
	exit 1
	fi

count=`grep fldcw $filename | wc -l`

if [ $count -gt 0 ]; then
	echo -e "\n\nError : found $count bad instructions.\n\n"
	exit 1
	fi

echo "ok"

