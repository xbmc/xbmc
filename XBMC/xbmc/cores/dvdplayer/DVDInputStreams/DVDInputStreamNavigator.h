
#pragma once

#include "DVDInputStream.h"
#include "..\IDVDPlayer.h"
#include <string>

#ifdef __cplusplus
extern "C"
{
#endif
 #define DVDNAV_COMPILE
 #include "dvdnav/dvdnav.h"
#ifdef __cplusplus
}
#endif

#define NAVRESULT_OK                0x00000001
#define NAVRESULT_SKIPPED_STILL     0x00000002
#define NAVRESULT_STILL_NOT_SKIPPED 0x00000004

class DllLoader;
class CDVDDemuxSPU;
struct SPUInfo;
struct DVDOverlayPicture;

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

  virtual bool Open(const char* strFile);
  virtual void Close();
  virtual int Read(BYTE* buf, int buf_size);
  virtual __int64 Seek(__int64 offset, int whence);
  void Lock();
  void Unlock();

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

  int GetCurrentButton();
  int GetTotalButtons();
  bool GetHighLightArea(int* iXStart, int* iXEnd, int* iYStart, int* iYEnd, int iButton);
  bool GetButtonInfo(struct DVDOverlayPicture* pOverlayPicture, CDVDDemuxSPU* pSPU);
  bool IsInMenu();

  int GetActiveSubtitleStream();
  std::string GetSubtitleStreamLanguage(int iId);
  int GetSubTitleStreamCount();

  int GetActiveAudioStream();
  std::string GetAudioStreamLanguage(int iId);
  int GetAudioStreamCount();
  bool SetActiveAudioStream(int iPhysicalId);
  bool SetActiveSubtitleStream(int iPhysicalId);

  int GetTotalTime(); // the total time in milli seconds
  int GetTime(); // the current position in milli seconds

  bool Seek(int iTimeInMsec); //seek within current pg(c)
  //float GetPercentage(); //percentage within current pg(c)


protected:

  bool LoadDLL();
  void UnloadDLL();

  int ProcessBlock();

  unsigned __int8 m_mem[32 * 1048];
  unsigned __int8 m_temp[32 * 1024];
  int m_pBufferSize;

  CRITICAL_SECTION m_critSection;
  struct dvdnav_s* m_dvdnav;
  DllLoader* m_pDLLlibdvdnav;
  DllLoader* m_pDLLlibdvdcss;
  IDVDPlayer* m_pDVDPlayer;

  bool m_bDiscardHop;
  int m_iTotalTime;
  int m_iCurrentTime;
};
