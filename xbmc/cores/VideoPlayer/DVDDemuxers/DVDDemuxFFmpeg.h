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

#include "DVDDemux.h"
#include "threads/CriticalSection.h"
#include "threads/SystemClock.h"
#include <map>
#include <vector>

extern "C" {
#include "libavformat/avformat.h"
}

class CDVDDemuxFFmpeg;
class CURL;

class CDemuxStreamVideoFFmpeg
  : public CDemuxStreamVideo
{
  CDVDDemuxFFmpeg *m_parent;
  AVStream*        m_stream;
public:
  CDemuxStreamVideoFFmpeg(CDVDDemuxFFmpeg *parent, AVStream* stream)
    : m_parent(parent)
    , m_stream(stream)
  {}
  std::string      m_description;

  virtual std::string GetStreamName() override;
};


class CDemuxStreamAudioFFmpeg
  : public CDemuxStreamAudio
{
  CDVDDemuxFFmpeg *m_parent;
  AVStream*        m_stream;
public:
  CDemuxStreamAudioFFmpeg(CDVDDemuxFFmpeg *parent, AVStream* stream)
    : m_parent(parent)
    , m_stream(stream)
  {}
  std::string m_description;

  virtual std::string GetStreamName() override;
};

class CDemuxStreamSubtitleFFmpeg
  : public CDemuxStreamSubtitle
{
  CDVDDemuxFFmpeg *m_parent;
  AVStream*        m_stream;
public:
  CDemuxStreamSubtitleFFmpeg(CDVDDemuxFFmpeg *parent, AVStream* stream)
    : m_parent(parent)
    , m_stream(stream)
  {}
  std::string m_description;

  virtual std::string GetStreamName() override;

};

#define FFMPEG_DVDNAV_BUFFER_SIZE 2048  // for dvd's

struct StereoModeConversionMap;

class CDVDDemuxFFmpeg : public CDVDDemux
{
public:
  CDVDDemuxFFmpeg();
  virtual ~CDVDDemuxFFmpeg();

  bool Open(CDVDInputStream* pInput, bool streaminfo = true, bool fileinfo = false);
  void Dispose();
  void Reset() override ;
  void Flush() override;
  void Abort() override;
  void SetSpeed(int iSpeed) override;
  virtual std::string GetFileName() override;

  DemuxPacket* Read() override;

  bool SeekTime(int time, bool backwords = false, double* startpts = NULL) override;
  bool SeekByte(int64_t pos);
  int GetStreamLength() override;
  CDemuxStream* GetStream(int iStreamId) const override;
  std::vector<CDemuxStream*> GetStreams() const override;
  int GetNrOfStreams() const override;

  bool SeekChapter(int chapter, double* startpts = NULL);
  int GetChapterCount();
  int GetChapter();
  void GetChapterName(std::string& strChapterName, int chapterIdx=-1);
  int64_t GetChapterPos(int chapterIdx=-1);
  virtual std::string GetStreamCodecName(int iStreamId) override;

  bool Aborted();

  AVFormatContext* m_pFormatContext;
  CDVDInputStream* m_pInput;

protected:
  friend class CDemuxStreamAudioFFmpeg;
  friend class CDemuxStreamVideoFFmpeg;
  friend class CDemuxStreamSubtitleFFmpeg;

  int ReadFrame(AVPacket *packet);
  CDemuxStream* AddStream(int streamIdx);
  void AddStream(int streamIdx, CDemuxStream* stream);
  void CreateStreams(unsigned int program = UINT_MAX);
  void DisposeStreams();
  void ParsePacket(AVPacket *pkt);
  bool IsVideoReady();
  void ResetVideoStreams();

  AVDictionary *GetFFMpegOptionsFromInput();
  double ConvertTimestamp(int64_t pts, int den, int num);
  void UpdateCurrentPTS();
  bool IsProgramChange();

  std::string GetStereoModeFromMetadata(AVDictionary *pMetadata);
  std::string ConvertCodecToInternalStereoMode(const std::string &mode, const StereoModeConversionMap *conversionMap);

  void GetL16Parameters(int &channels, int &samplerate);
  double SelectAspect(AVStream* st, bool& forced);

  CCriticalSection m_critSection;
  std::map<int, CDemuxStream*> m_streams;

  AVIOContext* m_ioContext;

  double   m_currentPts; // used for stream length estimation
  bool     m_bMatroska;
  bool     m_bAVI;
  int      m_speed;
  unsigned m_program;
  XbmcThreads::EndTime  m_timeout;

  // Due to limitations of ffmpeg, we only can detect a program change
  // with a packet. This struct saves the packet for the next read and
  // signals STREAMCHANGE to player
  struct
  {
    AVPacket pkt;       // packet ffmpeg returned
    int      result;    // result from av_read_packet
  }m_pkt;

  bool m_streaminfo;
  bool m_checkvideo;
  int m_displayTime;
  double m_dtsAtDisplayTime;
};

