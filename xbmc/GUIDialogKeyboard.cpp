
#include "stdafx.h"
#include "guiWindowManager.h"
#include "guiDialogKeyboard.h"
#include "guiLabelControl.h"
#include "guiButtonControl.h"
#include "localizeStrings.h"
#include <vector>
using namespace std;

// TODO: Add support for symbols.

#define CTL_BUTTON_DONE		300
#define CTL_BUTTON_CANCEL	301
#define CTL_BUTTON_SHIFT	302
#define CTL_BUTTON_CAPS		303
#define CTL_BUTTON_SYMBOLS	304
#define CTL_BUTTON_LEFT		305
#define CTL_BUTTON_RIGHT	306

#define CTL_LABEL_EDIT		310

#define CTL_BUTTON_BACKSPACE  8

CGUIDialogKeyboard::CGUIDialogKeyboard(void) : CGUIDialog(0)
{
	m_bDirty = false;
	m_bShift = false;
	m_bCapsLock = false;
}

CGUIDialogKeyboard::~CGUIDialogKeyboard(void)
{
}

void CGUIDialogKeyboard::OnInitWindow()
{
	char szLabel[2];
	szLabel[0]=32;
	szLabel[1]=0;

	CStdString aLabel = szLabel;
	int iButton;

	// set numerals
	for (iButton=48; iButton<=57; iButton++)
	{
		CGUIButtonControl* pButton = ((CGUIButtonControl*)GetControl(iButton));
		
		if(pButton)
		{
			aLabel[0]=iButton;
			pButton->SetText(aLabel);
		}
	}

	// set alphabetic (capitals)
	UpdateButtons();

	CGUILabelControl* pEdit = ((CGUILabelControl*)GetControl(CTL_LABEL_EDIT));
	if (pEdit)
	{
		pEdit->ShowCursor();
	}
}

void CGUIDialogKeyboard::OnAction(const CAction &action)
{
	if (action.wID == ACTION_CLOSE_DIALOG || action.wID == ACTION_PREVIOUS_MENU)
	{
		m_bDirty = false;
		Close();
	}
	else
	{
		CGUIWindow::OnAction(action);
	}
}

bool CGUIDialogKeyboard::OnMessage(CGUIMessage& message)
{
	CGUIDialog::OnMessage(message);

	switch ( message.GetMessage() )
	{
		case GUI_MSG_CLICKED:
		{
			int iControl=message.GetSenderId();
		
			switch (iControl)
			{
				case CTL_BUTTON_DONE:
				{
					CGUILabelControl* pEdit = ((CGUILabelControl*)GetControl(CTL_LABEL_EDIT));

					if (pEdit)
					{
						m_strEdit = pEdit->GetLabel();
					}

					Close();
					break;
				}
				case CTL_BUTTON_CANCEL:
					m_bDirty = false;
					Close();
					break;
				case CTL_BUTTON_SHIFT:
					m_bShift = !m_bShift;
					UpdateButtons();
					break;
				case CTL_BUTTON_CAPS:
					m_bCapsLock = !m_bCapsLock;
					UpdateButtons();
					break;
				case CTL_BUTTON_SYMBOLS:
					break;
				case CTL_BUTTON_LEFT:
					{
						CGUILabelControl* pEdit = ((CGUILabelControl*)GetControl(CTL_LABEL_EDIT));
						if (pEdit)
						{
							pEdit->SetCursorPos(pEdit->GetCursorPos()-1);
						}
					}
					break;
				case CTL_BUTTON_RIGHT:
					{
						CGUILabelControl* pEdit = ((CGUILabelControl*)GetControl(CTL_LABEL_EDIT));
						if (pEdit)
						{
							pEdit->SetCursorPos(pEdit->GetCursorPos()+1);
						}
					}
					break;
				default:
					OnClickButton(iControl);
					break;
			}
		}
		break;
	}

	return true;
}

void CGUIDialogKeyboard::SetText(CStdString& aTextString)
{
	m_strEdit = aTextString;
	m_bDirty = false;

	CGUILabelControl* pEdit = ((CGUILabelControl*)GetControl(CTL_LABEL_EDIT));
	if (pEdit)
	{
		pEdit->SetText(aTextString);
		// Set the cursor position to the end of the edit control
		if (pEdit->GetCursorPos() >= 0)
		{
			pEdit->SetCursorPos(aTextString.length());
		}
	}
}

void CGUIDialogKeyboard::Character(WCHAR wch)
{
	CGUILabelControl* pEdit = ((CGUILabelControl*)GetControl(CTL_LABEL_EDIT));
	if (pEdit)
	{
		m_bDirty = true;
		CStdString strLabel = pEdit->GetLabel();
		int iPos = pEdit->GetCursorPos();
		strLabel.Insert(iPos, (TCHAR)wch);
		pEdit->SetText(strLabel);
		pEdit->SetCursorPos(iPos+1);
	}
}

void CGUIDialogKeyboard::Backspace()
{
	CGUILabelControl* pEdit = ((CGUILabelControl*)GetControl(CTL_LABEL_EDIT));
	if (pEdit)
	{
		CStdString strLabel = pEdit->GetLabel();
		if (pEdit->GetCursorPos()>0)
		{
			m_bDirty = true;
			strLabel.erase(pEdit->GetCursorPos()-1,1);
			pEdit->SetText(strLabel);
			pEdit->SetCursorPos(pEdit->GetCursorPos()-1);
		}
	}
}

void CGUIDialogKeyboard::OnClickButton(int iButtonControl)
{
	if ( ((iButtonControl>=48) && (iButtonControl<=57)) || (iButtonControl==32) )
	{	// number or space
		Character((WCHAR)iButtonControl);
	}
	else if ((iButtonControl>=65) && (iButtonControl<=90))
	{	// letter
		Character(GetCharacter(iButtonControl));
	}
	else if (iButtonControl==CTL_BUTTON_BACKSPACE)
	{
		Backspace();
	}
}

WCHAR CGUIDialogKeyboard::GetCharacter(int iButton)
{
	if ((m_bCapsLock && m_bShift) || (!m_bCapsLock && !m_bShift))
	{	// make lower case
		iButton += 32;
	}
	if (m_bShift)
	{	// turn off the shift key
		m_bShift = false;
		UpdateButtons();
	}
	return (WCHAR) iButton;
}

void CGUIDialogKeyboard::UpdateButtons()
{
	if (m_bShift)
	{	// show the button depressed
		CGUIMessage msg(GUI_MSG_SELECTED, GetID(), CTL_BUTTON_SHIFT);
		OnMessage(msg);
	}
	else
	{
		CGUIMessage msg(GUI_MSG_DESELECTED, GetID(), CTL_BUTTON_SHIFT);
		OnMessage(msg);
	}
	if (m_bCapsLock)
	{
		CGUIMessage msg(GUI_MSG_SELECTED, GetID(), CTL_BUTTON_CAPS);
		OnMessage(msg);
	}
	else
	{
		CGUIMessage msg(GUI_MSG_DESELECTED, GetID(), CTL_BUTTON_CAPS);
		OnMessage(msg);
	}

	// set correct alphabet characters...
	char szLabel[2];
	szLabel[0]=32;
	szLabel[1]=0;
	CStdString aLabel = szLabel;

	for (int iButton=65; iButton<=90; iButton++)
	{
		CGUIButtonControl* pButton = ((CGUIButtonControl*)GetControl(iButton));
		
		if(pButton)
		{
			// set the correct case...
			if ((m_bCapsLock && m_bShift) || (!m_bCapsLock && !m_bShift))
			{	// make lower case
				aLabel[0] = iButton + 32;
			}
			else
			{
				aLabel[0] = iButton;
			}
			pButton->SetText(aLabel);
		}
	}
}
