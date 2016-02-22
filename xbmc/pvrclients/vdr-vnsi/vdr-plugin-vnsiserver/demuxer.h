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

#ifndef VNSI_DEMUXER_H
#define VNSI_DEMUXER_H

#include <vdr/device.h>
#include <queue>

#define DVD_TIME_BASE 1000000
#define DVD_NOPTS_VALUE    (-1LL<<52) // should be possible to represent in both double and __int64

/* PES PIDs */
#define PRIVATE_STREAM1   0xBD
#define PADDING_STREAM    0xBE
#define PRIVATE_STREAM2   0xBF
#define PRIVATE_STREAM3   0xFD
#define AUDIO_STREAM_S    0xC0      /* 1100 0000 */
#define AUDIO_STREAM_E    0xDF      /* 1101 1111 */
#define VIDEO_STREAM_S    0xE0      /* 1110 0000 */
#define VIDEO_STREAM_E    0xEF      /* 1110 1111 */

#define AUDIO_STREAM_MASK 0x1F  /* 0001 1111 */
#define VIDEO_STREAM_MASK 0x0F  /* 0000 1111 */
#define AUDIO_STREAM      0xC0  /* 1100 0000 */
#define VIDEO_STREAM      0xE0  /* 1110 0000 */

#define ECM_STREAM        0xF0
#define EMM_STREAM        0xF1
#define DSM_CC_STREAM     0xF2
#define ISO13522_STREAM   0xF3
#define PROG_STREAM_DIR   0xFF

inline bool PesIsHeader(const uchar *p)
{
  return !(p)[0] && !(p)[1] && (p)[2] == 1;
}

inline int PesHeaderLength(const uchar *p)
{
  return 8 + (p)[8] + 1;
}

inline bool PesIsVideoPacket(const uchar *p)
{
  return (((p)[3] & ~VIDEO_STREAM_MASK) == VIDEO_STREAM);
}

inline bool PesIsMPEGAudioPacket(const uchar *p)
{
  return (((p)[3] & ~AUDIO_STREAM_MASK) == AUDIO_STREAM);
}

inline bool PesIsPS1Packet(const uchar *p)
{
  return ((p)[3] == PRIVATE_STREAM1 || (p)[3] == PRIVATE_STREAM3 );
}

inline bool PesIsPaddingPacket(const uchar *p)
{
  return ((p)[3] == PADDING_STREAM);
}

inline bool PesIsAudioPacket(const uchar *p)
{
  return (PesIsMPEGAudioPacket(p) || PesIsPS1Packet(p));
}

#if APIVERSNUM < 10701

#define TS_ERROR              0x80
#define TS_PAYLOAD_START      0x40
#define TS_TRANSPORT_PRIORITY 0x20
#define TS_PID_MASK_HI        0x1F
#define TS_SCRAMBLING_CONTROL 0xC0
#define TS_ADAPT_FIELD_EXISTS 0x20
#define TS_PAYLOAD_EXISTS     0x10
#define TS_CONT_CNT_MASK      0x0F
#define TS_ADAPT_DISCONT      0x80
#define TS_ADAPT_RANDOM_ACC   0x40 // would be perfect for detecting independent frames, but unfortunately not used by all broadcasters
#define TS_ADAPT_ELEM_PRIO    0x20
#define TS_ADAPT_PCR          0x10
#define TS_ADAPT_OPCR         0x08
#define TS_ADAPT_SPLICING     0x04
#define TS_ADAPT_TP_PRIVATE   0x02
#define TS_ADAPT_EXTENSION    0x01

inline bool TsHasPayload(const uchar *p)
{
  return p[3] & TS_PAYLOAD_EXISTS;
}

inline bool TsHasAdaptationField(const uchar *p)
{
  return p[3] & TS_ADAPT_FIELD_EXISTS;
}

inline bool TsPayloadStart(const uchar *p)
{
  return p[1] & TS_PAYLOAD_START;
}

inline bool TsError(const uchar *p)
{
  return p[1] & TS_ERROR;
}

inline int TsPid(const uchar *p)
{
  return (p[1] & TS_PID_MASK_HI) * 256 + p[2];
}

inline bool TsIsScrambled(const uchar *p)
{
  return p[3] & TS_SCRAMBLING_CONTROL;
}

inline int TsPayloadOffset(const uchar *p)
{
  return (p[3] & TS_ADAPT_FIELD_EXISTS) ? p[4] + 5 : 4;
}

inline int TsGetPayload(const uchar **p)
{
  int o = TsPayloadOffset(*p);
  *p += o;
  return TS_SIZE - o;
}

inline int TsContinuityCounter(const uchar *p)
{
  return p[3] & TS_CONT_CNT_MASK;
}

inline int TsGetAdaptationField(const uchar *p)
{
  return TsHasAdaptationField(p) ? p[5] : 0x00;
}
#endif

enum eStreamContent
{
  scVIDEO,
  scAUDIO,
  scSUBTITLE,
  scTELETEXT,
  scPROGRAMM
};

enum eStreamType
{
  stNone,
  stAC3,
  stMPEG2AUDIO,
  stEAC3,
  stAAC,
  stDTS,
  stMPEG2VIDEO,
  stH264,
  stDVBSUB,
  stTEXTSUB,
  stTELETEXT,
};

#define PKT_I_FRAME 1
#define PKT_P_FRAME 2
#define PKT_B_FRAME 3
#define PKT_NTYPES  4
struct sStreamPacket
{
  sStreamPacket() {
    frametype = 0;
  }

  int64_t   id;
  int64_t   dts;
  int64_t   pts;
  int       duration;

  uint8_t   commercial;
  uint8_t   componentindex;
  uint8_t   frametype;

  uint8_t  *data;
  int       size;
};

class cLiveStreamer;

class cParser
{
public:
  cParser(cLiveStreamer *streamer, int streamID);
  virtual ~cParser() {};

  virtual void Parse(unsigned char *data, int size, bool pusi) = 0;

  int ParsePESHeader(uint8_t *buf, size_t len);
  void SendPacket(sStreamPacket *pkt);
  void BufferPacket(sStreamPacket *pkt);
  int64_t Rescale(int64_t a);

  cLiveStreamer *m_Streamer;

  int64_t     m_LastDTS;
  int64_t     m_curPTS;
  int64_t     m_curDTS;
  int64_t     m_epochDTS;

  int         m_badDTS;
  int         m_streamID;

protected:
  bool        m_FoundFrame;

  std::queue<sStreamPacket*> m_queue;
};


class cTSDemuxer
{
private:
  cLiveStreamer        *m_Streamer;
  const int             m_streamID;
  const int             m_pID;
  eStreamContent        m_streamContent;
  eStreamType           m_streamType;

  bool                  m_pesError;
  cParser              *m_pesParser;

  char                  m_language[4];  // ISO 639 3-letter language code (empty string if undefined)

  int                   m_FpsScale;     // scale of 1000 and a rate of 29970 will result in 29.97 fps
  int                   m_FpsRate;
  int                   m_Height;       // height of the stream reported by the demuxer
  int                   m_Width;        // width of the stream reported by the demuxer
  float                 m_Aspect;       // display aspect of stream

  int                   m_Channels;
  int                   m_SampleRate;
  int                   m_BitRate;
  int                   m_BitsPerSample;
  int                   m_BlockAlign;

  unsigned char         m_subtitlingType;
  uint16_t              m_compositionPageId;
  uint16_t              m_ancillaryPageId;

public:
  cTSDemuxer(cLiveStreamer *streamer, int id, eStreamType type, int pid);
  virtual ~cTSDemuxer();

  bool ProcessTSPacket(unsigned char *data);

  void SetLanguage(const char *language);
  const char *GetLanguage() { return m_language; }
  const eStreamContent Content() const { return m_streamContent; }
  const eStreamType Type() const { return m_streamType; }
  const int GetPID() const { return m_pID; }
  const int GetStreamID() const { return m_streamID; }

  /* Video Stream Information */
  void SetVideoInformation(int FpsScale, int FpsRate, int Height, int Width, float Aspect);
  uint32_t GetFpsScale() const { return m_FpsScale; }
  uint32_t GetFpsRate() const { return m_FpsRate; }
  uint32_t GetHeight() const { return m_Height; }
  uint32_t GetWidth() const { return m_Width; }
  double GetAspect() const { return m_Aspect; }

  /* Audio Stream Information */
  void SetAudioInformation(int Channels, int SampleRate, int BitRate, int BitsPerSample, int BlockAlign);
  uint32_t GetChannels() const { return m_Channels; }
  uint32_t GetSampleRate() const { return m_SampleRate; }
  uint32_t GetBlockAlign() const { return m_BlockAlign; }
  uint32_t GetBitRate() const { return m_BitRate; }
  uint32_t GetBitsPerSample() const { return m_BitsPerSample; }

  /* Subtitle related stream information */
  void SetSubtitlingDescriptor(unsigned char SubtitlingType, uint16_t CompositionPageId, uint16_t AncillaryPageId);
  unsigned char SubtitlingType() const { return m_subtitlingType; }
  uint16_t CompositionPageId() const { return m_compositionPageId; }
  uint16_t AncillaryPageId() const { return m_ancillaryPageId; }
};

#endif // VNSI_DEMUXER_H
