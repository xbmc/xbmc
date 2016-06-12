#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "filesystem/File.h"
#include "music/tags/MusicInfoTag.h"

#include "cores/AudioEngine/Utils/AEAudioFormat.h"

#include <string>

#define READ_EOF      -1
#define READ_SUCCESS   0
#define READ_ERROR     1

class CFileItem;

class ICodec
{
public:
  ICodec()
  {
    m_TotalTime = 0;
    m_bitRate = 0;
    m_bitsPerSample = 0;
    m_bitsPerCodedSample = 0;
  };
  virtual ~ICodec() {};

  // Virtual functions that all codecs should implement.  Note that these may need
  // enhancing and or refactoring at a later date.  It works currently well for MP3 and
  // APE codecs.

  // Init(filename)
  // This routine should handle any initialization necessary.  At a minimum it needs to:
  // 1.  Load any dlls and make sure any buffers etc. are allocated.
  // 2.  Load the file (or at least attempt to load it)
  // 3.  Fill in the m_TotalTime, m_SampleRate, m_BitsPerSample and m_Channels parameters.
  virtual bool Init(const CFileItem &file, unsigned int filecache)=0;

  // DeInit()
  // Should just cleanup anything as necessary.  No need to free buffers here if they
  // are allocated and destroyed in the destructor.
  virtual void DeInit()=0;

  virtual bool CanSeek() {return true;}

  // Seek()
  // Should seek to the appropriate time (in ms) in the file, and return the
  // time to which we managed to seek (in the case where seeking is problematic)
  // This is used in FFwd/Rewd so can be called very often.
  virtual bool Seek(int64_t iSeekTime)=0;

  // ReadPCM()
  // Decodes audio into pBuffer up to size bytes.  The actual amount of returned data
  // is given in actualsize.  Returns READ_SUCCESS on success.  Returns READ_EOF when
  // the data has been exhausted, and READ_ERROR on error.
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize)=0;

  virtual int ReadRaw(uint8_t **pBuffer, int *bufferSize) { return READ_ERROR; }

  // CanInit()
  // Should return true if the codec can be initialized
  // eg. check if a dll needed for the codec exists
  virtual bool CanInit()=0;

  // SkipNext()
  // Skip to next track/item inside the current media (if supported).
  virtual bool SkipNext(){return false;}

  // set the total time - useful when info comes from a preset tag
  virtual void SetTotalTime(int64_t totaltime) {}

  virtual bool IsCaching()    const    {return false;}
  virtual int GetCacheLevel() const    {return -1;}

  int64_t m_TotalTime;  // time in ms
  int m_bitRate;
  int m_bitsPerSample;
  int m_bitsPerCodedSample;
  std::string m_CodecName;
  MUSIC_INFO::CMusicInfoTag m_tag;
  XFILE::CFile m_file;
  AEAudioFormat m_format;
};

