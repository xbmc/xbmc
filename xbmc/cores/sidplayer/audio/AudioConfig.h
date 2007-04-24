/***************************************************************************
                         AudioConfig.h  -  description
                            -------------------
   begin                : Sat Jul 8 2000
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
 *  Revision 1.3  2002/01/10 22:03:27  s_a_white
 *  Namespace not used yet (enable later).
 *
 *  Revision 1.2  2002/01/10 18:58:17  s_a_white
 *  Interface changes for 2.0.8.
 *
 *  Revision 1.1  2001/01/08 16:41:43  s_a_white
 *  App and Library Seperation
 *
 *  Revision 1.3  2000/12/11 19:07:14  s_a_white
 *  AC99 Update.
 *
 ***************************************************************************/

#ifndef _AudioConfig_h_
#define _AudioConfig_h_

#include <sidplay/sidtypes.h> 
//typedef SID2::uint uint;
#define FOREVER SID_FOREVER
#define SWAP    SID_SWAP 
/*
#ifdef SIDPLAY2_NAMESPACE
    using namespace SIDPLAY2_NAMESPACE;
#endif
*/ 
// Configuration constants.
enum
{
  AUDIO_SIGNED_PCM = 0x7f,
  AUDIO_UNSIGNED_PCM = 0x80
};


class AudioConfig
{
public:
  uint_least32_t frequency;
  int precision;
  int channels;
  int encoding;
  uint_least32_t bufSize;       // sample buffer size

  AudioConfig()
  {
    frequency = 22050;
    precision = 8;
    channels = 1;
    encoding = AUDIO_UNSIGNED_PCM;
    bufSize = 0;
  }
};

#endif  // _AudioConfig_h_
