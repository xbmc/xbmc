#ifndef _ENCODERVORBIS_H
#define _ENCODERVORBIS_H

/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Encoder.h"
#include "DllOgg.h"
#include "DllVorbis.h"
#include "DllVorbisEnc.h"

class CEncoderVorbis : public CEncoder
{
public:
  CEncoderVorbis();
  virtual ~CEncoderVorbis() {};
  bool Init(const char* strFile, int iInChannels, int iInRate, int iInBits);
  int Encode(int nNumBytesRead, BYTE* pbtStream);
  bool Close();
  void AddTag(int key, const char* value);

protected:
  vorbis_info m_sVorbisInfo; /* struct that stores all the static vorbis bitstream settings */
  vorbis_dsp_state m_sVorbisDspState; /* central working state for the packet->PCM decoder */
  vorbis_block m_sVorbisBlock; /* local working space for packet->PCM decode */
  vorbis_comment m_sVorbisComment;

  ogg_stream_state m_sOggStreamState; /* take physical pages, weld into a logical stream of packets */
  ogg_page m_sOggPage; /* one Ogg bitstream page.  Vorbis packets are inside */
  ogg_packet m_sOggPacket; /* one raw packet of data for decode */

  BYTE* m_pBuffer;

  DllOgg m_OggDll;
  DllVorbis m_VorbisDll;
  DllVorbisEnc m_VorbisEncDll;
};

#endif // _ENCODERVORBIS_H
