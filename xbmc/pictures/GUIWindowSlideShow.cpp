/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowSlideShow.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "GUIDialogPictureInfo.h"
#include "GUIInfoManager.h"
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "application/Application.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "application/ApplicationPowerHandling.h"
#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodec.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUILabelControl.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/Texture.h"
#include "imagefiles/ImageFileURL.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "input/mouse/MouseEvent.h"
#include "interfaces/AnnouncementManager.h"
#include "pictures/GUIViewStatePictures.h"
#include "pictures/MotionPhoto.h"
#include "pictures/PictureThumbLoader.h"
#include "pictures/SlideShowDelegator.h"
#include "pictures/UltraHdr.h"
#include "playlists/PlayListTypes.h"
#include "rendering/RenderSystem.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "settings/DisplaySettings.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/ArtTypes.h"
#include "utils/Random.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/XTimeUtils.h"
#include "utils/log.h"
#include "video/VideoFileItemClassify.h"
#include "windowing/WinSystem.h"

#include <algorithm>
#include <memory>
#include <mutex>
#include <utility>

using namespace KODI;
using namespace KODI::VIDEO;
using namespace MESSAGING;
using namespace XFILE;
using namespace std::chrono_literals;

#define MAX_ZOOM_FACTOR                     10
#define MAX_PICTURE_SIZE             2048*2048

#define IMMEDIATE_TRANSITION_TIME          1

#define PICTURE_MOVE_AMOUNT              0.02f
#define PICTURE_MOVE_AMOUNT_ANALOG       0.01f
#define PICTURE_MOVE_AMOUNT_TOUCH        0.002f
#define PICTURE_VIEW_BOX_COLOR      0xffffff00 // YELLOW
#define PICTURE_VIEW_BOX_BACKGROUND 0xff000000 // BLACK

#define ROTATION_SNAP_RANGE              10.0f

#define LABEL_ROW1                          10
#define CONTROL_PAUSE                       13

static float zoomamount[10] = { 1.0f, 1.2f, 1.5f, 2.0f, 2.8f, 4.0f, 6.0f, 9.0f, 13.5f, 20.0f };

CBackgroundPicLoader::CBackgroundPicLoader() : CThread("BgPicLoader")
{
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
  auto totalTime = std::chrono::milliseconds(0);
  unsigned int count = 0;
  while (!m_bStop)
  { // loop around forever, waiting for the app to call LoadPic
    if (AbortableWait(m_loadPic, 10ms) == WAIT_SIGNALED)
    {
      if (m_pCallback)
      {
        auto start = std::chrono::steady_clock::now();
        KODI::PICTURE::GainMap gainMap;
        if (m_checkHdr)
          gainMap = KODI::PICTURE::ReadGainMap(m_strFileName);
        std::unique_ptr<CTexture> gainMapTexture;
        if (!gainMap.image.empty())
          gainMapTexture = CTexture::LoadFromFileInMemory(
              reinterpret_cast<unsigned char*>(gainMap.image.data()), gainMap.image.size(),
              "image/jpeg", m_maxWidth, m_maxHeight, CAspectRatio::CENTER, ImageColorMode::AS_IS);
        // a picture drawn in HDR keeps its colours beyond sRGB; without its gain map it isn't
        std::unique_ptr<CTexture> texture = CTexture::LoadFromFile(
            m_strFileName, m_maxWidth, m_maxHeight, CAspectRatio::CENTER, "",
            gainMapTexture ? ImageColorMode::KEEP_GAMUT : ImageColorMode::CONVERT_TO_SRGB);
        std::string motionClip;
        if (texture && m_checkMotion)
          motionClip = KODI::PICTURE::ExtractMotionPhotoClip(m_strFileName);

        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        totalTime += duration;
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
            if (!bFullSize && texture->GetWidth() == CServiceBroker::GetRenderSystem()->GetMaxTextureSize())
              bFullSize = true;
            if (!bFullSize && texture->GetHeight() == CServiceBroker::GetRenderSystem()->GetMaxTextureSize())
              bFullSize = true;
          }
        }
        m_pCallback->OnLoadPic(m_iPic, m_iSlideNumber, m_strFileName, std::move(texture), bFullSize,
                               motionClip, std::move(gainMapTexture), gainMap.params);
        m_isLoading = false;
      }
    }
  }
  if (count > 0)
    CLog::Log(LOGDEBUG, "Time for loading {} images: {} ms, average {} ms", count,
              totalTime.count(), totalTime.count() / count);
}

void CBackgroundPicLoader::LoadPic(int iPic,
                                   int iSlideNumber,
                                   const std::string& strFileName,
                                   const int maxWidth,
                                   const int maxHeight,
                                   bool checkMotion,
                                   bool checkHdr)
{
  m_iPic = iPic;
  m_iSlideNumber = iSlideNumber;
  m_strFileName = strFileName;
  m_maxWidth = maxWidth;
  m_maxHeight = maxHeight;
  m_checkMotion = checkMotion;
  m_checkHdr = checkHdr;
  m_isLoading = true;
  m_loadPic.Set();
}

CGUIWindowSlideShow::CGUIWindowSlideShow(void) : CGUIDialog(WINDOW_SLIDESHOW, "SlideShow.xml")
{
  m_loadType = KEEP_IN_MEMORY;
  m_bLoadNextPic = false;
  CServiceBroker::GetSlideShowDelegator().SetDelegate(this);
  CServiceBroker::GetAnnouncementManager()->AddAnnouncer(this, ANNOUNCEMENT::Player);
  Reset();
}

CGUIWindowSlideShow::~CGUIWindowSlideShow()
{
  CServiceBroker::GetSlideShowDelegator().ResetDelegate();
  CServiceBroker::GetAnnouncementManager()->RemoveAnnouncer(this);
}

void CGUIWindowSlideShow::Announce(ANNOUNCEMENT::AnnouncementFlag flag,
                                   const std::string& sender,
                                   const std::string& message,
                                   const CVariant& data)
{
  if (message == "OnPlay" || message == "OnResume")
  {
    if (data.isMember("player") && data["player"].isMember("playerid") &&
        data["player"]["playerid"] == static_cast<int>(PLAYLIST::Id::TYPE_VIDEO))
      Close();
  }
}

void CGUIWindowSlideShow::AnnouncePlayerPlay(const CFileItemPtr& item)
{
  CVariant param;
  param["player"]["speed"] = m_bSlideShow && !m_bPause ? 1 : 0;
  param["player"]["playerid"] = static_cast<int>(PLAYLIST::Id::TYPE_PICTURE);
  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Player, "OnPlay", item, param);
}

void CGUIWindowSlideShow::AnnouncePlayerPause(const CFileItemPtr& item)
{
  CVariant param;
  param["player"]["speed"] = 0;
  param["player"]["playerid"] = static_cast<int>(PLAYLIST::Id::TYPE_PICTURE);
  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Player, "OnPause", item, param);
}

void CGUIWindowSlideShow::AnnouncePlayerStop(const CFileItemPtr& item)
{
  CVariant param;
  param["player"]["playerid"] = static_cast<int>(PLAYLIST::Id::TYPE_PICTURE);
  param["end"] = true;
  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Player, "OnStop", item, param);
}

void CGUIWindowSlideShow::AnnouncePlaylistClear()
{
  CVariant data;
  data["playlistid"] = static_cast<int>(PLAYLIST::Id::TYPE_PICTURE);
  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Playlist, "OnClear", data);
}

void CGUIWindowSlideShow::AnnouncePlaylistAdd(const CFileItemPtr& item, int pos)
{
  CVariant data;
  data["playlistid"] = static_cast<int>(PLAYLIST::Id::TYPE_PICTURE);
  data["position"] = pos;
  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Playlist, "OnAdd", item, data);
}

void CGUIWindowSlideShow::AnnouncePropertyChanged(const std::string &strProperty, const CVariant &value)
{
  if (strProperty.empty() || value.isNull())
    return;

  CVariant data;
  data["player"]["playerid"] = static_cast<int>(PLAYLIST::Id::TYPE_PICTURE);
  data["property"][strProperty] = value;
  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Player, "OnPropertyChanged",
                                                     data);
}

bool CGUIWindowSlideShow::IsPlaying() const
{
  return m_Image[m_iCurrentPic]->IsLoaded();
}

void CGUIWindowSlideShow::Reset()
{
  if (m_bPlayingMotion)
  {
    const auto appPlayer = CServiceBroker::GetAppComponents().GetComponent<CApplicationPlayer>();
    appPlayer->ClosePlayer();
  }
  m_bSlideShow = false;
  m_bShuffled = false;
  m_bPause = false;
  m_bPlayingVideo = false;
  m_iVideoSlide = -1;
  m_bPlayingMotion = false;
  m_iMotionSlide = -1;
  m_iMotionClipsSlide = -1;
  m_bErrorMessage = false;
  {
    std::unique_lock lock(m_motionClipsSection);
    m_motionClips.clear();
  }

  if (m_Image[0])
  {
    m_Image[0]->UnLoad();
    m_Image[0]->Close();
  }

  m_Image[0] = CSlideShowPic::CreateSlideShowPicture();

  if (m_Image[1])
  {
    m_Image[1]->UnLoad();
    m_Image[1]->Close();
  }

  m_Image[1] = CSlideShowPic::CreateSlideShowPicture();

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
  m_slides.clear();
  AnnouncePlaylistClear();
  m_Resolution = CServiceBroker::GetWinSystem()->GetGfxContext().GetVideoResolution();
}

void CGUIWindowSlideShow::OnDeinitWindow(int nextWindowID)
{
  if (m_Resolution != CDisplaySettings::GetInstance().GetCurrentResolution())
  {
    //FIXME: Use GUI resolution for now
    //CServiceBroker::GetWinSystem()->GetGfxContext().SetVideoResolution(CDisplaySettings::GetInstance().GetCurrentResolution(), true);
  }

  SetHdrOutput(false);

  if (m_bPlayingMotion)
  {
    // The clip is part of the photo, so it ends with the slideshow
    const auto appPlayer = CServiceBroker::GetAppComponents().GetComponent<CApplicationPlayer>();
    appPlayer->ClosePlayer();
    m_bPlayingMotion = false;
    m_bPlayingVideo = false;
    m_iVideoSlide = -1;
  }

  if (nextWindowID != WINDOW_FULLSCREEN_VIDEO &&
      nextWindowID != WINDOW_FULLSCREEN_GAME)
  {
    // wait for any outstanding picture loads
    if (m_pBackgroundLoader)
    {
      // sleep until the loader finishes loading the current pic
      CLog::Log(LOGDEBUG,"Waiting for BackgroundLoader thread to close");
      while (m_pBackgroundLoader->IsLoading())
        KODI::TIME::Sleep(10ms);
      // stop the thread
      CLog::Log(LOGDEBUG,"Stopping BackgroundLoader thread");
      m_pBackgroundLoader->StopThread();
      m_pBackgroundLoader.reset();
    }
    // and close the images.
    m_Image[0]->Close();
    m_Image[1]->Close();

    {
      std::unique_lock lock(m_motionClipsSection);
      m_motionClips.clear();
    }
    KODI::PICTURE::ClearMotionPhotoClips();
  }
  CServiceBroker::GetGUI()->GetInfoManager().GetInfoProviders().GetPicturesInfoProvider().SetCurrentSlide(nullptr);
  m_bSlideShow = false;

  CGUIDialog::OnDeinitWindow(nextWindowID);
}

void CGUIWindowSlideShow::FreeResources(bool forceUnLoad)
{
  m_Image[0]->Free();
  m_Image[1]->Free();

  CGUIDialog::FreeResources(forceUnLoad);
}

void CGUIWindowSlideShow::Add(const CFileItem *picture)
{
  CFileItemPtr item(new CFileItem(*picture));
  if (!item->HasVideoInfoTag() && !item->HasPictureInfoTag())
  {
    // item without tag; get mimetype then we can tell whether it's video item
    item->FillInMimeType();

    if (!IsVideo(*item))
      // then it is a picture and force tag generation
      item->GetPictureInfoTag();
  }
  AnnouncePlaylistAdd(item, m_slides.size());

  m_slides.emplace_back(std::move(item));
}

void CGUIWindowSlideShow::ShowNext()
{
  if (m_slides.size() == 1)
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
  if (m_slides.size() == 1)
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
  for (size_t i = 0; i < m_slides.size(); ++i)
  {
    const CFileItemPtr item = m_slides.at(i);
    if (item->GetPath() == strPicture)
    {
      m_iDirection = 1;
      if (!m_Image[m_iCurrentPic]->IsLoaded() &&
          (!m_pBackgroundLoader || !m_pBackgroundLoader->IsLoading()))
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

void CGUIWindowSlideShow::GetSlideShowContents(CFileItemList &list)
{
  for (size_t index = 0; index < m_slides.size(); index++)
    list.Add(std::make_shared<CFileItem>(*m_slides.at(index)));
}

std::shared_ptr<const CFileItem> CGUIWindowSlideShow::GetCurrentSlide()
{
  if (m_iCurrentSlide >= 0 && m_iCurrentSlide < static_cast<int>(m_slides.size()))
    return m_slides.at(m_iCurrentSlide);
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
  if (!m_slides.empty())
    AnnouncePlayerPlay(m_slides.at(m_iCurrentSlide));
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
  CWinSystemBase* const winSystem{CServiceBroker::GetWinSystem()};
  const RESOLUTION_INFO res = winSystem->GetGfxContext().GetResInfo();

  // reset the screensaver if we're in a slideshow
  // (unless we are the screensaver!)
  auto& components = CServiceBroker::GetAppComponents();
  const auto appPower = components.GetComponent<CApplicationPowerHandling>();
  if (m_bSlideShow && !m_bPause && !appPower->IsInScreenSaver())
    appPower->ResetScreenSaver();
  // A slideshow that Reset() emptied, with its window still open, has nothing to show in HDR
  if (m_hdrOutput && m_slides.empty())
    SetHdrOutput(false);
  int iSlides = m_slides.size();
  if (!iSlides)
    return;

  // if we haven't processed yet, we should mark the whole screen
  if (!HasProcessed())
  {
    regions.emplace_back(CRect(
        0.0f, 0.0f, static_cast<float>(winSystem->GetGfxContext().GetWidth()),
        static_cast<float>(winSystem->GetGfxContext().GetHeight())));
    MarkDirtyRegion();
  }

  if (m_iCurrentSlide < 0 || m_iCurrentSlide >= static_cast<int>(m_slides.size()))
    m_iCurrentSlide = 0;
  if (m_iNextSlide < 0 || m_iNextSlide >= static_cast<int>(m_slides.size()))
    m_iNextSlide = GetNextSlide();

  // Create our background loader if necessary
  if (!m_pBackgroundLoader)
  {
    m_pBackgroundLoader = std::make_unique<CBackgroundPicLoader>();
    m_pBackgroundLoader->Create(this);
  }

  bool bSlideShow = m_bSlideShow && !m_bPause && !m_bPlayingVideo;
  // A thumbnail still loading into the current image would land after the skip, so wait for it
  const bool loadingCurrent = m_pBackgroundLoader->IsLoading() &&
                              m_pBackgroundLoader->Pic() == m_iCurrentPic &&
                              m_pBackgroundLoader->SlideNumber() == m_iCurrentSlide;
  if (bSlideShow && !loadingCurrent && m_slides.at(m_iCurrentSlide)->HasProperty("unplayable"))
  {
    m_iNextSlide = GetNextSlide();
    if (m_iCurrentSlide == m_iNextSlide)
      return;
    // A video that won't play still shows its thumbnail, so move on to the next slide's image too
    if (m_Image[m_iCurrentPic]->IsLoaded() &&
        m_Image[m_iCurrentPic]->SlideNumber() == m_iCurrentSlide)
    {
      m_Image[m_iCurrentPic]->Close();
      if ((m_Image[1 - m_iCurrentPic]->IsLoaded() &&
           m_Image[1 - m_iCurrentPic]->SlideNumber() == m_iNextSlide) ||
          (m_pBackgroundLoader->IsLoading() && m_pBackgroundLoader->SlideNumber() == m_iNextSlide &&
           m_pBackgroundLoader->Pic() == 1 - m_iCurrentPic))
        m_iCurrentPic = 1 - m_iCurrentPic;
    }
    m_iCurrentSlide = m_iNextSlide;
    m_iNextSlide = GetNextSlide();
    // A Next or Previous pressed on the slide that failed is answered by the skip itself
    m_bLoadNextPic = false;
  }

  if (m_bErrorMessage)
  { // we have an error when loading either the current or next picture
    // check to see if we have a picture loaded
    CLog::Log(LOGDEBUG, "We have an error loading picture {}!", m_pBackgroundLoader->SlideNumber());
    if (m_iCurrentSlide == m_pBackgroundLoader->SlideNumber())
    {
      if (m_Image[m_iCurrentPic]->IsLoaded())
      {
        // current image was already loaded, so we can ignore this error.
        m_bErrorMessage = false;
      }
      else
      {
        CLog::Log(LOGERROR, "Error loading the current image {}: {}", m_iCurrentSlide,
                  m_slides.at(m_iCurrentSlide)->GetPath());
        if (!IsVideo(*m_slides.at(m_iCurrentSlide)))
        {
          // try next if we are in slideshow
          CLog::Log(LOGINFO, "set image {} unplayable", m_slides.at(m_iCurrentSlide)->GetPath());
          m_slides.at(m_iCurrentSlide)->SetProperty("unplayable", true);
        }
        if (m_bLoadNextPic || (bSlideShow && !m_bPause && !IsVideo(*m_slides.at(m_iCurrentSlide))))
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
      CLog::Log(LOGERROR, "Error loading the next image {}: {}", m_iNextSlide,
                m_slides.at(m_iNextSlide)->GetPath());
      // load next image failed, then skip to load next of next if next is not video.
      if (!IsVideo(*m_slides.at(m_iNextSlide)))
      {
        CLog::Log(LOGINFO, "set image {} unplayable", m_slides.at(m_iNextSlide)->GetPath());
        m_slides.at(m_iNextSlide)->SetProperty("unplayable", true);
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
      CLog::Log(LOGERROR, "Error loading the non-current non-next image {}/{}: {}", m_iNextSlide,
                m_pBackgroundLoader->SlideNumber(), m_slides.at(m_iNextSlide)->GetPath());
      m_bErrorMessage = false;
    }
  }

  if (m_bErrorMessage)
  { // hack, just mark it all
    regions.emplace_back(CRect(
        0.0f, 0.0f, static_cast<float>(winSystem->GetGfxContext().GetWidth()),
        static_cast<float>(winSystem->GetGfxContext().GetHeight())));
    MarkDirtyRegion();
    return;
  }

  if (!m_Image[m_iCurrentPic]->IsLoaded() && !m_pBackgroundLoader->IsLoading())
  { // load first image
    CFileItemPtr item = m_slides.at(m_iCurrentSlide);
    std::string picturePath = GetPicturePath(item.get());
    if (!picturePath.empty())
    {
      if (IsVideo(*item))
        CLog::Log(LOGDEBUG, "Loading the thumb {} for current video {}: {}", picturePath,
                  m_iCurrentSlide, item->GetPath());
      else
        CLog::Log(LOGDEBUG, "Loading the current image {}: {}", m_iCurrentSlide, item->GetPath());

      // load using the background loader
      int maxWidth, maxHeight;

      GetCheckedSize((float)res.iWidth * m_fZoom,
        (float)res.iHeight * m_fZoom,
        maxWidth, maxHeight);
      m_pBackgroundLoader->LoadPic(m_iCurrentPic, m_iCurrentSlide, picturePath, maxWidth, maxHeight,
                                   !IsVideo(*item), m_showHdr && !IsVideo(*item));
      m_iLastFailedNextSlide = -1;
      m_bLoadNextPic = false;
    }
  }

  // check if we should discard an already loaded next slide
  if (m_Image[1 - m_iCurrentPic]->IsLoaded() &&
      m_Image[1 - m_iCurrentPic]->SlideNumber() != m_iNextSlide)
    m_Image[1 - m_iCurrentPic]->Close();

  if (m_iNextSlide != m_iCurrentSlide && m_Image[m_iCurrentPic]->IsLoaded() &&
      !m_Image[1 - m_iCurrentPic]->IsLoaded() && !m_pBackgroundLoader->IsLoading() &&
      m_iLastFailedNextSlide != m_iNextSlide)
  { // load the next image
    m_iLastFailedNextSlide = -1;
    CFileItemPtr item = m_slides.at(m_iNextSlide);
    std::string picturePath = GetPicturePath(item.get());
    if (!picturePath.empty())
    {
      if (IsVideo(*item))
        CLog::Log(LOGDEBUG, "Loading the thumb {} for next video {}: {}", picturePath, m_iNextSlide,
                  item->GetPath());
      else
        CLog::Log(LOGDEBUG, "Loading the next image {}: {}", m_iNextSlide, item->GetPath());

      int maxWidth, maxHeight;
      GetCheckedSize((float)res.iWidth * m_fZoom,
                     (float)res.iHeight * m_fZoom,
                     maxWidth, maxHeight);
      m_pBackgroundLoader->LoadPic(1 - m_iCurrentPic, m_iNextSlide, picturePath, maxWidth,
                                   maxHeight, !IsVideo(*item), m_showHdr && !IsVideo(*item));
    }
  }

  bool bPlayVideo = IsVideo(*m_slides.at(m_iCurrentSlide)) && m_iVideoSlide != m_iCurrentSlide;
  if (bPlayVideo)
    bSlideShow = false;

  // render the current image
  if (m_Image[m_iCurrentPic]->IsLoaded())
  {
    if (m_Image[m_iCurrentPic]->IsAnimating())
      MarkDirtyRegion();

    m_Image[m_iCurrentPic]->SetInSlideshow(bSlideShow);
    m_Image[m_iCurrentPic]->Pause(!bSlideShow);
    m_Image[m_iCurrentPic]->Process(currentTime, regions);
  }

  // Check if we should be transitioning immediately
  if (m_bLoadNextPic && m_Image[m_iCurrentPic]->IsLoaded())
  {
    CLog::Log(LOGDEBUG, "Starting immediate transition due to user wanting slide {}",
              m_slides.at(m_iNextSlide)->GetPath());
    if (m_Image[m_iCurrentPic]->StartTransition())
    {
      m_Image[m_iCurrentPic]->SetTransitionTime(1, IMMEDIATE_TRANSITION_TIME);
      m_bLoadNextPic = false;
      MarkDirtyRegion();
    }
  }

  const auto appPlayer = components.GetComponent<CApplicationPlayer>();

  // render the next image
  if (m_Image[m_iCurrentPic]->DrawNextImage())
  {
    if (m_Image[1 - m_iCurrentPic]->IsLoaded())
    {
      if (appPlayer->IsPlayingVideo())
        appPlayer->ClosePlayer();
      m_bPlayingVideo = false;
      m_bPlayingMotion = false;
      m_iVideoSlide = -1;

      // first time render the next image, make sure using current display effect.
      if (!m_Image[1 - m_iCurrentPic]->IsStarted())
      {
        CSlideShowPic::DISPLAY_EFFECT effect = GetDisplayEffect(m_iNextSlide);
        if (m_Image[1 - m_iCurrentPic]->DisplayEffectNeedChange(effect))
          m_Image[1 - m_iCurrentPic]->Reset(effect);
      }

      if (m_Image[1 - m_iCurrentPic]->IsAnimating())
        MarkDirtyRegion();

      // set the appropriate transition time
      m_Image[1 - m_iCurrentPic]->SetTransitionTime(0,
                                                    m_Image[m_iCurrentPic]->GetTransitionTime(1));
      m_Image[1 - m_iCurrentPic]->Pause(!m_bSlideShow || m_bPause ||
                                        IsVideo(*m_slides.at(m_iNextSlide)));
      m_Image[1 - m_iCurrentPic]->Process(currentTime, regions);
    }
    else // next pic isn't loaded.  We should hang around if it is in progress
    {
      if (m_pBackgroundLoader->IsLoading())
      {
        //        CLog::Log(LOGDEBUG, "Having to hold the current image ({}) while we load {}", m_vecSlides[m_iCurrentSlide], m_vecSlides[m_iNextSlide]);
        m_Image[m_iCurrentPic]->Keep();
      }
    }
  }

  // check if we should swap images now
  if (m_Image[m_iCurrentPic]->IsFinished() ||
      (m_bLoadNextPic && !m_Image[m_iCurrentPic]->IsLoaded()))
  {
    m_bLoadNextPic = false;
    if (m_Image[m_iCurrentPic]->IsFinished())
      CLog::Log(LOGDEBUG, "Image {} is finished rendering, switching to {}",
                m_slides.at(m_iCurrentSlide)->GetPath(), m_slides.at(m_iNextSlide)->GetPath());
    else
      // what if it's bg loading?
      CLog::Log(LOGDEBUG, "Image {} is not loaded, switching to {}",
                m_slides.at(m_iCurrentSlide)->GetPath(), m_slides.at(m_iNextSlide)->GetPath());

    if (m_Image[m_iCurrentPic]->IsFinished() && m_iCurrentSlide == m_iNextSlide &&
        m_Image[m_iCurrentPic]->SlideNumber() == m_iNextSlide)
      m_Image[m_iCurrentPic]->Reset(GetDisplayEffect(m_iCurrentSlide));
    else
    {
      if (m_Image[m_iCurrentPic]->IsLoaded())
        m_Image[m_iCurrentPic]->Reset(GetDisplayEffect(m_iCurrentSlide));
      else
        m_Image[m_iCurrentPic]->Close();

      if ((m_Image[1 - m_iCurrentPic]->IsLoaded() &&
           m_Image[1 - m_iCurrentPic]->SlideNumber() == m_iNextSlide) ||
          (m_pBackgroundLoader->IsLoading() && m_pBackgroundLoader->SlideNumber() == m_iNextSlide &&
           m_pBackgroundLoader->Pic() == 1 - m_iCurrentPic))
      {
        m_iCurrentPic = 1 - m_iCurrentPic;
      }
      else
      {
        m_Image[1 - m_iCurrentPic]->Close();
        m_iCurrentPic = 1 - m_iCurrentPic;
      }
      m_iCurrentSlide = m_iNextSlide;
      m_iNextSlide    = GetNextSlide();

      bPlayVideo = IsVideo(*m_slides.at(m_iCurrentSlide)) && m_iVideoSlide != m_iCurrentSlide;
    }
    AnnouncePlayerPlay(m_slides.at(m_iCurrentSlide));

    m_iZoomFactor = 1;
    m_fZoom = 1.0f;
    m_fRotate = 0.0f;
  }

  if (bPlayVideo && !PlayVideo())
      return;

  // Only the current and next pictures' clips can still play
  if (m_iMotionClipsSlide != m_iCurrentSlide)
  {
    m_iMotionClipsSlide = m_iCurrentSlide;
    const std::string& current = m_slides.at(m_iCurrentSlide)->GetDynPath();
    const std::string& next = m_slides.at(m_iNextSlide)->GetDynPath();
    std::unique_lock lock(m_motionClipsSection);
    std::erase_if(m_motionClips,
                  [&current, &next](const auto& clip)
                  {
                    if (clip.first == current || clip.first == next)
                      return false;
                    XFILE::CFile::Delete(clip.second);
                    return true;
                  });
  }

  if (m_hdrOutput && !m_showHdr)
    SetHdrOutput(false);

  // out of HDR, a picture that kept its colours beyond sRGB would look dull, so load it again
  if (!m_showHdr && !m_pBackgroundLoader->IsLoading())
  {
    for (const int pic : {m_iCurrentPic, 1 - m_iCurrentPic})
    {
      const int slide{m_Image[pic]->SlideNumber()};
      // once per slide, so a picture that fails to load again isn't tried every frame
      if (!m_reloadAsSRGB[pic] && m_sRGBReloadSlide[pic] != slide && m_Image[pic]->IsLoaded() &&
          m_Image[pic]->HasColorMatrix() && slide >= 0 && slide < static_cast<int>(m_slides.size()))
      {
        int maxWidth;
        int maxHeight;
        GetCheckedSize(static_cast<float>(res.iWidth) * m_fZoom,
                       static_cast<float>(res.iHeight) * m_fZoom, maxWidth, maxHeight);
        m_reloadAsSRGB[pic] = true;
        m_sRGBReloadSlide[pic] = slide;
        m_pBackgroundLoader->LoadPic(pic, slide, GetPicturePath(m_slides.at(slide).get()), maxWidth,
                                     maxHeight, false, false);
        break;
      }
    }
  }

  // The first HDR picture switches the display to HDR, and it stays there until the slideshow ends.
  // A video sets the display up for itself, so the next picture after one switches back.
  if (m_showHdr && !m_hdrOutput && !m_bPlayingVideo && m_Image[m_iCurrentPic]->IsLoaded() &&
      (m_Image[m_iCurrentPic]->HasGainMap() ||
       (m_hdrEntered && !IsVideo(*m_slides.at(m_iCurrentSlide)))))
  {
    SetHdrOutput(true);
    m_hdrEntered = m_hdrEntered || m_hdrOutput;
  }

  // A new visit to a moving photo plays its clip again, even if a video was showing in between
  if (m_iMotionSlide != m_iCurrentSlide)
    m_iMotionSlide = -1;

  // While browsing by hand, a moving photo plays its clip each time it comes up. Its clip isn't
  // HDR, so it doesn't play while the display is in HDR.
  if ((!m_bSlideShow || m_bPause) && !m_bPlayingVideo && !m_hdrOutput &&
      m_iMotionSlide != m_iCurrentSlide && m_Image[m_iCurrentPic]->IsLoaded() &&
      m_Image[m_iCurrentPic]->SlideNumber() == m_iCurrentSlide &&
      !m_Image[m_iCurrentPic]->DrawNextImage())
  {
    m_iMotionSlide = m_iCurrentSlide;
    PlayMotion();
  }

  if (m_Image[m_iCurrentPic]->IsLoaded())
    CServiceBroker::GetGUI()->GetInfoManager().GetInfoProviders().GetPicturesInfoProvider().SetCurrentSlide(m_slides.at(m_iCurrentSlide).get());

  RenderPause();
  // Single-plane mode (video drawn into the GUI back buffer during the
  // render pass): MarkDirty every frame to keep the render pass running.
  // D2P (video has its own hardware plane): video updates independently
  // of the render pass.
  if ((IsVideo(*m_slides.at(m_iCurrentSlide)) && appPlayer &&
       !appPlayer->IsRenderingVideoLayer()) ||
      m_bPlayingMotion)
  {
    MarkDirtyRegion();
  }
  CGUIWindow::Process(currentTime, regions);
  m_renderRegion.SetRect(0, 0, (float)winSystem->GetGfxContext().GetWidth(), (float)winSystem->GetGfxContext().GetHeight());
}

void CGUIWindowSlideShow::Render()
{
  if (m_slides.empty())
    return;

  CGraphicContext& gfxCtx = CServiceBroker::GetWinSystem()->GetGfxContext();
  // HDR pictures are drawn underneath the GUI by RenderEx(), and HDR video by the window below
  // the slideshow
  const bool hdrUnderneath =
      m_hdrOutput || (IsShowingVideo() && CServiceBroker::GetWinSystem()->IsHdrComposite());
  gfxCtx.Clear(hdrUnderneath ? 0 : 0xff000000);

  if (IsShowingVideo())
  {
    gfxCtx.SetViewWindow(0, 0, m_coordsRes.iWidth, m_coordsRes.iHeight);
    gfxCtx.SetRenderingResolution(gfxCtx.GetVideoResolution(), false);

    auto& components = CServiceBroker::GetAppComponents();
    const auto appPlayer = components.GetComponent<CApplicationPlayer>();

    if (appPlayer->IsRenderingVideoLayer())
    {
      const CRect old = gfxCtx.GetScissors();
      CRect region = GetRenderRegion();
      region.Intersect(old);
      gfxCtx.SetScissors(region);
      gfxCtx.Clear(0);
      gfxCtx.SetScissors(old);
    }
    else if (appPlayer)
    {
      const ::UTILS::COLOR::Color alpha = gfxCtx.MergeAlpha(0xff000000) >> 24;
      appPlayer->Render(false, alpha);
    }

    gfxCtx.SetRenderingResolution(m_coordsRes, m_needsScaling);
  }
  else
  {
    if (m_Image[m_iCurrentPic]->IsLoaded())
      m_Image[m_iCurrentPic]->Render(!m_hdrOutput);

    if (m_Image[m_iCurrentPic]->DrawNextImage() && m_Image[1 - m_iCurrentPic]->IsLoaded())
      m_Image[1 - m_iCurrentPic]->Render(!m_hdrOutput);
  }

  RenderErrorMessage();
  CGUIWindow::Render();
}

void CGUIWindowSlideShow::RenderEx()
{
  if (IsShowingVideo())
  {
    auto& components = CServiceBroker::GetAppComponents();
    const auto appPlayer = components.GetComponent<CApplicationPlayer>();
    appPlayer->Render(false, 255, false);
  }
  else if (m_hdrOutput)
  {
    CServiceBroker::GetWinSystem()->GetGfxContext().SetRenderingResolution(m_coordsRes,
                                                                           m_needsScaling);
    bool rendered = true;
    if (m_Image[m_iCurrentPic]->IsLoaded())
      rendered = m_Image[m_iCurrentPic]->RenderHdr();

    if (m_Image[m_iCurrentPic]->DrawNextImage() && m_Image[1 - m_iCurrentPic]->IsLoaded())
      rendered = m_Image[1 - m_iCurrentPic]->RenderHdr() && rendered;

    // Process() switches back to SDR
    if (!rendered)
      m_showHdr = false;
  }

  CGUIWindow::RenderEx();
}

int CGUIWindowSlideShow::GetNextSlide()
{
  if (m_slides.size() <= 1)
    return m_iCurrentSlide;
  int step = m_iDirection >= 0 ? 1 : -1;
  int nextSlide = (m_iCurrentSlide + step + m_slides.size()) % m_slides.size();
  while (nextSlide != m_iCurrentSlide)
  {
    if (!m_slides.at(nextSlide)->HasProperty("unplayable"))
      return nextSlide;
    nextSlide = (nextSlide + step + m_slides.size()) % m_slides.size();
  }
  return m_iCurrentSlide;
}

EVENT_RESULT CGUIWindowSlideShow::OnMouseEvent(const CPoint& point, const MOUSE::CMouseEvent& event)
{
  if (event.m_id == ACTION_GESTURE_NOTIFY)
  {
    int result = EVENT_RESULT_ROTATE | EVENT_RESULT_ZOOM;
    if (m_iZoomFactor == 1 || !m_Image[m_iCurrentPic]->m_bCanMoveHorizontally)
      result |= EVENT_RESULT_SWIPE;
    else
      result |= EVENT_RESULT_PAN_HORIZONTAL;

    if (m_Image[m_iCurrentPic]->m_bCanMoveVertically)
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
    if (m_iZoomFactor != 1 && (m_Image[m_iCurrentPic]->m_bCanMoveHorizontally ||
                               m_Image[m_iCurrentPic]->m_bCanMoveVertically))
    {
      Move(PICTURE_MOVE_AMOUNT_TOUCH / m_iZoomFactor * (m_firstGesturePoint.x - point.x), PICTURE_MOVE_AMOUNT_TOUCH / m_iZoomFactor * (m_firstGesturePoint.y - point.y));
      m_firstGesturePoint = point;
    }
    return EVENT_RESULT_HANDLED;
  }
  else if (event.m_id == ACTION_GESTURE_SWIPE_LEFT || event.m_id == ACTION_GESTURE_SWIPE_RIGHT)
  {
    if (m_iZoomFactor == 1 || !m_Image[m_iCurrentPic]->m_bCanMoveHorizontally)
    {
      // on zoomlevel 1 just detect swipe left and right
      if (event.m_id == ACTION_GESTURE_SWIPE_LEFT)
        OnAction(CAction(ACTION_NEXT_PICTURE));
      else
        OnAction(CAction(ACTION_PREV_PICTURE));
    }
  }
  else if (event.m_id == ACTION_GESTURE_END || event.m_id == ACTION_GESTURE_ABORT)
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
  case ACTION_SHOW_INFO:
    {
      CGUIDialogPictureInfo *pictureInfo = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogPictureInfo>(WINDOW_DIALOG_PICTURE_INFO);
      if (pictureInfo)
      {
        // no need to set the picture here, it's done in Render()
        pictureInfo->Open();
      }
    }
    break;
  case ACTION_STOP:
  {
    if (!m_slides.empty())
      AnnouncePlayerStop(m_slides.at(m_iCurrentSlide));
    auto& components = CServiceBroker::GetAppComponents();
    const auto appPlayer = components.GetComponent<CApplicationPlayer>();
    if (appPlayer->IsPlayingVideo())
      appPlayer->ClosePlayer();
    Close();
    break;
  }

  case ACTION_NEXT_PICTURE:
      ShowNext();
    break;

  case ACTION_PREV_PICTURE:
      ShowPrevious();
    break;

  case ACTION_MOVE_RIGHT:
    if (m_iZoomFactor == 1 || !m_Image[m_iCurrentPic]->m_bCanMoveHorizontally)
      ShowNext();
    else
      Move(PICTURE_MOVE_AMOUNT, 0);
    break;

  case ACTION_MOVE_LEFT:
    if (m_iZoomFactor == 1 || !m_Image[m_iCurrentPic]->m_bCanMoveHorizontally)
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
    if (m_slides.empty())
      break;
    if (IsVideo(*m_slides.at(m_iCurrentSlide)))
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
      // A running slideshow shows moving photos as stills
      if (m_bPlayingMotion)
      {
        CServiceBroker::GetAppComponents().GetComponent<CApplicationPlayer>()->ClosePlayer();
        EndMotion();
      }
      m_bSlideShow = true;
      m_bPause = false;
      SetDirection(1);
      if (m_Image[m_iCurrentPic]->IsLoaded())
      {
        CSlideShowPic::DISPLAY_EFFECT effect = GetDisplayEffect(m_iCurrentSlide);
        if (m_Image[m_iCurrentPic]->DisplayEffectNeedChange(effect))
          m_Image[m_iCurrentPic]->Reset(effect);
      }
      AnnouncePlayerPlay(m_slides.at(m_iCurrentSlide));
    }
    else if (action.GetID() == ACTION_PAUSE)
    {
      m_bPause = true;
      AnnouncePlayerPause(m_slides.at(m_iCurrentSlide));
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
    if (m_iZoomFactor == 1 || !m_Image[m_iCurrentPic]->m_bCanMoveVertically)
    {
      bool swipeOnLeft = action.GetAmount() < CServiceBroker::GetWinSystem()->GetGfxContext().GetWidth() / 2.0f;
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
    // this action is used and works, when CAction object provides both x and y coordinates
    Move(action.GetAmount()*PICTURE_MOVE_AMOUNT_ANALOG, -action.GetAmount(1)*PICTURE_MOVE_AMOUNT_ANALOG);
    break;
  case ACTION_ANALOG_MOVE_X_LEFT:
    Move(-action.GetAmount()*PICTURE_MOVE_AMOUNT_ANALOG, 0.0f);
    break;
  case ACTION_ANALOG_MOVE_X_RIGHT:
    Move(action.GetAmount()*PICTURE_MOVE_AMOUNT_ANALOG, 0.0f);
    break;
  case ACTION_ANALOG_MOVE_Y_UP:
    Move(0.0f, -action.GetAmount()*PICTURE_MOVE_AMOUNT_ANALOG);
    break;
  case ACTION_ANALOG_MOVE_Y_DOWN:
    Move(0.0f, action.GetAmount()*PICTURE_MOVE_AMOUNT_ANALOG);
    break;

  default:
    return CGUIDialog::OnAction(action);
  }
  MarkDirtyRegion();
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

  CGUIFont *pFont = static_cast<const CGUILabelControl*>(control)->GetLabelInfo().font;
  CGUITextLayout::DrawText(pFont, 0.5f * CServiceBroker::GetWinSystem()->GetGfxContext().GetWidth(),
                           0.5f * CServiceBroker::GetWinSystem()->GetGfxContext().GetHeight(),
                           0xffffffff, 0,
                           CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(747),
                           XBFONT_CENTER_X | XBFONT_CENTER_Y);
}

bool CGUIWindowSlideShow::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
    {
      m_Resolution = (RESOLUTION) CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_PICTURES_DISPLAYRESOLUTION);

      //FIXME: Use GUI resolution for now
      if (false /*m_Resolution != CDisplaySettings::GetInstance().GetCurrentResolution() && m_Resolution != INVALID && m_Resolution!=AUTORES*/)
        CServiceBroker::GetWinSystem()->GetGfxContext().SetVideoResolution(m_Resolution, false);
      else
        m_Resolution = CServiceBroker::GetWinSystem()->GetGfxContext().GetVideoResolution();

      // A slideshow with videos in it stays out of HDR: the display would change mode, and go
      // blank for a moment, on the way into and out of every video
      m_showHdr = CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
                      CSettings::SETTING_SLIDESHOW_HDRPICTURES) &&
                  m_Image[0]->SupportsHdr() && CServiceBroker::GetWinSystem()->IsHDRDisplay() &&
                  std::ranges::none_of(m_slides, [](const auto& slide) { return IsVideo(*slide); });
      m_hdrEntered = false;
      m_reloadAsSRGB[0] = m_reloadAsSRGB[1] = false;
      m_sRGBReloadSlide[0] = m_sRGBReloadSlide[1] = -1;

      CGUIDialog::OnMessage(message);

      // turn off slideshow if we only have 1 image
      if (m_slides.size() <= 1)
        m_bSlideShow = false;

      return true;
    }
    break;

  case GUI_MSG_SHOW_PICTURE:
    {
      const std::string& strFile = message.GetStringParam();
      Reset();
      CFileItem item(strFile, false);
      Add(&item);
      RunSlideShow("", false, false, true, "", false);
    }
    break;

  case GUI_MSG_START_SLIDESHOW:
    {
      const std::string& strFolder = message.GetStringParam();
      unsigned int iParams = message.GetParam1();
      const std::string& beginSlidePath = message.GetStringParam(1);
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

    case GUI_MSG_PLAYBACK_STOPPED:
      {
        if (m_bPlayingMotion)
        {
          EndMotion();
          break;
        }
        if (m_bPlayingVideo)
        {
          m_bPlayingVideo = false;
          m_iVideoSlide = -1;
          if (m_bSlideShow)
            m_bPause = true;
        }
      }
      break;

    case GUI_MSG_PLAYBACK_ENDED:
      {
        if (m_bPlayingMotion)
        {
          EndMotion();
          break;
        }
        if (m_bPlayingVideo)
        {
          m_bPlayingVideo = false;
          m_iVideoSlide = -1;
          if (m_bSlideShow)
          {
            m_bPause = false;
            if (m_iCurrentSlide == m_iNextSlide)
              break;
            m_Image[m_iCurrentPic]->Close();
            m_iCurrentPic = 1 - m_iCurrentPic;
            m_iCurrentSlide = m_iNextSlide;
            m_iNextSlide    = GetNextSlide();
            AnnouncePlayerPlay(m_slides.at(m_iCurrentSlide));
            m_iZoomFactor = 1;
            m_fZoom = 1.0f;
            m_fRotate = 0.0f;
          }
        }
      }
      break;

      case GUI_MSG_PLAYBACK_ERROR:
        // A moving photo whose clip won't play stays in the slideshow as a photo
        if (m_bPlayingMotion &&
            !CServiceBroker::GetAppComponents().GetComponent<CApplicationPlayer>()->IsPlaying())
        {
          EndMotion();
          break;
        }
        // While a player is running, the error is a late one from a video already left behind
        if (m_bPlayingVideo && m_iVideoSlide >= 0 &&
            m_iVideoSlide < static_cast<int>(m_slides.size()) &&
            !CServiceBroker::GetAppComponents().GetComponent<CApplicationPlayer>()->IsPlaying())
        {
          // Skip it like a picture that won't load. m_iVideoSlide stays set, so it isn't retried.
          CLog::Log(LOGINFO, "set video {} unplayable", m_slides.at(m_iVideoSlide)->GetPath());
          m_slides.at(m_iVideoSlide)->SetProperty("unplayable", true);
          m_bPlayingVideo = false;
        }
        break;
  }
  return CGUIDialog::OnMessage(message);
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
}

void CGUIWindowSlideShow::Rotate(float fAngle, bool immediate /* = false */)
{
  if (m_Image[m_iCurrentPic]->DrawNextImage())
    return;

  m_fRotate += fAngle;

  m_Image[m_iCurrentPic]->Rotate(fAngle, immediate);
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

  if (m_Image[m_iCurrentPic]->DrawNextImage())
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

  m_Image[m_iCurrentPic]->Zoom(m_fZoom, immediate);
}

void CGUIWindowSlideShow::Move(float fX, float fY)
{
  if (m_Image[m_iCurrentPic]->IsLoaded() && m_Image[m_iCurrentPic]->GetZoom() > 1)
  { // we move in the opposite direction, due to the fact we are moving
    // the viewing window, not the picture.
    m_Image[m_iCurrentPic]->Move(-fX, -fY);
  }
}

bool CGUIWindowSlideShow::PlayVideo()
{
  CFileItemPtr item = m_slides.at(m_iCurrentSlide);
  if (!item || !IsVideo(*item))
    return false;
  CLog::Log(LOGDEBUG, "Playing current video slide {}", item->GetPath());
  m_bPlayingVideo = true;
  m_iVideoSlide = m_iCurrentSlide;
  // The video sets up the display for itself
  SetHdrOutput(false);
  // The clip plays inside the slideshow, so it mustn't switch the display mode like a film
  const bool startWindowed = CMediaSettings::GetInstance().DoesMediaStartWindowed();
  CMediaSettings::GetInstance().SetMediaStartWindowed(true);
  bool ret = g_application.PlayFile(*item, "");
  CMediaSettings::GetInstance().SetMediaStartWindowed(startWindowed);
  if (ret == true)
    return true;
  else
  {
    CLog::Log(LOGINFO, "set video {} unplayable", item->GetPath());
    item->SetProperty("unplayable", true);
  }
  // m_iVideoSlide stays set, so the video isn't retried while it's showing
  m_bPlayingVideo = false;
  return false;
}

bool CGUIWindowSlideShow::PlayMotion()
{
  const std::string clip = GetMotionClip(*m_slides.at(m_iCurrentSlide));
  if (clip.empty())
    return false;

  CLog::Log(LOGDEBUG, "Playing the clip of moving photo {}",
            m_slides.at(m_iCurrentSlide)->GetPath());
  m_bPlayingVideo = true;
  m_bPlayingMotion = true;
  m_iVideoSlide = m_iCurrentSlide;
  // The clip is part of the photo, so it mustn't switch the display mode like a film
  const bool startWindowed = CMediaSettings::GetInstance().DoesMediaStartWindowed();
  CMediaSettings::GetInstance().SetMediaStartWindowed(true);
  const bool ret = g_application.PlayFile(CFileItem(clip, false), "");
  CMediaSettings::GetInstance().SetMediaStartWindowed(startWindowed);
  if (!ret)
    EndMotion();
  return ret;
}

void CGUIWindowSlideShow::EndMotion()
{
  m_bPlayingMotion = false;
  m_bPlayingVideo = false;
  m_iVideoSlide = -1;
  MarkDirtyRegion();
}

bool CGUIWindowSlideShow::IsShowingVideo() const
{
  // A reset slideshow has no current slide
  if (m_iCurrentSlide < 0 || m_iCurrentSlide >= static_cast<int>(m_slides.size()))
    return false;
  // A video's thumbnail, or a moving photo, stays on screen until the clip has a picture to show
  if (!IsVideo(*m_slides.at(m_iCurrentSlide)) && !m_bPlayingMotion)
    return false;
  const auto appPlayer = CServiceBroker::GetAppComponents().GetComponent<CApplicationPlayer>();
  return appPlayer->IsRenderingVideo();
}

std::string CGUIWindowSlideShow::GetMotionClip(const CFileItem& item) const
{
  if (item.HasProperty("motionclip"))
    return item.GetProperty("motionclip").asString();

  std::unique_lock lock(m_motionClipsSection);
  const auto clip = m_motionClips.find(item.GetDynPath());
  return clip != m_motionClips.end() ? clip->second : "";
}

void CGUIWindowSlideShow::SetHdrOutput(bool hdr)
{
  if (hdr == m_hdrOutput)
    return;

  CWinSystemBase* winSystem = CServiceBroker::GetWinSystem();
  if (hdr)
  {
    // Ask for the output a 10-bit PQ video would get
    VideoPicture picture{};
    picture.color_primaries = AVCOL_PRI_BT2020;
    picture.color_transfer = AVCOL_TRC_SMPTE2084;
    picture.color_space = AVCOL_SPC_BT2020_NCL;
    picture.colorBits = 10;
    picture.iWidth = static_cast<unsigned int>(winSystem->GetGfxContext().GetWidth());
    picture.iHeight = static_cast<unsigned int>(winSystem->GetGfxContext().GetHeight());

    // Describe the pictures as HDR10 mastered on a 1000-nit display, the brightest a gain map is
    // drawn. Some displays stay in SDR when this is left empty.
    picture.hasDisplayMetadata = true;
    AVMasteringDisplayMetadata& display = picture.displayMetadata;
    display.display_primaries[0][0] = av_make_q(708, 1000);
    display.display_primaries[0][1] = av_make_q(292, 1000);
    display.display_primaries[1][0] = av_make_q(170, 1000);
    display.display_primaries[1][1] = av_make_q(797, 1000);
    display.display_primaries[2][0] = av_make_q(131, 1000);
    display.display_primaries[2][1] = av_make_q(46, 1000);
    display.white_point[0] = av_make_q(3127, 10000);
    display.white_point[1] = av_make_q(3290, 10000);
    display.max_luminance = av_make_q(1000, 1);
    display.min_luminance = av_make_q(5, 1000);
    display.has_primaries = 1;
    display.has_luminance = 1;
    picture.hasLightMetadata = true;
    picture.lightMetadata.MaxCLL = 1000;
    picture.lightMetadata.MaxFALL = 400;

    winSystem->SetVideoOutput(&picture);
    winSystem->SetColorimetry(&picture);
    if (winSystem->SetHDR(&picture) && winSystem->SetGuiCompositing(AVCOL_TRC_SMPTE2084))
    {
      CLog::Log(LOGINFO, "Slideshow: switched the display to HDR for HDR pictures");
      m_hdrOutput = true;
      MarkDirtyRegion();
      return;
    }

    CLog::Log(LOGWARNING, "Slideshow: the display couldn't switch to HDR");
    m_showHdr = false;
  }

  winSystem->SetGuiCompositing(0);
  winSystem->SetHDR(nullptr);
  winSystem->SetColorimetry(nullptr);
  winSystem->SetVideoOutput(nullptr);
  m_hdrOutput = false;
  MarkDirtyRegion();
}

CSlideShowPic::DISPLAY_EFFECT CGUIWindowSlideShow::GetDisplayEffect(int iSlideNumber) const
{
  if (m_bSlideShow && !m_bPause && !IsVideo(*m_slides.at(iSlideNumber)))
    return CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_SLIDESHOW_DISPLAYEFFECTS) ? CSlideShowPic::EFFECT_RANDOM : CSlideShowPic::EFFECT_NONE;
  else
    return CSlideShowPic::EFFECT_NO_TIMEOUT;
}

void CGUIWindowSlideShow::OnLoadPic(int iPic,
                                    int iSlideNumber,
                                    const std::string& strFileName,
                                    std::unique_ptr<CTexture> pTexture,
                                    bool bFullSize,
                                    const std::string& motionClip,
                                    std::unique_ptr<CTexture> gainMap,
                                    const KODI::PICTURE::GainMapParams& gainMapParams)
{
  // the sRGB copy of a picture that kept its colours for HDR replaces it where it still is
  if (std::exchange(m_reloadAsSRGB[iPic], false))
  {
    if (pTexture && m_Image[iPic]->IsLoaded() && m_Image[iPic]->SlideNumber() == iSlideNumber)
      m_Image[iPic]->UpdateTexture(std::move(pTexture));
    return;
  }

  if (pTexture)
  {
    // set the pic's texture + size etc.
    if (iSlideNumber >= static_cast<int>(m_slides.size()) || GetPicturePath(m_slides.at(iSlideNumber).get()) != strFileName)
    { // throw this away - we must have cleared the slideshow while we were still loading
      return;
    }
    CLog::Log(LOGDEBUG, "Finished background loading slot {}, {}: {}", iPic, iSlideNumber,
              m_slides.at(iSlideNumber)->GetPath());
    // Process() looks for the clip as soon as the picture is loaded
    if (!motionClip.empty())
    {
      std::unique_lock lock(m_motionClipsSection);
      m_motionClips[strFileName] = motionClip;
    }
    m_Image[iPic]->SetOriginalSize(pTexture->GetOriginalWidth(), pTexture->GetOriginalHeight(),
                                   bFullSize);
    m_Image[iPic]->SetTexture(iSlideNumber, std::move(pTexture), GetDisplayEffect(iSlideNumber));
    if (gainMap)
      m_Image[iPic]->SetGainMap(std::move(gainMap), gainMapParams);

    m_Image[iPic]->m_bIsComic = false;
    if (URIUtils::IsInRAR(m_slides.at(m_iCurrentSlide)->GetPath()) || URIUtils::IsInZIP(m_slides.at(m_iCurrentSlide)->GetPath())) // move to top for cbr/cbz
    {
      CURL url(m_slides.at(m_iCurrentSlide)->GetPath());
      const std::string& strHostName = url.GetHostName();
      if (URIUtils::HasExtension(strHostName, ".cbr|.cbz"))
      {
        m_Image[iPic]->m_bIsComic = true;
        m_Image[iPic]->Move((float)m_Image[iPic]->GetOriginalWidth(),
                            (float)m_Image[iPic]->GetOriginalHeight());
      }
    }
  }
  else if (iSlideNumber >= static_cast<int>(m_slides.size()) || GetPicturePath(m_slides.at(iSlideNumber).get()) != strFileName)
  { // Failed to load image. and not match values calling LoadPic, then something is changed, ignore.
    CLog::Log(LOGDEBUG,
              "CGUIWindowSlideShow::OnLoadPic({}, {}, {}) on failure not match current state (cur "
              "{}, next {}, curpic {}, pic[0, 1].slidenumber={}, {}, {})",
              iPic, iSlideNumber, strFileName, m_iCurrentSlide, m_iNextSlide, m_iCurrentPic,
              m_Image[0]->SlideNumber(), m_Image[1]->SlideNumber(),
              iSlideNumber >= static_cast<int>(m_slides.size())
                  ? ""
                  : m_slides.at(iSlideNumber)->GetPath());
  }
  else
  { // Failed to load image.  What should be done??
    // We should wait for the current pic to finish rendering, then transition it out,
    // release the texture, and try and reload this pic from scratch
    m_bErrorMessage = true;
  }
  MarkDirtyRegion();
}

void CGUIWindowSlideShow::Shuffle()
{
  KODI::UTILS::RandomShuffle(m_slides.begin(), m_slides.end());
  m_iCurrentSlide = 0;
  m_iNextSlide = GetNextSlide();
  m_bShuffled = true;

  AnnouncePropertyChanged("shuffled", true);
}

int CGUIWindowSlideShow::NumSlides() const
{
  return m_slides.size();
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
  if (!strPath.empty())
  {
    // reset the slideshow
    Reset();
    if (bRecursive)
    {
      path_set recursivePaths;
      AddItems(strPath, &recursivePaths, method, order, sortAttributes);
    }
    else
      AddItems(strPath, NULL, method, order, sortAttributes);
  }
}

void CGUIWindowSlideShow::RunSlideShow(const std::string& strPath,
                                       bool bRecursive /* = false */,
                                       bool bRandom /* = false */,
                                       bool bNotRandom /* = false */,
                                       const std::string& beginSlidePath /* = "" */,
                                       bool startSlideShow /* = true */,
                                       SortBy method /* = SortBy::LABEL */,
                                       SortOrder order /* = SortOrder::ASCENDING */,
                                       SortAttribute sortAttributes /* = SortAttributeNone */,
                                       const std::string& strExtensions)
{
  // stop any video
  const auto& components = CServiceBroker::GetAppComponents();
  const auto appPlayer = components.GetComponent<CApplicationPlayer>();
  if (appPlayer->IsPlayingVideo())
    g_application.StopPlaying();

  AddFromPath(strPath, bRecursive, method, order, sortAttributes, strExtensions);

  if (!NumSlides())
    return;

  // mutually exclusive options
  // if both are set, clear both and use the gui setting
  if (bRandom && bNotRandom)
    bRandom = bNotRandom = false;

  // NotRandom overrides the window setting
  if ((!bNotRandom && CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_SLIDESHOW_SHUFFLE)) || bRandom)
    Shuffle();

  if (!beginSlidePath.empty())
    Select(beginSlidePath);

  if (startSlideShow)
    StartSlideShow();
  else
  {
    PlayPicture();
  }

  CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_SLIDESHOW);
}

void CGUIWindowSlideShow::PlayPicture()
{
  if (m_iCurrentSlide >= 0 && m_iCurrentSlide < static_cast<int>(m_slides.size()))
    AnnouncePlayerPlay(m_slides.at(m_iCurrentSlide));
}

void CGUIWindowSlideShow::AddItems(const std::string &strPath, path_set *recursivePaths, SortBy method, SortOrder order, SortAttribute sortAttributes)
{
  // check whether we've already added this path
  if (recursivePaths)
  {
    std::string path(strPath);
    URIUtils::RemoveSlashAtEnd(path);
    if (recursivePaths->contains(path))
      return;
    recursivePaths->insert(path);
  }

  CFileItemList items;
  CGUIViewStateWindowPictures viewState(items);

  // fetch directory and sort accordingly
  if (!CDirectory::GetDirectory(strPath, items, viewState.GetExtensions(), DIR_FLAG_NO_FILE_DIRS))
    return;

  KODI::PICTURE::GroupLivePhotos(items);

  items.Sort(method, order, sortAttributes);

  // need to go into all subdirs
  for (int i = 0; i < items.Size(); i++)
  {
    CFileItemPtr item = items[i];
    if (item->IsFolder() && recursivePaths)
    {
      AddItems(item->GetPath(), recursivePaths);
    }
    else if (!item->IsFolder() && !URIUtils::IsRAR(item->GetPath()) &&
             !URIUtils::IsZIP(item->GetPath()))
    { // add to the slideshow
      Add(item.get());
    }
  }
}

void CGUIWindowSlideShow::GetCheckedSize(float width, float height, int &maxWidth, int &maxHeight)
{
  maxWidth = CServiceBroker::GetRenderSystem()->GetMaxTextureSize();
  maxHeight = CServiceBroker::GetRenderSystem()->GetMaxTextureSize();
}

std::string CGUIWindowSlideShow::GetPicturePath(CFileItem *item)
{
  bool isVideo = IsVideo(*item);
  std::string picturePath = item->GetDynPath();
  if (isVideo)
  {
    picturePath = item->GetArt(ART::TYPE::THUMB);
    if (picturePath.empty() && !item->HasProperty("nothumb"))
    {
      CPictureThumbLoader thumbLoader;
      thumbLoader.LoadItem(item);
      picturePath = item->GetArt(ART::TYPE::THUMB);
      if (picturePath.empty())
        item->SetProperty("nothumb", true);
    }
  }
  return picturePath;
}


void CGUIWindowSlideShow::RunSlideShow(const std::vector<std::string>& paths, int start /* = 0*/)
{
  auto dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIWindowSlideShow>(WINDOW_SLIDESHOW);
  if (dialog)
  {
    std::vector<CFileItemPtr> items;
    items.reserve(paths.size());
    for (const auto& path : paths)
      items.push_back(std::make_shared<CFileItem>(IMAGE_FILES::URLFromFile(path), false));

    dialog->Reset();
    dialog->m_bPause = true;
    dialog->m_bSlideShow = false;
    dialog->m_iDirection = 1;
    dialog->m_iCurrentSlide = start;
    dialog->m_iNextSlide = (start + 1) % items.size();
    dialog->m_slides = std::move(items);
    dialog->Open();
  }
}
