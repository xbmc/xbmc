
#include "stdafx.h"
#include "guiDialogInvite.h"
#include "guiWindowManager.h"
#include "GUISpinControl.h"
#include "GUILabelControl.h"
#include "GUIButtonControl.h"
#include "localizeStrings.h"
#include <vector>
using namespace std;

#define CTL_LABEL_CAPTION	1
#define CTL_LABEL_SCHEDULE	2
#define CTL_LABEL_GAME		3
#define CTL_LABEL_DATE		4
#define CTL_LABEL_TIME		5
#define CTL_LABEL_CONFIRM	6
#define CTL_LABEL_CONFIRM2	7
#define CTL_BUTTON_SEND		10
#define CTL_BUTTON_CANCEL	11
#define CTL_SPIN_GAMES		30


CGUIDialogInvite::CGUIDialogInvite(void)
:CGUIDialog(0)
{
	m_bConfirmed=false;
}

CGUIDialogInvite::~CGUIDialogInvite(void)
{
}

void CGUIDialogInvite::OnInitWindow()
{
	CGUISpinControl& spin_control	= *((CGUISpinControl*)GetControl(CTL_SPIN_GAMES));
	CGUILabelControl& label_caption = *((CGUILabelControl*)GetControl(CTL_LABEL_CAPTION));
	CGUILabelControl& label_schedul = *((CGUILabelControl*)GetControl(CTL_LABEL_SCHEDULE));
	CGUILabelControl& label_game	= *((CGUILabelControl*)GetControl(CTL_LABEL_GAME));
	CGUILabelControl& label_date	= *((CGUILabelControl*)GetControl(CTL_LABEL_DATE));
	CGUILabelControl& label_time	= *((CGUILabelControl*)GetControl(CTL_LABEL_TIME));
	CGUILabelControl& label_confirm	= *((CGUILabelControl*)GetControl(CTL_LABEL_CONFIRM));
	CGUILabelControl& label_confirm2= *((CGUILabelControl*)GetControl(CTL_LABEL_CONFIRM2));
	CGUIButtonControl& button_send	= *((CGUIButtonControl*)GetControl(CTL_BUTTON_SEND));
	CGUIButtonControl& button_cancel= *((CGUIButtonControl*)GetControl(CTL_BUTTON_CANCEL));

	// Initialise Labels;
	label_caption.SetText("INVITE");
	label_schedul.SetText("Schedule a game and time:");
	label_game.SetText("GAME:");
	label_date.SetText("TIME:");
	label_time.SetText("DATE:");
	label_confirm.SetText("Please confirm these details and select");
	label_confirm2.SetText("Send to continue.");
	button_send.SetText("Send");
	button_cancel.SetText("Cancel");

	// Initialise Spin Control
	if (m_pGames)
	{
		spin_control.Clear();

		CGUIList::GUILISTITEMS& list = m_pGames->Lock();

		for (int i=0; i < (int) list.size(); ++i)
		{
			spin_control.AddLabel(list[i]->GetName(), i);
		}

		m_pGames->Release();

		spin_control.SetNonProportional(true);
		spin_control.SetValue(0);
	}
}

bool CGUIDialogInvite::OnMessage(CGUIMessage& message)
{
	CGUIDialog::OnMessage(message);

	switch ( message.GetMessage() )
	{
		case GUI_MSG_CLICKED:
		{
			int iControl=message.GetSenderId();
		
			switch (iControl)
			{
				case CTL_BUTTON_SEND:
					m_bConfirmed = true;
					Close();
					break;

				case CTL_BUTTON_CANCEL:
					m_bConfirmed = false;
					Close();
					break;
			}
		}
		break;
	}

	return true;
}

void CGUIDialogInvite::SetGames(CGUIList* aGamesList)
{
	m_pGames = aGamesList;
}

bool CGUIDialogInvite::IsConfirmed() const
{
	return m_bConfirmed;
}