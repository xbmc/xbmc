#include "guidialogok.h"
#include "guiWindowManager.h"
#include "localizeStrings.h"

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
			m_bConfirmed=false;
		}
		break;

    case GUI_MSG_CLICKED:
    {
      int iControl=message.GetSenderId();
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
		break;
	}
	return CGUIDialog::OnMessage(message);
}

bool CGUIDialogOK::IsConfirmed() const
{
	return m_bConfirmed;
}


void  CGUIDialogOK::SetHeading(const wstring& strLine)
{
	int iControl=1;
  CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),iControl,0,0,(void*)strLine.c_str());
  OnMessage(msg);

}

void CGUIDialogOK::SetLine(int iLine, const wstring& strLine)
{
	int iControl=iLine+2;

  CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),iControl,0,0,(void*)strLine.c_str());
  OnMessage(msg);
}

void CGUIDialogOK::SetLine(int iLine, const string& strLine)
{
	WCHAR wszLine[1024];
	swprintf(wszLine,L"%S", strLine.c_str());
	SetLine(iLine,wszLine);
}
void CGUIDialogOK::SetHeading(int iString)
{
	const WCHAR* pszTxt=g_localizeStrings.Get(iString).c_str();
	SetHeading(pszTxt);
}


void	CGUIDialogOK::SetLine(int iLine, int iString)
{
	const WCHAR* pszTxt=g_localizeStrings.Get(iString).c_str();
	SetLine(iLine,pszTxt);
}