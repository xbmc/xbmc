#include "stdafx.h"
#include "GUIDialogYesNo.h"
#include "application.h"
#include "util.h"

CGUIDialogYesNo::CGUIDialogYesNo(void)
:CGUIDialog(0)
{
	m_bConfirmed=false;
}

CGUIDialogYesNo::~CGUIDialogYesNo(void)
{
}

bool CGUIDialogYesNo::OnMessage(CGUIMessage& message)
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
      int iControl=message.GetSenderId();
			int iAction=message.GetParam1();
			if (1||ACTION_SELECT_ITEM==iAction)
			{
				if (iControl==10)
				{
					m_bConfirmed=false;
					Close();
					return true;
				}
				if (iControl==11)
				{
					m_bConfirmed=true;
					Close();
					return true;
				}
			}
		}
		break;
	}
	return CGUIDialog::OnMessage(message);
}

// \brief Show CGUIDialogYesNo dialog, then wait for user to dismiss it.
// \param dlgHeading String shown on dialog title. Converts to localized string if contains a positive integer.
// \param dlgLine0 String shown on dialog line 0. Converts to localized string if contains a positive integer.
// \param dlgLine1 String shown on dialog line 1. Converts to localized string if contains a positive integer.
// \param dlgLine2 String shown on dialog line 2. Converts to localized string if contains a positive integer.
// \return true if user selects Yes, false if user selects No.
bool CGUIDialogYesNo::ShowAndGetInput(const CStdStringW& dlgHeading, const CStdStringW& dlgLine0, 
                                      const CStdStringW& dlgLine1, const CStdStringW& dlgLine2)
{
  CGUIDialogYesNo *pDialog = &(g_application.m_guiDialogYesNo);

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

  return (pDialog->IsConfirmed()) ? true : false;
}

bool CGUIDialogYesNo::IsConfirmed() const
{
	return m_bConfirmed;
}

void  CGUIDialogYesNo::SetHeading(const wstring& strLine)
{
	CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),1);
	msg.SetLabel(strLine);
	OnMessage(msg);
}

void  CGUIDialogYesNo::SetHeading(const string& strLine)
{
	CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),1);
	msg.SetLabel(strLine);
	OnMessage(msg);
}

void CGUIDialogYesNo::SetLine(int iLine, const wstring& strLine)
{
	CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),iLine+2);
	msg.SetLabel(strLine);
	OnMessage(msg);
}

void CGUIDialogYesNo::SetLine(int iLine, const string& strLine)
{
	CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),iLine+2);
	msg.SetLabel(strLine);
	OnMessage(msg);
}
void CGUIDialogYesNo::SetHeading(int iString)
{
	CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),1);
	msg.SetLabel(iString);
	OnMessage(msg);
}


void	CGUIDialogYesNo::SetLine(int iLine, int iString)
{
	CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),iLine+2);
	msg.SetLabel(iString);
	OnMessage(msg);
}