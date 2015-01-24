#!/usr/bin/env python
"""
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
""" 

import sys, subprocess

gVolumeLevel = "90"

def runSilent(aCmdline):
	process = subprocess.Popen(aCmdline, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE)
	stdout_value, stderr_value = process.communicate()
	return stdout_value

if __name__ == '__main__':
	if len(sys.argv) > 1:
		gVolumeLevel = sys.argv[1]

	print "Setting volumes at " + gVolumeLevel + "%"

	mixerList = runSilent("amixer scontrols")

	arMixers = mixerList.split('\n')	
	for aMixer in arMixers:
		nameStart = aMixer.find("'")
		if nameStart>0:
			# print "Mixer name=" + aMixer[nameStart:]
			if "Mic" in aMixer[nameStart:]:
				output = runSilent("amixer sset " + aMixer[nameStart:] + " mute")
			else:
				output = runSilent("amixer sget " + aMixer[nameStart:])
				if output.find("pvolume") > 0:
					 output = runSilent("amixer sset " + aMixer[nameStart:] + " " + gVolumeLevel + "% unmute")
				if output.find("pswitch") > 0:
					  output = runSilent("amixer sset " + aMixer[nameStart:] + " unmute")
	runSilent("amixer sset Capture nocap")
