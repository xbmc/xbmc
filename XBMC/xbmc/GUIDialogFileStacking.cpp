#include "GUIDialogFileStacking.h"
#include "guiWindowManager.h"
#include "localizeStrings.h"

CGUIDialogFileStacking::CGUIDialogFileStacking(void)
:CGUIDialog(0)
{
	m_iSelectedFile=-1;
	m_iNumberOfFiles=0;
}

CGUIDialogFileStacking::~CGUIDialogFileStacking(void)
{
}
void CGUIDialogFileStacking::OnAction(const CAction &action)
{
	if (action.wID == ACTION_CLOSE_DIALOG || action.wID == ACTION_PREVIOUS_MENU)
  {
		Close();
		return;
  }
	CGUIWindow::OnAction(action);
}

bool CGUIDialogFileStacking::OnMessage(CGUIMessage& message)
{
	switch ( message.GetMessage() )
  {
    case GUI_MSG_WINDOW_INIT:
    {
			CGUIDialog::OnMessage(message);		
			m_iSelectedFile=-1;
			m_iFrames=0;
			
			// enable the CD's
			for (int i=1;  i <= m_iNumberOfFiles; ++i)
			{
				CONTROL_ENABLE(GetID(),i);
				SET_CONTROL_VISIBLE(GetID(),i);
			}

			// disable CD's we dont use
			for (int i=m_iNumberOfFiles+1;  i <= 5; ++i)
			{
				SET_CONTROL_HIDDEN(GetID(), i);
				CONTROL_DISABLE(GetID(), i);
			}
			return true;
		}
		break;

    case GUI_MSG_CLICKED:
    {
			m_iSelectedFile=message.GetSenderId();			
			Close();
			return true;
		}
		break;
	}
	return CGUIDialog::OnMessage(message);
}

int CGUIDialogFileStacking::GetSelectedFile() const
{
	return m_iSelectedFile;
}
void CGUIDialogFileStacking::SetNumberOfFiles(int iFiles)
{
	m_iNumberOfFiles=iFiles;
}


void CGUIDialogFileStacking::Render()
{
	if (m_iFrames <=25)
	{
		// slide in...
		int dwScreenWidth=g_graphicsContext.GetWidth();
		for (int i=1;  i <= m_iNumberOfFiles; ++i)
		{
			CGUIControl* pControl=(CGUIControl*)GetControl(i);
			DWORD dwEndPos     = dwScreenWidth - ((m_iNumberOfFiles-i)*32)-140;
			DWORD dwStartPos   = dwScreenWidth;
			float fStep= (float)(dwStartPos - dwEndPos);
			fStep/=25.0f;
      fStep*=(float)m_iFrames;
			DWORD dwPosX = (DWORD) ( ((float)dwStartPos)-fStep );
			pControl->SetPosition( dwPosX, pControl->GetYPosition() );
		}
		m_iFrames++;
	}
	CGUIDialog::Render();
}