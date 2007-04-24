/***************************************************************************
                         AudioDrv.h  -  Just include all the drivers
                            -------------------
   begin                : Thu Jul 20 2000
   copyright            : (C) 2000 by Simon White
   email                : s_a_white@email.com
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/ 
/***************************************************************************
 *  $Log$
 *  Revision 1.2  2005/03/19 17:33:04  jmarshallnz
 *  Formatting for tabs -> 2 spaces
 *
 *    - 19-03-2005 added: <autodetectFG> tag to XBoxMediaCenter.xml.  Set to false if you have an old bios that causes a crash on autodetection.
 *
 *  Revision 1.1  2004/03/09 00:18:24  butcheruk
 *  Sid playback support
 *
 *  Revision 1.2  2001/01/18 18:34:30  s_a_white
 *  Support for multiple drivers added.  Drivers are now arranged in
 *  preference order.
 *
 *  Revision 1.1  2001/01/08 16:41:43  s_a_white
 *  App and Library Seperation
 *
 *  Revision 1.6  2000/12/18 15:16:42  s_a_white
 *  No hardware support generates only a warning now.  Allows code to default
 *  to wav file generation on any platform.
 *
 *  Revision 1.5  2000/12/11 19:07:14  s_a_white
 *  AC99 Update.
 *
 ***************************************************************************/

#ifndef _AudioDrv_h_
#define _AudioDrv_h_

// Drivers must be put in order of preference
#include "config.h"

// Hardsid Compatibility Driver
#include "null/null.h"

// Unix Sound Drivers
//#include "alsa/audiodrv.h"
//#include "oss/audiodrv.h"
//#include "hpux/audiodrv.h"
//#include "irix/audiodrv.h"
//#include "sunos/audiodrv.h"

// Windows Sound Drivers
//#include "directx/directx.h"
//#include "mmsystem/mmsystem.h"

// XBOX driver
#include "xbox/xbox.h"

// Make sure that a sound driver was used
#ifndef AudioDriver
#   warning Audio hardware not recognised, please check configuration files.
#endif

// Add music conversion drivers
//#include "wav/WavFile.h"

#endif // _AudioDrv_h_
