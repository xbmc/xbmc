#ifndef _PORT_AUDIO_H_
#define _PORT_AUDIO_H_

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <portaudio.h>
#include <vector>
#include "StdString.h"

#define SAFELY(op)     \
{                      \
    int err;           \
    if ((err=op) != 0) \
       CLog::Log(LOGERROR, "[PortAudio] ERROR[%s:%d]: %s.", __FILE__, __LINE__, Pa_GetErrorText(err)); \
}

class CPortAudio
{
 public:

  //
  // Get a list of output devices.
  //
  static std::vector<PaDeviceInfo*> GetDeviceList(bool includeOutput=true, bool includeInput=false);

  //
  // Create an output stream.
  //
  static PaStream* CreateOutputStream(const CStdString& strName, int channels, int sampleRate, int bitsPerSample, bool isDigital, int packetSize);
};

#endif
