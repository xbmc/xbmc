#include "stdafx.h"
#include "GUIDialogVisualisationPresetList.h"
#include "GUIListControl.h"

#define CONTROL_LIST           2
#define CONTROL_PRESETS_LABEL  3
#define CONTROL_NONE_AVAILABLE 4

CGUIDialogVisualisationPresetList::CGUIDialogVisualisationPresetList(void)
    : CGUIDialog(0)
{
  m_currentPreset = 0;
}

CGUIDialogVisualisationPresetList::~CGUIDialogVisualisationPresetList(void)
{
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
        CGUIListControl *pList = (CGUIListControl *)GetControl(CONTROL_LIST);
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
      CGUIMessage msg(GUI_MSG_GET_VISUALISATION, 0, 0);
      g_graphicsContext.SendMessage(msg);

      SetVisualisation((CVisualisation *)msg.GetLPVOID());

      CGUIDialog::OnMessage(message);
      return true;
    }
    break;
  case GUI_MSG_WINDOW_DEINIT:
  case GUI_MSG_VISUALISATION_UNLOADING:
    {
      m_pVisualisation = NULL;
      CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_LIST);
      OnMessage(msg);
      m_vecPresets.Clear();
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
    m_vecPresets[m_currentPreset]->Select(false);
    m_currentPreset = currentPreset;
    m_vecPresets[m_currentPreset]->Select(true);
  }
  CGUIDialog::Render();
}

void CGUIDialogVisualisationPresetList::SetVisualisation(CVisualisation *pVisualisation)
{
  m_pVisualisation = pVisualisation;
  m_vecPresets.Clear();
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
      m_vecPresets.Clear();
      for (int i = 0; i < numPresets; i++)
      {
        CFileItem *pItem = new CFileItem(presets[i]);
        if (i == m_currentPreset)
          pItem->Select(true);
        pItem->RemoveExtension();
        pItem->SetLabel2(" ");
        m_vecPresets.Add(pItem);
        CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_LIST, 0, 0, pItem);
        OnMessage(msg);
      }
    }
  }
  // update our settings label
  CStdString strVis = g_guiSettings.GetString("MyMusic.Visualisation");
  if (strVis != "None" && strVis.size() > 4)
  { // make it look pretty
    strVis = strVis.Left(strVis.size() - 4);
    strVis[0] = toupper(strVis[0]);
  }
  CStdStringW strSettings;
  strSettings.Format(L"%S %s", strVis.c_str(), L" Presets");
  SET_CONTROL_LABEL(CONTROL_PRESETS_LABEL, strSettings);
  // if there is no presets, add a label saying so
  if (m_vecPresets.Size() == 0)
  {
    SET_CONTROL_VISIBLE(CONTROL_NONE_AVAILABLE);
  }
  else
  {
    SET_CONTROL_HIDDEN(CONTROL_NONE_AVAILABLE);
  }
}
