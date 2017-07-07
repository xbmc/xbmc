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

#include "GUIDialogSelect.h"
#include "FileItem.h"
#include "input/Key.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"

#define CONTROL_HEADING         1
#define CONTROL_NUMBER_OF_ITEMS 2
#define CONTROL_SIMPLE_LIST     3
#define CONTROL_DETAILED_LIST   6
#define CONTROL_EXTRA_BUTTON    5
#define CONTROL_CANCEL_BUTTON   7

CGUIDialogSelect::CGUIDialogSelect() : CGUIDialogSelect(WINDOW_DIALOG_SELECT) {}

CGUIDialogSelect::CGUIDialogSelect(int windowId)
    : CGUIDialogBoxBase(windowId, "DialogSelect.xml"),
    m_bButtonEnabled(false),
    m_bButtonPressed(false),
    m_buttonLabel(-1),
    m_selectedItem(nullptr),
    m_useDetails(false),
    m_multiSelection(false),
    m_selectedItems(),
    m_vecList(new CFileItemList())
{
  m_bConfirmed = false;
  m_loadType = KEEP_IN_MEMORY;
}

CGUIDialogSelect::~CGUIDialogSelect(void) = default;

bool CGUIDialogSelect::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      CGUIDialogBoxBase::OnMessage(message);

      m_bButtonEnabled = false;
      m_useDetails = false;
      m_multiSelection = false;

      // construct selected items list
      m_selectedItems.clear();
      m_selectedItem = nullptr;
      for (int i = 0 ; i < m_vecList->Size() ; i++)
      {
        CFileItemPtr item = m_vecList->Get(i);
        if (item->IsSelected())
        {
          m_selectedItems.push_back(i);
          if (!m_selectedItem)
            m_selectedItem = item;
        }
      }
      m_vecList->Clear();
      return true;
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      m_bButtonPressed = false;
      m_bConfirmed = false;
      CGUIDialogBoxBase::OnMessage(message);
      return true;
    }
    break;


  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (m_viewControl.HasControl(CONTROL_SIMPLE_LIST))
      {
        int iAction = message.GetParam1();
        if (ACTION_SELECT_ITEM == iAction || ACTION_MOUSE_LEFT_CLICK == iAction)
        {
          int iSelected = m_viewControl.GetSelectedItem();
          if (iSelected >= 0 && iSelected < m_vecList->Size())
          {
            CFileItemPtr item(m_vecList->Get(iSelected));
            if (m_multiSelection)
              item->Select(!item->IsSelected());
            else
            {
              for (int i = 0 ; i < m_vecList->Size() ; i++)
                m_vecList->Get(i)->Select(false);
              item->Select(true);
              OnSelect(iSelected);
            }
          }
        }
      }
      if (iControl == CONTROL_EXTRA_BUTTON)
      {
        m_selectedItem = nullptr;
        m_bButtonPressed = true;
        if (m_multiSelection)
          m_bConfirmed = true;
        Close();
      }
      else if (iControl == CONTROL_CANCEL_BUTTON)
      {
        m_selectedItem = nullptr;
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
          if (m_bButtonEnabled)
            SET_CONTROL_FOCUS(CONTROL_EXTRA_BUTTON, 0);
          else
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

void CGUIDialogSelect::OnSelect(int idx)
{
  m_bConfirmed = true;
  Close();
}

bool CGUIDialogSelect::OnBack(int actionID)
{
  m_selectedItem = nullptr;
  m_selectedItems.clear();
  m_bConfirmed = false;
  return CGUIDialogBoxBase::OnBack(actionID);
}

void CGUIDialogSelect::Reset()
{
  m_bButtonEnabled = false;
  m_bButtonPressed = false;
  m_useDetails = false;
  m_multiSelection = false;
  m_selectedItem = nullptr;
  m_vecList->Clear();
  m_selectedItems.clear();
}

int CGUIDialogSelect::Add(const std::string& strLabel)
{
  CFileItemPtr pItem(new CFileItem(strLabel));
  m_vecList->Add(pItem);
  return m_vecList->Size() - 1;
}

int CGUIDialogSelect::Add(const CFileItem& item)
{
  m_vecList->Add(CFileItemPtr(new CFileItem(item)));
  return m_vecList->Size() - 1;
}

void CGUIDialogSelect::SetItems(const CFileItemList& pList)
{
  // need to make internal copy of list to be sure dialog is owner of it
  m_vecList->Clear();
  m_vecList->Copy(pList);
}

int CGUIDialogSelect::GetSelectedItem() const
{
  return m_selectedItems.size() > 0 ? m_selectedItems[0] : -1;
}

const CFileItemPtr CGUIDialogSelect::GetSelectedFileItem() const
{
  if (m_selectedItem)
    return m_selectedItem;
  return CFileItemPtr(new CFileItem);
}

const std::vector<int>& CGUIDialogSelect::GetSelectedItems() const
{
  return m_selectedItems;
}

void CGUIDialogSelect::EnableButton(bool enable, int label)
{
  m_bButtonEnabled = enable;
  m_buttonLabel = label;
}

bool CGUIDialogSelect::IsButtonPressed()
{
  return m_bButtonPressed;
}

void CGUIDialogSelect::Sort(bool bSortOrder /*=true*/)
{
  m_vecList->Sort(SortByLabel, bSortOrder ? SortOrderAscending : SortOrderDescending);
}

void CGUIDialogSelect::SetSelected(int iSelected)
{
  if (iSelected < 0 || iSelected >= (int)m_vecList->Size() ||
      m_vecList->Get(iSelected).get() == NULL) 
    return;

  // only set m_iSelected if there is no multi-select
  // or if it doesn't have a valid value yet
  // or if the current value is bigger than the new one
  // so that we always focus the item nearest to the beginning of the list
  if (!m_multiSelection || !m_selectedItem ||
      (!m_selectedItems.empty() && m_selectedItems.back() > iSelected))
    m_selectedItem = m_vecList->Get(iSelected);
  m_vecList->Get(iSelected)->Select(true);
  m_selectedItems.push_back(iSelected);
}

void CGUIDialogSelect::SetSelected(const std::string &strSelectedLabel)
{
  for (int index = 0; index < m_vecList->Size(); index++)
  {
    if (strSelectedLabel == m_vecList->Get(index)->GetLabel())
    {
      SetSelected(index);
      return;
    }
  }
}

void CGUIDialogSelect::SetSelected(std::vector<int> selectedIndexes)
{
  for (auto i : selectedIndexes)
    SetSelected(i);
}

void CGUIDialogSelect::SetSelected(const std::vector<std::string> &selectedLabels)
{
  for (const auto& label : selectedLabels)
    SetSelected(label);
}

void CGUIDialogSelect::SetUseDetails(bool useDetails)
{
  m_useDetails = useDetails;
}

void CGUIDialogSelect::SetMultiSelection(bool multiSelection)
{
  m_multiSelection = multiSelection;
}

CGUIControl *CGUIDialogSelect::GetFirstFocusableControl(int id)
{
  if (m_viewControl.HasControl(id))
    id = m_viewControl.GetCurrentControl();
  return CGUIDialogBoxBase::GetFirstFocusableControl(id);
}

void CGUIDialogSelect::OnWindowLoaded()
{
  CGUIDialogBoxBase::OnWindowLoaded();
  m_viewControl.Reset();
  m_viewControl.SetParentWindow(GetID());
  m_viewControl.AddView(GetControl(CONTROL_SIMPLE_LIST));
  m_viewControl.AddView(GetControl(CONTROL_DETAILED_LIST));
}

void CGUIDialogSelect::OnInitWindow()
{
  m_viewControl.SetItems(*m_vecList);
  m_selectedItems.clear();
  for(int i = 0 ; i < m_vecList->Size(); i++)
  {
    auto item = m_vecList->Get(i);
    if (item->IsSelected())
    {
      m_selectedItems.push_back(i);
      if (m_selectedItem == nullptr)
        m_selectedItem = item;
    }
  }
  m_viewControl.SetCurrentView(m_useDetails ? CONTROL_DETAILED_LIST : CONTROL_SIMPLE_LIST);

  SET_CONTROL_LABEL(CONTROL_NUMBER_OF_ITEMS, StringUtils::Format("%i %s",
      m_vecList->Size(), g_localizeStrings.Get(127).c_str()));
  
  if (m_multiSelection)
    EnableButton(true, 186);

  if (m_bButtonEnabled)
  {
    SET_CONTROL_LABEL(CONTROL_EXTRA_BUTTON, g_localizeStrings.Get(m_buttonLabel));
    SET_CONTROL_VISIBLE(CONTROL_EXTRA_BUTTON);
  }
  else
    SET_CONTROL_HIDDEN(CONTROL_EXTRA_BUTTON);

  SET_CONTROL_LABEL(CONTROL_CANCEL_BUTTON, g_localizeStrings.Get(222));

  CGUIDialogBoxBase::OnInitWindow();

  // if nothing is selected, select first item
  m_viewControl.SetSelectedItem(std::max(GetSelectedItem(), 0));
}

void CGUIDialogSelect::OnDeinitWindow(int nextWindowID)
{
  m_viewControl.Clear();
  CGUIDialogBoxBase::OnDeinitWindow(nextWindowID);
}

void CGUIDialogSelect::OnWindowUnload()
{
  CGUIDialogBoxBase::OnWindowUnload();
  m_viewControl.Reset();
}
