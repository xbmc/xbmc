
#pragma once

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

struct dvdnav_s;

class DVDNavResult
{
public:
  DVDNavResult(){};
  DVDNavResult(void* p, int t) { pData = p; type = t; };
  void* pData;
  int type;
};

class CDVDInputStreamNavigator : public CDVDInputStream
{
public:
  CDVDInputStreamNavigator(IDVDPlayer* player);
  virtual ~CDVDInputStreamNavigator();

  virtual bool Open(const char* strFile, const std::string& content);
  virtual void Close();
  virtual int Read(BYTE* buf, int buf_size);
  virtual __int64 Seek(__int64 offset, int whence);
  virtual int GetBlockSize() { return DVDSTREAM_BLOCK_SIZE_DVD; }
  virtual bool IsEOF() { return m_bEOF; }
  virtual __int64 GetLength()             { return 0; }

  void ActivateButton();
  void SelectButton(int iButton);
  void SkipStill();
  void SkipWait();
  void SkipHold();
  void OnUp();
  void OnDown();
  void OnLeft();
  void OnRight();
  void OnMenu();
  void OnBack();
  void OnNext();
  void OnPrevious();

  int GetCurrentButton();
  int GetTotalButtons();
  bool GetCurrentButtonInfo(CDVDOverlaySpu* pOverlayPicture, CDVDDemuxSPU* pSPU, int iButtonType /* 0 = selection, 1 = action (clicked)*/);

  bool IsInMenu() { return m_bInMenu; }
  bool IsHeld();

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

  int GetTotalTime(); // the total time in milli seconds
  int GetTime(); // the current position in milli seconds

  float GetVideoAspectRatio();

  bool Seek(int iTimeInMsec); //seek within current pg(c)
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
  __int64 m_iCellStart; // start time of current cell in pts units (90khz clock)

  bool m_bInMenu;

  __int64 m_iVobUnitStart;
  __int64 m_iVobUnitStop;
  __int64 m_iVobUnitCorrection;

  int m_iTitleCount;
  int m_iTitle;

  int m_iPartCount;
  int m_iPart;

  struct dvdnav_s* m_dvdnav;

  IDVDPlayer* m_pDVDPlayer;

  BYTE m_lastblock[DVD_VIDEO_BLOCKSIZE];
  int  m_lastevent;
};

