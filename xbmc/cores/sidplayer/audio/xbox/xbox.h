/***************************************************************************
                         audiodrv.h    -  ``DirectSound for Xbox''
                                          specific audio driver interface.
                            -------------------
   begin                : Mon Feb 18 2004
   copyright            : (C) 2004 by Richard Crockford
   email                : 
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef audio_xbox_h_
#define audio_xbox_h_

#include "../config.h"
#ifdef   HAVE_DIRECTX
#ifdef _XBOX

#ifndef AudioDriver
#define AudioDriver Audio_Xbox
#endif

#include <xtl.h>

#include "../AudioBase.h" 
// 0.5s per buffer
#define AUDIO_XBOX_BUFFERS (4)

class Audio_Xbox: public AudioBase
{
private:   // ------------------------------------------------------- private
  // DirectSound Support
  LPDIRECTSOUND pDS;
  LPDIRECTSOUNDSTREAM pStream;
  XMEDIAPACKET pMPacket[AUDIO_XBOX_BUFFERS];
  DWORD dwStreamed[AUDIO_XBOX_BUFFERS];
  int BufIdx;

  bool isOpen;
  bool isPlaying;

public:   // --------------------------------------------------------- public
  Audio_Xbox();
  ~Audio_Xbox();

  void *open (AudioConfig &cfg, const char *name);
  void close ();
  void *reset ();
  void *write ();
  void pause ();
  void Eof();
  void SetVolume(long nVolume);
};

#endif // _XBOX
#endif // HAVE_DIRECTX
#endif // audio_xbox_h_
