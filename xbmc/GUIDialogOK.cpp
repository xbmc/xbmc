#include "stdafx.h"
#include "GUIDialogOK.h"
#include "application.h"
#include "util.h"

#define ID_BUTTON_NO   10
#define ID_BUTTON_YES  11

CGUIDialogOK::CGUIDialogOK(void)
:CGUIDialog(0)
{
	m_bConfirmed=false;
}

CGUIDialogOK::~CGUIDialogOK(void)
{
}

bool CGUIDialogOK::OnMessage(CGUIMessage& message)
{
	switch ( message.GetMessage() )
  {
    case GUI_MSG_WINDOW_INIT:
    {
			CGUIDialog::OnMessage(message);
			m_bConfirmed=false;
			return true;
		}
		break;

    case GUI_MSG_CLICKED:
    {
			int iAction=message.GetParam1();
			if (1||ACTION_SELECT_ITEM==iAction)
			{
				int iControl=message.GetSenderId();
        if ( GetControl(ID_BUTTON_YES) == NULL)
        {
          m_bConfirmed=true;
					Close();
					return true;
        }
				if (iControl==ID_BUTTON_NO)
				{
					m_bConfirmed=false;
					Close();
					return true;
				}
				if (iControl==ID_BUTTON_YES)
				{
					m_bConfirmed=true;
					Close();
					return true;
				}
			}
		}
		break;
	}
  //only allow messages from or for this dialog
  if (
    (message.GetSenderId()  == GetID()) || 
    (message.GetControlId() == GetID()) || 
    (message.GetControlId() == 0)
  ) {
	  return CGUIDialog::OnMessage(message);
  }
  return false;
}

// \brief Show CGUIDialogOK dialog, then wait for user to dismiss it.
// \param dlgHeading String shown on dialog title. Converts to localized string if contains a positive integer.
// \param dlgLine0 String shown on dialog line 0. Converts to localized string if contains a positive integer.
// \param dlgLine1 String shown on dialog line 1. Converts to localized string if contains a positive integer.
// \param dlgLine2 String shown on dialog line 2. Converts to localized string if contains a positive integer.
void CGUIDialogOK::ShowAndGetInput(const CStdStringW& dlgHeading, const CStdStringW& dlgLine0, const CStdStringW& dlgLine1, const CStdStringW& dlgLine2)
{
  CGUIDialogOK *pDialog = &(g_application.m_guiDialogOK);

  // HACK: This won't work if the label specified is actually a positive numeric value, but that's very unlikely
  if (!CUtil::IsNaturalNumber(dlgHeading))
    pDialog->SetHeading( dlgHeading );
  else
    pDialog->SetHeading( _wtoi(dlgHeading.c_str()) );

  if (!CUtil::IsNaturalNumber(dlgLine0))
    pDialog->SetLine( 0, dlgLine0 );
  else
    pDialog->SetLine( 0, _wtoi(dlgLine0.c_str()) );

  if (!CUtil::IsNaturalNumber(dlgLine1))
    pDialog->SetLine( 1, dlgLine1 );
  else
    pDialog->SetLine( 1, _wtoi(dlgLine1.c_str()) );

  if (!CUtil::IsNaturalNumber(dlgLine2))
    pDialog->SetLine( 2, dlgLine2 );
  else
    pDialog->SetLine( 2, _wtoi(dlgLine2.c_str()) );

  pDialog->DoModal(m_gWindowManager.GetActiveWindow());
  
  return;
}

bool CGUIDialogOK::IsConfirmed() const
{
	return m_bConfirmed;
}


void  CGUIDialogOK::SetHeading(const wstring& strLine)
{
	CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),1);
	msg.SetLabel(strLine);
	OnMessage(msg);
}

void  CGUIDialogOK::SetHeading(const string& strLine)
{
	CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),1);
	msg.SetLabel(strLine);
	OnMessage(msg);
}

void CGUIDialogOK::SetLine(int iLine, const wstring& strLine)
{
	CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),iLine+2);
	msg.SetLabel(strLine);
	OnMessage(msg);
}

void CGUIDialogOK::SetLine(int iLine, const string& strLine)
{
	CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),iLine+2);
	msg.SetLabel(strLine);
	OnMessage(msg);
}
void CGUIDialogOK::SetHeading(int iString)
{
	CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),1);
	msg.SetLabel(iString);
	OnMessage(msg);
}

void	CGUIDialogOK::SetLine(int iLine, int iString)
{
	CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),iLine+2);
	msg.SetLabel(iString);
	OnMessage(msg);
}

void CGUIDialogOK::OnAction(const CAction &action)
{
	if (action.wID == ACTION_CLOSE_DIALOG || action.wID == ACTION_PREVIOUS_MENU)
  {
		Close();
		return;
  }
	CGUIDialog::OnAction(action);
}
