/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GUIDialogVisualisationPresetList.h"
#include "GUIListContainer.h"
#include "GUISettings.h"
#include "FileItem.h"

#define CONTROL_LIST           2
#define CONTROL_PRESETS_LABEL  3
#define CONTROL_NONE_AVAILABLE 4

CGUIDialogVisualisationPresetList::CGUIDialogVisualisationPresetList(void)
    : CGUIDialog(WINDOW_DIALOG_VIS_PRESET_LIST, "VisualisationPresetList.xml")
{
  m_currentPreset = 0;
  LoadOnDemand(false);    // we are loaded by the vis window.
  m_vecPresets = new CFileItemList;
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
      if (message.GetSenderId() == CONTROL_LIST && message.GetParam1() == ACTION_SELECT_ITEM)
      {
        //clicked - ask for the preset to be changed to the new one
        CGUIListContainer *pList = (CGUIListContainer *)GetControl(CONTROL_LIST);
        if (pList)
        {
          int iItem = pList->GetSelectedItem();
          if (m_pVisualisation)
            m_pVisualisation->OnAction(CVisualisation::VIS_ACTION_LOAD_PRESET, (void *)&iItem);
        }
        return true;
      }
    }
    break;
  case GUI_MSG_WINDOW_INIT:
    {
      CGUIDialog::OnMessage(message);

      CGUIMessage msg(GUI_MSG_GET_VISUALISATION, 0, 0);
      g_graphicsContext.SendMessage(msg);
      SetVisualisation((CVisualisation *)msg.GetLPVOID());
      return true;
    }
    break;
  case GUI_MSG_WINDOW_DEINIT:
  case GUI_MSG_VISUALISATION_UNLOADING:
    {
      m_pVisualisation = NULL;
      CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_LIST);
      OnMessage(msg);
      m_vecPresets->Clear();
    }
    break;
  case GUI_MSG_VISUALISATION_LOADED:
    {
      if (message.GetLPVOID())
      {
        SetVisualisation((CVisualisation *)message.GetLPVOID());
      }
    }
  }
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogVisualisationPresetList::Render()
{
  char **presets = NULL;
  int numPresets = 0;
  int currentPreset = 0;
  bool locked = false;
  if (m_pVisualisation)
    m_pVisualisation->GetPresets(&presets, &currentPreset, &numPresets, &locked);
  if (currentPreset != m_currentPreset)
  { // current preset changed...
    m_vecPresets->Get(m_currentPreset)->Select(false);
    m_currentPreset = currentPreset;
    m_vecPresets->Get(m_currentPreset)->Select(true);
  }
  CGUIDialog::Render();
}

void CGUIDialogVisualisationPresetList::SetVisualisation(CVisualisation *pVisualisation)
{
  m_pVisualisation = pVisualisation;
  m_vecPresets->Clear();
  if (m_pVisualisation)
  {
    //clear filelist
    CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_LIST);
    OnMessage(msg);
    char **presets = NULL;
    int numPresets = 0;
    m_currentPreset = 0;
    bool locked = false;
    m_pVisualisation->GetPresets(&presets, &m_currentPreset, &numPresets, &locked);
    if (presets)
    {
      //clear filelist
      CGUIMessage msg2(GUI_MSG_LABEL_RESET, GetID(), CONTROL_LIST);
      OnMessage(msg2);
      m_vecPresets->Clear();
      for (int i = 0; i < numPresets; i++)
      {
        CFileItemPtr pItem(new CFileItem(presets[i]));
        if (i == m_currentPreset)
          pItem->Select(true);
        pItem->RemoveExtension();
        pItem->SetLabel2(" ");
        m_vecPresets->Add(pItem);
        CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_LIST, 0, 0, pItem);
        OnMessage(msg);
      }
    }
  }
  // update our settings label
  CStdString strVis = g_guiSettings.GetString("mymusic.visualisation");
  if (strVis != "None" && strVis.size() > 4)
  { // make it look pretty
    strVis = strVis.Left(strVis.size() - 4);
    strVis[0] = toupper(strVis[0]);
  }
  CStdString strSettings;
  strSettings.Format(g_localizeStrings.Get(13407).c_str(), strVis.c_str());
  SET_CONTROL_LABEL(CONTROL_PRESETS_LABEL, strSettings);
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

