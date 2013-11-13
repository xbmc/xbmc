#include "GUIDialogRating.h"
#include "GUIWindowManager.h"
#include "GUIImage.h"
#include "Key.h"
#include "PlexTypes.h"
#include "threads/SingleLock.h"

#include "video/VideoInfoTag.h"
#include "PlexDirectory.h"
#include "PlexApplication.h"
#include "Client/PlexMediaServerClient.h"

CGUIDialogRating::CGUIDialogRating(void)
: CGUIDialog(WINDOW_DIALOG_RATING, "DialogRating.xml")
{
  m_loadType = LOAD_ON_GUI_INIT;
  m_iRating = 5;
  m_bConfirmed = false;
}

CGUIDialogRating::~CGUIDialogRating(void)
{
}

bool CGUIDialogRating::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_MOVE_LEFT && m_iRating > 0)
  {
    SetRating(m_iRating-1);
    return true;
  }
  else if (action.GetID() == ACTION_MOVE_RIGHT && m_iRating < 10)
  {
    SetRating(m_iRating+1);
    return true;
  }
  else if (action.GetID() == ACTION_MOVE_UP && m_iRating < 10)
  {
    SetRating(m_iRating+2);
    return true;
  }
  else if (action.GetID() == ACTION_MOVE_DOWN && m_iRating > 0)
  {
    SetRating(m_iRating-2);
    return true;
  }
  else if (action.GetID() == ACTION_SELECT_ITEM)
  {
    m_bConfirmed = true;
    Close(false);
  }
  return CGUIDialog::OnAction(action);
}

void CGUIDialogRating::SetHeading(int iHeading)
{
  Initialize();
  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), 1);
  msg.SetLabel(iHeading);
  
  CSingleTryLock tryLock(g_graphicsContext);
  if (tryLock.IsOwner())
    CGUIDialog::OnMessage(msg);
  else
    g_windowManager.SendThreadMessage(msg, GetID());
}

void CGUIDialogRating::SetTitle(const CStdString& strTitle)
{
  Initialize();
  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), 2);
  msg.SetLabel(strTitle);

  CSingleTryLock tryLock(g_graphicsContext);
  if (tryLock.IsOwner())
    CGUIDialog::OnMessage(msg);
  else
    g_windowManager.SendThreadMessage(msg, GetID());
}

void CGUIDialogRating::SetRating(float iRating)
{
  Initialize();
  
  CGUIImage* image = (CGUIImage*)GetControl(10);
  if (!image)
    return;

  CStdString fileName;
  fileName.Format("rating%d.png", (int)(iRating + 0.5f));
  image->SetFileName(fileName);
  m_iRating = (int)iRating;
  SetInvalid();
  
  if (m_fileItem)
  {
    g_plexApplication.mediaServerClient->SetItemRating(m_fileItem, iRating);
    m_fileItem->SetProperty("userRating", iRating);
    
    if (iRating == 0)
    {
      /* If we set it to 0 we need to switch to IMDb ratings instead */
      m_fileItem->SetProperty("hasUserRating", "");
      iRating = m_fileItem->GetProperty("rating").asDouble();
    }
    else
      m_fileItem->SetProperty("hasUserRating", "yes");
    
    if (m_fileItem->HasVideoInfoTag())
      m_fileItem->GetVideoInfoTag()->m_fRating = iRating;
  }
}

bool CGUIDialogRating::OnMessage(CGUIMessage &msg)
{
  
  if (msg.GetMessage() == GUI_MSG_WINDOW_INIT)
  {
    CFileItemPtr item;
    if (g_plexApplication.m_preplayItem)
    {
      item = g_plexApplication.m_preplayItem;
    }
    else if (!msg.GetStringParam().empty())
    {
      XFILE::CPlexDirectory dir;
      CFileItemList list;
      
      if (dir.GetDirectory(msg.GetStringParam(), list))
      {
        if (list.Size() > 0)
          item = list.Get(0);
      }
    }
    
    if (item)
    {
      bool hasUserRating = item->HasProperty("userRating") && item->GetProperty("userRating").asDouble() > 0.0;
      SetHeading(hasUserRating ? 40208 : 40207);
      
      if (item->HasProperty("userRating"))
        SetRating(item->GetProperty("userRating").asDouble());
      else if (item && item->HasVideoInfoTag())
      {
        SetRating(item->GetVideoInfoTag()->m_fRating);
        SetTitle(item->GetVideoInfoTag()->m_strTitle);
      }
      
      m_fileItem = item;
    }
    
  }
  
  return CGUIDialog::OnMessage(msg);
}
