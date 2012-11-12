/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "threads/SystemClock.h"
#include "GUIMediaWindow.h"
#include "GUIUserMessages.h"
#include "Util.h"
#include "PlayListPlayer.h"
#include "addons/AddonManager.h"
#include "addons/PluginSource.h"
#include "filesystem/PluginDirectory.h"
#include "filesystem/MultiPathDirectory.h"
#include "GUIPassword.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "network/Network.h"
#include "utils/RegExp.h"
#include "PartyModeManager.h"
#include "dialogs/GUIDialogMediaSource.h"
#include "GUIWindowFileManager.h"
#include "Favourites.h"
#include "utils/LabelFormatter.h"
#include "dialogs/GUIDialogProgress.h"
#include "settings/AdvancedSettings.h"
#include "settings/GUISettings.h"
#include "URL.h"

#include "dialogs/GUIDialogSmartPlaylistEditor.h"
#include "addons/GUIDialogAddonSettings.h"
#include "dialogs/GUIDialogYesNo.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogOK.h"
#include "playlists/PlayList.h"
#include "storage/MediaManager.h"
#include "settings/Settings.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "guilib/LocalizeStrings.h"
#include "utils/TimeUtils.h"
#include "filesystem/File.h"
#include "filesystem/FileDirectoryFactory.h"
#include "utils/log.h"
#include "utils/FileUtils.h"
#include "guilib/GUIEditControl.h"
#include "guilib/GUIKeyboardFactory.h"
#ifdef HAS_PYTHON
#include "interfaces/python/XBPython.h"
#endif
#include "interfaces/Builtins.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogMediaFilter.h"
#include "filesystem/SmartPlaylistDirectory.h"
#if defined(TARGET_ANDROID)
#include "xbmc/android/activity/XBMCApp.h"
#endif

#define CONTROL_BTNVIEWASICONS       2
#define CONTROL_BTNSORTBY            3
#define CONTROL_BTNSORTASC           4
#define CONTROL_BTN_FILTER          19

#define CONTROL_LABELFILES          12

#define PROPERTY_PATH_DB            "path.db"
#define PROPERTY_SORT_ORDER         "sort.order"
#define PROPERTY_SORT_ASCENDING     "sort.ascending"

using namespace std;
using namespace ADDON;

CGUIMediaWindow::CGUIMediaWindow(int id, const char *xmlFile)
    : CGUIWindow(id, xmlFile)
{
  m_loadType = KEEP_IN_MEMORY;
  m_vecItems = new CFileItemList;
  m_unfilteredItems = new CFileItemList;
  m_vecItems->SetPath("?");
  m_iLastControl = -1;
  m_iSelectedItem = -1;
  m_canFilterAdvanced = false;

  m_guiState.reset(CGUIViewState::GetViewState(GetID(), *m_vecItems));
}

CGUIMediaWindow::~CGUIMediaWindow()
{
  delete m_vecItems;
  delete m_unfilteredItems;
}

#define CONTROL_VIEW_START        50
#define CONTROL_VIEW_END          59

void CGUIMediaWindow::LoadAdditionalTags(TiXmlElement *root)
{
  CGUIWindow::LoadAdditionalTags(root);
  // configure our view control
  m_viewControl.Reset();
  m_viewControl.SetParentWindow(GetID());
  TiXmlElement *element = root->FirstChildElement("views");
  if (element && element->FirstChild())
  { // format is <views>50,29,51,95</views>
    CStdString allViews = element->FirstChild()->Value();
    CStdStringArray views;
    StringUtils::SplitString(allViews, ",", views);
    for (unsigned int i = 0; i < views.size(); i++)
    {
      int controlID = atol(views[i].c_str());
      CGUIControl *control = (CGUIControl *)GetControl(controlID);
      if (control && control->IsContainer())
        m_viewControl.AddView(control);
    }
  }
  else
  { // backward compatibility
    vector<CGUIControl *> controls;
    GetContainers(controls);
    for (ciControls it = controls.begin(); it != controls.end(); it++)
    {
      CGUIControl *control = *it;
      if (control->GetID() >= CONTROL_VIEW_START && control->GetID() <= CONTROL_VIEW_END)
        m_viewControl.AddView(control);
    }
  }
  m_viewControl.SetViewControlID(CONTROL_BTNVIEWASICONS);
}

void CGUIMediaWindow::OnWindowLoaded()
{
  SendMessage(GUI_MSG_SET_TYPE, CONTROL_BTN_FILTER, CGUIEditControl::INPUT_TYPE_FILTER);
  CGUIWindow::OnWindowLoaded();
  SetupShares();
}

void CGUIMediaWindow::OnWindowUnload()
{
  CGUIWindow::OnWindowUnload();
  m_viewControl.Reset();
}

CFileItemPtr CGUIMediaWindow::GetCurrentListItem(int offset)
{
  int item = m_viewControl.GetSelectedItem();
  if (!m_vecItems->Size() || item < 0)
    return CFileItemPtr();
  item = (item + offset) % m_vecItems->Size();
  if (item < 0) item += m_vecItems->Size();
  return m_vecItems->Get(item);
}

bool CGUIMediaWindow::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_PARENT_DIR)
  {
    GoParentFolder();
    return true;
  }

  // the non-contextual menu can be called at any time
  if (action.GetID() == ACTION_CONTEXT_MENU && !m_viewControl.HasControl(GetFocusedControlID()))
  {
    OnPopupMenu(-1);
    return true;
  }

  if (CGUIWindow::OnAction(action))
    return true;

  if (action.GetID() == ACTION_FILTER)
    return Filter();

  // live filtering
  if (action.GetID() == ACTION_FILTER_CLEAR)
  {
    CGUIMessage message(GUI_MSG_NOTIFY_ALL, GetID(), 0, GUI_MSG_FILTER_ITEMS);
    message.SetStringParam("");
    OnMessage(message);
    return true;
  }

  if (action.GetID() == ACTION_BACKSPACE)
  {
    CGUIMessage message(GUI_MSG_NOTIFY_ALL, GetID(), 0, GUI_MSG_FILTER_ITEMS, 2); // 2 for delete
    OnMessage(message);
    return true;
  }

  if (action.GetID() >= ACTION_FILTER_SMS2 && action.GetID() <= ACTION_FILTER_SMS9)
  {
    CStdString filter;
    filter.Format("%i", (int)(action.GetID() - ACTION_FILTER_SMS2 + 2));
    CGUIMessage message(GUI_MSG_NOTIFY_ALL, GetID(), 0, GUI_MSG_FILTER_ITEMS, 1); // 1 for append
    message.SetStringParam(filter);
    OnMessage(message);
    return true;
  }

  return false;
}

bool CGUIMediaWindow::OnBack(int actionID)
{
  CURL filterUrl(m_strFilterPath);
  if (actionID == ACTION_NAV_BACK && !m_vecItems->IsVirtualDirectoryRoot() &&
     (m_vecItems->GetPath() != m_startDirectory || (m_canFilterAdvanced && filterUrl.HasOption("filter"))))
  {
    GoParentFolder();
    return true;
  }
  return CGUIWindow::OnBack(actionID);
}

bool CGUIMediaWindow::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      m_iSelectedItem = m_viewControl.GetSelectedItem();
      m_iLastControl = GetFocusedControlID();
      CGUIWindow::OnMessage(message);
      CGUIDialogContextMenu* pDlg = (CGUIDialogContextMenu*)g_windowManager.GetWindow(WINDOW_DIALOG_CONTEXT_MENU);
      if (pDlg && pDlg->IsActive())
        pDlg->Close();

      // get rid of any active filtering
      if (m_canFilterAdvanced)
      {
        m_canFilterAdvanced = false;
        m_filter.Reset();
      }
      m_strFilterPath.clear();
      
      // Call ClearFileItems() after our window has finished doing any WindowClose
      // animations
      ClearFileItems();
      return true;
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTNVIEWASICONS)
      {
        // view as control could be a select button
        int viewMode = 0;
        const CGUIControl *control = GetControl(CONTROL_BTNVIEWASICONS);
        if (control && control->GetControlType() != CGUIControl::GUICONTROL_BUTTON)
        {
          CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_BTNVIEWASICONS);
          OnMessage(msg);
          viewMode = m_viewControl.GetViewModeNumber(msg.GetParam1());
        }
        else
          viewMode = m_viewControl.GetNextViewMode();

        if (m_guiState.get())
          m_guiState->SaveViewAsControl(viewMode);

        UpdateButtons();
        return true;
      }
      else if (iControl == CONTROL_BTNSORTASC) // sort asc
      {
        if (m_guiState.get())
          m_guiState->SetNextSortOrder();
        UpdateFileList();
        return true;
      }
      else if (iControl == CONTROL_BTNSORTBY) // sort by
      {
        if (m_guiState.get())
          m_guiState->SetNextSortMethod();
        UpdateFileList();
        return true;
      }
      else if (iControl == CONTROL_BTN_FILTER)
      {
        if (m_canFilterAdvanced)
          return true;

        return Filter();
      }
      else if (m_viewControl.HasControl(iControl))  // list/thumb control
      {
        int iItem = m_viewControl.GetSelectedItem();
        int iAction = message.GetParam1();
        if (iItem < 0) break;
        if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
        {
          OnSelect(iItem);
        }
        else if (iAction == ACTION_CONTEXT_MENU || iAction == ACTION_MOUSE_RIGHT_CLICK)
        {
          OnPopupMenu(iItem);
          return true;
        }
      }
    }
    break;

  case GUI_MSG_SETFOCUS:
    {
      if (m_viewControl.HasControl(message.GetControlId()) && m_viewControl.GetCurrentControl() != message.GetControlId())
      {
        m_viewControl.SetFocused();
        return true;
      }
    }
    break;

  case GUI_MSG_NOTIFY_ALL:
    { // Message is received even if this window is inactive
      if (message.GetParam1() == GUI_MSG_WINDOW_RESET)
      {
        m_vecItems->SetPath("?");
        return true;
      }
      else if ( message.GetParam1() == GUI_MSG_REFRESH_THUMBS )
      {
        for (int i = 0; i < m_vecItems->Size(); i++)
          m_vecItems->Get(i)->FreeMemory(true);
        break;  // the window will take care of any info images
      }
      else if (message.GetParam1() == GUI_MSG_REMOVED_MEDIA)
      {
        if ((m_vecItems->IsVirtualDirectoryRoot() ||
             m_vecItems->IsSourcesPath()) && IsActive())
        {
          int iItem = m_viewControl.GetSelectedItem();
          Refresh();
          m_viewControl.SetSelectedItem(iItem);
        }
        else if (m_vecItems->IsRemovable())
        { // check that we have this removable share still
          if (!m_rootDir.IsInSource(m_vecItems->GetPath()))
          { // don't have this share any more
            if (IsActive()) Update("");
            else
            {
              m_history.ClearPathHistory();
              m_vecItems->SetPath("");
            }
          }
        }

        return true;
      }
      else if (message.GetParam1()==GUI_MSG_UPDATE_SOURCES)
      { // State of the sources changed, so update our view
        if ((m_vecItems->IsVirtualDirectoryRoot() ||
             m_vecItems->IsSourcesPath()) && IsActive())
        {
          int iItem = m_viewControl.GetSelectedItem();
          Refresh();
          m_viewControl.SetSelectedItem(iItem);
        }
        return true;
      }
      else if (message.GetParam1()==GUI_MSG_UPDATE && IsActive())
      {
        if (message.GetNumStringParams())
        {
          if (message.GetParam2()) // param2 is used for resetting the history
            SetHistoryForPath(message.GetStringParam());

          CFileItemList list(message.GetStringParam());
          list.RemoveDiscCache(GetID());
          Update(message.GetStringParam());
        }
        else
          Refresh(true); // refresh the listing
      }
      else if (message.GetParam1()==GUI_MSG_UPDATE_ITEM && message.GetItem())
      {
        CFileItemPtr newItem = boost::static_pointer_cast<CFileItem>(message.GetItem());
        if (IsActive())
        {
          if (m_vecItems->UpdateItem(newItem.get()) && message.GetParam2() == 1)
          { // need the list updated as well
            UpdateFileList();
          }
        }
        else if (newItem)
        { // need to remove the disc cache
          CFileItemList items;
          CStdString path;
          URIUtils::GetDirectory(newItem->GetPath(), path);
          items.SetPath(path);
          items.RemoveDiscCache(GetID());
        }
      }
      else if (message.GetParam1()==GUI_MSG_UPDATE_PATH)
      {
        if (IsActive())
        {
          if((message.GetStringParam() == m_vecItems->GetPath()) ||
             (m_vecItems->IsMultiPath() && XFILE::CMultiPathDirectory::HasPath(m_vecItems->GetPath(), message.GetStringParam())))
            Refresh();
        }
      }
      else if (message.GetParam1() == GUI_MSG_FILTER_ITEMS && IsActive())
      {
        CStdString filter;
        // check if this is meant for advanced filtering
        if (message.GetParam2() != 10)
        {
          filter = GetProperty("filter").asString();
          if (message.GetParam2() == 1) // append
            filter += message.GetStringParam();
          else if (message.GetParam2() == 2)
          { // delete
            if (filter.size())
              filter = filter.Left(filter.size() - 1);
          }
          else
            filter = message.GetStringParam();
        }
        OnFilterItems(filter);
        return true;
      }
      else
        return CGUIWindow::OnMessage(message);

      return true;
    }
    break;
  case GUI_MSG_PLAYBACK_STARTED:
  case GUI_MSG_PLAYBACK_ENDED:
  case GUI_MSG_PLAYBACK_STOPPED:
  case GUI_MSG_PLAYLIST_CHANGED:
  case GUI_MSG_PLAYLISTPLAYER_STOPPED:
  case GUI_MSG_PLAYLISTPLAYER_STARTED:
  case GUI_MSG_PLAYLISTPLAYER_CHANGED:
    { // send a notify all to all controls on this window
      CGUIMessage msg(GUI_MSG_NOTIFY_ALL, GetID(), 0, GUI_MSG_REFRESH_LIST);
      OnMessage(msg);
      break;
    }
  case GUI_MSG_CHANGE_VIEW_MODE:
    {
      int viewMode = 0;
      if (message.GetParam1())  // we have an id
        viewMode = m_viewControl.GetViewModeByID(message.GetParam1());
      else if (message.GetParam2())
        viewMode = m_viewControl.GetNextViewMode((int)message.GetParam2());

      if (m_guiState.get())
        m_guiState->SaveViewAsControl(viewMode);
      UpdateButtons();
      return true;
    }
    break;
  case GUI_MSG_CHANGE_SORT_METHOD:
    {
      if (m_guiState.get())
      {
        if (message.GetParam1())
          m_guiState->SetCurrentSortMethod((int)message.GetParam1());
        else if (message.GetParam2())
          m_guiState->SetNextSortMethod((int)message.GetParam2());
      }
      UpdateFileList();
      return true;
    }
    break;
  case GUI_MSG_CHANGE_SORT_DIRECTION:
    {
      if (m_guiState.get())
        m_guiState->SetNextSortOrder();
      UpdateFileList();
      return true;
    }
    break;
  case GUI_MSG_WINDOW_INIT:
    {
      if (m_vecItems->GetPath() == "?")
        m_vecItems->SetPath("");
      CStdString dir = message.GetStringParam(0);
      const CStdString &ret = message.GetStringParam(1);
      bool returning = ret.CompareNoCase("return") == 0;
      if (!dir.IsEmpty())
      {
        m_history.ClearPathHistory();
        // ensure our directory is valid
        dir = GetStartFolder(dir);
        if (!returning || m_vecItems->GetPath().Left(dir.GetLength()) != dir)
        { // we're not returning to the same path, so set our directory to the requested path
          m_vecItems->SetPath(dir);
        }
        // check for network up
        if (URIUtils::IsRemote(m_vecItems->GetPath()) && !WaitForNetwork())
          m_vecItems->SetPath("");
        SetHistoryForPath(m_vecItems->GetPath());
      }
      if (message.GetParam1() != WINDOW_INVALID)
      { // first time to this window - make sure we set the root path
        m_startDirectory = returning ? dir : "";
      }
    }
    break;
  }

  return CGUIWindow::OnMessage(message);
}

// \brief Updates the states (enable, disable, visible...)
// of the controls defined by this window
// Override this function in a derived class to add new controls
void CGUIMediaWindow::UpdateButtons()
{
  if (m_guiState.get())
  {
    // Update sorting controls
    if (m_guiState->GetDisplaySortOrder() == SortOrderNone)
    {
      CONTROL_DISABLE(CONTROL_BTNSORTASC);
    }
    else
    {
      CONTROL_ENABLE(CONTROL_BTNSORTASC);
      if (m_guiState->GetDisplaySortOrder() == SortOrderAscending)
      {
        CGUIMessage msg(GUI_MSG_DESELECTED, GetID(), CONTROL_BTNSORTASC);
        g_windowManager.SendMessage(msg);
      }
      else
      {
        CGUIMessage msg(GUI_MSG_SELECTED, GetID(), CONTROL_BTNSORTASC);
        g_windowManager.SendMessage(msg);
      }
    }

    // Update list/thumb control
    m_viewControl.SetCurrentView(m_guiState->GetViewAsControl());

    // Update sort by button
    if (m_guiState->GetSortMethod()==SORT_METHOD_NONE)
    {
      CONTROL_DISABLE(CONTROL_BTNSORTBY);
    }
    else
    {
      CONTROL_ENABLE(CONTROL_BTNSORTBY);
    }
    CStdString sortLabel;
    sortLabel.Format(g_localizeStrings.Get(550).c_str(), g_localizeStrings.Get(m_guiState->GetSortMethodLabel()).c_str());
    SET_CONTROL_LABEL(CONTROL_BTNSORTBY, sortLabel);
  }

  CStdString items;
  items.Format("%i %s", m_vecItems->GetObjectCount(), g_localizeStrings.Get(127).c_str());
  SET_CONTROL_LABEL(CONTROL_LABELFILES, items);

  if (!m_canFilterAdvanced)
    SET_CONTROL_LABEL2(CONTROL_BTN_FILTER, GetProperty("filter").asString());
}

void CGUIMediaWindow::ClearFileItems()
{
  m_viewControl.Clear();
  m_vecItems->Clear();
  m_unfilteredItems->Clear();
}

// \brief Sorts Fileitems based on the sort method and sort oder provided by guiViewState
void CGUIMediaWindow::SortItems(CFileItemList &items)
{
  auto_ptr<CGUIViewState> guiState(CGUIViewState::GetViewState(GetID(), items));

  if (guiState.get())
  {
    bool sorted = false;
    SORT_METHOD sortMethod = guiState->GetSortMethod();
    // If the sort method is "sort by playlist" and we have a specific
    // sort order available we can use the specified sort order to do the sorting
    // We do this as the new SortBy methods are a superset of the SORT_METHOD methods, thus
    // not all are available. This may be removed once SORT_METHOD_* have been replaced by
    // SortBy.
    if ((sortMethod == SORT_METHOD_PLAYLIST_ORDER) && items.HasProperty(PROPERTY_SORT_ORDER))
    {
      SortBy sortBy = (SortBy)items.GetProperty(PROPERTY_SORT_ORDER).asInteger();
      if (sortBy != SortByNone && sortBy != SortByPlaylistOrder && sortBy != SortByProgramCount)
      {
        SortDescription sorting;
        sorting.sortBy = sortBy;
        sorting.sortOrder = items.GetProperty(PROPERTY_SORT_ASCENDING).asBoolean() ? SortOrderAscending : SortOrderDescending;
        sorting.sortAttributes = g_guiSettings.GetBool("filelists.ignorethewhensorting") ? SortAttributeIgnoreArticle : SortAttributeNone;

        // if the sort order is descending, we need to switch the original sort order, as we assume
        // in CGUIViewState::AddPlaylistOrder that SORT_METHOD_PLAYLIST_ORDER is ascending.
        if (guiState->GetDisplaySortOrder() == SortOrderDescending)
          sorting.sortOrder = sorting.sortOrder == SortOrderDescending ? SortOrderAscending : SortOrderDescending;

        items.Sort(sorting);
        sorted = true;
      }
    }

    if (!sorted)
      items.Sort(sortMethod, guiState->GetDisplaySortOrder());
  }
}

// \brief Formats item labels based on the formatting provided by guiViewState
void CGUIMediaWindow::FormatItemLabels(CFileItemList &items, const LABEL_MASKS &labelMasks)
{
  CLabelFormatter fileFormatter(labelMasks.m_strLabelFile, labelMasks.m_strLabel2File);
  CLabelFormatter folderFormatter(labelMasks.m_strLabelFolder, labelMasks.m_strLabel2Folder);
  for (int i=0; i<items.Size(); ++i)
  {
    CFileItemPtr pItem=items[i];

    if (pItem->IsLabelPreformated())
      continue;

    if (pItem->m_bIsFolder)
      folderFormatter.FormatLabels(pItem.get());
    else
      fileFormatter.FormatLabels(pItem.get());
  }

  if(items.GetSortMethod() == SORT_METHOD_LABEL_IGNORE_THE
  || items.GetSortMethod() == SORT_METHOD_LABEL)
    items.ClearSortState();
}

// \brief Prepares and adds the fileitems list/thumb panel
void CGUIMediaWindow::FormatAndSort(CFileItemList &items)
{
  auto_ptr<CGUIViewState> viewState(CGUIViewState::GetViewState(GetID(), items));

  if (viewState.get())
  {
    LABEL_MASKS labelMasks;
    viewState->GetSortMethodLabelMasks(labelMasks);
    FormatItemLabels(items, labelMasks);

    items.Sort(viewState->GetSortMethod(), viewState->GetDisplaySortOrder());
  }
}

/*!
  \brief Overwrite to fill fileitems from a source
  \param strDirectory Path to read
  \param items Fill with items specified in \e strDirectory
  */
bool CGUIMediaWindow::GetDirectory(const CStdString &strDirectory, CFileItemList &items)
{
  // cleanup items
  if (items.Size())
    items.Clear();

  CStdString strParentPath = m_history.GetParentPath();

  CLog::Log(LOGDEBUG,"CGUIMediaWindow::GetDirectory (%s)", strDirectory.c_str());
  CLog::Log(LOGDEBUG,"  ParentPath = [%s]", strParentPath.c_str());

  // see if we can load a previously cached folder
  CFileItemList cachedItems(strDirectory);
  if (!strDirectory.IsEmpty() && cachedItems.Load(GetID()))
  {
    items.Assign(cachedItems);
  }
  else
  {
    unsigned int time = XbmcThreads::SystemClockMillis();

    if (strDirectory.IsEmpty())
      SetupShares();

    if (!m_rootDir.GetDirectory(strDirectory, items))
      return false;

    // took over a second, and not normally cached, so cache it
    if ((XbmcThreads::SystemClockMillis() - time) > 1000  && items.CacheToDiscIfSlow())
      items.Save(GetID());

    // if these items should replace the current listing, then pop it off the top
    if (items.GetReplaceListing())
      m_history.RemoveParentPath();
  }

  if (m_guiState.get() && !m_guiState->HideParentDirItems() && !items.GetPath().IsEmpty())
  {
    CFileItemPtr pItem(new CFileItem(".."));
    pItem->SetPath(strParentPath);
    pItem->m_bIsFolder = true;
    pItem->m_bIsShareOrDrive = false;
    items.AddFront(pItem, 0);
  }

  int iWindow = GetID();
  CStdStringArray regexps;

  // TODO: Do we want to limit the directories we apply the video ones to?
  if (iWindow == WINDOW_VIDEO_NAV)
    regexps = g_advancedSettings.m_videoExcludeFromListingRegExps;
  if (iWindow == WINDOW_MUSIC_FILES)
    regexps = g_advancedSettings.m_audioExcludeFromListingRegExps;
  if (iWindow == WINDOW_PICTURES)
    regexps = g_advancedSettings.m_pictureExcludeFromListingRegExps;

  if (regexps.size())
  {
    for (int i=0; i < items.Size();)
    {
      if (CUtil::ExcludeFileOrFolder(items[i]->GetPath(), regexps))
        items.Remove(i);
      else
        i++;
    }
  }

  // clear the filter
  SetProperty("filter", "");
  m_canFilterAdvanced = false;
  m_filter.Reset();
  return true;
}

// \brief Set window to a specific directory
// \param strDirectory The directory to be displayed in list/thumb control
// This function calls OnPrepareFileItems() and OnFinalizeFileItems()
bool CGUIMediaWindow::Update(const CStdString &strDirectory, bool updateFilterPath /* = true */)
{
  // TODO: OnInitWindow calls Update() before window path has been set properly.
  if (strDirectory == "?")
    return false;

  // get selected item
  int iItem = m_viewControl.GetSelectedItem();
  CStdString strSelectedItem = "";
  if (iItem >= 0 && iItem < m_vecItems->Size())
  {
    CFileItemPtr pItem = m_vecItems->Get(iItem);
    if (!pItem->IsParentFolder())
    {
      GetDirectoryHistoryString(pItem.get(), strSelectedItem);
    }
  }
  
  CStdString strCurrentDirectory = m_vecItems->GetPath();
  m_history.SetSelectedItem(strSelectedItem, strCurrentDirectory);

  CStdString directory = strDirectory;
  // check if the path contains a filter and temporarily remove it
  // so that the retrieved list of items is unfiltered
  bool canfilter = CanContainFilter(directory);
  CStdString filter;
  CURL url(directory);
  if (canfilter && url.HasOption("filter"))
  {
    filter = url.GetOption("filter");
    directory = RemoveParameterFromPath(directory, "filter");
  }

  CFileItemList items;
  if (!GetDirectory(directory, items))
  {
    CLog::Log(LOGERROR,"CGUIMediaWindow::GetDirectory(%s) failed", strDirectory.c_str());
    // if the directory is the same as the old directory, then we'll return
    // false.  Else, we assume we can get the previous directory
    if (strDirectory.Equals(strCurrentDirectory))
      return false;

    // We assume, we can get the parent
    // directory again, but we have to
    // return false to be able to eg. show
    // an error message.
    Update(m_history.RemoveParentPath());
    return false;
  }

  if (items.GetLabel().IsEmpty())
    items.SetLabel(CUtil::GetTitleFromPath(items.GetPath(), true));
  
  ClearFileItems();
  m_vecItems->Copy(items);

  // only set the filter path if it hasn't been marked
  // as preset or if it's empty
  if (updateFilterPath || m_strFilterPath.empty())
  {
    if (items.HasProperty(PROPERTY_PATH_DB))
      m_strFilterPath = items.GetProperty(PROPERTY_PATH_DB).asString();
    else
      m_strFilterPath = items.GetPath();
  }
  
  // maybe the filter path can contain a filter
  if (!canfilter && CanContainFilter(m_strFilterPath))
    canfilter = true;

  // check if the filter path contains a filter
  CURL filterPathUrl(m_strFilterPath);
  if (canfilter && filter.empty())
  {
    if (filterPathUrl.HasOption("filter"))
      filter = filterPathUrl.GetOption("filter");
  }

  // check if there is a filter and re-apply it
  if (canfilter && !filter.empty())
  {
    if (!m_filter.LoadFromJson(filter))
    {
      CLog::Log(LOGWARNING, "CGUIMediaWindow::Update: unable to load existing filter (%s)", filter.c_str());
      m_filter.Reset();
      m_strFilterPath = m_vecItems->GetPath();
    }
    else
    {
      // add the filter to the filter path
      filterPathUrl.SetOption("filter", filter);
      m_strFilterPath = filterPathUrl.Get();
    }
  }
    
  // if we're getting the root source listing
  // make sure the path history is clean
  if (strDirectory.IsEmpty())
    m_history.ClearPathHistory();

  int iWindow = GetID();
  int showLabel = 0;
  if (strDirectory.IsEmpty() && (iWindow == WINDOW_MUSIC_FILES ||
                                 iWindow == WINDOW_FILES ||
                                 iWindow == WINDOW_PICTURES ||
                                 iWindow == WINDOW_PROGRAMS))
    showLabel = 1026;
  if (strDirectory.Equals("sources://video/"))
    showLabel = 999;
  if (showLabel && (m_vecItems->Size() == 0 || !m_guiState->DisableAddSourceButtons())) // add 'add source button'
  {
    CStdString strLabel = g_localizeStrings.Get(showLabel);
    CFileItemPtr pItem(new CFileItem(strLabel));
    pItem->SetPath("add");
    pItem->SetIconImage("DefaultAddSource.png");
    pItem->SetLabel(strLabel);
    pItem->SetLabelPreformated(true);
    pItem->m_bIsFolder = true;
    pItem->SetSpecialSort(SortSpecialOnBottom);
    m_vecItems->Add(pItem);
  }
  m_iLastControl = GetFocusedControlID();

  // Check whether to enabled advanced filtering based on the content type
  m_canFilterAdvanced = CheckFilterAdvanced(*m_vecItems);
  if (m_canFilterAdvanced)
    m_filter.SetType(m_vecItems->GetContent());

  //  Ask the derived class if it wants to load additional info
  //  for the fileitems like media info or additional
  //  filtering on the items, setting thumbs.
  OnPrepareFileItems(*m_vecItems);

  // The idea here is to ensure we have something to focus if our file list
  // is empty.  As such, this check MUST be last and ignore the hide parent
  // fileitems settings.
  if (m_vecItems->IsEmpty())
  {
    CFileItemPtr pItem(new CFileItem(".."));
    pItem->SetPath(m_history.GetParentPath());
    pItem->m_bIsFolder = true;
    pItem->m_bIsShareOrDrive = false;
    m_vecItems->AddFront(pItem, 0);
  }

  m_vecItems->FillInDefaultIcons();

  m_guiState.reset(CGUIViewState::GetViewState(GetID(), *m_vecItems));

  // remember the original (untouched) list of items (for filtering etc)
  m_unfilteredItems->SetPath(m_vecItems->GetPath()); // use the original path - it'll likely be relied on for other things later.
  m_unfilteredItems->Append(*m_vecItems);

  // Cache the list of items if possible
  OnCacheFileItems(*m_vecItems);

  // Filter and group the items if necessary
  CStdString titleFilter = GetProperty("filter").asString();
  OnFilterItems(titleFilter);

  // Ask the devived class if it wants to do custom list operations,
  // eg. changing the label
  OnFinalizeFileItems(*m_vecItems);
  UpdateButtons();

  strSelectedItem = m_history.GetSelectedItem(m_vecItems->GetPath());

  bool bSelectedFound = false;
  //int iSongInDirectory = -1;
  for (int i = 0; i < m_vecItems->Size(); ++i)
  {
    CFileItemPtr pItem = m_vecItems->Get(i);

    // Update selected item
    CStdString strHistory;
    GetDirectoryHistoryString(pItem.get(), strHistory);
    if (strHistory == strSelectedItem)
    {
      m_viewControl.SetSelectedItem(i);
      bSelectedFound = true;
      break;
    }
  }

  // if we haven't found the selected item, select the first item
  if (!bSelectedFound)
    m_viewControl.SetSelectedItem(0);

  if (iWindow != WINDOW_PVR || (iWindow == WINDOW_PVR && m_vecItems->GetPath().Left(17) == "pvr://recordings/"))
    m_history.AddPath(m_vecItems->GetPath(), m_strFilterPath);

  //m_history.DumpPathHistory();

  return true;
}

bool CGUIMediaWindow::Refresh(bool clearCache /* = false */)
{
  CStdString strCurrentDirectory = m_vecItems->GetPath();
  if (strCurrentDirectory.Equals("?"))
    return false;

  if (clearCache)
    m_vecItems->RemoveDiscCache(GetID());

  // get the original number of items
  int oldCount = m_filter.IsEmpty() ? m_vecItems->Size() : m_unfilteredItems->Size();
  if (!Update(strCurrentDirectory, false))
    return false;

  // check if we previously had at least 1 item
  // in the list and whether it now went down to 0
  // if there are no more items to show after the update
  // we go one level up in the hierachry to not show an
  // empty list
  if (oldCount > 0 &&
     (m_filter.IsEmpty() ? m_vecItems->Size() : m_unfilteredItems->Size()) <= 0)
  {
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(2080), g_localizeStrings.Get(2081));
    GoParentFolder();
  }

  return true;
}

// \brief This function will be called by Update() before the
// labels of the fileitems are formatted. Override this function
// to set custom thumbs or load additional media info.
// It's used to load tag info for music.
void CGUIMediaWindow::OnPrepareFileItems(CFileItemList &items)
{

}

// \brief This function will be called by Update() before
// any additional formatting, filtering or sorting is applied.
// Override this function to define a custom caching behaviour.
void CGUIMediaWindow::OnCacheFileItems(CFileItemList &items)
{
  // Should these items be saved to the hdd
  if (items.CacheToDiscAlways() && !IsFiltered())
    items.Save(GetID());
}

// \brief This function will be called by Update() after the
// labels of the fileitems are formatted. Override this function
// to modify the fileitems. Eg. to modify the item label
void CGUIMediaWindow::OnFinalizeFileItems(CFileItemList &items)
{

}

// \brief With this function you can react on a users click in the list/thumb panel.
// It returns true, if the click is handled.
// This function calls OnPlayMedia()
bool CGUIMediaWindow::OnClick(int iItem)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems->Size() ) return true;
  CFileItemPtr pItem = m_vecItems->Get(iItem);

  if (pItem->IsParentFolder())
  {
    GoParentFolder();
    return true;
  }
  if (pItem->GetPath() == "add" || pItem->GetPath() == "sources://add/") // 'add source button' in empty root
  {
    OnContextButton(iItem, CONTEXT_BUTTON_ADD_SOURCE);
    return true;
  }

  if (!pItem->m_bIsFolder && pItem->IsFileFolder())
  {
    XFILE::IFileDirectory *pFileDirectory = NULL;
    pFileDirectory = XFILE::CFileDirectoryFactory::Create(pItem->GetPath(), pItem.get(), "");
    if(pFileDirectory)
      pItem->m_bIsFolder = true;
    else if(pItem->m_bIsFolder)
      pItem->m_bIsFolder = false;
    delete pFileDirectory;
  }

  if (pItem->IsScript())
  {
    // execute the script
    CURL url(pItem->GetPath());
    AddonPtr addon;
    if (CAddonMgr::Get().GetAddon(url.GetHostName(), addon, ADDON_SCRIPT))
    {
#ifdef HAS_PYTHON
      if (!g_pythonParser.StopScript(addon->LibPath()))
        g_pythonParser.evalFile(addon->LibPath(),addon);
#endif
      return true;
    }
  }

  if (pItem->m_bIsFolder)
  {
    if ( pItem->m_bIsShareOrDrive )
    {
      const CStdString& strLockType=m_guiState->GetLockType();
      if (g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE)
        if (!strLockType.IsEmpty() && !g_passwordManager.IsItemUnlocked(pItem.get(), strLockType))
            return true;

      if (!HaveDiscOrConnection(pItem->GetPath(), pItem->m_iDriveType))
        return true;
    }

    // check for the partymode playlist items - they may not exist yet
    if ((pItem->GetPath() == g_settings.GetUserDataItem("PartyMode.xsp")) ||
        (pItem->GetPath() == g_settings.GetUserDataItem("PartyMode-Video.xsp")))
    {
      // party mode playlist item - if it doesn't exist, prompt for user to define it
      if (!XFILE::CFile::Exists(pItem->GetPath()))
      {
        m_vecItems->RemoveDiscCache(GetID());
        if (CGUIDialogSmartPlaylistEditor::EditPlaylist(pItem->GetPath()))
          Refresh();
        return true;
      }
    }

    // remove the directory cache if the folder is not normally cached
    CFileItemList items(pItem->GetPath());
    if (!items.AlwaysCache())
      items.RemoveDiscCache(GetID());

    // if we have a filtered list, we need to add the filtered
    // path to be able to come back to the filtered view
    CStdString strCurrentDirectory = m_vecItems->GetPath();
    if (m_canFilterAdvanced && !m_filter.IsEmpty() &&
      !m_strFilterPath.Equals(strCurrentDirectory))
    {
      m_history.RemoveParentPath();
      m_history.AddPath(strCurrentDirectory, m_strFilterPath);
    }

    CFileItem directory(*pItem);
    if (!Update(directory.GetPath()))
      ShowShareErrorMessage(&directory);

    return true;
  }
  else if (pItem->IsPlugin() && !pItem->GetProperty("isplayable").asBoolean())
  {
    return XFILE::CPluginDirectory::RunScriptWithParams(pItem->GetPath());
  }
#if defined(TARGET_ANDROID)
  else if (pItem->IsAndroidApp())
  {
    CStdString appName = URIUtils::GetFileName(pItem->GetPath());
    CLog::Log(LOGDEBUG, "CGUIMediaWindow::OnClick Trying to run: %s",appName.c_str());
    return CXBMCApp::StartActivity(appName);
  }
#endif
  else
  {
    m_iSelectedItem = m_viewControl.GetSelectedItem();

    if (pItem->GetPath() == "newplaylist://")
    {
      m_vecItems->RemoveDiscCache(GetID());
      g_windowManager.ActivateWindow(WINDOW_MUSIC_PLAYLIST_EDITOR,"newplaylist://");
      return true;
    }
    else if (pItem->GetPath().Left(19).Equals("newsmartplaylist://"))
    {
      m_vecItems->RemoveDiscCache(GetID());
      if (CGUIDialogSmartPlaylistEditor::NewPlaylist(pItem->GetPath().Mid(19)))
        Refresh();
      return true;
    }
    else if (pItem->GetPath().Left(14).Equals("addons://more/"))
    {
      CBuiltins::Execute("ActivateWindow(AddonBrowser,addons://all/xbmc.addon." + pItem->GetPath().Mid(14) + ",return)");
      return true;
    }

    // If karaoke song is being played AND popup autoselector is enabled, the playlist should not be added
    bool do_not_add_karaoke = g_guiSettings.GetBool("karaoke.enabled") &&
      g_guiSettings.GetBool("karaoke.autopopupselector") && pItem->IsKaraoke();
    bool autoplay = m_guiState.get() && m_guiState->AutoPlayNextItem();

    if (m_vecItems->IsPlugin())
    {
      CURL url(m_vecItems->GetPath());
      AddonPtr addon;
      if (CAddonMgr::Get().GetAddon(url.GetHostName(),addon))
      {
        PluginPtr plugin = boost::dynamic_pointer_cast<CPluginSource>(addon);
        if (plugin && plugin->Provides(CPluginSource::AUDIO))
        {
          CFileItemList items;
          auto_ptr<CGUIViewState> state(CGUIViewState::GetViewState(GetID(), items));
          autoplay = state.get() && state->AutoPlayNextItem();
        }
      }
    }

    if (autoplay && !g_partyModeManager.IsEnabled() && 
        !pItem->IsPlayList() && !do_not_add_karaoke)
    {
      return OnPlayAndQueueMedia(pItem);
    }
    else
    {
      return OnPlayMedia(iItem);
    }
  }

  return false;
}

bool CGUIMediaWindow::OnSelect(int item)
{
  return OnClick(item);
}

// \brief Checks if there is a disc in the dvd drive and whether the
// network is connected or not.
bool CGUIMediaWindow::HaveDiscOrConnection(const CStdString& strPath, int iDriveType)
{
  if (iDriveType==CMediaSource::SOURCE_TYPE_DVD)
  {
    if (!g_mediaManager.IsDiscInDrive(strPath))
    {
      CGUIDialogOK::ShowAndGetInput(218, 219, 0, 0);
      return false;
    }
  }
  else if (iDriveType==CMediaSource::SOURCE_TYPE_REMOTE)
  {
    // TODO: Handle not connected to a remote share
    if ( !g_application.getNetwork().IsConnected() )
    {
      CGUIDialogOK::ShowAndGetInput(220, 221, 0, 0);
      return false;
    }
  }

  return true;
}

// \brief Shows a standard errormessage for a given pItem.
void CGUIMediaWindow::ShowShareErrorMessage(CFileItem* pItem)
{
  if (pItem->m_bIsShareOrDrive)
  {
    int idMessageText=0;
    const CURL& url=pItem->GetAsUrl();
    const CStdString& strHostName=url.GetHostName();

    if (pItem->m_iDriveType != CMediaSource::SOURCE_TYPE_REMOTE) //  Local shares incl. dvd drive
      idMessageText=15300;
    else if (url.GetProtocol() == "smb" && strHostName.IsEmpty()) //  smb workgroup
      idMessageText=15303;
    else  //  All other remote shares
      idMessageText=15301;

    CGUIDialogOK::ShowAndGetInput(220, idMessageText, 0, 0);
  }
}

// \brief The functon goes up one level in the directory tree
void CGUIMediaWindow::GoParentFolder()
{
  //m_history.DumpPathHistory();

  // remove current directory if its on the stack
  // there were some issues due some folders having a trailing slash and some not
  // so just add a trailing slash to all of them for comparison.
  CStdString strPath = m_vecItems->GetPath();
  URIUtils::AddSlashAtEnd(strPath);
  CStdString strParent = m_history.GetParentPath();
  // in case the path history is messed up and the current folder is on
  // the stack more than once, keep going until there's nothing left or they
  // dont match anymore.
  while (!strParent.IsEmpty())
  {
    URIUtils::AddSlashAtEnd(strParent);
    if (strParent.Equals(strPath))
      m_history.RemoveParentPath();
    else
      break;
    strParent = m_history.GetParentPath();
  }

  // remove the current filter but only if the parent
  // item doesn't have a filter as well
  CURL filterUrl(m_strFilterPath);
  if (filterUrl.HasOption("filter"))
  {
    CURL parentUrl(m_history.GetParentPath(true));
    if (!parentUrl.HasOption("filter"))
    {
      // we need to overwrite m_strFilterPath because
      // Refresh() will set updateFilterPath to false
      m_strFilterPath.clear();
      Refresh();
      return;
    }
  }

  // if vector is not empty, pop parent
  // if vector is empty, parent is root source listing
  m_strFilterPath = m_history.GetParentPath(true);
  strParent = m_history.RemoveParentPath();
  if (!Update(strParent, false))
    return;

  // No items to show so go another level up
  if (!m_vecItems->GetPath().empty() && (m_filter.IsEmpty() ? m_vecItems->Size() : m_unfilteredItems->Size()) <= 0)
  {
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(2080), g_localizeStrings.Get(2081));
    GoParentFolder();
  }
}

// \brief Override the function to change the default behavior on how
// a selected item history should look like
void CGUIMediaWindow::GetDirectoryHistoryString(const CFileItem* pItem, CStdString& strHistoryString)
{
  if (pItem->m_bIsShareOrDrive)
  {
    // We are in the virual directory

    // History string of the DVD drive
    // must be handel separately
    if (pItem->m_iDriveType == CMediaSource::SOURCE_TYPE_DVD)
    {
      // Remove disc label from item label
      // and use as history string, m_strPath
      // can change for new discs
      CStdString strLabel = pItem->GetLabel();
      int nPosOpen = strLabel.Find('(');
      int nPosClose = strLabel.ReverseFind(')');
      if (nPosOpen > -1 && nPosClose > -1 && nPosClose > nPosOpen)
      {
        strLabel.Delete(nPosOpen + 1, (nPosClose) - (nPosOpen + 1));
        strHistoryString = strLabel;
      }
      else
        strHistoryString = strLabel;
    }
    else
    {
      // Other items in virual directory
      CStdString strPath = pItem->GetPath();
      URIUtils::RemoveSlashAtEnd(strPath);

      strHistoryString = pItem->GetLabel() + strPath;
    }
  }
  else if (pItem->m_lEndOffset>pItem->m_lStartOffset && pItem->m_lStartOffset != -1)
  {
    // Could be a cue item, all items of a cue share the same filename
    // so add the offsets to build the history string
    strHistoryString.Format("%ld%ld", pItem->m_lStartOffset, pItem->m_lEndOffset);
    strHistoryString += pItem->GetPath();
  }
  else
  {
    // Normal directory items
    strHistoryString = pItem->GetPath();
  }

  // remove any filter
  if (CanContainFilter(strHistoryString))
    strHistoryString = RemoveParameterFromPath(strHistoryString, "filter");

  URIUtils::RemoveSlashAtEnd(strHistoryString);
  strHistoryString.ToLower();
}

// \brief Call this function to create a directory history for the
// path given by strDirectory.
void CGUIMediaWindow::SetHistoryForPath(const CStdString& strDirectory)
{
  // Make sure our shares are configured
  SetupShares();
  if (!strDirectory.IsEmpty())
  {
    // Build the directory history for default path
    CStdString strPath, strParentPath;
    strPath = strDirectory;
    URIUtils::RemoveSlashAtEnd(strPath);

    CFileItemList items;
    m_rootDir.GetDirectory("", items);

    m_history.ClearPathHistory();

    while (URIUtils::GetParentPath(strPath, strParentPath))
    {
      for (int i = 0; i < (int)items.Size(); ++i)
      {
        CFileItemPtr pItem = items[i];
        CStdString path(pItem->GetPath());
        URIUtils::RemoveSlashAtEnd(path);
        if (path == strPath)
        {
          CStdString strHistory;
          GetDirectoryHistoryString(pItem.get(), strHistory);
          m_history.SetSelectedItem(strHistory, "");
          URIUtils::AddSlashAtEnd(strPath);
          m_history.AddPathFront(strPath);
          m_history.AddPathFront("");

          //m_history.DumpPathHistory();
          return ;
        }
      }

      URIUtils::AddSlashAtEnd(strPath);
      m_history.AddPathFront(strPath);
      m_history.SetSelectedItem(strPath, strParentPath);
      strPath = strParentPath;
      URIUtils::RemoveSlashAtEnd(strPath);
    }
  }
  else
    m_history.ClearPathHistory();

  //m_history.DumpPathHistory();
}

// \brief Override if you want to change the default behavior, what is done
// when the user clicks on a file.
// This function is called by OnClick()
bool CGUIMediaWindow::OnPlayMedia(int iItem)
{
  // Reset Playlistplayer, playback started now does
  // not use the playlistplayer.
  g_playlistPlayer.Reset();
  g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_NONE);
  CFileItemPtr pItem=m_vecItems->Get(iItem);

  CLog::Log(LOGDEBUG, "%s %s", __FUNCTION__, pItem->GetPath().c_str());

  bool bResult = false;
  if (pItem->IsInternetStream() || pItem->IsPlayList())
    bResult = g_application.PlayMedia(*pItem, m_guiState->GetPlaylist());
  else
    bResult = g_application.PlayFile(*pItem);

  if (pItem->m_lStartOffset == STARTOFFSET_RESUME)
    pItem->m_lStartOffset = 0;

  return bResult;
}

// \brief Override if you want to change the default behavior of what is done
// when the user clicks on a file in a "folder" with similar files.
// This function is called by OnClick()
bool CGUIMediaWindow::OnPlayAndQueueMedia(const CFileItemPtr &item)
{
  //play and add current directory to temporary playlist
  int iPlaylist = m_guiState->GetPlaylist();
  if (iPlaylist != PLAYLIST_NONE)
  {
    g_playlistPlayer.ClearPlaylist(iPlaylist);
    g_playlistPlayer.Reset();
    int mediaToPlay = 0;
    CFileItemList queueItems;
    for ( int i = 0; i < m_vecItems->Size(); i++ )
    {
      CFileItemPtr nItem = m_vecItems->Get(i);

      if (nItem->m_bIsFolder)
        continue;

      if (!nItem->IsPlayList() && !nItem->IsZIP() && !nItem->IsRAR())
        queueItems.Add(nItem);

      if (nItem == item)
      { // item that was clicked
        mediaToPlay = queueItems.Size() - 1;
      }
    }
    g_playlistPlayer.Add(iPlaylist, queueItems);

    // Save current window and directory to know where the selected item was
    if (m_guiState.get())
      m_guiState->SetPlaylistDirectory(m_vecItems->GetPath());

    // figure out where we start playback
    if (g_playlistPlayer.IsShuffled(iPlaylist))
    {
      int iIndex = g_playlistPlayer.GetPlaylist(iPlaylist).FindOrder(mediaToPlay);
      g_playlistPlayer.GetPlaylist(iPlaylist).Swap(0, iIndex);
      mediaToPlay = 0;
    }

    // play
    g_playlistPlayer.SetCurrentPlaylist(iPlaylist);
    g_playlistPlayer.Play(mediaToPlay);
  }
  return true;
}

// \brief Synchonize the fileitems with the playlistplayer
// It recreated the playlist of the playlistplayer based
// on the fileitems of the window
void CGUIMediaWindow::UpdateFileList()
{
  int nItem = m_viewControl.GetSelectedItem();
  CStdString strSelected;
  if (nItem >= 0)
    strSelected = m_vecItems->Get(nItem)->GetPath();

  FormatAndSort(*m_vecItems);
  UpdateButtons();

  m_viewControl.SetItems(*m_vecItems);
  m_viewControl.SetSelectedItem(strSelected);

  //  set the currently playing item as selected, if its in this directory
  if (m_guiState.get() && m_guiState->IsCurrentPlaylistDirectory(m_vecItems->GetPath()))
  {
    int iPlaylist=m_guiState->GetPlaylist();
    int nSong = g_playlistPlayer.GetCurrentSong();
    CFileItem playlistItem;
    if (nSong > -1 && iPlaylist > -1)
      playlistItem=*g_playlistPlayer.GetPlaylist(iPlaylist)[nSong];

    g_playlistPlayer.ClearPlaylist(iPlaylist);
    g_playlistPlayer.Reset();

    for (int i = 0; i < m_vecItems->Size(); i++)
    {
      CFileItemPtr pItem = m_vecItems->Get(i);
      if (pItem->m_bIsFolder)
        continue;

      if (!pItem->IsPlayList() && !pItem->IsZIP() && !pItem->IsRAR())
        g_playlistPlayer.Add(iPlaylist, pItem);

      if (pItem->GetPath() == playlistItem.GetPath() &&
          pItem->m_lStartOffset == playlistItem.m_lStartOffset)
        g_playlistPlayer.SetCurrentSong(g_playlistPlayer.GetPlaylist(iPlaylist).size() - 1);
    }
  }
}

void CGUIMediaWindow::OnDeleteItem(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems->Size()) return;
  CFileItemPtr item = m_vecItems->Get(iItem);

  if (item->IsPlayList())
    item->m_bIsFolder = false;

  if (g_settings.GetCurrentProfile().getLockMode() != LOCK_MODE_EVERYONE && g_settings.GetCurrentProfile().filesLocked())
    if (!g_passwordManager.IsMasterLockUnlocked(true))
      return;

  if (!CFileUtils::DeleteItem(item))
    return;
  Refresh(true);
  m_viewControl.SetSelectedItem(iItem);
}

void CGUIMediaWindow::OnRenameItem(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems->Size()) return;

  if (g_settings.GetCurrentProfile().getLockMode() != LOCK_MODE_EVERYONE && g_settings.GetCurrentProfile().filesLocked())
    if (!g_passwordManager.IsMasterLockUnlocked(true))
      return;

  if (!CFileUtils::RenameFile(m_vecItems->Get(iItem)->GetPath()))
    return;
  Refresh(true);
  m_viewControl.SetSelectedItem(iItem);
}

void CGUIMediaWindow::OnInitWindow()
{
  // initial fetch is done unthreaded to ensure the items are setup prior to skin animations kicking off
  m_rootDir.SetAllowThreads(false);
  Refresh();
  m_rootDir.SetAllowThreads(true);

  if (m_iSelectedItem > -1)
    m_viewControl.SetSelectedItem(m_iSelectedItem);

  CGUIWindow::OnInitWindow();
}

CGUIControl *CGUIMediaWindow::GetFirstFocusableControl(int id)
{
  if (m_viewControl.HasControl(id))
    id = m_viewControl.GetCurrentControl();
  return CGUIWindow::GetFirstFocusableControl(id);
}

void CGUIMediaWindow::SetupShares()
{
  // Setup shares and filemasks for this window
  CFileItemList items;
  CGUIViewState* viewState=CGUIViewState::GetViewState(GetID(), items);
  if (viewState)
  {
    m_rootDir.SetMask(viewState->GetExtensions());
    m_rootDir.SetSources(viewState->GetSources());
    delete viewState;
  }
}

bool CGUIMediaWindow::OnPopupMenu(int iItem)
{
  // popup the context menu
  // grab our context menu
  CContextButtons buttons;
  GetContextButtons(iItem, buttons);

  if (buttons.size())
  {
    // mark the item
    if (iItem >= 0 && iItem < m_vecItems->Size())
      m_vecItems->Get(iItem)->Select(true);

    int choice = CGUIDialogContextMenu::ShowAndGetChoice(buttons);

    // deselect our item
    if (iItem >= 0 && iItem < m_vecItems->Size())
      m_vecItems->Get(iItem)->Select(false);

    if (choice >= 0)
      return OnContextButton(iItem, (CONTEXT_BUTTON)choice);
  }
  return false;
}

void CGUIMediaWindow::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  CFileItemPtr item = (itemNumber >= 0 && itemNumber < m_vecItems->Size()) ? m_vecItems->Get(itemNumber) : CFileItemPtr();

  if (!item)
    return;

  // user added buttons
  CStdString label;
  CStdString action;
  for (int i = CONTEXT_BUTTON_USER1; i <= CONTEXT_BUTTON_USER10; i++)
  {
    label.Format("contextmenulabel(%i)", i - CONTEXT_BUTTON_USER1);
    if (item->GetProperty(label).empty())
      break;

    action.Format("contextmenuaction(%i)", i - CONTEXT_BUTTON_USER1);
    if (item->GetProperty(action).empty())
      break;

    buttons.Add((CONTEXT_BUTTON)i, item->GetProperty(label).asString());
  }

  if (item->GetProperty("pluginreplacecontextitems").asBoolean())
    return;

  // TODO: FAVOURITES Conditions on masterlock and localisation
  if (!item->IsParentFolder() && !item->GetPath().Equals("add") && !item->GetPath().Equals("newplaylist://") &&
      !item->GetPath().Left(19).Equals("newsmartplaylist://") && !item->GetPath().Left(9).Equals("newtag://"))
  {
    if (CFavourites::IsFavourite(item.get(), GetID()))
      buttons.Add(CONTEXT_BUTTON_ADD_FAVOURITE, 14077);     // Remove Favourite
    else
      buttons.Add(CONTEXT_BUTTON_ADD_FAVOURITE, 14076);     // Add To Favourites;
  }
}

bool CGUIMediaWindow::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  switch (button)
  {
  case CONTEXT_BUTTON_ADD_FAVOURITE:
    {
      CFileItemPtr item = m_vecItems->Get(itemNumber);
      CFavourites::AddOrRemove(item.get(), GetID());
      return true;
    }
  case CONTEXT_BUTTON_PLUGIN_SETTINGS:
    {
      CURL plugin(m_vecItems->Get(itemNumber)->GetPath());
      ADDON::AddonPtr addon;
      if (CAddonMgr::Get().GetAddon(plugin.GetHostName(), addon))
        if (CGUIDialogAddonSettings::ShowAndGetInput(addon))
          Refresh();
      return true;
    }
  case CONTEXT_BUTTON_USER1:
  case CONTEXT_BUTTON_USER2:
  case CONTEXT_BUTTON_USER3:
  case CONTEXT_BUTTON_USER4:
  case CONTEXT_BUTTON_USER5:
  case CONTEXT_BUTTON_USER6:
  case CONTEXT_BUTTON_USER7:
  case CONTEXT_BUTTON_USER8:
  case CONTEXT_BUTTON_USER9:
  case CONTEXT_BUTTON_USER10:
    {
      CStdString action;
      action.Format("contextmenuaction(%i)", button - CONTEXT_BUTTON_USER1);
      CApplicationMessenger::Get().ExecBuiltIn(m_vecItems->Get(itemNumber)->GetProperty(action).asString());
      return true;
    }
  default:
    break;
  }
  return false;
}

const CGUIViewState *CGUIMediaWindow::GetViewState() const
{
  return m_guiState.get();
}

const CFileItemList& CGUIMediaWindow::CurrentDirectory() const
{
  return *m_vecItems;
}

bool CGUIMediaWindow::WaitForNetwork() const
{
  if (g_application.getNetwork().IsAvailable())
    return true;

  CGUIDialogProgress *progress = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  if (!progress)
    return true;

  CURL url(m_vecItems->GetPath());
  progress->SetHeading(1040); // Loading Directory
  progress->SetLine(1, url.GetWithoutUserDetails());
  progress->ShowProgressBar(false);
  progress->StartModal();
  while (!g_application.getNetwork().IsAvailable())
  {
    progress->Progress();
    if (progress->IsCanceled())
    {
      progress->Close();
      return false;
    }
  }
  progress->Close();
  return true;
}

void CGUIMediaWindow::OnFilterItems(const CStdString &filter)
{
  CFileItemPtr currentItem;
  CStdString currentItemPath;
  int item = m_viewControl.GetSelectedItem();
  if (item >= 0 && item < m_vecItems->Size())
  {
    currentItem = m_vecItems->Get(item);
    currentItemPath = currentItem->GetPath();
  }
  
  m_viewControl.Clear();
  
  CFileItemList items(m_vecItems->GetPath()); // use the original path - it'll likely be relied on for other things later.
  items.Append(*m_unfilteredItems);
  bool filtered = GetFilteredItems(filter, items);

  m_vecItems->ClearItems();
  // we need to clear the sort state and re-sort the items
  m_vecItems->ClearSortState();
  m_vecItems->Append(items);
  
  // if the filter has changed, get the new filter path
  if (filtered && m_canFilterAdvanced)
  {
    if (items.HasProperty(PROPERTY_PATH_DB))
      m_strFilterPath = items.GetProperty(PROPERTY_PATH_DB).asString();
    else
      m_strFilterPath = items.GetPath();
  }
  
  GetGroupedItems(*m_vecItems);
  FormatAndSort(*m_vecItems);

  // get the "filter" option
  CStdString filterOption;
  CURL filterUrl(m_strFilterPath);
  if (filterUrl.HasOption("filter"))
    filterOption = filterUrl.GetOption("filter");

  // apply the "filter" option to any folder item so that
  // the filter can be passed down to the sub-directory
  for (int index = 0; index < m_vecItems->Size(); index++)
  {
    CFileItemPtr pItem = m_vecItems->Get(index);
    // if the item is a folder we need to copy the path of
    // the filtered item to be able to keep the applied filters
    if (pItem->m_bIsFolder)
    {
      CURL itemUrl(pItem->GetPath());
      itemUrl.SetOption("filter", filterOption);
      pItem->SetPath(itemUrl.Get());
    }
  }

  if (filtered)
  {
    if (!m_canFilterAdvanced)
      SetProperty("filter", filter);
    else
    {
      // to be able to select the same item as before we need to adjust
      // the path of the item i.e. add or remove the "filter=" URL option
      // but that's only necessary for folder items
      if (currentItem.get() != NULL && currentItem->m_bIsFolder)
      {
        CURL curUrl(currentItemPath), newUrl(m_strFilterPath);
        if (newUrl.HasOption("filter"))
          curUrl.SetOption("filter", newUrl.GetOption("filter"));
        else if (curUrl.HasOption("filter"))
          curUrl.RemoveOption("filter");

        currentItemPath = curUrl.Get();
      }
    }
  }
  
  // and update our view control + buttons
  m_viewControl.SetItems(*m_vecItems);
  m_viewControl.SetSelectedItem(currentItemPath);
  UpdateButtons();
}

bool CGUIMediaWindow::GetFilteredItems(const CStdString &filter, CFileItemList &items)
{
  if (m_canFilterAdvanced)
    return GetAdvanceFilteredItems(items);

  CStdString trimmedFilter(filter);
  trimmedFilter.TrimLeft().ToLower();
  
  if (trimmedFilter.IsEmpty())
    return true;

  CFileItemList filteredItems(items.GetPath()); // use the original path - it'll likely be relied on for other things later.
  bool numericMatch = StringUtils::IsNaturalNumber(trimmedFilter);
  for (int i = 0; i < items.Size(); i++)
  {
    CFileItemPtr item = items.Get(i);
    if (item->IsParentFolder())
    {
      filteredItems.Add(item);
      continue;
    }
    // TODO: Need to update this to get all labels, ideally out of the displayed info (ie from m_layout and m_focusedLayout)
    // though that isn't practical.  Perhaps a better idea would be to just grab the info that we should filter on based on
    // where we are in the library tree.
    // Another idea is tying the filter string to the current level of the tree, so that going deeper disables the filter,
    // but it's re-enabled on the way back out.
    CStdString match;
    /*    if (item->GetFocusedLayout())
     match = item->GetFocusedLayout()->GetAllText();
     else if (item->GetLayout())
     match = item->GetLayout()->GetAllText();
     else*/
    match = item->GetLabel(); // Filter label only for now
    
    if (numericMatch)
      StringUtils::WordToDigits(match);
    
    size_t pos = StringUtils::FindWords(match.c_str(), trimmedFilter.c_str());
    if (pos != CStdString::npos)
      filteredItems.Add(item);
  }

  items.ClearItems();
  items.Append(filteredItems);

  return items.GetObjectCount() > 0;
}

bool CGUIMediaWindow::GetAdvanceFilteredItems(CFileItemList &items)
{
  // don't run the advanced filter if the filter is empty
  // and there hasn't been a filter applied before which
  // would have to be removed
  CURL url(m_strFilterPath);
  if (m_filter.IsEmpty() && !url.HasOption("filter"))
    return false;

  CFileItemList resultItems;
  XFILE::CSmartPlaylistDirectory::GetDirectory(m_filter, resultItems, m_strFilterPath, true);

  // put together a lookup map for faster path comparison
  map<CStdString, CFileItemPtr> lookup;
  for (int j = 0; j < resultItems.Size(); j++)
  {
    CStdString itemPath = RemoveParameterFromPath(resultItems[j]->GetPath(), "filter");
    itemPath.ToLower();

    lookup[itemPath] = resultItems[j];
  }

  // loop through all the original items and find
  // those which are still part of the filter
  CFileItemList filteredItems;
  for (int i = 0; i < items.Size(); i++)
  {
    CFileItemPtr item = items.Get(i);
    if (item->IsParentFolder())
    {
      filteredItems.Add(item);
      continue;
    }

    // check if the item is part of the resultItems list
    // by comparing their paths (but ignoring any special
    // options because they differ from filter to filter)
    CStdString path = RemoveParameterFromPath(item->GetPath(), "filter");
    path.ToLower();

    map<CStdString, CFileItemPtr>::iterator itItem = lookup.find(path);
    if (itItem != lookup.end())
    {
      // add the item to the list of filtered items
      filteredItems.Add(item);

      // remove the item from the lists
      resultItems.Remove(itItem->second.get());
      lookup.erase(itItem);
    }
  }

  if (resultItems.Size() > 0)
    CLog::Log(LOGWARNING, "CGUIMediaWindow::GetAdvanceFilteredItems(): %d unknown items", resultItems.Size());

  items.ClearItems();
  items.Append(filteredItems);
  items.SetPath(resultItems.GetPath());
  if (resultItems.HasProperty(PROPERTY_PATH_DB))
    items.SetProperty(PROPERTY_PATH_DB, resultItems.GetProperty(PROPERTY_PATH_DB));
  return true;
}

bool CGUIMediaWindow::IsFiltered()
{
  return (!m_canFilterAdvanced && !GetProperty("filter").empty()) ||
         (m_canFilterAdvanced && !m_filter.IsEmpty());
}

bool CGUIMediaWindow::Filter()
{
  // basic filtering
  if (!m_canFilterAdvanced)
  {
    const CGUIControl *btnFilter = GetControl(CONTROL_BTN_FILTER);
    if (btnFilter != NULL && btnFilter->GetControlType() == CGUIControl::GUICONTROL_EDIT)
    { // filter updated
      CGUIMessage selected(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_BTN_FILTER);
      OnMessage(selected);
      OnFilterItems(selected.GetLabel());
      return true;
    }
    if (GetProperty("filter").empty())
    {
      CStdString filter = GetProperty("filter").asString();
      CGUIKeyboardFactory::ShowAndGetFilter(filter, false);
      SetProperty("filter", filter);
    }
    else
      OnFilterItems("");
  }
  // advanced filtering
  else
    CGUIDialogMediaFilter::ShowAndEditMediaFilter(m_strFilterPath, m_filter);

  return true;
}

CStdString CGUIMediaWindow::GetStartFolder(const CStdString &dir)
{
  if (dir.Equals("$ROOT") || dir.Equals("Root"))
    return "";
  return dir;
}

CStdString CGUIMediaWindow::RemoveParameterFromPath(const CStdString &strDirectory, const CStdString &strParameter)
{
  CURL url(strDirectory);
  if (url.HasOption(strParameter))
  {
    url.RemoveOption(strParameter);
    return url.Get();
  }

  return strDirectory;
}
