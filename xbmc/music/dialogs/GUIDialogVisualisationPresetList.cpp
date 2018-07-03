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
