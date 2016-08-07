/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "GUIMediaWindow.h"
#include "Application.h"
#include "messaging/ApplicationMessenger.h"
#include "ContextMenuManager.h"
#include "FileItemListModification.h"
#include "GUIPassword.h"
#include "GUIUserMessages.h"
#include "PartyModeManager.h"
#include "PlayListPlayer.h"
#include "URL.h"
#include "Util.h"
#include "addons/AddonManager.h"
#include "addons/GUIDialogAddonSettings.h"
#include "addons/PluginSource.h"
#if defined(TARGET_ANDROID)
#include "platform/android/activity/XBMCApp.h"
#endif
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogMediaFilter.h"
#include "dialogs/GUIDialogMediaSource.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogSmartPlaylistEditor.h"
#include "filesystem/FavouritesDirectory.h"
#include "filesystem/File.h"
#include "filesystem/FileDirectoryFactory.h"
#include "filesystem/MultiPathDirectory.h"
#include "filesystem/PluginDirectory.h"
#include "filesystem/SmartPlaylistDirectory.h"
#include "guilib/GUIEditControl.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "interfaces/builtins/Builtins.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "input/Key.h"
#include "network/Network.h"
#include "playlists/PlayList.h"
#include "profiles/ProfilesManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "storage/MediaManager.h"
#include "threads/SystemClock.h"
#include "utils/FileUtils.h"
#include "utils/LabelFormatter.h"
#include "utils/log.h"
#include "utils/SortUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "video/VideoLibraryQueue.h"
#include "view/GUIViewState.h"

#define CONTROL_BTNVIEWASICONS       2
#define CONTROL_BTNSORTBY            3
#define CONTROL_BTNSORTASC           4
#define CONTROL_BTN_FILTER          19

#define CONTROL_LABELFILES          12

#define CONTROL_VIEW_START          50
#define CONTROL_VIEW_END            59

#define PROPERTY_PATH_DB            "path.db"
#define PROPERTY_SORT_ORDER         "sort.order"
#define PROPERTY_SORT_ASCENDING     "sort.ascending"

#define PLUGIN_REFRESH_DELAY 200

using namespace ADDON;
using namespace KODI::MESSAGING;

CGUIMediaWindow::CGUIMediaWindow(int id, const char *xmlFile)
    : CGUIWindow(id, xmlFile)
{
  m_loadType = KEEP_IN_MEMORY;
  m_vecItems = new CFileItemList;
  m_unfilteredItems = new CFileItemList;
  m_vecItems->SetPath("?");
  m_iLastControl = -1;
  m_canFilterAdvanced = false;

  m_guiState.reset(CGUIViewState::GetViewState(GetID(), *m_vecItems));
}

CGUIMediaWindow::~CGUIMediaWindow()
{
  delete m_vecItems;
  delete m_unfilteredItems;
}

void CGUIMediaWindow::LoadAdditionalTags(TiXmlElement *root)
{
  CGUIWindow::LoadAdditionalTags(root);
  // configure our view control
  m_viewControl.Reset();
  m_viewControl.SetParentWindow(GetID());
  TiXmlElement *element = root->FirstChildElement("views");
  if (element && element->FirstChild())
  { // format is <views>50,29,51,95</views>
    const std::string &allViews = element->FirstChild()->ValueStr();
    std::vector<std::string> views = StringUtils::Split(allViews, ",");
    for (std::vector<std::string>::const_iterator i = views.begin(); i != views.end(); ++i)
    {
      int controlID = atol(i->c_str());
      CGUIControl *control = GetControl(controlID);
      if (control && control->IsContainer())
        m_viewControl.AddView(control);
    }
  }
  else
  { // backward compatibility
    std::vector<CGUIControl *> controls;
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
    std::string filter = StringUtils::Format("%i", (int)(action.GetID() - ACTION_FILTER_SMS2 + 2));
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
  if (actionID == ACTION_NAV_BACK &&
      !m_vecItems->IsVirtualDirectoryRoot() &&
      !URIUtils::PathEquals(m_vecItems->GetPath(), GetRootPath(), true) &&
     (!URIUtils::PathEquals(m_vecItems->GetPath(), m_startDirectory, true) || (m_canFilterAdvanced && filterUrl.HasOption("filter"))))
  {
    if (GoParentFolder())
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
      m_iLastControl = GetFocusedControlID();
      CGUIWindow::OnMessage(message);

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
        if (m_guiState.get() && m_guiState->ChooseSortMethod())
          UpdateFileList();
        return true;
      }
      else if (iControl == CONTROL_BTN_FILTER)
        return Filter(false);
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
          Refresh(true);
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
        CFileItemPtr newItem = std::static_pointer_cast<CFileItem>(message.GetItem());
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
          items.SetPath(URIUtils::GetDirectory(newItem->GetPath()));
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
        std::string filter = GetProperty("filter").asString();
        // check if this is meant for advanced filtering
        if (message.GetParam2() != 10)
        {
          if (message.GetParam2() == 1) // append
            filter += message.GetStringParam();
          else if (message.GetParam2() == 2)
          { // delete
            if (filter.size())
              filter.erase(filter.size() - 1);
          }
          else
            filter = message.GetStringParam();
        }
        OnFilterItems(filter);
        UpdateButtons();
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
      std::string dir = message.GetStringParam(0);
      const std::string &ret = message.GetStringParam(1);
      bool returning = StringUtils::EqualsNoCase(ret, "return");
      if (!dir.empty())
      {
        // ensure our directory is valid
        dir = GetStartFolder(dir);
        bool resetHistory = false;
        if (!returning || !m_history.IsInHistory(dir))
        { // we're not returning to the same path, so set our directory to the requested path
          m_vecItems->SetPath(dir);
          resetHistory = true;
        }
        // check for network up
        if (URIUtils::IsRemote(m_vecItems->GetPath()) && !WaitForNetwork())
        {
          m_vecItems->SetPath("");
          resetHistory = true;
        }
        if (resetHistory)
          SetHistoryForPath(m_vecItems->GetPath());
      }
      if (message.GetParam1() != WINDOW_INVALID)
      { // first time to this window - make sure we set the root path
        m_startDirectory = returning ? dir : GetRootPath();
      }
      if (message.GetParam2() == PLUGIN_REFRESH_DELAY)
      {
        Refresh();
        return true;
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
    if (m_guiState->GetSortOrder() == SortOrderNone)
    {
      CONTROL_DISABLE(CONTROL_BTNSORTASC);
    }
    else
    {
      CONTROL_ENABLE(CONTROL_BTNSORTASC);
      SET_CONTROL_SELECTED(GetID(), CONTROL_BTNSORTASC, m_guiState->GetSortOrder() != SortOrderAscending);
    }

    // Update list/thumb control
    m_viewControl.SetCurrentView(m_guiState->GetViewAsControl());

    // Update sort by button
    if (!m_guiState->HasMultipleSortMethods())
      CONTROL_DISABLE(CONTROL_BTNSORTBY);
    else
      CONTROL_ENABLE(CONTROL_BTNSORTBY);

    std::string sortLabel = StringUtils::Format(g_localizeStrings.Get(550).c_str(),
                                                g_localizeStrings.Get(m_guiState->GetSortMethodLabel()).c_str());
    SET_CONTROL_LABEL(CONTROL_BTNSORTBY, sortLabel);
  }

  std::string items = StringUtils::Format("%i %s", m_vecItems->GetObjectCount(), g_localizeStrings.Get(127).c_str());
  SET_CONTROL_LABEL(CONTROL_LABELFILES, items);

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
  std::unique_ptr<CGUIViewState> guiState(CGUIViewState::GetViewState(GetID(), items));

  if (guiState.get())
  {
    SortDescription sorting = guiState->GetSortMethod();
    sorting.sortOrder = guiState->GetSortOrder();
    // If the sort method is "sort by playlist" and we have a specific
    // sort order available we can use the specified sort order to do the sorting
    // We do this as the new SortBy methods are a superset of the SORT_METHOD methods, thus
    // not all are available. This may be removed once SORT_METHOD_* have been replaced by
    // SortBy.
    if ((sorting.sortBy == SortByPlaylistOrder) && items.HasProperty(PROPERTY_SORT_ORDER))
    {
      SortBy sortBy = (SortBy)items.GetProperty(PROPERTY_SORT_ORDER).asInteger();
      if (sortBy != SortByNone && sortBy != SortByPlaylistOrder && sortBy != SortByProgramCount)
      {
        sorting.sortBy = sortBy;
        sorting.sortOrder = items.GetProperty(PROPERTY_SORT_ASCENDING).asBoolean() ? SortOrderAscending : SortOrderDescending;
        sorting.sortAttributes = CSettings::GetInstance().GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING) ? SortAttributeIgnoreArticle : SortAttributeNone;

        // if the sort order is descending, we need to switch the original sort order, as we assume
        // in CGUIViewState::AddPlaylistOrder that SortByPlaylistOrder is ascending.
        if (guiState->GetSortOrder() == SortOrderDescending)
          sorting.sortOrder = sorting.sortOrder == SortOrderDescending ? SortOrderAscending : SortOrderDescending;
      }
    }

    items.Sort(sorting);
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

  if (items.GetSortMethod() == SortByLabel)
    items.ClearSortState();
}

// \brief Prepares and adds the fileitems list/thumb panel
void CGUIMediaWindow::FormatAndSort(CFileItemList &items)
{
  std::unique_ptr<CGUIViewState> viewState(CGUIViewState::GetViewState(GetID(), items));

  if (viewState.get())
  {
    LABEL_MASKS labelMasks;
    viewState->GetSortMethodLabelMasks(labelMasks);
    FormatItemLabels(items, labelMasks);

    items.Sort(viewState->GetSortMethod().sortBy, viewState->GetSortOrder(), viewState->GetSortMethod().sortAttributes);
  }
}

/*!
  \brief Overwrite to fill fileitems from a source
  \param strDirectory Path to read
  \param items Fill with items specified in \e strDirectory
  */
bool CGUIMediaWindow::GetDirectory(const std::string &strDirectory, CFileItemList &items)
{
  const CURL pathToUrl(strDirectory);

  std::string strParentPath = m_history.GetParentPath();

  CLog::Log(LOGDEBUG,"CGUIMediaWindow::GetDirectory (%s)",
            CURL::GetRedacted(strDirectory).c_str());
  CLog::Log(LOGDEBUG,"  ParentPath = [%s]", CURL::GetRedacted(strParentPath).c_str());

  if (pathToUrl.IsProtocol("plugin"))
    CAddonMgr::GetInstance().UpdateLastUsed(pathToUrl.GetHostName());

  // see if we can load a previously cached folder
  CFileItemList cachedItems(strDirectory);
  if (!strDirectory.empty() && cachedItems.Load(GetID()))
  {
    items.Assign(cachedItems);
  }
  else
  {
    unsigned int time = XbmcThreads::SystemClockMillis();

    if (strDirectory.empty())
      SetupShares();
    
    CFileItemList dirItems;
    if (!m_rootDir.GetDirectory(pathToUrl, dirItems))
      return false;
    
    // assign fetched directory items
    items.Assign(dirItems);

    // took over a second, and not normally cached, so cache it
    if ((XbmcThreads::SystemClockMillis() - time) > 1000  && items.CacheToDiscIfSlow())
      items.Save(GetID());

    // if these items should replace the current listing, then pop it off the top
    if (items.GetReplaceListing())
      m_history.RemoveParentPath();
  }

  // update the view state's reference to the current items
  m_guiState.reset(CGUIViewState::GetViewState(GetID(), items));

  if (m_guiState.get() && !m_guiState->HideParentDirItems() && items.GetPath() != GetRootPath())
  {
    CFileItemPtr pItem(new CFileItem(".."));
    pItem->SetPath(strParentPath);
    pItem->m_bIsFolder = true;
    pItem->m_bIsShareOrDrive = false;
    items.AddFront(pItem, 0);
  }

  int iWindow = GetID();
  std::vector<std::string> regexps;

  //! @todo Do we want to limit the directories we apply the video ones to?
  if (iWindow == WINDOW_VIDEO_NAV)
    regexps = g_advancedSettings.m_videoExcludeFromListingRegExps;
  if (iWindow == WINDOW_MUSIC_NAV)
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

bool CGUIMediaWindow::Update(const std::string &strDirectory, bool updateFilterPath /* = true */)
{
  //! @todo OnInitWindow calls Update() before window path has been set properly.
  if (strDirectory == "?")
    return false;

  // The path to load. Empty string is used in various places to denote root, so translate to the
  // real root path first
  const std::string path = strDirectory.empty() ? GetRootPath() : strDirectory;

  // stores the selected item in history
  SaveSelectedItemInHistory();

  const std::string previousPath = m_vecItems->GetPath();

  // check if the path contains a filter and temporarily remove it
  // so that the retrieved list of items is unfiltered
  std::string pathNoFilter = path;
  if (CanContainFilter(pathNoFilter) && CURL(pathNoFilter).HasOption("filter"))
    pathNoFilter = RemoveParameterFromPath(pathNoFilter, "filter");

  if (!GetDirectory(pathNoFilter, *m_vecItems))
  {
    CLog::Log(LOGERROR,"CGUIMediaWindow::GetDirectory(%s) failed", CURL(path).GetRedacted().c_str());

    if (URIUtils::PathEquals(path, GetRootPath()))
      return false; // Nothing to fallback to

    // Try to return to the previous directory, if not the same
    // else fallback to root
    if (URIUtils::PathEquals(path, previousPath) || !Update(m_history.RemoveParentPath()))
      Update(""); // Fallback to root

    // Return false to be able to eg. show
    // an error message.
    return false;
  }

  if (m_vecItems->GetLabel().empty())
    m_vecItems->SetLabel(CUtil::GetTitleFromPath(m_vecItems->GetPath(), true));

  // check the given path for filter data
  UpdateFilterPath(path, *m_vecItems, updateFilterPath);
    
  // if we're getting the root source listing
  // make sure the path history is clean
  if (URIUtils::PathEquals(path, GetRootPath()))
    m_history.ClearPathHistory();

  int iWindow = GetID();
  int showLabel = 0;
  if (URIUtils::PathEquals(path, GetRootPath()))
  {
    if (iWindow == WINDOW_PICTURES)
      showLabel = 997;
    else if (iWindow == WINDOW_FILES)
      showLabel = 1026;
  }
  if (m_vecItems->IsPath("sources://video/"))
    showLabel = 999;
  else if (m_vecItems->IsPath("sources://music/"))
    showLabel = 998;
  else if (m_vecItems->IsPath("sources://pictures/"))
    showLabel = 997;
  else if (m_vecItems->IsPath("sources://files/"))
    showLabel = 1026;
  if (showLabel && (m_vecItems->Size() == 0 || !m_guiState->DisableAddSourceButtons())) // add 'add source button'
  {
    std::string strLabel = g_localizeStrings.Get(showLabel);
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

  m_vecItems->FillInDefaultIcons();

  // remember the original (untouched) list of items (for filtering etc)
  m_unfilteredItems->Assign(*m_vecItems);

  // Cache the list of items if possible
  OnCacheFileItems(*m_vecItems);

  // Filter and group the items if necessary
  OnFilterItems(GetProperty("filter").asString());
  UpdateButtons();

  // Restore selected item from history
  RestoreSelectedItemFromHistory();

  m_history.AddPath(m_vecItems->GetPath(), m_strFilterPath);

  //m_history.DumpPathHistory();

  return true;
}

bool CGUIMediaWindow::Refresh(bool clearCache /* = false */)
{
  std::string strCurrentDirectory = m_vecItems->GetPath();
  if (strCurrentDirectory == "?")
    return false;

  if (clearCache)
    m_vecItems->RemoveDiscCache(GetID());

  // get the original number of items
  if (!Update(strCurrentDirectory, false))
    return false;

  return true;
}

// \brief This function will be called by Update() before the
// labels of the fileitems are formatted. Override this function
// to set custom thumbs or load additional media info.
// It's used to load tag info for music.
void CGUIMediaWindow::OnPrepareFileItems(CFileItemList &items)
{
  CFileItemListModification::GetInstance().Modify(items);
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

// \brief With this function you can react on a users click in the list/thumb panel.
// It returns true, if the click is handled.
// This function calls OnPlayMedia()
bool CGUIMediaWindow::OnClick(int iItem, const std::string &player)
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
    if (CProfilesManager::GetInstance().IsMasterProfile())
    {
      if (!g_passwordManager.IsMasterLockUnlocked(true))
        return false;
    }
    else if (!CProfilesManager::GetInstance().GetCurrentProfile().canWriteSources() && !g_passwordManager.IsProfileLockUnlocked())
      return false;

    if (OnAddMediaSource())
      Refresh(true);

    return true;
  }

  if (!pItem->m_bIsFolder && pItem->IsFileFolder(EFILEFOLDER_MASK_ONCLICK))
  {
    XFILE::IFileDirectory *pFileDirectory = NULL;
    pFileDirectory = XFILE::CFileDirectoryFactory::Create(pItem->GetURL(), pItem.get(), "");
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
    if (CAddonMgr::GetInstance().GetAddon(url.GetHostName(), addon, ADDON_SCRIPT))
    {
      if (!CScriptInvocationManager::GetInstance().Stop(addon->LibPath()))
      {
        CAddonMgr::GetInstance().UpdateLastUsed(addon->ID());
        CScriptInvocationManager::GetInstance().ExecuteAsync(addon->LibPath(), addon);
      }
      return true;
    }
  }

  if (pItem->m_bIsFolder)
  {
    if ( pItem->m_bIsShareOrDrive )
    {
      const std::string& strLockType=m_guiState->GetLockType();
      if (CProfilesManager::GetInstance().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE)
        if (!strLockType.empty() && !g_passwordManager.IsItemUnlocked(pItem.get(), strLockType))
            return true;

      if (!HaveDiscOrConnection(pItem->GetPath(), pItem->m_iDriveType))
        return true;
    }

    // check for the partymode playlist items - they may not exist yet
    if ((pItem->GetPath() == CProfilesManager::GetInstance().GetUserDataItem("PartyMode.xsp")) ||
        (pItem->GetPath() == CProfilesManager::GetInstance().GetUserDataItem("PartyMode-Video.xsp")))
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
    std::string strCurrentDirectory = m_vecItems->GetPath();
    if (m_canFilterAdvanced && !m_filter.IsEmpty() &&
        !URIUtils::PathEquals(m_strFilterPath, strCurrentDirectory))
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
    std::string appName = URIUtils::GetFileName(pItem->GetPath());
    CLog::Log(LOGDEBUG, "CGUIMediaWindow::OnClick Trying to run: %s",appName.c_str());
    return CXBMCApp::StartActivity(appName);
  }
#endif
  else
  {
    SaveSelectedItemInHistory();

    if (pItem->GetPath() == "newplaylist://")
    {
      m_vecItems->RemoveDiscCache(GetID());
      g_windowManager.ActivateWindow(WINDOW_MUSIC_PLAYLIST_EDITOR,"newplaylist://");
      return true;
    }
    else if (StringUtils::StartsWithNoCase(pItem->GetPath(), "newsmartplaylist://"))
    {
      m_vecItems->RemoveDiscCache(GetID());
      if (CGUIDialogSmartPlaylistEditor::NewPlaylist(pItem->GetPath().substr(19)))
        Refresh();
      return true;
    }

    bool autoplay = m_guiState.get() && m_guiState->AutoPlayNextItem();

    if (m_vecItems->IsPlugin())
    {
      CURL url(m_vecItems->GetPath());
      AddonPtr addon;
      if (CAddonMgr::GetInstance().GetAddon(url.GetHostName(),addon))
      {
        PluginPtr plugin = std::dynamic_pointer_cast<CPluginSource>(addon);
        if (plugin && plugin->Provides(CPluginSource::AUDIO))
        {
          CFileItemList items;
          std::unique_ptr<CGUIViewState> state(CGUIViewState::GetViewState(GetID(), items));
          autoplay = state.get() && state->AutoPlayNextItem();
        }
      }
    }

    if (autoplay && !g_partyModeManager.IsEnabled() && 
        !pItem->IsPlayList())
    {
      return OnPlayAndQueueMedia(pItem, player);
    }
    else
    {
      return OnPlayMedia(iItem, player);
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
bool CGUIMediaWindow::HaveDiscOrConnection(const std::string& strPath, int iDriveType)
{
  if (iDriveType==CMediaSource::SOURCE_TYPE_DVD)
  {
    if (!g_mediaManager.IsDiscInDrive(strPath))
    {
      CGUIDialogOK::ShowAndGetInput(CVariant{218}, CVariant{219});
      return false;
    }
  }
  else if (iDriveType==CMediaSource::SOURCE_TYPE_REMOTE)
  {
    //! @todo Handle not connected to a remote share
    if ( !g_application.getNetwork().IsConnected() )
    {
      CGUIDialogOK::ShowAndGetInput(CVariant{220}, CVariant{221});
      return false;
    }
  }

  return true;
}

// \brief Shows a standard errormessage for a given pItem.
void CGUIMediaWindow::ShowShareErrorMessage(CFileItem* pItem)
{
  if (!pItem->m_bIsShareOrDrive)
    return;

  int idMessageText = 0;
  CURL url(pItem->GetPath());

  if (url.IsProtocol("smb") && url.GetHostName().empty()) //  smb workgroup
    idMessageText = 15303; // Workgroup not found
  else if (pItem->m_iDriveType == CMediaSource::SOURCE_TYPE_REMOTE || URIUtils::IsRemote(pItem->GetPath()))
    idMessageText = 15301; // Could not connect to network server
  else
    idMessageText = 15300; // Path not found or invalid

  CGUIDialogOK::ShowAndGetInput(CVariant{220}, CVariant{idMessageText});
}

// \brief The functon goes up one level in the directory tree
bool CGUIMediaWindow::GoParentFolder()
{
  if (m_vecItems->IsVirtualDirectoryRoot())
    return false;

  if (URIUtils::PathEquals(m_vecItems->GetPath(), GetRootPath()))
    return false;

  //m_history.DumpPathHistory();

  const std::string currentPath = m_vecItems->GetPath();
  std::string parentPath = m_history.GetParentPath();
  // in case the path history is messed up and the current folder is on
  // the stack more than once, keep going until there's nothing left or they
  // dont match anymore.
  while (!parentPath.empty() && URIUtils::PathEquals(parentPath, currentPath, true))
  {
    m_history.RemoveParentPath();
    parentPath = m_history.GetParentPath();
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
      return true;
    }
  }

  // pop directory path from the stack
  m_strFilterPath = m_history.GetParentPath(true);
  m_history.RemoveParentPath();

  if (!Update(parentPath, false))
    return false;

  // No items to show so go another level up
  if (!m_vecItems->GetPath().empty() && (m_filter.IsEmpty() ? m_vecItems->Size() : m_unfilteredItems->Size()) <= 0)
  {
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(2080), g_localizeStrings.Get(2081));
    return GoParentFolder();
  }
  return true;
}

void CGUIMediaWindow::SaveSelectedItemInHistory()
{
  int iItem = m_viewControl.GetSelectedItem();
  std::string strSelectedItem;
  if (iItem >= 0 && iItem < m_vecItems->Size())
  {
    CFileItemPtr pItem = m_vecItems->Get(iItem);
    if (!pItem->IsParentFolder())
      GetDirectoryHistoryString(pItem.get(), strSelectedItem);
  }

  m_history.SetSelectedItem(strSelectedItem, m_vecItems->GetPath());
}

void CGUIMediaWindow::RestoreSelectedItemFromHistory()
{
  std::string strSelectedItem = m_history.GetSelectedItem(m_vecItems->GetPath());

  if (!strSelectedItem.empty())
  {
    for (int i = 0; i < m_vecItems->Size(); ++i)
    {
      CFileItemPtr pItem = m_vecItems->Get(i);
      std::string strHistory;
      GetDirectoryHistoryString(pItem.get(), strHistory);
      // set selected item if equals with history
      if (strHistory == strSelectedItem)
      {
        m_viewControl.SetSelectedItem(i);
        return;
      }
    }
  }

  // if we haven't found the selected item, select the first item
  m_viewControl.SetSelectedItem(0);
}

// \brief Override the function to change the default behavior on how
// a selected item history should look like
void CGUIMediaWindow::GetDirectoryHistoryString(const CFileItem* pItem, std::string& strHistoryString)
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
      std::string strLabel = pItem->GetLabel();
      size_t nPosOpen = strLabel.find('(');
      size_t nPosClose = strLabel.rfind(')');
      if (nPosOpen != std::string::npos &&
          nPosClose != std::string::npos &&
          nPosClose > nPosOpen)
      {
        strLabel.erase(nPosOpen + 1, (nPosClose) - (nPosOpen + 1));
        strHistoryString = strLabel;
      }
      else
        strHistoryString = strLabel;
    }
    else
    {
      // Other items in virual directory
      std::string strPath = pItem->GetPath();
      URIUtils::RemoveSlashAtEnd(strPath);

      strHistoryString = pItem->GetLabel() + strPath;
    }
  }
  else if (pItem->m_lEndOffset>pItem->m_lStartOffset && pItem->m_lStartOffset != -1)
  {
    // Could be a cue item, all items of a cue share the same filename
    // so add the offsets to build the history string
    strHistoryString = StringUtils::Format("%i%i",
                                           pItem->m_lStartOffset,
                                           pItem->m_lEndOffset);
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
  StringUtils::ToLower(strHistoryString);
}

// \brief Call this function to create a directory history for the
// path given by strDirectory.
void CGUIMediaWindow::SetHistoryForPath(const std::string& strDirectory)
{
  // Make sure our shares are configured
  SetupShares();
  if (!strDirectory.empty())
  {
    // Build the directory history for default path
    std::string strPath, strParentPath;
    strPath = strDirectory;
    URIUtils::RemoveSlashAtEnd(strPath);

    CFileItemList items;
    m_rootDir.GetDirectory(CURL(), items);

    m_history.ClearPathHistory();

    bool originalPath = true;
    while (URIUtils::GetParentPath(strPath, strParentPath))
    {
      for (int i = 0; i < (int)items.Size(); ++i)
      {
        CFileItemPtr pItem = items[i];
        std::string path(pItem->GetPath());
        URIUtils::RemoveSlashAtEnd(path);
        if (URIUtils::PathEquals(path, strPath))
        {
          std::string strHistory;
          GetDirectoryHistoryString(pItem.get(), strHistory);
          m_history.SetSelectedItem(strHistory, "");
          URIUtils::AddSlashAtEnd(strPath);
          m_history.AddPathFront(strPath);
          m_history.AddPathFront("");

          //m_history.DumpPathHistory();
          return ;
        }
      }

      if (URIUtils::IsVideoDb(strPath))
      {
        CURL url(strParentPath);
        url.SetOptions(""); // clear any URL options from recreated parent path
        strParentPath = url.Get();
      }

      // set the original path exactly as it was passed in
      if (URIUtils::PathEquals(strPath, strDirectory, true))
        strPath = strDirectory;
      else
        URIUtils::AddSlashAtEnd(strPath);

      m_history.AddPathFront(strPath, originalPath ? m_strFilterPath : "");
      m_history.SetSelectedItem(strPath, strParentPath);
      originalPath = false;
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
bool CGUIMediaWindow::OnPlayMedia(int iItem, const std::string &player)
{
  // Reset Playlistplayer, playback started now does
  // not use the playlistplayer.
  g_playlistPlayer.Reset();
  g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_NONE);
  CFileItemPtr pItem=m_vecItems->Get(iItem);

  CLog::Log(LOGDEBUG, "%s %s", __FUNCTION__, CURL::GetRedacted(pItem->GetPath()).c_str());

  bool bResult = false;
  if (pItem->IsInternetStream() || pItem->IsPlayList())
    bResult = g_application.PlayMedia(*pItem, player, m_guiState->GetPlaylist());
  else
    bResult = g_application.PlayFile(*pItem, player) == PLAYBACK_OK;

  if (pItem->m_lStartOffset == STARTOFFSET_RESUME)
    pItem->m_lStartOffset = 0;

  return bResult;
}

// \brief Override if you want to change the default behavior of what is done
// when the user clicks on a file in a "folder" with similar files.
// This function is called by OnClick()
bool CGUIMediaWindow::OnPlayAndQueueMedia(const CFileItemPtr &item, std::string player)
{
  //play and add current directory to temporary playlist
  int iPlaylist = m_guiState->GetPlaylist();
  if (iPlaylist != PLAYLIST_NONE)
  {
    g_playlistPlayer.ClearPlaylist(iPlaylist);
    g_playlistPlayer.Reset();
    int mediaToPlay = 0;
    
    // first try to find mainDVD file (VIDEO_TS.IFO). 
    // If we find this we should not allow to queue VOB files
    std::string mainDVD; 
    for (int i = 0; i < m_vecItems->Size(); i++) 
    { 
      std::string path = URIUtils::GetFileName(m_vecItems->Get(i)->GetPath()); 
      if (StringUtils::EqualsNoCase(path, "VIDEO_TS.IFO")) 
      { 
        mainDVD = path; 
        break; 
      } 
    }

    // now queue...
    for ( int i = 0; i < m_vecItems->Size(); i++ )
    {
      CFileItemPtr nItem = m_vecItems->Get(i);

      if (nItem->m_bIsFolder)
        continue;

      if (!nItem->IsPlayList() && !nItem->IsZIP() && !nItem->IsRAR() && (!nItem->IsDVDFile() || (URIUtils::GetFileName(nItem->GetPath()) == mainDVD)))
        g_playlistPlayer.Add(iPlaylist, nItem);

      if (item->IsSamePath(nItem.get()))
      { // item that was clicked
        mediaToPlay = g_playlistPlayer.GetPlaylist(iPlaylist).size() - 1;
      }
    }

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
    g_playlistPlayer.Play(mediaToPlay, player);
  }
  return true;
}

// \brief Synchonize the fileitems with the playlistplayer
// It recreated the playlist of the playlistplayer based
// on the fileitems of the window
void CGUIMediaWindow::UpdateFileList()
{
  int nItem = m_viewControl.GetSelectedItem();
  std::string strSelected;
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

  if (CProfilesManager::GetInstance().GetCurrentProfile().getLockMode() != LOCK_MODE_EVERYONE && CProfilesManager::GetInstance().GetCurrentProfile().filesLocked())
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

  if (CProfilesManager::GetInstance().GetCurrentProfile().getLockMode() != LOCK_MODE_EVERYONE && CProfilesManager::GetInstance().GetCurrentProfile().filesLocked())
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

  // the start directory may change during Refresh
  bool updateStartDirectory = URIUtils::PathEquals(m_vecItems->GetPath(), m_startDirectory, true);

  // we have python scripts hooked in everywhere :(
  // those scripts may open windows and we can't open a window
  // while opening this one.
  // for plugin sources delay call to Refresh
  if (!URIUtils::IsPlugin(m_vecItems->GetPath()))
  {
    Refresh();
  }
  else
  {
    CGUIMessage msg(GUI_MSG_WINDOW_INIT, 0, 0, 0, PLUGIN_REFRESH_DELAY);
    g_windowManager.SendThreadMessage(msg, m_controlID);
  }

  if (updateStartDirectory)
  {
    // reset the start directory to the path of the items
    m_startDirectory = m_vecItems->GetPath();

    // reset the history based on the path of the items
    SetHistoryForPath(m_startDirectory);
  }

  m_rootDir.SetAllowThreads(true);

  CGUIWindow::OnInitWindow();
}

void CGUIMediaWindow::SaveControlStates()
{
  CGUIWindow::SaveControlStates();
  SaveSelectedItemInHistory();
}

void CGUIMediaWindow::RestoreControlStates()
{
  CGUIWindow::RestoreControlStates();
  RestoreSelectedItemFromHistory();
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

bool CGUIMediaWindow::OnPopupMenu(int itemIdx)
{
  auto InRange = [](int i, std::pair<int, int> range){ return i >= range.first && i < range.second; };

  if (itemIdx < 0 || itemIdx >= m_vecItems->Size())
    return false;

  auto item = m_vecItems->Get(itemIdx);
  if (!item)
    return false;

  CContextButtons buttons;

  //Add items from plugin
  {
    int i = 0;
    while (item->HasProperty(StringUtils::Format("contextmenulabel(%i)", i)))
    {
      buttons.emplace_back(-buttons.size(), item->GetProperty(StringUtils::Format("contextmenulabel(%i)", i)).asString());
      ++i;
    }
  }
  auto pluginMenuRange = std::make_pair(0, buttons.size());

  //Add the global menu
  auto globalMenu = CContextMenuManager::GetInstance().GetItems(*item, CContextMenuManager::MAIN);
  auto globalMenuRange = std::make_pair(buttons.size(), buttons.size() + globalMenu.size());
  for (const auto& menu : globalMenu)
    buttons.emplace_back(-buttons.size(), menu->GetLabel(*item));

  //Add legacy items from windows
  auto windowMenuRange = std::make_pair(buttons.size(), -1);
  GetContextButtons(itemIdx, buttons);
  windowMenuRange.second = buttons.size();

  //Add addon menus
  auto addonMenu = CContextMenuManager::GetInstance().GetAddonItems(*item, CContextMenuManager::MAIN);
  auto addonMenuRange = std::make_pair(buttons.size(), buttons.size() + addonMenu.size());
  for (const auto& menu : addonMenu)
    buttons.emplace_back(-buttons.size(), menu->GetLabel(*item));

  if (buttons.empty())
    return true;

  int idx = CGUIDialogContextMenu::Show(buttons);
  if (idx < 0 || idx >= static_cast<int>(buttons.size()))
    return false;

  if (InRange(idx, pluginMenuRange))
  {
    CApplicationMessenger::GetInstance().SendMsg(TMSG_EXECUTE_BUILT_IN, -1, -1, nullptr,
        item->GetProperty(StringUtils::Format("contextmenuaction(%i)", idx - pluginMenuRange.first)).asString());
    return true;
  }

  if (InRange(idx, windowMenuRange))
    return OnContextButton(itemIdx, static_cast<CONTEXT_BUTTON>(buttons[idx].first));

  if (InRange(idx, globalMenuRange))
    return CONTEXTMENU::LoopFrom(*globalMenu[idx - globalMenuRange.first], item);

  return CONTEXTMENU::LoopFrom(*addonMenu[idx - addonMenuRange.first], item);
}

void CGUIMediaWindow::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  CFileItemPtr item = (itemNumber >= 0 && itemNumber < m_vecItems->Size()) ? m_vecItems->Get(itemNumber) : CFileItemPtr();

  if (!item || item->IsParentFolder())
    return;

  //! @todo FAVOURITES Conditions on masterlock and localisation
  if (!item->IsParentFolder() && !item->IsPath("add") && !item->IsPath("newplaylist://") &&
      !URIUtils::IsProtocol(item->GetPath(), "newsmartplaylist") && !URIUtils::IsProtocol(item->GetPath(), "newtag") &&
      !URIUtils::IsProtocol(item->GetPath(), "musicsearch") &&
      !StringUtils::StartsWith(item->GetPath(), "pvr://guide/") && !StringUtils::StartsWith(item->GetPath(), "pvr://timers/"))
  {
    if (XFILE::CFavouritesDirectory::IsFavourite(item.get(), GetID()))
      buttons.Add(CONTEXT_BUTTON_ADD_FAVOURITE, 14077);     // Remove Favourite
    else
      buttons.Add(CONTEXT_BUTTON_ADD_FAVOURITE, 14076);     // Add To Favourites;
  }

  if (item->IsFileFolder(EFILEFOLDER_MASK_ONBROWSE))
    buttons.Add(CONTEXT_BUTTON_BROWSE_INTO, 37015);

}

bool CGUIMediaWindow::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  switch (button)
  {
  case CONTEXT_BUTTON_ADD_FAVOURITE:
    {
      CFileItemPtr item = m_vecItems->Get(itemNumber);
      XFILE::CFavouritesDirectory::AddOrRemove(item.get(), GetID());
      return true;
    }
  case CONTEXT_BUTTON_BROWSE_INTO:
    {
      CFileItemPtr item = m_vecItems->Get(itemNumber);
      Update(item->GetPath());
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
  progress->SetHeading(CVariant{1040}); // Loading Directory
  progress->SetLine(1, CVariant{url.GetWithoutUserDetails()});
  progress->ShowProgressBar(false);
  progress->Open();
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

void CGUIMediaWindow::UpdateFilterPath(const std::string &strDirectory, const CFileItemList &items, bool updateFilterPath)
{
  bool canfilter = CanContainFilter(strDirectory);

  std::string filter;
  CURL url(strDirectory);
  if (canfilter && url.HasOption("filter"))
    filter = url.GetOption("filter");

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
      CLog::Log(LOGWARNING, "CGUIMediaWindow::UpdateFilterPath(): unable to load existing filter (%s)", filter.c_str());
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
}

void CGUIMediaWindow::OnFilterItems(const std::string &filter)
{
  CFileItemPtr currentItem;
  std::string currentItemPath;
  int item = m_viewControl.GetSelectedItem();
  if (item >= 0 && item < m_vecItems->Size())
  {
    currentItem = m_vecItems->Get(item);
    currentItemPath = currentItem->GetPath();
  }
  
  m_viewControl.Clear();
  
  CFileItemList items;
  items.Copy(*m_vecItems, false); // use the original path - it'll likely be relied on for other things later.
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
    // only set m_strFilterPath if it hasn't been set before
    // otherwise we might overwrite it with a non-filter path
    // in case GetFilteredItems() returns true even though no
    // db-based filter (e.g. watched filter) has been applied
    else if (m_strFilterPath.empty())
      m_strFilterPath = items.GetPath();
  }
  
  GetGroupedItems(*m_vecItems);
  FormatAndSort(*m_vecItems);

  // get the "filter" option
  std::string filterOption;
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
      if (!filterOption.empty())
        itemUrl.SetOption("filter", filterOption);
      else
        itemUrl.RemoveOption("filter");
      pItem->SetPath(itemUrl.Get());
    }
  }

  SetProperty("filter", filter);
  if (filtered && m_canFilterAdvanced)
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

  // and update our view control + buttons
  m_viewControl.SetItems(*m_vecItems);
  m_viewControl.SetSelectedItem(currentItemPath);
}

bool CGUIMediaWindow::GetFilteredItems(const std::string &filter, CFileItemList &items)
{
  bool result = false;
  if (m_canFilterAdvanced)
    result = GetAdvanceFilteredItems(items);

  std::string trimmedFilter(filter);
  StringUtils::TrimLeft(trimmedFilter);
  StringUtils::ToLower(trimmedFilter);
  
  if (trimmedFilter.empty())
    return result;

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
    //! @todo Need to update this to get all labels, ideally out of the displayed info (ie from m_layout and m_focusedLayout)
    //! though that isn't practical.  Perhaps a better idea would be to just grab the info that we should filter on based on
    //! where we are in the library tree.
    //! Another idea is tying the filter string to the current level of the tree, so that going deeper disables the filter,
    //! but it's re-enabled on the way back out.
    std::string match;
    /*    if (item->GetFocusedLayout())
     match = item->GetFocusedLayout()->GetAllText();
     else if (item->GetLayout())
     match = item->GetLayout()->GetAllText();
     else*/
    match = item->GetLabel(); // Filter label only for now
    
    if (numericMatch)
      StringUtils::WordToDigits(match);
    
    size_t pos = StringUtils::FindWords(match.c_str(), trimmedFilter.c_str());
    if (pos != std::string::npos)
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
  std::map<std::string, CFileItemPtr> lookup;
  for (int j = 0; j < resultItems.Size(); j++)
  {
    std::string itemPath = CURL(resultItems[j]->GetPath()).GetWithoutOptions();
    StringUtils::ToLower(itemPath);

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
    std::string path = CURL(item->GetPath()).GetWithoutOptions();
    StringUtils::ToLower(path);

    std::map<std::string, CFileItemPtr>::iterator itItem = lookup.find(path);
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

bool CGUIMediaWindow::IsSameStartFolder(const std::string &dir)
{
  const std::string startFolder = GetStartFolder(dir);
  return URIUtils::PathHasParent(m_vecItems->GetPath(), startFolder);
}

bool CGUIMediaWindow::Filter(bool advanced /* = true */)
{
  // basic filtering
  if (!m_canFilterAdvanced || !advanced)
  {
    const CGUIControl *btnFilter = GetControl(CONTROL_BTN_FILTER);
    if (btnFilter != NULL && btnFilter->GetControlType() == CGUIControl::GUICONTROL_EDIT)
    { // filter updated
      CGUIMessage selected(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_BTN_FILTER);
      OnMessage(selected);
      OnFilterItems(selected.GetLabel());
      UpdateButtons();
      return true;
    }
    if (GetProperty("filter").empty())
    {
      std::string filter = GetProperty("filter").asString();
      CGUIKeyboardFactory::ShowAndGetFilter(filter, false);
      SetProperty("filter", filter);
    }
    else
    {
      OnFilterItems("");
      UpdateButtons();
    }
  }
  // advanced filtering
  else
    CGUIDialogMediaFilter::ShowAndEditMediaFilter(m_strFilterPath, m_filter);

  return true;
}

std::string CGUIMediaWindow::GetStartFolder(const std::string &dir)
{
  std::string lower(dir); StringUtils::ToLower(lower);
  if (lower == "$root" || lower == "root")
    return "";
  return dir;
}

std::string CGUIMediaWindow::RemoveParameterFromPath(const std::string &strDirectory, const std::string &strParameter)
{
  CURL url(strDirectory);
  if (url.HasOption(strParameter))
  {
    url.RemoveOption(strParameter);
    return url.Get();
  }

  return strDirectory;
}

void CGUIMediaWindow::ProcessRenderLoop(bool renderOnly)
{
  g_windowManager.ProcessRenderLoop(renderOnly);
}
