/*
 *  Copyright (C) 2005-2018 Team Kodi
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
#include "guilib/LocalizeStrings.h"
#include "input/actions/ActionIDs.h"
#include "profiles/ProfileManager.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/SortUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"

#include <utility>

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

#define CONTROL_OK              20
#define CONTROL_CANCEL          21
#define CONTROL_TYPE            22

typedef struct
{
  CGUIDialogSmartPlaylistEditor::PLAYLIST_TYPE type;
  char string[13];
  int localizedString;
} translateType;

static const translateType types[] = { { CGUIDialogSmartPlaylistEditor::TYPE_SONGS, "songs", 134 },
                                       { CGUIDialogSmartPlaylistEditor::TYPE_ALBUMS, "albums", 132 },
                                       { CGUIDialogSmartPlaylistEditor::TYPE_ARTISTS, "artists", 133 },
                                       { CGUIDialogSmartPlaylistEditor::TYPE_MIXED, "mixed", 20395 },
                                       { CGUIDialogSmartPlaylistEditor::TYPE_MUSICVIDEOS, "musicvideos", 20389 },
                                       { CGUIDialogSmartPlaylistEditor::TYPE_MOVIES, "movies", 20342 },
                                       { CGUIDialogSmartPlaylistEditor::TYPE_TVSHOWS, "tvshows", 20343 },
                                       { CGUIDialogSmartPlaylistEditor::TYPE_EPISODES, "episodes", 20360 }
                                     };

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
            m_mode = "partymusic";
          else if (party == 2)
            m_mode = "partyvideo";
          else
          {
            PLAYLIST_TYPE type = ConvertType(m_playlist.GetType());
            if (type == TYPE_SONGS || type == TYPE_ALBUMS || type == TYPE_ARTISTS)
              m_mode = "music";
            else
              m_mode = "video";
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
  if (item < 0 || static_cast<size_t>(item) >= m_playlist.m_ruleCombination.m_rules.size())
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
  if (item < 0 || item > static_cast<int>(m_playlist.m_ruleCombination.m_rules.size()))
    return;
  if (item == static_cast<int>(m_playlist.m_ruleCombination.m_rules.size()))
    OnRuleAdd();
  else
  {
    CSmartPlaylistRule rule = *std::static_pointer_cast<CSmartPlaylistRule>(m_playlist.m_ruleCombination.m_rules[item]);
    if (CGUIDialogSmartPlaylistRule::EditRule(rule, m_playlist.GetType()))
      *m_playlist.m_ruleCombination.m_rules[item] = rule;
  }
  UpdateButtons();
}

void CGUIDialogSmartPlaylistEditor::OnOK()
{
  std::string systemPlaylistsPath = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_SYSTEM_PLAYLISTSPATH);
  // save our playlist
  if (m_path.empty())
  {
    std::string filename(CUtil::MakeLegalFileName(m_playlist.m_playlistName));
    std::string path;
    if (CGUIKeyboardFactory::ShowAndGetInput(filename, CVariant{g_localizeStrings.Get(16013)}, false))
    {
      path = URIUtils::AddFileToFolder(systemPlaylistsPath, m_playlist.GetSaveLocation(),
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
      if (strFolder != m_playlist.GetSaveLocation())
      { // move to the correct folder
        XFILE::CFile::Delete(m_path);
        m_path = URIUtils::AddFileToFolder(systemPlaylistsPath, m_playlist.GetSaveLocation(), filename);
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
  if (m_playlist.m_ruleCombination.GetType() == CSmartPlaylistRuleCombination::CombinationOr)
    m_playlist.m_ruleCombination.SetType(CSmartPlaylistRuleCombination::CombinationAnd);
  else
    m_playlist.m_ruleCombination.SetType(CSmartPlaylistRuleCombination::CombinationOr);
  UpdateButtons();
}

void CGUIDialogSmartPlaylistEditor::OnName()
{
  std::string name = m_playlist.m_playlistName;
  if (CGUIKeyboardFactory::ShowAndGetInput(name, CVariant{16012}, false))
  {
    m_playlist.m_playlistName = name;
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
    if (*limit == static_cast<int>(m_playlist.m_limit))
      selected = std::distance(limits.begin(), limit);
    if (*limit == 0)
      dialog->Add(g_localizeStrings.Get(21428));
    else
      dialog->Add(StringUtils::Format(g_localizeStrings.Get(21436), *limit));
  }
  dialog->SetHeading(CVariant{ 21427 });
  dialog->SetSelected(selected);
  dialog->Open();
  int newSelected = dialog->GetSelectedItem();
  if (!dialog->IsConfirmed() || newSelected < 0 || limits[newSelected] == static_cast<int>(m_playlist.m_limit))
    return;
  m_playlist.m_limit = limits[newSelected];
  UpdateButtons();
}

void CGUIDialogSmartPlaylistEditor::OnType()
{
  std::vector<PLAYLIST_TYPE> allowedTypes = GetAllowedTypes(m_mode);
  CGUIDialogSelect* dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
  dialog->Reset();
  for (auto allowedType: allowedTypes)
    dialog->Add(GetLocalizedType(allowedType));
  dialog->SetHeading(CVariant{ 564 });
  dialog->SetSelected(GetLocalizedType(ConvertType(m_playlist.GetType())));
  dialog->Open();
  int newSelected = dialog->GetSelectedItem();
  if (!dialog->IsConfirmed() || newSelected < 0 || allowedTypes[newSelected] == ConvertType(m_playlist.GetType()))
    return;

  m_playlist.SetType(ConvertType(allowedTypes[newSelected]));
  
  // Remove any invalid grouping left over when changing the type
  Field currentGroup = CSmartPlaylistRule::TranslateGroup(m_playlist.GetGroup().c_str());
  if (currentGroup != FieldNone && currentGroup != FieldUnknown)
  {
    std::vector<Field> groups = CSmartPlaylistRule::GetGroups(m_playlist.GetType());
    if (std::find(groups.begin(), groups.end(), currentGroup) == groups.end())
      m_playlist.SetGroup(CSmartPlaylistRule::TranslateGroup(FieldUnknown));
  }

  UpdateButtons();
}

void CGUIDialogSmartPlaylistEditor::OnOrder()
{
  std::vector<SortBy> orders = CSmartPlaylistRule::GetOrders(m_playlist.GetType());
  CGUIDialogSelect* dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
  dialog->Reset();
  for (auto order: orders)
    dialog->Add(g_localizeStrings.Get(SortUtils::GetSortLabel(order)));
  dialog->SetHeading(CVariant{ 21429 });
  dialog->SetSelected(g_localizeStrings.Get(SortUtils::GetSortLabel(m_playlist.m_orderField)));
  dialog->Open();
  int newSelected = dialog->GetSelectedItem();
  if (!dialog->IsConfirmed() || newSelected < 0 || orders[newSelected] == m_playlist.m_orderField)
    return;
  m_playlist.m_orderField = orders[newSelected];
  UpdateButtons();
}

void CGUIDialogSmartPlaylistEditor::OnOrderDirection()
{
  if (m_playlist.m_orderDirection == SortOrderDescending)
    m_playlist.m_orderDirection = SortOrderAscending;
  else
    m_playlist.m_orderDirection = SortOrderDescending;
  UpdateButtons();
}

void CGUIDialogSmartPlaylistEditor::OnGroupBy()
{
  std::vector<Field> groups = CSmartPlaylistRule::GetGroups(m_playlist.GetType());
  Field currentGroup = CSmartPlaylistRule::TranslateGroup(m_playlist.GetGroup().c_str());
  CGUIDialogSelect* dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
  dialog->Reset();
  for (auto group : groups)
    dialog->Add(CSmartPlaylistRule::GetLocalizedGroup(group));
  dialog->SetHeading(CVariant{ 21458 });
  dialog->SetSelected(CSmartPlaylistRule::GetLocalizedGroup(currentGroup));
  dialog->Open();
  int newSelected = dialog->GetSelectedItem();
   // check if selection has changed
  if (!dialog->IsConfirmed() || newSelected < 0 || groups[newSelected] == currentGroup)
    return;
  m_playlist.SetGroup(CSmartPlaylistRule::TranslateGroup(groups[newSelected]));

  if (m_playlist.IsGroupMixed() && !CSmartPlaylistRule::CanGroupMix(currentGroup))
    m_playlist.SetGroupMixed(false);

  UpdateButtons();
}

void CGUIDialogSmartPlaylistEditor::OnGroupMixed()
{
  m_playlist.SetGroupMixed(!m_playlist.IsGroupMixed());
  UpdateButtons();
}

void CGUIDialogSmartPlaylistEditor::UpdateButtons()
{
  CONTROL_ENABLE(CONTROL_OK); // always enabled since we can have no rules -> match everything (as we do with default partymode playlists)

  if (m_mode == "partyvideo" || m_mode == "partymusic")
  {
    SET_CONTROL_LABEL2(CONTROL_NAME, g_localizeStrings.Get(16035));
    CONTROL_DISABLE(CONTROL_NAME);
  }
  else
    SET_CONTROL_LABEL2(CONTROL_NAME, m_playlist.m_playlistName);

  UpdateRuleControlButtons();

  if (m_playlist.m_ruleCombination.GetType() == CSmartPlaylistRuleCombination::CombinationOr)
    SET_CONTROL_LABEL2(CONTROL_MATCH, g_localizeStrings.Get(21426)); // one or more of the rules
  else
    SET_CONTROL_LABEL2(CONTROL_MATCH, g_localizeStrings.Get(21425)); // all of the rules
  CONTROL_ENABLE_ON_CONDITION(CONTROL_MATCH, m_playlist.m_ruleCombination.m_rules.size() > 1);
  if (m_playlist.m_limit == 0)
    SET_CONTROL_LABEL2(CONTROL_LIMIT, g_localizeStrings.Get(21428)); // no limit
  else
    SET_CONTROL_LABEL2(CONTROL_LIMIT,
                       StringUtils::Format(g_localizeStrings.Get(21436), m_playlist.m_limit));
  int currentItem = GetSelectedItem();
  CGUIMessage msgReset(GUI_MSG_LABEL_RESET, GetID(), CONTROL_RULE_LIST);
  OnMessage(msgReset);
  m_ruleLabels->Clear();
  for (const auto& rule: m_playlist.m_ruleCombination.m_rules)
  {
    CFileItemPtr item(new CFileItem("", false));
    item->SetLabel(std::static_pointer_cast<CSmartPlaylistRule>(rule)->GetLocalizedRule());
    m_ruleLabels->Add(item);
  }
  CFileItemPtr item(new CFileItem("", false));
  item->SetLabel(g_localizeStrings.Get(21423));
  m_ruleLabels->Add(item);
  CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_RULE_LIST, 0, 0, m_ruleLabels);
  OnMessage(msg);
  SendMessage(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_RULE_LIST, currentItem);

  if (m_playlist.m_orderDirection != SortOrderDescending)
  {
    SET_CONTROL_LABEL2(CONTROL_ORDER_DIRECTION, g_localizeStrings.Get(21430));
  }
  else
  {
    SET_CONTROL_LABEL2(CONTROL_ORDER_DIRECTION, g_localizeStrings.Get(21431));
  }

  SET_CONTROL_LABEL2(CONTROL_ORDER_FIELD, g_localizeStrings.Get(SortUtils::GetSortLabel(m_playlist.m_orderField)));
  SET_CONTROL_LABEL2(CONTROL_TYPE, GetLocalizedType(ConvertType(m_playlist.GetType())));

  // setup groups
  std::vector<Field> groups = CSmartPlaylistRule::GetGroups(m_playlist.GetType());
  Field currentGroup = CSmartPlaylistRule::TranslateGroup(m_playlist.GetGroup().c_str());
  SET_CONTROL_LABEL2(CONTROL_GROUP_BY, CSmartPlaylistRule::GetLocalizedGroup(currentGroup));
  if (m_playlist.IsGroupMixed())
    CONTROL_SELECT(CONTROL_GROUP_MIXED);
  else
    CONTROL_DESELECT(CONTROL_GROUP_MIXED);

  // disable the group controls if there's no group
  // or only one group which can't be mixed
  if (groups.empty() ||
     (groups.size() == 1 && !CSmartPlaylistRule::CanGroupMix(groups[0])))
  {
    CONTROL_DISABLE(CONTROL_GROUP_BY);
    CONTROL_DISABLE(CONTROL_GROUP_MIXED);
  }
  else
  {
    CONTROL_ENABLE(CONTROL_GROUP_BY);
    CONTROL_ENABLE_ON_CONDITION(CONTROL_GROUP_MIXED, CSmartPlaylistRule::CanGroupMix(currentGroup));
  }
}

void CGUIDialogSmartPlaylistEditor::UpdateRuleControlButtons()
{
  int iSize = m_playlist.m_ruleCombination.m_rules.size();
  int iItem = GetSelectedItem();
  // only enable the remove control if ...
  CONTROL_ENABLE_ON_CONDITION(CONTROL_RULE_REMOVE,
                              iSize > 0 && // there is at least one item
                              iItem >= 0 && iItem < iSize && // and a valid item is selected
                              m_playlist.m_ruleCombination.m_rules[iItem]->m_field != FieldNone); // and it is not be empty
}

void CGUIDialogSmartPlaylistEditor::OnInitWindow()
{
  m_cancelled = false;

  std::vector<PLAYLIST_TYPE> allowedTypes = GetAllowedTypes(m_mode);
  // check if our playlist type is allowed
  PLAYLIST_TYPE type = ConvertType(m_playlist.GetType());
  bool allowed = false;
  for (auto allowedType: allowedTypes)
  {
    if (type == allowedType)
      allowed = true;
  }
  if (!allowed && allowedTypes.size())
    m_playlist.SetType(ConvertType(allowedTypes[0]));

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

CGUIDialogSmartPlaylistEditor::PLAYLIST_TYPE CGUIDialogSmartPlaylistEditor::ConvertType(const std::string &type)
{
  for (const translateType& t : types)
    if (type == t.string)
      return t.type;
  assert(false);
  return TYPE_SONGS;
}

std::string CGUIDialogSmartPlaylistEditor::GetLocalizedType(PLAYLIST_TYPE type)
{
  for (const translateType& t : types)
    if (t.type == type)
      return g_localizeStrings.Get(t.localizedString);
  assert(false);
  return "";
}

std::string CGUIDialogSmartPlaylistEditor::ConvertType(PLAYLIST_TYPE type)
{
  for (const translateType& t : types)
    if (t.type == type)
      return t.string;
  assert(false);
  return "songs";
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

std::vector<CGUIDialogSmartPlaylistEditor::PLAYLIST_TYPE> CGUIDialogSmartPlaylistEditor::GetAllowedTypes(const std::string& mode)
{
  std::vector<PLAYLIST_TYPE> allowedTypes;
  if (mode == "partymusic")
  {
    allowedTypes.push_back(TYPE_SONGS);
    allowedTypes.push_back(TYPE_MIXED);
  }
  else if (mode == "partyvideo")
  {
    allowedTypes.push_back(TYPE_MUSICVIDEOS);
    allowedTypes.push_back(TYPE_MIXED);
  }
  else if (mode == "music")
  { // music types + mixed
    allowedTypes.push_back(TYPE_SONGS);
    allowedTypes.push_back(TYPE_ALBUMS);
    allowedTypes.push_back(TYPE_ARTISTS);
    allowedTypes.push_back(TYPE_MIXED);
  }
  else if (mode == "video")
  { // general category for videos
    allowedTypes.push_back(TYPE_MOVIES);
    allowedTypes.push_back(TYPE_TVSHOWS);
    allowedTypes.push_back(TYPE_EPISODES);
    allowedTypes.push_back(TYPE_MUSICVIDEOS);
    allowedTypes.push_back(TYPE_MIXED);
  }
  return allowedTypes;
}

void CGUIDialogSmartPlaylistEditor::OnRuleRemove(int item)
{
  if (item < 0 || item >= (int)m_playlist.m_ruleCombination.m_rules.size()) return;
  m_playlist.m_ruleCombination.m_rules.erase(m_playlist.m_ruleCombination.m_rules.begin() + item);

  UpdateButtons();
  if (item >= m_ruleLabels->Size())
    HighlightItem(m_ruleLabels->Size() - 1);
  else
    HighlightItem(item);
}

void CGUIDialogSmartPlaylistEditor::OnRuleAdd()
{
  CSmartPlaylistRule rule;
  if (CGUIDialogSmartPlaylistRule::EditRule(rule,m_playlist.GetType()))
    m_playlist.m_ruleCombination.AddRule(rule);
  UpdateButtons();
}

bool CGUIDialogSmartPlaylistEditor::NewPlaylist(const std::string &type)
{
  CGUIDialogSmartPlaylistEditor *editor = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSmartPlaylistEditor>(WINDOW_DIALOG_SMART_PLAYLIST_EDITOR);
  if (!editor) return false;

  editor->m_path = "";
  editor->m_playlist = CSmartPlaylist();
  editor->m_mode = type;
  editor->Initialize();
  editor->Open();
  return !editor->m_cancelled;
}

bool CGUIDialogSmartPlaylistEditor::EditPlaylist(const std::string &path, const std::string &type)
{
  CGUIDialogSmartPlaylistEditor *editor = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSmartPlaylistEditor>(WINDOW_DIALOG_SMART_PLAYLIST_EDITOR);
  if (!editor) return false;

  editor->m_mode = type;
  if (URIUtils::PathEquals(path, CServiceBroker::GetSettingsComponent()->GetProfileManager()->GetUserDataItem("PartyMode.xsp")))
    editor->m_mode = "partymusic";
  if (URIUtils::PathEquals(path, CServiceBroker::GetSettingsComponent()->GetProfileManager()->GetUserDataItem("PartyMode-Video.xsp")))
    editor->m_mode = "partyvideo";

  CSmartPlaylist playlist;
  bool loaded(playlist.Load(path));
  if (!loaded)
  { // failed to load
    if (!StringUtils::StartsWithNoCase(editor->m_mode, "party"))
      return false; // only edit normal playlists that exist
    // party mode playlists can be edited even if they don't exist
    playlist.SetType(editor->m_mode == "partymusic" ? "songs" : "musicvideos");
  }

  editor->m_playlist = playlist;
  editor->m_path = path;
  editor->Initialize();
  editor->Open();
  return !editor->m_cancelled;
}
