/*
 *      Copyright (C) 2012-2015 Team KODI
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIDialogAudioDSPManager.h"

#include "FileItem.h"
#include "cores/AudioEngine/Engines/ActiveAE/AudioDSPAddons/ActiveAEDSP.h"
#include "dialogs/GUIDialogTextViewer.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogBusy.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/GUIListContainer.h"
#include "guilib/GUIRadioButtonControl.h"
#include "input/Key.h"
#include "utils/StringUtils.h"

#define CONTROL_LIST_AVAILABLE                  20
#define CONTROL_LIST_ACTIVE                     21
#define CONTROL_RADIO_BUTTON_CONTINOUS_SAVING   22
#define CONTROL_BUTTON_APPLY_CHANGES            23
#define CONTROL_BUTTON_CLEAR_ACTIVE_MODES       24
#define CONTROL_LIST_MODE_SELECTION             9000

#define LIST_AVAILABLE                          0
#define LIST_ACTIVE                             1

#define LIST_INPUT_RESAMPLE                     0
#define LIST_PRE_PROCESS                        1
#define LIST_MASTER_PROCESS                     2
#define LIST_POST_PROCESS                       3
#define LIST_OUTPUT_RESAMPLE                    4

using namespace ActiveAE;

typedef struct
{
  const char* sModeType;
  int iModeType;
  int iName;
  int iDescription;
} DSP_MODE_TYPES;

static const DSP_MODE_TYPES dsp_mode_types[] = {
  { "inputresampling",  AE_DSP_MODE_TYPE_INPUT_RESAMPLE,  15057, 15114 },
  { "preprocessing",    AE_DSP_MODE_TYPE_PRE_PROCESS,     15058, 15113 },
  { "masterprocessing", AE_DSP_MODE_TYPE_MASTER_PROCESS,  15059, 15115 },
  { "postprocessing",   AE_DSP_MODE_TYPE_POST_PROCESS,    15060, 15117 },
  { "outputresampling", AE_DSP_MODE_TYPE_OUTPUT_RESAMPLE, 15061, 15116 },
  { "undefined",        AE_DSP_MODE_TYPE_UNDEFINED,       0,     0 }
};

CGUIDialogAudioDSPManager::CGUIDialogAudioDSPManager(void)
 : CGUIDialog(WINDOW_DIALOG_AUDIO_DSP_MANAGER, "DialogAudioDSPManager.xml")
{
  m_bMovingMode               = false;
  m_bContainsChanges          = false;
  m_bContinousSaving          = true;
  m_iSelected[LIST_AVAILABLE] = 0;
  m_iSelected[LIST_ACTIVE]    = 0;

  for (int ii = 0; ii < AE_DSP_MODE_TYPE_MAX; ii++)
  {
    m_activeItems[ii] = new CFileItemList;
    m_availableItems[ii] = new CFileItemList;
  }

  m_iCurrentType = AE_DSP_MODE_TYPE_MASTER_PROCESS;
}

CGUIDialogAudioDSPManager::~CGUIDialogAudioDSPManager(void)
{
  for (int ii = 0; ii < AE_DSP_MODE_TYPE_MAX; ii++)
  {
    delete m_activeItems[ii];
    delete m_availableItems[ii];
  }
}

bool CGUIDialogAudioDSPManager::OnActionMove(const CAction &action)
{
  bool bReturn(false);
  int iActionId = action.GetID();

  if (GetFocusedControlID() == CONTROL_LIST_ACTIVE)
  {
    if (iActionId == ACTION_MOUSE_MOVE)
    {
      int iSelected = m_activeViewControl.GetSelectedItem();
      if (m_iSelected[LIST_ACTIVE] < iSelected)
      {
        iActionId = ACTION_MOVE_DOWN;
      }
      else if (m_iSelected[LIST_ACTIVE] > iSelected)
      {
        iActionId = ACTION_MOVE_UP;
      }
      else
      {
        return bReturn;
      }
    }

    if (iActionId == ACTION_MOVE_DOWN || iActionId == ACTION_MOVE_UP ||
        iActionId == ACTION_PAGE_DOWN || iActionId == ACTION_PAGE_UP)
    {
      bReturn = true;
      CGUIDialog::OnAction(action);

      int iSelected = m_activeViewControl.GetSelectedItem();
      if (!m_bMovingMode)
      {
        if (iSelected != m_iSelected[LIST_ACTIVE])
        {
          m_iSelected[LIST_ACTIVE] = iSelected;
        }
      }
      else
      {
        bool bMoveUp        = iActionId == ACTION_PAGE_UP || iActionId == ACTION_MOVE_UP;
        unsigned int iLines = bMoveUp ? abs(m_iSelected[LIST_ACTIVE] - iSelected) : 1;
        bool bOutOfBounds   = bMoveUp ? m_iSelected[LIST_ACTIVE] <= 0  : m_iSelected[LIST_ACTIVE] >= m_activeItems[m_iCurrentType]->Size() - 1;
        if (bOutOfBounds)
        {
          bMoveUp = !bMoveUp;
          iLines  = m_activeItems[m_iCurrentType]->Size() - 1;
        }

        std::string strNumber;
        for (unsigned int iLine = 0; iLine < iLines; iLine++)
        {
          unsigned int iNewSelect = bMoveUp ? m_iSelected[LIST_ACTIVE] - 1 : m_iSelected[LIST_ACTIVE] + 1;
          if (m_activeItems[m_iCurrentType]->Get(iNewSelect)->GetProperty("Number").asString() != "-")
          {
            strNumber = StringUtils::Format("%i", m_iSelected[LIST_ACTIVE]+1);
            m_activeItems[m_iCurrentType]->Get(iNewSelect)->SetProperty("Number", strNumber);
            strNumber = StringUtils::Format("%i", iNewSelect+1);
            m_activeItems[m_iCurrentType]->Get(m_iSelected[LIST_ACTIVE])->SetProperty("Number", strNumber);
          }
          m_activeItems[m_iCurrentType]->Swap(iNewSelect, m_iSelected[LIST_ACTIVE]);
          m_iSelected[LIST_ACTIVE] = iNewSelect;
        }

        SET_CONTROL_FOCUS(CONTROL_LIST_ACTIVE, 0);

        m_activeViewControl.SetItems(*m_activeItems[m_iCurrentType]);
        m_activeViewControl.SetSelectedItem(m_iSelected[LIST_ACTIVE]);
      }
    }
  }

  return bReturn;
}

bool CGUIDialogAudioDSPManager::OnAction(const CAction& action)
{
  return OnActionMove(action) ||
         CGUIDialog::OnAction(action);
}

void CGUIDialogAudioDSPManager::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  m_iSelected[LIST_AVAILABLE] = 0;
  m_iSelected[LIST_ACTIVE]    = 0;
  m_bMovingMode               = false;
  m_bContainsChanges          = false;

  CGUIRadioButtonControl *radioButton = dynamic_cast<CGUIRadioButtonControl*>(GetControl(CONTROL_RADIO_BUTTON_CONTINOUS_SAVING));
  CGUIButtonControl *applyButton = dynamic_cast<CGUIButtonControl*>(GetControl(CONTROL_BUTTON_APPLY_CHANGES));
  if (!radioButton || !applyButton)
  {
    helper_LogError(__FUNCTION__);
    return;
  }

  SET_CONTROL_SELECTED(GetID(), CONTROL_RADIO_BUTTON_CONTINOUS_SAVING, m_bContinousSaving);
  applyButton->SetEnabled(!m_bContinousSaving);

  Update();
  SetSelectedModeType();
}

void CGUIDialogAudioDSPManager::OnDeinitWindow(int nextWindowID)
{
  if (m_bContainsChanges)
  {
    if (m_bContinousSaving)
    {
      SaveList();
    }
    else
    {
      if (CGUIDialogYesNo::ShowAndGetInput(g_localizeStrings.Get(19098), g_localizeStrings.Get(15079)))
      {
        SaveList();
      }
      else
      {
        m_bContinousSaving = false;
      }
    }
  }

  Clear();

  CGUIDialog::OnDeinitWindow(nextWindowID);
}

bool CGUIDialogAudioDSPManager::OnClickListAvailable(CGUIMessage &message)
{
  int iAction = message.GetParam1();
  int iItem = m_availableViewControl.GetSelectedItem();

  /* Check file item is in list range and get his pointer */
  if (iItem < 0 || iItem >= (int)m_availableItems[m_iCurrentType]->Size()) return true;

  /* Process actions */
  if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_CONTEXT_MENU || iAction == ACTION_MOUSE_LEFT_CLICK || iAction == ACTION_MOUSE_RIGHT_CLICK)
  {
    /* Show Contextmenu */
    OnPopupMenu(iItem, LIST_AVAILABLE);

    return true;
  }

  return false;
}

bool CGUIDialogAudioDSPManager::OnClickListActive(CGUIMessage &message)
{
  if (!m_bMovingMode)
  {
    int iAction = message.GetParam1();
    int iItem = m_activeViewControl.GetSelectedItem();

    /* Check file item is in list range and get his pointer */
    if (iItem < 0 || iItem >= (int)m_activeItems[m_iCurrentType]->Size()) return true;

    /* Process actions */
    if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_CONTEXT_MENU || iAction == ACTION_MOUSE_LEFT_CLICK || iAction == ACTION_MOUSE_RIGHT_CLICK)
    {
      /* Show Contextmenu */
      OnPopupMenu(iItem, LIST_ACTIVE);

      return true;
    }
  }
  else
  {
    CFileItemPtr pItem = m_activeItems[m_iCurrentType]->Get(m_iSelected[LIST_ACTIVE]);
    if (pItem)
    {
      pItem->Select(false);
      pItem->SetProperty("Changed", true);
      m_bMovingMode = false;
      m_bContainsChanges = true;

      if (m_bContinousSaving)
      {
        SaveList();
      }

      CGUIListContainer *modeList = dynamic_cast<CGUIListContainer*>(GetControl(CONTROL_LIST_MODE_SELECTION));
      CGUIButtonControl *applyButton = dynamic_cast<CGUIButtonControl*>(GetControl(CONTROL_BUTTON_APPLY_CHANGES));
      CGUIButtonControl *clearActiveModesButton = dynamic_cast<CGUIButtonControl*>(GetControl(CONTROL_BUTTON_CLEAR_ACTIVE_MODES));
      if (!modeList || !applyButton || !clearActiveModesButton)
      {
        helper_LogError(__FUNCTION__);
        return false;
      }

      // reenable all buttons and mode selection list
      modeList->SetEnabled(true);
      clearActiveModesButton->SetEnabled(true);
      if (!m_bContinousSaving)
      {
        applyButton->SetEnabled(true);
      }

      return true;
    }
  }

  return false;
}

bool CGUIDialogAudioDSPManager::OnClickRadioContinousSaving(CGUIMessage &message)
{
  CGUIRadioButtonControl *radioButton = dynamic_cast<CGUIRadioButtonControl*>(GetControl(CONTROL_RADIO_BUTTON_CONTINOUS_SAVING));
  CGUIButtonControl *applyChangesButton = dynamic_cast<CGUIButtonControl*>(GetControl(CONTROL_BUTTON_APPLY_CHANGES));

  if (!radioButton || !applyChangesButton)
  {
    helper_LogError(__FUNCTION__);
    return false;
  }

  if (!radioButton->IsSelected())
  {
    applyChangesButton->SetEnabled(true);
    m_bContinousSaving = false;
  }
  else
  {
    m_bContinousSaving = true;
    applyChangesButton->SetEnabled(false);
  }

  return true;
}

bool CGUIDialogAudioDSPManager::OnClickApplyChanges(CGUIMessage &message)
{
  SaveList();
  return true;
}

bool CGUIDialogAudioDSPManager::OnClickClearActiveModes(CGUIMessage &message)
{
  if (m_activeItems[m_iCurrentType]->Size() > 0)
  {
    for (int iItem = 0; iItem < m_activeItems[m_iCurrentType]->Size(); iItem++)
    {
      CFileItemPtr pItem = m_activeItems[m_iCurrentType]->Get(iItem);
      if (pItem)
      {
        // remove mode from active mode list and add it to available mode list
        CFileItemPtr newItem(dynamic_cast<CFileItem*>(pItem->Clone()));
        newItem->SetProperty("ActiveMode", false);
        newItem->SetProperty("Changed", true);
        m_availableItems[m_iCurrentType]->Add(newItem);
      }
    }
    m_activeItems[m_iCurrentType]->Clear();

    // reorder available mode list, so that the mode order is always consistent
    m_availableItems[m_iCurrentType]->ClearSortState();
    m_availableItems[m_iCurrentType]->Sort(SortByLabel, SortOrderAscending);

    // update active and available mode list
    m_availableViewControl.SetItems(*m_availableItems[m_iCurrentType]);
    m_activeViewControl.SetItems(*m_activeItems[m_iCurrentType]);

    m_bContainsChanges = true;
    if (m_bContinousSaving)
    {
      SaveList();
    }
  }

  return true;
}

bool CGUIDialogAudioDSPManager::OnMessageClick(CGUIMessage &message)
{
  int iControl = message.GetSenderId();
  switch(iControl)
  {
  case CONTROL_LIST_AVAILABLE:
    return OnClickListAvailable(message);
  case CONTROL_LIST_ACTIVE:
    return OnClickListActive(message);
  case CONTROL_RADIO_BUTTON_CONTINOUS_SAVING:
    return OnClickRadioContinousSaving(message);
  case CONTROL_BUTTON_CLEAR_ACTIVE_MODES:
    return OnClickClearActiveModes(message);
  case CONTROL_BUTTON_APPLY_CHANGES:
    return OnClickApplyChanges(message);
  default:
    return false;
  }
}

bool CGUIDialogAudioDSPManager::OnMessage(CGUIMessage& message)
{
  unsigned int iMessage = message.GetMessage();

  switch (iMessage)
  {
    case GUI_MSG_CLICKED:
      return OnMessageClick(message);

    case GUI_MSG_ITEM_SELECT:
    {
      int focusedControl = GetFocusedControlID();
      if (focusedControl == CONTROL_LIST_MODE_SELECTION)
      {
        CGUIListContainer *modeListPtr = dynamic_cast<CGUIListContainer*>(GetControl(CONTROL_LIST_MODE_SELECTION));

        if (modeListPtr)
        {
          CGUIListItemPtr modeListItem = modeListPtr->GetListItem(0); // get current selected list item
          if (modeListItem)
          {
            std::string currentModeString = modeListItem->GetProperty("currentMode").asString();
            int newModeType = helper_TranslateModeType(currentModeString);

            if (m_iCurrentType != newModeType)
            {
              m_iCurrentType = newModeType;
              SetSelectedModeType();
            }
          }
        }
      }
    }
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogAudioDSPManager::OnWindowLoaded(void)
{
  g_graphicsContext.Lock();

  m_availableViewControl.Reset();
  m_availableViewControl.SetParentWindow(GetID());
  m_availableViewControl.AddView(GetControl(CONTROL_LIST_AVAILABLE));

  m_activeViewControl.Reset();
  m_activeViewControl.SetParentWindow(GetID());
  m_activeViewControl.AddView(GetControl(CONTROL_LIST_ACTIVE));

  g_graphicsContext.Unlock();

  CGUIDialog::OnWindowLoaded();
}

void CGUIDialogAudioDSPManager::OnWindowUnload(void)
{
  CGUIDialog::OnWindowUnload();

  m_availableViewControl.Reset();
  m_activeViewControl.Reset();
}

bool CGUIDialogAudioDSPManager::OnPopupMenu(int iItem, int listType)
{
  // popup the context menu
  // grab our context menu
  CContextButtons buttons;
  int listSize = 0;
  CFileItemPtr pItem;

  if (listType == LIST_ACTIVE)
  {
    listSize = m_activeItems[m_iCurrentType]->Size();
    pItem = m_activeItems[m_iCurrentType]->Get(iItem);
  }
  else if (listType == LIST_AVAILABLE)
  {
    listSize = m_availableItems[m_iCurrentType]->Size();
    pItem = m_availableItems[m_iCurrentType]->Get(iItem);
  }

  if (!pItem)
  {
    return false;
  }

  // mark the item
  if (iItem >= 0 && iItem < listSize)
  {
    pItem->Select(true);
  }
  else
  {
    return false;
  }

  if (listType == LIST_ACTIVE)
  {
    if (listSize > 1)
    {
      buttons.Add(CONTEXT_BUTTON_MOVE, 116);              /* Move mode up or down */
    }
    buttons.Add(CONTEXT_BUTTON_ACTIVATE, 24021);          /* Used to deactivate addon process type */
  }
  else if (listType == LIST_AVAILABLE)
  {
    if (m_activeItems[m_iCurrentType]->Size() > 0 && (m_iCurrentType == AE_DSP_MODE_TYPE_INPUT_RESAMPLE || m_iCurrentType == AE_DSP_MODE_TYPE_OUTPUT_RESAMPLE))
    {
      buttons.Add(CONTEXT_BUTTON_ACTIVATE, 15080);        /* Used to swap addon resampling process type */
    }
    else
    {
      buttons.Add(CONTEXT_BUTTON_ACTIVATE, 24022);        /* Used to activate addon process type */
    }
  }

  if (pItem->GetProperty("SettingsDialog").asInteger() != 0)
  {
    buttons.Add(CONTEXT_BUTTON_SETTINGS, 15078);          /* Used to activate addon process type help description*/
  }

  if (pItem->GetProperty("Help").asInteger() > 0)
  {
    buttons.Add(CONTEXT_BUTTON_HELP, 15062);              /* Used to activate addon process type help description*/
  }

  int choice = CGUIDialogContextMenu::ShowAndGetChoice(buttons);

  // deselect our item
  if (iItem >= 0 && iItem < listSize)
  {
    pItem->Select(false);
  }

  if (choice < 0)
  {
    return false;
  }

  return OnContextButton(iItem, (CONTEXT_BUTTON)choice, listType);
}

bool CGUIDialogAudioDSPManager::OnContextButton(int itemNumber, CONTEXT_BUTTON button, int listType)
{
  CFileItemPtr pItem;
  int listSize = 0;
  if (listType == LIST_ACTIVE)
  {
    pItem = m_activeItems[m_iCurrentType]->Get(itemNumber);
    listSize = m_activeItems[m_iCurrentType]->Size();
  }
  else if (listType == LIST_AVAILABLE)
  {
    pItem = m_availableItems[m_iCurrentType]->Get(itemNumber);
    listSize = m_availableItems[m_iCurrentType]->Size();
  }

  /* Check file item is in list range and get his pointer */
  if (!pItem || itemNumber < 0 || itemNumber >= listSize)
  {
    return false;
  }

  if (button == CONTEXT_BUTTON_HELP)
  {
    /*!
    * Open audio dsp addon mode help text dialog
    */
    AE_DSP_ADDON addon;
    if (CServiceBroker::GetADSP().GetAudioDSPAddon((int)pItem->GetProperty("AddonId").asInteger(), addon))
    {
      CGUIDialogTextViewer* pDlgInfo = (CGUIDialogTextViewer*)g_windowManager.GetWindow(WINDOW_DIALOG_TEXT_VIEWER);
      pDlgInfo->SetHeading(g_localizeStrings.Get(15062) + " - " + pItem->GetProperty("Name").asString());
      pDlgInfo->SetText(g_localizeStrings.GetAddonString(addon->ID(), (uint32_t)pItem->GetProperty("Help").asInteger()));
      pDlgInfo->Open();
    }
  }
  else if (button == CONTEXT_BUTTON_ACTIVATE)
  {
    /*!
    * Deactivate selected processing mode
    */
    if (pItem->GetProperty("ActiveMode").asBoolean())
    {
      // remove mode from active mode list and add it to available mode list
      CFileItemPtr newItem(dynamic_cast<CFileItem*>(pItem->Clone()));
      newItem->SetProperty("ActiveMode", false);
      newItem->SetProperty("Changed", true);
      m_activeItems[m_iCurrentType]->Remove(itemNumber);
      m_availableItems[m_iCurrentType]->Add(newItem);
    }
    else
    {
      /*!
      * Activate selected processing mode
      */
      if ((m_iCurrentType == AE_DSP_MODE_TYPE_INPUT_RESAMPLE || m_iCurrentType == AE_DSP_MODE_TYPE_OUTPUT_RESAMPLE) && m_activeItems[m_iCurrentType]->Size() > 0)
      { // if there is already an active resampler, now we remove it
        CFileItemPtr activeResampler = m_activeItems[m_iCurrentType]->Get(0);
        if (activeResampler)
        {
          CFileItemPtr newItem(dynamic_cast<CFileItem*>(activeResampler->Clone()));
          newItem->SetProperty("ActiveMode", false);
          newItem->SetProperty("Changed", true);

          m_availableItems[m_iCurrentType]->Add(newItem);
          // clear active list, because only one active resampling mode is supported by ActiveAEDSP
          m_activeItems[m_iCurrentType]->Clear();
        }
      }

      // remove mode from available mode list and add it to active mode list
      CFileItemPtr newItem(dynamic_cast<CFileItem*>(pItem->Clone()));

      newItem->SetProperty("Number", (int)m_activeItems[m_iCurrentType]->Size() +1);
      newItem->SetProperty("Changed", true);
      newItem->SetProperty("ActiveMode", true);

      m_availableItems[m_iCurrentType]->Remove(itemNumber);
      m_activeItems[m_iCurrentType]->Add(newItem);
    }

    m_bContainsChanges = true;
    if (m_bContinousSaving)
    {
      SaveList();
    }

    // reorder available mode list, so that the mode order is always consistent
    m_availableItems[m_iCurrentType]->ClearSortState();
    m_availableItems[m_iCurrentType]->Sort(SortByLabel, SortOrderAscending);

    // update active and available mode list
    Renumber();
    m_availableViewControl.SetItems(*m_availableItems[m_iCurrentType]);
    m_activeViewControl.SetItems(*m_activeItems[m_iCurrentType]);
  }
  else if (button == CONTEXT_BUTTON_MOVE)
  {
    m_bMovingMode = true;
    pItem->Select(true);

    CGUIListContainer *modeList = dynamic_cast<CGUIListContainer*>(GetControl(CONTROL_LIST_MODE_SELECTION));
    CGUIButtonControl *applyButton = dynamic_cast<CGUIButtonControl*>(GetControl(CONTROL_BUTTON_APPLY_CHANGES));
    CGUIButtonControl *clearActiveModesButton = dynamic_cast<CGUIButtonControl*>(GetControl(CONTROL_BUTTON_CLEAR_ACTIVE_MODES));
    if (!modeList || !applyButton || !clearActiveModesButton)
    {
      helper_LogError(__FUNCTION__);
      return false;
    }

    // if we are in MovingMode all buttons and mode selection list will be disabled!
    modeList->SetEnabled(false);
    clearActiveModesButton->SetEnabled(false);
    if (!m_bContinousSaving)
    {
      applyButton->SetEnabled(false);
    }
  }
  else if (button == CONTEXT_BUTTON_SETTINGS)
  {
    int hookId = (int)pItem->GetProperty("SettingsDialog").asInteger();
    if (hookId > 0)
    {
      AE_DSP_ADDON addon;
      if (CServiceBroker::GetADSP().GetAudioDSPAddon((int)pItem->GetProperty("AddonId").asInteger(), addon))
      {
        AE_DSP_MENUHOOK       hook;
        AE_DSP_MENUHOOK_DATA  hookData;

        hook.category           = AE_DSP_MENUHOOK_ALL;
        hook.iHookId            = hookId;
        hook.iRelevantModeId    = (unsigned int)pItem->GetProperty("AddonModeNumber").asInteger();
        hookData.category       = AE_DSP_MENUHOOK_ALL;
        hookData.data.iStreamId = -1;

        /*!
         * @note the addon dialog becomes always opened on the back of Kodi ones for this reason a
         * "<animation effect="fade" start="100" end="0" time="400" condition="Window.IsVisible(Addon)">Conditional</animation>"
         * on skin is needed to hide dialog.
         */
        addon->CallMenuHook(hook, hookData);
      }
    }
    else
    {
      CGUIDialogOK::ShowAndGetInput(19033, 0, 15040, 0);
    }
  }

  return true;
}

void CGUIDialogAudioDSPManager::Update()
{
  CGUIDialogBusy* pDlgBusy = (CGUIDialogBusy*)g_windowManager.GetWindow(WINDOW_DIALOG_BUSY);
  if (!pDlgBusy)
  {
    helper_LogError(__FUNCTION__);
    return;
  }
  pDlgBusy->Open();

  Clear();

  AE_DSP_MODELIST modes;
  CActiveAEDSPDatabase db;
  if (!db.Open())
  {
    pDlgBusy->Close();
    CLog::Log(LOGERROR, "DSP Manager - %s - Could not open DSP database for update!", __FUNCTION__);
    return;
  }

  // construct a CFileItemList to pass 'em on to the list
  CFileItemList items;
  for (int i = 0; i < AE_DSP_MODE_TYPE_MAX; ++i)
  {
    int iModeType = dsp_mode_types[i].iModeType;

    modes.clear();
    db.GetModes(modes, iModeType);

    // No modes available, nothing to do.
    if (!modes.empty())
    {
      CFileItemPtr item(new CFileItem());
      item->SetLabel(g_localizeStrings.Get(dsp_mode_types[i].iName));
      item->SetLabel2(g_localizeStrings.Get(dsp_mode_types[i].iDescription));
      item->SetProperty("currentMode", dsp_mode_types[i].sModeType);
      items.Add(item);

      AE_DSP_MENUHOOK_CAT menuHook = helper_GetMenuHookCategory(iModeType);
      int continuesNo = 1;
      for (unsigned int iModePtr = 0; iModePtr < modes.size(); iModePtr++)
      {
        CFileItem *listItem = helper_CreateModeListItem(modes[iModePtr].first, menuHook, &continuesNo);
        if (listItem)
        {
          CFileItemPtr pItem(listItem);

          if (pItem->GetProperty("ActiveMode").asBoolean())
          {
            m_activeItems[iModeType]->Add(pItem);
          }
          else
          {
            m_availableItems[iModeType]->Add(pItem);
          }
        }
        ProcessRenderLoop(false);
      }

      m_availableItems[iModeType]->Sort(SortByLabel, SortOrderAscending);
      if (iModeType == AE_DSP_MODE_TYPE_MASTER_PROCESS)
      {
        m_activeItems[iModeType]->Sort(SortByLabel, SortOrderAscending);
      }

    }
  }

  CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LIST_MODE_SELECTION, 0, 0, &items);
  OnMessage(msg);

  db.Close();

  pDlgBusy->Close();
}

void CGUIDialogAudioDSPManager::SetSelectedModeType(void)
{
  /* lock our display, as this window is rendered from the player thread */
  g_graphicsContext.Lock();
  if (m_iCurrentType > AE_DSP_MODE_TYPE_UNDEFINED && m_iCurrentType < AE_DSP_MODE_TYPE_MAX && !m_bMovingMode)
  {
    m_availableViewControl.SetCurrentView(CONTROL_LIST_AVAILABLE);
    m_activeViewControl.SetCurrentView(CONTROL_LIST_ACTIVE);

    m_availableViewControl.SetItems(*m_availableItems[m_iCurrentType]);
    m_activeViewControl.SetItems(*m_activeItems[m_iCurrentType]);
  }

  g_graphicsContext.Unlock();
}

void CGUIDialogAudioDSPManager::Clear(void)
{
  m_availableViewControl.Clear();
  m_activeViewControl.Clear();

  for (int ii = 0; ii < AE_DSP_MODE_TYPE_MAX; ii++)
  {
    m_activeItems[ii]->Clear();
    m_availableItems[ii]->Clear();
  }
}

void CGUIDialogAudioDSPManager::SaveList(void)
{
  if (!m_bContainsChanges)
   return;

  /* display the progress dialog */
  CGUIDialogBusy* pDlgBusy = (CGUIDialogBusy*)g_windowManager.GetWindow(WINDOW_DIALOG_BUSY);
  if (!pDlgBusy)
  {
    helper_LogError(__FUNCTION__);
    return;
  }
  pDlgBusy->Open();

  /* persist all modes */
  if (UpdateDatabase(pDlgBusy))
  {
    CServiceBroker::GetADSP().TriggerModeUpdate();

    m_bContainsChanges = false;
    SetItemsUnchanged();
  }

  pDlgBusy->Close();
}

bool CGUIDialogAudioDSPManager::UpdateDatabase(CGUIDialogBusy* pDlgBusy)
{
  CActiveAEDSPDatabase db;
  if (!db.Open())
  {
    CLog::Log(LOGERROR, "DSP Manager - %s - Could not open DSP database for update!", __FUNCTION__);
    return false;
  }

  // calculate available items
  int maxItems = 0;
  for (int i = 0; i < AE_DSP_MODE_TYPE_MAX; i++)
  {
    maxItems += m_activeItems[i]->Size() + m_availableItems[i]->Size();
  }

  int processedItems = 0;
  for (int i = 0; i < AE_DSP_MODE_TYPE_MAX; i++)
  {
    for (int iListPtr = 0; iListPtr < m_activeItems[i]->Size(); iListPtr++)
    {
      CFileItemPtr pItem = m_activeItems[i]->Get(iListPtr);
      if (pItem->GetProperty("Changed").asBoolean())
      {
        bool success = db.UpdateMode( i,
                                      pItem->GetProperty("ActiveMode").asBoolean(),
                                      (int)pItem->GetProperty("AddonId").asInteger(),
                                      (int)pItem->GetProperty("AddonModeNumber").asInteger(),
                                      (int)pItem->GetProperty("Number").asInteger());

        if (!success)
        {
          CLog::Log(LOGERROR, "DSP Manager - Could not update DSP database for active mode %i - %s!",
                                (int)pItem->GetProperty("AddonModeNumber").asInteger(),
                                pItem->GetProperty("Name").asString().c_str());
        }
      }

      processedItems++;
      if (pDlgBusy)
      {
        pDlgBusy->SetProgress((float)(processedItems * 100 / maxItems));

        if (pDlgBusy->IsCanceled())
        {
          return false;
        }
      }

      ProcessRenderLoop(false);
    }

    for (int iListPtr = 0; iListPtr < m_availableItems[i]->Size(); iListPtr++)
    {
      CFileItemPtr pItem = m_availableItems[i]->Get(iListPtr);
      if (pItem && pItem->GetProperty("Changed").asBoolean())
      {
        bool success = db.UpdateMode( i,
                                      pItem->GetProperty("ActiveMode").asBoolean(),
                                      (int)pItem->GetProperty("AddonId").asInteger(),
                                      (int)pItem->GetProperty("AddonModeNumber").asInteger(),
                                      (int)pItem->GetProperty("Number").asInteger());

        if (!success)
        {
          CLog::Log(LOGERROR, "DSP Manager - Could not update DSP database for available mode %i - %s!",
                                (int)pItem->GetProperty("AddonModeNumber").asInteger(),
                                pItem->GetProperty("Name").asString().c_str());
        }
      }

      processedItems++;
      if (pDlgBusy)
      {
        pDlgBusy->SetProgress((float)(processedItems * 100 / maxItems));

        if (pDlgBusy->IsCanceled())
        {
          return false;
        }
      }

      ProcessRenderLoop(false);
    }
  }
  db.Close();
  return true;
}

void CGUIDialogAudioDSPManager::SetItemsUnchanged()
{
  for (int i = 0; i < AE_DSP_MODE_TYPE_MAX; i++)
  {
    for (int iItemPtr = 0; iItemPtr < m_activeItems[i]->Size(); iItemPtr++)
    {
      CFileItemPtr pItem = m_activeItems[i]->Get(iItemPtr);
      if (pItem)
        pItem->SetProperty("Changed", false);
    }

    for (int iItemPtr = 0; iItemPtr < m_availableItems[i]->Size(); iItemPtr++)
    {
      CFileItemPtr pItem = m_availableItems[i]->Get(iItemPtr);
      if (pItem)
        pItem->SetProperty("Changed", false);
    }
  }
}

void CGUIDialogAudioDSPManager::Renumber(void)
{
  int iNextModeNumber(0);
  std::string strNumber;
  CFileItemPtr pItem;

  for (int iModePtr = 0; iModePtr < m_activeItems[m_iCurrentType]->Size(); iModePtr++)
  {
    pItem = m_activeItems[m_iCurrentType]->Get(iModePtr);
    strNumber = StringUtils::Format("%i", ++iNextModeNumber);
    pItem->SetProperty("Number", strNumber);
  }
}


//! ---- Helper functions ----

void CGUIDialogAudioDSPManager::helper_LogError(const char *function)
{
  CLog::Log(LOGERROR, "DSP Manager - %s - GUI value error", function);
}

int CGUIDialogAudioDSPManager::helper_TranslateModeType(std::string ModeString)
{
  int iType = AE_DSP_MODE_TYPE_UNDEFINED;
  for (unsigned int ii = 0; ii < ARRAY_SIZE(dsp_mode_types) && iType == AE_DSP_MODE_TYPE_UNDEFINED; ii++)
  {
    if (StringUtils::EqualsNoCase(ModeString, dsp_mode_types[ii].sModeType))
    {
      iType = dsp_mode_types[ii].iModeType;
    }
  }

  return iType;
}

CFileItem *CGUIDialogAudioDSPManager::helper_CreateModeListItem(CActiveAEDSPModePtr &ModePointer, AE_DSP_MENUHOOK_CAT &MenuHook, int *ContinuesNo)
{
  CFileItem *pItem = NULL;

  if (!ContinuesNo)
  {
    return pItem;
  }

  // start to get Addon and Mode properties
  const int AddonID = ModePointer->AddonID();

  std::string addonName;
  if (!CServiceBroker::GetADSP().GetAudioDSPAddonName(AddonID, addonName))
  {
    return pItem;
  }

  AE_DSP_ADDON addon;
  if (!CServiceBroker::GetADSP().GetAudioDSPAddon(AddonID, addon))
  {
    return pItem;
  }

  std::string modeName = g_localizeStrings.GetAddonString(addon->ID(), ModePointer->ModeName());

  std::string description;
  if (ModePointer->ModeDescription() > -1)
  {
    description = g_localizeStrings.GetAddonString(addon->ID(), ModePointer->ModeDescription());
  }
  else
  {
    description = g_localizeStrings.Get(15063);
  }

  bool isActive = ModePointer->IsEnabled();
  int number = ModePointer->ModePosition();
  int dialogId = helper_GetDialogId(ModePointer, MenuHook, addon, addonName);
  // end to get Addon and Mode properties

  if (isActive)
  {
    if (number <= 0)
    {
      number = *ContinuesNo;
      (*ContinuesNo)++;
    }

    std::string str = StringUtils::Format("%i:%i:%i:%s",
                                      number,
                                      AddonID,
                                      ModePointer->AddonModeNumber(),
                                      ModePointer->AddonModeName().c_str());

    pItem = new CFileItem(str);
  }
  else
  {
    pItem = new CFileItem(modeName);
  }

  // set list item properties
  pItem->SetProperty("ActiveMode", isActive);
  pItem->SetProperty("Number", number);
  pItem->SetLabel(modeName);
  pItem->SetProperty("Description", description);
  pItem->SetProperty("Help", ModePointer->ModeHelp());
  if (ModePointer->IconOwnModePath().empty())
    pItem->SetIconImage("DefaultAddonAudioDSP.png");
  else
    pItem->SetIconImage(ModePointer->IconOwnModePath());
  pItem->SetProperty("SettingsDialog", dialogId);
  pItem->SetProperty("AddonId", AddonID);
  pItem->SetProperty("AddonModeNumber", ModePointer->AddonModeNumber());
  pItem->SetLabel2(addonName);
  pItem->SetProperty("Changed", false);

  return pItem;
}

int CGUIDialogAudioDSPManager::helper_GetDialogId(CActiveAEDSPModePtr &ModePointer, AE_DSP_MENUHOOK_CAT &MenuHook, AE_DSP_ADDON &Addon, std::string AddonName)
{
  int dialogId = 0;

  if (ModePointer->HasSettingsDialog())
  {
    AE_DSP_MENUHOOKS hooks;

    // Find first general settings dialog about mode
    if (CServiceBroker::GetADSP().GetMenuHooks(ModePointer->AddonID(), AE_DSP_MENUHOOK_SETTING, hooks))
    {
      for (unsigned int i = 0; i < hooks.size() && dialogId == 0; i++)
      {
        if (hooks[i].iRelevantModeId == ModePointer->AddonModeNumber())
        {
          dialogId = hooks[i].iHookId;
        }
      }
    }

    // If nothing was present, check for playback settings
    if (dialogId == 0 && CServiceBroker::GetADSP().GetMenuHooks(ModePointer->AddonID(), MenuHook, hooks))
    {
      for (unsigned int i = 0; i < hooks.size() && (dialogId == 0 || dialogId != -1); i++)
      {
        if (hooks[i].iRelevantModeId == ModePointer->AddonModeNumber())
        {
          if (!hooks[i].bNeedPlayback)
          {
            dialogId = hooks[i].iHookId;
          }
          else
          {
            dialogId = -1;
          }
        }
      }
    }

    if (dialogId == 0)
      CLog::Log(LOGERROR, "DSP Dialog Manager - %s - Present marked settings dialog of mode %s on addon %s not found",
                            __FUNCTION__,
                            g_localizeStrings.GetAddonString(Addon->ID(), ModePointer->ModeName()).c_str(),
                            AddonName.c_str());
  }

  return dialogId;
}

AE_DSP_MENUHOOK_CAT CGUIDialogAudioDSPManager::helper_GetMenuHookCategory(int CurrentType)
{
  AE_DSP_MENUHOOK_CAT menuHook = AE_DSP_MENUHOOK_ALL;
  switch (CurrentType)
  {
  case AE_DSP_MODE_TYPE_PRE_PROCESS:
    menuHook = AE_DSP_MENUHOOK_PRE_PROCESS;
    break;
  case AE_DSP_MODE_TYPE_MASTER_PROCESS:
    menuHook = AE_DSP_MENUHOOK_MASTER_PROCESS;
    break;
  case AE_DSP_MODE_TYPE_POST_PROCESS:
    menuHook = AE_DSP_MENUHOOK_POST_PROCESS;
    break;
  case AE_DSP_MODE_TYPE_INPUT_RESAMPLE:
  case AE_DSP_MODE_TYPE_OUTPUT_RESAMPLE:
    menuHook = AE_DSP_MENUHOOK_RESAMPLE;
    break;
  default:
    menuHook = AE_DSP_MENUHOOK_ALL;
    break;
  };

  return menuHook;
}
