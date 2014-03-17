#include "GUIPlexScreenSaverPhoto.h"
#include "PlexTypes.h"
#include "Application.h"
#include "guilib/GUIMultiImage.h"
#include "guilib/GUIWindowManager.h"
#include "Application.h"
#include "GraphicContext.h"
#include "JobManager.h"
#include "PlexJobs.h"
#include "Client/PlexServerManager.h"
#include "PlexApplication.h"
#include "settings/GUISettings.h"
#include "File.h"
#include "guilib/GUIFontManager.h"
#include "GUIInfoManager.h"
#include "addons/AddonManager.h"

using namespace XFILE;

///////////////////////////////////////////////////////////////////////////////////////////////////
CGUIPlexScreenSaverPhoto::CGUIPlexScreenSaverPhoto() : CGUIDialog(WINDOW_DIALOG_PLEX_SS_PHOTOS, "")
{
  m_needsScaling = false;
  m_animations.push_back(CAnimation::CreateFader(0, 100, 0, 700, ANIM_TYPE_WINDOW_OPEN));
  m_animations.push_back(CAnimation::CreateFader(100, 0, 0, 700, ANIM_TYPE_WINDOW_CLOSE));

  m_renderOrder = INT_MAX;
  m_multiImage = NULL;
  m_overlayImage = NULL;
  m_clockLabel = NULL;
  m_imageLabel = NULL;
  m_currentPosition = TOP_LEFT;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIPlexScreenSaverPhoto::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_INIT:
    {
      if (!m_multiImage)
      {
        CPlexServerPtr server = g_plexApplication.serverManager->GetBestServer();
        if (!server)
          return false;

        CURL art = server->BuildPlexURL("/library/arts");

        CJobManager::GetInstance().AddJob(new CPlexDirectoryFetchJob(art), this);

        if (XFILE::CFile::Exists("special://xbmc/media/SlideshowOverlay.png"))
        {
          if (m_overlayImage)
            delete m_overlayImage;

          m_overlayImage = new CGUIImage(GetID(), 1235, 0, 0,
                                         g_graphicsContext.GetWidth(),
                                         g_graphicsContext.GetHeight(),
                                         CTextureInfo());
          m_overlayImage->SetFileName("special://xbmc/media/SlideshowOverlay.png");
        }


        CLabelInfo info;
        info.textColor = 0xfff5f5f5;
        info.font = g_fontManager.GetFont("Regular-30", true);
        if (!info.font)
        {
          CLog::Log(LOGWARNING, "Cant find font DefaultFont!");
        }
        info.shadowColor = 0xff000000;

        /* we need to get settings */
        ADDON::AddonPtr screensaver;
        ADDON::CAddonMgr::Get().GetAddon("screensaver.xbmc.builtin.plexphotos", screensaver, ADDON::ADDON_SCREENSAVER);
        if (!screensaver)
          return false;

        if (m_clockLabel)
          delete m_clockLabel;

        int posX, posY, align;
        getXYPosition(posX, posY, align);

        if (screensaver->GetSetting("showclock") == "true")
        {
          m_clockLabel = new CGUILabelControl(GetID(), 1236, posX, posY,
                                              400, 50, info, false, false);
          m_clockLabel->SetLabel("00:00");
          m_clockLabel->SetAlignment(align);
          m_clockLabel->UpdateInfo();
          m_clockLabel->SetVisible(true);
          posY += 50;
        }

        if (m_imageLabel)
          delete m_imageLabel;

        if (screensaver->GetSetting("showinfo") == "true")
        {
          info.font = g_fontManager.GetFont("Regular-18", true);
          m_imageLabel = new CGUILabelControl(GetID(), 1237, posX, posY, 400, 50, info, false, false);
          m_imageLabel->SetLabel("No images found...");
          m_imageLabel->SetAlignment(align);
          m_imageLabel->UpdateInfo();
        }

        m_moveTimer.restart();

      }
      break;
    }
    case GUI_MSG_WINDOW_DEINIT:
    {
      if (m_multiImage)
        delete m_multiImage;
      m_multiImage = NULL;

      if (m_overlayImage)
        delete m_overlayImage;
      m_overlayImage = NULL;

      if (m_clockLabel)
        delete m_clockLabel;
      m_clockLabel = NULL;

      if (m_imageLabel)
        delete m_imageLabel;
      m_imageLabel = NULL;

      break;
    }

  }
  return CGUIDialog::OnMessage(message);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIPlexScreenSaverPhoto::getXYPosition(int& x, int& y, int& alignment)
{
  switch (m_currentPosition)
  {
    case TOP_LEFT:
      x = 50;
      y = 25;
      alignment = XBFONT_LEFT;
      break;
    case BOTTOM_LEFT:
      x = 50;
      y = g_graphicsContext.GetHeight() - 125;
      alignment = XBFONT_LEFT;
      break;
    case TOP_RIGHT:
      x = g_graphicsContext.GetWidth() - 450;
      y = 25;
      alignment = XBFONT_RIGHT;
      break;
    case BOTTOM_RIGHT:
      x = g_graphicsContext.GetWidth() - 450;
      y = g_graphicsContext.GetHeight() - 125;
      alignment = XBFONT_RIGHT;
      break;
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIPlexScreenSaverPhoto::moveLabels()
{
  switch (m_currentPosition)
  {
    case TOP_LEFT:
      m_currentPosition = BOTTOM_RIGHT;
      break;
    case BOTTOM_RIGHT:
      m_currentPosition = BOTTOM_LEFT;
      break;
    case BOTTOM_LEFT:
      m_currentPosition = TOP_RIGHT;
      break;
    case TOP_RIGHT:
      m_currentPosition = TOP_LEFT;
      break;
  }

  int x, y, align;

  getXYPosition(x, y, align);

  if (m_clockLabel)
  {
    m_clockLabel->SetPosition(x, y);
    m_clockLabel->SetAlignment(align);
    m_clockLabel->MarkDirtyRegion();
    y += 50;
  }

  if (m_imageLabel)
  {
    m_imageLabel->SetPosition(x, y);
    m_imageLabel->SetAlignment(align);
    m_imageLabel->MarkDirtyRegion();
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIPlexScreenSaverPhoto::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  CGUIDialog::Process(currentTime, dirtyregions);


  if (m_moveTimer.elapsedMs() >= 2 * 60 * 1000)
  {
    moveLabels();
    m_moveTimer.restart();
  }

  if (m_multiImage)
    m_multiImage->Process(currentTime, dirtyregions);

  if (m_overlayImage)
    m_overlayImage->Process(currentTime, dirtyregions);

  if (m_clockLabel)
  {
    m_clockLabel->Process(currentTime, dirtyregions);
    m_clockLabel->SetLabel(g_infoManager.GetTime(TIME_FORMAT_GUESS));
    m_clockLabel->UpdateInfo();
  }

  if (m_imageLabel)
  {
    m_imageLabel->Process(currentTime, dirtyregions);
    if (m_multiImage)
    {
      m_imageLabel->SetLabel(m_multiImage->GetCurrentPlexLabel());
      m_imageLabel->UpdateInfo();
    }
  }

  m_renderRegion.SetRect(0, 0, (float)g_graphicsContext.GetWidth(), (float)g_graphicsContext.GetHeight());
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIPlexScreenSaverPhoto::UpdateVisibility()
{
  // such a hack
  if (g_application.GetDimScreenSaverLevel() == 0.042f)
    Show();
  else
    Close();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIPlexScreenSaverPhoto::Render()
{
  CRect rect(0, 0, (float)g_graphicsContext.GetWidth(), (float)g_graphicsContext.GetHeight());

  CGUITexture::DrawQuad(rect, 0xff000000);

  if (m_active)
  {
    if (m_multiImage)
      m_multiImage->Render();

    if (m_overlayImage)
      m_overlayImage->Render();

    if (m_clockLabel)
      m_clockLabel->Render();

    if (m_imageLabel)
      m_imageLabel->Render();
  }

  CGUIDialog::Render();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIPlexScreenSaverPhoto::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  if (success)
  {
    CPlexDirectoryFetchJob *fj = static_cast<CPlexDirectoryFetchJob*>(job);
    if (fj)
    {
      m_images = CFileItemListPtr(new CFileItemList());
      m_images->Assign(fj->m_items);

      m_multiImage = new CGUIMultiImage(GetID(), 1234, 0, 0,
                                        g_graphicsContext.GetWidth(),
                                        g_graphicsContext.GetHeight(),
                                        CTextureInfo(),
                                        15000, 500, true, true, 0);
      m_multiImage->SetVisible(true);

      m_multiImage->SetAspectRatio(CAspectRatio(CAspectRatio::AR_KEEP));

      CGUIMessage msg(GUI_MSG_LABEL_BIND, 0, m_multiImage->GetID(), 0, 0, m_images.get());
      m_multiImage->OnMessage(msg);

   }
  }
}
