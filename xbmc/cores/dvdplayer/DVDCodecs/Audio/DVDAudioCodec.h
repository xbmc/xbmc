#pragma once

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

#include <vector>
#ifndef _LINUX

enum CodecID;
#else
extern "C" {
#include "ffmpeg/avcodec.h"
}
#endif

struct AVStream;

class CDVDStreamInfo;
class CDVDCodecOption;
typedef std::vector<CDVDCodecOption> CDVDCodecOptions;

class CDVDAudioCodec
{
public:

  CDVDAudioCodec() {}
  virtual ~CDVDAudioCodec() {}
   
  /*
   * Open the decoder, returns true on success
   */
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options) = 0;

  /*
   * Dispose, Free all resources
   */
  virtual void Dispose() = 0;
  
  /*
   * returns bytes used or -1 on error
   * 
   */
  virtual int Decode(BYTE* pData, int iSize) = 0;
  
  /*
   * returns nr of bytes used or -1 on error
   * the data is valid until the next Decode call
   */
  virtual int GetData(BYTE** dst) = 0;
  
  /*
   * resets the decoder
   */
  virtual void Reset() = 0;
  
  /*
   * returns the nr of channels for the decoded audio stream
   */
  virtual int GetOutputChannels() = 0;

  /*
   * returns the nr of channels in the source audio stream
   */
  virtual int GetSourceChannels() = 0;
  
  /*
   * returns the samplerate for the decoded audio stream
   */
  virtual int GetSampleRate() = 0;
  
  /*
   * returns the bitspersample for the decoded audio stream (eg 16 bits)
   */
  virtual int GetBitsPerSample() = 0;
  
  /*
   * returns if the codec requests to use passtrough
   */
  virtual bool NeedPasstrough() { return false; }

  /*
   * should return class name
   */
  virtual const char* GetName() = 0;

  /*
   *
   * should return actual underlying codec name in the case where a codec class is 
   * representing several underlying codecs
   */
  virtual const char* GetCodecName() = 0;

  /*
   * should return amount of data decoded has buffered in preparation for next audio frame
   */
  virtual int GetBufferSize() { return 0; }
};
