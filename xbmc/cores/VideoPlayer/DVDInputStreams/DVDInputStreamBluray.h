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
#include <list>
#include <memory>

extern "C"
{
#include <libbluray/bluray.h>
#include <libbluray/bluray-version.h>
#include <libbluray/keys.h>
#include <libbluray/overlay.h>
#include <libbluray/player_settings.h>
}

#define MAX_PLAYLIST_ID 99999

class CDVDOverlayImage;
class DllLibbluray;
class IVideoPlayer;

class CDVDInputStreamBluray 
  : public CDVDInputStream
  , public CDVDInputStream::IDisplayTime
  , public CDVDInputStream::IChapter
  , public CDVDInputStream::IPosTime
  , public CDVDInputStream::IMenus
{
public:
  CDVDInputStreamBluray(IVideoPlayer* player, const CFileItem& fileitem);
  virtual ~CDVDInputStreamBluray();
  virtual bool Open() override;
  virtual void Close() override;
  virtual int Read(uint8_t* buf, int buf_size) override;
  virtual int64_t Seek(int64_t offset, int whence) override;
  virtual bool Pause(double dTime) override { return false; };
  void Abort() override;
  virtual bool IsEOF() const  override;
  virtual int64_t GetLength() const override;
  virtual int GetBlockSize() const override { return 6144; }
  virtual ENextStream NextStream() override;


  /* IMenus */
  virtual void ActivateButton() override          { UserInput(BD_VK_ENTER); }
  virtual void SelectButton(int iButton) override
  {
    if(iButton < 10)
      UserInput((bd_vk_key_e)(BD_VK_0 + iButton));
  }
  virtual int  GetCurrentButton() const override        { return 0; }
  virtual int  GetTotalButtons() const override         { return 0; }
  virtual void OnUp() override                    { UserInput(BD_VK_UP); }
  virtual void OnDown() override                  { UserInput(BD_VK_DOWN); }
  virtual void OnLeft() override                  { UserInput(BD_VK_LEFT); }
  virtual void OnRight() override                { UserInput(BD_VK_RIGHT); }
  virtual void OnMenu() override;
  virtual void OnBack() override
  {
    if(IsInMenu())
      OnMenu();
  }
  virtual void OnNext() override                  {}
  virtual void OnPrevious() override              {}
  virtual bool HasMenu() const override;
  virtual bool IsInMenu() const override;
  virtual bool OnMouseMove(const CPoint &point) override  { return MouseMove(point); }
  virtual bool OnMouseClick(const CPoint &point) override { return MouseClick(point); }
  virtual void SkipStill() override;
  virtual bool GetState(std::string &xmlstate) const override         { return false; }
  virtual bool SetState(const std::string &xmlstate) override   { return false; }


  void UserInput(bd_vk_key_e vk);
  bool MouseMove(const CPoint &point);
  bool MouseClick(const CPoint &point);

  int GetChapter() const override;
  int GetChapterCount() const override;
  void GetChapterName(std::string& name, int ch=-1) const override {};
  int64_t GetChapterPos(int ch) const override;
  bool SeekChapter(int ch) override;

  CDVDInputStream::IDisplayTime* GetIDisplayTime() override { return this; }
  int GetTotalTime() const override;
  int GetTime() const override;

  CDVDInputStream::IPosTime* GetIPosTime() override { return this; }
  bool PosTime(int ms) override;

  void GetStreamInfo(int pid, char* language) const;

  void OverlayCallback(const BD_OVERLAY * const);
#ifdef HAVE_LIBBLURAY_BDJ
  void OverlayCallbackARGB(const struct bd_argb_overlay_s * const);
#endif

  BLURAY_TITLE_INFO* GetTitleLongest();
  BLURAY_TITLE_INFO* GetTitleFile(const std::string& name);

  void ProcessEvent();

protected:
  struct SPlane;

  void OverlayFlush(int64_t pts);
  void OverlayClose();
  static void OverlayClear(SPlane& plane, int x, int y, int w, int h);
  static void OverlayInit (SPlane& plane, int w, int h);

  IVideoPlayer*         m_player;
  DllLibbluray*       m_dll;
  BLURAY*             m_bd;
  BLURAY_TITLE_INFO*  m_title;
  uint32_t            m_playlist;
  uint32_t            m_clip;
  uint32_t            m_angle;
  bool                m_menu;
  bool                m_navmode;
  int m_dispTimeBeforeRead;

  typedef std::shared_ptr<CDVDOverlayImage> SOverlay;
  typedef std::list<SOverlay>                 SOverlays;

  struct SPlane
  {
    SOverlays o;
    int       w;
    int       h;

    SPlane()
    : w(0)
    , h(0)
    {}
  };

  SPlane m_planes[2];
  enum EHoldState {
    HOLD_NONE = 0,
    HOLD_HELD,
    HOLD_DATA,
    HOLD_STILL,
    HOLD_ERROR,
    HOLD_EXIT
  } m_hold;
  BD_EVENT m_event;
#ifdef HAVE_LIBBLURAY_BDJ
  struct bd_argb_buffer_s m_argb;
#endif

  private:
    void SetupPlayerSettings();
};
