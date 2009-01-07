------------------------------------------------------------------
This document describes about TiMidity++ skin interface.

 Skin interface is the front-end of TiMidity++ to allow users to
control TiMidity++ with funny GUI. The face of GUI is changable
by using "skin data" of WinAmp / x11amp.


* SETTING UP THE X-SKIN INTERFACE *

 First, choose a skin data to use and place it on some directory.
And set this path to the environment variable "timidity_skin"
 e.g, If your favourite skin data (*.bmp) is placed on 
 ~/.x11amp/Skins/timidity/ :

% setenv timidity_skin ~/.x11amp/Skins/timidity/                 (csh)
$ timidity_skin=~/.x11amp/Skins/timidity ; export timidity_skin  (sh)

 If your skin data archive file is on /dos/programs/winamp/skins/winamp.zip: 

% setenv timidity_skin /dos/programs/winamp/skins/winamp.zip
$ timidity_skin=/dos/programs/winamp/skins/winamp.zip ; export timidity_skin

 Only one skin can be specified. There are no selector of skin data.
 Since default skin is not prepared, if the environment variable isn't 
specified or the data specified is broken, TiMidity++ won't boot.

 This skin interface requires 10 BMP files for skin data such that: 

 main, titlebar, playpaus, cbuttons, monoster,
 posbar, shufrep, text, volume, number

 If your data losts some of these files, please borrow the corresponding
data from other skin data.
 In case the data has viscolor.txt, the color of spectrum analyzer is
determined by this file. If not, the default colors are used.


* BOOTING TiMidity++ *

 To use skin interface, boot TiMidity++ with command-line option -ii(tv).
After initializing of internal of TiMidity++, a window looks like WinAmp :)
will raise. 


* USAGE OF INTERFACE *

 Features of this interface now available are:

  Prev/Play/Pause/Stop/Next/ Shuffle/Repeat/ Volume/ Exit 

with every corresponding buttons.
 Clicking the area of displaying elapsed-time changes the contents of
this area as elapsed-time / remained-time.
 Clicking the area below the time displaying area changes the contents of
this area as spectrum analyzer. If you want to display the spectrum analyzer,
you should compile TiMidity++ with configure option --enable-spectrogram.

 You can move interface's window by grabbing any place of this window.

 To exit this interface, click the little button on right corner.


* BUG? *

 Text displaying area can display only English, digits and some graphical
characters. Any of multi-byte character won't displayed.
 Cannot set panpot, Equalizer and PlayList of TiMidity++.
 There no effect by clicking the eject button.
 Cannot iconize, smalling.


 YOU CAN USE THIS INTERFACE AT YOUR OWN RISK.

                              Daisuke Nagano <breeze_geo@geocities.co.jp>
                      http://www.geocities.co.jp/Playtown/4370/index.html
