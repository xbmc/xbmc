
#include "stdafx.h"
#include "Application.h"
#include "GUIUserMessages.h"
#include "GUIWindowBuddies.h"
#include "settings.h"
#include "guiWindowManager.h"
#include "GUIDialogInvite.h"
#include "GUIDialogProgress.h"
#include "GUIListControl.h"
#include "localizestrings.h"
#include "util.h"
#include <algorithm>
#include "utils/log.h"

#define CONTROL_BTNMODE			12				// Games Button
#define CONTROL_BTNJOIN			13				// Join Button
#define CONTROL_BTNINVITE		14				// Invite Button
#define CONTROL_BTNREMOVE		15				// Remove Button

#define CONTROL_LISTEX			31
#define CONTROL_BTNPLAY			33				// Play Button
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
	m_friends.SetSortingAlgorithm(CGUIWindowBuddies::SortFriends);
	m_arena.SetSortingAlgorithm(CGUIWindowBuddies::SortArenaItems);
	m_games.SetSortingAlgorithm(CGUIWindowBuddies::SortArenaItems);

	ON_CLICK_MESSAGE(CONTROL_BTNMODE,	CGUIWindowBuddies, OnClickModeButton);
	ON_CLICK_MESSAGE(CONTROL_BTNADD,	CGUIWindowBuddies, OnClickAddButton);
	ON_CLICK_MESSAGE(CONTROL_BTNREMOVE,	CGUIWindowBuddies, OnClickRemoveButton);
	ON_CLICK_MESSAGE(CONTROL_BTNINVITE,	CGUIWindowBuddies, OnClickInviteButton);
	ON_CLICK_MESSAGE(CONTROL_BTNJOIN,	CGUIWindowBuddies, OnClickJoinButton);
	ON_CLICK_MESSAGE(CONTROL_BTNPLAY,	CGUIWindowBuddies, OnClickPlayButton);
	ON_CLICK_MESSAGE(CONTROL_LISTEX,	CGUIWindowBuddies, OnClickListItem);
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

	if (window_state==State::Uninitialized)
	{
		ChangeState(State::Buddies);

		CGUIMessage msgb(GUI_MSG_LABEL_BIND,GetID(),CONTROL_LISTEX,0,0,&m_friends);
		g_graphicsContext.SendMessage(msgb);
	}

	//NOTE: We now call this method from Application.cpp
	//CKaiClient::GetInstance()->SetObserver(this);
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
//	return dynamic_cast<CArenaItem*>((CGUIListExItem*)msg.GetLPVOID());

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
		CKaiClient::GetInstance()->AddContact(contact);
	}
}

void CGUIWindowBuddies::OnClickRemoveButton(CGUIMessage& aMessage)
{
	CBuddyItem* pBuddy;
	if ( (pBuddy = GetBuddySelection()) )
	{
		CStdString contact = pBuddy->GetName();
		CKaiClient::GetInstance()->RemoveContact(contact);
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
	}
}

void CGUIWindowBuddies::OnClickJoinButton(CGUIMessage& aMessage)
{
	CBuddyItem* pBuddy;
	if ( (pBuddy = GetBuddySelection()) )
	{
		ChangeState(State::Arenas);
		SET_CONTROL_FOCUS(GetID(),  CONTROL_LISTEX, 0);
		CKaiClient::GetInstance()->EnterVector(pBuddy->m_strVector);
	}
}

void CGUIWindowBuddies::OnClickPlayButton(CGUIMessage& aMessage)
{
	switch (window_state)
	{
		case State::Arenas:
		{
			Play( CKaiClient::GetInstance()->GetCurrentVector() );
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
				Play( CKaiClient::GetInstance()->GetCurrentVector() );
			}
			break;
		}
	}
}

void CGUIWindowBuddies::OnClickListItem(CGUIMessage& aMessage)
{
	CArenaItem* pArena;
	if ( (pArena = GetArenaSelection()) )
	{
		Enter(*pArena);
	}
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
					CKaiClient::GetInstance()->ExitVector();
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
	SET_CONTROL_DISABLED(GetID(), CONTROL_BTNINVITE);	
	SET_CONTROL_DISABLED(GetID(), CONTROL_BTNREMOVE);	
	SET_CONTROL_DISABLED(GetID(), CONTROL_BTNADD);	

	if (pBuddy!=NULL)
	{
		SET_CONTROL_ENABLED(GetID(), CONTROL_BTNADD);	
		SET_CONTROL_ENABLED(GetID(), CONTROL_BTNINVITE);	
		SET_CONTROL_ENABLED(GetID(), CONTROL_BTNREMOVE);	

		if (pBuddy->m_bIsOnline)
		{
			SET_CONTROL_ENABLED(GetID(), CONTROL_BTNJOIN);	
		}
	}
}


void CGUIWindowBuddies::Render()
{
	CGUIWindow::Render();

	// Update buttons
	UpdatePanel();		

	// Update current Arena name
	CKaiClient* pClient = CKaiClient::GetInstance();
	CStdString currentVector = pClient->GetCurrentVector();
	INT arenaDelimiter = currentVector.ReverseFind('/')+1;
	CStdString arenaName = currentVector.Mid(arenaDelimiter);
	SET_CONTROL_LABEL(GetID(),  CONTROL_LABELUPDATED, arenaName);

	// Update mode/item selection specific labels
	switch(window_state)
	{
		case State::Buddies:
		{
			UpdateFriends();
			break;
		}
		case State::Games:
		case State::Arenas:
		{
			UpdateArena();
			break;
		}
	}
}

void CGUIWindowBuddies::UpdateFriends()
{
	CBuddyItem* pBuddy = GetBuddySelection();

	if (pBuddy!=NULL)
	{
		CStdString buddyInfo;
		if (pBuddy->m_bIsOnline)
		{					
			buddyInfo.Format("%s (%ums)", pBuddy->GetName().c_str(), pBuddy->m_dwPing);

			SET_CONTROL_LABEL(GetID(), CONTROL_LABELBUDDYSTAT, pBuddy->GetArena());
		}
		else
		{
			buddyInfo.Format("%s", pBuddy->GetName().c_str());

			SET_CONTROL_LABEL(GetID(), CONTROL_LABELBUDDYSTAT, "Offline" );
		}

		SET_CONTROL_LABEL(GetID(), CONTROL_LABELBUDDYNAME, buddyInfo);
		SET_CONTROL_HIDDEN(GetID(),  pBuddy->m_bIsOnline ? CONTROL_IMAGEBUDDYICON1 : CONTROL_IMAGEBUDDYICON2);
		SET_CONTROL_VISIBLE(GetID(), pBuddy->m_bIsOnline ? CONTROL_IMAGEBUDDYICON2 : CONTROL_IMAGEBUDDYICON1);
	}
}

void CGUIWindowBuddies::UpdateArena()
{
	CArenaItem* pArena = GetArenaSelection();

	SET_CONTROL_HIDDEN(GetID(), CONTROL_IMAGEOPPONENT);
	SET_CONTROL_HIDDEN(GetID(), CONTROL_IMAGEARENA);

	// Update current Player Count
	int iPlayers = 0;
	CGUIList::GUILISTITEMS& list = m_arena.Lock();
	for(int iItem=0;iItem<(int)list.size();iItem++)
	{
		if (list[iItem]->GetCookie()==CKaiClient::Item::Player)
		{
			iPlayers++;
		}
	}
	m_arena.Release();

	CStdString strPlayers;
	strPlayers.Format("(%d)", iPlayers);
	SET_CONTROL_LABEL(GetID(), CONTROL_LABELPLAYERCNT, strPlayers.c_str());

	// If item selected is an Arena
	if (pArena!=NULL)
	{
		// Update selected Arena name
		SET_CONTROL_LABEL(GetID(), CONTROL_LABELBUDDYNAME, pArena->GetName());

		// Update selected Arena category
		CStdString strCategory;

		switch (pArena->GetTier())
		{
			case CArenaItem::Tier::Custom:							 
				pArena->GetTier(CArenaItem::Tier::Game,strCategory);
				break;
			case CArenaItem::Tier::Platform:
				break;
			default:
				pArena->GetTier(CArenaItem::Tier::Platform,strCategory);
				strCategory = strCategory.ToUpper();
				break;
		}

		SET_CONTROL_LABEL(GetID(),	CONTROL_LABELBUDDYSTAT, strCategory.c_str());

		// Update selected Arena icon
		SET_CONTROL_VISIBLE(GetID(), CONTROL_IMAGEARENA);
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
			SET_CONTROL_VISIBLE(GetID(), CONTROL_IMAGEOPPONENT);
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

			CKaiClient* pClient = CKaiClient::GetInstance();
			
			if (pClient->GetCurrentVector().IsEmpty())
			{
				CStdString aRootVector = KAI_SYSTEM_ROOT;
				pClient->EnterVector(aRootVector);
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
			SET_CONTROL_VISIBLE(GetID(),CONTROL_BTNINVITE);
			SET_CONTROL_VISIBLE(GetID(),CONTROL_BTNREMOVE);
			SET_CONTROL_HIDDEN(GetID(), CONTROL_BTNPLAY);
			SET_CONTROL_HIDDEN(GetID(), CONTROL_BTNADD);
			SET_CONTROL_HIDDEN(GetID(), CONTROL_LABELPLAYERCNT);

			SET_CONTROL_LABEL(GetID(),  CONTROL_LABELBUDDYWIN,  "Friends");
			SET_CONTROL_LABEL(GetID(),  CONTROL_BTNMODE,		"Games");	
			SET_CONTROL_LABEL(GetID(),  CONTROL_BTNJOIN,		"Join");	
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
			SET_CONTROL_HIDDEN(GetID(), CONTROL_BTNINVITE);
			SET_CONTROL_HIDDEN(GetID(), CONTROL_BTNREMOVE);
			SET_CONTROL_VISIBLE(GetID(),CONTROL_LISTEX);
			SET_CONTROL_VISIBLE(GetID(),CONTROL_BTNMODE);
			SET_CONTROL_VISIBLE(GetID(),CONTROL_BTNPLAY);
			SET_CONTROL_VISIBLE(GetID(),CONTROL_BTNADD);
			SET_CONTROL_HIDDEN(GetID(), CONTROL_LABELPLAYERCNT);

			SET_CONTROL_LABEL(GetID(),  CONTROL_LABELBUDDYWIN,  "Games");
			SET_CONTROL_LABEL(GetID(),  CONTROL_BTNMODE,		"Arenas");	
			SET_CONTROL_LABEL(GetID(),  CONTROL_BTNPLAY,		"Enter");
			SET_CONTROL_LABEL(GetID(),  CONTROL_BTNADD,			"Add");	
			SET_CONTROL_DISABLED(GetID(), CONTROL_BTNADD);	

			QueryInstalledGames();
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
			SET_CONTROL_HIDDEN(GetID(), CONTROL_BTNINVITE);
			SET_CONTROL_HIDDEN(GetID(), CONTROL_BTNREMOVE);
			SET_CONTROL_VISIBLE(GetID(),CONTROL_LISTEX);
			SET_CONTROL_VISIBLE(GetID(),CONTROL_BTNMODE);
			SET_CONTROL_VISIBLE(GetID(),CONTROL_BTNPLAY);
			SET_CONTROL_VISIBLE(GetID(),CONTROL_BTNADD);
			SET_CONTROL_VISIBLE(GetID(),CONTROL_LABELPLAYERCNT);

			SET_CONTROL_LABEL(GetID(),  CONTROL_LABELBUDDYWIN,  "Arenas");
			SET_CONTROL_LABEL(GetID(),  CONTROL_BTNMODE,		"Friends");
			SET_CONTROL_LABEL(GetID(),  CONTROL_BTNPLAY,		"Play");
			SET_CONTROL_LABEL(GetID(),  CONTROL_BTNADD,			"Add");	
			SET_CONTROL_DISABLED(GetID(), CONTROL_BTNADD);	
			break;
		}
	}
}

void CGUIWindowBuddies::Enter(CArenaItem& aArena)
{
	CLog::Log("CGUIWindowBuddies::Enter");	
	ChangeState(State::Arenas);
	CKaiClient::GetInstance()->EnterVector(aArena.m_strVector);
}






void CGUIWindowBuddies::OnContactOffline(CStdString& aFriend)
{
	CGUIList::GUILISTITEMS& list = m_friends.Lock();

	CBuddyItem* pBuddy = (CBuddyItem*) m_friends.Find(aFriend);
	if (pBuddy==NULL)
	{
		pBuddy = new CBuddyItem(aFriend);
		m_friends.Add(pBuddy);
	}

	pBuddy->SetIcon(16,16,"buddyitem-offline.png");
	pBuddy->m_bIsOnline = false;
	pBuddy->m_dwPing	= 0;
		
	m_friends.Release();
}

void CGUIWindowBuddies::OnContactOnline(CStdString& aFriend)
{
	CGUIList::GUILISTITEMS& list = m_friends.Lock();

	CBuddyItem* pBuddy = (CBuddyItem*) m_friends.Find(aFriend);
	if (pBuddy==NULL)
	{
		pBuddy = new CBuddyItem(aFriend);
		m_friends.Add(pBuddy);
	}

	pBuddy->m_bIsOnline = true;
	pBuddy->SetIcon(16,16,"buddyitem-online.png");
	pBuddy->SetPingIcon(12,12,"buddyitem-ping.png");

	m_friends.Release();
}

void CGUIWindowBuddies::OnContactPing(CStdString& aFriend, CStdString& aVector, CStdString& aPing)
{
//	CLog::Log("CGUIWindowBuddies::OnContactPing");	

	CGUIList::GUILISTITEMS& list = m_friends.Lock();

	CBuddyItem* pBuddy = (CBuddyItem*) m_friends.Find(aFriend);
	if (pBuddy)
	{
		pBuddy->m_bIsOnline = true;
		pBuddy->m_strVector = aVector;
		pBuddy->m_dwPing	= strtoul(aPing.c_str(),NULL,10);
	}

	m_friends.Release();
}

void CGUIWindowBuddies::OnContactRemove(CStdString& aFriend)
{
	CLog::Log("CGUIWindowBuddies::OnContactRemove");	
	m_friends.Remove(aFriend);
}

void CGUIWindowBuddies::QueryInstalledGames()
{
	m_games.Clear();

	WIN32_FIND_DATA wfd;
	memset(&wfd,0,sizeof(wfd));

	HANDLE hFind = FindFirstFile("E:\\tdata\\*.*",&wfd);

	if (hFind!=NULL)
	{
		CKaiClient* pClient = CKaiClient::GetInstance();

		do
		{
			if (wfd.cFileName[0]!=0)
			{
				if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					DWORD titleId = strtoul((CHAR*)wfd.cFileName,NULL,16);
					pClient->ResolveVector(titleId);
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
	CLog::Log("CGUIWindowBuddies::OnSupportedTitle");	

	INT arenaDelimiter = aVector.ReverseFind('/')+1;
	CStdString arenaLabel = aVector.Mid(arenaDelimiter);

	CGUIList::GUILISTITEMS& list = m_games.Lock();

	CArenaItem* pArena = (CArenaItem*) m_games.Find(arenaLabel);
	if (!pArena)
	{
		pArena = new CArenaItem(arenaLabel);
		pArena->m_strVector = aVector;
		m_games.Add(pArena);
	}

	m_games.Release();
}

void CGUIWindowBuddies::OnEnterArena(CStdString& aVector)
{
	CLog::Log("CGUIWindowBuddies::OnEnterArena");	

	m_arena.Clear();

	CKaiClient::GetInstance()->GetSubVectors(aVector);
}

void CGUIWindowBuddies::OnNewArena(CStdString& aVector)
{
	CLog::Log("CGUIWindowBuddies::OnNewArena");	

	INT arenaDelimiter = aVector.ReverseFind('/')+1;
	CStdString arenaLabel = aVector.Mid(arenaDelimiter);

	CGUIList::GUILISTITEMS& list = m_arena.Lock();

	CArenaItem* pArena = (CArenaItem*) m_arena.Find(arenaLabel);
	if (!pArena)
	{
		pArena = new CArenaItem(arenaLabel);
		pArena->m_strVector = aVector;
		m_arena.Add(pArena);
	}

	m_arena.Release();
}

void CGUIWindowBuddies::OnOpponentEnter(CStdString& aOpponent)
{
	CLog::Log("CGUIWindowBuddies::OnOpponentEnter");	

	CGUIList::GUILISTITEMS& list = m_arena.Lock();

	CBuddyItem* pOpponent = (CBuddyItem*) m_arena.Find(aOpponent);
	if (!pOpponent)
	{
		pOpponent = new CBuddyItem(aOpponent);
		pOpponent->m_bIsOnline = true;
		pOpponent->SetIcon(16,16,"buddyitem-online.png");
		pOpponent->SetPingIcon(12,12,"buddyitem-ping.png");
		
		m_arena.Add(pOpponent);
	}

	m_arena.Release();
}

void CGUIWindowBuddies::OnOpponentPing(CStdString& aOpponent, CStdString& aPing)
{
	CGUIList::GUILISTITEMS& list = m_arena.Lock();

	CBuddyItem* pOpponent = (CBuddyItem*) m_arena.Find(aOpponent);
	if (pOpponent)
	{
		pOpponent->m_dwPing	= strtoul(aPing.c_str(),NULL,10);
	}

	m_arena.Release();
}

void CGUIWindowBuddies::OnOpponentLeave(CStdString& aOpponent)
{
	CLog::Log("CGUIWindowBuddies::OnOpponentLeave");	
	m_arena.Remove(aOpponent);
}
