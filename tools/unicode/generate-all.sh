#!/bin/sh

#      Copyright (C) 2013 Team XBMC
#      http://xbmc.org
# 
# This script is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This script is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this script; see the file COPYING.  If not, see
# <http://www.gnu.org/licenses/>.

# Original author: Karlson2k (Evgeny Grin)
 
outputFilename='Utf32Utils-data.cpp'

scriptPath="${0%/*}"

if [ -e "$outputFilename" ]; then
  rm "$outputFilename" || exit 1
fi

cprtYears='2013'
currentYear=$(date +%Y)
[ "$cprtYears" = "$currentYear" ] || cprtYears="$cprtYears-$currentYear"

echo 'Writing .cpp data file header...'
cat << _EOF_ > "$outputFilename" || exit 1 
/*
 *      Copyright (C) $cprtYears Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
 
#include "Utf32Utils.h"
_EOF_

echo 'Starting digits data processing...'
if ! $scriptPath/generate-digitsdata.sh $outputFilename; then
  echo 'Error!'
  exit 1
fi

echo 'Starting folding data processing...'
if ! $scriptPath/generate-folddata.sh $outputFilename; then
  echo 'Error!'
  exit 1
fi

echo
echo 'All done.'
