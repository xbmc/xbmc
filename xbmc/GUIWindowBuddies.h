#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "guiImage.h"
#include "GUIWindow.h"
#include "GUIList.h"

#include "GUIEditControl.h"

#include "utils/UdpClient.h"
#include "utils/KaiClient.h"

#include "KaiVector.h"
#include "BuddyItem.h"
#include "ArenaItem.h"

class CGUIConsoleControl;

class CGUIWindowBuddies : public CGUIWindow, public IBuddyObserver,
      public IEditControlObserver
{
public:
  CGUIWindowBuddies(void);
  virtual ~CGUIWindowBuddies(void);

  virtual void OnInitialise(CKaiClient* pClient);
  virtual void OnDeInitialise();
  virtual void OnEngineDetached();
  virtual void OnAuthenticationFailed(CStdString& aUsername);
  virtual void OnNetworkError(CStdString& aError);
  virtual void OnNetworkReachable(CStdString& aServerName);
  virtual void OnContactOffline(CStdString& aFriend);
  virtual void OnContactOnline(CStdString& aFriend);
  virtual void OnContactsOnline(int nCount);
  virtual void OnContactPing(CStdString& aFriend, CStdString& aVector, DWORD aPing, int aStatus, CStdString& aBearerCapability);
  virtual void OnContactRemove(CStdString& aFriend);
  virtual void OnContactSpeexStatus(CStdString& aFriend, bool bSpeexEnabled);
  virtual void OnContactSpeexRing(CStdString& aFriend);
  virtual void OnContactSpeex(CStdString& aFriend);
  virtual void OnContactInvite(CStdString& aFriend, CStdString& aVector, CStdString& aTime, CStdString& aMessage);
  virtual void OnEnterArena(CStdString& aVector, BOOL bCanHost);
  virtual void OnEnterArenaFailed(CStdString& aVector, CStdString& aReason);
  virtual void OnNewArena( CStdString& aVector, CStdString& aDescription,
                           int nPlayers, int nPlayerLimit, int nPassword, bool bPersonal );
  virtual void OnUpdateArena( CStdString& aVector, int nPlayers );
  virtual void OnUpdateOpponent(CStdString& aOpponent, CStdString& aAge,
                                CStdString& aBandwidth, CStdString& aLocation, CStdString& aBio);
  virtual void OnUpdateOpponent(CStdString& aOpponent, CStdString& aAvatarURL);
  virtual void OnUpdateHostingStatus(BOOL bIsHosting);
  virtual void OnOpponentEnter(CStdString& aOpponent);
  virtual void OnOpponentPing(CStdString& aOpponent, DWORD aPing, int aStatus, CStdString& aBearerCapability);
  virtual void OnOpponentLeave(CStdString& aOpponent);
  virtual void OnSupportedTitle(DWORD aTitleId, CStdString& aVector);
  virtual void OnJoinsChat(CStdString& aOpponent);
  virtual void OnChat(CStdString& aVector, CStdString& aOpponent, CStdString& aMessage, bool bPrivate);
  virtual void OnLeavesChat(CStdString& aOpponent);

  virtual void OnEditTextComplete(CStdString& strLineOfText);

  virtual void OnWindowUnload();
  virtual void OnWindowLoaded();
  virtual void OnInitWindow();
  virtual bool OnMessage(CGUIMessage &message);
  virtual bool OnAction(const CAction &action);
  virtual void Render();

protected:
  enum State {Uninitialized, Buddies, Games, Arenas, Chat};

  void OnClickModeButton (CGUIMessage& aMessage);
  void OnClickAddButton (CGUIMessage& aMessage);
  void OnClickRemoveButton (CGUIMessage& aMessage);
  void OnClickSpeexButton (CGUIMessage& aMessage);
  void OnClickInviteButton (CGUIMessage& aMessage);
  void OnClickJoinButton (CGUIMessage& aMessage);
  void OnClickPlayButton (CGUIMessage& aMessage);
  void OnClickHostButton (CGUIMessage& aMessage);
  void OnClickKeyboardButton (CGUIMessage& aMessage);
  void OnClickListItem (CGUIMessage& aMessage);
  void OnClickTabFriends (CGUIMessage& aMessage);
  void OnClickTabGames (CGUIMessage& aMessage);
  void OnClickTabArena (CGUIMessage& aMessage);
  void OnClickTabChat (CGUIMessage& aMessage);
  void OnSelectListItem (CGUIMessage& aMessage);

  void QueryInstalledGames();
  bool GetGamePathFromTitleId(DWORD aTitleId, CStdString& aGamePath);
  void UpdatePanel();
  void UpdateFriends();
  void UpdateArena();

  void UpdateGamesPlayerCount();
  void UpdatePlayAndHost();

  void PreviousView();
  void NextView();
  void ChangeState(State aNewState);

  void Enter(CArenaItem& aArena);
  void Play(CStdString& aVector);
  void SelectTab(int nTabId);
  void FlickerTab(int nTabId);

  static bool SortFriends(CGUIItem* pStart, CGUIItem* pEnd);
  static bool SortGames(CGUIItem* pStart, CGUIItem* pEnd);
  static bool SortArena(CGUIItem* pStart, CGUIItem* pEnd);

  CBuddyItem* GetBuddySelection();
  CArenaItem* GetArenaSelection();
  CGUIImage* GetCurrentAvatar();

  struct Invitation
  {
    CStdString vector;
    CStdString time;
    CStdString message;
  };

  CGUIList m_friends;
  CGUIList m_arena;
  CGUIList m_games;
  State window_state;

  CGUIImage* m_pOpponentImage;
  CGUIImage* m_pCurrentAvatar;
  CBuddyItem* m_pMe;

  CKaiClient* m_pKaiClient;
  CGUIConsoleControl* m_pConsole;

  BOOL m_bContactNotifications;

  DWORD m_dwGamesUpdateTimer;
  DWORD m_dwArenaUpdateTimer;

  typedef std::map<CStdString, Invitation> INVITETABLE;
  INVITETABLE m_invitations;

  CKaiVector m_vectors;

  typedef std::map<CStdString, DWORD> TITLETABLE;
  TITLETABLE m_titles;
};
