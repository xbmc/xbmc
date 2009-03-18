#!/usr/bin/env python
"""
  Set output volume for all amixer controls
  V0.991 - 20090317
  Luigi Capriotti @2009
""" 

import sys, subprocess

gVolumeLevel = "100"

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
			output = runSilent("amixer sget " + aMixer[nameStart:])
			if output.find("pvolume") > 0:
				output = runSilent("amixer sset " + aMixer[nameStart:] + " " + gVolumeLevel + "% unmute")
	# runSilent("alsactl store")
