/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
#include "addons/Visualisation.h"
#include "guilib/GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "FileItem.h"
#include "input/Key.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"

#define CONTROL_HEADER_LABEL   2
#define CONTROL_NONE_AVAILABLE 4
#define CONTROL_LIST           5

using ADDON::CVisualisation;

CGUIDialogVisualisationPresetList::CGUIDialogVisualisationPresetList(void)
    : CGUIDialog(WINDOW_DIALOG_VIS_PRESET_LIST, "VisualisationPresetList.xml")
{
  m_currentPreset = 0;
  m_vecPresets = new CFileItemList;
  m_viz = NULL;
  m_loadType = KEEP_IN_MEMORY;
}

CGUIDialogVisualisationPresetList::~CGUIDialogVisualisationPresetList(void)
{
  delete m_vecPresets;
}

bool CGUIDialogVisualisationPresetList::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_CLICKED:
    {
      if (message.GetSenderId() == CONTROL_LIST && (message.GetParam1() == ACTION_SELECT_ITEM ||
                                                    message.GetParam1() == ACTION_MOUSE_LEFT_CLICK))
      {
        //clicked - ask for the preset to be changed to the new one
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_LIST);
        if (OnMessage(msg))
        {
          int iItem = (int)msg.GetParam1();
          if (m_viz)
            m_viz->OnAction(VIS_ACTION_LOAD_PRESET, (void *)&iItem);
        }
        return true;
      }
    }
    break;
  case GUI_MSG_VISUALISATION_UNLOADING:
    {
      m_viz = NULL;
      Update();
    }
    break;
  case GUI_MSG_VISUALISATION_LOADED:
    {
      SetVisualisation((CVisualisation*)message.GetPointer());
    }
    break;
  }
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogVisualisationPresetList::SetVisualisation(CVisualisation* vis)
{
  m_viz = NULL;
  if (vis)
  {
    m_viz = vis;
    Update();
  }
}

void CGUIDialogVisualisationPresetList::FrameMove()
{
  //FIXME we shouldn't have to check preset each frame
  // a viz callback could push GUI_MSG_VISUALISATION_UPDATED
  if (m_viz)
  {
    unsigned preset = m_viz->GetPreset();
    if (preset != m_currentPreset && preset < (unsigned int)m_vecPresets->Size())
    {
      m_vecPresets->Get(m_currentPreset)->Select(false);
      m_currentPreset = preset;
      m_vecPresets->Get(m_currentPreset)->Select(true);
    }
  }
  CGUIDialog::FrameMove();
}

void CGUIDialogVisualisationPresetList::OnInitWindow()
{
  CGUIMessage msg(GUI_MSG_GET_VISUALISATION, 0, 0);
  g_windowManager.SendMessage(msg);
  SetVisualisation((CVisualisation*)msg.GetPointer());
  CGUIDialog::OnInitWindow();
}

void CGUIDialogVisualisationPresetList::OnDeinitWindow(int nextWindowID)
{
  CGUIDialog::OnDeinitWindow(nextWindowID);
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_LIST);
  OnMessage(msg);
  SET_CONTROL_LABEL(CONTROL_HEADER_LABEL, "");
  m_vecPresets->Clear();
}

void CGUIDialogVisualisationPresetList::Update()
{
  m_vecPresets->Clear();
  std::string strHeading;
  if (m_viz)
  {
    strHeading = StringUtils::Format(g_localizeStrings.Get(13407).c_str(), m_viz->Name().c_str());

    //clear filelist
    CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_LIST);
    OnMessage(msg);
    std::vector<std::string> presets;
    if (m_viz->GetPresetList(presets))
    {
      m_currentPreset = m_viz->GetPreset();
      for (unsigned i = 0; i < presets.size(); i++)
      {
        CFileItemPtr pItem(new CFileItem(presets[i]));
        if (i == m_currentPreset)
          pItem->Select(true);
        pItem->RemoveExtension();
        pItem->SetLabel2(" ");
        m_vecPresets->Add(pItem);
      }
      CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LIST, m_currentPreset, 0, m_vecPresets);
      OnMessage(msg);
    }
  }

  // update our dialog's label
  SET_CONTROL_LABEL(CONTROL_HEADER_LABEL, strHeading);

  // if there is no presets, add a label saying so
  if (m_vecPresets->Size() == 0)
  {
    SET_CONTROL_VISIBLE(CONTROL_NONE_AVAILABLE);
  }
  else
  {
    SET_CONTROL_HIDDEN(CONTROL_NONE_AVAILABLE);
    CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_LIST, m_currentPreset);
    OnMessage(msg);
  }
}

