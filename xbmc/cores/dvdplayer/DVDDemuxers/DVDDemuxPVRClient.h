#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "DVDDemux.h"
#include <map>

#ifndef _LINUX
#include <libavformat/avformat.h>
#else
extern "C" {
#if (defined USE_EXTERNAL_FFMPEG)
  #if (defined HAVE_LIBAVFORMAT_AVFORMAT_H)
    #include <libavformat/avformat.h>
  #elif (defined HAVE_FFMPEG_AVFORMAT_H)
    #include <ffmpeg/avformat.h>
  #endif
#else
  #include "libavformat/avformat.h"
#endif
}
#endif

class CDVDDemuxPVRClient;
struct PVR_STREAM_PROPERTIES;

class CDemuxStreamVideoPVRClient : public CDemuxStreamVideo
{
  CDVDDemuxPVRClient *m_parent;
public:
  CDemuxStreamVideoPVRClient(CDVDDemuxPVRClient *parent)
    : m_parent(parent)
  {}
  virtual void GetStreamInfo(std::string& strInfo);
};

class CDemuxStreamAudioPVRClient : public CDemuxStreamAudio
{
  CDVDDemuxPVRClient *m_parent;
public:
  CDemuxStreamAudioPVRClient(CDVDDemuxPVRClient *parent)
    : m_parent(parent)
  {}
  virtual void GetStreamInfo(std::string& strInfo);
};

class CDemuxStreamSubtitlePVRClient : public CDemuxStreamSubtitle
{
  CDVDDemuxPVRClient *m_parent;
public:
  CDemuxStreamSubtitlePVRClient(CDVDDemuxPVRClient *parent)
    : m_parent(parent)
  {}
  virtual void GetStreamInfo(std::string& strInfo);
};


class CDVDDemuxPVRClient : public CDVDDemux
{
public:

  CDVDDemuxPVRClient();
  ~CDVDDemuxPVRClient();

  bool Open(CDVDInputStream* pInput);
  void Dispose();
  void Reset();
  void Abort();
  void Flush();
  DemuxPacket* Read();
  bool SeekTime(int time, bool backwords = false, double* startpts = NULL) { return false; }
  void SetSpeed(int iSpeed) {};
  int GetStreamLength() { return 0; }
  CDemuxStream* GetStream(int iStreamId);
  int GetNrOfStreams();
  std::string GetFileName();
  virtual void GetStreamCodecName(int iStreamId, CStdString &strName);

protected:
  CDVDInputStream* m_pInput;

  CDemuxStream* m_streams[MAX_STREAMS]; // maximum number of streams that ffmpeg can handle

private:
  void RequestStreams();
  void UpdateStreams(PVR_STREAM_PROPERTIES *props);
};

