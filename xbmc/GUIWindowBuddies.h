#pragma once
#include "guiImage.h"
#include "guiwindow.h"
#include "guiList.h"
#include "stdstring.h"
#include "utils\UdpClient.h"
#include "tinyxml/tinyxml.h"
#include "guiDialogProgress.h"

#include "utils/KaiClient.h"
#include "BuddyItem.h"
#include "ArenaItem.h"

using namespace std;
#include <map>

class CGUIWindowBuddies : 	public CGUIWindow, public IBuddyObserver
{
public:
	CGUIWindowBuddies(void);
	virtual ~CGUIWindowBuddies(void);

	virtual void			OnContactOffline(CStdString& aFriend);
	virtual void			OnContactOnline(CStdString& aFriend);
	virtual void			OnContactPing(CStdString& aFriend, CStdString& aVector, CStdString& aPing);
	virtual void			OnContactRemove(CStdString& aFriend);
	virtual void			OnEnterArena(CStdString& aVector);
	virtual void			OnEnterArenaFailed(CStdString& aVector, CStdString& aReason);
	virtual void			OnNewArena( CStdString& aVector, CStdString& aDescription,
										int nPlayers, int nPlayerLimit, int bPrivate );
	virtual void			OnOpponentEnter(CStdString& aOpponent);
	virtual void			OnOpponentPing(CStdString& aOpponent, CStdString& aPing);
	virtual void			OnOpponentLeave(CStdString& aOpponent);
	virtual void			OnSupportedTitle(DWORD aTitleId, CStdString& aVector);

	virtual void			OnInitWindow();
	virtual void			OnAction(const CAction &action);
	virtual void			Render();

protected:
	enum State {Uninitialized,Buddies,Games,Arenas};

	void OnClickModeButton	(CGUIMessage& aMessage);
	void OnClickAddButton	(CGUIMessage& aMessage);
	void OnClickRemoveButton(CGUIMessage& aMessage);
	void OnClickInviteButton(CGUIMessage& aMessage);
	void OnClickJoinButton	(CGUIMessage& aMessage);
	void OnClickPlayButton	(CGUIMessage& aMessage);
	void OnClickListItem	(CGUIMessage& aMessage);

	void	QueryInstalledGames();
	void	UpdatePanel();
	void	UpdateFriends();
	void	UpdateArena();
	void	ChangeState();
	void	ChangeState(State aNewState);

	void	Enter(CArenaItem& aArena);
	void	Play(CStdString& aVector);

	static bool	SortFriends(CGUIItem* pStart, CGUIItem* pEnd);
	static bool	SortArenaItems(CGUIItem* pStart, CGUIItem* pEnd);

	CBuddyItem* GetBuddySelection();
	CArenaItem* GetArenaSelection();

	CGUISortedList			m_friends;
	CGUISortedList			m_arena;
	CGUISortedList			m_games;
	State window_state;
};
