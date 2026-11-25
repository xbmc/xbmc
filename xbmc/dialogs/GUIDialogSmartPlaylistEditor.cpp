/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogSmartPlaylistEditor.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "GUIDialogContextMenu.h"
#include "GUIDialogSelect.h"
#include "GUIDialogSmartPlaylistRule.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "filesystem/File.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIWindowManager.h"
#include "input/actions/ActionIDs.h"
#include "media/LibraryQueryType.h"
#include "profiles/ProfileManager.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/SortUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"

#include <cassert>
#include <string_view>
#include <utility>

using namespace KODI;

#define CONTROL_HEADING         2
#define CONTROL_RULE_LIST       10
#define CONTROL_NAME            12
#define CONTROL_RULE_ADD        13
#define CONTROL_RULE_REMOVE     14
#define CONTROL_RULE_EDIT       15
#define CONTROL_MATCH           16
#define CONTROL_LIMIT           17
#define CONTROL_ORDER_FIELD     18
#define CONTROL_ORDER_DIRECTION 19
#define CONTROL_GROUP_BY        23
#define CONTROL_GROUP_MIXED     24
constexpr int CONTROL_WATCHED_MODE = 25;

#define CONTROL_OK              20
#define CONTROL_CANCEL          21
#define CONTROL_TYPE            22

namespace LIBRARY_QUERY = KODI::MEDIA::LIBRARY_QUERY;
using KODI::MEDIA::MediaSection;

CGUIDialogSmartPlaylistEditor::CGUIDialogSmartPlaylistEditor(void)
    : CGUIDialog(WINDOW_DIALOG_SMART_PLAYLIST_EDITOR, "SmartPlaylistEditor.xml")
{
  m_cancelled = false;
  m_ruleLabels = new CFileItemList;
  m_loadType = KEEP_IN_MEMORY;
}

CGUIDialogSmartPlaylistEditor::~CGUIDialogSmartPlaylistEditor()
{
  delete m_ruleLabels;
}

bool CGUIDialogSmartPlaylistEditor::OnBack(int actionID)
{
  m_cancelled = true;
  return CGUIDialog::OnBack(actionID);
}

bool CGUIDialogSmartPlaylistEditor::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      int iAction = message.GetParam1();
      if (iControl == CONTROL_RULE_LIST && (iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK))
        OnRuleList(GetSelectedItem());
      else if (iControl == CONTROL_RULE_ADD)
        OnRuleAdd();
      else if (iControl == CONTROL_RULE_EDIT)
        OnRuleList(GetSelectedItem());
      else if (iControl == CONTROL_RULE_REMOVE)
        OnRuleRemove(GetSelectedItem());
      else if (iControl == CONTROL_NAME)
        OnName();
      else if (iControl == CONTROL_OK)
        OnOK();
      else if (iControl == CONTROL_CANCEL)
        OnCancel();
      else if (iControl == CONTROL_MATCH)
        OnMatch();
      else if (iControl == CONTROL_LIMIT)
        OnLimit();
      else if (iControl == CONTROL_ORDER_FIELD)
        OnOrder();
      else if (iControl == CONTROL_ORDER_DIRECTION)
        OnOrderDirection();
      else if (iControl == CONTROL_TYPE)
        OnType();
      else if (iControl == CONTROL_GROUP_BY)
        OnGroupBy();
      else if (iControl == CONTROL_GROUP_MIXED)
        OnGroupMixed();
      else if (iControl == CONTROL_RULE_LIST && (iAction == ACTION_CONTEXT_MENU || iAction == ACTION_MOUSE_RIGHT_CLICK))
        OnPopupMenu(GetSelectedItem());
      else if (iControl == CONTROL_WATCHED_MODE)
        OnWatchedMode();
      else
        return CGUIDialog::OnMessage(message);
      return true;
    }
    break;
  case GUI_MSG_FOCUSED:
    if (message.GetControlId() == CONTROL_RULE_REMOVE ||
        message.GetControlId() == CONTROL_RULE_EDIT)
      HighlightItem(GetSelectedItem());
    else
    {
      if (message.GetControlId() == CONTROL_RULE_LIST)
        UpdateRuleControlButtons();

      HighlightItem(-1);
    }
    break;
  case GUI_MSG_WINDOW_INIT:
    {
      const std::string& startupList = message.GetStringParam(0);
      if (!startupList.empty())
      {
        int party = 0;
        if (URIUtils::PathEquals(startupList, CServiceBroker::GetSettingsComponent()->GetProfileManager()->GetUserDataItem("PartyMode.xsp")))
          party = 1;
        else if (URIUtils::PathEquals(startupList, CServiceBroker::GetSettingsComponent()->GetProfileManager()->GetUserDataItem("PartyMode-Video.xsp")))
          party = 2;

        if ((party && !XFILE::CFile::Exists(startupList)) ||
             m_playlist.Load(startupList))
        {
          m_path = startupList;

          if (party == 1)
            m_mode = Mode::PARTY_MUSIC;
          else if (party == 2)
            m_mode = Mode::PARTY_VIDEO;
          else
          {
            m_mode = LIBRARY_QUERY::ListsFrom(m_playlist.GetType(), MediaSection::VIDEO)
                         ? Mode::VIDEO
                         : Mode::MUSIC;
          }
        }
        else
          return false;
      }
    }
    break;
    case GUI_MSG_WINDOW_DEINIT:
    {
      m_playlist.Reset();
    }
    break;
  }
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogSmartPlaylistEditor::OnPopupMenu(int item)
{
  if (item < 0 || static_cast<size_t>(item) >= m_playlist.GetRules().size())
    return;
  // highlight the item
  m_ruleLabels->Get(item)->Select(true);

  CContextButtons choices;
  choices.Add(1, 15015);

  int button = CGUIDialogContextMenu::ShowAndGetChoice(choices);

  // unhighlight the item
  m_ruleLabels->Get(item)->Select(false);

  if (button == 1)
    OnRuleRemove(item);
}

void CGUIDialogSmartPlaylistEditor::OnRuleList(int item)
{
  if (item < 0 || item > static_cast<int>(m_playlist.GetRules().size()))
    return;
  if (item == static_cast<int>(m_playlist.GetRules().size()))
    OnRuleAdd();
  else
  {
    const std::shared_ptr<MEDIA::CLibraryQueryRule> rule{m_playlist.GetRules()[item]};
    MEDIA::CLibraryQueryRule ruleCopy{*rule};
    if (CGUIDialogSmartPlaylistRule::EditRule(ruleCopy, m_playlist.GetType()))
      *rule = ruleCopy;
  }
  UpdateButtons();
}

void CGUIDialogSmartPlaylistEditor::OnOK()
{
  std::string systemPlaylistsPath = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_SYSTEM_PLAYLISTSPATH);
  const std::string saveFolder{LIBRARY_QUERY::SaveFolderOf(m_playlist.GetType())};
  // save our playlist
  if (m_path.empty())
  {
    std::string filename(CUtil::MakeLegalFileName(m_playlist.GetName()));
    std::string path;
    if (CGUIKeyboardFactory::ShowAndGetInput(
            filename,
            CVariant{CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(16013)},
            false))
    {
      path = URIUtils::AddFileToFolder(systemPlaylistsPath, saveFolder,
                                       CUtil::MakeLegalFileName(std::move(filename)));
    }
    else
      return;
    if (!URIUtils::HasExtension(path, ".xsp"))
      path += ".xsp";

    // should we check whether we should overwrite?
    m_path = path;
  }
  else
  {
    // check if we need to actually change the save location for this playlist
    // this occurs if the user switches from music video <> songs <> mixed
    if (StringUtils::StartsWith(m_path, systemPlaylistsPath))
    {
      std::string filename = URIUtils::GetFileName(m_path);
      std::string strFolder = m_path.substr(systemPlaylistsPath.size(), m_path.size() - filename.size() - systemPlaylistsPath.size() - 1);
      if (strFolder != saveFolder)
      { // move to the correct folder
        XFILE::CFile::Delete(m_path);
        m_path = URIUtils::AddFileToFolder(systemPlaylistsPath, saveFolder, filename);
      }
    }
  }

  m_playlist.Save(m_path);

  m_cancelled = false;
  Close();
}

void CGUIDialogSmartPlaylistEditor::OnCancel()
{
  m_cancelled = true;
  Close();
}

void CGUIDialogSmartPlaylistEditor::OnMatch()
{
  // toggle between AND and OR setting
  m_playlist.SetMatchAllRules(!m_playlist.GetMatchAllRules());
  UpdateButtons();
}

void CGUIDialogSmartPlaylistEditor::OnName()
{
  std::string name = m_playlist.GetName();
  if (CGUIKeyboardFactory::ShowAndGetInput(name, CVariant{16012}, false))
  {
    m_playlist.SetName(name);
    UpdateButtons();
  }
}

void CGUIDialogSmartPlaylistEditor::OnLimit()
{
  std::vector<int> limits = {0, 10, 25, 50, 100, 250, 500, 1000};
  CGUIDialogSelect* dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
  dialog->Reset();
  int selected = -1;
  for (auto limit = limits.begin(); limit != limits.end(); limit++)
  {
    if (*limit == static_cast<int>(m_playlist.GetLimit()))
      selected = std::distance(limits.begin(), limit);
    if (*limit == 0)
      dialog->Add(CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(21428));
    else
      dialog->Add(StringUtils::Format(
          CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(21436), *limit));
  }
  dialog->SetHeading(CVariant{ 21427 });
  dialog->SetSelected(selected);
  dialog->Open();
  int newSelected = dialog->GetSelectedItem();
  if (!dialog->IsConfirmed() || newSelected < 0 ||
      limits[newSelected] == static_cast<int>(m_playlist.GetLimit()))
    return;
  m_playlist.SetLimit(limits[newSelected]);
  UpdateButtons();
}

void CGUIDialogSmartPlaylistEditor::OnType()
{
  std::vector<LIBRARY_QUERY::TYPE> allowedTypes = GetAllowedTypes(m_mode);
  CGUIDialogSelect* dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
  dialog->Reset();
  for (auto allowedType: allowedTypes)
    dialog->Add(GetLocalizedType(allowedType));
  dialog->SetHeading(CVariant{ 564 });
  dialog->SetSelected(GetLocalizedType(m_playlist.GetType()));
  dialog->Open();
  int newSelected = dialog->GetSelectedItem();
  if (!dialog->IsConfirmed() || newSelected < 0 ||
      allowedTypes[newSelected] == m_playlist.GetType())
    return;

  m_playlist.SetType(allowedTypes[newSelected]);

  // Remove any invalid grouping left over when changing the type
  const LIBRARY_QUERY::GROUP currentGroup{m_playlist.GetGroup()};
  if (currentGroup != LIBRARY_QUERY::GROUP::NONE && currentGroup != LIBRARY_QUERY::GROUP::DEFAULT)
  {
    const std::vector<LIBRARY_QUERY::GROUP> groups{
        MEDIA::CLibraryQueryRule::GetGroups(m_playlist.GetType())};
    if (std::ranges::find(groups, currentGroup) == groups.end())
      m_playlist.SetGroup(LIBRARY_QUERY::GROUP::DEFAULT);
  }

  UpdateButtons();
}

void CGUIDialogSmartPlaylistEditor::OnOrder()
{
  std::vector<SortBy> orders = MEDIA::CLibraryQueryRule::GetOrders(m_playlist.GetType());
  CGUIDialogSelect* dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
  dialog->Reset();
  for (auto order: orders)
    dialog->Add(CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(
        SortUtils::GetSortLabel(order)));
  dialog->SetHeading(CVariant{ 21429 });
  dialog->SetSelected(CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(
      SortUtils::GetSortLabel(m_playlist.GetOrder())));
  dialog->Open();
  int newSelected = dialog->GetSelectedItem();
  if (!dialog->IsConfirmed() || newSelected < 0 || orders[newSelected] == m_playlist.GetOrder())
    return;
  m_playlist.SetOrder(orders[newSelected]);
  UpdateButtons();
}

void CGUIDialogSmartPlaylistEditor::OnOrderDirection()
{
  m_playlist.SetOrderAscending(m_playlist.GetOrderDirection() == SortOrder::DESCENDING);
  UpdateButtons();
}

void CGUIDialogSmartPlaylistEditor::OnGroupBy()
{
  const LIBRARY_QUERY::TYPE type{m_playlist.GetType()};
  const std::vector<LIBRARY_QUERY::GROUP> groups{MEDIA::CLibraryQueryRule::GetGroups(type)};
  const LIBRARY_QUERY::GROUP currentGroup{m_playlist.GetGroup()};
  CGUIDialogSelect* dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
  dialog->Reset();
  for (auto group : groups)
    dialog->Add(MEDIA::CLibraryQueryRule::GetLocalizedGroup(group, type));
  dialog->SetHeading(CVariant{ 21458 });
  dialog->SetSelected(MEDIA::CLibraryQueryRule::GetLocalizedGroup(currentGroup, type));
  dialog->Open();
  int newSelected = dialog->GetSelectedItem();
   // check if selection has changed
  if (!dialog->IsConfirmed() || newSelected < 0 || groups[newSelected] == currentGroup)
    return;
  m_playlist.SetGroup(groups[newSelected]);

  if (m_playlist.IsGroupMixed() && !LIBRARY_QUERY::CanMix(currentGroup))
    m_playlist.SetGroupMixed(false);

  UpdateButtons();
}

void CGUIDialogSmartPlaylistEditor::OnGroupMixed()
{
  m_playlist.SetGroupMixed(!m_playlist.IsGroupMixed());
  UpdateButtons();
}

void CGUIDialogSmartPlaylistEditor::OnWatchedMode()
{
  // only valid for video types
  if (!LIBRARY_QUERY::ListsFrom(m_playlist.GetType(), MediaSection::VIDEO))
    return;

  CGUIDialogSelect* dialog =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(
          WINDOW_DIALOG_SELECT);
  dialog->Reset();
  dialog->SetHeading(CVariant{20479}); // "Watched Mode"

  // Special entry to leave the watched mode of the result alone
  dialog->Add(CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(571)); // "Default"

  // Actual watched mode values
  for (int i = 0; i < CMediaSettings::WatchedModesCount(); i++)
    dialog->Add(CMediaSettings::GetInstance().LocalizeWatchedMode(WatchedMode{i}));

  // Dialog indexes: 0 for nullopt, index = value + 1 otherwise
  const int currentIndex = m_playlist.GetWatchedMode().has_value()
                               ? static_cast<int>(m_playlist.GetWatchedMode().value()) + 1
                               : 0;
  dialog->SetSelected(currentIndex);
  dialog->Open();

  const int newSelected = dialog->GetSelectedItem();
  if (!dialog->IsConfirmed() || newSelected < 0 || newSelected == currentIndex)
    return;

  if (newSelected == 0)
    m_playlist.SetWatchedMode(std::nullopt);
  else
    m_playlist.SetWatchedMode(WatchedMode{newSelected - 1});

  UpdateButtons();
}

void CGUIDialogSmartPlaylistEditor::UpdateButtons()
{
  CONTROL_ENABLE(CONTROL_OK); // always enabled since we can have no rules -> match everything (as we do with default partymode playlists)

  if (m_mode == Mode::PARTY_VIDEO || m_mode == Mode::PARTY_MUSIC)
  {
    SET_CONTROL_LABEL2(CONTROL_NAME,
                       CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(16035));
    CONTROL_DISABLE(CONTROL_NAME);
  }
  else
    SET_CONTROL_LABEL2(CONTROL_NAME, m_playlist.GetName());

  UpdateRuleControlButtons();

  if (!m_playlist.GetMatchAllRules())
    SET_CONTROL_LABEL2(CONTROL_MATCH,
                       CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(
                           21426)); // one or more of the rules
  else
    SET_CONTROL_LABEL2(CONTROL_MATCH,
                       CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(
                           21425)); // all of the rules
  CONTROL_ENABLE_ON_CONDITION(CONTROL_MATCH, m_playlist.GetRules().size() > 1);
  if (m_playlist.GetLimit() == 0)
    SET_CONTROL_LABEL2(
        CONTROL_LIMIT,
        CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(21428)); // no limit
  else
    SET_CONTROL_LABEL2(
        CONTROL_LIMIT,
        StringUtils::Format(CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(21436),
                            m_playlist.GetLimit()));
  int currentItem = GetSelectedItem();
  CGUIMessage msgReset(GUI_MSG_LABEL_RESET, GetID(), CONTROL_RULE_LIST);
  OnMessage(msgReset);
  m_ruleLabels->Clear();
  for (const auto& rule : m_playlist.GetRules())
  {
    CFileItemPtr item(new CFileItem("", false));
    item->SetLabel(rule->GetLocalizedRule());
    m_ruleLabels->Add(item);
  }
  CFileItemPtr item(new CFileItem("", false));
  item->SetLabel(CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(21423));
  m_ruleLabels->Add(item);
  CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_RULE_LIST, 0, 0, m_ruleLabels);
  OnMessage(msg);
  SendMessage(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_RULE_LIST, currentItem);

  if (m_playlist.GetOrderDirection() != SortOrder::DESCENDING)
  {
    SET_CONTROL_LABEL2(CONTROL_ORDER_DIRECTION,
                       CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(21430));
  }
  else
  {
    SET_CONTROL_LABEL2(CONTROL_ORDER_DIRECTION,
                       CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(21431));
  }

  SET_CONTROL_LABEL2(CONTROL_ORDER_FIELD,
                     CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(
                         SortUtils::GetSortLabel(m_playlist.GetOrder())));
  SET_CONTROL_LABEL2(CONTROL_TYPE, GetLocalizedType(m_playlist.GetType()));

  // setup groups
  const std::vector<LIBRARY_QUERY::GROUP> groups{
      MEDIA::CLibraryQueryRule::GetGroups(m_playlist.GetType())};
  const LIBRARY_QUERY::GROUP currentGroup{m_playlist.GetGroup()};
  SET_CONTROL_LABEL2(CONTROL_GROUP_BY, MEDIA::CLibraryQueryRule::GetLocalizedGroup(
                                           currentGroup, m_playlist.GetType()));
  if (m_playlist.IsGroupMixed())
    CONTROL_SELECT(CONTROL_GROUP_MIXED);
  else
    CONTROL_DESELECT(CONTROL_GROUP_MIXED);

  // disable the group controls if there's no group
  // or only one group which can't be mixed
  if (groups.empty() || (groups.size() == 1 && !LIBRARY_QUERY::CanMix(groups[0])))
  {
    CONTROL_DISABLE(CONTROL_GROUP_BY);
    CONTROL_DISABLE(CONTROL_GROUP_MIXED);
  }
  else
  {
    CONTROL_ENABLE(CONTROL_GROUP_BY);
    CONTROL_ENABLE_ON_CONDITION(CONTROL_GROUP_MIXED, LIBRARY_QUERY::CanMix(currentGroup));
  }

  // watched mode
  if (LIBRARY_QUERY::ListsFrom(m_playlist.GetType(), MediaSection::VIDEO))
  {
    const std::string label =
        m_playlist.GetWatchedMode().has_value()
            ? CMediaSettings::GetInstance().LocalizeWatchedMode(*m_playlist.GetWatchedMode())
            : CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(571); // "Default"
    SET_CONTROL_LABEL2(CONTROL_WATCHED_MODE, label);
    CONTROL_ENABLE(CONTROL_WATCHED_MODE);
  }
  else
  {
    SET_CONTROL_LABEL2(CONTROL_WATCHED_MODE, "");
    CONTROL_DISABLE(CONTROL_WATCHED_MODE);
  }
}

void CGUIDialogSmartPlaylistEditor::UpdateRuleControlButtons()
{
  const auto iSize = static_cast<int>(m_playlist.GetRules().size());
  int iItem = GetSelectedItem();
  // only enable the remove control if ...
  CONTROL_ENABLE_ON_CONDITION(CONTROL_RULE_REMOVE,
                              iSize > 0 && // there is at least one item
                                  iItem >= 0 && iItem < iSize && // and a valid item is selected
                                  m_playlist.GetRules()[iItem]->m_field !=
                                      Field::NONE); // and it is not be empty
}

void CGUIDialogSmartPlaylistEditor::OnInitWindow()
{
  m_cancelled = false;

  std::vector<LIBRARY_QUERY::TYPE> allowedTypes = GetAllowedTypes(m_mode);
  // check if our playlist type is allowed
  const LIBRARY_QUERY::TYPE type{m_playlist.GetType()};
  bool allowed = false;
  for (auto allowedType: allowedTypes)
  {
    if (type == allowedType)
      allowed = true;
  }
  if (!allowed && !allowedTypes.empty())
    m_playlist.SetType(allowedTypes[0]);

  UpdateButtons();

  SET_CONTROL_LABEL(CONTROL_HEADING, 21432);

  CGUIDialog::OnInitWindow();
}

void CGUIDialogSmartPlaylistEditor::OnDeinitWindow(int nextWindowID)
{
  CGUIDialog::OnDeinitWindow(nextWindowID);
  SendMessage(GUI_MSG_LABEL_RESET, CONTROL_RULE_LIST);
  SendMessage(GUI_MSG_LABEL_RESET, CONTROL_TYPE);
  m_ruleLabels->Clear();
}

std::string CGUIDialogSmartPlaylistEditor::GetLocalizedType(LIBRARY_QUERY::TYPE type)
{
  return CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(
      LIBRARY_QUERY::LabelOf(type));
}

int CGUIDialogSmartPlaylistEditor::GetSelectedItem()
{
  CGUIMessage message(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_RULE_LIST);
  OnMessage(message);
  return message.GetParam1();
}

void CGUIDialogSmartPlaylistEditor::HighlightItem(int item)
{
  for (int i = 0; i < m_ruleLabels->Size(); i++)
    (*m_ruleLabels)[i]->Select(false);
  if (item >= 0 && item < m_ruleLabels->Size())
    (*m_ruleLabels)[item]->Select(true);
  CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_RULE_LIST, item);
  OnMessage(msg);
}

std::vector<LIBRARY_QUERY::TYPE> CGUIDialogSmartPlaylistEditor::GetAllowedTypes(Mode mode)
{
  std::vector<LIBRARY_QUERY::TYPE> allowedTypes;
  if (mode == Mode::PARTY_MUSIC)
  {
    allowedTypes.push_back(LIBRARY_QUERY::TYPE::SONGS);
    allowedTypes.push_back(LIBRARY_QUERY::TYPE::MIXED);
  }
  else if (mode == Mode::PARTY_VIDEO)
  {
    allowedTypes.push_back(LIBRARY_QUERY::TYPE::MUSIC_VIDEOS);
    allowedTypes.push_back(LIBRARY_QUERY::TYPE::MIXED);
  }
  else if (mode == Mode::MUSIC)
  { // music types + mixed
    allowedTypes.push_back(LIBRARY_QUERY::TYPE::SONGS);
    allowedTypes.push_back(LIBRARY_QUERY::TYPE::ALBUMS);
    allowedTypes.push_back(LIBRARY_QUERY::TYPE::ARTISTS);
    allowedTypes.push_back(LIBRARY_QUERY::TYPE::MIXED);
  }
  else if (mode == Mode::VIDEO)
  { // general category for videos
    allowedTypes.push_back(LIBRARY_QUERY::TYPE::MOVIES);
    allowedTypes.push_back(LIBRARY_QUERY::TYPE::TV_SHOWS);
    allowedTypes.push_back(LIBRARY_QUERY::TYPE::EPISODES);
    allowedTypes.push_back(LIBRARY_QUERY::TYPE::MUSIC_VIDEOS);
    allowedTypes.push_back(LIBRARY_QUERY::TYPE::MIXED);
  }
  return allowedTypes;
}

void CGUIDialogSmartPlaylistEditor::OnRuleRemove(int item)
{
  if (item < 0 || item >= static_cast<int>(m_playlist.GetRules().size()))
    return;

  m_playlist.RemoveRule(item);

  UpdateButtons();
  if (item >= m_ruleLabels->Size())
    HighlightItem(m_ruleLabels->Size() - 1);
  else
    HighlightItem(item);
}

void CGUIDialogSmartPlaylistEditor::OnRuleAdd()
{
  const auto rule{std::make_shared<MEDIA::CLibraryQueryRule>()};
  if (CGUIDialogSmartPlaylistRule::EditRule(*rule, m_playlist.GetType()))
    m_playlist.AddRule(rule);
  UpdateButtons();
}

bool CGUIDialogSmartPlaylistEditor::NewPlaylist(const std::string &type)
{
  CGUIDialogSmartPlaylistEditor *editor = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSmartPlaylistEditor>(WINDOW_DIALOG_SMART_PLAYLIST_EDITOR);
  if (!editor) return false;

  editor->m_path = "";
  editor->m_playlist = MEDIA::CLibraryQuery();
  editor->m_mode = type == "music" ? Mode::MUSIC : type == "video" ? Mode::VIDEO : Mode::NONE;
  editor->Initialize();
  editor->Open();
  return !editor->m_cancelled;
}

bool CGUIDialogSmartPlaylistEditor::EditPlaylist(const std::string& path, Mode mode)
{
  CGUIDialogSmartPlaylistEditor *editor = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSmartPlaylistEditor>(WINDOW_DIALOG_SMART_PLAYLIST_EDITOR);
  if (!editor) return false;

  editor->m_mode = mode;
  if (URIUtils::PathEquals(path, CServiceBroker::GetSettingsComponent()->GetProfileManager()->GetUserDataItem("PartyMode.xsp")))
    editor->m_mode = Mode::PARTY_MUSIC;
  if (URIUtils::PathEquals(path, CServiceBroker::GetSettingsComponent()->GetProfileManager()->GetUserDataItem("PartyMode-Video.xsp")))
    editor->m_mode = Mode::PARTY_VIDEO;

  MEDIA::CLibraryQuery playlist;
  bool loaded(playlist.Load(path));
  if (!loaded)
  { // failed to load
    if (editor->m_mode != Mode::PARTY_MUSIC && editor->m_mode != Mode::PARTY_VIDEO)
      return false; // only edit normal playlists that exist
    // party mode playlists can be edited even if they don't exist
    playlist.SetType(editor->m_mode == Mode::PARTY_MUSIC ? LIBRARY_QUERY::TYPE::SONGS
                                                         : LIBRARY_QUERY::TYPE::MUSIC_VIDEOS);
  }

  editor->m_playlist = playlist;
  editor->m_path = path;
  editor->Initialize();
  editor->Open();
  return !editor->m_cancelled;
}
