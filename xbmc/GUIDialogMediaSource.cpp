#include "stdafx.h"
#include "GUIDialogMediaSource.h"
#include "GUIDialogKeyboard.h"
#include "GUIDialogFileBrowser.h"
#include "Util.h"

#define CONTROL_HEADING         2
#define CONTROL_PATH            10
#define CONTROL_PATH_BROWSE     11
#define CONTROL_NAME            12
#define CONTROL_OK              18
#define CONTROL_CANCEL          19

CGUIDialogMediaSource::CGUIDialogMediaSource(void)
    : CGUIDialog(WINDOW_DIALOG_MEDIA_SOURCE, "DialogMediaSource.xml")
{
}

CGUIDialogMediaSource::~CGUIDialogMediaSource()
{
}

bool CGUIDialogMediaSource::OnAction(const CAction &action)
{
  if (action.wID == ACTION_PREVIOUS_MENU)
    m_confirmed = false;
  return CGUIDialog::OnAction(action);
}

bool CGUIDialogMediaSource::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_PATH)
        OnPath();
      else if (iControl == CONTROL_PATH_BROWSE)
        OnPathBrowse();
      else if (iControl == CONTROL_NAME)
        OnName();
      else if (iControl == CONTROL_OK)
        OnOK();
      else if (iControl == CONTROL_CANCEL)
        OnCancel();
      return true;
    }
    break;
  case GUI_MSG_WINDOW_INIT:
    {
      UpdateButtons();
    }
    break;
  }
  return CGUIDialog::OnMessage(message);
}

// \brief Show CGUIDialogMediaSource dialog and prompt for a new media source.
// \return True if the media source is added, false otherwise.
bool CGUIDialogMediaSource::ShowAndAddMediaSource(const CStdString &type)
{
  CGUIDialogMediaSource *dialog = (CGUIDialogMediaSource *)m_gWindowManager.GetWindow(WINDOW_DIALOG_MEDIA_SOURCE);
  if (!dialog) return false;
  dialog->Initialize();
  dialog->SetPathAndName("", "");
  dialog->SetTypeOfMedia(type);
  dialog->DoModal(m_gWindowManager.GetActiveWindow());
  if (dialog->IsConfirmed())
  { // yay, add this share
    g_settings.AddBookmark(type, dialog->m_name, dialog->m_path);
    return true;
  }
  return false;
}

bool CGUIDialogMediaSource::ShowAndEditMediaSource(const CStdString &type, const CStdString &name, const CStdString &path)
{
  CGUIDialogMediaSource *dialog = (CGUIDialogMediaSource *)m_gWindowManager.GetWindow(WINDOW_DIALOG_MEDIA_SOURCE);
  if (!dialog) return false;
  dialog->Initialize();
  dialog->SetPathAndName(path, name);
  dialog->SetTypeOfMedia(type, true);
  dialog->DoModal(m_gWindowManager.GetActiveWindow());
  if (dialog->IsConfirmed())
  { // yay, add this share
    g_settings.UpdateBookmark(type, name, "path", dialog->m_path);
    g_settings.UpdateBookmark(type, name, "name", dialog->m_name);
    return true;
  }
  return false;
}

void CGUIDialogMediaSource::OnPathBrowse()
{
  // Browse is called.  Open the filebrowser dialog.
  // Ignore current path is best at this stage??
  CStdString path;
  bool allowNetworkShares = m_gWindowManager.GetActiveWindow() != WINDOW_PROGRAMS;
  if (CGUIDialogFileBrowser::ShowAndGetShare(path, allowNetworkShares))
  {
    m_path = path;
    if (m_name.IsEmpty())
    {
      m_name = m_path;
      if (CUtil::HasSlashAtEnd(m_name))
        m_name.Delete(m_name.size() - 1);
      m_name = CUtil::GetTitleFromPath(m_name);
    }
    UpdateButtons();
  }
}

void CGUIDialogMediaSource::OnPath()
{
  CGUIDialogKeyboard::ShowAndGetInput(m_path, g_localizeStrings.Get(1021), false);
  UpdateButtons();
}

void CGUIDialogMediaSource::OnName()
{
  CGUIDialogKeyboard::ShowAndGetInput(m_name, g_localizeStrings.Get(1022), false);
  UpdateButtons();
}

void CGUIDialogMediaSource::OnOK()
{
  // verify the path by doing a GetDirectory.
  CFileItemList items;
  if (CDirectory::GetDirectory(m_path, items, "", false, true) || CGUIDialogYesNo::ShowAndGetInput(1001,1025,1003,1004))
  {
    m_confirmed = true;
    Close();
  }
}

void CGUIDialogMediaSource::OnCancel()
{
  m_confirmed = false;
  Close();
}

void CGUIDialogMediaSource::UpdateButtons()
{
  if (m_path.IsEmpty() || m_name.IsEmpty())
  {
    CONTROL_DISABLE(CONTROL_OK);
  }
  else
  {
    CONTROL_ENABLE(CONTROL_OK);
  }
  // path and name
  CStdString path;
  CURL url(m_path);
  url.GetURLWithoutUserDetails(path);
  SET_CONTROL_LABEL(CONTROL_PATH, path);
  SET_CONTROL_LABEL(CONTROL_NAME, m_name);
}

void CGUIDialogMediaSource::SetPathAndName(const CStdString &path, const CStdString &name)
{
  m_path = path;
  m_name = name;
  UpdateButtons();
}

void CGUIDialogMediaSource::SetTypeOfMedia(const CStdString &type, bool editNotAdd)
{
  int typeStringID = -1;
  if (type == "music")
    typeStringID = 249; // "Music"
  else if (type == "video")
    typeStringID = 291;  // "Video"
  else if (type == "myprograms")
    typeStringID = 350;  // "Programs"
  else if (type == "pictures")
    typeStringID = 1213;  // "Pictures"
  else // if (type == "files");
    typeStringID = 744;  // "Files"
  CStdString format;
  format.Format(g_localizeStrings.Get(editNotAdd ? 1028 : 1020).c_str(), g_localizeStrings.Get(typeStringID).c_str());
  SET_CONTROL_LABEL(CONTROL_HEADING, format);
}