/*
 *  Copyright (C) 2005-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../DVDCodecs/Overlay/DVDOverlaySpu.h"
#include "../IVideoPlayer.h"
#include "DVDDemuxers/DVDDemux.h"
#include "DVDInputStream.h"
#include "DVDInputStreamFile.h"
#include "DVDStateSerializer.h"
#include "DllDvdNav.h"
#include "cores/MenuType.h"
#include "utils/Geometry.h"

#include <string>

#define DVD_VIDEO_BLOCKSIZE         DVD_VIDEO_LB_LEN // 2048 bytes

#define NAVRESULT_NOP               0x00000001 // keep processing messages
#define NAVRESULT_DATA              0x00000002 // return data to demuxer
#define NAVRESULT_ERROR             0x00000003 // return read error to demuxer
#define NAVRESULT_HOLD              0x00000004 // return eof to demuxer

#define LIBDVDNAV_BUTTON_NORMAL 0
#define LIBDVDNAV_BUTTON_CLICKED 1

#define DVDNAV_ERROR -1

class CDVDDemuxSPU;
class CSPUInfo;
class CDVDOverlayPicture;

struct dvdnav_s;

class CDVDInputStreamNavigator
  : public CDVDInputStream
  , public CDVDInputStream::IDisplayTime
  , public CDVDInputStream::IChapter
  , public CDVDInputStream::IPosTime
  , public CDVDInputStream::IMenus
{
public:
  CDVDInputStreamNavigator(IVideoPlayer* player, const CFileItem& fileitem);
  ~CDVDInputStreamNavigator() override;

  bool Open() override;
  void Close() override;
  int Read(uint8_t* buf, int buf_size) override;
  int64_t Seek(int64_t offset, int whence) override;
  int GetBlockSize() override { return DVDSTREAM_BLOCK_SIZE_DVD; }
  bool IsEOF() override { return m_bEOF; }
  int64_t GetLength() override { return 0; }
  ENextStream NextStream() override ;

  void ActivateButton() override;
  void SelectButton(int iButton) override;
  void SkipStill() override;
  void SkipWait();
  void OnUp() override;
  void OnDown() override;
  void OnLeft() override;
  void OnRight() override;

  /*! \brief Open the Menu
  * \return true if the menu is successfully opened, false otherwise
  */
  bool OnMenu() override;

  void OnBack() override;
  void OnNext() override;
  void OnPrevious() override;
  bool OnMouseMove(const CPoint &point) override;
  bool OnMouseClick(const CPoint &point) override;

  int GetCurrentButton() override;
  int GetTotalButtons() override;
  bool GetCurrentButtonInfo(CDVDOverlaySpu& pOverlayPicture,
                            CDVDDemuxSPU* pSPU,
                            int iButtonType /* 0 = selection, 1 = action (clicked)*/);

  /*!
   * \brief Get the supported menu type
   * \return The supported menu type
  */
  MenuType GetSupportedMenuType() override { return MenuType::NATIVE; }

  bool IsInMenu() override { return m_bInMenu; }
  double GetTimeStampCorrection() override { return (double)(m_iVobUnitCorrection * 1000) / 90; }

  int GetActiveSubtitleStream();
  int GetSubTitleStreamCount();
  SubtitleStreamInfo GetSubtitleStreamInfo(const int iId);

  bool SetActiveSubtitleStream(int iId);
  void EnableSubtitleStream(bool bEnable);
  bool IsSubtitleStreamEnabled();

  int GetActiveAudioStream();
  int GetAudioStreamCount();
  int GetActiveAngle();
  bool SetAngle(int angle);
  bool SetActiveAudioStream(int iId);
  AudioStreamInfo GetAudioStreamInfo(const int iId);

  bool GetState(std::string &xmlstate) override;
  bool SetState(const std::string &xmlstate) override;

  int GetChapter() override { return m_iPart; } // the current part in the current title
  int GetChapterCount() override { return m_iPartCount; } // the number of parts in the current title
  void GetChapterName(std::string& name, int idx=-1) override {};
  int64_t GetChapterPos(int ch=-1) override;
  bool SeekChapter(int iChapter) override;

  CDVDInputStream::IDisplayTime* GetIDisplayTime() override { return this; }
  int GetTotalTime() override; // the total time in milli seconds
  int GetTime() override; // the current position in milli seconds

  float GetVideoAspectRatio();

  CDVDInputStream::IPosTime* GetIPosTime() override { return this; }
  bool PosTime(int iTimeInMsec) override; //seek within current pg(c)

  std::string GetDVDTitleString();

  /*!
   * \brief Get the DVD volume ID string. Alternative to the dvd title (since some DVD authors
    even forget to set it).
   * \return The DVD volume id
  */
  std::string GetDVDVolIdString();

  std::string GetDVDSerialString();

  void CheckButtons();

  VideoStreamInfo GetVideoStreamInfo();

protected:

  int ProcessBlock(uint8_t* buffer, int* read);

  static void SetAudioStreamName(AudioStreamInfo &info, const audio_attr_t &audio_attributes);
  static void SetSubtitleStreamName(SubtitleStreamInfo &info, const subp_attr_t &subp_attributes);

  int GetAngleCount();
  void GetVideoResolution(uint32_t * width, uint32_t * height);

  /*! \brief Provided a pod DVDState struct, fill it with the current dvdnav state
  * \param[in,out] dvdstate the DVD state struct to be filled
  * \return true if it was possible to fill the state struct based on the current dvdnav state, false otherwise
  */
  bool FillDVDState(DVDState& dvdstate);

  DllDvdNav m_dll;
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
  dvdnav_stream_cb m_dvdnav_stream_cb;
  std::unique_ptr<CDVDInputStreamFile> m_pstream;

  IVideoPlayer* m_pVideoPlayer;

  uint8_t m_lastblock[DVD_VIDEO_BLOCKSIZE];
  int m_lastevent;

  std::map<int, std::map<int, int64_t>> m_mapTitleChapters;

  /*! DVD state serializer handler */
  CDVDStateSerializer m_dvdStateSerializer;
};

