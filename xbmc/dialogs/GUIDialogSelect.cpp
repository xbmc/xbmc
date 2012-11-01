/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIDialogSelect.h"
#include "guilib/GUIWindowManager.h"
#include "FileItem.h"
#include "guilib/LocalizeStrings.h"

#define CONTROL_HEADING       1
#define CONTROL_LIST          3
#define CONTROL_NUMBEROFFILES 2
#define CONTROL_BUTTON        5
#define CONTROL_DETAILS       6

CGUIDialogSelect::CGUIDialogSelect(void)
    : CGUIDialogBoxBase(WINDOW_DIALOG_SELECT, "DialogSelect.xml")
{
  m_bButtonEnabled = false;
  m_buttonString = -1;
  m_useDetails = false;
  m_vecListInternal = new CFileItemList;
  m_selectedItems = new CFileItemList;
  m_multiSelection = false;
  m_vecList = m_vecListInternal;
  m_iSelected = -1;
  m_loadType = KEEP_IN_MEMORY;
}

CGUIDialogSelect::~CGUIDialogSelect(void)
{
  delete m_vecListInternal;
  delete m_selectedItems;
}

bool CGUIDialogSelect::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      CGUIDialog::OnMessage(message);
      m_viewControl.Clear();

      m_bButtonEnabled = false;
      m_useDetails = false;
      m_multiSelection = false;

      // construct selected items list
      m_selectedItems->Clear();
      m_iSelected = -1;
      for (int i = 0 ; i < m_vecList->Size() ; i++)
      {
        CFileItemPtr item = m_vecList->Get(i);
        if (item->IsSelected())
        {
          m_selectedItems->Add(item);
          if (m_iSelected == -1)
            m_iSelected = i;
        }
      }

      m_vecListInternal->Clear();
      m_vecList = m_vecListInternal;

      m_buttonString = -1;
      SET_CONTROL_LABEL(CONTROL_BUTTON, "");
      return true;
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      m_bButtonPressed = false;
      m_bConfirmed = false;
      CGUIDialog::OnMessage(message);
      return true;
    }
    break;


  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (m_viewControl.HasControl(CONTROL_LIST))
      {
        int iAction = message.GetParam1();
        if (ACTION_SELECT_ITEM == iAction || ACTION_MOUSE_LEFT_CLICK == iAction)
        {
          int iSelected = m_viewControl.GetSelectedItem();
          if(iSelected >= 0 && iSelected < (int)m_vecList->Size())
          {
            CFileItemPtr item(m_vecList->Get(iSelected));
            if (m_multiSelection)
              item->Select(!item->IsSelected());
            else
            {
              for (int i = 0 ; i < m_vecList->Size() ; i++)
                m_vecList->Get(i)->Select(false);
              item->Select(true);
              m_bConfirmed = true;
              Close();
            }
          }
        }
      }
      if (CONTROL_BUTTON == iControl)
      {
        m_iSelected = -1;
        m_bButtonPressed = true;
        if (m_multiSelection)
          m_bConfirmed = true;
        Close();
      }
    }
    break;
  case GUI_MSG_SETFOCUS:
    {
      if (m_viewControl.HasControl(message.GetControlId()) && m_viewControl.GetCurrentControl() != message.GetControlId())
      {
        m_viewControl.SetFocused();
        return true;
      }
    }
    break;
  }

  return CGUIDialog::OnMessage(message);
}

bool CGUIDialogSelect::OnBack(int actionID)
{
  m_iSelected = -1;
  m_selectedItems->Clear();
  m_bConfirmed = false;
  return CGUIDialog::OnBack(actionID);
}

void CGUIDialogSelect::Reset()
{
  m_bButtonEnabled = false;
  m_useDetails = false;
  m_multiSelection = false;
  m_iSelected = -1;
  m_vecListInternal->Clear();
  m_selectedItems->Clear();
  m_vecList = m_vecListInternal;
}

void CGUIDialogSelect::Add(const CStdString& strLabel)
{
  CFileItemPtr pItem(new CFileItem(strLabel));
  m_vecListInternal->Add(pItem);
}

void CGUIDialogSelect::Add(const CFileItemList& items)
{
  for (int i=0;i<items.Size();++i)
  {
    CFileItemPtr item = items[i];
    Add(item.get());
  }
}

void CGUIDialogSelect::Add(const CFileItem* pItem)
{
  CFileItemPtr item(new CFileItem(*pItem));
  m_vecListInternal->Add(item);
}

void CGUIDialogSelect::SetItems(CFileItemList* pList)
{
  m_vecList = pList;
}

int CGUIDialogSelect::GetSelectedLabel() const
{
  return m_iSelected;
}

const CFileItemPtr CGUIDialogSelect::GetSelectedItem()
{
  return m_selectedItems->Size() > 0 ? m_selectedItems->Get(0) : CFileItemPtr(new CFileItem);
}

const CStdString& CGUIDialogSelect::GetSelectedLabelText()
{
  return GetSelectedItem()->GetLabel();
}

const CFileItemList& CGUIDialogSelect::GetSelectedItems() const
{
  return *m_selectedItems;
}

void CGUIDialogSelect::EnableButton(bool enable, int string)
{
  m_bButtonEnabled = enable;
  m_buttonString = string;

  if (IsActive())
    SetupButton();
}

bool CGUIDialogSelect::IsButtonPressed()
{
  return m_bButtonPressed;
}

void CGUIDialogSelect::Sort(bool bSortOrder /*=true*/)
{
  m_vecList->Sort(SORT_METHOD_LABEL, bSortOrder ? SortOrderAscending : SortOrderDescending);
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
  if (!m_multiSelection || m_iSelected < 0 || m_iSelected > iSelected)
    m_iSelected = iSelected;
  m_vecList->Get(iSelected)->Select(true);
  m_selectedItems->Add(m_vecList->Get(iSelected));
}

void CGUIDialogSelect::SetSelected(const CStdString &strSelectedLabel)
{
  if (strSelectedLabel.empty())
    return;

  for (int index = 0; index < m_vecList->Size(); index++)
  {
    if (strSelectedLabel.Equals(m_vecList->Get(index)->GetLabel()))
    {
      SetSelected(index);
      return;
    }
  }
}

void CGUIDialogSelect::SetSelected(std::vector<int> selectedIndexes)
{
  if (selectedIndexes.empty())
    return;

  for (std::vector<int>::const_iterator it = selectedIndexes.begin(); it != selectedIndexes.end(); it++)
    SetSelected(*it);
}

void CGUIDialogSelect::SetSelected(const std::vector<CStdString> &selectedLabels)
{
  if (selectedLabels.empty())
    return;

  for (std::vector<CStdString>::const_iterator it = selectedLabels.begin(); it != selectedLabels.end(); it++)
    SetSelected(*it);
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
  m_viewControl.AddView(GetControl(CONTROL_LIST));
  m_viewControl.AddView(GetControl(CONTROL_DETAILS));
}

void CGUIDialogSelect::OnInitWindow()
{
  m_viewControl.SetItems(*m_vecList);
  m_selectedItems->Clear();
  if (m_iSelected == -1)
  {
    for(int i = 0 ; i < m_vecList->Size(); i++)
    {
      if (m_vecList->Get(i)->IsSelected())
      {
        m_iSelected = i;
        break;
      }
    }
  }
  m_viewControl.SetCurrentView(m_useDetails ? CONTROL_DETAILS : CONTROL_LIST);

  CStdString items;
  items.Format("%i %s", m_vecList->Size(), g_localizeStrings.Get(127).c_str());
  SET_CONTROL_LABEL(CONTROL_NUMBEROFFILES, items);
  
  if (m_multiSelection)
    EnableButton(true, 186);

  SetupButton();
  CGUIDialogBoxBase::OnInitWindow();

  if (m_iSelected >= 0)
    m_viewControl.SetSelectedItem(m_iSelected);
}

void CGUIDialogSelect::OnWindowUnload()
{
  CGUIDialog::OnWindowUnload();
  m_viewControl.Reset();
}

void CGUIDialogSelect::SetupButton()
{
  if (m_bButtonEnabled)
  {
    SET_CONTROL_LABEL(CONTROL_BUTTON, g_localizeStrings.Get(m_buttonString));
    SET_CONTROL_VISIBLE(CONTROL_BUTTON);
  }
  else
    SET_CONTROL_HIDDEN(CONTROL_BUTTON);
}
