/*
 *  Copyright (C) 2005-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogColorPicker.h"

#include "FileItem.h"
#include "filesystem/SpecialProtocol.h"
#include "guilib/GUIColorManager.h"
#include "guilib/GUIMessage.h"
#include "guilib/LocalizeStrings.h"
#include "input/Key.h"
#include "utils/ColorUtils.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#define CONTROL_HEADING 1
#define CONTROL_NUMBER_OF_ITEMS 2
#define CONTROL_ICON_LIST 6
#define CONTROL_CANCEL_BUTTON 7

CGUIDialogColorPicker::CGUIDialogColorPicker()
  : CGUIDialogBoxBase(WINDOW_DIALOG_COLOR_PICKER, "DialogColorPicker.xml"),
    m_vecList(new CFileItemList())
{
  m_bConfirmed = false;
  m_loadType = KEEP_IN_MEMORY;
}

CGUIDialogColorPicker::~CGUIDialogColorPicker()
{
  delete m_vecList;
}

bool CGUIDialogColorPicker::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_DEINIT:
    {
      CGUIDialogBoxBase::OnMessage(message);
      m_selectedColor.clear();
      for (int i = 0; i < m_vecList->Size(); i++)
      {
        CFileItemPtr item = m_vecList->Get(i);
        if (item->IsSelected())
        {
          m_selectedColor = item->GetLabel2();
          break;
        }
      }
      m_vecList->Clear();
      return true;
    }
    break;

    case GUI_MSG_WINDOW_INIT:
    {
      m_bConfirmed = false;
      CGUIDialogBoxBase::OnMessage(message);
      return true;
    }
    break;

    case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (m_viewControl.HasControl(CONTROL_ICON_LIST))
      {
        int iAction = message.GetParam1();
        if (ACTION_SELECT_ITEM == iAction || ACTION_MOUSE_LEFT_CLICK == iAction)
        {
          int iSelected = m_viewControl.GetSelectedItem();
          if (iSelected >= 0 && iSelected < m_vecList->Size())
          {
            CFileItemPtr item(m_vecList->Get(iSelected));
            for (int i = 0; i < m_vecList->Size(); i++)
              m_vecList->Get(i)->Select(false);
            item->Select(true);
            OnSelect(iSelected);
          }
        }
      }
      if (iControl == CONTROL_CANCEL_BUTTON)
      {
        m_selectedColor.clear();
        m_vecList->Clear();
        m_bConfirmed = false;
        Close();
      }
    }
    break;

    case GUI_MSG_SETFOCUS:
    {
      if (m_viewControl.HasControl(message.GetControlId()))
      {
        if (m_vecList->IsEmpty())
        {
          SET_CONTROL_FOCUS(CONTROL_CANCEL_BUTTON, 0);
          return true;
        }
        if (m_viewControl.GetCurrentControl() != message.GetControlId())
        {
          m_viewControl.SetFocused();
          return true;
        }
      }
    }
    break;
  }

  return CGUIDialogBoxBase::OnMessage(message);
}

void CGUIDialogColorPicker::OnSelect(int idx)
{
  m_bConfirmed = true;
  Close();
}

bool CGUIDialogColorPicker::OnBack(int actionID)
{
  m_vecList->Clear();
  m_bConfirmed = false;
  return CGUIDialogBoxBase::OnBack(actionID);
}

void CGUIDialogColorPicker::Reset()
{
  m_focusToButton = false;
  m_selectedColor.clear();
  m_vecList->Clear();
}

void CGUIDialogColorPicker::AddItem(const CFileItem& item)
{
  m_vecList->Add(std::make_shared<CFileItem>(item));
}

void CGUIDialogColorPicker::SetItems(const CFileItemList& pList)
{
  // need to make internal copy of list to be sure dialog is owner of it
  m_vecList->Clear();
  m_vecList->Copy(pList);
}

void CGUIDialogColorPicker::LoadColors()
{
  LoadColors(CSpecialProtocol::TranslatePathConvertCase("special://xbmc/system/dialogcolors.xml"));
}

void CGUIDialogColorPicker::LoadColors(const std::string& filePath)
{
  CGUIColorManager colorManager;
  std::vector<std::pair<std::string, UTILS::COLOR::ColorInfo>> colors;
  if (colorManager.LoadColorsListFromXML(filePath, colors, true))
  {
    for (auto& color : colors)
    {
      CFileItem* item = new CFileItem(color.first);
      item->SetLabel2(StringUtils::Format("{:08X}", color.second.colorARGB));
      m_vecList->Add(CFileItemPtr(item));
    }
  }
  else
    CLog::Log(LOGERROR, "{} - An error occurred when loading colours from the xml file.",
              __FUNCTION__);
}

std::string CGUIDialogColorPicker::GetSelectedColor() const
{
  return m_selectedColor;
}

int CGUIDialogColorPicker::GetSelectedItem() const
{
  if (m_selectedColor.empty())
    return -1;
  for (int index = 0; index < m_vecList->Size(); index++)
  {
    if (StringUtils::CompareNoCase(m_selectedColor, m_vecList->Get(index)->GetLabel2()) == 0)
    {
      return index;
    }
  }
  return -1;
}

void CGUIDialogColorPicker::SetSelectedColor(const std::string& colorHex)
{
  m_selectedColor = colorHex;
}

void CGUIDialogColorPicker::SetButtonFocus(bool buttonFocus)
{
  m_focusToButton = buttonFocus;
}

CGUIControl* CGUIDialogColorPicker::GetFirstFocusableControl(int id)
{
  if (m_viewControl.HasControl(id))
    id = m_viewControl.GetCurrentControl();
  return CGUIDialogBoxBase::GetFirstFocusableControl(id);
}

void CGUIDialogColorPicker::OnWindowLoaded()
{
  CGUIDialogBoxBase::OnWindowLoaded();
  m_viewControl.Reset();
  m_viewControl.SetParentWindow(GetID());
  m_viewControl.AddView(GetControl(CONTROL_ICON_LIST));
}

void CGUIDialogColorPicker::OnInitWindow()
{
  if (!m_vecList || m_vecList->IsEmpty())
    CLog::Log(LOGERROR, "{} - No colours have been added in the list.", __FUNCTION__);

  m_viewControl.SetItems(*m_vecList);
  m_viewControl.SetCurrentView(CONTROL_ICON_LIST);

  SET_CONTROL_LABEL(CONTROL_NUMBER_OF_ITEMS,
                    StringUtils::Format("{} {}", m_vecList->Size(), g_localizeStrings.Get(127)));

  SET_CONTROL_LABEL(CONTROL_CANCEL_BUTTON, g_localizeStrings.Get(222));

  CGUIDialogBoxBase::OnInitWindow();

  // focus one of the buttons if explicitly requested
  // ATTENTION: this must be done after calling CGUIDialogBoxBase::OnInitWindow()
  if (m_focusToButton)
  {
    SET_CONTROL_FOCUS(CONTROL_CANCEL_BUTTON, 0);
  }

  // if nothing is selected, select first item
  m_viewControl.SetSelectedItem(std::max(GetSelectedItem(), 0));
}

void CGUIDialogColorPicker::OnDeinitWindow(int nextWindowID)
{
  m_viewControl.Clear();
  CGUIDialogBoxBase::OnDeinitWindow(nextWindowID);
}

void CGUIDialogColorPicker::OnWindowUnload()
{
  CGUIDialogBoxBase::OnWindowUnload();
  m_viewControl.Reset();
}
