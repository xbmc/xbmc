
#include "GUIDialogSelect.h"
#include "localizestrings.h"
#include "application.h"

#define CONTROL_HEADING	4
#define CONTROL_LIST 3
#define CONTROL_NUMBEROFFILES 2

CGUIDialogSelect::CGUIDialogSelect(void)
:CGUIDialog(0)
{
}

CGUIDialogSelect::~CGUIDialogSelect(void)
{
}


void CGUIDialogSelect::OnAction(const CAction &action)
{
	if (action.wID == ACTION_CLOSE_DIALOG || action.wID == ACTION_PREVIOUS_MENU)
  {
		Close();
		return;
  }
	CGUIWindow::OnAction(action);
}

bool CGUIDialogSelect::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
		case GUI_MSG_WINDOW_DEINIT:
    {
			Reset();
			g_application.EnableOverlay();			
		}
		break;
    case GUI_MSG_WINDOW_INIT:
    {
			CGUIDialog::OnMessage(message);
			g_application.DisableOverlay();
			m_iSelected=-1;
			CGUIMessage msg(GUI_MSG_LABEL_RESET,GetID(),CONTROL_LIST,0,0,NULL);
			g_graphicsContext.SendMessage(msg);         

			for (int i=0; i < (int)m_vecList.size(); i++)
			{
				CGUIListItem* pItem=m_vecList[i];
				CGUIMessage msg(GUI_MSG_LABEL_ADD,GetID(),CONTROL_LIST,0,0,(void*)pItem);
				g_graphicsContext.SendMessage(msg);          
			}
			WCHAR wszText[20];
			const WCHAR* szText=g_localizeStrings.Get(127).c_str();
			swprintf(wszText,L"%i %s", m_vecList.size(),szText);

			SET_CONTROL_LABEL(GetID(), CONTROL_NUMBEROFFILES,wszText);
			return true;
		}
		break;


    case GUI_MSG_CLICKED:
    {
			
      int iControl=message.GetSenderId();
			if (CONTROL_LIST==iControl)
			{
				int iAction=message.GetParam1();
				if (ACTION_SELECT_ITEM==iAction)
				{
					CGUIMessage msg(GUI_MSG_ITEM_SELECTED,GetID(),iControl,0,0,NULL);
					g_graphicsContext.SendMessage(msg);         
					m_iSelected=msg.GetParam1();
					Close();
				}
			}
    }
    break;
  }

  return CGUIWindow::OnMessage(message);
}


void CGUIDialogSelect::Reset()
{
	for (int i=0; i < (int)m_vecList.size(); ++i)
	{
		CGUIListItem* pItem=m_vecList[i];
		delete pItem;
	}
	m_vecList.erase(m_vecList.begin(),m_vecList.end());
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
void  CGUIDialogSelect::SetHeading(const wstring& strLine)
{
	CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),CONTROL_HEADING);
	msg.SetLabel(strLine);
	OnMessage(msg);
}

void  CGUIDialogSelect::SetHeading(const string& strLine)
{
	CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),CONTROL_HEADING);
	msg.SetLabel(strLine);
	OnMessage(msg);
}

void CGUIDialogSelect::SetHeading(int iString)
{
	CGUIMessage msg(GUI_MSG_LABEL_SET,GetID(),1);
	msg.SetLabel(iString);
	OnMessage(msg);
}
