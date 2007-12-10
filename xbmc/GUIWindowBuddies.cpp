/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GUIWindowBuddies.h"
#include "Application.h"
#include "GUIDialogInvite.h"
#include "GUIDialogHost.h"
#include "GUIConsoleControl.h"
#include "Util.h"
#include "xbox/xbeheader.h"
#include "ProgramDatabase.h"

using namespace XFILE;

#define KAI_CONSOLE_PEN_NORMAL 0
#define KAI_CONSOLE_PEN_ACTION 1
#define KAI_CONSOLE_PEN_SYSTEM 2
#define KAI_CONSOLE_PEN_PRIVATE 3

#define CONTROL_BTNMODE   3012    // Games Button
#define CONTROL_BTNJOIN   3013    // Join Button
#define CONTROL_BTNSPEEX  3016    // Speex Button
#define CONTROL_BTNINVITE  3014    // Invite Button
#define CONTROL_BTNREMOVE  3015    // Remove Button

#define CONTROL_LISTEX   3031
#define CONTROL_BTNPLAY   3033    // Play Button
#define CONTROL_BTNADD   3034    // Add Button
#define CONTROL_BTNHOST   3035    // Host Button
#define CONTROL_BTNKEYBOARD  3036    // Keyboard Button

#define CONTROL_LABELBUDDYWIN 3050
#define CONTROL_LABELUSERNAME 3051    // Xlink Kai Username
#define CONTROL_LABELUPDATED 3052    // Last update time
#define CONTROL_IMAGELOGO  3053    // Xlink Kai Logo

#define CONTROL_IMAGEBUDDYICON1 3060    // Buddy (offline) icon
#define CONTROL_IMAGEBUDDYICON2 3061    // Buddy (online) icon
#define CONTROL_IMAGEOPPONENT 3062    // Opponent icon
#define CONTROL_IMAGEARENA  3063    // Arena icon
#define CONTROL_IMAGEME   3064    // My icon

#define CONTROL_LABELBUDDYNAME 3070    // Buddy Name
#define CONTROL_LABELBUDDYSTAT 3071    // Buddy Game Status
#define CONTROL_LABELBUDDYINVT 3072    // Buddy Invite Status
#define CONTROL_LABELPLAYERCNT 3073    // Arena Player Count

#define CONTROL_KAI_CONSOLE  3074    // Text Chat console
#define CONTROL_KAI_TEXTEDIT 3075    // Text Edit control
#define CONTROL_KAI_TEXTEDAREA 3076    // Text Edit Area

#define CONTROL_KAI_TAB_FRIENDS 3080    // Friends tab button
#define CONTROL_KAI_TAB_GAMES 3081    // Games tab button
#define CONTROL_KAI_TAB_ARENA 3082    // Arena tab button
#define CONTROL_KAI_TAB_CHAT 3083    // Chat tab button

#define KAI_XBOX_ROOT  "Arena/XBox"

bool CGUIWindowBuddies::SortGames(CGUIItem* pStart, CGUIItem* pEnd)
{
  CArenaItem& rpStart = * ((CArenaItem*)pStart);
  CArenaItem& rpEnd = * ((CArenaItem*)pEnd);

  // order by number of players: games with most players at the top of the list.
  if (rpStart.m_nPlayers != rpEnd.m_nPlayers)
  {
    return rpStart.m_nPlayers > rpEnd.m_nPlayers;
  }

  // order alphabetically
  return ( rpStart.GetName().CompareNoCase( rpEnd.GetName() ) < 0 );
}

bool CGUIWindowBuddies::SortArena(CGUIItem* pStart, CGUIItem* pEnd)
{
  CGUIItem& rpStart = *pStart;
  CGUIItem& rpEnd = *pEnd;

  // order by item time: arenas at the top of list, players at bottom.
  if ( rpStart.GetCookie() != rpEnd.GetCookie() )
  {
    return ( rpStart.GetCookie() == CKaiClient::Arena );
  }

  // if both items are players
  if ( rpStart.GetCookie() == CKaiClient::Player &&
       rpEnd.GetCookie() == CKaiClient::Player)
  {
    CBuddyItem& plStart = * ((CBuddyItem*)pStart);
    CBuddyItem& plEnd = * ((CBuddyItem*)pEnd);

    // order by player status: hosters on top, non-hosters at bottom
    if ( plStart.m_nStatus != plEnd.m_nStatus )
    {
      return ( plStart.m_nStatus > plEnd.m_nStatus );
    }

    // order by player comms capability: headsets at the top
    if ( plStart.m_bHeadset != plEnd.m_bHeadset )
    {
      return ( plStart.m_bHeadset );
    }
  }

  // order alphabetically
  return ( rpStart.GetName().CompareNoCase( rpEnd.GetName() ) < 0 );
}

bool CGUIWindowBuddies::SortFriends(CGUIItem* pStart, CGUIItem* pEnd)
{
  CBuddyItem& rpStart = * ((CBuddyItem*)pStart);
  CBuddyItem& rpEnd = * ((CBuddyItem*)pEnd);

  // order by online status: contacts online at the top of the list.
  if (rpStart.m_bIsOnline != rpEnd.m_bIsOnline)
  {
    return rpStart.m_bIsOnline;
  }

  // order alphabetically
  return ( rpStart.GetName().CompareNoCase( rpEnd.GetName() ) < 0 );
}

CGUIWindowBuddies::CGUIWindowBuddies(void)
    : CGUIWindow(WINDOW_BUDDIES, "MyBuddies.xml")
{
  m_pKaiClient = NULL;
  m_pConsole = NULL;

  m_pOpponentImage = NULL;
  m_pCurrentAvatar = NULL;
  m_pMe = NULL;
  m_dwGamesUpdateTimer = 0;
  m_dwArenaUpdateTimer = 0;
  m_bContactNotifications = FALSE;

  m_friends.SetSortingAlgorithm(CGUIWindowBuddies::SortFriends);
  m_arena.SetSortingAlgorithm(CGUIWindowBuddies::SortArena);
  m_games.SetSortingAlgorithm(CGUIWindowBuddies::SortGames);

  ON_CLICK_MESSAGE(CONTROL_BTNMODE, CGUIWindowBuddies, CGUIWindowBuddies::OnClickModeButton);
  ON_CLICK_MESSAGE(CONTROL_BTNADD, CGUIWindowBuddies, CGUIWindowBuddies::OnClickAddButton);
  ON_CLICK_MESSAGE(CONTROL_BTNREMOVE, CGUIWindowBuddies, CGUIWindowBuddies::OnClickRemoveButton);
  ON_CLICK_MESSAGE(CONTROL_BTNSPEEX, CGUIWindowBuddies, CGUIWindowBuddies::OnClickSpeexButton);
  ON_CLICK_MESSAGE(CONTROL_BTNINVITE, CGUIWindowBuddies, CGUIWindowBuddies::OnClickInviteButton);
  ON_CLICK_MESSAGE(CONTROL_BTNJOIN, CGUIWindowBuddies, CGUIWindowBuddies::OnClickJoinButton);
  ON_CLICK_MESSAGE(CONTROL_BTNPLAY, CGUIWindowBuddies, CGUIWindowBuddies::OnClickPlayButton);
  ON_CLICK_MESSAGE(CONTROL_BTNHOST, CGUIWindowBuddies, CGUIWindowBuddies::OnClickHostButton);
  ON_CLICK_MESSAGE(CONTROL_BTNKEYBOARD, CGUIWindowBuddies, CGUIWindowBuddies::OnClickKeyboardButton);
  ON_CLICK_MESSAGE(CONTROL_LISTEX, CGUIWindowBuddies, CGUIWindowBuddies::OnClickListItem);
  ON_CLICK_MESSAGE(CONTROL_KAI_TAB_FRIENDS, CGUIWindowBuddies, CGUIWindowBuddies::OnClickTabFriends);
  ON_CLICK_MESSAGE(CONTROL_KAI_TAB_GAMES, CGUIWindowBuddies, CGUIWindowBuddies::OnClickTabGames);
  ON_CLICK_MESSAGE(CONTROL_KAI_TAB_ARENA, CGUIWindowBuddies, CGUIWindowBuddies::OnClickTabArena);
  ON_CLICK_MESSAGE(CONTROL_KAI_TAB_CHAT, CGUIWindowBuddies, CGUIWindowBuddies::OnClickTabChat);

  ON_SELECTED_MESSAGE(CONTROL_LISTEX, CGUIWindowBuddies, CGUIWindowBuddies::OnSelectListItem);
  window_state = Uninitialized;
}

CGUIWindowBuddies::~CGUIWindowBuddies(void)
{}

void CGUIWindowBuddies::OnWindowUnload()
{
  //  free the resources of the lists if we switch
  //  the another skin
  CGUIList::GUILISTITEMS& friends = m_friends.Lock();
  for (int i = 0; i < (int)friends.size(); ++i)
  {
    CGUIItem* pItem = friends[i];
    pItem->FreeResources();
  }
  m_friends.Release();

  CGUIList::GUILISTITEMS& arena = m_arena.Lock();
  for (int i = 0; i < (int)arena.size(); ++i)
  {
    CGUIItem* pItem = arena[i];
    pItem->FreeResources();
  }
  m_arena.Release();

  CGUIList::GUILISTITEMS& games = m_games.Lock();
  for (int i = 0; i < (int)games.size(); ++i)
  {
    CGUIItem* pItem = games[i];
    pItem->FreeResources();
  }
  m_games.Release();

  // unbind our opponent image + console control
  m_pOpponentImage = NULL;
  m_pConsole = NULL;

  CGUIWindow::OnWindowUnload();
}

void CGUIWindowBuddies::OnWindowLoaded()
{
  //  allocate resources of the listitems if switched
  //  to a new skin when xbmc is already running
  CGUIList::GUILISTITEMS& friends = m_friends.Lock();
  for (int i = 0; i < (int)friends.size(); ++i)
  {
    CGUIItem* pItem = friends[i];
    pItem->AllocResources();
  }
  m_friends.Release();

  CGUIList::GUILISTITEMS& arena = m_arena.Lock();
  for (int i = 0; i < (int)arena.size(); ++i)
  {
    CGUIItem* pItem = arena[i];
    pItem->AllocResources();
  }
  m_arena.Release();

  CGUIList::GUILISTITEMS& games = m_games.Lock();
  for (int i = 0; i < (int)games.size(); ++i)
  {
    CGUIItem* pItem = games[i];
    pItem->AllocResources();
  }
  m_games.Release();

  // bind to the control defined in the xml
  m_pConsole = (CGUIConsoleControl*)GetControl(CONTROL_KAI_CONSOLE);

  // set edit control observer
  CGUIEditControl* pEdit = ((CGUIEditControl*)GetControl(CONTROL_KAI_TEXTEDIT));
  if (pEdit)
  {
    // use inline text edit control
    pEdit->SetObserver(this);
  }

  // bind this image to the control defined in the xml, later we'll use this
  // as a template to determine the size and position of avatars
  m_pOpponentImage = (CGUIImage*)GetControl(CONTROL_IMAGEOPPONENT);

  for (int nTabButtonId = CONTROL_KAI_TAB_FRIENDS; nTabButtonId <= CONTROL_KAI_TAB_CHAT; nTabButtonId++)
  {
    CGUIButtonControl* pButton = ((CGUIButtonControl*)GetControl(nTabButtonId));
    if (pButton)
    {
      pButton->SetTabButton();
    }
  }

  // bind the listcontrol to our friends list
  CGUIMessage msgb(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LISTEX, 0, 0, &m_friends);
  OnMessage(msgb);

  CGUIWindow::OnWindowLoaded();
}

void CGUIWindowBuddies::OnInitWindow()
{
  SET_CONTROL_LABEL( CONTROL_LABELUSERNAME, g_guiSettings.GetString("xlinkkai.username"));
  SET_CONTROL_LABEL( CONTROL_LABELUPDATED, "");
  SET_CONTROL_LABEL( CONTROL_LABELBUDDYNAME, "");
  SET_CONTROL_LABEL( CONTROL_LABELBUDDYSTAT, "");
  SET_CONTROL_LABEL( CONTROL_LABELBUDDYINVT, "");
  SET_CONTROL_LABEL( CONTROL_LABELPLAYERCNT, "");

  // set buddy status icons
  CBuddyItem::SetIcons(12, 12, "buddyitem-headset.png",
                       "buddyitem-chat.png",
                       "buddyitem-ping.png",
                       "buddyitem-invite.png",
                       "buddyitem-play.png",
                       "buddyitem-idle.png",
                       "buddyitem-host.png",
                       "buddyitem-keyboard.png");

  CArenaItem::SetIcons(12, 12, "arenaitem-private.png");

  // initialize our window, and set our state
  CGUIWindow::OnInitWindow();

  // set the window state after we have init'd our window
  // so that control visibility is set correctly (else any <visible> tags
  // in controls will override these)
  if (window_state == Uninitialized)
    ChangeState(Buddies);
  else
    ChangeState(window_state);

  if (m_pKaiClient)
  {
    while (!m_pKaiClient->IsEngineConnected())
    {
      m_bContactNotifications = FALSE;
      CONTROL_DISABLE(CONTROL_BTNMODE);

      if (CGUIDialogYesNo::ShowAndGetInput(15000, 15001, 15002, 0))
      {
        m_pKaiClient->Reattach();
        Sleep(3000);
      }
      else
      {
        m_gWindowManager.PreviousWindow();
        return;
      }
    }

    if (m_pKaiClient->IsEngineConnected())
    {
      if (m_vectors.IsEmpty())
      {
        CStdString kaiVectors;
        CUtil::AddFileToFolder(g_settings.GetUserDataFolder(), "kai-vectors.xml", kaiVectors);
        m_vectors.Load(kaiVectors);
        QueryInstalledGames();
      }

      CONTROL_ENABLE(CONTROL_BTNMODE);

      if (m_pMe == NULL)
      {
        CStdString strXtag = g_guiSettings.GetString("xlinkkai.username");
        m_pMe = new CBuddyItem(strXtag);

        if (!m_pMe->m_pAvatar)
        {
          m_pKaiClient->QueryAvatar(strXtag);
        }
      }
    }
  }
  else
  {
    CGUIDialogOK::ShowAndGetInput(15000, 0, 14073, 0);
    m_gWindowManager.PreviousWindow();
  }
}

// Called just as soon as this window has be assigned to the CKaiClient as an observer
void CGUIWindowBuddies::OnInitialise(CKaiClient* pClient)
{
  m_pKaiClient = pClient;
}

void CGUIWindowBuddies::OnDeInitialise()
{
  m_pKaiClient = NULL;
}

CBuddyItem* CGUIWindowBuddies::GetBuddySelection()
{
  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_LISTEX, 0, 0, NULL);
  g_graphicsContext.SendMessage(msg);
  //return dynamic_cast<CBuddyItem*>((CGUIListExItem*)msg.GetLPVOID());

  CGUIListExItem* pItem = (CGUIListExItem*)msg.GetLPVOID();
  if (pItem == NULL)
    return NULL;

  return (pItem->GetCookie() == CKaiClient::Player) ?
         (CBuddyItem*)pItem : NULL;
}

CArenaItem* CGUIWindowBuddies::GetArenaSelection()
{
  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_LISTEX, 0, 0, NULL);
  g_graphicsContext.SendMessage(msg);

  CGUIListExItem* pItem = (CGUIListExItem*)msg.GetLPVOID();
  if (pItem == NULL)
    return NULL;

  return (pItem->GetCookie() == CKaiClient::Arena) ?
         (CArenaItem*)pItem : NULL;
}

void CGUIWindowBuddies::OnClickModeButton(CGUIMessage& aMessage)
{
  NextView();
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
    CStdString contact = pBuddy->GetName();  
    m_pKaiClient->EnableContactVoice(contact, !pBuddy->m_bSpeex);
  }
}

void CGUIWindowBuddies::OnClickInviteButton(CGUIMessage& aMessage)
{
  CBuddyItem* pBuddy;
  if ( (pBuddy = GetBuddySelection()) )
  {
    CGUIDialogInvite & dialog = *((CGUIDialogInvite*)m_gWindowManager.GetWindow(WINDOW_DIALOG_INVITE));
    dialog.SetGames(&m_games);
    dialog.DoModal();
    dialog.Close();

    if (dialog.IsConfirmed())
    {
      CStdString aVector;
      if (dialog.GetSelectedVector(aVector))
      {
        CStdString contact = pBuddy->GetName();  
        CStdString strMessage = "";
        dialog.GetPersonalMessage(strMessage);
        m_pKaiClient->Invite(contact, aVector, strMessage);
      }
    }
  }
}

void CGUIWindowBuddies::OnClickJoinButton(CGUIMessage& aMessage)
{
  CBuddyItem* pBuddy;
  if ( (pBuddy = GetBuddySelection()) )
  {
    ChangeState(Arenas);
    SET_CONTROL_FOCUS( CONTROL_LISTEX, 0);
    CStdString strPassword = "";
    m_pKaiClient->EnterVector(pBuddy->m_strVector, strPassword);
  }
}

void CGUIWindowBuddies::OnClickPlayButton(CGUIMessage& aMessage)
{
  switch (window_state)
  {
  case Arenas:
    {
      CStdString currentVector = m_pKaiClient->GetCurrentVector();  
      Play(currentVector);
      break;
    }
  case Games:
    {
      CArenaItem* pArena;
      if ( (pArena = GetArenaSelection()) )
      {
        Enter(*pArena);
      }
      else
      {
        CStdString currentVector = m_pKaiClient->GetCurrentVector();  
        Play(currentVector);
      }
      break;
    }
  }
}

void CGUIWindowBuddies::OnClickHostButton(CGUIMessage& aMessage)
{
  CGUIDialogHost& dialog = *((CGUIDialogHost*)m_gWindowManager.GetWindow(WINDOW_DIALOG_HOST));

  dialog.DoModal();
  dialog.Close();

  if (dialog.IsOK())
  {
    if (dialog.IsPrivate())
    {
      CStdString strPassword;
      CStdString strDescription;
      INT nPlayerLimit;

      dialog.GetConfiguration(strPassword, strDescription, nPlayerLimit);

      m_pKaiClient->Host(strPassword, strDescription, nPlayerLimit);
    }
    else
    {
      m_pKaiClient->Host();
    }
  }
}

void CGUIWindowBuddies::OnEditTextComplete(CStdString& strLineOfText)
{
  CStdString strMessage = strLineOfText.Trim();
  if (strMessage.length() > 0)
  {
    m_pKaiClient->Chat(strMessage);
  }
}

void CGUIWindowBuddies::OnClickKeyboardButton(CGUIMessage& aMessage)
{
  CStdString strMessage;
  CStdString strHeading = g_localizeStrings.Get(15003); // Enter your message.
  if (CGUIDialogKeyboard::ShowAndGetInput(strMessage, strHeading, false))
  {
    m_pKaiClient->Chat(strMessage);
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
  if ( window_state == Buddies && (pBuddy = GetBuddySelection()) )
  {
    // if we clicked on a buddy and we're in Friends mode
    Invitation & invite = m_invitations[pBuddy->GetName()];
    if (invite.vector.length() > 0 && pBuddy->m_bInvite)
    {
      // stop flashing envelope icon
      pBuddy->m_bInvite = FALSE;

      // if that buddy had sent us an invite
      CStdString strAccepted = "Invite accepted!";
      CStdString strDate;
      CStdString strFormat = g_localizeStrings.Get(15005); // Date: %s
      CStdString strFrom;
      CStdString strFormat1 = g_localizeStrings.Get(15006); // From: %s
      strDate.Format(strFormat.c_str(), invite.time);
      strFrom.Format(strFormat1.c_str(), pBuddy->GetName());

      if (invite.message.Compare(strAccepted) == 0)
      {
        // if that invite was actually a receipt of acceptance, display it
        CGUIDialogOK* pDialog = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
        pDialog->SetHeading(15007); // Invitation
        pDialog->SetLine(0, strDate);
        pDialog->SetLine(1, strFrom);
        pDialog->SetLine(2, g_localizeStrings.Get(15059)); // Invite accepted!

        pDialog->DoModal();
      }
      else
      {
        // if this is an invite, display it
        CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);
        pDialog->SetHeading(15007); // Invitation
        pDialog->SetLine(0, strDate);
        pDialog->SetLine(1, strFrom);

        CStdString strMsg = invite.message;

        CStdString strGame;
        CArenaItem::GetTier(CArenaItem::Game, invite.vector, strGame);

        if (strMsg.length() == 0)
        {
          if (strGame.length() > 0)
          {
            CStdString strFormat = g_localizeStrings.Get(15004);
            strMsg.Format(strFormat.c_str(), strGame); // "You've been invited to play %s."
          }
          else
          {
            CStdString strFormat = g_localizeStrings.Get(15008);
            strMsg.Format(strFormat.c_str(), pBuddy->GetName()); // "You've been invited to join %s."
          }
        }

        CStdString strInviteMsg;
        strInviteMsg.Format("You're invited to play %s.", strGame);
        if (strInviteMsg == strMsg)
        {
          CStdString strFormat = g_localizeStrings.Get(15061); // You're invited to play %s.
          strMsg.Format(strFormat.c_str(), strGame);
        }

        pDialog->SetLine(2, strMsg);

        pDialog->DoModal();

        if (pDialog->IsConfirmed())
        {
          // if the user accepted the invitation, send a reciept
          CStdString name = pBuddy->GetName();
          m_pKaiClient->Invite(name, invite.vector, strAccepted);

          // join the arena
          ChangeState(Arenas);
          CStdString strPassword = "";
          m_pKaiClient->EnterVector(invite.vector, strPassword );
        }
      }

      // erase the invite
      invite.vector = "";
    }
  }
}

void CGUIWindowBuddies::SelectTab(int nTabId)
{
  for (int nTabButtonId = CONTROL_KAI_TAB_FRIENDS; nTabButtonId <= CONTROL_KAI_TAB_CHAT; nTabButtonId++)
  {
    CGUIButtonControl* pButton = ((CGUIButtonControl*)GetControl(nTabButtonId));
    if (pButton)
    {
      pButton->SetSelected( nTabId == nTabButtonId );
      pButton->SetAlpha( nTabId == nTabButtonId ? 255 : 179 );
    }
  }
}
void CGUIWindowBuddies::FlickerTab(int nTabId)
{
  CGUIButtonControl* pButton = ((CGUIButtonControl*)GetControl(nTabId));
  if (pButton)
  {
    pButton->Flicker();
  }
}

void CGUIWindowBuddies::OnClickTabFriends(CGUIMessage& aMessage)
{
  ChangeState(Buddies);
}

void CGUIWindowBuddies::OnClickTabGames(CGUIMessage& aMessage)
{
  ChangeState(Games);
}

void CGUIWindowBuddies::OnClickTabArena(CGUIMessage& aMessage)
{
  ChangeState(Arenas);
}

void CGUIWindowBuddies::OnClickTabChat(CGUIMessage& aMessage)
{
  ChangeState(Chat);
}

void CGUIWindowBuddies::OnSelectListItem(CGUIMessage& aMessage)
{}

CGUIImage* CGUIWindowBuddies::GetCurrentAvatar()
{
  CKaiItem* pItem;

  if ( (pItem = GetBuddySelection()) || (pItem = GetArenaSelection()) )
  {
    // a item is selected, and his avatar is different to the one we last showed
    if (pItem->m_pAvatar != m_pCurrentAvatar)
    {
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
          m_pCurrentAvatar->SetPosition(m_pOpponentImage->GetXPosition(), m_pOpponentImage->GetYPosition());
          m_pCurrentAvatar->SetWidth(m_pOpponentImage->GetWidth());
          m_pCurrentAvatar->SetHeight(m_pOpponentImage->GetHeight());
        }
      }
    }
  }
  else
  {
    // no item is selected so free up the current avatar if we have one
    if (m_pCurrentAvatar)
    {
      m_pCurrentAvatar->FreeResources();
      m_pCurrentAvatar = NULL;
    }
  }

  return m_pCurrentAvatar;
}

bool CGUIWindowBuddies::OnMessage(CGUIMessage &message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      if (m_pCurrentAvatar)
      {
        m_pCurrentAvatar->FreeResources();
        m_pCurrentAvatar = NULL;
      }

      if (m_pMe)
      {
        if (m_pMe->m_pAvatar)
        {
          m_pMe->m_pAvatar->FreeResources();
        }
        delete m_pMe;
        m_pMe = NULL;
      }

      CBuddyItem::FreeIcons();
      CArenaItem::FreeIcons();
    }
    break;
  case GUI_MSG_WINDOW_INIT:
    {
      // check to see if we've come here with a string parameter
      CStdString dest = message.GetStringParam();
      if (dest.CompareNoCase("chat") == 0)
        window_state = Chat;
      else if (dest.CompareNoCase("arenas") == 0)
        window_state = Arenas;
      else if (dest.CompareNoCase("buddies") == 0)
        window_state = Buddies;
      else if (dest.CompareNoCase("games") == 0)
        window_state = Games;
    }
    break;
  }
  return CGUIWindow::OnMessage(message);
}


bool CGUIWindowBuddies::OnAction(const CAction &action)
{
  if (window_state == Chat && (action.wID & KEY_ASCII || action.wID & KEY_VKEY) )
  {
    CGUIEditControl* pEdit = ((CGUIEditControl*)GetControl(CONTROL_KAI_TEXTEDIT));
    if (pEdit)
    {
      // use inline text edit control
      pEdit->OnKeyPress(action); // FIXME TESTME: NEW/CHANGE parameter and NOT tested CAN'T do it/DON'T know where (window 2700)/how exactly 
    }
    else
    { // use virtual keyboard
      CGUIMessage dummy(0, 0, 0);
      OnClickKeyboardButton(dummy);
    }
    return true;
  }

  switch (action.wID)
  {
  case ACTION_PARENT_DIR:
    {
      switch (window_state)
      {
      case Arenas:
        {
          INT arenaDelimiter = m_pKaiClient->GetCurrentVector().ReverseFind('/') + 1;
          if (arenaDelimiter == 0)
          {
            m_gWindowManager.PreviousWindow();
            return true;
          }
          m_pKaiClient->ExitVector();
          UpdatePlayAndHost();
          break;
        }
      }
      return true;
      break;
    }
  case ACTION_PREVIOUS_MENU:
    {
      m_gWindowManager.PreviousWindow();
      return true;
      break;
    }
  case ACTION_PREV_PICTURE:
    {
      PreviousView();
      return true;
      break;
    }
  case ACTION_NEXT_PICTURE:
    {
      NextView();
      return true;
      break;
    }
  default:
    {
      return CGUIWindow::OnAction(action);
      break;
    }
  }
}

void CGUIWindowBuddies::UpdatePanel()
{
  CBuddyItem* pBuddy = GetBuddySelection();

  CONTROL_DISABLE(CONTROL_BTNJOIN);
  CONTROL_DISABLE(CONTROL_BTNSPEEX);
  CONTROL_DISABLE(CONTROL_BTNINVITE);
  CONTROL_DISABLE(CONTROL_BTNREMOVE);
  CONTROL_DISABLE(CONTROL_BTNADD);

  if (pBuddy != NULL)
  {
    CONTROL_ENABLE(CONTROL_BTNADD);
    CONTROL_ENABLE(CONTROL_BTNREMOVE);
    //CONTROL_ENABLE(CONTROL_BTNSPEEX);

    if (pBuddy->m_bIsOnline)
    {
      CONTROL_ENABLE(CONTROL_BTNINVITE);
      if (pBuddy->m_strVector != "Home")
      {
        CONTROL_ENABLE(CONTROL_BTNJOIN);
      }
    }
  }

  CStdString strEmpty = "";
  SET_CONTROL_LABEL(CONTROL_LABELBUDDYNAME, strEmpty);
  SET_CONTROL_LABEL(CONTROL_LABELBUDDYSTAT, strEmpty);
}


void CGUIWindowBuddies::Render()
{
  CGUIWindow::Render();
  if (!m_pKaiClient) return;

  m_dwGamesUpdateTimer++;
  m_dwArenaUpdateTimer++;

  // Every 5 minutes
  if (m_dwGamesUpdateTimer % 15000 == 0)
  {
    // Update the game list player count.
    UpdateGamesPlayerCount();
  }

  // Every minute
  if (m_dwArenaUpdateTimer % 3000 == 0)
  {
    // Save the vector table (if it changed).
    CStdString kaiVectors;
    CUtil::AddFileToFolder(g_settings.GetUserDataFolder(), "kai-vectors.xml", kaiVectors);
    m_vectors.Save(kaiVectors);

    // Sort the arena.
    m_arena.Sort();
  }

  // Update buttons
  UpdatePanel();

  if (m_pMe && m_pMe->m_pAvatar)
  {
    if (!m_pMe->m_bProfileRequested)
    {
      m_pMe->m_bProfileRequested = TRUE;

//      SET_CONTROL_HIDDEN(CONTROL_IMAGEME);

      CGUIImage& placeholder = *(CGUIImage*)GetControl(CONTROL_IMAGEME);
      placeholder.SetFileName(m_pMe->m_pAvatar->GetFileName());
//      m_pMe->m_pAvatar->SetPosition(x, y);
//      m_pMe->m_pAvatar->SetWidth((int)w);
//      m_pMe->m_pAvatar->SetHeight((int)h);
//      m_pMe->m_pAvatar->AllocResources();
    }
    else
    {
//      m_pMe->m_pAvatar->Render();
    }
  }

  // Request buddy selection if needed
  CBuddyItem* pBuddy = GetBuddySelection();
  if (pBuddy)
  {
    BOOL bItemFocusedLongEnough = (pBuddy->GetFramesFocused() > 100);

    if (bItemFocusedLongEnough && !pBuddy->m_bProfileRequested)
    {
      pBuddy->m_bProfileRequested = TRUE;
      CStdString aName = pBuddy->GetName();
      m_pKaiClient->QueryUserProfile(aName);

      if (!pBuddy->m_pAvatar)
      {
        m_pKaiClient->QueryAvatar(aName);
      }
    }
  }

  // Update mode/item selection specific labels
  switch (window_state)
  {
  case Buddies:
    {
      UpdateFriends();
      break;
    }
  case Games:
  case Arenas:
    {
      UpdateArena();
      break;
    }
  }

  // Update current Arena name
  CStdString currentVector = m_pKaiClient->GetCurrentVector();
  INT arenaDelimiter = currentVector.ReverseFind('/') + 1;
  CStdString arenaName = arenaDelimiter > 0 ? currentVector.Mid(arenaDelimiter) : "Home" ;
  if (arenaName.CompareNoCase("xbox") == 0) arenaName.ToUpper();
  SET_CONTROL_LABEL( CONTROL_LABELUPDATED, arenaName);
}

void CGUIWindowBuddies::UpdateFriends()
{
  CBuddyItem* pBuddy = GetBuddySelection();

  SET_CONTROL_HIDDEN(CONTROL_IMAGEBUDDYICON1);
  SET_CONTROL_HIDDEN(CONTROL_IMAGEBUDDYICON2);
  SET_CONTROL_HIDDEN(CONTROL_IMAGEARENA);
  SET_CONTROL_HIDDEN(CONTROL_LABELBUDDYNAME);
  SET_CONTROL_HIDDEN(CONTROL_LABELBUDDYSTAT);

  if (pBuddy != NULL)
  {
    CStdString strName = pBuddy->GetName();
    CStdString strLocation = pBuddy->m_strGeoLocation;
    CStdString strStatus = g_localizeStrings.Get(15009); // Offline

    if (pBuddy->m_bIsOnline)
    {
      strStatus = pBuddy->GetArena();
    }

    CStdString strNameAndStatus;
    strNameAndStatus.Format("%s [%s]", strName, strStatus);

    SET_CONTROL_LABEL(CONTROL_LABELBUDDYNAME, strNameAndStatus);
    SET_CONTROL_LABEL(CONTROL_LABELBUDDYSTAT, strLocation);
    SET_CONTROL_VISIBLE(CONTROL_LABELBUDDYNAME);
    SET_CONTROL_VISIBLE(CONTROL_LABELBUDDYSTAT);

    if ( GetCurrentAvatar() )
    {
      m_pCurrentAvatar->Render();
    }
    else
    {
      SET_CONTROL_VISIBLE(pBuddy->m_bIsOnline ? CONTROL_IMAGEBUDDYICON2 : CONTROL_IMAGEBUDDYICON1);
    }
  }
}

void CGUIWindowBuddies::UpdateArena()
{
  CArenaItem* pArena = GetArenaSelection();

  SET_CONTROL_HIDDEN(CONTROL_IMAGEOPPONENT);
  SET_CONTROL_HIDDEN(CONTROL_IMAGEARENA);

  // If item selected is an Arena
  if (pArena != NULL)
  {
    // Update selected Arena name
    CStdString strDisplayText;
    pArena->GetDisplayText(strDisplayText);
    SET_CONTROL_LABEL(CONTROL_LABELBUDDYNAME, strDisplayText);

    // Update selected Arena category
    CStdString strDescription;

    switch (pArena->GetTier())
    {
    case CArenaItem::Custom:
      if (pArena->m_bIsPersonal && pArena->m_strDescription.length() > 0)
      {
        strDescription = pArena->m_strDescription;
      }
      else
      {
        pArena->GetTier(CArenaItem::Game, strDescription);
      }
      break;
    case CArenaItem::Platform:
      break;
    default:
      pArena->GetTier(CArenaItem::Platform, strDescription);
      strDescription.ToUpper();
      break;
    }

    // Update selected Arena icon
    SET_CONTROL_LABEL(CONTROL_LABELBUDDYSTAT, strDescription.c_str());
    SET_CONTROL_VISIBLE(CONTROL_LABELBUDDYSTAT);

    if ( GetCurrentAvatar() )
    {
      m_pCurrentAvatar->Render();
    }
    else
    {
      SET_CONTROL_VISIBLE(CONTROL_IMAGEARENA);
    }
  }
  else
  {
    CBuddyItem* pBuddy = GetBuddySelection();

    // if item selected is an opponent
    if (pBuddy != NULL)
    {
      CStdString opponentInfo;
      opponentInfo.Format("%s (%ums)", pBuddy->GetName().c_str(), pBuddy->m_dwPing);

      SET_CONTROL_LABEL(CONTROL_LABELBUDDYNAME, opponentInfo);

      if (pBuddy->m_strGeoLocation.length())
      {
        SET_CONTROL_LABEL(CONTROL_LABELBUDDYSTAT, pBuddy->m_strGeoLocation);
        SET_CONTROL_VISIBLE(CONTROL_LABELBUDDYSTAT);
      }
      else
      {
        SET_CONTROL_HIDDEN(CONTROL_LABELBUDDYSTAT);
      }

      if ( GetCurrentAvatar() )
      {
        m_pCurrentAvatar->Render();
      }
      else
      {
        SET_CONTROL_VISIBLE(CONTROL_IMAGEOPPONENT);
      }
    }
  }
}

void CGUIWindowBuddies::PreviousView()
{
  switch (window_state)
  {
  case Buddies:
    {
      ChangeState(Chat);
      break;
    }
  case Games:
    {
      ChangeState(Buddies);
      break;
    }
  case Arenas:
    {
      ChangeState(Games);
      break;
    }
  case Chat:
    {
      ChangeState(Arenas);
      break;
    }
  }
}

void CGUIWindowBuddies::NextView()
{
  switch (window_state)
  {
  case Buddies:
    {
      ChangeState(Games);
      break;
    }
  case Games:
    {
      ChangeState(Arenas);
      break;
    }
  case Arenas:
    {
      ChangeState(Chat);
      break;
    }
  case Chat:
    {
      ChangeState(Buddies);
      break;
    }
  }
}

void CGUIWindowBuddies::ChangeState(CGUIWindowBuddies::State aNewState)
{
  window_state = aNewState;

  CGUIButtonControl& mode = *((CGUIButtonControl*)GetControl(CONTROL_BTNMODE));

  switch (window_state)
  {
  case Buddies:
    {
      CGUIMessage msgb(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LISTEX, 0, 0, &m_friends);
      g_graphicsContext.SendMessage(msgb);

      mode.SetNavigation(CONTROL_BTNREMOVE, CONTROL_BTNJOIN, CONTROL_BTNMODE, CONTROL_LISTEX);

      SET_CONTROL_HIDDEN(CONTROL_IMAGEOPPONENT);
      SET_CONTROL_HIDDEN(CONTROL_IMAGEARENA);
      SET_CONTROL_HIDDEN(CONTROL_IMAGEBUDDYICON1);
      SET_CONTROL_HIDDEN(CONTROL_IMAGEBUDDYICON2);

      SET_CONTROL_VISIBLE(CONTROL_LISTEX);
      SET_CONTROL_VISIBLE(CONTROL_BTNMODE);
      SET_CONTROL_VISIBLE(CONTROL_BTNJOIN);
      SET_CONTROL_VISIBLE(CONTROL_BTNSPEEX);
      SET_CONTROL_VISIBLE(CONTROL_BTNINVITE);
      SET_CONTROL_VISIBLE(CONTROL_BTNREMOVE);
      SET_CONTROL_HIDDEN(CONTROL_KAI_CONSOLE);
      SET_CONTROL_HIDDEN(CONTROL_KAI_TEXTEDAREA);
      SET_CONTROL_HIDDEN(CONTROL_KAI_TEXTEDIT);
      SET_CONTROL_HIDDEN(CONTROL_BTNPLAY);
      SET_CONTROL_HIDDEN(CONTROL_BTNADD);
      SET_CONTROL_HIDDEN(CONTROL_BTNHOST);
      SET_CONTROL_HIDDEN(CONTROL_BTNKEYBOARD);
      SET_CONTROL_HIDDEN(CONTROL_LABELPLAYERCNT);

      SET_CONTROL_LABEL(CONTROL_LABELBUDDYWIN, g_localizeStrings.Get(15010)); // Friends
      SET_CONTROL_LABEL(CONTROL_BTNMODE, g_localizeStrings.Get(15011)); // View: Friends
      SET_CONTROL_LABEL(CONTROL_BTNJOIN, g_localizeStrings.Get(15012)); // Join
      SET_CONTROL_LABEL(CONTROL_BTNSPEEX, g_localizeStrings.Get(15013)); // Voice
      SET_CONTROL_LABEL(CONTROL_BTNINVITE, g_localizeStrings.Get(15014)); // Invite
      SET_CONTROL_LABEL(CONTROL_BTNREMOVE, g_localizeStrings.Get(15015)); // Remove

      SelectTab(CONTROL_KAI_TAB_FRIENDS);
      break;
    }

  case Games:
    {
      CGUIMessage msgb(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LISTEX, 0, 0, &m_games);
      g_graphicsContext.SendMessage(msgb);

      mode.SetNavigation(CONTROL_BTNADD, CONTROL_BTNPLAY, CONTROL_BTNMODE, CONTROL_LISTEX);

      SET_CONTROL_HIDDEN(CONTROL_IMAGEBUDDYICON1);
      SET_CONTROL_HIDDEN(CONTROL_IMAGEBUDDYICON2);

      SET_CONTROL_HIDDEN(CONTROL_BTNJOIN);
      SET_CONTROL_HIDDEN(CONTROL_BTNSPEEX);
      SET_CONTROL_HIDDEN(CONTROL_BTNINVITE);
      SET_CONTROL_HIDDEN(CONTROL_BTNREMOVE);
      SET_CONTROL_HIDDEN(CONTROL_BTNKEYBOARD);
      SET_CONTROL_HIDDEN(CONTROL_KAI_CONSOLE);
      SET_CONTROL_HIDDEN(CONTROL_KAI_TEXTEDAREA);
      SET_CONTROL_HIDDEN(CONTROL_KAI_TEXTEDIT);
      SET_CONTROL_VISIBLE(CONTROL_LISTEX);
      SET_CONTROL_VISIBLE(CONTROL_BTNMODE);
      SET_CONTROL_VISIBLE(CONTROL_BTNPLAY);
      SET_CONTROL_VISIBLE(CONTROL_BTNHOST);
      SET_CONTROL_VISIBLE(CONTROL_BTNADD);
      SET_CONTROL_HIDDEN(CONTROL_LABELPLAYERCNT);

      SET_CONTROL_LABEL( CONTROL_LABELBUDDYWIN, g_localizeStrings.Get(15016)); // Games
      SET_CONTROL_LABEL( CONTROL_BTNMODE, g_localizeStrings.Get(15017)); // View: Games
      SET_CONTROL_LABEL(CONTROL_BTNPLAY, g_localizeStrings.Get(15023)); // Play
      SET_CONTROL_LABEL( CONTROL_BTNADD, g_localizeStrings.Get(15019)); // Add
      SET_CONTROL_LABEL( CONTROL_BTNHOST, g_localizeStrings.Get(15020)); // Host
      CONTROL_DISABLE(CONTROL_BTNPLAY);
      CONTROL_DISABLE(CONTROL_BTNADD);
      CONTROL_DISABLE(CONTROL_BTNHOST);

      SelectTab(CONTROL_KAI_TAB_GAMES);
      break;
    }

  case Arenas:
    {
      CGUIMessage msgb(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LISTEX, 0, 0, &m_arena);
      g_graphicsContext.SendMessage(msgb);

      mode.SetNavigation(CONTROL_BTNADD, CONTROL_BTNPLAY, CONTROL_BTNMODE, CONTROL_LISTEX);

      SET_CONTROL_HIDDEN(CONTROL_IMAGEBUDDYICON1);
      SET_CONTROL_HIDDEN(CONTROL_IMAGEBUDDYICON2);

      SET_CONTROL_HIDDEN(CONTROL_BTNJOIN);
      SET_CONTROL_HIDDEN(CONTROL_BTNSPEEX);
      SET_CONTROL_HIDDEN(CONTROL_BTNINVITE);
      SET_CONTROL_HIDDEN(CONTROL_BTNREMOVE);
      SET_CONTROL_HIDDEN(CONTROL_BTNKEYBOARD);
      SET_CONTROL_HIDDEN(CONTROL_KAI_CONSOLE);
      SET_CONTROL_HIDDEN(CONTROL_KAI_TEXTEDAREA);
      SET_CONTROL_HIDDEN(CONTROL_KAI_TEXTEDIT);
      SET_CONTROL_VISIBLE(CONTROL_LISTEX);
      SET_CONTROL_VISIBLE(CONTROL_BTNMODE);
      SET_CONTROL_VISIBLE(CONTROL_BTNPLAY);
      SET_CONTROL_VISIBLE(CONTROL_BTNADD);
      SET_CONTROL_VISIBLE(CONTROL_BTNHOST);
      SET_CONTROL_VISIBLE(CONTROL_LABELPLAYERCNT);

      SET_CONTROL_LABEL(CONTROL_LABELBUDDYWIN, g_localizeStrings.Get(15021)); // Arenas
      SET_CONTROL_LABEL(CONTROL_BTNMODE, g_localizeStrings.Get(15022)); // View: Arenas
      SET_CONTROL_LABEL(CONTROL_BTNPLAY, g_localizeStrings.Get(15023)); // Play
      SET_CONTROL_LABEL(CONTROL_BTNADD, g_localizeStrings.Get(15024)); // Add
      SET_CONTROL_LABEL(CONTROL_BTNHOST, g_localizeStrings.Get(15025)); // Host

      UpdatePlayAndHost();
      CONTROL_DISABLE(CONTROL_BTNADD);

      SelectTab(CONTROL_KAI_TAB_ARENA);

      if (m_pKaiClient->GetCurrentVector().IsEmpty())
      {
        CStdString aRootVector = KAI_XBOX_ROOT;
        CStdString strPassword = "";
        m_pKaiClient->EnterVector(aRootVector, strPassword);
      }

      break;
    }

  case Chat:
    {
      mode.SetNavigation(CONTROL_BTNKEYBOARD, CONTROL_BTNKEYBOARD, CONTROL_BTNMODE, CONTROL_BTNMODE);

      SET_CONTROL_HIDDEN(CONTROL_IMAGEOPPONENT);
      SET_CONTROL_HIDDEN(CONTROL_IMAGEARENA);
      SET_CONTROL_HIDDEN(CONTROL_IMAGEBUDDYICON1);
      SET_CONTROL_HIDDEN(CONTROL_IMAGEBUDDYICON2);

      SET_CONTROL_VISIBLE(CONTROL_KAI_CONSOLE);
      SET_CONTROL_VISIBLE(CONTROL_KAI_TEXTEDAREA);
      SET_CONTROL_VISIBLE(CONTROL_KAI_TEXTEDIT);
      SET_CONTROL_VISIBLE(CONTROL_BTNMODE);
      SET_CONTROL_VISIBLE(CONTROL_BTNKEYBOARD);
      SET_CONTROL_HIDDEN(CONTROL_LISTEX);
      SET_CONTROL_HIDDEN(CONTROL_BTNJOIN);
      SET_CONTROL_HIDDEN(CONTROL_BTNSPEEX);
      SET_CONTROL_HIDDEN(CONTROL_BTNINVITE);
      SET_CONTROL_HIDDEN(CONTROL_BTNREMOVE);
      SET_CONTROL_HIDDEN(CONTROL_BTNPLAY);
      SET_CONTROL_HIDDEN(CONTROL_BTNADD);
      SET_CONTROL_HIDDEN(CONTROL_BTNHOST);
      SET_CONTROL_HIDDEN(CONTROL_LABELPLAYERCNT);

      SET_CONTROL_LABEL( CONTROL_LABELBUDDYWIN, g_localizeStrings.Get(15026)); // Chat
      SET_CONTROL_LABEL( CONTROL_BTNMODE, g_localizeStrings.Get(15027)); // View: Chat
      SET_CONTROL_LABEL( CONTROL_BTNKEYBOARD, g_localizeStrings.Get(15028)); // Keyboard

      SelectTab(CONTROL_KAI_TAB_CHAT);
      break;
    }
  }
}

void CGUIWindowBuddies::Enter(CArenaItem& aArena)
{
  if (aArena.m_bIsPrivate)
  {
    CStdString strHeading = g_localizeStrings.Get(15029); // A password is required to join this arena.
    if (!CGUIDialogKeyboard::ShowAndGetInput(aArena.m_strPassword, strHeading, false))
    {
      return ;
    }
  }

  m_pKaiClient->EnterVector(aArena.m_strVector, aArena.m_strPassword );
  ChangeState(Arenas);
}


/* IBuddyObserver methods */
void CGUIWindowBuddies::OnEngineDetached()
{
  CStdString caption = g_localizeStrings.Get(15000); // XLink Kai
  CStdString description = g_localizeStrings.Get(15030); // Service has disconnected
  g_application.SetKaiNotification(caption, description);
}
void CGUIWindowBuddies::OnAuthenticationFailed(CStdString& aUsername)
{
  CGUIDialogOK* pDialog = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
  pDialog->SetHeading(15031); // XLink Kai Authentication
  pDialog->SetLine(0, 15032); // Your username and/or password was rejected by the
  pDialog->SetLine(1, 15033); // orbital server. Please check your configuration.
  pDialog->SetLine(2, "");;

  ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_OK, m_gWindowManager.GetActiveWindow()};
  g_application.getApplicationMessenger().SendMessage(tMsg, false);
}

void CGUIWindowBuddies::OnNetworkError(CStdString& aError)
{
  CStdString caption = g_localizeStrings.Get(15000); // XLink Kai
  g_application.SetKaiNotification(caption, aError);
}

void CGUIWindowBuddies::OnNetworkReachable(CStdString& aServerName)
{
  CStdString strCaption = g_localizeStrings.Get(15000); // XLink Kai
  g_application.SetKaiNotification(strCaption, aServerName);
}

void CGUIWindowBuddies::OnContactOffline(CStdString& aFriend)
{
  CGUIList::GUILISTITEMS& list = m_friends.Lock();

  CBuddyItem* pBuddy = (CBuddyItem*) m_friends.Find(aFriend);
  if (pBuddy == NULL)
  {
    pBuddy = new CBuddyItem(aFriend);
    pBuddy->m_bIsContact = true;
    m_friends.Add(pBuddy);
  }

  pBuddy->SetIcon(16, 16, "buddyitem-offline.png");
  pBuddy->m_bIsOnline = false;
  pBuddy->m_dwPing = 0;

  m_friends.Sort();
  m_friends.Release();
}

void CGUIWindowBuddies::OnContactsOnline(INT nCount)
{
  m_bContactNotifications = TRUE;
  m_friends.Sort();
}
void CGUIWindowBuddies::OnContactOnline(CStdString& aFriend)
{
  CGUIList::GUILISTITEMS& list = m_friends.Lock();

  CBuddyItem* pBuddy = (CBuddyItem*) m_friends.Find(aFriend);

  if (pBuddy == NULL)
  {
    pBuddy = new CBuddyItem(aFriend);
    pBuddy->m_bIsContact = true;
    m_friends.Add(pBuddy);
  }

  pBuddy->m_bIsOnline = true;

  // if last on, then leave speex on
  if (pBuddy->m_bSpeex)
  {
    m_pKaiClient->EnableContactVoice(aFriend, pBuddy->m_bSpeex);
  }

  pBuddy->SetIcon(16, 16, "buddyitem-online.png");

  m_friends.Sort();
  m_friends.Release();

  if (m_bContactNotifications)
  {
    CStdString note = g_localizeStrings.Get(15034); // has just signed in
    g_application.SetKaiNotification(aFriend, note, pBuddy->m_pAvatar);
  }
}

void CGUIWindowBuddies::OnContactPing(CStdString& aFriend, CStdString& aVector, DWORD aPing, int aStatus, CStdString& aBearerCapability)
{
  bool bSort = false;

  CGUIList::GUILISTITEMS& list = m_friends.Lock();

  CBuddyItem* pBuddy = (CBuddyItem*) m_friends.Find(aFriend);
  if (pBuddy)
  {
    bSort = (pBuddy->m_bIsOnline == false);
    pBuddy->m_bIsOnline = true;
    pBuddy->m_strVector = aVector;
    pBuddy->m_dwPing = aPing;
    pBuddy->m_nStatus = aStatus;
    pBuddy->m_bBusy = aBearerCapability.length() == 0;
    if (!pBuddy->m_bBusy)
    {
      pBuddy->m_bHeadset = aBearerCapability.Find('2') >= 0;
      pBuddy->m_bKeyboard = aBearerCapability.Find('3') >= 0;
    }
  }

  if (bSort)
  {
    m_friends.Sort();
  }

  m_friends.Release();
}

void CGUIWindowBuddies::OnUpdateHostingStatus(BOOL bIsHosting)
{
  CStdString strHost = g_localizeStrings.Get(15058); // Host
  SET_CONTROL_LABEL( CONTROL_LABELPLAYERCNT, bIsHosting ? strHost.c_str() : "");
}

void CGUIWindowBuddies::OnContactRemove(CStdString& aFriend)
{
  if (m_pCurrentAvatar)
  {
    m_pCurrentAvatar->FreeResources();
    m_pCurrentAvatar = NULL;
  }

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
    pBuddy->m_dwRingCounter = 120;

    if (pBuddy->m_bRingIndicator)
    {
      pBuddy->m_bRingIndicator = FALSE;

      CStdString note = g_localizeStrings.Get(15035); // has initiated voice chat
      g_application.SetKaiNotification(aFriend, note, pBuddy->m_pAvatar);
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
    m_invitations[aFriend] = invite;

    pBuddy->m_bInvite = TRUE;

    CStdString note = g_localizeStrings.Get(15036); // has sent you an invitation
    g_application.SetKaiNotification(aFriend, note, pBuddy->m_pAvatar);
  }

  m_friends.Release();
}

void CGUIWindowBuddies::QueryInstalledGames()
{
  m_games.Clear();

  WIN32_FIND_DATA wfd;
  memset(&wfd, 0, sizeof(wfd));

  CAutoPtrFind hFind (FindFirstFile("E:\\tdata\\*.*", &wfd));

  if (hFind.isValid())
  {
    do
    {
      if (wfd.cFileName[0] != 0)
      {
        if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
          DWORD titleId = strtoul((CHAR*)wfd.cFileName, NULL, 16);

          // is this a game title or some homebrew title?
          CHAR firstChar = (CHAR)(titleId >> 24);
          if (firstChar >= 48 && firstChar <= 'z')
          {
            // if its a game title, do we have it in our local mapping file
            CStdString strVector = "";
            if (m_vectors.GetTitle(titleId, strVector))
            {
              OnSupportedTitle(titleId, strVector);
              m_pKaiClient->QueryVectorPlayerCount(strVector);
            }
            else
            {
              // no, so we need to ask kai to resolve it
              m_pKaiClient->QueryVector(titleId);
            }
          }
        }
      }
    }
    while (FindNextFile((HANDLE)hFind, &wfd));
  }
}

void CGUIWindowBuddies::Play(CStdString& aVector)
{
  if (!m_pKaiClient->IsNetworkReachable())
  {
    if (!CGUIDialogYesNo::ShowAndGetInput(15000, 15037, 15038, 20013))
      return;
  }

  CStdString strGame;
  CArenaItem::GetTier(CArenaItem::Game, aVector, strGame);

  // Get TitleId from current vector
  DWORD dwTitleId = m_titles[strGame];

  //make sure we got an id
  if (dwTitleId != 0x00000000)
  {
    bool foundPath = false;
    CStdString strGamePath;

    //first check with the db.
    CProgramDatabase db;
    if (db.Open())
    {
      foundPath = db.GetXBEPathByTitleId(dwTitleId, strGamePath) && CFile::Exists(strGamePath);
      db.Close();
    }

    if (!foundPath)
    {
      // else try searching through our sources
      foundPath = GetGamePathFromTitleId(dwTitleId, strGamePath);
    }

    //finally, if we found the game path..run it!
    if (foundPath && !strGamePath.IsEmpty())
    {
      int iRegion;
      if (g_guiSettings.GetBool("myprograms.gameautoregion"))
      {
        iRegion = db.GetRegion(strGamePath);
        if (iRegion == -1)
        {
          CXBE xbe;
          iRegion = xbe.ExtractGameRegion(strGamePath);
          if (iRegion < 1 || iRegion > 7)
            iRegion = 0;
        }
        iRegion = CXBE::FilterRegion(iRegion);
      }
      else
        iRegion = 0;

      CUtil::RunXBE(strGamePath,NULL,F_VIDEO(iRegion));
      return ;
    }
  }

  CGUIDialogProgress& dialog = *((CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS));

  dialog.SetHeading(strGame.c_str());
  dialog.SetLine(0, 15039); // Xbox Media Center unable to locate game automatically.
  dialog.SetLine(1, 15040); // Insert the game disk into your XBOX or press 'A' to cancel
  dialog.SetLine(2, 15041); // and launch the game manually.

  while (true)
  {
    if (CDetectDVDMedia::IsDiscInDrive())
    {
      if ( CFile::Exists("D:\\default.xbe") )
      {
        g_application.Stop();
        CUtil::LaunchXbe( "Cdrom0", "D:\\default.xbe", NULL );
      }
    }

    // process the dialog
    if (!dialog.IsDialogRunning())
      dialog.StartModal();
    dialog.Progress();
    if (dialog.IsCanceled())
      break;
  }

  dialog.Close();
}

bool CGUIWindowBuddies::GetGamePathFromTitleId(DWORD aTitleId, CStdString& aGamePath)
{
  WIN32_FIND_DATA wfd;
  memset(&wfd, 0, sizeof(wfd));

  // Run through our sources and construct a multi folder path to search
  CStdStringArray gamesDirs;
  for (unsigned int i = 0; i < g_settings.m_programSources.size(); i++)
  {
    CShare &share = g_settings.m_programSources[i];
    for (unsigned int j = 0; j < share.vecPaths.size(); j++)
      gamesDirs.push_back(share.vecPaths[j]);
  }

  for (int i = 0; i < (int)gamesDirs.size(); ++i)
  {
    CStdString gameDir = gamesDirs[i];

    // Search for XBE in within GamesDir subfolders matching same TitleId.
    CStdString strSearchMask;
    strSearchMask.Format("%s\\*.*", gameDir.c_str());
    CAutoPtrFind hFind (FindFirstFile(strSearchMask.c_str(), &wfd));

    if (hFind.isValid())
    {
      do
      {
        if (wfd.cFileName[0] != 0)
        {
          if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
          {
            // Calculate the path of the game's xbe
            CStdString strGamePath;
            strGamePath.Format("%s\\%s\\default.xbe", gameDir.c_str(), (CHAR*)wfd.cFileName);

            // If the XBE actually exists
            if (CFile::Exists(strGamePath))
            {
              // Read its header info
              FILE* hFile = fopen(strGamePath.c_str(), "rb");
              _XBE_HEADER HS;
              fread(&HS, 1, sizeof(HS), hFile);
              fseek(hFile, HS.XbeHeaderSize, SEEK_SET);
              _XBE_CERTIFICATE HC;
              fread(&HC, 1, sizeof(HC), hFile);
              fclose(hFile);

              // Is this the title we're looking for?
              if (aTitleId == HC.TitleId)
              {
                // Store its path and stop searching
                aGamePath = strGamePath;
                break;
              }
            }
          }
        }
      }
      while (FindNextFile((HANDLE)hFind, &wfd));
    }

    //break out of for loop if the game was found
    if (!aGamePath.IsEmpty())
      break;
  }

  return aGamePath.length() > 0;
}

void CGUIWindowBuddies::OnSupportedTitle(DWORD aTitleId, CStdString& aVector)
{
  if (aVector.length() > 0)
  {
    if (!m_vectors.ContainsTitle(aTitleId))
    {
      m_vectors.AddTitle(aTitleId, aVector);
    }

    INT arenaDelimiter = aVector.ReverseFind('/') + 1;
    CStdString arenaLabel = aVector.Mid(arenaDelimiter);

    CStdString strGame;
    CArenaItem::GetTier(CArenaItem::Game, aVector, strGame);
    m_titles[strGame] = aTitleId;

    CGUIList::GUILISTITEMS& list = m_games.Lock();

    CArenaItem* pArena = (CArenaItem*) m_games.Find(arenaLabel);
    if (!pArena)
    {
      pArena = new CArenaItem(arenaLabel);
      pArena->m_strVector = aVector;
      pArena->SetIcon(16, 16, "arenaitem-small.png");

      if (!pArena->m_pAvatar)
      {
        CStdString strSafeVector = aVector;
        strSafeVector.Replace(" ", "%20");
        strSafeVector.Replace(":", "%3A");

        CStdString aAvatarUrl;
        aAvatarUrl.Format("http://www.teamxlink.co.uk/media/avatars/%s.jpg", strSafeVector);
        pArena->SetAvatar(aAvatarUrl);
      }

      m_games.Add(pArena);
      m_games.Sort();
    }

    m_games.Release();

    m_pKaiClient->QueryVectorPlayerCount(aVector);
  }
}

void CGUIWindowBuddies::OnEnterArena(CStdString& aVector, BOOL bCanHost)
{
  if (m_pCurrentAvatar)
  {
    m_pCurrentAvatar->FreeResources();
    m_pCurrentAvatar = NULL;
  }

  m_arena.Clear();

  if (m_pConsole)
  {
    m_pConsole->Clear();
  }

  m_pKaiClient->GetSubVectors(aVector);
}

void CGUIWindowBuddies::OnEnterArenaFailed(CStdString& aVector, CStdString& aReason)
{
  CGUIDialogOK* pDialog = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
  pDialog->SetHeading(15042); // Access Denied
  pDialog->SetLine(0, aReason);
  pDialog->SetLine(1, "");
  pDialog->SetLine(2, "");

  ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_OK, m_gWindowManager.GetActiveWindow()};
  g_application.getApplicationMessenger().SendMessage(tMsg, false);
}

void CGUIWindowBuddies::OnNewArena( CStdString& aVector, CStdString& aDescription,
                                    int nPlayers, int nPlayerLimit, int nPassword, bool bPersonal )

{
  INT arenaDelimiter = aVector.ReverseFind('/') + 1;

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
    pArena->m_bIsPrivate = nPassword > 0;
    pArena->SetIcon(16, 16, "arenaitem-small.png");

    if (!pArena->m_pAvatar)
    {
      CStdString strSafeVector = aVector;
      strSafeVector.Replace(" ", "%20");
      strSafeVector.Replace(":", "%3A");

      CStdString aAvatarUrl;
      aAvatarUrl.Format("http://www.teamxlink.co.uk/media/avatars/%s.jpg", strSafeVector);
      pArena->SetAvatar(aAvatarUrl);
    }

    m_arena.Add(pArena);
    m_arena.Sort();
  }

  m_arena.Release();

  m_pKaiClient->JoinTextChat();
}

void CGUIWindowBuddies::OnUpdateArena( CStdString& aVector, int nPlayers )
{
  INT arenaDelimiter = aVector.ReverseFind('/') + 1;
  CStdString arenaLabel = aVector.Mid(arenaDelimiter);

  //CStdString strDebug;
  //strDebug.Format("KAI: updated %s player count: %d",aVector,nPlayers);
  //CLog::Log(LOGINFO,strDebug.c_str());

  m_arena.Lock();
  CArenaItem* pArena = (CArenaItem*) m_arena.Find(arenaLabel);
  if (pArena)
  {
    pArena->m_nPlayers = nPlayers;
  }
  m_arena.Release();

  m_games.Lock();
  CArenaItem* pGame = (CArenaItem*) m_games.Find(arenaLabel);
  if (pGame)
  {
    pGame->m_nPlayers = nPlayers;
    m_games.Sort();
  }
  m_games.Release();
}

void CGUIWindowBuddies::OnUpdateOpponent(CStdString& aOpponent, CStdString& aAge,
    CStdString& aBandwidth, CStdString& aLocation, CStdString& aBio)
{
  //CStdString strDebug;
  //strDebug.Format("Received profile for %s.\r\n",aOpponent.c_str());
  //OutputDebugString(strDebug.c_str());

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
  // CStdString strDebug;
  // strDebug.Format("Received avatar for %s.\r\n",aOpponent.c_str());
  // OutputDebugString(strDebug.c_str());

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

  // may as well check if this avatar is mine
  if (m_pMe && (m_pMe->GetName() == aOpponent))
  {
    m_pMe->SetAvatar(aAvatarURL);
  }
}

void CGUIWindowBuddies::OnOpponentEnter(CStdString& aOpponent)
{
  CGUIList::GUILISTITEMS& list = m_arena.Lock();

  CBuddyItem* pOpponent = (CBuddyItem*) m_arena.Find(aOpponent);
  if (!pOpponent)
  {
    pOpponent = new CBuddyItem(aOpponent);
    pOpponent->m_bIsOnline = true;
    pOpponent->SetIcon(16, 16, "buddyitem-online.png");

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
    pOpponent->m_dwPing = aPing;
    pOpponent->m_nStatus = aStatus;
    pOpponent->m_bBusy = aBearerCapability.length() == 0;
    if (!pOpponent->m_bBusy)
    {
      pOpponent->m_bHeadset = aBearerCapability.Find('2') >= 0;
      pOpponent->m_bKeyboard = aBearerCapability.Find('3') >= 0;
    }
  }

  m_arena.Release();
}

void CGUIWindowBuddies::OnOpponentLeave(CStdString& aOpponent)
{
  m_arena.Remove(aOpponent);
}

void CGUIWindowBuddies::UpdateGamesPlayerCount()
{
  CArenaItem* pItem = NULL;
  CGUIList::GUILISTITEMS& games = m_games.Lock();
  CGUIList::GUILISTITEMS::iterator iterator = games.begin();

  while (iterator != games.end())
  {
    pItem = (CArenaItem*) (*iterator);
    m_pKaiClient->QueryVectorPlayerCount(pItem->m_strVector);
    iterator++;
  }

  m_games.Release();
}

void CGUIWindowBuddies::OnJoinsChat(CStdString& aOpponent)
{
  if (m_pConsole)
  {
    CStdString strMessage;
    CStdString strFormat = g_localizeStrings.Get(15043); // %s joins chat.
    strMessage.Format(strFormat.c_str(), aOpponent);
    m_pConsole->Write(strMessage, KAI_CONSOLE_PEN_ACTION);
  }
}

void CGUIWindowBuddies::OnChat(CStdString& aVector, CStdString& aOpponent, CStdString& aMessage, bool bPrivate)
{
  if (m_pConsole)
  {
    CStdString strMessage;
    DWORD dwColour = KAI_CONSOLE_PEN_NORMAL;

    if (aOpponent.CompareNoCase("Kai Orbital Mesh") == 0)
    {
      dwColour = KAI_CONSOLE_PEN_SYSTEM;
      strMessage = aMessage;
    }
    else
    {
      // if a player has written a message and we're not in the chat window
      if (window_state != Chat)
      {
        // alert us by flashing the tab briefly
        FlickerTab(CONTROL_KAI_TAB_CHAT);
      }

      dwColour = bPrivate ? KAI_CONSOLE_PEN_PRIVATE : dwColour;
      strMessage.Format("%s: %s", aOpponent, aMessage);
    }

    m_pConsole->Write(strMessage, dwColour);
  }
}
void CGUIWindowBuddies::OnLeavesChat(CStdString& aOpponent)
{
  if (m_pConsole)
  {
    CStdString strMessage;
    CStdString strFormat = g_localizeStrings.Get(15044); // %s leaves chat.
    DWORD dwColour = KAI_CONSOLE_PEN_ACTION;
    strMessage.Format(strFormat.c_str(), aOpponent);
    m_pConsole->Write(strMessage, dwColour);
  }
}

void CGUIWindowBuddies::UpdatePlayAndHost()
{
  // only update if we're active
  if (!IsActive()) return;

  CStdString strGame;
  CArenaItem::GetTier(CArenaItem::Game, CKaiClient::GetInstance()->GetCurrentVector(), strGame);
  int arenaDelimiter = CKaiClient::GetInstance()->GetCurrentVector().ReverseFind('/') + 1;
  CStdString arenaLabel = CKaiClient::GetInstance()->GetCurrentVector().Mid(arenaDelimiter);
  CArenaItem* pItem = (CArenaItem*)m_arena.Find(arenaLabel);

  // This is completely wrong, but is the best I can think of at the moment.
  // According to Sollie, Play and Host should be disabled unless in an arena which allows
  // hosting etc. (eg not in the Halo/ folder, as this is further split into Halo/Americas etc.
  // As there doesn't seem to be any reliable way to get this information at present,
  // this is the best thing we can come up with.  It will fail dismally on backward
  // navigation, as we only rely on the forward navigation for things to work.

  // As the above fails, we'll just always enable these buttons if strGame isn't empty
  if (!strGame.IsEmpty())
  {
    // play should be enabled in personal and public arenas
    CONTROL_ENABLE(CONTROL_BTNPLAY);
/*
    if (pItem->m_bIsPersonal)
    {
      CONTROL_ENABLE(CONTROL_BTNPLAY);
    }
    else
    {
      CONTROL_ENABLE(CONTROL_BTNPLAY);
    }*/
  }
  else
  {
    CONTROL_DISABLE(CONTROL_BTNPLAY);
  }
  if (strGame.IsEmpty() || CKaiClient::GetInstance()->IsHosting())
  {
    CONTROL_DISABLE(CONTROL_BTNHOST);
  }
  else
  {
    CONTROL_ENABLE(CONTROL_BTNHOST);
  }
}

