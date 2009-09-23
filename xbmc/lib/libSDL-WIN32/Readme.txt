Building SDL 1.2 for XBMC
=========================

1. Grab the SDL 1.2 sources by checking out from SVN: http://svn.libsdl.org/branches/SDL-1.2
2. Apply the patch SDL_SetWidthHeight.diff
3. Load up the project file using the normal instructions
4. Set the Code Generation->Runtime Library to Multi-threaded (MT).
5. Build the dll and .lib's in release mode.

Additional libraries are available here:

SDL_image and SDL_mixer from: http://libsdl.org/download-1.2.php

GLEW: http://downloads.sourceforge.net/glew/glew-1.5.0-win32.zip?modtime=1198796014&big_mirror=0

SDL rotozoom: http://www.ferzkopp.net/Software/SDL_rotozoom/SDL_rotozoom-1.6.tar.gz