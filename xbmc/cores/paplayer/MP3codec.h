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

#include "CachingCodec.h"
#include "music/tags/MusicInfoTagLoaderMP3.h"
#include "../dvdplayer/DVDCodecs/Audio/DllLibMad.h"

enum madx_sig {
	ERROR_OCCURED,
	MORE_INPUT,
	FLUSH_BUFFER,
	CALL_AGAIN,
  SKIP_FRAME
};

struct madx_house {
  struct mad_stream stream;
  struct mad_frame  frame;
  struct mad_synth  synth;
  mad_timer_t       timer;
  unsigned long     frame_cnt;
  unsigned char*    output_ptr;
};

struct madx_stat {
  size_t 		     write_size;
  size_t		     readsize;
  size_t		     remaining;
  size_t 		     framepcmsize;
  bool           flushed;
};

class MP3Codec : public CachingCodec
{
public:
  MP3Codec();
  virtual ~MP3Codec();

  virtual bool Init(const CStdString &strFile, unsigned int filecache);
  virtual void DeInit();
  virtual bool CanSeek();
  virtual __int64 Seek(__int64 iSeekTime);
  virtual int ReadPCM(BYTE *pBuffer, int size, int *actualsize);
  virtual int ReadSamples(float *pBuffer, int numsamples, int *actualsamples);
  virtual bool CanInit();
  virtual bool SkipNext();
  virtual bool HasFloatData() const { return m_BitsPerSampleInternal == 32; };
private:

  /* TODO decoder functions */
  virtual int Decode(int *out_len);
  virtual void Flush();
  int madx_init(madx_house* mxhouse);
  madx_sig madx_read(madx_house *mxhouse, madx_stat* mxstat, int maxwrite, bool discard = false);
  void madx_deinit(madx_house* mxhouse);
  /* END decoder functions */

  void OnFileReaderClearEvent();
  void FlushDecoder();
  int Read(int size, bool init = false);

  /* TODO decoder vars */
  int m_BytesDecoded;
  bool m_HaveData;
  unsigned int m_formatdata[8];
  unsigned char  flushcnt;

  madx_house mxhouse;
  madx_stat  mxstat;
  madx_sig   mxsig;
  /* END decoder vars */

  // Decoding variables
  __int64 m_lastByteOffset;
  bool    m_eof;
  bool    m_Decoding;
  bool    m_CallAgainWithSameBuffer;
  int     m_readRetries;

  // Input buffer to read our mp3 data into
  BYTE*         m_InputBuffer;
  unsigned int  m_InputBufferSize;
  unsigned int  m_InputBufferPos;

  // Output buffer.  We require this, as mp3 decoding means keeping at least 2 frames (1152 * 2 samples)
  // of data in order to remove that data at the end as it may be surplus to requirements.
  BYTE*         m_OutputBuffer;
  unsigned int  m_OutputBufferSize;
  unsigned int  m_OutputBufferPos;    // position in our buffer

  unsigned int m_Formatdata[8];

  // Seeking helpers
  MUSIC_INFO::CVBRMP3SeekHelper m_seekInfo;

  // Gapless playback
  bool m_IgnoreFirst;     // Ignore first samples if this is true (for gapless playback)
  bool m_IgnoreLast;      // Ignore first samples if this is true (for gapless playback)
  int m_IgnoredBytes;     // amount of samples ignored thus far

  int m_BitsPerSampleInternal;

  DllLibMad m_dll;
};

