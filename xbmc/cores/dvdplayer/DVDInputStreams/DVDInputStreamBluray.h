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


extern "C"
{
#include <libbluray/bluray.h>
#include <libbluray/keys.h>
#include <libbluray/overlay.h>
}

class CDVDOverlayImage;
class DllLibbluray;
class IDVDPlayer;

class CDVDInputStreamBluray 
  : public CDVDInputStream
  , public CDVDInputStream::IDisplayTime
  , public CDVDInputStream::IChapter
  , public CDVDInputStream::ISeekTime
  , public CDVDInputStream::IMenus
{
public:
  CDVDInputStreamBluray(IDVDPlayer* player);
  virtual ~CDVDInputStreamBluray();
  virtual bool Open(const char* strFile, const std::string &content);
  virtual void Close();
  virtual int Read(BYTE* buf, int buf_size);
  virtual int64_t Seek(int64_t offset, int whence);
  virtual bool Pause(double dTime) { return false; };
  virtual bool IsEOF();
  virtual int64_t GetLength();
  virtual int GetBlockSize() { return 6144; }
  virtual ENextStream NextStream();


  /* IMenus */
  virtual void ActivateButton()          { UserInput(BD_VK_ENTER); }
  virtual void SelectButton(int iButton)
  {
    if(iButton < 10)
      UserInput((bd_vk_key_e)(BD_VK_0 + iButton));
  }
  virtual int  GetCurrentButton()        { return 0; }
  virtual int  GetTotalButtons()         { return 0; }
  virtual void OnUp()                    { UserInput(BD_VK_UP); }
  virtual void OnDown()                  { UserInput(BD_VK_DOWN); }
  virtual void OnLeft()                  { UserInput(BD_VK_LEFT); }
  virtual void OnRight()                 { UserInput(BD_VK_RIGHT); }
  virtual void OnMenu();
  virtual void OnBack()
  {
    if(IsInMenu())
      OnMenu();
  }
  virtual void OnNext()                  {}
  virtual void OnPrevious()              {}
  virtual bool IsInMenu();
  virtual bool OnMouseMove(const CPoint &point)  { return false; }
  virtual bool OnMouseClick(const CPoint &point) { return false; }
  virtual double GetTimeStampCorrection()        { return 0.0; }

  void UserInput(bd_vk_key_e vk);

  int GetChapter();
  int GetChapterCount();
  void GetChapterName(std::string& name) {};
  bool SeekChapter(int ch);

  int GetTotalTime();
  int GetTime();
  bool SeekTime(int ms);

  void GetStreamInfo(int pid, char* language);

  void OverlayCallback(const BD_OVERLAY * const);

  BLURAY_TITLE_INFO* GetTitleLongest();
  BLURAY_TITLE_INFO* GetTitleFile(const std::string& name);

protected:
  IDVDPlayer*   m_player;
  DllLibbluray *m_dll;
  BLURAY* m_bd;
  BLURAY_TITLE_INFO* m_title;
  bool               m_title_playing;
  uint32_t           m_clip;
  bool m_navmode;
  std::vector<CDVDOverlayImage*> m_overlays[2];
  enum EHoldState {
    HOLD_NONE = 0,
    HOLD_HELD,
    HOLD_SKIP,
    HOLD_DATA,
  } m_hold;
  BD_EVENT m_event;
};
