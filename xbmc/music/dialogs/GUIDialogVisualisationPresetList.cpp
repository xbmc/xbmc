/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogVisualisationPresetList.h"

#include "FileItem.h"
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIVisualisationControl.h"
#include "guilib/GUIWindowManager.h"
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
    ClearVisualisation();
    break;
  }
  return CGUIDialogSelect::OnMessage(message);
}

void CGUIDialogVisualisationPresetList::OnSelect(int idx)
{
  if (m_viz)
    m_viz->SetPreset(idx);
}

void CGUIDialogVisualisationPresetList::ClearVisualisation()
{
  m_viz = nullptr;
  Reset();
}

void CGUIDialogVisualisationPresetList::SetVisualisation(CGUIVisualisationControl* vis)
{
  m_viz = vis;
  Reset();
  if (!m_viz)
  { // No viz, but show something if this dialog activated
    SetHeading(CVariant{ 10122 });
    CFileItem item(g_localizeStrings.Get(13389));
    Add(item);
  }
  else
  {
    SetUseDetails(false);
    SetMultiSelection(false);
    SetHeading(CVariant{StringUtils::Format(g_localizeStrings.Get(13407), m_viz->Name())});
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
    else
    { // Viz does not have any presets
      // "There are no presets available for this visualisation"
      CFileItem item(g_localizeStrings.Get(13389));
      Add(item);
    }
  }
}

void CGUIDialogVisualisationPresetList::OnInitWindow()
{
  CGUIMessage msg(GUI_MSG_GET_VISUALISATION, 0, 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
  SetVisualisation(static_cast<CGUIVisualisationControl*>(msg.GetPointer()));
  CGUIDialogSelect::OnInitWindow();
}

void CGUIDialogVisualisationPresetList::OnDeinitWindow(int nextWindowID)
{
  ClearVisualisation();
  CGUIDialogSelect::OnDeinitWindow(nextWindowID);
}
