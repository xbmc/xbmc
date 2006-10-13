This tool depends on xbmemdump, so make sure it works properly and it is added to your PATH environment
Test this by starting cmd.exe and type 'xbmemdump'

Next enable memory tracking with the command 'xbmemdump -e 32', this has to be done before the xbox title is started.
Because memory tracking does cost a lot of xbox resources do not forget to turn it off after using it ('xbmemdump -d')
Restart the xbox title and start the tool after the titel is started.

Do not forget to use 'xbmemdump -d' to disable memory tracking after using the tool.!!

Usage:

	- Click new in the snapshot window
	- Double click the row newly added item in that window to use it
	- In the main window select the Tree tab, for now this is the only usable tab
	- If in this tab only address show up and no function names, you have to check settings.xml for correct pdf files
	  if you believe these paths are correct, clean your xbox title build and rebuild it to generate a new pdb file.
