/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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

#include "GUIDialogTextViewer.h"
#include "GUIUserMessages.h"
#include "filesystem/File.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "input/Action.h"
#include "input/ActionIDs.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "ServiceBroker.h"

using namespace XFILE;

#define CONTROL_HEADING  1
#define CONTROL_TEXTAREA 5

CGUIDialogTextViewer::CGUIDialogTextViewer(void)
    : CGUIDialog(WINDOW_DIALOG_TEXT_VIEWER, "DialogTextViewer.xml")
{
  m_loadType = KEEP_IN_MEMORY;
}

CGUIDialogTextViewer::~CGUIDialogTextViewer(void) = default;

bool CGUIDialogTextViewer::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_TOGGLE_FONT)
  {
    UseMonoFont(!m_mono);
    return true;
  }

  return CGUIDialog::OnAction(action);
}

bool CGUIDialogTextViewer::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
    {
      CGUIDialog::OnMessage(message);
      SetHeading();
      SetText();
      UseMonoFont(m_mono);
      return true;
    }
    break;
  case GUI_MSG_NOTIFY_ALL:
    {
      if (message.GetParam1() == GUI_MSG_UPDATE)
      {
        SetText();
        SetHeading();
        return true;
      }
    }
    break;
  default:
    break;
  }
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogTextViewer::SetText()
{
  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), CONTROL_TEXTAREA);
  msg.SetLabel(m_strText);
  OnMessage(msg);
}

void CGUIDialogTextViewer::SetHeading()
{
  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), CONTROL_HEADING);
  msg.SetLabel(m_strHeading);
  OnMessage(msg);
}

void CGUIDialogTextViewer::UseMonoFont(bool use)
{
  m_mono = use;
  CGUIMessage msg(GUI_MSG_SET_TYPE, GetID(), CONTROL_TEXTAREA, use ? 1 : 0);
  OnMessage(msg);
}

void CGUIDialogTextViewer::OnDeinitWindow(int nextWindowID)
{
  CGUIDialog::OnDeinitWindow(nextWindowID);

  // reset text area
  CGUIMessage msgReset(GUI_MSG_LABEL_RESET, GetID(), CONTROL_TEXTAREA);
  OnMessage(msgReset);

  // reset heading
  SET_CONTROL_LABEL(CONTROL_HEADING, "");
}

void CGUIDialogTextViewer::ShowForFile(const std::string& path, bool useMonoFont)
{
  CFile file;
  if (file.Open(path))
  {
    std::string data;
    try
    {
      data.resize(file.GetLength()+1);
      file.Read(&data[0], file.GetLength());
      CGUIDialogTextViewer* pDialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogTextViewer>(WINDOW_DIALOG_TEXT_VIEWER);
      pDialog->SetHeading(URIUtils::GetFileName(path));
      pDialog->SetText(data);
      pDialog->UseMonoFont(useMonoFont);
      pDialog->Open();
    }
    catch(const std::bad_alloc&)
    {
      CLog::Log(LOGERROR, "Not enough memory to load text file %s", path.c_str());
    }
    catch(...)
    {
      CLog::Log(LOGERROR, "Exception while trying to view text file %s", path.c_str());
    }
  }
}
