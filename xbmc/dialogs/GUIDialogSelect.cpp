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
  m_useDetails = false;
  m_vecListInternal = new CFileItemList;
  m_selectedItem = new CFileItem;
  m_vecList = m_vecListInternal;
  m_iSelected = -1;
}

CGUIDialogSelect::~CGUIDialogSelect(void)
{
  delete m_vecListInternal;
  delete m_selectedItem;
}

bool CGUIDialogSelect::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      CGUIDialog::OnMessage(message);
      m_viewControl.Reset();
      m_bButtonEnabled = false;
      m_useDetails = false;
      m_vecListInternal->Clear();
      m_vecList = m_vecListInternal;
      return true;
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      m_bButtonPressed = false;
      CGUIDialog::OnMessage(message);
      m_iSelected = -1;

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
          m_iSelected = m_viewControl.GetSelectedItem();
          if(m_iSelected >= 0 && m_iSelected < (int)m_vecList->Size())
          {
            *m_selectedItem = *m_vecList->Get(m_iSelected);
            Close();
          }
          else
            m_iSelected = -1;
        }
      }
      if (CONTROL_BUTTON == iControl)
      {
        m_iSelected = -1;
        m_bButtonPressed = true;
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

void CGUIDialogSelect::Reset()
{
  m_bButtonEnabled = false;
  m_useDetails = false;
  m_iSelected = -1;
  m_vecListInternal->Clear();
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

const CFileItem& CGUIDialogSelect::GetSelectedItem()
{
  return *m_selectedItem;
}

const CStdString& CGUIDialogSelect::GetSelectedLabelText()
{
  return m_selectedItem->GetLabel();
}

void CGUIDialogSelect::EnableButton(bool enable, int string)
{
  m_bButtonEnabled = enable;
  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), CONTROL_BUTTON);
  msg.SetLabel(string);
  OnMessage(msg);
}

bool CGUIDialogSelect::IsButtonPressed()
{
  return m_bButtonPressed;
}

void CGUIDialogSelect::Sort(bool bSortOrder /*=true*/)
{
  m_vecList->Sort(SORT_METHOD_LABEL,bSortOrder?SORT_ORDER_ASC:SORT_ORDER_DESC);
}

void CGUIDialogSelect::SetSelected(int iSelected)
{
  if (iSelected < 0 || iSelected >= (int)m_vecList->Size()) return;
  m_iSelected = iSelected;
}

void CGUIDialogSelect::SetUseDetails(bool useDetails)
{
  m_useDetails = useDetails;
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
  m_viewControl.SetCurrentView(m_useDetails ? CONTROL_DETAILS : CONTROL_LIST);

  CStdString items;
  items.Format("%i %s", m_vecList->Size(), g_localizeStrings.Get(127).c_str());
  SET_CONTROL_LABEL(CONTROL_NUMBEROFFILES, items);

  if (m_bButtonEnabled)
  {
    CGUIMessage msg2(GUI_MSG_VISIBLE, GetID(), CONTROL_BUTTON);
    g_windowManager.SendMessage(msg2);
  }
  else
  {
    CGUIMessage msg2(GUI_MSG_HIDDEN, GetID(), CONTROL_BUTTON);
    g_windowManager.SendMessage(msg2);
  }
  CGUIDialogBoxBase::OnInitWindow();

  if (m_iSelected >= 0)
    m_viewControl.SetSelectedItem(m_iSelected);
}
