#pragma once
/*
 *      Copyright (C) 2016 Chris Browet
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <stdexcept>

#include "JNIBase.h"
#include "ByteBuffer.h"
#include "MediaTimestamp.h"
#include "PlaybackParams.h"
#include "AudioTrack.h"

namespace jni
{

class CJNIMediaSync : public CJNIBase
{
public:
  static int  MEDIASYNC_ERROR_AUDIOTRACK_FAIL;
  static int  MEDIASYNC_ERROR_SURFACE_FAIL;
  static void PopulateStaticFields();

  CJNIMediaSync();
  void flush();
  void release();

  CJNIMediaTimestamp getTimestamp();

  void setAudioTrack(const CJNIAudioTrack& audioTrack);
  void queueAudio(uint8_t* audioData, int sizeInBytes, int bufferId, int64_t presentationTimeUs);
  void queueAudio(const CJNIByteBuffer& audioData, int bufferId, int64_t presentationTimeUs);

  CJNIPlaybackParams getPlaybackParams();
  void setPlaybackParams(const CJNIPlaybackParams& params);

protected:
};

}

