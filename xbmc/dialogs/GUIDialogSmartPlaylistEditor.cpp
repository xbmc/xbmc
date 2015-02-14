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

#include "GUIDialogSmartPlaylistEditor.h"
#include "guilib/GUIKeyboardFactory.h"
#include "Util.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "GUIDialogSmartPlaylistRule.h"
#include "guilib/GUIWindowManager.h"
#include "filesystem/File.h"
#include "profiles/ProfilesManager.h"
#include "settings/Settings.h"
#include "FileItem.h"
#include "input/Key.h"
#include "guilib/LocalizeStrings.h"

using namespace std;

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

#define NUM_TYPES (sizeof(types) / sizeof(translateType))

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
        OnEditChanged(iControl, m_playlist.m_playlistName);
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
  }
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogSmartPlaylistEditor::OnRuleList(int item)
{
  if (item < 0 || item >= (int)m_playlist.m_ruleCombination.m_rules.size()) return;

  CSmartPlaylistRule rule = *std::static_pointer_cast<CSmartPlaylistRule>(m_playlist.m_ruleCombination.m_rules[item]);

  if (CGUIDialogSmartPlaylistRule::EditRule(rule,m_playlist.GetType()))
    *m_playlist.m_ruleCombination.m_rules[item] = rule;

  UpdateButtons();
}

void CGUIDialogSmartPlaylistEditor::OnOK()
{
  std::string systemPlaylistsPath = CSettings::Get().GetString("system.playlistspath");
  // save our playlist
  if (m_path.empty())
  {
    std::string filename(CUtil::MakeLegalFileName(m_playlist.m_playlistName));
    std::string path;
    if (CGUIKeyboardFactory::ShowAndGetInput(filename, g_localizeStrings.Get(16013), false))
    {
      path = URIUtils::AddFileToFolder(systemPlaylistsPath, m_playlist.GetSaveLocation());
      path = URIUtils::AddFileToFolder(path, CUtil::MakeLegalFileName(filename));
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
        m_path = URIUtils::AddFileToFolder(systemPlaylistsPath, m_playlist.GetSaveLocation());
        m_path = URIUtils::AddFileToFolder(m_path, filename);
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
  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_MATCH);
  OnMessage(msg);
  m_playlist.m_ruleCombination.SetType(msg.GetParam1() == 0 ? CSmartPlaylistRuleCombination::CombinationAnd : CSmartPlaylistRuleCombination::CombinationOr);
  UpdateButtons();
}

void CGUIDialogSmartPlaylistEditor::OnLimit()
{
  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_LIMIT);
  OnMessage(msg);
  m_playlist.m_limit = msg.GetParam1();
  UpdateButtons();
}

void CGUIDialogSmartPlaylistEditor::OnType()
{
  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_TYPE);
  OnMessage(msg);
  m_playlist.SetType(ConvertType((PLAYLIST_TYPE)msg.GetParam1()));
  UpdateButtons();
}

void CGUIDialogSmartPlaylistEditor::OnOrder()
{
  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_ORDER_FIELD);
  OnMessage(msg);
  m_playlist.m_orderField = (SortBy)msg.GetParam1();
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
  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_GROUP_BY);
  OnMessage(msg);
  m_playlist.SetGroup(CSmartPlaylistRule::TranslateGroup((Field)msg.GetParam1()));

  if (m_playlist.IsGroupMixed() && !CSmartPlaylistRule::CanGroupMix((Field)msg.GetParam1()))
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
  
  // if there's no rule available, add a dummy one the user can edit
  if (m_playlist.m_ruleCombination.m_rules.size() <= 0)
    m_playlist.m_ruleCombination.AddRule(CSmartPlaylistRule());

  // name
  if (m_mode == "partyvideo" || m_mode == "partymusic")
  {
    SET_CONTROL_LABEL2(CONTROL_NAME, g_localizeStrings.Get(16035));
    CONTROL_DISABLE(CONTROL_NAME);
  }
  else
    SET_CONTROL_LABEL2(CONTROL_NAME, m_playlist.m_playlistName);
  
  UpdateRuleControlButtons();

  CONTROL_ENABLE_ON_CONDITION(CONTROL_MATCH, m_playlist.m_ruleCombination.m_rules.size() > 1);

  int currentItem = GetSelectedItem();
  CGUIMessage msgReset(GUI_MSG_LABEL_RESET, GetID(), CONTROL_RULE_LIST);
  OnMessage(msgReset);
  m_ruleLabels->Clear();
  for (unsigned int i = 0; i < m_playlist.m_ruleCombination.m_rules.size(); i++)
  {
    CFileItemPtr item(new CFileItem("", false));
    if (m_playlist.m_ruleCombination.m_rules[i]->m_field == FieldNone)
      item->SetLabel(g_localizeStrings.Get(21423));
    else
      item->SetLabel(std::static_pointer_cast<CSmartPlaylistRule>(m_playlist.m_ruleCombination.m_rules[i])->GetLocalizedRule());
    m_ruleLabels->Add(item);
  }
  CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_RULE_LIST, 0, 0, m_ruleLabels);
  OnMessage(msg);
  SendMessage(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_RULE_LIST, currentItem);

  if (m_playlist.m_orderDirection != SortOrderDescending)
  {
    CONTROL_SELECT(CONTROL_ORDER_DIRECTION);
  }
  else
  {
    CONTROL_DESELECT(CONTROL_ORDER_DIRECTION);
  }

  // sort out the order fields
  std::vector< std::pair<std::string, int> > labels;
  vector<SortBy> orders = CSmartPlaylistRule::GetOrders(m_playlist.GetType());
  for (unsigned int i = 0; i < orders.size(); i++)
    labels.push_back(make_pair(g_localizeStrings.Get(SortUtils::GetSortLabel(orders[i])), orders[i]));
  SET_CONTROL_LABELS(CONTROL_ORDER_FIELD, m_playlist.m_orderField, &labels);

  // setup groups
  labels.clear();
  vector<Field> groups = CSmartPlaylistRule::GetGroups(m_playlist.GetType());
  Field currentGroup = CSmartPlaylistRule::TranslateGroup(m_playlist.GetGroup().c_str());
  for (unsigned int i = 0; i < groups.size(); i++)
    labels.push_back(make_pair(CSmartPlaylistRule::GetLocalizedGroup(groups[i]), groups[i]));
  SET_CONTROL_LABELS(CONTROL_GROUP_BY, currentGroup, &labels);

  if (m_playlist.IsGroupMixed())
    CONTROL_SELECT(CONTROL_GROUP_MIXED);
  else
    CONTROL_DESELECT(CONTROL_GROUP_MIXED);

  // disable the group controls if there's no group
  // or only one group which can't be mixed
  if (groups.size() == 0 ||
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

void CGUIDialogSmartPlaylistEditor::OnWindowLoaded()
{
  CGUIDialog::OnWindowLoaded();

  SendMessage(GUI_MSG_SET_TYPE, CONTROL_NAME, 0, 16012);
  // setup the match spinner
  std::vector< std::pair<std::string, int> > labels;
  labels.push_back(make_pair(g_localizeStrings.Get(21425), 0));
  labels.push_back(make_pair(g_localizeStrings.Get(21426), 1));
  SET_CONTROL_LABELS(CONTROL_MATCH, m_playlist.m_ruleCombination.GetType() == CSmartPlaylistRuleCombination::CombinationAnd ? 0 : 1, &labels);

  // and now the limit spinner
  labels.clear();
  labels.push_back(make_pair(g_localizeStrings.Get(21428), 0));
  const int limits[] = { 10, 25, 50, 100, 250, 500, 1000 };
  for (unsigned int i = 0; i < sizeof(limits) / sizeof(int); i++)
    labels.push_back(make_pair(StringUtils::Format(g_localizeStrings.Get(21436).c_str(), limits[i]), limits[i]));
  SET_CONTROL_LABELS(CONTROL_LIMIT, 0, &labels);
}

void CGUIDialogSmartPlaylistEditor::OnInitWindow()
{
  m_cancelled = false;

  SendMessage(GUI_MSG_ITEM_SELECT, CONTROL_LIMIT, m_playlist.m_limit);

  vector<PLAYLIST_TYPE> allowedTypes;
  if (m_mode == "partymusic")
  {
    allowedTypes.push_back(TYPE_SONGS);
    allowedTypes.push_back(TYPE_MIXED);
  }
  else if (m_mode == "partyvideo")
  {
    allowedTypes.push_back(TYPE_MUSICVIDEOS);
    allowedTypes.push_back(TYPE_MIXED);
  }
  else if (m_mode == "music")
  { // music types + mixed
    allowedTypes.push_back(TYPE_SONGS);
    allowedTypes.push_back(TYPE_ALBUMS);
    allowedTypes.push_back(TYPE_ARTISTS);
    allowedTypes.push_back(TYPE_MIXED);
  }
  else if (m_mode == "video")
  { // general category for videos
    allowedTypes.push_back(TYPE_MOVIES);
    allowedTypes.push_back(TYPE_TVSHOWS);
    allowedTypes.push_back(TYPE_EPISODES);
    allowedTypes.push_back(TYPE_MUSICVIDEOS);
    allowedTypes.push_back(TYPE_MIXED);
  }
  // add to the spinner
  std::vector< std::pair<std::string, int> > labels;
  for (unsigned int i = 0; i < allowedTypes.size(); i++)
    labels.push_back(make_pair(GetLocalizedType(allowedTypes[i]), allowedTypes[i]));
  // check our playlist type is allowed
  PLAYLIST_TYPE type = ConvertType(m_playlist.GetType());
  bool allowed = false;
  for (unsigned int i = 0; i < allowedTypes.size(); i++)
    if (type == allowedTypes[i])
      allowed = true;
  if (!allowed && allowedTypes.size())
    type = allowedTypes[0];

  SET_CONTROL_LABELS(CONTROL_TYPE, type, &labels);
  m_playlist.SetType(ConvertType(type));
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
  for (unsigned int i = 0; i < NUM_TYPES; i++)
    if (type == types[i].string)
      return types[i].type;
  assert(false);
  return TYPE_SONGS;
}

std::string CGUIDialogSmartPlaylistEditor::GetLocalizedType(PLAYLIST_TYPE type)
{
  for (unsigned int i = 0; i < NUM_TYPES; i++)
    if (types[i].type == type)
      return g_localizeStrings.Get(types[i].localizedString);
  assert(false);
  return "";
}

std::string CGUIDialogSmartPlaylistEditor::ConvertType(PLAYLIST_TYPE type)
{
  for (unsigned int i = 0; i < NUM_TYPES; i++)
    if (types[i].type == type)
      return types[i].string;
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

void CGUIDialogSmartPlaylistEditor::OnRuleRemove(int item)
{
  if (item < 0 || item >= (int)m_playlist.m_ruleCombination.m_rules.size()) return;
  m_playlist.m_ruleCombination.m_rules.erase(m_playlist.m_ruleCombination.m_rules.begin() + item);

  UpdateButtons();
  if (item >= m_ruleLabels->Size())
    HighlightItem(m_ruleLabels->Size() - 1);
  else
    HighlightItem(item);
  if (m_ruleLabels->Size() <= 1)
  {
    SET_CONTROL_FOCUS(CONTROL_RULE_ADD, 0);
  }
}

void CGUIDialogSmartPlaylistEditor::OnRuleAdd()
{
  CSmartPlaylistRule rule;
  if (CGUIDialogSmartPlaylistRule::EditRule(rule,m_playlist.GetType()))
  {
    if (m_playlist.m_ruleCombination.m_rules.size() == 1 && m_playlist.m_ruleCombination.m_rules[0]->m_field == FieldNone)
      *m_playlist.m_ruleCombination.m_rules[0] = rule;
    else
      m_playlist.m_ruleCombination.AddRule(rule);
  }
  UpdateButtons();
}

bool CGUIDialogSmartPlaylistEditor::NewPlaylist(const std::string &type)
{
  CGUIDialogSmartPlaylistEditor *editor = (CGUIDialogSmartPlaylistEditor *)g_windowManager.GetWindow(WINDOW_DIALOG_SMART_PLAYLIST_EDITOR);
  if (!editor) return false;

  editor->m_path = "";
  editor->m_playlist = CSmartPlaylist();
  editor->m_mode = type;
  editor->Initialize();
  editor->DoModal(g_windowManager.GetActiveWindow());
  return !editor->m_cancelled;
}

bool CGUIDialogSmartPlaylistEditor::EditPlaylist(const std::string &path, const std::string &type)
{
  CGUIDialogSmartPlaylistEditor *editor = (CGUIDialogSmartPlaylistEditor *)g_windowManager.GetWindow(WINDOW_DIALOG_SMART_PLAYLIST_EDITOR);
  if (!editor) return false;

  editor->m_mode = type;
  if (URIUtils::PathEquals(path, CProfilesManager::Get().GetUserDataItem("PartyMode.xsp")))
    editor->m_mode = "partymusic";
  if (URIUtils::PathEquals(path, CProfilesManager::Get().GetUserDataItem("PartyMode-Video.xsp")))
    editor->m_mode = "partyvideo";

  CSmartPlaylist playlist;
  bool loaded(playlist.Load(path));
  if (!loaded)
  { // failed to load
    if (!StringUtils::StartsWithNoCase(editor->m_mode, "party"))
      return false; // only edit normal playlists that exist
    // party mode playlists can be editted even if they don't exist
    playlist.SetType(editor->m_mode == "partymusic" ? "songs" : "musicvideos");
  }

  editor->m_playlist = playlist;
  editor->m_path = path;
  editor->Initialize();
  editor->DoModal(g_windowManager.GetActiveWindow());
  return !editor->m_cancelled;
}
