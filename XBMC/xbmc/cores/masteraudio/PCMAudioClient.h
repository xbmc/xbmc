/*
 *      Copyright (C) 2009 phi2039
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

#ifndef __PCM_AUDIO_CLIENT_H__
#define __PCM_AUDIO_CLIENT_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "AudioManagerClient.h"

class CPCMAudioClient : public CAudioManagerClient
{
public:
  CPCMAudioClient(CAudioManager* pManager) : CAudioManagerClient(pManager){};
  bool OpenStream(size_t blockSize, int channels, int bitsPerSample, int samplesPerSecond);
protected:
};

#endif __PCM_AUDIO_CLIENT_H__