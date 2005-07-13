#include "stdafx.h"
#include "GUIDialogSelect.h"
#include "Application.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define CONTROL_HEADING       4
#define CONTROL_LIST          3
#define CONTROL_NUMBEROFFILES 2
#define CONTROL_BUTTON        5

struct SSortDialogSelect
{
  bool operator()(CGUIListItem* pStart, CGUIListItem* pEnd)
  {
    CGUIListItem& rpStart = *pStart;
    CGUIListItem& rpEnd = *pEnd;

    CStdString strLabel1 = rpStart.GetLabel();
    strLabel1.ToLower();

    CStdString strLabel2 = rpEnd.GetLabel();
    strLabel2.ToLower();

    if (m_bSortAscending)
      return (strcmp(strLabel1.c_str(), strLabel2.c_str()) < 0);
    else
      return (strcmp(strLabel1.c_str(), strLabel2.c_str()) >= 0);
  }

  bool m_bSortAscending;
};

CGUIDialogSelect::CGUIDialogSelect(void)
    : CGUIDialog(0)
{
  m_bButtonEnabled = false;
}

CGUIDialogSelect::~CGUIDialogSelect(void)
{}

bool CGUIDialogSelect::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      Reset();
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      m_bButtonPressed = false;
      CGUIDialog::OnMessage(message);
      m_iSelected = -1;
      CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_LIST, 0, 0, NULL);
      g_graphicsContext.SendMessage(msg);

      for (int i = 0; i < (int)m_vecList.size(); i++)
      {
        CGUIListItem* pItem = m_vecList[i];
        CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_LIST, 0, 0, (void*)pItem);
        g_graphicsContext.SendMessage(msg);
      }
      WCHAR wszText[20];
      const WCHAR* szText = g_localizeStrings.Get(127).c_str();
      swprintf(wszText, L"%i %s", m_vecList.size(), szText);

      SET_CONTROL_LABEL(CONTROL_NUMBEROFFILES, wszText);

      if (m_bButtonEnabled)
      {
        CGUIMessage msg2(GUI_MSG_VISIBLE, GetID(), CONTROL_BUTTON, 0, 0, NULL);
        g_graphicsContext.SendMessage(msg2);
      }
      else
      {
        CGUIMessage msg2(GUI_MSG_HIDDEN, GetID(), CONTROL_BUTTON, 0, 0, NULL);
        g_graphicsContext.SendMessage(msg2);
      }
      return true;
    }
    break;


  case GUI_MSG_CLICKED:
    {

      int iControl = message.GetSenderId();
      if (CONTROL_LIST == iControl)
      {
        int iAction = message.GetParam1();
        if (ACTION_SELECT_ITEM == iAction)
        {
          CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControl, 0, 0, NULL);
          g_graphicsContext.SendMessage(msg);
          m_iSelected = msg.GetParam1();
          m_strSelected = m_vecList[m_iSelected]->GetLabel();
          Close();
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
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogSelect::Close()
{
  CGUIDialog::Close();

  for (int i = 0; i < (int)m_vecList.size(); ++i)
  {
    CGUIListItem* pItem = m_vecList[i];
    delete pItem;
  }
  m_vecList.erase(m_vecList.begin(), m_vecList.end());
}

void CGUIDialogSelect::Reset()
{
  m_bButtonEnabled = false;
}

void CGUIDialogSelect::Add(const CStdString& strLabel)
{
  CGUIListItem* pItem = new CGUIListItem(strLabel);
  m_vecList.push_back(pItem);
}

int CGUIDialogSelect::GetSelectedLabel() const
{
  return m_iSelected;
}

const CStdString& CGUIDialogSelect::GetSelectedLabelText()
{
  return m_strSelected;
}

void CGUIDialogSelect::SetHeading(const wstring& strLine)
{
  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), CONTROL_HEADING);
  msg.SetLabel(strLine);
  OnMessage(msg);
}

void CGUIDialogSelect::SetHeading(const string& strLine)
{
  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), CONTROL_HEADING);
  msg.SetLabel(strLine);
  OnMessage(msg);
}

void CGUIDialogSelect::SetHeading(int iString)
{
  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), CONTROL_HEADING);
  msg.SetLabel(iString);
  OnMessage(msg);
}

void CGUIDialogSelect::EnableButton(bool bOnOff)
{
  m_bButtonEnabled = bOnOff;
}

void CGUIDialogSelect::SetButtonLabel(int iString)
{
  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), CONTROL_BUTTON);
  msg.SetLabel(iString);
  OnMessage(msg);
}

bool CGUIDialogSelect::IsButtonPressed()
{
  return m_bButtonPressed;
}

void CGUIDialogSelect::Sort(bool bSortAscending /*=true*/)
{
  SSortDialogSelect sortmethod;
  sortmethod.m_bSortAscending = bSortAscending;
  sort(m_vecList.begin(), m_vecList.end(), sortmethod);
}

