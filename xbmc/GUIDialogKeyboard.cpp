
#include "stdafx.h"
#include "guiWindowManager.h"
#include "guiDialogKeyboard.h"
#include "guiLabelControl.h"
#include "guiButtonControl.h"
#include "localizeStrings.h"
#include <vector>
using namespace std;

#define CTL_BUTTON_DONE		300
#define CTL_BUTTON_CANCEL	301
#define CTL_BUTTON_SHIFT	302
#define CTL_BUTTON_CAPS		303
#define CTL_BUTTON_SYMBOLS	304

#define CTL_LABEL_EDIT		310

#define CTL_BUTTON_BACKSPACE  8

CGUIDialogKeyboard::CGUIDialogKeyboard(void) : CGUIDialog(0)
{
	m_bDirty = false;
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
	for (iButton=65; iButton<=90; iButton++)
	{
		CGUIButtonControl* pButton = ((CGUIButtonControl*)GetControl(iButton));
		
		if(pButton)
		{
			aLabel[0]=iButton;
			pButton->SetText(aLabel);
		}
	}

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
					break;
				case CTL_BUTTON_CAPS:
					break;
				case CTL_BUTTON_SYMBOLS:
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
	}
}

void CGUIDialogKeyboard::Character(WCHAR wch)
{
	CGUILabelControl* pEdit = ((CGUILabelControl*)GetControl(CTL_LABEL_EDIT));
	if (pEdit)
	{
		m_bDirty = true;
		wstring label = pEdit->GetLabel();
		label+=wch;
		pEdit->SetText(label);
	}
}

void CGUIDialogKeyboard::Backspace()
{
	CGUILabelControl* pEdit = ((CGUILabelControl*)GetControl(CTL_LABEL_EDIT));
	if (pEdit)
	{
		wstring label = pEdit->GetLabel();
		int characters = label.length();
		if(characters>0)
		{
			m_bDirty = true;
			label[characters-1]=(WCHAR)0;
			pEdit->SetText(label);
		}
	}
}

void CGUIDialogKeyboard::OnClickButton(int iButtonControl)
{
	bool bChar = ((	(iButtonControl>=48) && (iButtonControl<=57))	||
				  (	(iButtonControl>=65) && (iButtonControl<=90))	||
				    (iButtonControl==32)							);

	if (bChar)
	{
		Character((WCHAR)iButtonControl);
	}
	else if (iButtonControl==CTL_BUTTON_BACKSPACE)
	{
		Backspace();
	}
}