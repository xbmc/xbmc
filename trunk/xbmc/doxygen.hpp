// Mainpage layout
/*!
	\mainpage Developer documentation for XBMC
	\section intro Introduction
	Introduction to the manual

	\section requirements  Requirements 
	You'll need the following to build XBMC \n
	- Visual Studio.NET 2003
	- XDK 5778 or higher
	- XBMC sources offcourse \n
	  You can download the latest snapshot of XBMC here: \n
	  http://xbmc.sourceforge.net/xbmc.tar.gz  or \n
	  use CVS to get the sources. Look here: http://sourceforge.net/cvs/?group_id=87054 \n

	Next
	  - Install Visual Studio.NET \n
	      When installing VS.NET. Make sure to install all C++ stuff
	  - Install XDK 5778 (or higher) \n
	      Make sure to do a full install of the XDK!!! Minimal or customized install wont work
	  - use winrar or a similar program to uncompress xbmc.tar.gz into a folder \n

	\section compile Compiling
	-# Start Visual Studio.NET and open the xbmc.sln solution file
	-# Next build the solution with Build->Build Solution (Ctrl+Shift+B) \n
	NOTE:  Dont worry about the following warnings which appear at the end of the build. \n
	They are normal and can be ignored \n
	  Creating Xbox Image... \n
	  IMAGEBLD : warning IM1029: library XONLINE is unapproved\n
	  IMAGEBLD : warning IM1030: this image may not be accepted for certification \n
	  Copying files to the Xbox... \n
	-# Then when all is build, its time to install XBMC on your xbox! \n
	(You can also use the build.bat file to make a build of xbmc) \n

	\section install Installation
	-# edit xboxmediacenter.xml and fill in all the details for your installation \n
	After editting xboxmediacenter.xml you need to copy the following files & folder to your xbox \n
	lets say you're installing XBMC on your xbox in the folder e:\\apps\\xbmc then: \n
	-# create e:\\apps\\xbmc on your xbox \n
	-# copy following files from pc->xbox \n

	   PC                       XBOX \n
	-------------------------------------------------------------------------------  \n 
	XboxMediaCenter.xml ->  e:\\apps\\xbmc\\XboxMediaCenter.xml \n
	keymap.xml          ->  e:\\apps\\xbmc\\keymap.xml \n
	FileZilla Server.xml->  e:\\apps\\xbmc\\FileZilla Server.xml \n
	release/default.xbe ->  e:\\apps\\xbmc\\default.xbe \n
	mplayer/            ->  e:\\apps\\xbmc\\mplayer        (copy all files & subdirs) \n
	skin/               ->  e:\\apps\\xbmc\\skin           (copy all files & subdirs) \n
	language/           ->  e:\\apps\\xbmc\\language       (copy all files & subdirs) \n
	weather/            ->  e:\\apps\\xbmc\\weather        (copy all files & subdirs) \n
	xbmc/keyboard/media ->  e:\\apps\\xbmc\\media          (copy all files & subdirs) \n
	visualisations      ->  e:\\apps\\xbmc\\visualisations (copy all files & subdirs) \n
	scripts/            ->  e:\\apps\\xbmc\\scripts        (these are just samples, only extract if you want to experiment with it) \n
	python              ->  e:\\apps\\xbmc\\python         (unpack the .rar file) \n
	web                 -> e:\\apps\\xbmc\\web             (unpack the .rar file) \n

	!!! please not that you unpack the .rar files in scripts/ python/ and web/ !!! \n

	\section links Links
	Related links
	XBMC - http://xbmc.org \n
	XBMC Forum - http://xbmc.org/forum/index.php \n
  */


///////////////////////////////////////
//
// xbmc project
//
///////////////////////////////////////

/*!
	\defgroup windows XBMC windows
	
	XBMC windows
*/

/*!
	\defgroup music Music info
	
	Elements used in my music
*/


//////////////////////////////////////
//
// guilib project
//
//////////////////////////////////////

/*!
	\defgroup guilib guilib classes
	
	guilib classes
*/

/*!
	\defgroup winref Window Reference
	\ingroup guilib
	
	The window reference
*/

/*!
	\defgroup winmsg Windows and Messages
	\ingroup winref
	Windows and messages
*/

/*!
	\defgroup controls Controls
	\ingroup winref
	Control classes
*/

/*!
	\defgroup winman Window Manager and Callbacks
	\ingroup winref
	
	Everything about window manager and callbacks
*/

/*!
	\defgroup actionkeys Actions and Keys
	\ingroup winref
	
	Everything around action mapping and key processing
*/

/*!
	\defgroup graphics Graphics and Screen
	\ingroup guilib
	
	Everything around graphics and Screen
*/

/*!
	\defgroup textures Textures and Fonts
	\ingroup graphics
	
	Everything about textures and fonts
*/

/*!
	\defgroup strings Strings and Localization
	\ingroup guilib
	
	Everything around Strings and localization
*/

/*!
	\defgroup tinyxml XML Parser
	\ingroup strings
	
	Tiny XML - XML Parser
*/

/*!
	\defgroup jobs Asynchronous jobs

	Threaded job execution
*/
