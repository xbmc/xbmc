
#include "stdafx.h"
#include "GUIDialogVideoBookmarks.h"
#include "VideoDatabase.h"
#include "Application.h"
#include "Util.h"
#include "cores/VideoRenderers/RenderManager.h"
#include "Picture.h"

#define CONTROL_ADD_BOOKMARK     2
#define CONTROL_CLEAR_BOOKMARKS  3

#define CONTROL_LIST             10
#define CONTROL_THUMBS           11

CGUIDialogVideoBookmarks::CGUIDialogVideoBookmarks()
    : CGUIDialog(WINDOW_DIALOG_VIDEO_BOOKMARKS, "VideoOSDBookmarks.xml")
{
}

CGUIDialogVideoBookmarks::~CGUIDialogVideoBookmarks()
{
}

bool CGUIDialogVideoBookmarks::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      Clear();
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      CGUIWindow::OnMessage(message);
      Update();
      return true;
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_ADD_BOOKMARK)
      {
        AddBookmark();
      }
      else if (iControl == CONTROL_CLEAR_BOOKMARKS)
      {
        ClearBookmarks();
      }
      else if (m_viewControl.HasControl(iControl))  // list/thumb control
      {
        int iItem = m_viewControl.GetSelectedItem();
        GotoBookmark(iItem);
      }
    }
    break;
  case GUI_MSG_SETFOCUS:
    {
      if (m_viewControl.HasControl(message.GetControlId()) && m_viewControl.GetCurrentControl() != message.GetControlId())
      {
        m_viewControl.SetFocused();
        return true;
      }
    }
    break;
  }
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogVideoBookmarks::Update()
{
  // lock our display, as this window is rendered from the player thread
  g_graphicsContext.Lock();
  m_viewControl.SetCurrentView(1);

  VECBOOKMARKS bookmarks;

  // open the d/b and retrieve the bookmarks for the current movie
  CVideoDatabase videoDatabase;
  videoDatabase.Open();
  videoDatabase.GetBookMarksForMovie(g_application.CurrentFile(), bookmarks);
  videoDatabase.Close();

  // empty the list ready for population
  Clear();

  // cycle through each stored bookmark and add it to our list control
  for (unsigned int i = 0; i < bookmarks.size(); ++i)
  {
    CStdString bookmarkTime;
    CUtil::SecondsToHMSString(bookmarks[i].timeInSeconds, bookmarkTime, true);
    CFileItem *item = new CFileItem(bookmarkTime);
    item->SetThumbnailImage(bookmarks[i].thumbNailImage);
    m_vecItems.Add(item);
  }
  m_viewControl.SetItems(m_vecItems);
  g_graphicsContext.Unlock();
}

void CGUIDialogVideoBookmarks::Clear()
{
  m_viewControl.Clear();
  m_vecItems.Clear();
}

void CGUIDialogVideoBookmarks::GotoBookmark(int item)
{
  CVideoDatabase videoDatabase;
  videoDatabase.Open();
  VECBOOKMARKS bookmarks;
  videoDatabase.GetBookMarksForMovie(g_application.CurrentFile(), bookmarks);
  videoDatabase.Close();
  if (item < 0 || item >= (int)bookmarks.size()) return;
  if (g_application.m_pPlayer)
    g_application.m_pPlayer->SeekTime((__int64)bookmarks[item].timeInSeconds * 1000);
}

void CGUIDialogVideoBookmarks::ClearBookmarks()
{
  CVideoDatabase videoDatabase;
  videoDatabase.Open();
  videoDatabase.ClearBookMarksOfMovie(g_application.CurrentFile());
  videoDatabase.Close();
  Update();
}

void CGUIDialogVideoBookmarks::AddBookmark()
{
  CVideoDatabase videoDatabase;
  CBookmark bookmark;
  bookmark.timeInSeconds = (int)(g_application.m_pPlayer->GetTime() / 1000);
  // create the thumbnail image
  RECT rs, rd;
  g_renderManager.GetVideoRect(rs, rd);
  int width = 128;
  float aspectRatio = g_renderManager.GetAspectRatio();
  int height = (int)(128.0f / aspectRatio);
  if (height > 128)
  {
    height = 128;
    width = (int)(128.0f * aspectRatio);
  }
  LPDIRECT3DTEXTURE8 texture = NULL;
  if (D3D_OK == g_graphicsContext.Get3DDevice()->CreateTexture(width, height, 1, 0, D3DFMT_LIN_A8R8G8B8, 0, &texture))
  {
    LPDIRECT3DSURFACE8 surface = NULL;
    texture->GetSurfaceLevel(0, &surface);
    g_renderManager.CreateThumbnail(surface, width, height);
    D3DLOCKED_RECT lockedRect;
    surface->LockRect(&lockedRect, NULL, NULL);
    // compute the thumb name + create the thumb image
    Crc32 crc;
    crc.ComputeFromLowerCase(g_application.CurrentFile());
    bookmark.thumbNailImage.Format("%s\\bookmarks\\%08x_%i.jpg", g_stSettings.szThumbnailsDirectory, crc, m_vecItems.Size() + 1);
    CPicture pic;
    if (!pic.CreateThumbnailFromSurface((BYTE *)lockedRect.pBits, width, height, lockedRect.Pitch, bookmark.thumbNailImage))
      bookmark.thumbNailImage.Empty();
    surface->UnlockRect();
    surface->Release();
    texture->Release();
  }
  videoDatabase.Open();
  videoDatabase.AddBookMarkToMovie(g_application.CurrentFile(), bookmark);
  videoDatabase.Close();
  Update();
}

void CGUIDialogVideoBookmarks::OnWindowLoaded()
{
  CGUIDialog::OnWindowLoaded();
  m_viewControl.Reset();
  m_viewControl.SetParentWindow(GetID());
  m_viewControl.AddView(VIEW_AS_ICONS, GetControl(CONTROL_THUMBS));
}

void CGUIDialogVideoBookmarks::OnWindowUnload()
{
  CGUIDialog::OnWindowUnload();
  m_viewControl.Reset();
}