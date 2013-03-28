/*
 *      Copyright (C) 2011 Plex
 *      http://www.plexapp.com
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

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread/mutex.hpp>

#include "CharsetConverter.h"
#include "Key.h"
#include "TimeUtils.h"
#include "FileItem.h"
#include "GUIBaseContainer.h"
#include "GUIInfoManager.h"
#include "GUILabelControl.h"
#include "GUIWindowPlexSearch.h"
#include "GUIUserMessages.h"
#include "PlexContentWorker.h"
#include "PlexDirectory.h"
#include "Client/PlexServerManager.h"
#include "Settings.h"
#include "Util.h"
#include "PlexUtils.h"
#include "input/XBMC_vkeys.h"

#define CTL_LABEL_EDIT       310
#define CTL_BUTTON_BACKSPACE 8
#define CTL_BUTTON_CLEAR     10
#define CTL_BUTTON_SPACE     32

#define SEARCH_DELAY         750

///////////////////////////////////////////////////////////////////////////////
CGUIWindowPlexSearch::CGUIWindowPlexSearch()
  : CGUIWindow(WINDOW_PLEX_SEARCH, "PlexSearch.xml")
  , m_lastSearchUpdate(0)
  , m_lastArrowKey(0)
  , m_resetOnNextResults(false)
  , m_selectedContainerID(-1)
  , m_selectedItem(-1)
{
  // Initialize results lists.
  m_categoryResults[PLEX_METADATA_MOVIE] = Group(kVIDEO_LOADER);
  m_categoryResults[PLEX_METADATA_SHOW] = Group(kVIDEO_LOADER);
  m_categoryResults[PLEX_METADATA_EPISODE] = Group(kVIDEO_LOADER);
  m_categoryResults[PLEX_METADATA_ARTIST] = Group(kMUSIC_LOADER);
  m_categoryResults[PLEX_METADATA_ALBUM] = Group(kMUSIC_LOADER);
  m_categoryResults[PLEX_METADATA_TRACK] = Group(kMUSIC_LOADER);
  m_categoryResults[PLEX_METADATA_PERSON] = Group(kVIDEO_LOADER);
  m_categoryResults[PLEX_METADATA_CLIP] = Group(kVIDEO_LOADER);
  
  // Create the worker. We're not going to destroy it because whacking it on exit can cause problems.
  m_workerManager = new PlexContentWorkerManager();
}

///////////////////////////////////////////////////////////////////////////////
CGUIWindowPlexSearch::~CGUIWindowPlexSearch()
{
}

///////////////////////////////////////////////////////////////////////////////
void CGUIWindowPlexSearch::OnInitWindow()
{
  CGUILabelControl* pEdit = ((CGUILabelControl*)GetControl(CTL_LABEL_EDIT));

  if (m_selectedItem != -1)
  {
    // Convert back to utf8.
    CStdString utf8Edit;
    g_charsetConverter.wToUTF8(m_strEdit, utf8Edit);
    pEdit->SetLabel(utf8Edit);
    pEdit->SetCursorPos(utf8Edit.size());

    // Bind the lists.
    Bind();

    // Select group.
    CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), m_selectedContainerID);
    OnMessage(msg);

    // Select item.
    CGUIMessage msg2(GUI_MSG_ITEM_SELECT, GetID(), m_selectedContainerID, m_selectedItem);
    OnMessage(msg2);

    m_selectedContainerID = -1;
    m_selectedItem = -1;
  }
  else
  {
    // Reset the view.
    Reset();
    m_strEdit = "";
    UpdateLabel();

    // Select button.
    CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), 65);
    OnMessage(msg);
  }
}

///////////////////////////////////////////////////////////////////////////////
bool CGUIWindowPlexSearch::OnAction(const CAction &action)
{
  CStdString strAction = action.GetName();
  strAction = strAction.ToLower();
  
  // Eat returns.
  if (action.GetUnicode() == 13 && GetFocusedControlID() < 9000)
  {
    return true;
  }
  else if (action.GetID() == ACTION_PREVIOUS_MENU ||
           (action.GetID() == ACTION_PARENT_DIR && m_strEdit.size() == 0))
  {
    g_windowManager.PreviousWindow();
    return true;
  }
  else if (action.GetID() == ACTION_PARENT_DIR || action.GetID() == ACTION_BACKSPACE)
  {
    Backspace();
    return true;
  }
  else if (action.GetID() == ACTION_MOVE_LEFT || action.GetID() == ACTION_MOVE_RIGHT ||
           action.GetID() == ACTION_MOVE_UP   || action.GetID() == ACTION_MOVE_DOWN  ||
           action.GetID() == ACTION_PAGE_UP   || action.GetID() == ACTION_PAGE_DOWN ||
           // action.wID == ACTION_HOME      || action.wID == ACTION_END)
           action.GetID() == ACTION_SELECT_ITEM)
  {
    // If we're going to reset the search time, then make sure we track time since the key.
    if (GetFocusedControlID() < 9000 && m_lastSearchUpdate != 0)
      m_lastArrowKey = XbmcThreads::SystemClockMillis();
    
    // Reset search time.
    m_lastSearchUpdate = 0;

    // Allow cursor keys to work.
    return CGUIWindow::OnAction(action);
  }
  else if (action.GetID() >= KEY_VKEY && action.GetID() < KEY_ASCII)
  { // input from the keyboard (vkey, not ascii)
    uint8_t b = action.GetID() & 0xFF;
    if (b == XBMCVK_RETURN || b == XBMCVK_NUMPADENTER)
    {
      return CGUIWindow::OnAction(action);
    }
    else if (b == XBMCVK_DELETE)
    {
      if (GetCursorPos() < m_strEdit.GetLength())
      {
        MoveCursor(1);
        Backspace();
      }
    }
    else if (b == XBMCVK_BACK) Backspace();
    else if (b == XBMCVK_ESCAPE) Close();
  }
  else if (action.GetID() >= KEY_ASCII)
  {
    int ch = action.GetUnicode();

    // Input from the keyboard.
    switch (ch)
    {
    case 0x1B: // escape.
      Close();
      break;
    case 0x0:
      return false;
    default:
      Character(action.GetUnicode());
    }
    return true;
  }

  return false;
}

///////////////////////////////////////////////////////////////////////////////
bool CGUIWindowPlexSearch::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_SEARCH_HELPER_COMPLETE:
  {
    PlexContentWorkerPtr worker = m_workerManager->find(message.GetParam1());
    if (worker)
    {
      //printf("Processing results from worker: %d.\n", worker->getID());
      CFileItemListPtr results = worker->getResults();

      int lastFocusedList = -1;
      if (m_resetOnNextResults)
      {
        // Get the last focused list.
        lastFocusedList = GetFocusedControlID();
        if (lastFocusedList < 9000)
          lastFocusedList = -1;

        Reset();
        m_resetOnNextResults = false;
      }

      /*
      // If we have any additional providers, run them in parallel.
      vector<CFileItemPtr>& providers = results->GetProviders();
      BOOST_FOREACH(CFileItemPtr& provider, providers)
      {
        // Convert back to utf8.
        CStdString search;
        g_charsetConverter.wToUTF8(m_strEdit, search);

        // Create a new worker.
        m_workerManager->enqueue(WINDOW_PLEX_SEARCH, BuildSearchUrl(provider->GetPath(), search), 0);
      }
       */

      // Put the items in the right category.
      for (int i=0; i<results->Size(); i++)
      {
        // Get the item and the type.
        CFileItemPtr item = results->Get(i);
        int type = item->GetProperty("typeNumber").asInteger();

        // Add it to the correct "bucket".
        if (m_categoryResults.find(type) != m_categoryResults.end())
          m_categoryResults[type].list->Add(item);
        else
          printf("Warning, skipping item with category %d.\n", type);
      }

      // Bind all the lists.
      int firstListWithStuff = -1;
      BOOST_FOREACH(int_list_pair pair, m_categoryResults)
      {
        int controlID = 9000 + pair.first;
        CGUIBaseContainer* control = (CGUIBaseContainer* )GetControl(controlID);
        if (control && pair.second.list->Size() > 0)
        {
          if (control->GetRows() != (unsigned)pair.second.list->Size())
          {
            // Save selected item.
            int selectedItem = control->GetSelectedItem();
            
            // Set the list.
            CGUIMessage msg(GUI_MSG_LABEL_BIND, CTL_LABEL_EDIT, controlID, 0, 0, pair.second.list.get());
            OnMessage(msg);
            
            // Restore selected item.
            CONTROL_SELECT_ITEM(control->GetID(), selectedItem);
            
            // Make sure it's visible.
            SET_CONTROL_VISIBLE(controlID);
            SET_CONTROL_VISIBLE(controlID-2000);
          }
          
          if (firstListWithStuff == -1)
            firstListWithStuff = controlID;
        }
      }

      // If we lost focus, restore it.
      if (lastFocusedList > 0)
      {
        CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), firstListWithStuff != -1 ? firstListWithStuff : 70);
        OnMessage(msg);
      }

      // Get thumbs.
      BOOST_FOREACH(int_list_pair pair, m_categoryResults)
        pair.second.loader->Load(*pair.second.list.get());

      // Whack it.
      m_workerManager->destroy(message.GetParam1());
    }
  }
  break;

  case GUI_MSG_WINDOW_DEINIT:
  {
    if (m_videoThumbLoader.IsLoading())
      m_videoThumbLoader.StopThread();

    if (m_musicThumbLoader.IsLoading())
      m_musicThumbLoader.StopThread();
    
    m_workerManager->cancelPending();
  }
  break;

  case GUI_MSG_CLICKED:
  {
    int iControl = message.GetSenderId();
    OnClickButton(iControl);
    return true;
  }
  break;
  }

  return CGUIWindow::OnMessage(message);
}

///////////////////////////////////////////////////////////////////////////////
void CGUIWindowPlexSearch::Render()
{
  // Enough time has passed since a key was pressed.
  if (m_lastSearchUpdate && m_lastSearchUpdate + SEARCH_DELAY < XbmcThreads::SystemClockMillis())
    UpdateLabel();
  
  // Enough time has passed since an arrow key was pressed after we had input.
  if (m_lastArrowKey && m_lastArrowKey + SEARCH_DELAY < XbmcThreads::SystemClockMillis())
    UpdateLabel();

  CGUIWindow::Render();
}

///////////////////////////////////////////////////////////////////////////////
void CGUIWindowPlexSearch::Character(WCHAR ch)
{
  if (!ch)
    return;

  m_strEdit.Insert(GetCursorPos(), ch);
  UpdateLabel();
  MoveCursor(1);
}

///////////////////////////////////////////////////////////////////////////////
void CGUIWindowPlexSearch::Backspace()
{
  int iPos = GetCursorPos();
  if (iPos > 0)
  {
    m_strEdit.erase(iPos - 1, 1);
    MoveCursor(-1);
    UpdateLabel();
  }
}

///////////////////////////////////////////////////////////////////////////////
void CGUIWindowPlexSearch::UpdateLabel()
{
  CGUILabelControl* pEdit = ((CGUILabelControl*)GetControl(CTL_LABEL_EDIT));
  if (pEdit)
  {
    // Convert back to utf8.
    CStdString utf8Edit;
    g_charsetConverter.wToUTF8(m_strEdit, utf8Edit);
    pEdit->SetLabel(utf8Edit);

    // Send off a search message if it's been SEARCH_DELAY since last search.
    DWORD now = XbmcThreads::SystemClockMillis();
    if (!m_lastSearchUpdate || m_lastSearchUpdate + SEARCH_DELAY >= now)
    {
      m_lastSearchUpdate = now;
      m_lastArrowKey = 0;
    }

    if (m_lastSearchUpdate + SEARCH_DELAY < now ||
        (m_lastArrowKey && m_lastArrowKey     + SEARCH_DELAY < now))
    {
      m_lastSearchUpdate = 0;
      m_lastArrowKey = 0;
      StartSearch(utf8Edit);
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
void CGUIWindowPlexSearch::Bind()
{
  // Bind all the lists.
  BOOST_FOREACH(int_list_pair pair, m_categoryResults)
  {
    int controlID = 9000 + pair.first;
    CGUIBaseContainer* control = (CGUIBaseContainer* )GetControl(controlID);
    if (control && pair.second.list->Size() > 0)
    {
      CGUIMessage msg(GUI_MSG_LABEL_BIND, CTL_LABEL_EDIT, controlID, 0, 0, pair.second.list.get());
      OnMessage(msg);

      SET_CONTROL_VISIBLE(controlID);
      SET_CONTROL_VISIBLE(controlID-2000);
    }
    else
    {
      SET_CONTROL_HIDDEN(controlID);
      SET_CONTROL_HIDDEN(controlID-2000);
    }
  }

  // Get thumbs.
  BOOST_FOREACH(int_list_pair pair, m_categoryResults)
    pair.second.loader->Load(*pair.second.list.get());
}

///////////////////////////////////////////////////////////////////////////////
void CGUIWindowPlexSearch::Reset()
{
  // Reset results.
  printf("Resetting results.\n");
  BOOST_FOREACH(int_list_pair pair, m_categoryResults)
  {
    int controlID = 9000 + pair.first;
    CGUIBaseContainer* control = (CGUIBaseContainer* )GetControl(controlID);
    if (control)
    {
      CGUIMessage msg(GUI_MSG_LABEL_RESET, CTL_LABEL_EDIT, controlID);
      OnMessage(msg);

      SET_CONTROL_HIDDEN(controlID);
      SET_CONTROL_HIDDEN(controlID-2000);
    }
  }

  // Fix focus if needed.
  if (GetFocusedControlID() >= 9000)
  {
    CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), 70);
    OnMessage(msg);
  }

  // Reset results.
  BOOST_FOREACH(int_list_pair pair, m_categoryResults)
    pair.second.list->Clear();
}

///////////////////////////////////////////////////////////////////////////////
void CGUIWindowPlexSearch::StartSearch(const string& search)
{
  // Cancel pending requests.
  m_workerManager->cancelPending();

  if (search.empty())
  {
    // Reset the results, we're not searching for anything at the moment.
    Reset();
  }
  else
  {
    if (g_plexServerManager.GetBestServer())
    {
      m_workerManager->enqueue(WINDOW_PLEX_SEARCH, BuildSearchUrl(g_plexServerManager.GetBestServer(), search), 0);
    }
    else
    {
      // Issue the request to the cloud.
      /* FIXME: a Node Server in CPlexServerManager?
      m_workerManager->enqueue(WINDOW_PLEX_SEARCH, BuildSearchUrl("http://node.plexapp.com:32400/system/search", search), 0);
       */
    }
    
    // If we have shared servers, search them too.
    if (g_guiSettings.GetBool("myplex.searchsharedlibraries"))
    {
      PlexServerList sharedServers = g_plexServerManager.GetAllServers(CPlexServerManager::SERVER_SHARED);
      
      BOOST_FOREACH(CPlexServerPtr server, sharedServers)
        m_workerManager->enqueue(WINDOW_PLEX_SEARCH, BuildSearchUrl(server, search), 0);
    }
    
    // Note that when we receive results, we need to clear out the old ones.
    m_resetOnNextResults = true;
  }
}

///////////////////////////////////////////////////////////////////////////////
void CGUIWindowPlexSearch::MoveCursor(int iAmount)
{
  CGUILabelControl* pEdit = ((CGUILabelControl*)GetControl(CTL_LABEL_EDIT));
  if (pEdit)
    pEdit->SetCursorPos(pEdit->GetCursorPos() + iAmount);
}

///////////////////////////////////////////////////////////////////////////////
int CGUIWindowPlexSearch::GetCursorPos() const
{
  const CGUILabelControl* pEdit = (const CGUILabelControl*)GetControl(CTL_LABEL_EDIT);
  if (pEdit)
    return pEdit->GetCursorPos();

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
char CGUIWindowPlexSearch::GetCharacter(int iButton)
{
  if (iButton >= 65 && iButton <= 90)
  {
    // It's a letter.
    return 'A' + (iButton-65);
  }
  else if (iButton >= 91 && iButton <= 100)
  {
    // It's a number.
    return '0' + (iButton-91);
  }

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
void CGUIWindowPlexSearch::OnClickButton(int iButtonControl)
{
  if (iButtonControl == CTL_BUTTON_BACKSPACE)
  {
    Backspace();
  }
  else if (iButtonControl == CTL_BUTTON_CLEAR)
  {
    CGUILabelControl* pEdit = ((CGUILabelControl*)GetControl(CTL_LABEL_EDIT));
    if (pEdit)
    {
      m_strEdit = "";
      UpdateLabel();

      // Convert back to utf8.
      CStdString utf8Edit;
      g_charsetConverter.wToUTF8(m_strEdit, utf8Edit);
      pEdit->SetLabel(utf8Edit);

      // Reset cursor position.
      pEdit->SetCursorPos(0);
    }
  }
  else if (iButtonControl == CTL_BUTTON_SPACE)
  {
    Character(' ');
  }
  else
  {
    char ch = GetCharacter(iButtonControl);
    if (ch != 0)
    {
      // A keyboard button was pressed.
      Character(ch);
    }
    else
    {
      // We'll try to play the selected item.
      PlayFileFromContainer(GetControl(iButtonControl));
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
void CGUIWindowPlexSearch::SaveStateBeforePlay(CGUIBaseContainer* container)
{
  // Save state.
  m_selectedContainerID = container->GetID();
  m_selectedItem = container->GetSelectedItem();
}

///////////////////////////////////////////////////////////////////////////////
string CGUIWindowPlexSearch::BuildSearchUrl(const CPlexServerPtr& server, const string& theQuery)
{
  CURL url = server->BuildURL("/search");
  url.SetOption("query", theQuery);
  return url.Get();
}

///////////////////////////////////////////////////////////////////////////////
bool CGUIWindowPlexSearch::InProgress()
{
  return (m_workerManager->pendingWorkers() > 0);
}
