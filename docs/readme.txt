Building XBMC:
--------------

You'll need the following to build XBMC
- Visual Studio.NET 2003
- XDK 5669 or higher
- XBMC sources offcourse

Start Visual Studio.NET and open the xbmc.sln solution file
Look @ the toolbar. There should b a dropdown box called 'Solution Configurations'
It shows this when you hover over it with the mouse. In it it says either
'Debug Xbox' or 'Release Xbox'.
If doesnt say 'Release Xbox' then change it!

Next build the solution with Build->Build Solution
Then when all is build, its time to install XBMC on your xbox!



Installing XBMC:
-----------------

After building XBMC copy the following files & directories to your xbox

lets say you're installing XBMC on e:\apps\xbmc then:

release/default.xbe	-> 	e:\apps\xbmc\default.xbe
XboxMediaCenter.xml	->	e:\apps\xbmc\XboxMediaCenter.xml
mplayer/		->	e:\apps\xbmc\mplayer	(copy all files & subdirs)
skin/			->	e:\apps\xbmc\skin	(copy all files & subdirs)
language/               ->      e:\apps\xbmc\language	(copy all files & subdirs)
xbmc/keyboard/media	->	e:\apps\xbmc\media	(copy all files & subdirs)
scripts/		->	e:\apps\xbmc\scripts	(unpack the .rar file)
python			->	e:\apps\xbmc\python  	(unpack the .rar file)

!!! please not that you unpack the .rar files in scripts/ and python/ !!!