/*
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
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

#ifndef VNSIDEMUXER_H
#define VNSIDEMUXER_H

#define DVD_TIME_BASE 1000000
#define DVD_NOPTS_VALUE    (-1LL<<52) // should be possible to represent in both double and __int64

/* PES PIDs */
#define PRIVATE_STREAM1   0xBD
#define PADDING_STREAM    0xBE
#define PRIVATE_STREAM2   0xBF
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
  return ((p)[3] == PRIVATE_STREAM1);
}

inline bool PesIsPaddingPacket(const uchar *p)
{
  return ((p)[3] == PADDING_STREAM);
}

inline bool PesIsAudioPacket(const uchar *p)
{
  return (PesIsMPEGAudioPacket(p) || PesIsPS1Packet(p));
}

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
  stAC3,
  stMPEG2AUDIO,
  stAAC,
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
private:
  cLiveStreamer *m_Streamer;

public:
  cParser(cLiveStreamer *streamer, int streamID);
  virtual ~cParser() {};

  virtual void Parse(unsigned char *data, int size, bool pusi) = 0;

  int ParsePESHeader(uint8_t *buf, size_t len);
  void SendPacket(sStreamPacket *pkt, bool checkTimestamp = true);
  int64_t Rescale(int64_t a);

  static int64_t m_startDTS;
  int64_t     m_LastDTS;
  int64_t     m_curPTS;
  int64_t     m_curDTS;
  int64_t     m_epochDTS;

  int         m_badDTS;
  int         m_streamID;
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

  void ProcessTSPacket(unsigned char *data);

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

#endif /* VNSIDEMUXER_H */
