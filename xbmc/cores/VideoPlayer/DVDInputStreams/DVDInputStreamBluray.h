/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "BlurayStateSerializer.h"
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
#include "DVDInputStreamFile.h"
}

#define MAX_PLAYLIST_ID 99999
#define MAX_CLIP_ID 99999
#define BD_EVENT_MENU_OVERLAY -1
#define BD_EVENT_MENU_ERROR   -2
#define BD_EVENT_ENC_ERROR    -3

#define HDMV_PID_VIDEO            0x1011
#define HDMV_PID_AUDIO_FIRST      0x1100
#define HDMV_PID_AUDIO_LAST       0x111f
#define HDMV_PID_PG_FIRST         0x1200
#define HDMV_PID_PG_LAST          0x121f
#define HDMV_PID_PG_HDR_FIRST     0x12a0
#define HDMV_PID_PG_HDR_LAST      0x12bf
#define HDMV_PID_IG_FIRST         0x1400
#define HDMV_PID_IG_LAST          0x141f

class CDVDOverlayImage;
class IVideoPlayer;

class CDVDInputStreamBluray
  : public CDVDInputStream
  , public CDVDInputStream::IDisplayTime
  , public CDVDInputStream::IChapter
  , public CDVDInputStream::IPosTime
  , public CDVDInputStream::IMenus
{
public:
  CDVDInputStreamBluray() = delete;
  CDVDInputStreamBluray(IVideoPlayer* player, const CFileItem& fileitem);
  ~CDVDInputStreamBluray() override;
  bool Open() override;
  void Close() override;
  int Read(uint8_t* buf, int buf_size) override;
  int64_t Seek(int64_t offset, int whence) override;
  void Abort() override;
  bool IsEOF() override;
  int64_t GetLength() override;
  int GetBlockSize() override { return 6144; }
  ENextStream NextStream() override;


  /* IMenus */
  void ActivateButton() override { UserInput(BD_VK_ENTER); }
  void SelectButton(int iButton) override
  {
    if(iButton < 10)
      UserInput((bd_vk_key_e)(BD_VK_0 + iButton));
  }
  int  GetCurrentButton() override { return 0; }
  int  GetTotalButtons() override { return 0; }
  void OnUp() override  { UserInput(BD_VK_UP); }
  void OnDown() override  { UserInput(BD_VK_DOWN); }
  void OnLeft() override { UserInput(BD_VK_LEFT); }
  void OnRight() override { UserInput(BD_VK_RIGHT); }

  /*! \brief Open the Menu
  * \return true if the menu is successfully opened, false otherwise
  */
  bool OnMenu() override;
  void OnBack() override
  {
    if(IsInMenu())
      OnMenu();
  }
  void OnNext() override {}
  void OnPrevious() override {}

  /*!
   * \brief Get the supported menu type
   * \return The supported menu type
  */
  MenuType GetSupportedMenuType() override;

  bool IsInMenu() override;
  bool OnMouseMove(const CPoint &point) override  { return MouseMove(point); }
  bool OnMouseClick(const CPoint &point) override { return MouseClick(point); }
  void SkipStill() override;
  bool GetState(std::string& xmlstate) override;
  bool SetState(const std::string& xmlstate) override;
  bool CanSeek() override;


  void UserInput(bd_vk_key_e vk);
  bool MouseMove(const CPoint &point);
  bool MouseClick(const CPoint &point);

  int GetChapter() override;
  int GetChapterCount() override;
  void GetChapterName(std::string& name, int ch=-1) override {};
  int64_t GetChapterPos(int ch) override;
  bool SeekChapter(int ch) override;

  CDVDInputStream::IDisplayTime* GetIDisplayTime() override { return this; }
  int GetTotalTime() override;
  int GetTime() override;

  CDVDInputStream::IPosTime* GetIPosTime() override { return this; }
  bool PosTime(int ms) override;

  void GetStreamInfo(int pid, std::string &language);

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

  IVideoPlayer* m_player = nullptr;
  BLURAY* m_bd = nullptr;
  const BLURAY_TITLE* m_title = nullptr;
  BLURAY_TITLE_INFO* m_titleInfo = nullptr;
  uint32_t m_playlist = MAX_PLAYLIST_ID + 1;
  BLURAY_CLIP_INFO* m_clip = nullptr;
  uint32_t m_angle = 0;
  bool m_menu = false;
  bool m_isInMainMenu = false;
  bool m_hasOverlay = false;
  bool m_navmode = false;
  int m_dispTimeBeforeRead = 0;

  typedef std::shared_ptr<CDVDOverlayImage> SOverlay;
  typedef std::list<SOverlay> SOverlays;

  struct SPlane
  {
    SOverlays o;
    int w = 0;
    int h = 0;
  };

  SPlane m_planes[2];
  enum EHoldState {
    HOLD_NONE = 0,
    HOLD_HELD,
    HOLD_DATA,
    HOLD_STILL,
    HOLD_ERROR,
    HOLD_EXIT
  } m_hold = HOLD_NONE;
  BD_EVENT m_event;
#ifdef HAVE_LIBBLURAY_BDJ
  struct bd_argb_buffer_s m_argb;
#endif

  private:
    bool OpenStream(CFileItem &item);
    void SetupPlayerSettings();
    void FreeTitleInfo();
    std::unique_ptr<CDVDInputStreamFile> m_pstream;
    std::string m_rootPath;

    /*! Bluray state serializer handler */
    CBlurayStateSerializer m_blurayStateSerializer;
};
