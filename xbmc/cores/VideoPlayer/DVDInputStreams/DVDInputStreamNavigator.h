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

#include "DVDInputStream.h"
#include "DVDDemuxers/DVDDemux.h"
#include "../IVideoPlayer.h"
#include "../DVDCodecs/Overlay/DVDOverlaySpu.h"
#include <string>
#include "guilib/Geometry.h"

#include "DllDvdNav.h"

#define DVD_VIDEO_BLOCKSIZE         DVD_VIDEO_LB_LEN // 2048 bytes

#define NAVRESULT_NOP               0x00000001 // keep processing messages
#define NAVRESULT_DATA              0x00000002 // return data to demuxer
#define NAVRESULT_ERROR             0x00000003 // return read error to demuxer
#define NAVRESULT_HOLD              0x00000004 // return eof to demuxer

#define LIBDVDNAV_BUTTON_NORMAL 0
#define LIBDVDNAV_BUTTON_CLICKED 1

class CDVDDemuxSPU;
class CSPUInfo;
class CDVDOverlayPicture;

struct dvdnav_s;

struct DVDNavStreamInfo
{
  std::string name;
  std::string language;

  DVDNavStreamInfo() {}
};

struct DVDNavAudioStreamInfo : DVDNavStreamInfo
{
  std::string codec;
  int channels;

  DVDNavAudioStreamInfo() : DVDNavStreamInfo(),
    channels(0) {}
};

struct DVDNavSubtitleStreamInfo : DVDNavStreamInfo
{
  CDemuxStream::EFlags flags;

  DVDNavSubtitleStreamInfo() : DVDNavStreamInfo(),
    flags(CDemuxStream::EFlags::FLAG_NONE) {}
};

struct DVDNavVideoStreamInfo : DVDNavStreamInfo
{
  int angles;
  float aspectRatio;
  std::string codec;
  uint32_t width;
  uint32_t height;

  DVDNavVideoStreamInfo() : DVDNavStreamInfo(),
    angles(0),
    aspectRatio(0.0f),
    width(0),
    height(0)
  {}
};

class DVDNavResult
{
public:
  DVDNavResult() :
      pData (NULL ),
      type  (0    )
  {
  };
  DVDNavResult(void* p, int t) { pData = p; type = t; };
  void* pData;
  int type;
};

class CDVDInputStreamNavigator
  : public CDVDInputStream
  , public CDVDInputStream::IDisplayTime
  , public CDVDInputStream::IChapter
  , public CDVDInputStream::IPosTime
  , public CDVDInputStream::IMenus
{
public:
  CDVDInputStreamNavigator(IVideoPlayer* player, const CFileItem& fileitem);
  virtual ~CDVDInputStreamNavigator();

  virtual bool Open() override;
  virtual void Close() override;
  virtual int Read(uint8_t* buf, int buf_size) override;
  virtual int64_t Seek(int64_t offset, int whence) override;
  virtual bool Pause(double dTime) override { return false; };
  virtual int GetBlockSize() const override { return DVDSTREAM_BLOCK_SIZE_DVD; }
  virtual bool IsEOF() const override { return m_bEOF; }
  virtual int64_t GetLength() const override { return 0; }
  virtual ENextStream NextStream() override;

  void ActivateButton() override;
  void SelectButton(int iButton) override;
  void SkipStill() override;
  void SkipWait();
  void OnUp() override;
  void OnDown() override;
  void OnLeft() override;
  void OnRight() override;
  void OnMenu() override;
  void OnBack() override;
  void OnNext() override;
  void OnPrevious() override;
  bool OnMouseMove(const CPoint &point) override;
  bool OnMouseClick(const CPoint &point) override;

  int GetCurrentButton() const override;
  int GetTotalButtons() const override;
  bool GetCurrentButtonInfo(CDVDOverlaySpu* pOverlayPicture, CDVDDemuxSPU* pSPU, int iButtonType /* 0 = selection, 1 = action (clicked)*/);

  bool HasMenu() const override { return true; }
  bool IsInMenu() const override { return m_bInMenu; }
  double GetTimeStampCorrection() const override { return (double)(m_iVobUnitCorrection * 1000) / 90; }

  int GetActiveSubtitleStream() const;
  int GetSubTitleStreamCount() const;
  DVDNavSubtitleStreamInfo GetSubtitleStreamInfo(const int iId) const;

  bool SetActiveSubtitleStream(int iId);
  void EnableSubtitleStream(bool bEnable);
  bool IsSubtitleStreamEnabled() const;

  int GetActiveAudioStream();
  int GetAudioStreamCount();
  int GetActiveAngle();
  bool SetAngle(int angle);
  bool SetActiveAudioStream(int iId);
  DVDNavAudioStreamInfo GetAudioStreamInfo(const int iId);

  bool GetState(std::string &xmlstate) const override;
  bool SetState(const std::string &xmlstate) override;

  int GetChapter() const override      { return m_iPart; }      // the current part in the current title
  int GetChapterCount() const override { return m_iPartCount; } // the number of parts in the current title
  void GetChapterName(std::string& name, int idx=-1) const override {};
  int64_t GetChapterPos(int ch=-1) const override;
  bool SeekChapter(int iChapter) override;

  CDVDInputStream::IDisplayTime* GetIDisplayTime() override { return this; }
  int GetTotalTime() const override; // the total time in milli seconds
  int GetTime() const override; // the current position in milli seconds

  float GetVideoAspectRatio() const;

  CDVDInputStream::IPosTime* GetIPosTime() override { return this; }
  bool PosTime(int iTimeInMsec) override; //seek within current pg(c)

  std::string GetDVDTitleString() const;
  std::string GetDVDSerialString() const;

  void CheckButtons();

  DVDNavVideoStreamInfo GetVideoStreamInfo();

protected:

  int ProcessBlock(uint8_t* buffer, int* read);

  /**
   * XBMC     : the audio stream id we use in xbmc
   * external : the audio stream id that is used in libdvdnav
   */
  int ConvertAudioStreamId_XBMCToExternal(int id);
  int ConvertAudioStreamId_ExternalToXBMC(int id);

  /**
   * XBMC     : the subtitle stream id we use in xbmc
   * external : the subtitle stream id that is used in libdvdnav
   */
  int ConvertSubtitleStreamId_XBMCToExternal(int id) const;
  int ConvertSubtitleStreamId_ExternalToXBMC(int id) const;

  static void SetAudioStreamName(DVDNavStreamInfo &info, const audio_attr_t &audio_attributes);
  static void SetSubtitleStreamName(DVDNavStreamInfo &info, const subp_attr_t &subp_attributes);

  int GetAngleCount();
  void GetVideoResolution(uint32_t * width, uint32_t * height);

  mutable DllDvdNav m_dll;
  bool m_bCheckButtons;
  bool m_bEOF;

  int m_holdmode;

  int m_iTotalTime;
  int m_iTime;
  int64_t m_iCellStart; // start time of current cell in pts units (90khz clock)

  bool m_bInMenu;

  int64_t m_iVobUnitStart;
  int64_t m_iVobUnitStop;
  int64_t m_iVobUnitCorrection;

  int m_iTitleCount;
  int m_iTitle;

  int m_iPartCount;
  int m_iPart;

  struct dvdnav_s* m_dvdnav;

  IVideoPlayer* m_pVideoPlayer;

  uint8_t m_lastblock[DVD_VIDEO_BLOCKSIZE];
  int     m_lastevent;

  std::map<int, std::map<int, int64_t>> m_mapTitleChapters;
};

