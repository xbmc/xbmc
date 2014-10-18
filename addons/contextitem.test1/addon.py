

#
# Imports
#
import os
import sys
 

xbmc.executebuiltin("Notification(Test 1, "+sys.listitem.getLabel()+")")
sys.listitem.setLabel("Shouldn't update gui")