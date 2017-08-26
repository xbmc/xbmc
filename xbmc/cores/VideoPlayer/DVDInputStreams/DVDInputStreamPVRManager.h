#pragma once

/*
 *      Copyright (C) 2012-2013 Team XBMC
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

/*
* for DESCRIPTION see 'DVDInputStreamPVRManager.cpp'
*/

#include <vector>
#include "DVDInputStream.h"
#include "FileItem.h"
#include "threads/SystemClock.h"

class IVideoPlayer;
struct PVR_STREAM_PROPERTIES;
class CDemuxStreamAudio;
class CDemuxStreamVideo;
class CDemuxStreamSubtitle;
class CDemuxStreamTeletext;
class CDemuxStreamRadioRDS;
class IDemux;

class CDVDInputStreamPVRManager
  : public CDVDInputStream
  , public CDVDInputStream::ITimes
  , public CDVDInputStream::IDisplayTime
  , public CDVDInputStream::IDemux
{
public:
  CDVDInputStreamPVRManager(IVideoPlayer* pPlayer, const CFileItem& fileitem);
  ~CDVDInputStreamPVRManager() override;
  bool Open() override;
  void Close() override;
  int Read(uint8_t* buf, int buf_size) override;
  int64_t Seek(int64_t offset, int whence) override;
  bool Pause(double dTime) override { return false; }
  bool IsEOF() override;
  int64_t GetLength() override;

  ENextStream NextStream() override;
  bool IsRealtime() override;

  PVR::CPVRChannelPtr GetSelectedChannel();

  CDVDInputStream::ITimes* GetITimes() override { return this; }
  bool GetTimes(Times &times) override;

  // deprecated
  CDVDInputStream::IDisplayTime* GetIDisplayTime() override { return this; }
  int GetTotalTime() override;
  int GetTime() override;

  bool CanRecord();
  bool IsRecording();
  void Record(bool bOnOff);
  bool CanSeek() override;
  bool CanPause() override;
  void Pause(bool bPaused);

  /* overloaded is streamtype to support m_pOtherStream */
  bool IsStreamType(DVDStreamType type) const;

  /*! \brief Get the input format from the Backend
   If it is empty ffmpeg scanning the stream to find the right input format.
   See "xbmc/cores/VideoPlayer/Codecs/ffmpeg/libavformat/allformats.c" for a
   list of the input formats.
   \return The name of the input format
   */
  std::string GetInputFormat();

  void ResetScanTimeout(unsigned int iTimeoutMs) override;

  // Demux interface
  CDVDInputStream::IDemux* GetIDemux() override;
  bool OpenDemux() override;
  DemuxPacket* ReadDemux() override;
  CDemuxStream* GetStream(int iStreamId) const override;
  std::vector<CDemuxStream*> GetStreams() const override;
  int GetNrOfStreams() const override;
  void SetSpeed(int iSpeed) override;
  bool SeekTime(double time, bool backward = false, double* startpts = NULL) override;
  void AbortDemux() override;
  void FlushDemux() override;

protected:
  void UpdateStreamMap();
  std::shared_ptr<CDemuxStream> GetStreamInternal(int iStreamId);
  IVideoPlayer* m_pPlayer;
  bool m_eof;
  bool m_demuxActive;
  std::string m_strContent;
  XbmcThreads::EndTime m_ScanTimeout;
  PVR_STREAM_PROPERTIES *m_StreamProps;
  std::map<int, std::shared_ptr<CDemuxStream>> m_streamMap;
  bool m_isRecording;
};


inline bool CDVDInputStreamPVRManager::IsStreamType(DVDStreamType type) const
{
  return m_streamType == type;
}

