# auto execute scripts when xbmc starts, place this file in xbmchome\scripts\
# 
# note: - do not execute more than one script at a time which asks for user input!

import xbmc

xbmc.executescript('q:\\scripts\\medusa\\start_medusa.py')

