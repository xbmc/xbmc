#pragma once

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

#include "DVDInputStream.h"
#include "../IDVDPlayer.h"
#include "../DVDCodecs/Overlay/DVDOverlaySpu.h"
#include <string>

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
class CPoint;

struct dvdnav_s;

class DVDNavResult
{
public:
  DVDNavResult(){};
  DVDNavResult(void* p, int t) { pData = p; type = t; };
  void* pData;
  int type;
};

class CDVDInputStreamNavigator
  : public CDVDInputStream
  , public CDVDInputStream::IDisplayTime
  , public CDVDInputStream::IChapter
  , public CDVDInputStream::ISeekTime
  , public CDVDInputStream::IMenus
{
public:
  CDVDInputStreamNavigator(IDVDPlayer* player);
  virtual ~CDVDInputStreamNavigator();

  virtual bool Open(const char* strFile, const std::string& content);
  virtual void Close();
  virtual int Read(BYTE* buf, int buf_size);
  virtual int64_t Seek(int64_t offset, int whence);
  virtual bool Pause(double dTime) { return false; };
  virtual int GetBlockSize() { return DVDSTREAM_BLOCK_SIZE_DVD; }
  virtual bool IsEOF() { return m_bEOF; }
  virtual int64_t GetLength()             { return 0; }
  virtual ENextStream NextStream() ;

  void ActivateButton();
  void SelectButton(int iButton);
  void SkipStill();
  void SkipWait();
  void OnUp();
  void OnDown();
  void OnLeft();
  void OnRight();
  void OnMenu();
  void OnBack();
  void OnNext();
  void OnPrevious();
  bool OnMouseMove(const CPoint &point);
  bool OnMouseClick(const CPoint &point);

  int GetCurrentButton();
  int GetTotalButtons();
  bool GetCurrentButtonInfo(CDVDOverlaySpu* pOverlayPicture, CDVDDemuxSPU* pSPU, int iButtonType /* 0 = selection, 1 = action (clicked)*/);

  bool IsInMenu() { return m_bInMenu; }

  int GetActiveSubtitleStream();
  std::string GetSubtitleStreamLanguage(int iId);
  int GetSubTitleStreamCount();

  bool SetActiveSubtitleStream(int iId);
  void EnableSubtitleStream(bool bEnable);
  bool IsSubtitleStreamEnabled();

  int GetActiveAudioStream();
  std::string GetAudioStreamLanguage(int iId);
  int GetAudioStreamCount();
  bool SetActiveAudioStream(int iId);

  bool GetNavigatorState(std::string &xmlstate);
  bool SetNavigatorState(std::string &xmlstate);

  int GetChapter()      { return m_iPart; }      // the current part in the current title
  int GetChapterCount() { return m_iPartCount; } // the number of parts in the current title
  void GetChapterName(std::string& name) {};
  bool SeekChapter(int iChapter);

  int GetTotalTime(); // the total time in milli seconds
  int GetTime(); // the current position in milli seconds

  float GetVideoAspectRatio();

  bool SeekTime(int iTimeInMsec); //seek within current pg(c)
  virtual int GetCurrentGroupId() { return m_icurrentGroupId; }

  double GetTimeStampCorrection() { return (double)(m_iVobUnitCorrection * 1000) / 90; }
protected:

  int ProcessBlock(BYTE* buffer, int* read);

  void CheckButtons();

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
  int ConvertSubtitleStreamId_XBMCToExternal(int id);
  int ConvertSubtitleStreamId_ExternalToXBMC(int id);

  DllDvdNav m_dll;
  bool m_bCheckButtons;
  bool m_bEOF;

  unsigned int m_icurrentGroupId;
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

  IDVDPlayer* m_pDVDPlayer;

  BYTE m_lastblock[DVD_VIDEO_BLOCKSIZE];
  int  m_lastevent;
};

