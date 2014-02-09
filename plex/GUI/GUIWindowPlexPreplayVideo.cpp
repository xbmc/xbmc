
#include "GUIDialogPlexAudioSubtitlePicker.h"

#include "GUIWindowPlexPreplayVideo.h"
#include "plex/PlexTypes.h"
#include "guilib/GUIWindow.h"
#include "filesystem/Directory.h"

#include "FileItem.h"
#include "guilib/GUILabelControl.h"
#include "ApplicationMessenger.h"
#include "PlexApplication.h"
#include "PlexContentPlayerMixin.h"
#include "PlexThemeMusicPlayer.h"

#include "dialogs/GUIDialogBusy.h"
#include "dialogs/GUIDialogKeyboardGeneric.h"
#include "dialogs/GUIDialogYesNo.h"
#include "PlexJobs.h"
#include "Client/PlexServerManager.h"

#include "DirectoryCache.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
CGUIWindowPlexPreplayVideo::CGUIWindowPlexPreplayVideo(void)
 : CGUIMediaWindow(WINDOW_PLEX_PREPLAY_VIDEO, "PlexPreplayVideo.xml")
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CGUIWindowPlexPreplayVideo::~CGUIWindowPlexPreplayVideo()
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIWindowPlexPreplayVideo::OnMessage(CGUIMessage &message)
{
  bool ret = CGUIMediaWindow::OnMessage(message);

  if (message.GetMessage() == GUI_MSG_WINDOW_INIT)
  {
    if (!message.GetStringParam(2).empty())
      m_parentPath = message.GetStringParam(2);

    UpdateItem();
  }
  else if (message.GetMessage() == GUI_MSG_WINDOW_DEINIT)
  {
    CFileItemPtr item = g_plexApplication.m_preplayItem;
    g_plexApplication.m_preplayItem.reset();

    CPlexServerPtr server = g_plexApplication.serverManager->FindFromItem(item);
    if (server && server->GetActiveConnection() && server->GetActiveConnection()->IsLocal())
    {
      CLog::Log(LOGDEBUG, "CGUIWindowPlexPreplayVideo::OnMessage(deinit) killing local item out of directory cache");
      g_directoryCache.ClearDirectory(item->GetPath());
    }

  }
  else if (message.GetMessage() == GUI_MSG_CLICKED)
  {
    if (message.GetSenderId() == 106)
      Recommend();
    else if (message.GetSenderId() == 107)
      Share();
  }

  return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIWindowPlexPreplayVideo::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_PLAYER_PLAY)
  {
    PlexContentPlayerMixin::PlayPlexItem(g_plexApplication.m_preplayItem);
    return true;
  }
  else if (action.GetID() == ACTION_TOGGLE_WATCHED)
  {
    if (m_vecItems->Get(0)->GetOverlayImageID() == CGUIListItem::ICON_OVERLAY_WATCHED)
      m_vecItems->Get(0)->MarkAsUnWatched();
    else
      m_vecItems->Get(0)->MarkAsWatched();
    return true;
  }
  else if (action.GetID() == ACTION_MARK_AS_UNWATCHED)
  {
    m_vecItems->Get(0)->MarkAsUnWatched();
    SetInvalid();
    return true;
  }
  else if (action.GetID() == ACTION_MARK_AS_WATCHED)
  {
    m_vecItems->Get(0)->MarkAsWatched();
    SetInvalid();
    return true;
  }
  else if (action.GetID() == ACTION_MOVE_RIGHT)
  {
    MoveToItem(1);
  }
  else if (action.GetID() == ACTION_MOVE_LEFT)
  {
    MoveToItem(-1);
  }
  
  return CGUIMediaWindow::OnAction(action);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIWindowPlexPreplayVideo::MoveToItem(int idx)
{
  CFileItemPtr item = m_vecItems->Get(0);
  if (item && item->GetPlexDirectoryType() == PLEX_DIR_TYPE_EPISODE)
  {
    if (item->HasProperty("parentKey"))
    {
      bool cancel;

      CURL u(item->GetProperty("parentKey").asString());
      PlexUtils::AppendPathToURL(u, "children");

      if (m_navHelper.CacheUrl(u.Get(), cancel, false))
      {
        CFileItemList list;
        if (GetDirectory(u.Get(), list))
        {
          for (int i = 0; i < list.Size(); i ++)
          {
            if (list.Get(i)->GetPath() == item->GetPath())
            {
              CFileItemPtr i2 = list.Get(i + idx);
              if (!i2)
                return;

              if (m_navHelper.CacheUrl(i2->GetPath(), cancel))
                Update(i2->GetPath(), false);
            }
          }
        }
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIWindowPlexPreplayVideo::Recommend()
{
  if (m_friends.Size() < 1)
  {
    m_dataLoaded.Reset();

    CJobManager::GetInstance().AddJob(new CPlexDirectoryFetchJob(CURL("plexserver://myplex/pms/friends/all.xml")), this, CJob::PRIORITY_HIGH);
    CGUIDialogBusy* dialog = (CGUIDialogBusy*)g_windowManager.GetWindow(WINDOW_DIALOG_BUSY);

    if (dialog)
      dialog->Show();

    while (true)
    {
      if (m_dataLoaded.WaitMSec(1))
        break;

      if (dialog && dialog->IsCanceled())
        break;

      g_windowManager.ProcessRenderLoop(false);
    }

    dialog->Close();
  }

  if (m_friends.Size() > 0)
  {
    CGUIDialogSelect *select = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
    select->SetItems(&m_friends);
    select->SetMultiSelection(true);
    select->DoModal();
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIWindowPlexPreplayVideo::Share()
{
  if (m_networks.Size() < 1)
  {
    m_dataLoaded.Reset();

    CJobManager::GetInstance().AddJob(new CPlexDirectoryFetchJob(CURL("plexserver://myplex/pms/social/networks.xml")), this, CJob::PRIORITY_HIGH);
    CGUIDialogBusy* dialog = (CGUIDialogBusy*)g_windowManager.GetWindow(WINDOW_DIALOG_BUSY);

    if (dialog)
      dialog->Show();

    while (true)
    {
      if (m_dataLoaded.WaitMSec(1))
        break;

      if (dialog && dialog->IsCanceled())
        break;

      g_windowManager.ProcessRenderLoop(false);
    }

    dialog->Close();
  }

  CFileItemList linkedNetworks;
  if (m_networks.Size() > 0)
  {
    for(int i=0; i < m_networks.Size(); i++)
    {
      CFileItemPtr network = m_networks.Get(i);
      if (network->GetProperty("linked").asBoolean())
        linkedNetworks.Add(network);
    }
  }

  CFileItemPtr net;
  if (linkedNetworks.Size() < 1)
  {
    CGUIDialogOK::ShowAndGetInput(g_localizeStrings.Get(52400), "", g_localizeStrings.Get(52401), "");
    return;
  }
  else if (linkedNetworks.Size() == 1)
  {
    net = linkedNetworks.Get(0);
  }

  if (net)
  {
    CStdString message;
    CGUIDialogKeyboardGeneric *keyb = (CGUIDialogKeyboardGeneric *)g_windowManager.GetWindow(WINDOW_DIALOG_KEYBOARD);
    if (keyb)
    {
      if (keyb->ShowAndGetInput(NULL, "", message, g_localizeStrings.Get(52402), false))
      {
        bool canceled;
        CStdString shareStr;
        shareStr.Format(g_localizeStrings.Get(52403), m_vecItems->Get(0)->GetLabel(), net->GetLabel());
        CGUIDialogYesNo::ShowAndGetInput(g_localizeStrings.Get(750), shareStr, message, "", canceled);
        if (!canceled)
        {
          g_plexApplication.mediaServerClient->share(m_vecItems->Get(0), net->GetProperty("unprocessed_key").asString(), message);
        }
      }
    }
  }

}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIWindowPlexPreplayVideo::OnBack(int actionID)
{
  g_windowManager.PreviousWindow();
  return true;
}

///////////////////////////////////the////////////////////////////////////////////////////////////////
bool CGUIWindowPlexPreplayVideo::Update(const CStdString &strDirectory, bool updateFilterPath)
{
  bool ret = CGUIMediaWindow::Update(strDirectory, updateFilterPath);

  if (ret)
    UpdateItem();

  return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIWindowPlexPreplayVideo::UpdateItem()
{
  if (m_vecItems->GetContent() == "movies")
    m_vecItems->SetContent("movie");
  if (m_vecItems->GetContent() == "episodes")
    m_vecItems->SetContent("episode");
  if (m_vecItems->GetContent() == "clips")
    m_vecItems->SetContent("clip");

  m_vecItems->SetProperty("PlexPreplay", "yes");

  g_plexApplication.m_preplayItem = m_vecItems->Get(0);
  g_plexApplication.themeMusicPlayer->playForItem(*m_vecItems->Get(0));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIWindowPlexPreplayVideo::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  CPlexDirectoryFetchJob *fjob = static_cast<CPlexDirectoryFetchJob*>(job);
  if (success && fjob)
  {
    if (fjob->m_url.GetFileName() == "pms/friends/all.xml")
      m_friends.Copy(fjob->m_items);
    else if (fjob->m_url.GetFileName() == "pms/social/networks.xml")
      m_networks.Copy(fjob->m_items);
  }
  m_dataLoaded.Set();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CFileItemPtr CGUIWindowPlexPreplayVideo::GetCurrentListItem(int offset)
{
  if (offset == 0 && m_vecItems->Size() > 0)
    return m_vecItems->Get(0);
  
  return CFileItemPtr();
}
