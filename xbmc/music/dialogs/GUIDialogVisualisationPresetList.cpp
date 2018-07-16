/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogVisualisationPresetList.h"
#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/GUIVisualisationControl.h"
#include "GUIUserMessages.h"
#include "FileItem.h"
#include "input/Key.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

CGUIDialogVisualisationPresetList::CGUIDialogVisualisationPresetList()
    : CGUIDialogSelect(WINDOW_DIALOG_VIS_PRESET_LIST)
{
  m_loadType = KEEP_IN_MEMORY;
}

bool CGUIDialogVisualisationPresetList::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_VISUALISATION_UNLOADING:
    SetVisualisation(nullptr);
    break;
  }
  return CGUIDialogSelect::OnMessage(message);
}

void CGUIDialogVisualisationPresetList::OnSelect(int idx)
{
  if (m_viz)
    m_viz->SetPreset(idx);
}

void CGUIDialogVisualisationPresetList::SetVisualisation(CGUIVisualisationControl* vis)
{
  m_viz = vis;
  Reset();
  if (m_viz)
  {
    SetUseDetails(false);
    SetMultiSelection(false);
    SetHeading(CVariant{StringUtils::Format(g_localizeStrings.Get(13407).c_str(), m_viz->Name().c_str())});
    std::vector<std::string> presets;
    if (m_viz->GetPresetList(presets))
    {
      for (const auto& preset : presets)
      {
        CFileItem item(preset);
        item.RemoveExtension();
        Add(item);
      }
      SetSelected(m_viz->GetActivePreset());
    }
  }
}

void CGUIDialogVisualisationPresetList::OnInitWindow()
{
  CGUIMessage msg(GUI_MSG_GET_VISUALISATION, 0, 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
  if (msg.GetPointer())
    SetVisualisation(static_cast<CGUIVisualisationControl*>(msg.GetPointer()));
  CGUIDialogSelect::OnInitWindow();
}

void CGUIDialogVisualisationPresetList::OnDeinitWindow(int nextWindowID)
{
  SetVisualisation(nullptr);
  CGUIDialogSelect::OnDeinitWindow(nextWindowID);
}
