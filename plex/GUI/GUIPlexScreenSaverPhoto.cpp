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

///////////////////////////////////////////////////////////////////////////////////////////////////
CGUIPlexScreenSaverPhoto::CGUIPlexScreenSaverPhoto() : CGUIDialog(WINDOW_DIALOG_PLEX_SS_PHOTOS, "")
{
  m_needsScaling = false;
  m_animations.push_back(CAnimation::CreateFader(0, 100, 0, 1000, ANIM_TYPE_WINDOW_OPEN));
  m_animations.push_back(CAnimation::CreateFader(100, 0, 0, 1000, ANIM_TYPE_WINDOW_CLOSE));
  m_renderOrder = INT_MAX;
  m_multiImage = NULL;
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
      }
      break;
    }
    case GUI_MSG_WINDOW_DEINIT:
    {
      if (m_multiImage)
        delete m_multiImage;
      m_multiImage = NULL;
      break;
    }

  }
  return CGUIDialog::OnMessage(message);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIPlexScreenSaverPhoto::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  CGUIDialog::Process(currentTime, dirtyregions);

  if (m_multiImage)
    m_multiImage->Process(currentTime, dirtyregions);

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
  CGUIDialog::Render();

  CRect rect(0, 0, (float)g_graphicsContext.GetWidth(), (float)g_graphicsContext.GetHeight());
  CGUITexture::DrawQuad(rect, 0xff000000);

  if (m_active && m_multiImage)
    m_multiImage->Render();
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
                                        1, 20000, true, true, 12000);
      m_multiImage->SetVisible(true);

      CGUIMessage msg(GUI_MSG_LABEL_BIND, 0, m_multiImage->GetID(), 0, 0, m_images.get());
      m_multiImage->OnMessage(msg);
    }
  }
}
