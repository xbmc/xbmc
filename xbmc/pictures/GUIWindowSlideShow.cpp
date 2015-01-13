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

#include "threads/SystemClock.h"
#include "system.h"
#include "GUIWindowSlideShow.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "utils/URIUtils.h"
#include "URL.h"
#include "guilib/TextureManager.h"
#include "guilib/GUILabelControl.h"
#include "guilib/Key.h"
#include "GUIInfoManager.h"
#include "filesystem/Directory.h"
#include "GUIDialogPictureInfo.h"
#include "GUIUserMessages.h"
#include "guilib/GUIWindowManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/DisplaySettings.h"
#include "settings/Settings.h"
#include "FileItem.h"
#include "guilib/Texture.h"
#include "windowing/WindowingFactory.h"
#include "guilib/Texture.h"
#include "guilib/LocalizeStrings.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"
#include "interfaces/AnnouncementManager.h"
#include "pictures/PictureInfoTag.h"
#include "pictures/PictureThumbLoader.h"

using namespace XFILE;

#define MAX_ZOOM_FACTOR                     10
#define MAX_PICTURE_SIZE             2048*2048

#define IMMEDIATE_TRANSISTION_TIME          20

#define PICTURE_MOVE_AMOUNT              0.02f
#define PICTURE_MOVE_AMOUNT_ANALOG       0.01f
#define PICTURE_MOVE_AMOUNT_TOUCH        0.002f
#define PICTURE_VIEW_BOX_COLOR      0xffffff00 // YELLOW
#define PICTURE_VIEW_BOX_BACKGROUND 0xff000000 // BLACK

#define ROTATION_SNAP_RANGE              10.0f

#define FPS                                 25

#define BAR_IMAGE                            1
#define LABEL_ROW1                          10
#define LABEL_ROW2                          11
#define LABEL_ROW2_EXTRA                    12
#define CONTROL_PAUSE                       13

static float zoomamount[10] = { 1.0f, 1.2f, 1.5f, 2.0f, 2.8f, 4.0f, 6.0f, 9.0f, 13.5f, 20.0f };

CBackgroundPicLoader::CBackgroundPicLoader() : CThread("BgPicLoader")
{
  m_pCallback = NULL;
  m_isLoading = false;
}

CBackgroundPicLoader::~CBackgroundPicLoader()
{
  StopThread();
}

void CBackgroundPicLoader::Create(CGUIWindowSlideShow *pCallback)
{
  m_pCallback = pCallback;
  m_isLoading = false;
  CThread::Create(false);
}

void CBackgroundPicLoader::Process()
{
  unsigned int totalTime = 0;
  unsigned int count = 0;
  while (!m_bStop)
  { // loop around forever, waiting for the app to call LoadPic
    if (AbortableWait(m_loadPic,10) == WAIT_SIGNALED)
    {
      if (m_pCallback)
      {
        unsigned int start = XbmcThreads::SystemClockMillis();
        CBaseTexture* texture = CTexture::LoadFromFile(m_strFileName, m_maxWidth, m_maxHeight, CSettings::Get().GetBool("pictures.useexifrotation"));
        totalTime += XbmcThreads::SystemClockMillis() - start;
        count++;
        // tell our parent
        bool bFullSize = false;
        if (texture)
        {
          bFullSize = ((int)texture->GetWidth() < m_maxWidth) && ((int)texture->GetHeight() < m_maxHeight);
          if (!bFullSize)
          {
            int iSize = texture->GetWidth() * texture->GetHeight() - MAX_PICTURE_SIZE;
            if ((iSize + (int)texture->GetWidth() > 0) || (iSize + (int)texture->GetHeight() > 0))
              bFullSize = true;
            if (!bFullSize && texture->GetWidth() == g_Windowing.GetMaxTextureSize())
              bFullSize = true;
            if (!bFullSize && texture->GetHeight() == g_Windowing.GetMaxTextureSize())
              bFullSize = true;
          }
        }
        m_pCallback->OnLoadPic(m_iPic, m_iSlideNumber, m_strFileName, texture, bFullSize);
        m_isLoading = false;
      }
    }
  }
  if (count > 0)
    CLog::Log(LOGDEBUG, "Time for loading %u images: %u ms, average %u ms",
              count, totalTime, totalTime / count);
}

void CBackgroundPicLoader::LoadPic(int iPic, int iSlideNumber, const std::string &strFileName, const int maxWidth, const int maxHeight)
{
  m_iPic = iPic;
  m_iSlideNumber = iSlideNumber;
  m_strFileName = strFileName;
  m_maxWidth = maxWidth;
  m_maxHeight = maxHeight;
  m_isLoading = true;
  m_loadPic.Set();
}

CGUIWindowSlideShow::CGUIWindowSlideShow(void)
    : CGUIWindow(WINDOW_SLIDESHOW, "SlideShow.xml")
{
  m_pBackgroundLoader = NULL;
  m_slides = new CFileItemList;
  m_Resolution = RES_INVALID;
  m_loadType = KEEP_IN_MEMORY;
  Reset();
}

CGUIWindowSlideShow::~CGUIWindowSlideShow(void)
{
  Reset();
  delete m_slides;
}

void CGUIWindowSlideShow::AnnouncePlayerPlay(const CFileItemPtr& item)
{
  CVariant param;
  param["player"]["speed"] = m_bSlideShow && !m_bPause ? 1 : 0;
  param["player"]["playerid"] = PLAYLIST_PICTURE;
  ANNOUNCEMENT::CAnnouncementManager::Get().Announce(ANNOUNCEMENT::Player, "xbmc", "OnPlay", item, param);
}

void CGUIWindowSlideShow::AnnouncePlayerPause(const CFileItemPtr& item)
{
  CVariant param;
  param["player"]["speed"] = 0;
  param["player"]["playerid"] = PLAYLIST_PICTURE;
  ANNOUNCEMENT::CAnnouncementManager::Get().Announce(ANNOUNCEMENT::Player, "xbmc", "OnPause", item, param);
}

void CGUIWindowSlideShow::AnnouncePlayerStop(const CFileItemPtr& item)
{
  CVariant param;
  param["player"]["playerid"] = PLAYLIST_PICTURE;
  param["end"] = true;
  ANNOUNCEMENT::CAnnouncementManager::Get().Announce(ANNOUNCEMENT::Player, "xbmc", "OnStop", item, param);
}

void CGUIWindowSlideShow::AnnouncePlaylistRemove(int pos)
{
  CVariant data;
  data["playlistid"] = PLAYLIST_PICTURE;
  data["position"] = pos;
  ANNOUNCEMENT::CAnnouncementManager::Get().Announce(ANNOUNCEMENT::Playlist, "xbmc", "OnRemove", data);
}

void CGUIWindowSlideShow::AnnouncePlaylistClear()
{
  CVariant data;
  data["playlistid"] = PLAYLIST_PICTURE;
  ANNOUNCEMENT::CAnnouncementManager::Get().Announce(ANNOUNCEMENT::Playlist, "xbmc", "OnClear", data);
}

void CGUIWindowSlideShow::AnnouncePlaylistAdd(const CFileItemPtr& item, int pos)
{
  CVariant data;
  data["playlistid"] = PLAYLIST_PICTURE;
  data["position"] = pos;
  ANNOUNCEMENT::CAnnouncementManager::Get().Announce(ANNOUNCEMENT::Playlist, "xbmc", "OnAdd", item, data);
}

void CGUIWindowSlideShow::AnnouncePropertyChanged(const std::string &strProperty, const CVariant &value)
{
  if (strProperty.empty() || value.isNull())
    return;

  CVariant data;
  data["player"]["playerid"] = PLAYLIST_PICTURE;
  data["property"][strProperty] = value;
  ANNOUNCEMENT::CAnnouncementManager::Get().Announce(ANNOUNCEMENT::Player, "xbmc", "OnPropertyChanged", data);
}

bool CGUIWindowSlideShow::IsPlaying() const
{
  return m_Image[m_iCurrentPic].IsLoaded();
}

void CGUIWindowSlideShow::Reset()
{
  g_infoManager.SetShowCodec(false);
  m_bSlideShow = false;
  m_bShuffled = false;
  m_bPause = false;
  m_bPlayingVideo = false;
  m_bErrorMessage = false;
  m_Image[0].UnLoad();
  m_Image[0].Close();
  m_Image[1].UnLoad();
  m_Image[1].Close();

  m_fRotate = 0.0f;
  m_fInitialRotate = 0.0f;
  m_iZoomFactor = 1;
  m_fZoom = 1.0f;
  m_fInitialZoom = 0.0f;
  m_iCurrentSlide = 0;
  m_iNextSlide = 1;
  m_iCurrentPic = 0;
  m_iDirection = 1;
  m_iLastFailedNextSlide = -1;
  CSingleLock lock(m_slideSection);
  m_slides->Clear();
  AnnouncePlaylistClear();
  m_Resolution = g_graphicsContext.GetVideoResolution();
}

void CGUIWindowSlideShow::OnDeinitWindow(int nextWindowID)
{ 
  if (m_Resolution != CDisplaySettings::Get().GetCurrentResolution())
  {
    //FIXME: Use GUI resolution for now
    //g_graphicsContext.SetVideoResolution(CDisplaySettings::Get().GetCurrentResolution(), TRUE);
  }

  //   Reset();
  if (nextWindowID != WINDOW_PICTURES)
    m_ImageLib.Unload();

  g_windowManager.ShowOverlay(OVERLAY_STATE_SHOWN);

  if (nextWindowID != WINDOW_FULLSCREEN_VIDEO)
  {
    // wait for any outstanding picture loads
    if (m_pBackgroundLoader)
    {
      // sleep until the loader finishes loading the current pic
      CLog::Log(LOGDEBUG,"Waiting for BackgroundLoader thread to close");
      while (m_pBackgroundLoader->IsLoading())
        Sleep(10);
      // stop the thread
      CLog::Log(LOGDEBUG,"Stopping BackgroundLoader thread");
      m_pBackgroundLoader->StopThread();
      delete m_pBackgroundLoader;
      m_pBackgroundLoader = NULL;
    }
    // and close the images.
    m_Image[0].Close();
    m_Image[1].Close();
  }
  g_infoManager.ResetCurrentSlide();

  CGUIWindow::OnDeinitWindow(nextWindowID);
}

void CGUIWindowSlideShow::Add(const CFileItem *picture)
{
  CFileItemPtr item(new CFileItem(*picture));
  if (!item->HasVideoInfoTag() && !item->HasPictureInfoTag())
  {
    // item without tag; get mimetype then we can tell whether it's video item
    item->FillInMimeType();

    if (!item->IsVideo())
      // then it is a picture and force tag generation
      item->GetPictureInfoTag();
  }
  AnnouncePlaylistAdd(item, m_slides->Size());

  m_slides->Add(item);
}

void CGUIWindowSlideShow::ShowNext()
{
  if (m_slides->Size() == 1)
    return;

  m_iDirection   = 1;
  m_iNextSlide   = GetNextSlide();
  m_iZoomFactor  = 1;
  m_fZoom        = 1.0f;
  m_fRotate      = 0.0f;
  m_bLoadNextPic = true;
}

void CGUIWindowSlideShow::ShowPrevious()
{
  if (m_slides->Size() == 1)
    return;

  m_iDirection   = -1;
  m_iNextSlide   = GetNextSlide();
  m_iZoomFactor  = 1;
  m_fZoom        = 1.0f;
  m_fRotate      = 0.0f;
  m_bLoadNextPic = true;
}


void CGUIWindowSlideShow::Select(const std::string& strPicture)
{
  for (int i = 0; i < m_slides->Size(); ++i)
  {
    const CFileItemPtr item = m_slides->Get(i);
    if (item->GetPath() == strPicture)
    {
      m_iDirection = 1;
      if (!m_Image[m_iCurrentPic].IsLoaded() && (!m_pBackgroundLoader || !m_pBackgroundLoader->IsLoading()))
      {
        // will trigger loading current slide when next Process call.
        m_iCurrentSlide = i;
        m_iNextSlide = GetNextSlide();
      }
      else
      {
        m_iNextSlide = i;
        m_bLoadNextPic = true;
      }
      return ;
    }
  }
}

const CFileItemList &CGUIWindowSlideShow::GetSlideShowContents()
{
  return *m_slides;
}

void CGUIWindowSlideShow::GetSlideShowContents(CFileItemList &list)
{
  for (int index = 0; index < m_slides->Size(); index++)
    list.Add(CFileItemPtr(new CFileItem(*m_slides->Get(index))));
}

const CFileItemPtr CGUIWindowSlideShow::GetCurrentSlide()
{
  if (m_iCurrentSlide >= 0 && m_iCurrentSlide < m_slides->Size())
    return m_slides->Get(m_iCurrentSlide);
  return CFileItemPtr();
}

bool CGUIWindowSlideShow::InSlideShow() const
{
  return m_bSlideShow;
}

void CGUIWindowSlideShow::StartSlideShow()
{
  m_bSlideShow = true;
  m_iDirection = 1;
  if (m_slides->Size())
    AnnouncePlayerPlay(m_slides->Get(m_iCurrentSlide));
}

void CGUIWindowSlideShow::SetDirection(int direction)
{
  direction = direction >= 0 ? 1 : -1;
  if (m_iDirection != direction)
  {
    m_iDirection = direction;
    m_iNextSlide = GetNextSlide();
  }
}

void CGUIWindowSlideShow::Process(unsigned int currentTime, CDirtyRegionList &regions)
{
  const RESOLUTION_INFO res = g_graphicsContext.GetResInfo();

  // reset the screensaver if we're in a slideshow
  // (unless we are the screensaver!)
  if (m_bSlideShow && !m_bPause && !g_application.IsInScreenSaver())
    g_application.ResetScreenSaver();
  int iSlides = m_slides->Size();
  if (!iSlides) return ;

  // if we haven't processed yet, we should mark the whole screen
  if (!HasProcessed())
    regions.push_back(CRect(0.0f, 0.0f, (float)g_graphicsContext.GetWidth(), (float)g_graphicsContext.GetHeight()));

  if (m_iCurrentSlide < 0 || m_iCurrentSlide >= m_slides->Size())
    m_iCurrentSlide = 0;
  if (m_iNextSlide < 0 || m_iNextSlide >= m_slides->Size())
    m_iNextSlide = GetNextSlide();

  // Create our background loader if necessary
  if (!m_pBackgroundLoader)
  {
    m_pBackgroundLoader = new CBackgroundPicLoader();

    if (!m_pBackgroundLoader)
    {
      throw 1;
    }
    m_pBackgroundLoader->Create(this);
  }

  bool bSlideShow = m_bSlideShow && !m_bPause && !m_bPlayingVideo;
  if (bSlideShow && m_slides->Get(m_iCurrentSlide)->HasProperty("unplayable"))
  {
    m_iNextSlide    = GetNextSlide();
    if (m_iCurrentSlide == m_iNextSlide)
      return;
    m_iCurrentSlide = m_iNextSlide;
    m_iNextSlide    = GetNextSlide();
  }

  if (m_bErrorMessage)
  { // we have an error when loading either the current or next picture
    // check to see if we have a picture loaded
    CLog::Log(LOGDEBUG, "We have an error loading picture %d!", m_pBackgroundLoader->SlideNumber());
    if (m_iCurrentSlide == m_pBackgroundLoader->SlideNumber())
    {
      if (m_Image[m_iCurrentPic].IsLoaded())
      {
        // current image was already loaded, so we can ignore this error.
        m_bErrorMessage = false;
      }
      else
      {
        CLog::Log(LOGERROR, "Error loading the current image %d: %s", m_iCurrentSlide, m_slides->Get(m_iCurrentSlide)->GetPath().c_str());
        if (!m_slides->Get(m_iCurrentPic)->IsVideo())
        {
          // try next if we are in slideshow
          CLog::Log(LOGINFO, "set image %s unplayable", m_slides->Get(m_iCurrentSlide)->GetPath().c_str());
          m_slides->Get(m_iCurrentSlide)->SetProperty("unplayable", true);
        }
        if (m_bLoadNextPic || (bSlideShow && !m_bPause && !m_slides->Get(m_iCurrentPic)->IsVideo()))
        {
          // change to next item, wait loading.
          m_iCurrentSlide = m_iNextSlide;
          m_iNextSlide    = GetNextSlide();
          m_bErrorMessage = false;
        }
        // else just drop through - there's nothing we can do (error message will be displayed)
      }
    }
    else if (m_iNextSlide == m_pBackgroundLoader->SlideNumber())
    {
      CLog::Log(LOGERROR, "Error loading the next image %d: %s", m_iNextSlide, m_slides->Get(m_iNextSlide)->GetPath().c_str());
      // load next image failed, then skip to load next of next if next is not video.
      if (!m_slides->Get(m_iNextSlide)->IsVideo())
      {
        CLog::Log(LOGINFO, "set image %s unplayable", m_slides->Get(m_iNextSlide)->GetPath().c_str());
        m_slides->Get(m_iNextSlide)->SetProperty("unplayable", true);
        // change to next item, wait loading.
        m_iNextSlide = GetNextSlide();
      }
      else
      { // prevent reload the next pic and repeat fail.
        m_iLastFailedNextSlide = m_iNextSlide;
      }
      m_bErrorMessage = false;
    }
    else
    { // Non-current and non-next slide, just ignore error.
      CLog::Log(LOGERROR, "Error loading the non-current non-next image %d/%d: %s", m_iNextSlide, m_pBackgroundLoader->SlideNumber(), m_slides->Get(m_iNextSlide)->GetPath().c_str());
      m_bErrorMessage = false;
    }
  }

  if (m_bErrorMessage)
  { // hack, just mark it all
    regions.push_back(CRect(0.0f, 0.0f, (float)g_graphicsContext.GetWidth(), (float)g_graphicsContext.GetHeight()));
    return;
  }

  CSingleLock lock(m_slideSection);

  if (!m_Image[m_iCurrentPic].IsLoaded() && !m_pBackgroundLoader->IsLoading())
  { // load first image
    CFileItemPtr item = m_slides->Get(m_iCurrentSlide);
    std::string picturePath = GetPicturePath(item.get());
    if (!picturePath.empty())
    {
      if (item->IsVideo())
        CLog::Log(LOGDEBUG, "Loading the thumb %s for current video %d: %s", picturePath.c_str(), m_iCurrentSlide, item->GetPath().c_str());
      else
        CLog::Log(LOGDEBUG, "Loading the current image %d: %s", m_iCurrentSlide, item->GetPath().c_str());

      // load using the background loader
      int maxWidth, maxHeight;

      GetCheckedSize((float)res.iWidth * m_fZoom,
                     (float)res.iHeight * m_fZoom,
                     maxWidth, maxHeight);
      m_pBackgroundLoader->LoadPic(m_iCurrentPic, m_iCurrentSlide, picturePath, maxWidth, maxHeight);
      m_iLastFailedNextSlide = -1;
      m_bLoadNextPic = false;
    }
  }

  // check if we should discard an already loaded next slide
  if (m_Image[1 - m_iCurrentPic].IsLoaded() && m_Image[1 - m_iCurrentPic].SlideNumber() != m_iNextSlide)
    m_Image[1 - m_iCurrentPic].Close();

  if (m_iNextSlide != m_iCurrentSlide && m_Image[m_iCurrentPic].IsLoaded() && !m_Image[1 - m_iCurrentPic].IsLoaded() && !m_pBackgroundLoader->IsLoading() && m_iLastFailedNextSlide != m_iNextSlide)
  { // load the next image
    m_iLastFailedNextSlide = -1;
    CFileItemPtr item = m_slides->Get(m_iNextSlide);
    std::string picturePath = GetPicturePath(item.get());
    if (!picturePath.empty() && (!item->IsVideo() || !m_bSlideShow || m_bPause))
    {
      if (item->IsVideo())
        CLog::Log(LOGDEBUG, "Loading the thumb %s for next video %d: %s", picturePath.c_str(), m_iNextSlide, item->GetPath().c_str());
      else
        CLog::Log(LOGDEBUG, "Loading the next image %d: %s", m_iNextSlide, item->GetPath().c_str());
      
      int maxWidth, maxHeight;
      GetCheckedSize((float)res.iWidth * m_fZoom,
                     (float)res.iHeight * m_fZoom,
                     maxWidth, maxHeight);
      m_pBackgroundLoader->LoadPic(1 - m_iCurrentPic, m_iNextSlide, picturePath, maxWidth, maxHeight);
    }
  }

  if (m_slides->Get(m_iCurrentSlide)->IsVideo() && bSlideShow)
  {
    if (!PlayVideo())
      return;
    bSlideShow = false;
  }
  
  // render the current image
  if (m_Image[m_iCurrentPic].IsLoaded())
  {
    m_Image[m_iCurrentPic].SetInSlideshow(bSlideShow);
    m_Image[m_iCurrentPic].Pause(!bSlideShow);
    m_Image[m_iCurrentPic].Process(currentTime, regions);
  }

  // Check if we should be transistioning immediately
  if (m_bLoadNextPic && m_Image[m_iCurrentPic].IsLoaded())
  {
    CLog::Log(LOGDEBUG, "Starting immediate transistion due to user wanting slide %s", m_slides->Get(m_iNextSlide)->GetPath().c_str());
    if (m_Image[m_iCurrentPic].StartTransistion())
    {
      m_Image[m_iCurrentPic].SetTransistionTime(1, IMMEDIATE_TRANSISTION_TIME); // only 20 frames for the transistion
      m_bLoadNextPic = false;
    }
  }

  // render the next image
  if (m_Image[m_iCurrentPic].DrawNextImage())
  {
    if (m_bSlideShow && !m_bPause && m_slides->Get(m_iNextSlide)->IsVideo())
    {
      // do not show thumb of video when playing slideshow
    }
    else if (m_Image[1 - m_iCurrentPic].IsLoaded())
    {
      // first time render the next image, make sure using current display effect.
      if (!m_Image[1 - m_iCurrentPic].IsStarted())
      {
        CSlideShowPic::DISPLAY_EFFECT effect = GetDisplayEffect(m_iNextSlide);
        if (m_Image[1 - m_iCurrentPic].DisplayEffectNeedChange(effect))
          m_Image[1 - m_iCurrentPic].Reset(effect);
      }
      // set the appropriate transistion time
      m_Image[1 - m_iCurrentPic].SetTransistionTime(0, m_Image[m_iCurrentPic].GetTransistionTime(1));
      m_Image[1 - m_iCurrentPic].Pause(!m_bSlideShow || m_bPause || m_slides->Get(m_iNextSlide)->IsVideo());
      m_Image[1 - m_iCurrentPic].Process(currentTime, regions);
    }
    else // next pic isn't loaded.  We should hang around if it is in progress
    {
      if (m_pBackgroundLoader->IsLoading())
      {
//        CLog::Log(LOGDEBUG, "Having to hold the current image (%s) while we load %s", m_vecSlides[m_iCurrentSlide].c_str(), m_vecSlides[m_iNextSlide].c_str());
        m_Image[m_iCurrentPic].Keep();
      }
    }
  }

  // check if we should swap images now
  if (m_Image[m_iCurrentPic].IsFinished() || (m_bLoadNextPic && !m_Image[m_iCurrentPic].IsLoaded()))
  {
    m_bLoadNextPic = false;
    if (m_Image[m_iCurrentPic].IsFinished())
      CLog::Log(LOGDEBUG, "Image %s is finished rendering, switching to %s", m_slides->Get(m_iCurrentSlide)->GetPath().c_str(), m_slides->Get(m_iNextSlide)->GetPath().c_str());
    else
      // what if it's bg loading?
      CLog::Log(LOGDEBUG, "Image %s is not loaded, switching to %s", m_slides->Get(m_iCurrentSlide)->GetPath().c_str(), m_slides->Get(m_iNextSlide)->GetPath().c_str());

    if (m_Image[m_iCurrentPic].IsFinished() && m_iCurrentSlide == m_iNextSlide && m_Image[m_iCurrentPic].SlideNumber() == m_iNextSlide)
      m_Image[m_iCurrentPic].Reset(GetDisplayEffect(m_iCurrentSlide));
    else
    {
      if (m_Image[m_iCurrentPic].IsLoaded())
        m_Image[m_iCurrentPic].Reset(GetDisplayEffect(m_iCurrentSlide));
      else
        m_Image[m_iCurrentPic].Close();

      if ((m_Image[1 - m_iCurrentPic].IsLoaded() && m_Image[1 - m_iCurrentPic].SlideNumber() == m_iNextSlide) ||
          (m_pBackgroundLoader->IsLoading() && m_pBackgroundLoader->SlideNumber() == m_iNextSlide && m_pBackgroundLoader->Pic() == 1 - m_iCurrentPic))
      {
        m_iCurrentPic = 1 - m_iCurrentPic;
      }
      else
      {
        m_Image[1 - m_iCurrentPic].Close();
        m_iCurrentPic = 1 - m_iCurrentPic;
      }
      m_iCurrentSlide = m_iNextSlide;
      m_iNextSlide    = GetNextSlide();
    }
    AnnouncePlayerPlay(m_slides->Get(m_iCurrentSlide));

    m_iZoomFactor = 1;
    m_fZoom = 1.0f;
    m_fRotate = 0.0f;
  }

  if (m_Image[m_iCurrentPic].IsLoaded())
    g_infoManager.SetCurrentSlide(*m_slides->Get(m_iCurrentSlide));

  RenderPause();
  CGUIWindow::Process(currentTime, regions);
}

void CGUIWindowSlideShow::Render()
{
  if (m_Image[m_iCurrentPic].IsLoaded())
    m_Image[m_iCurrentPic].Render();

  if (m_Image[m_iCurrentPic].DrawNextImage() && m_Image[1 - m_iCurrentPic].IsLoaded())
    m_Image[1 - m_iCurrentPic].Render();

  RenderErrorMessage();
  CGUIWindow::Render();
}

int CGUIWindowSlideShow::GetNextSlide()
{
  if (m_slides->Size() <= 1)
    return m_iCurrentSlide;
  int step = m_iDirection >= 0 ? 1 : -1;
  int nextSlide = (m_iCurrentSlide + step + m_slides->Size()) % m_slides->Size();
  while (nextSlide != m_iCurrentSlide)
  {
    if (!m_slides->Get(nextSlide)->HasProperty("unplayable"))
      return nextSlide;
    nextSlide = (nextSlide + step + m_slides->Size()) % m_slides->Size();
  }
  return m_iCurrentSlide;
}

EVENT_RESULT CGUIWindowSlideShow::OnMouseEvent(const CPoint &point, const CMouseEvent &event)
{
  if (event.m_id == ACTION_GESTURE_NOTIFY)
  {
    int result = EVENT_RESULT_ROTATE | EVENT_RESULT_ZOOM;
    if (m_iZoomFactor == 1 || !m_Image[m_iCurrentPic].m_bCanMoveHorizontally)
      result |= EVENT_RESULT_SWIPE;
    else
      result |= EVENT_RESULT_PAN_HORIZONTAL;

    if (m_Image[m_iCurrentPic].m_bCanMoveVertically)
      result |= EVENT_RESULT_PAN_VERTICAL;

    return (EVENT_RESULT)result;
  }  
  else if (event.m_id == ACTION_GESTURE_BEGIN)
  {
    m_firstGesturePoint = point;
    m_fInitialZoom = m_fZoom;
    m_fInitialRotate = m_fRotate;
    return EVENT_RESULT_HANDLED;
  }
  else if (event.m_id == ACTION_GESTURE_PAN)
  {
    // zoomed in - free move mode
    if (m_iZoomFactor != 1 &&
       (m_Image[m_iCurrentPic].m_bCanMoveHorizontally || m_Image[m_iCurrentPic].m_bCanMoveVertically))
    {
      Move(PICTURE_MOVE_AMOUNT_TOUCH / m_iZoomFactor * (m_firstGesturePoint.x - point.x), PICTURE_MOVE_AMOUNT_TOUCH / m_iZoomFactor * (m_firstGesturePoint.y - point.y));
      m_firstGesturePoint = point;
    }
    return EVENT_RESULT_HANDLED;
  }
  else if (event.m_id == ACTION_GESTURE_SWIPE_LEFT || event.m_id == ACTION_GESTURE_SWIPE_RIGHT)
  {
    if (m_iZoomFactor == 1 || !m_Image[m_iCurrentPic].m_bCanMoveHorizontally)
    {
      // on zoomlevel 1 just detect swipe left and right
      if (event.m_id == ACTION_GESTURE_SWIPE_LEFT)
        OnAction(CAction(ACTION_NEXT_PICTURE));
      else
        OnAction(CAction(ACTION_PREV_PICTURE));
    }
  }
  else if (event.m_id == ACTION_GESTURE_END)
  {
    if (m_fRotate != 0.0f)
    {
      // "snap" to nearest of 0, 90, 180 and 270 if the
      // difference in angle is +/-10 degrees
      float reminder = fmodf(m_fRotate, 90.0f);
      if (fabs(reminder) < ROTATION_SNAP_RANGE)
        Rotate(-reminder);
      else if (reminder > 90.0f - ROTATION_SNAP_RANGE)
        Rotate(90.0f - reminder);
      else if (-reminder > 90.0f - ROTATION_SNAP_RANGE)
        Rotate(-90.0f - reminder);
    }

    m_fInitialZoom = 0.0f;
    m_fInitialRotate = 0.0f;
    return EVENT_RESULT_HANDLED;
  }
  else if (event.m_id == ACTION_GESTURE_ZOOM)
  {
    ZoomRelative(m_fInitialZoom * event.m_offsetX, true);
    return EVENT_RESULT_HANDLED;
  }
  else if (event.m_id == ACTION_GESTURE_ROTATE)
  {
    Rotate(m_fInitialRotate + event.m_offsetX - m_fRotate, true);
    return EVENT_RESULT_HANDLED;
  }
  return EVENT_RESULT_UNHANDLED;
}

bool CGUIWindowSlideShow::OnAction(const CAction &action)
{
  switch (action.GetID())
  {
  case ACTION_SHOW_CODEC:
    {
      CGUIDialogPictureInfo *pictureInfo = (CGUIDialogPictureInfo *)g_windowManager.GetWindow(WINDOW_DIALOG_PICTURE_INFO);
      if (pictureInfo)
      {
        // no need to set the picture here, it's done in Render()
        pictureInfo->DoModal();
      }
    }
    break;

  case ACTION_PREVIOUS_MENU:
  case ACTION_NAV_BACK:
  case ACTION_STOP:
    if (m_slides->Size())
      AnnouncePlayerStop(m_slides->Get(m_iCurrentSlide));
    g_windowManager.PreviousWindow();
    break;

  case ACTION_NEXT_PICTURE:
      ShowNext();
    break;

  case ACTION_PREV_PICTURE:
      ShowPrevious();
    break;

  case ACTION_MOVE_RIGHT:
    if (m_iZoomFactor == 1 || !m_Image[m_iCurrentPic].m_bCanMoveHorizontally)
      ShowNext();
    else
      Move(PICTURE_MOVE_AMOUNT, 0);
    break;

  case ACTION_MOVE_LEFT:
    if (m_iZoomFactor == 1 || !m_Image[m_iCurrentPic].m_bCanMoveHorizontally)
      ShowPrevious();
    else
      Move( -PICTURE_MOVE_AMOUNT, 0);
    break;

  case ACTION_MOVE_DOWN:
    Move(0, PICTURE_MOVE_AMOUNT);
    break;

  case ACTION_MOVE_UP:
    Move(0, -PICTURE_MOVE_AMOUNT);
    break;

  case ACTION_PAUSE:
  case ACTION_PLAYER_PLAY:
    if (m_slides->Size() == 0)
      break;
    if (m_slides->Get(m_iCurrentSlide)->IsVideo())
    {
      if (!m_bPlayingVideo)
      {
        if (m_bSlideShow)
        {
          SetDirection(1);
          m_bPause = false;
        }
        PlayVideo();
      }
    }
    else if (!m_bSlideShow || m_bPause)
    {
      m_bSlideShow = true;
      m_bPause = false;
      SetDirection(1);
      if (m_Image[m_iCurrentPic].IsLoaded())
      {
        CSlideShowPic::DISPLAY_EFFECT effect = GetDisplayEffect(m_iCurrentSlide);
        if (m_Image[m_iCurrentPic].DisplayEffectNeedChange(effect))
          m_Image[m_iCurrentPic].Reset(effect);
      }
      AnnouncePlayerPlay(m_slides->Get(m_iCurrentSlide));
    }
    else if (action.GetID() == ACTION_PAUSE)
    {
      m_bPause = true;
      AnnouncePlayerPause(m_slides->Get(m_iCurrentSlide));
    }
    break;

  case ACTION_ZOOM_OUT:
    Zoom(m_iZoomFactor - 1);
    break;

  case ACTION_ZOOM_IN:
    Zoom(m_iZoomFactor + 1);
    break;

  case ACTION_GESTURE_SWIPE_UP:
  case ACTION_GESTURE_SWIPE_DOWN:
    if (m_iZoomFactor == 1 || !m_Image[m_iCurrentPic].m_bCanMoveVertically)
    {
      bool swipeOnLeft = action.GetAmount() < g_graphicsContext.GetWidth() / 2;
      bool swipeUp = action.GetID() == ACTION_GESTURE_SWIPE_UP;
      if (swipeUp == swipeOnLeft)
        Rotate(90.0f);
      else
        Rotate(-90.0f);
    }
    break;

  case ACTION_ROTATE_PICTURE_CW:
    Rotate(90.0f);
    break;

  case ACTION_ROTATE_PICTURE_CCW:
    Rotate(-90.0f);
    break;

  case ACTION_ZOOM_LEVEL_NORMAL:
  case ACTION_ZOOM_LEVEL_1:
  case ACTION_ZOOM_LEVEL_2:
  case ACTION_ZOOM_LEVEL_3:
  case ACTION_ZOOM_LEVEL_4:
  case ACTION_ZOOM_LEVEL_5:
  case ACTION_ZOOM_LEVEL_6:
  case ACTION_ZOOM_LEVEL_7:
  case ACTION_ZOOM_LEVEL_8:
  case ACTION_ZOOM_LEVEL_9:
    Zoom((action.GetID() - ACTION_ZOOM_LEVEL_NORMAL) + 1);
    break;

  case ACTION_ANALOG_MOVE:
    Move(action.GetAmount()*PICTURE_MOVE_AMOUNT_ANALOG, -action.GetAmount(1)*PICTURE_MOVE_AMOUNT_ANALOG);
    break;

  default:
    return CGUIWindow::OnAction(action);
  }
  return true;
}

void CGUIWindowSlideShow::RenderErrorMessage()
{
  if (!m_bErrorMessage)
    return ;

  const CGUIControl *control = GetControl(LABEL_ROW1);
  if (NULL == control || control->GetControlType() != CGUIControl::GUICONTROL_LABEL)
  {
     return;
  }

  CGUIFont *pFont = ((CGUILabelControl *)control)->GetLabelInfo().font;
  CGUITextLayout::DrawText(pFont, 0.5f*g_graphicsContext.GetWidth(), 0.5f*g_graphicsContext.GetHeight(), 0xffffffff, 0, g_localizeStrings.Get(747), XBFONT_CENTER_X | XBFONT_CENTER_Y);
}

bool CGUIWindowSlideShow::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
    {
      m_Resolution = (RESOLUTION) CSettings::Get().GetInt("pictures.displayresolution");

      //FIXME: Use GUI resolution for now
      if (0 /*m_Resolution != CDisplaySettings::Get().GetCurrentResolution() && m_Resolution != INVALID && m_Resolution!=AUTORES*/)
        g_graphicsContext.SetVideoResolution(m_Resolution);
      else
        m_Resolution = g_graphicsContext.GetVideoResolution();

      CGUIWindow::OnMessage(message);
      if (message.GetParam1() != WINDOW_PICTURES)
        m_ImageLib.Load();

      g_windowManager.ShowOverlay(OVERLAY_STATE_HIDDEN);

      // turn off slideshow if we only have 1 image
      if (m_slides->Size() <= 1)
        m_bSlideShow = false;

      return true;
    }
    break;

  case GUI_MSG_SHOW_PICTURE:
    {
      std::string strFile = message.GetStringParam();
      Reset();
      CFileItem item(strFile, false);
      Add(&item);
      RunSlideShow("", false, false, true, "", false);
    }
    break;

  case GUI_MSG_START_SLIDESHOW:
    {
      std::string strFolder = message.GetStringParam();
      unsigned int iParams = message.GetParam1();
      std::string beginSlidePath = message.GetStringParam(1);
      //decode params
      bool bRecursive = false;
      bool bRandom = false;
      bool bNotRandom = false;
      bool bPause = false;
      if (iParams > 0)
      {
        if ((iParams & 1) == 1)
          bRecursive = true;
        if ((iParams & 2) == 2)
          bRandom = true;
        if ((iParams & 4) == 4)
          bNotRandom = true;
        if ((iParams & 8) == 8)
          bPause = true;
      }
      RunSlideShow(strFolder, bRecursive, bRandom, bNotRandom, beginSlidePath, !bPause);
    }
    break;

    case GUI_MSG_PLAYLISTPLAYER_STOPPED:
      {
      }
      break;

    case GUI_MSG_PLAYBACK_STARTED:
      {
        if (m_bPlayingVideo)
          g_windowManager.ActivateWindow(WINDOW_FULLSCREEN_VIDEO);
      }
      break;

    case GUI_MSG_PLAYBACK_STOPPED:
      {
        if (m_bPlayingVideo)
        {
          m_bPlayingVideo = false;
          if (m_bSlideShow)
            m_bPause = true;
        }
      }
      break;

    case GUI_MSG_PLAYBACK_ENDED:
      {
        if (m_bPlayingVideo)
        {
          m_bPlayingVideo = false;
          if (m_bSlideShow)
          {
            m_bPause = false;
            if (m_iCurrentSlide == m_iNextSlide)
              break;
            m_Image[m_iCurrentPic].Close();
            m_iCurrentPic = 1 - m_iCurrentPic;
            m_iCurrentSlide = m_iNextSlide;
            m_iNextSlide    = GetNextSlide();
            AnnouncePlayerPlay(m_slides->Get(m_iCurrentSlide));
            m_iZoomFactor = 1;
            m_fZoom = 1.0f;
            m_fRotate = 0.0f;
          }
        }
      }
      break;
  }
  return CGUIWindow::OnMessage(message);
}

void CGUIWindowSlideShow::RenderPause()
{ // display the pause icon
  if (m_bPause)
  {
    SET_CONTROL_VISIBLE(CONTROL_PAUSE);
  }
  else
  {
    SET_CONTROL_HIDDEN(CONTROL_PAUSE);
  }
  /*
   static DWORD dwCounter=0;
   dwCounter++;
   if (dwCounter > 25)
   {
    dwCounter=0;
   }
   if (!m_bPause) return;
   if (dwCounter <13) return;*/

}

void CGUIWindowSlideShow::Rotate(float fAngle, bool immediate /* = false */)
{
  if (m_Image[m_iCurrentPic].DrawNextImage())
    return;

  m_fRotate += fAngle;

  m_Image[m_iCurrentPic].Rotate(fAngle, immediate);
}

void CGUIWindowSlideShow::Zoom(int iZoom)
{
  if (iZoom > MAX_ZOOM_FACTOR || iZoom < 1)
    return;

  ZoomRelative(zoomamount[iZoom - 1]);
}

void CGUIWindowSlideShow::ZoomRelative(float fZoom, bool immediate /* = false */)
{
  if (fZoom < zoomamount[0])
    fZoom = zoomamount[0];
  else if (fZoom > zoomamount[MAX_ZOOM_FACTOR - 1])
    fZoom = zoomamount[MAX_ZOOM_FACTOR - 1];

  if (m_Image[m_iCurrentPic].DrawNextImage())
    return;

  m_fZoom = fZoom;

  // find the nearest zoom factor
  for (unsigned int i = 1; i < MAX_ZOOM_FACTOR; i++)
  {
    if (m_fZoom > zoomamount[i])
      continue;

    if (fabs(m_fZoom - zoomamount[i - 1]) < fabs(m_fZoom - zoomamount[i]))
      m_iZoomFactor = i;
    else
      m_iZoomFactor = i + 1;

    break;
  }

  m_Image[m_iCurrentPic].Zoom(m_fZoom, immediate);
}

void CGUIWindowSlideShow::Move(float fX, float fY)
{
  if (m_Image[m_iCurrentPic].IsLoaded() && m_Image[m_iCurrentPic].GetZoom() > 1)
  { // we move in the opposite direction, due to the fact we are moving
    // the viewing window, not the picture.
    m_Image[m_iCurrentPic].Move( -fX, -fY);
  }
}

bool CGUIWindowSlideShow::PlayVideo()
{
  CFileItemPtr item = m_slides->Get(m_iCurrentSlide);
  if (!item || !item->IsVideo())
    return false;
  CLog::Log(LOGDEBUG, "Playing current video slide %s", item->GetPath().c_str());
  m_bPlayingVideo = true;
  PlayBackRet ret = g_application.PlayFile(*item);
  if (ret == PLAYBACK_OK)
    return true;
  if (ret == PLAYBACK_FAIL)
  {
    CLog::Log(LOGINFO, "set video %s unplayable", item->GetPath().c_str());
    item->SetProperty("unplayable", true);
  }
  else if (ret == PLAYBACK_CANCELED)
    m_bPause = true;
  m_bPlayingVideo = false;
  return false;
}

CSlideShowPic::DISPLAY_EFFECT CGUIWindowSlideShow::GetDisplayEffect(int iSlideNumber) const
{
  if (m_bSlideShow && !m_bPause && !m_slides->Get(iSlideNumber)->IsVideo())
    return CSettings::Get().GetBool("slideshow.displayeffects") ? CSlideShowPic::EFFECT_RANDOM : CSlideShowPic::EFFECT_NONE;
  else
    return CSlideShowPic::EFFECT_NO_TIMEOUT;
}

void CGUIWindowSlideShow::OnLoadPic(int iPic, int iSlideNumber, const std::string &strFileName, CBaseTexture* pTexture, bool bFullSize)
{
  if (pTexture)
  {
    // set the pic's texture + size etc.
    CSingleLock lock(m_slideSection);
    if (iSlideNumber >= m_slides->Size() || GetPicturePath(m_slides->Get(iSlideNumber).get()) != strFileName)
    { // throw this away - we must have cleared the slideshow while we were still loading
      delete pTexture;
      return;
    }
    CLog::Log(LOGDEBUG, "Finished background loading slot %d, %d: %s", iPic, iSlideNumber, m_slides->Get(iSlideNumber)->GetPath().c_str());
    m_Image[iPic].SetTexture(iSlideNumber, pTexture, GetDisplayEffect(iSlideNumber));
    m_Image[iPic].SetOriginalSize(pTexture->GetOriginalWidth(), pTexture->GetOriginalHeight(), bFullSize);
    
    m_Image[iPic].m_bIsComic = false;
    if (URIUtils::IsInRAR(m_slides->Get(m_iCurrentSlide)->GetPath()) || URIUtils::IsInZIP(m_slides->Get(m_iCurrentSlide)->GetPath())) // move to top for cbr/cbz
    {
      CURL url(m_slides->Get(m_iCurrentSlide)->GetPath());
      std::string strHostName = url.GetHostName();
      if (URIUtils::HasExtension(strHostName, ".cbr|.cbz"))
      {
        m_Image[iPic].m_bIsComic = true;
        m_Image[iPic].Move((float)m_Image[iPic].GetOriginalWidth(),(float)m_Image[iPic].GetOriginalHeight());
      }
    }
  }
  else if (iSlideNumber >= m_slides->Size() || GetPicturePath(m_slides->Get(iSlideNumber).get()) != strFileName)
  { // Failed to load image. and not match values calling LoadPic, then something is changed, ignore.
    CLog::Log(LOGDEBUG, "CGUIWindowSlideShow::OnLoadPic(%d, %d, %s) on failure not match current state (cur %d, next %d, curpic %d, pic[0, 1].slidenumber=%d, %d, %s)", iPic, iSlideNumber, strFileName.c_str(), m_iCurrentSlide, m_iNextSlide, m_iCurrentPic, m_Image[0].SlideNumber(), m_Image[1].SlideNumber(), iSlideNumber >= m_slides->Size() ? "" : m_slides->Get(iSlideNumber)->GetPath().c_str());
  }
  else
  { // Failed to load image.  What should be done??
    // We should wait for the current pic to finish rendering, then transistion it out,
    // release the texture, and try and reload this pic from scratch
    m_bErrorMessage = true;
  }
}

void CGUIWindowSlideShow::Shuffle()
{
  m_slides->Randomize();
  m_iCurrentSlide = 0;
  m_iNextSlide = GetNextSlide();
  m_bShuffled = true;

  AnnouncePropertyChanged("shuffled", true);
}

int CGUIWindowSlideShow::NumSlides() const
{
  return m_slides->Size();
}

int CGUIWindowSlideShow::CurrentSlide() const
{
  return m_iCurrentSlide + 1;
}

void CGUIWindowSlideShow::AddFromPath(const std::string &strPath,
                                      bool bRecursive, 
                                      SortBy method, SortOrder order, SortAttribute sortAttributes,
                                      const std::string &strExtensions)
{
  if (strPath!="")
  {
    // reset the slideshow
    Reset();
    m_strExtensions = strExtensions;
    if (bRecursive)
    {
      path_set recursivePaths;
      AddItems(strPath, &recursivePaths, method, order, sortAttributes);
    }
    else
      AddItems(strPath, NULL, method, order, sortAttributes);
  }
}

void CGUIWindowSlideShow::RunSlideShow(const std::string &strPath, 
                                       bool bRecursive /* = false */, bool bRandom /* = false */,
                                       bool bNotRandom /* = false */, const std::string &beginSlidePath /* = "" */,
                                       bool startSlideShow /* = true */, SortBy method /* = SortByLabel */, 
                                       SortOrder order /* = SortOrderAscending */, SortAttribute sortAttributes /* = SortAttributeNone */,
                                       const std::string &strExtensions)
{
  // stop any video
  if (g_application.m_pPlayer->IsPlayingVideo())
    g_application.StopPlaying();

  AddFromPath(strPath, bRecursive, method, order, sortAttributes, strExtensions);

  if (!NumSlides())
    return;

  // mutually exclusive options
  // if both are set, clear both and use the gui setting
  if (bRandom && bNotRandom)
    bRandom = bNotRandom = false;

  // NotRandom overrides the window setting
  if ((!bNotRandom && CSettings::Get().GetBool("slideshow.shuffle")) || bRandom)
    Shuffle();

  if (!beginSlidePath.empty())
    Select(beginSlidePath);

  if (startSlideShow)
    StartSlideShow();
  else 
  {
    CVariant param;
    param["player"]["speed"] = 0;
    param["player"]["playerid"] = PLAYLIST_PICTURE;
    ANNOUNCEMENT::CAnnouncementManager::Get().Announce(ANNOUNCEMENT::Player, "xbmc", "OnPlay", GetCurrentSlide(), param);
  }

  g_windowManager.ActivateWindow(WINDOW_SLIDESHOW);
}

void CGUIWindowSlideShow::AddItems(const std::string &strPath, path_set *recursivePaths, SortBy method, SortOrder order, SortAttribute sortAttributes)
{
  // check whether we've already added this path
  if (recursivePaths)
  {
    std::string path(strPath);
    URIUtils::RemoveSlashAtEnd(path);
    if (recursivePaths->find(path) != recursivePaths->end())
      return;
    recursivePaths->insert(path);
  }

  // fetch directory and sort accordingly
  CFileItemList items;
  if (!CDirectory::GetDirectory(strPath, items, m_strExtensions.empty()?g_advancedSettings.m_pictureExtensions:m_strExtensions,DIR_FLAG_NO_FILE_DIRS,true))
    return;

  items.Sort(method, order, sortAttributes);

  // need to go into all subdirs
  for (int i = 0; i < items.Size(); i++)
  {
    CFileItemPtr item = items[i];
    if (item->m_bIsFolder && recursivePaths)
    {
      AddItems(item->GetPath(), recursivePaths);
    }
    else if (!item->m_bIsFolder && !URIUtils::IsRAR(item->GetPath()) && !URIUtils::IsZIP(item->GetPath()))
    { // add to the slideshow
      Add(item.get());
    }
  }
}

void CGUIWindowSlideShow::GetCheckedSize(float width, float height, int &maxWidth, int &maxHeight)
{
  maxWidth = g_Windowing.GetMaxTextureSize();
  maxHeight = g_Windowing.GetMaxTextureSize();
}

std::string CGUIWindowSlideShow::GetPicturePath(CFileItem *item)
{
  bool isVideo = item->IsVideo();
  std::string picturePath = item->GetPath();
  if (isVideo)
  {
    picturePath = item->GetArt("thumb");
    if (picturePath.empty() && !item->HasProperty("nothumb"))
    {
      CPictureThumbLoader thumbLoader;
      thumbLoader.LoadItem(item);
      picturePath = item->GetArt("thumb");
      if (picturePath.empty())
        item->SetProperty("nothumb", true);
    }
  }
  return picturePath;
}

