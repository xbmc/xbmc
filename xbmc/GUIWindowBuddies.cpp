
#include "stdafx.h"
#include "Application.h"
#include "GUIUserMessages.h"
#include "GUIWindowBuddies.h"
#include "settings.h"
#include "guiWindowManager.h"
#include "GUIDialogInvite.h"
#include "GUIDialogProgress.h"
#include "GUIDialogYesNo.h"
#include "localizestrings.h"
#include "util.h"
#include <algorithm>
#include "utils/log.h"

#define CONTROL_BTNMODE			12				// Games Button
#define CONTROL_BTNJOIN			13				// Join Button
#define CONTROL_BTNSPEEX		16				// Speex Button
#define CONTROL_BTNINVITE		14				// Invite Button
#define CONTROL_BTNREMOVE		15				// Remove Button

#define CONTROL_LISTEX			31
#define CONTROL_BTNPLAY			33				// Play Button
#define CONTROL_BTNHOST			35				// Host Button
#define CONTROL_BTNADD			34				// Add Button

#define CONTROL_LABELBUDDYWIN	50
#define CONTROL_LABELUSERNAME	51				// Xlink Kai Username
#define CONTROL_LABELUPDATED	52				// Last update time
#define CONTROL_IMAGELOGO		53				// Xlink Kai Logo

#define CONTROL_IMAGEBUDDYICON1	60				// Buddy (offline) icon
#define CONTROL_IMAGEBUDDYICON2	61				// Buddy (online) icon
#define CONTROL_IMAGEOPPONENT	62				// Opponent icon
#define CONTROL_IMAGEARENA		63				// Arena icon

#define CONTROL_LABELBUDDYNAME	70				// Buddy Name
#define CONTROL_LABELBUDDYSTAT	71				// Buddy Game Status
#define CONTROL_LABELBUDDYINVT	72				// Buddy Invite Status
#define CONTROL_LABELPLAYERCNT	73				// Arena Player Count

#define SET_CONTROL_DISABLED(dwSenderId, dwControlID) \
{ \
	CGUIMessage msg(GUI_MSG_DISABLED, dwSenderId, dwControlID); \
	g_graphicsContext.SendMessage(msg); \
}

#define SET_CONTROL_ENABLED(dwSenderId, dwControlID) \
{ \
	CGUIMessage msg(GUI_MSG_ENABLED, dwSenderId, dwControlID); \
	g_graphicsContext.SendMessage(msg); \
}

#define SET_CONTROL_SELECTED(dwSenderId, dwControlID, bSelect) \
{ \
	CGUIMessage msg(bSelect?GUI_MSG_SELECTED:GUI_MSG_DESELECTED, dwSenderId, dwControlID); \
	g_graphicsContext.SendMessage(msg); \
}


bool CGUIWindowBuddies::SortArenaItems(CGUIItem* pStart, CGUIItem* pEnd)
{
	CGUIItem& rpStart	=*pStart;
	CGUIItem& rpEnd		=*pEnd;

	if ( rpStart.GetCookie() != rpEnd.GetCookie() )
	{
		return ( rpStart.GetCookie() == CKaiClient::Item::Arena );
	}

	return ( rpStart.GetName().CompareNoCase( rpEnd.GetName() ) < 0 );
}

bool CGUIWindowBuddies::SortFriends(CGUIItem* pStart, CGUIItem* pEnd)
{
	CBuddyItem& rpStart=* ((CBuddyItem*)pStart);
	CBuddyItem& rpEnd=*		((CBuddyItem*)pEnd);

	if (rpStart.m_bIsOnline != rpEnd.m_bIsOnline)
	{
		return rpStart.m_bIsOnline;
	}

	return ( rpStart.GetName().CompareNoCase( rpEnd.GetName() ) < 0 );
}

CGUIWindowBuddies::CGUIWindowBuddies(void)
:CGUIWindow(0)
{
	m_pKaiClient = NULL;
	m_pOpponentImage = NULL; 
	m_pCurrentAvatar = NULL;
	m_dwGamesUpdateTimer = 0;

	m_friends.SetSortingAlgorithm(CGUIWindowBuddies::SortFriends);
	m_arena.SetSortingAlgorithm(CGUIWindowBuddies::SortArenaItems);
	m_games.SetSortingAlgorithm(CGUIWindowBuddies::SortArenaItems);

	ON_CLICK_MESSAGE(CONTROL_BTNMODE,	CGUIWindowBuddies, OnClickModeButton);
	ON_CLICK_MESSAGE(CONTROL_BTNADD,	CGUIWindowBuddies, OnClickAddButton);
	ON_CLICK_MESSAGE(CONTROL_BTNREMOVE,	CGUIWindowBuddies, OnClickRemoveButton);
	ON_CLICK_MESSAGE(CONTROL_BTNSPEEX,	CGUIWindowBuddies, OnClickSpeexButton);
	ON_CLICK_MESSAGE(CONTROL_BTNINVITE,	CGUIWindowBuddies, OnClickInviteButton);
	ON_CLICK_MESSAGE(CONTROL_BTNJOIN,	CGUIWindowBuddies, OnClickJoinButton);
	ON_CLICK_MESSAGE(CONTROL_BTNPLAY,	CGUIWindowBuddies, OnClickPlayButton);
	ON_CLICK_MESSAGE(CONTROL_BTNHOST,	CGUIWindowBuddies, OnClickHostButton);
	ON_CLICK_MESSAGE(CONTROL_LISTEX,	CGUIWindowBuddies, OnClickListItem);
	ON_SELECTED_MESSAGE(CONTROL_LISTEX,	CGUIWindowBuddies, OnSelectListItem);
}

CGUIWindowBuddies::~CGUIWindowBuddies(void)
{
}

void CGUIWindowBuddies::OnInitWindow()
{
	SET_CONTROL_LABEL(GetID(),  CONTROL_LABELUSERNAME,  g_stSettings.szOnlineUsername);
	SET_CONTROL_LABEL(GetID(),  CONTROL_LABELUPDATED,   "");
	SET_CONTROL_LABEL(GetID(),  CONTROL_LABELBUDDYNAME, "");
	SET_CONTROL_LABEL(GetID(),  CONTROL_LABELBUDDYSTAT, "");
	SET_CONTROL_LABEL(GetID(),  CONTROL_LABELBUDDYINVT, "");
	SET_CONTROL_LABEL(GetID(),  CONTROL_LABELPLAYERCNT, "");

	SET_CONTROL_DISABLED(GetID(), CONTROL_BTNHOST);

	if (window_state==State::Uninitialized)
	{
		// bind this image to the control defined in the xml, later we'll use this
		// as a template to determine the size and position of avatars
		m_pOpponentImage = (CGUIImage*)GetControl(CONTROL_IMAGEOPPONENT);	

		// set buddy status icons
		CBuddyItem::SetIcons(12,12,	"buddyitem-headset.png",
									"buddyitem-chat.png",
									"buddyitem-ping.png",
									"buddyitem-invite.png",
									"buddyitem-play.png");

		CArenaItem::SetIcons(12,12, "arenaitem-private.png");

		ChangeState(State::Buddies);

		CGUIMessage msgb(GUI_MSG_LABEL_BIND,GetID(),CONTROL_LISTEX,0,0,&m_friends);
		g_graphicsContext.SendMessage(msgb);

		QueryInstalledGames();
		//g_VoiceManager.SetVoiceThroughSpeakers(true);
	}
}

// Called just as soon as this window has be assigned to the CKaiClient as an observer
void CGUIWindowBuddies::OnInitialise(CKaiClient* pClient)
{
	m_pKaiClient = pClient;
}

CBuddyItem* CGUIWindowBuddies::GetBuddySelection()
{
	CGUIMessage msg(GUI_MSG_ITEM_SELECTED,GetID(),CONTROL_LISTEX,0,0,NULL);
	g_graphicsContext.SendMessage(msg);         
	//return dynamic_cast<CBuddyItem*>((CGUIListExItem*)msg.GetLPVOID());

	CGUIListExItem* pItem = (CGUIListExItem*)msg.GetLPVOID();
	if (pItem==NULL)
		return NULL;

	return	(pItem->GetCookie()==CKaiClient::Item::Player) ?
			(CBuddyItem*)pItem : NULL;
}

CArenaItem* CGUIWindowBuddies::GetArenaSelection()
{
	CGUIMessage msg(GUI_MSG_ITEM_SELECTED,GetID(),CONTROL_LISTEX,0,0,NULL);
	g_graphicsContext.SendMessage(msg);         

	CGUIListExItem* pItem = (CGUIListExItem*)msg.GetLPVOID();
	if (pItem==NULL)
		return NULL;

	return	(pItem->GetCookie()==CKaiClient::Item::Arena) ?
			(CArenaItem*)pItem : NULL;
}

void CGUIWindowBuddies::OnClickModeButton(CGUIMessage& aMessage)
{
	ChangeState();
}

void CGUIWindowBuddies::OnClickAddButton(CGUIMessage& aMessage)
{
	CBuddyItem* pBuddy;
	if ( (pBuddy = GetBuddySelection()) )
	{
		CStdString contact = pBuddy->GetName();
		m_pKaiClient->AddContact(contact);
	}
}

void CGUIWindowBuddies::OnClickRemoveButton(CGUIMessage& aMessage)
{
	CBuddyItem* pBuddy;
	if ( (pBuddy = GetBuddySelection()) )
	{
		CStdString contact = pBuddy->GetName();
		m_pKaiClient->RemoveContact(contact);
	}
}

void CGUIWindowBuddies::OnClickSpeexButton(CGUIMessage& aMessage)
{
	CBuddyItem* pBuddy;
	if ( (pBuddy = GetBuddySelection()) )
	{
		m_pKaiClient->EnableContactVoice(pBuddy->GetName(),!pBuddy->m_bSpeex);
	}
}

void CGUIWindowBuddies::OnClickInviteButton(CGUIMessage& aMessage)
{
	CBuddyItem* pBuddy;
	if ( (pBuddy = GetBuddySelection()) )
	{
		CGUIDialogInvite& dialog = *((CGUIDialogInvite*)m_gWindowManager.GetWindow(WINDOW_DIALOG_INVITE));
		dialog.SetGames(&m_games);
		dialog.DoModal(GetID());
		dialog.Close();
		
		if (dialog.IsConfirmed())
		{
			CStdString aVector;
			if (dialog.GetSelectedVector(aVector))
			{
				CStdString strMessage = "";
				dialog.GetPersonalMessage(strMessage);
				m_pKaiClient->Invite(pBuddy->GetName(),aVector,strMessage);
			}
		}
	}
}

void CGUIWindowBuddies::OnClickJoinButton(CGUIMessage& aMessage)
{
	CBuddyItem* pBuddy;
	if ( (pBuddy = GetBuddySelection()) )
	{
		ChangeState(State::Arenas);
		SET_CONTROL_FOCUS(GetID(),  CONTROL_LISTEX, 0);
		CStdString strPassword = "";	
		m_pKaiClient->EnterVector(pBuddy->m_strVector,strPassword);
	}
}

void CGUIWindowBuddies::OnClickPlayButton(CGUIMessage& aMessage)
{
	switch (window_state)
	{
		case State::Arenas:
		{
			Play( m_pKaiClient->GetCurrentVector() );
			break;
		}
		case State::Games:
		{
			CArenaItem* pArena;
			if ( (pArena = GetArenaSelection()) )
			{
				Enter(*pArena);
			}
			else
			{
				Play( m_pKaiClient->GetCurrentVector() );
			}
			break;
		}
	}
}

void CGUIWindowBuddies::OnClickHostButton(CGUIMessage& aMessage)
{
	CStdString strDescription = "";
	CStdString strPrompt = "Please choose a description for your arena.";

	if (CGUIDialogKeyboard::ShowAndGetInput(strDescription, strPrompt, false))
	{
		CStdString strPassword = "";
		int nPlayerLimit = 12;
		m_pKaiClient->Host(strPassword,nPlayerLimit,strDescription);
	}
}


void CGUIWindowBuddies::OnClickListItem(CGUIMessage& aMessage)
{
	CArenaItem* pArena;
	if ( (pArena = GetArenaSelection()) )
	{
		Enter(*pArena);
	}

	CBuddyItem* pBuddy;
	if ( window_state == State::Buddies && (pBuddy = GetBuddySelection()) )
	{
		// if we clicked on a buddy and we're in Friends mode
		Invitation& invite = m_invitations[pBuddy->GetName()];
		if (invite.vector.length()>0 && pBuddy->m_bInvite)
		{
			// stop flashing envelope icon
			pBuddy->m_bInvite = FALSE;

			// if that buddy had sent us an invite
			CStdString strAccepted="Invite accepted!";
			CStdString strDate;
			CStdString strFrom;
			strDate.Format("Date: %s",invite.time);
			strFrom.Format("From: %s",pBuddy->GetName());

			if (invite.message.Compare(strAccepted)==0)
			{
				// if that invite was actually a receipt of acceptance, display it
				CGUIDialogOK* pDialog = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
				pDialog->SetHeading("Invitation");
				pDialog->SetLine(0,strDate);
				pDialog->SetLine(1,strFrom);
				pDialog->SetLine(2,invite.message);

				pDialog->DoModal(GetID());				
			}
			else
			{
				// if this is an invite, display it
				CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);
				pDialog->SetHeading("Invitation");
				pDialog->SetLine(0,strDate);
				pDialog->SetLine(1,strFrom);

				CStdString strMsg = invite.message;

				if (strMsg.length()==0)
				{
					CStdString strGame;
					CArenaItem::GetTier(CArenaItem::Tier::Game,invite.vector,strGame);
					if (strGame.length()>0)
					{
						strMsg.Format("You've been invited to play %s.",strGame);
					}
					else
					{
						strMsg.Format("You've been invited to join %s.",pBuddy->GetName()); 
					}
				}		

				pDialog->SetLine(2,strMsg);

				pDialog->DoModal(GetID());

				if (pDialog->IsConfirmed())
				{
					// if the user accepted the invitation, send a reciept
					m_pKaiClient->Invite(pBuddy->GetName(),invite.vector,strAccepted);

					// join the arena
					ChangeState(State::Arenas);
					CStdString strPassword = "";
					m_pKaiClient->EnterVector(invite.vector, strPassword );
				}
			}

			// erase the invite
			invite.vector = "";
		}
	}
}

void CGUIWindowBuddies::OnSelectListItem(CGUIMessage& aMessage)
{
}



CGUIImage* CGUIWindowBuddies::GetCurrentAvatar()
{
	CKaiItem* pItem;

	if ( (pItem = GetBuddySelection()) || (pItem = GetArenaSelection()) )
	{
		// a item is selected, and his avatar is different to the one we last showed
		if (pItem->m_pAvatar != m_pCurrentAvatar)
		{
			OutputDebugString("Changing avatar\r\n");

			// if we actually last showed an avatar free up its resources
			if (m_pCurrentAvatar)
			{
				m_pCurrentAvatar->FreeResources();
				m_pCurrentAvatar = NULL;
			}

			// if the item actually has an avatar
			if (pItem->m_pAvatar)
			{
				// allocate some resources and make it our current avatar
				pItem->m_pAvatar->AllocResources();
				m_pCurrentAvatar = pItem->m_pAvatar;

				if (m_pOpponentImage)
				{
					int x = m_pOpponentImage->GetXPosition();
					int y = m_pOpponentImage->GetYPosition();
					DWORD w = m_pOpponentImage->GetWidth();
					DWORD h = m_pOpponentImage->GetHeight();
					m_pCurrentAvatar->SetPosition(x,y);
					m_pCurrentAvatar->SetWidth((int)w);
					m_pCurrentAvatar->SetHeight((int)h);
				}
			}

			OutputDebugString("Avatar changed.\r\n");
		}
	}
	else
	{
		// no item is selected so free up the current avatar if we have one
		if (m_pCurrentAvatar)
		{
			OutputDebugString("Deallocating current avatar.\r\n");
			m_pCurrentAvatar->FreeResources();
			m_pCurrentAvatar = NULL;
			OutputDebugString("Avatar dellocated.\r\n");
		}
	}

	return m_pCurrentAvatar;
}





void CGUIWindowBuddies::OnAction(const CAction &action)
{
	switch(action.wID)
	{
		case ACTION_PARENT_DIR:
		{
			switch (window_state)
			{
				case State::Arenas:
				{
					m_pKaiClient->ExitVector();
					break;
				}
			}
			break;
		}
		case ACTION_PREVIOUS_MENU:
		{
			m_gWindowManager.PreviousWindow();
			break;
		}
		default:
		{
			CGUIWindow::OnAction(action);
			break;
		}
	}
}

void CGUIWindowBuddies::UpdatePanel()
{
	CBuddyItem* pBuddy = GetBuddySelection();

	SET_CONTROL_DISABLED(GetID(), CONTROL_BTNJOIN);	
	SET_CONTROL_DISABLED(GetID(), CONTROL_BTNSPEEX);	
	SET_CONTROL_DISABLED(GetID(), CONTROL_BTNINVITE);	
	SET_CONTROL_DISABLED(GetID(), CONTROL_BTNREMOVE);	
	SET_CONTROL_DISABLED(GetID(), CONTROL_BTNADD);	

	if (pBuddy!=NULL)
	{
		SET_CONTROL_ENABLED(GetID(), CONTROL_BTNADD);
		SET_CONTROL_ENABLED(GetID(), CONTROL_BTNREMOVE);
		SET_CONTROL_ENABLED(GetID(), CONTROL_BTNSPEEX);

		if (pBuddy->m_bIsOnline)
		{
			SET_CONTROL_ENABLED(GetID(), CONTROL_BTNINVITE);
			if (pBuddy->m_strVector!="Idle")
			{
				SET_CONTROL_ENABLED(GetID(), CONTROL_BTNJOIN);
			}
		}
	}

	CStdString strEmpty = "";
	SET_CONTROL_LABEL(GetID(), CONTROL_LABELBUDDYNAME, strEmpty);
	SET_CONTROL_LABEL(GetID(), CONTROL_LABELBUDDYSTAT, strEmpty);
}


void CGUIWindowBuddies::Render()
{
	CGUIWindow::Render();

	m_dwGamesUpdateTimer++;

	// Every 5 minutes
	if(m_dwGamesUpdateTimer%15000==0)
	{
		// Update the game list player count.
		UpdateGamesPlayerCount();
	}

	// Update buttons
	UpdatePanel();

	// Request buddy selection if needed
	CBuddyItem* pBuddy = GetBuddySelection();
	if (pBuddy)
	{
		BOOL bItemFocusedLongEnough = (pBuddy->GetFramesFocused() > 150);

		if (pBuddy && !pBuddy->m_bProfileRequested)
		{
			pBuddy->m_bProfileRequested = TRUE;
			CStdString aName = pBuddy->GetName();

			m_pKaiClient->QueryUserProfile(aName);
			OutputDebugString("Requested user profile");

			m_pKaiClient->QueryAvatar(aName);
			OutputDebugString(" and avatar");

			CStdString strDebug;
			strDebug.Format(" for %s.\r\n",aName.c_str());
			OutputDebugString(strDebug.c_str());
		}
	}

	// Update current Arena name
	CStdString arenaName="";
	SET_CONTROL_LABEL(GetID(),  CONTROL_LABELUPDATED, arenaName);

	// Update mode/item selection specific labels
	switch(window_state)
	{
		case State::Buddies:
		{
			UpdateFriends();
			break;
		}
		case State::Arenas:
		{
			CStdString currentVector = m_pKaiClient->GetCurrentVector();
			INT arenaDelimiter = currentVector.ReverseFind('/')+1;
			arenaName = currentVector.Mid(arenaDelimiter);
			SET_CONTROL_LABEL(GetID(),  CONTROL_LABELUPDATED, arenaName);
		}
		case State::Games:
		{
			UpdateArena();
			break;
		}
	}
}

void CGUIWindowBuddies::UpdateFriends()
{
	CBuddyItem* pBuddy = GetBuddySelection();

	SET_CONTROL_HIDDEN(GetID(), CONTROL_IMAGEOPPONENT);
	SET_CONTROL_HIDDEN(GetID(), CONTROL_IMAGEARENA);
	SET_CONTROL_HIDDEN(GetID(), CONTROL_LABELBUDDYNAME);
	SET_CONTROL_HIDDEN(GetID(), CONTROL_LABELBUDDYSTAT);

	if (pBuddy!=NULL)
	{
		CStdString strName		= pBuddy->GetName();
		CStdString strLocation	=  pBuddy->m_strGeoLocation;
		CStdString strStatus	= "Offline";

		if (pBuddy->m_bIsOnline)
		{
			strStatus = pBuddy->GetArena();
		}

		CStdString strNameAndStatus;
		strNameAndStatus.Format("%s [%s]", strName, strStatus);

		SET_CONTROL_LABEL(GetID(),	 CONTROL_LABELBUDDYNAME, strNameAndStatus);
		SET_CONTROL_LABEL(GetID(),	 CONTROL_LABELBUDDYSTAT, strLocation);
		SET_CONTROL_VISIBLE(GetID(), CONTROL_LABELBUDDYNAME);
		SET_CONTROL_VISIBLE(GetID(), CONTROL_LABELBUDDYSTAT);

		if ( GetCurrentAvatar() )
		{
			m_pCurrentAvatar->Render();
		}
		else
		{
			SET_CONTROL_VISIBLE(GetID(), CONTROL_IMAGEOPPONENT);
		}
	}
}

void CGUIWindowBuddies::UpdateArena()
{
	CArenaItem* pArena = GetArenaSelection();

	SET_CONTROL_HIDDEN(GetID(), CONTROL_IMAGEOPPONENT);
	SET_CONTROL_HIDDEN(GetID(), CONTROL_IMAGEARENA);
	
	// If item selected is an Arena
	if (pArena!=NULL)
	{
		// Update selected Arena name
		CStdString strDisplayText;
		pArena->GetDisplayText(strDisplayText);
		SET_CONTROL_LABEL(GetID(), CONTROL_LABELBUDDYNAME, strDisplayText);

		// Update selected Arena category
		CStdString strDescription;

		switch (pArena->GetTier())
		{
			case CArenaItem::Tier::Custom:
				if (pArena->m_bIsPersonal && pArena->m_strDescription.length()>0)
				{
					strDescription = pArena->m_strDescription;
				}
				else
				{
					pArena->GetTier(CArenaItem::Tier::Game,strDescription);
				}
				break;
			case CArenaItem::Tier::Platform:
				break;
			default:
				pArena->GetTier(CArenaItem::Tier::Platform,strDescription);
				strDescription = strDescription.ToUpper();
				break;
		}

		// Update selected Arena icon
		SET_CONTROL_LABEL(GetID(),	CONTROL_LABELBUDDYSTAT, strDescription.c_str());
		SET_CONTROL_VISIBLE(GetID(), CONTROL_LABELBUDDYSTAT);

		if ( GetCurrentAvatar() )
		{
			m_pCurrentAvatar->Render();
		}
		else
		{
			SET_CONTROL_VISIBLE(GetID(), CONTROL_IMAGEARENA);
		}
	}
	else 
	{
		CBuddyItem* pBuddy = GetBuddySelection();

		// if item selected is an opponent
		if (pBuddy!=NULL)
		{
			CStdString opponentInfo;
			opponentInfo.Format("%s (%ums)",pBuddy->GetName().c_str(),pBuddy->m_dwPing);

			SET_CONTROL_LABEL(GetID(), CONTROL_LABELBUDDYNAME, opponentInfo);

			if (pBuddy->m_strGeoLocation.length())
			{
				SET_CONTROL_LABEL(GetID(), CONTROL_LABELBUDDYSTAT, pBuddy->m_strGeoLocation);
				SET_CONTROL_VISIBLE(GetID(), CONTROL_LABELBUDDYSTAT);
			}
			else
			{
				SET_CONTROL_HIDDEN(GetID(), CONTROL_LABELBUDDYSTAT);
			}

			if ( GetCurrentAvatar() )
			{
				m_pCurrentAvatar->Render();
			}
			else
			{
				SET_CONTROL_VISIBLE(GetID(), CONTROL_IMAGEOPPONENT);
			}
		}
	}
}

void CGUIWindowBuddies::ChangeState()
{
	switch(window_state)
	{
		case State::Buddies:
		{
			ChangeState(State::Games);
			break;
		}
		case State::Games:
		{
			ChangeState(State::Arenas);

			if (m_pKaiClient->GetCurrentVector().IsEmpty())
			{
				CStdString aRootVector = KAI_SYSTEM_ROOT;
				CStdString strPassword = "";
				m_pKaiClient->EnterVector(aRootVector,strPassword);
			}
			break;
		}
		case State::Arenas:
		{
			ChangeState(State::Buddies);
			break;
		}
	}
}

void CGUIWindowBuddies::ChangeState(CGUIWindowBuddies::State aNewState)
{
	window_state = aNewState;

	CGUIButtonControl& mode = *((CGUIButtonControl*)GetControl(CONTROL_BTNMODE));

	switch(window_state)
	{
		case State::Buddies:
		{
			CGUIMessage msgb(GUI_MSG_LABEL_BIND,GetID(),CONTROL_LISTEX,0,0,&m_friends);
			g_graphicsContext.SendMessage(msgb);

			mode.SetNavigation(CONTROL_BTNREMOVE,CONTROL_BTNJOIN,CONTROL_BTNMODE,CONTROL_LISTEX);

			SET_CONTROL_HIDDEN(GetID(), CONTROL_IMAGEOPPONENT);
			SET_CONTROL_HIDDEN(GetID(), CONTROL_IMAGEARENA);
			SET_CONTROL_HIDDEN(GetID(), CONTROL_IMAGEBUDDYICON1);
			SET_CONTROL_HIDDEN(GetID(), CONTROL_IMAGEBUDDYICON2);

			SET_CONTROL_VISIBLE(GetID(),CONTROL_LISTEX);
			SET_CONTROL_VISIBLE(GetID(),CONTROL_BTNMODE);
			SET_CONTROL_VISIBLE(GetID(),CONTROL_BTNJOIN);
			SET_CONTROL_VISIBLE(GetID(),CONTROL_BTNSPEEX);
			SET_CONTROL_VISIBLE(GetID(),CONTROL_BTNINVITE);
			SET_CONTROL_VISIBLE(GetID(),CONTROL_BTNREMOVE);
			SET_CONTROL_HIDDEN(GetID(), CONTROL_BTNPLAY);
			SET_CONTROL_HIDDEN(GetID(), CONTROL_BTNADD);
			SET_CONTROL_HIDDEN(GetID(), CONTROL_BTNHOST);
			SET_CONTROL_HIDDEN(GetID(), CONTROL_LABELPLAYERCNT);

			SET_CONTROL_LABEL(GetID(),  CONTROL_LABELBUDDYWIN,  "Friends");
			SET_CONTROL_LABEL(GetID(),  CONTROL_BTNMODE,		"Games");	
			SET_CONTROL_LABEL(GetID(),  CONTROL_BTNJOIN,		"Join");	
			SET_CONTROL_LABEL(GetID(),  CONTROL_BTNSPEEX,		"Chat");	
			SET_CONTROL_LABEL(GetID(),  CONTROL_BTNINVITE,		"Invite");	
			SET_CONTROL_LABEL(GetID(),  CONTROL_BTNREMOVE,		"Remove");
			break;
		}

		case State::Games:
		{
			CGUIMessage msgb(GUI_MSG_LABEL_BIND,GetID(),CONTROL_LISTEX,0,0,&m_games);
			g_graphicsContext.SendMessage(msgb);

			mode.SetNavigation(CONTROL_BTNADD,CONTROL_BTNPLAY,CONTROL_BTNMODE,CONTROL_LISTEX);

			SET_CONTROL_HIDDEN(GetID(), CONTROL_IMAGEBUDDYICON1);
			SET_CONTROL_HIDDEN(GetID(), CONTROL_IMAGEBUDDYICON2);

			SET_CONTROL_HIDDEN(GetID(), CONTROL_BTNJOIN);
			SET_CONTROL_HIDDEN(GetID(), CONTROL_BTNSPEEX);
			SET_CONTROL_HIDDEN(GetID(), CONTROL_BTNINVITE);
			SET_CONTROL_HIDDEN(GetID(), CONTROL_BTNREMOVE);
			SET_CONTROL_VISIBLE(GetID(),CONTROL_LISTEX);
			SET_CONTROL_VISIBLE(GetID(),CONTROL_BTNMODE);
			SET_CONTROL_VISIBLE(GetID(),CONTROL_BTNPLAY);
			SET_CONTROL_VISIBLE(GetID(),CONTROL_BTNHOST);
			SET_CONTROL_VISIBLE(GetID(),CONTROL_BTNADD);
			SET_CONTROL_HIDDEN(GetID(), CONTROL_LABELPLAYERCNT);

			SET_CONTROL_LABEL(GetID(),  CONTROL_LABELBUDDYWIN,  "Games");
			SET_CONTROL_LABEL(GetID(),  CONTROL_BTNMODE,		"Arenas");	
			SET_CONTROL_LABEL(GetID(),  CONTROL_BTNPLAY,		"Enter");
			SET_CONTROL_LABEL(GetID(),  CONTROL_BTNADD,			"Add");
			SET_CONTROL_LABEL(GetID(),  CONTROL_BTNHOST,		"Host");	
			SET_CONTROL_DISABLED(GetID(), CONTROL_BTNADD);	
			break;
		}

		case State::Arenas:
		{
			CGUIMessage msgb(GUI_MSG_LABEL_BIND,GetID(),CONTROL_LISTEX,0,0,&m_arena);
			g_graphicsContext.SendMessage(msgb);

			mode.SetNavigation(CONTROL_BTNADD,CONTROL_BTNPLAY,CONTROL_BTNMODE,CONTROL_LISTEX);

			SET_CONTROL_HIDDEN(GetID(), CONTROL_IMAGEBUDDYICON1);
			SET_CONTROL_HIDDEN(GetID(), CONTROL_IMAGEBUDDYICON2);

			SET_CONTROL_HIDDEN(GetID(), CONTROL_BTNJOIN);
			SET_CONTROL_HIDDEN(GetID(), CONTROL_BTNSPEEX);
			SET_CONTROL_HIDDEN(GetID(), CONTROL_BTNINVITE);
			SET_CONTROL_HIDDEN(GetID(), CONTROL_BTNREMOVE);
			SET_CONTROL_VISIBLE(GetID(),CONTROL_LISTEX);
			SET_CONTROL_VISIBLE(GetID(),CONTROL_BTNMODE);
			SET_CONTROL_VISIBLE(GetID(),CONTROL_BTNPLAY);
			SET_CONTROL_VISIBLE(GetID(),CONTROL_BTNADD);
			SET_CONTROL_VISIBLE(GetID(),CONTROL_BTNHOST);
			SET_CONTROL_VISIBLE(GetID(),CONTROL_LABELPLAYERCNT);

			SET_CONTROL_LABEL(GetID(),  CONTROL_LABELBUDDYWIN,  "Arenas");
			SET_CONTROL_LABEL(GetID(),  CONTROL_BTNMODE,		"Friends");
			SET_CONTROL_LABEL(GetID(),  CONTROL_BTNPLAY,		"Play");
			SET_CONTROL_LABEL(GetID(),  CONTROL_BTNADD,			"Add");
			SET_CONTROL_LABEL(GetID(),  CONTROL_BTNHOST,		"Host");	
			SET_CONTROL_DISABLED(GetID(), CONTROL_BTNADD);	
			break;
		}
	}
}

void CGUIWindowBuddies::Enter(CArenaItem& aArena)
{
	CLog::Log(LOGDEBUG, "CGUIWindowBuddies::Enter");

	if (aArena.m_bIsPrivate)
	{
		CStdString strHeading = "A password is required to join this arena.";
		if (!CGUIDialogKeyboard::ShowAndGetInput(aArena.m_strPassword, strHeading, false))
		{
			return;
		}
	}

	ChangeState(State::Arenas);
	m_pKaiClient->EnterVector(aArena.m_strVector, aArena.m_strPassword );
}


/* IBuddyObserver methods */
void CGUIWindowBuddies::OnContactOffline(CStdString& aFriend)
{
	CGUIList::GUILISTITEMS& list = m_friends.Lock();

	CBuddyItem* pBuddy = (CBuddyItem*) m_friends.Find(aFriend);
	if (pBuddy==NULL)
	{
		pBuddy = new CBuddyItem(aFriend);
		pBuddy->m_bIsContact = true;
		m_friends.Add(pBuddy);
/*
		CStdString choke="Chokemaniac";
		if (aFriend.CompareNoCase(choke)==0)
		{
			Invitation invite;
			invite.message="Fancy a game in 5 mins?";
			invite.time="Sunday 5th November 2004, 12:34:22pm";
			invite.vector="Arena/XBOX/First Person Shooters/Halo 2";
			m_invitations[choke]=invite;
			pBuddy->m_bInvite = true;
		}
*/
	}

	pBuddy->SetIcon(16,16,"buddyitem-offline.png");
	pBuddy->m_bIsOnline = false;
	pBuddy->m_dwPing	= 0;
	if (pBuddy->IsAvatarCached())
	{
		pBuddy->UseCachedAvatar();
	}
		
	m_friends.Release();
}

void CGUIWindowBuddies::OnContactOnline(CStdString& aFriend)
{
	CGUIList::GUILISTITEMS& list = m_friends.Lock();

	CBuddyItem* pBuddy = (CBuddyItem*) m_friends.Find(aFriend);
	if (pBuddy==NULL)
	{
		pBuddy = new CBuddyItem(aFriend);
		pBuddy->m_bIsContact = true;
		m_friends.Add(pBuddy);
	}

	pBuddy->m_bIsOnline = true;
	
	// if last on, then leave speex on
	if (pBuddy->m_bSpeex)
	{
		m_pKaiClient->EnableContactVoice(aFriend,pBuddy->m_bSpeex);
	}

	pBuddy->SetIcon(16,16,"buddyitem-online.png");

	m_friends.Release();
}

void CGUIWindowBuddies::OnContactPing(CStdString& aFriend, CStdString& aVector, DWORD aPing, int aStatus, CStdString& aBearerCapability)
{
//	CLog::Log(LOGINFO, "CGUIWindowBuddies::OnContactPing");	

	CGUIList::GUILISTITEMS& list = m_friends.Lock();

	CBuddyItem* pBuddy = (CBuddyItem*) m_friends.Find(aFriend);
	if (pBuddy)
	{
		pBuddy->m_bIsOnline = true;
		pBuddy->m_strVector = aVector;
		pBuddy->m_dwPing	= aPing;
		pBuddy->m_nStatus	= aStatus;
		pBuddy->m_bBusy		= aBearerCapability.length()==0;
		if (!pBuddy->m_bBusy)
		{
			pBuddy->m_bHeadset  = aBearerCapability.Find('2')>=0;
		}
	}

	m_friends.Release();
}

void CGUIWindowBuddies::OnContactRemove(CStdString& aFriend)
{
	CLog::Log(LOGDEBUG, "CGUIWindowBuddies::OnContactRemove");	
	m_friends.Remove(aFriend);
}
void CGUIWindowBuddies::OnContactSpeexStatus(CStdString& aFriend, bool bSpeexEnabled)
{
	CGUIList::GUILISTITEMS& list = m_friends.Lock();

	CBuddyItem* pBuddy = (CBuddyItem*) m_friends.Find(aFriend);
	if (pBuddy)
	{
		pBuddy->m_bSpeex = bSpeexEnabled;
		pBuddy->m_bRingIndicator = !bSpeexEnabled;
	}

	m_friends.Release();
}
void CGUIWindowBuddies::OnContactSpeexRing(CStdString& aFriend)
{
	CGUIList::GUILISTITEMS& list = m_friends.Lock();

	CBuddyItem* pBuddy = (CBuddyItem*) m_friends.Find(aFriend);
	if (pBuddy)
	{
		pBuddy->m_dwRingCounter	= 120;

		if (pBuddy->m_bRingIndicator)
		{
			pBuddy->m_bRingIndicator = FALSE;
			CStdString strNotification;
			strNotification.Format("%s has invited you to voice chat",aFriend);
			CLog::Log(LOGINFO,strNotification);
			//pApplication->SetKaiNotification("X has invited you to voice chat");
		}
	}

	m_friends.Release();
}
void CGUIWindowBuddies::OnContactSpeex(CStdString& aFriend)
{
	CGUIList::GUILISTITEMS& list = m_friends.Lock();

	CBuddyItem* pBuddy = (CBuddyItem*) m_friends.Find(aFriend);
	if (pBuddy)
	{
		pBuddy->m_dwSpeexCounter = 120;
	}

	m_friends.Release();
}
void CGUIWindowBuddies::OnContactInvite(CStdString& aFriend, CStdString& aVector, CStdString& aTime, CStdString& aMessage)
{
	CGUIList::GUILISTITEMS& list = m_friends.Lock();

	CBuddyItem* pBuddy = (CBuddyItem*) m_friends.Find(aFriend);
	if (pBuddy)
	{
		Invitation invite;
		invite.vector = aVector;
		invite.time = aTime;
		invite.message = aMessage;
		m_invitations[aFriend]=invite;

		pBuddy->m_bInvite = TRUE;

		CStdString strNotification;
		strNotification.Format("%s has sent you an invitation",aFriend);
		CLog::Log(LOGINFO,strNotification);
		//pApplication->SetKaiNotification("X has sent you an invitation");
	}

	m_friends.Release();
}
void CGUIWindowBuddies::QueryInstalledGames()
{
	m_games.Clear();

	WIN32_FIND_DATA wfd;
	memset(&wfd,0,sizeof(wfd));

	HANDLE hFind = FindFirstFile("E:\\tdata\\*.*",&wfd);

	if (hFind!=NULL)
	{
		do
		{
			if (wfd.cFileName[0]!=0)
			{
				if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					DWORD titleId = strtoul((CHAR*)wfd.cFileName,NULL,16);
					m_pKaiClient->QueryVector(titleId);
				}
			}
		} while (FindNextFile(hFind, &wfd));

		CloseHandle(hFind);
	}
}


void CGUIWindowBuddies::Play(CStdString& aVector)
{
	CGUIDialogProgress& dialog = *((CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS));

	CStdString strGame;
	CArenaItem::GetTier(CArenaItem::Game, aVector, strGame);

	dialog.SetHeading(strGame.c_str());
	dialog.SetLine(0,"Please place the game into your XBOX and");
	dialog.SetLine(1,"select SYSTEM LINK to play online.");
	dialog.SetLine(2,"" );
	dialog.StartModal(GetID());

	while (true)
	{	
		if (dialog.IsCanceled()) 
		{
			break;
		}

		if (CDetectDVDMedia::IsDiscInDrive())
		{
			if ( CUtil::FileExists("D:\\default.xbe") ) 
			{
				CUtil::LaunchXbe( "Cdrom0", "D:\\default.xbe", NULL );
			}
		}
		
		dialog.Progress();
	}

	dialog.Close();	
}


void CGUIWindowBuddies::OnSupportedTitle(DWORD aTitleId, CStdString& aVector)
{
	CLog::Log(LOGDEBUG, "CGUIWindowBuddies::OnSupportedTitle");	

	INT arenaDelimiter = aVector.ReverseFind('/')+1;
	CStdString arenaLabel = aVector.Mid(arenaDelimiter);

	CGUIList::GUILISTITEMS& list = m_games.Lock();

	CArenaItem* pArena = (CArenaItem*) m_games.Find(arenaLabel);
	if (!pArena)
	{
		pArena = new CArenaItem(arenaLabel);
		pArena->m_strVector = aVector;
		pArena->SetIcon(16,16,"arenaitem-small.png");

		CStdString aAvatarUrl;
		aAvatarUrl.Format("http://www.teamxlink.co.uk/media/avatars/%s.jpg",aVector);
		pArena->SetAvatar(aAvatarUrl);

		m_games.Add(pArena);
	}

	m_games.Release();
}

void CGUIWindowBuddies::OnEnterArena(CStdString& aVector, BOOL bCanHost)
{
	CLog::Log(LOGDEBUG, "CGUIWindowBuddies::OnEnterArena");	

	m_arena.Clear();
	
	if (bCanHost)
	{
		SET_CONTROL_ENABLED(GetID(), CONTROL_BTNHOST);
	}
	else
	{
		SET_CONTROL_DISABLED(GetID(), CONTROL_BTNHOST);
	}

	m_pKaiClient->GetSubVectors(aVector);
}

void CGUIWindowBuddies::OnEnterArenaFailed(CStdString& aVector, CStdString& aReason)
{
	CLog::Log(LOGDEBUG, "CGUIWindowBuddies::OnEnterArenaFailed");

	CGUIDialogOK* pDialog = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
	pDialog->SetHeading("Access Denied");
	pDialog->SetLine(0,aReason);
	pDialog->SetLine(1,L"");
	pDialog->SetLine(2,L"");

	ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_OK, m_gWindowManager.GetActiveWindow()};
	g_applicationMessenger.SendMessage(tMsg, false);
}

void CGUIWindowBuddies::OnNewArena(	CStdString& aVector, CStdString& aDescription,
									int nPlayers, int nPlayerLimit, int nPassword, bool bPersonal )

{
	CLog::Log(LOGDEBUG, "CGUIWindowBuddies::OnNewArena");	

	INT arenaDelimiter = aVector.ReverseFind('/')+1;

	CStdString arenaLabel = aVector.Mid(arenaDelimiter);

	CGUIList::GUILISTITEMS& list = m_arena.Lock();

	CArenaItem* pArena = (CArenaItem*) m_arena.Find(arenaLabel);
	if (!pArena)
	{
		pArena = new CArenaItem(arenaLabel);
		pArena->m_strVector = aVector;
		pArena->m_strDescription = aDescription;
		pArena->m_nPlayers = nPlayers;
		pArena->m_nPlayerLimit = nPlayerLimit;
		pArena->m_bIsPersonal = bPersonal;
		pArena->m_bIsPrivate = nPassword>0;
		pArena->SetIcon(16,16,"arenaitem-small.png");

		aVector.Replace(" ","%20");
		aVector.Replace(":","%3A");

		CStdString aAvatarUrl;
		aAvatarUrl.Format("http://www.teamxlink.co.uk/media/avatars/%s.jpg",aVector);
		pArena->SetAvatar(aAvatarUrl);

		m_arena.Add(pArena);
	}

	m_arena.Release();
}

void CGUIWindowBuddies::OnUpdateArena(	CStdString& aVector, int nPlayers )
{
	INT arenaDelimiter = aVector.ReverseFind('/')+1;
	CStdString arenaLabel = aVector.Mid(arenaDelimiter);

	m_arena.Lock();
	CArenaItem* pArena = (CArenaItem*) m_arena.Find(arenaLabel);
	if (pArena)
	{
		pArena->m_nPlayers = nPlayers;
	}
	m_arena.Release();

	m_games.Lock();
	pArena = (CArenaItem*) m_games.Find(arenaLabel);
	if (pArena)
	{
		pArena->m_nPlayers = nPlayers;
	}
	m_games.Release();
}

void CGUIWindowBuddies::OnUpdateOpponent(CStdString& aOpponent, CStdString& aAge, 
			CStdString& aBandwidth, CStdString& aLocation, CStdString& aBio)
{
	CStdString strDebug;
	strDebug.Format("Received profile for %s.\r\n",aOpponent.c_str());
	OutputDebugString(strDebug.c_str());

	m_arena.Lock();
	CBuddyItem* pOpponent = (CBuddyItem*) m_arena.Find(aOpponent);
	if (pOpponent)
	{
		pOpponent->m_strGeoLocation = aLocation;
	}
	m_arena.Release();

	
	// may as well update friends as well as opponents ;)
	m_friends.Lock();
	pOpponent = (CBuddyItem*) m_friends.Find(aOpponent);
	if (pOpponent)
	{
		pOpponent->m_strGeoLocation = aLocation;
	}
	m_friends.Release();
}

void CGUIWindowBuddies::OnUpdateOpponent(CStdString& aOpponent, CStdString& aAvatarURL)
{
	CStdString strDebug;
	strDebug.Format("Received avatar for %s.\r\n",aOpponent.c_str());
	OutputDebugString(strDebug.c_str());

	m_arena.Lock();
	CBuddyItem* pOpponent = (CBuddyItem*) m_arena.Find(aOpponent);
	if (pOpponent)
	{
		pOpponent->SetAvatar(aAvatarURL);
	}
	m_arena.Release();

	// may as well update friends as well as opponents ;)
	m_friends.Lock();
	pOpponent = (CBuddyItem*) m_friends.Find(aOpponent);
	if (pOpponent)
	{
		pOpponent->SetAvatar(aAvatarURL);
	}
	m_friends.Release();
}

void CGUIWindowBuddies::OnOpponentEnter(CStdString& aOpponent)
{
	CLog::Log(LOGDEBUG, "CGUIWindowBuddies::OnOpponentEnter");	

	CGUIList::GUILISTITEMS& list = m_arena.Lock();

	CBuddyItem* pOpponent = (CBuddyItem*) m_arena.Find(aOpponent);
	if (!pOpponent)
	{
		pOpponent = new CBuddyItem(aOpponent);
		pOpponent->m_bIsOnline = true;
		pOpponent->SetIcon(16,16,"buddyitem-online.png");
		
		m_arena.Add(pOpponent);
	}

	m_arena.Release();
}

void CGUIWindowBuddies::OnOpponentPing(CStdString& aOpponent, DWORD aPing, int aStatus, CStdString& aBearerCapability)
{
	CGUIList::GUILISTITEMS& list = m_arena.Lock();

	CBuddyItem* pOpponent = (CBuddyItem*) m_arena.Find(aOpponent);
	if (pOpponent)
	{
		pOpponent->m_dwPing		= aPing;
		pOpponent->m_nStatus	= aStatus;
		pOpponent->m_bBusy		= aBearerCapability.length()==0;
		if (!pOpponent->m_bBusy)
		{
			pOpponent->m_bHeadset  = aBearerCapability.Find('2')>=0;
		}
	}

	m_arena.Release();
}

void CGUIWindowBuddies::OnOpponentLeave(CStdString& aOpponent)
{
	CLog::Log(LOGDEBUG, "CGUIWindowBuddies::OnOpponentLeave");	
	m_arena.Remove(aOpponent);
}

void CGUIWindowBuddies::UpdateGamesPlayerCount()
{
	CArenaItem* pItem = NULL;
	CGUIList::GUILISTITEMS& games			  = m_games.Lock();
	CGUIList::GUILISTITEMS::iterator iterator = games.begin();

	while (iterator != games.end())
	{
		pItem = (CArenaItem*) (*iterator);
		m_pKaiClient->QueryVectorPlayerCount(pItem->m_strVector);
		iterator++;
	}

	m_games.Release();
}