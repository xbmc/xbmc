From: "Peter L Jones" 
Sent: Wednesday, September 17, 2003 5:18 AM
Subject: Dev-C++ project files

I attach two project files intended for portaudio/pa_win/dev-cpp (i.e. in
parallel with the msvc directory), if you want them.  One is for a static
library build and one for a DLL.  I've used the static library (in building
a single monolithic DLL) but I can't guarantee the DLL version will build a
working library (I think it's mostly there, though!).

I also attach the resulting makefiles, which may be of use to other MinGW
users.

They're rooted in the directory given above and drop their object and
library files in the same place.  They assume the asiosdk2 files are in the
same directory as portaudio/ in a sub-directory called asiosdk2/.  Oh!  The
DLL is built against a static asiosdk2.a library... maybe not the best way
to do it...  I ought to figure out how to link against a "home made" dll in
Dev-C++, I guess ;-)

Cheers,

-- Peter
