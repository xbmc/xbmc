#! /usr/bin/perl -w
#   ---------------------------------------------------------------------------
#   This file is part of reSID, a MOS6581 SID emulator engine.
#   Copyright (C) 2004  Dag Lem <resid@nimrod.no>
# 
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
# 
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
# 
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#   ---------------------------------------------------------------------------

use strict;

die("Usage: samp2src name data-in src-out\n") unless @ARGV == 3;
my ($name, $in, $out) = @ARGV;

open(F, "<$in") or die($!);
local $/ = undef;
my $data = <F>;
close(F) or die($!);

open(F, ">$out") or die($!);

print F <<\EOF;
//  ---------------------------------------------------------------------------
//  This file is part of reSID, a MOS6581 SID emulator engine.
//  Copyright (C) 2004  Dag Lem <resid@nimrod.no>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//  ---------------------------------------------------------------------------

EOF

print F "#include \"wave.h\"\n\nRESID_NAMESPACE_START\n\nreg8 WaveformGenerator::$name\[\] =\n{\n";

for (my $i = 0; $i < length($data); $i += 8) {
  print F sprintf("/* 0x%03x: */ ", $i), map(sprintf(" 0x%02x,", $_), unpack("C*", substr($data, $i, 8))), "\n";
}

print F "};\n\nRESID_NAMESPACE_STOP\n";

close(F) or die($!);

exit(0);
