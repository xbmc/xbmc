/*
 *      vdr-plugin-vnsi - XBMC server plugin for VDR
 *
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
 *
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

#ifndef VNSI_DEMUXER_MPEGVIDEO_H
#define VNSI_DEMUXER_MPEGVIDEO_H

#include <deque>
#include "demuxer.h"

class cBitstream;

// --- cParserMPEG2Video -------------------------------------------------

class cParserMPEG2Video : public cParser
{
private:
  std::deque<sStreamPacket*> m_DurationQueue;
  std::deque<sStreamPacket*> m_PTSQueue;

  cTSDemuxer     *m_demuxer;
  uint8_t        *m_pictureBuffer;
  int             m_pictureBufferSize;
  int             m_pictureBufferPtr;
  int             m_FrameDuration;
  uint32_t        m_StartCond;
  uint32_t        m_StartCode;
  int             m_StartCodeOffset;
  sStreamPacket  *m_StreamPacket;
  int             m_vbvDelay;       /* -1 if CBR */
  int             m_vbvSize;        /* Video buffer size (in bytes) */
  int             m_Width;
  int             m_Height;

  bool Parse_MPEG2Video(size_t len, uint32_t next_startcode, int sc_offset);
  bool Parse_MPEG2Video_SeqStart(cBitstream *bs);
  bool Parse_MPEG2Video_PicStart(int *frametype, cBitstream *bs);
  void Parse_ComputePTS(sStreamPacket* pkt);
  void Parse_ComputeDuration(sStreamPacket* pkt);

public:
  cParserMPEG2Video(cTSDemuxer *demuxer, cLiveStreamer *streamer, int streamID);
  virtual ~cParserMPEG2Video();

  virtual void Parse(unsigned char *data, int size, bool pusi);
};

#endif // VNSI_DEMUXER_MPEGVIDEO_H
