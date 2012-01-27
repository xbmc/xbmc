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
#ifndef __AUDIO_RENDERER_FACTORY_H__
#define __AUDIO_RENDERER_FACTORY_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "IAudioRenderer.h"
#include "cores/IAudioCallback.h"

class CAudioRendererFactory
{
public:
  static IAudioRenderer *Create(IAudioCallback* pCallback, int iChannels, enum PCMChannels *channelMap, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, bool bResample, bool bIsMusic, IAudioRenderer::EEncoded encoded = IAudioRenderer::ENCODED_NONE);
  static void EnumerateAudioSinks(AudioSinkList& vAudioSinks, bool passthrough);
private:
  static IAudioRenderer *CreateFromUri(const CStdString &soundsystem, CStdString &renderer);
};
#endif
