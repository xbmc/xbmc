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

#ifndef VNSI_DEMUXER_MPEGAUDIO_H
#define VNSI_DEMUXER_MPEGAUDIO_H

#include "demuxer.h"

// --- cParserMPEG2Audio -------------------------------------------------

class cParserMPEG2Audio : public cParser
{
private:
  typedef struct MPADecodeHeader
  {
    int frame_size;
    int error_protection;
    int layer;
    int sample_rate;
    int sample_rate_index; /* between 0 and 8 */
    int bit_rate;
    int nb_channels;
    int mode;
    int mode_ext;
    int lsf;
  } MPADecodeHeader;

  cTSDemuxer *m_demuxer;
  bool        m_firstPUSIseen;
  bool        m_PESStart;

  bool        m_FetchTimestamp;
  int64_t     m_FrameOffset;        /* offset of the current frame */
  int64_t     m_CurrentOffset;      /* current offset (incremented by each av_parser_parse()) */
  int64_t     m_NextFrameOffset;    /* offset of the next frame */

#define AV_PARSER_PTS_NB 4
  int         m_CurrentFrameStartIndex;
  int64_t     m_CurrentFrameOffset[AV_PARSER_PTS_NB];
  int64_t     m_CurrentFramePTS[AV_PARSER_PTS_NB];
  int64_t     m_CurrentFrameDTS[AV_PARSER_PTS_NB];
  int64_t     m_CurrentFrameEnd[AV_PARSER_PTS_NB];
  int64_t     m_Offset;             /* byte offset from starting packet start */
  int64_t     m_PTS;                /* pts of the current frame */
  int64_t     m_DTS;                /* dts of the current frame */
  int64_t     m_NextDTS;

#define SAME_HEADER_MASK (0xffe00000 | (3 << 17) | (3 << 10) | (3 << 19))
#define MPA_HEADER_SIZE 4
#define MPA_MAX_CODED_FRAME_SIZE 1792
#define MPA_STEREO  0
#define MPA_JSTEREO 1
#define MPA_DUAL    2
#define MPA_MONO    3
  int         m_FrameSize;
  int         m_FrameFreeFormatFrameSize;
  int         m_FrameFreeFormatNextHeader;
  uint32_t    m_FrameHeader;
  int         m_HeaderCount;
  int         m_SampleRate;
  int         m_Channels;
  int         m_BitRate;
  uint8_t     m_FrameInBuffer[MPA_MAX_CODED_FRAME_SIZE];    /* input buffer */
  uint8_t    *m_FrameInBufferPtr;

  /* fast header check for resync */
  void FetchTimestamp(int off, bool remove);
  int FindHeaders(uint8_t **poutbuf, int *poutbuf_size,
                  uint8_t *buf, int buf_size,
                  int64_t pts, int64_t dts);
  static inline bool CheckHeader(uint32_t header)
  {
    /* header */
    if ((header & 0xffe00000) != 0xffe00000)
      return false;
    /* layer check */
    if ((header & (3<<17)) == 0)
      return false;
    /* bit rate */
    if ((header & (0xf<<12)) == 0xf<<12)
      return false;
    /* frequency */
    if ((header & (3<<10)) == 3<<10)
      return false;
    return true;
  }
  bool DecodeHeader(MPADecodeHeader *s, uint32_t header);

public:
  cParserMPEG2Audio(cTSDemuxer *demuxer, cLiveStreamer *streamer, int streamID);
  virtual ~cParserMPEG2Audio();

  virtual void Parse(unsigned char *data, int size, bool pusi);
};


#endif // VNSI_DEMUXER_MPEGAUDIO_H
