/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogTextViewer.h"

#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "filesystem/File.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

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
