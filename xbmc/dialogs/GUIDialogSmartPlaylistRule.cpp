/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogSmartPlaylistRule.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "GUIDialogFileBrowser.h"
#include "GUIDialogSelect.h"
#include "MediaSource.h"
#include "ServiceBroker.h"
#include "filesystem/Directory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIEditControl.h"
#include "guilib/GUIWindowManager.h"
#include "media/LibraryQueryDatabase.h"
#include "media/LibraryQueryReferences.h"
#include "media/LibraryQueryType.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "storage/MediaManager.h"
#include "utils/ContentNames.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

using enum CDatabaseQueryRule::FieldType;
using namespace KODI;

namespace LIBRARY_QUERY = KODI::MEDIA::LIBRARY_QUERY;

#define CONTROL_FIELD           15
#define CONTROL_OPERATOR        16
#define CONTROL_VALUE           17
#define CONTROL_OK              18
#define CONTROL_CANCEL          19
#define CONTROL_BROWSE          20

CGUIDialogSmartPlaylistRule::CGUIDialogSmartPlaylistRule(void)
    : CGUIDialog(WINDOW_DIALOG_SMART_PLAYLIST_RULE, "SmartPlaylistRule.xml")
{
  m_cancelled = false;
  m_loadType = KEEP_IN_MEMORY;
}

CGUIDialogSmartPlaylistRule::~CGUIDialogSmartPlaylistRule() = default;

bool CGUIDialogSmartPlaylistRule::OnBack(int actionID)
{
  m_cancelled = true;
  return CGUIDialog::OnBack(actionID);
}

bool CGUIDialogSmartPlaylistRule::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_OK)
        OnOK();
      else if (iControl == CONTROL_CANCEL)
        OnCancel();
      else if (iControl == CONTROL_VALUE)
      {
        std::string parameter;
        OnEditChanged(iControl, parameter);
        m_rule.SetParameter(parameter);
      }
      else if (iControl == CONTROL_OPERATOR)
        OnOperator();
      else if (iControl == CONTROL_FIELD)
        OnField();
      else if (iControl == CONTROL_BROWSE)
        OnBrowse();
      return true;
    }
    break;

  case GUI_MSG_VALIDITY_CHANGED:
    CONTROL_ENABLE_ON_CONDITION(CONTROL_OK, message.GetParam1());
    break;
  }
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogSmartPlaylistRule::OnOK()
{
  m_cancelled = false;
  Close();
}

namespace
{
using LIBRARY_QUERY::BROWSE;

//! The id of the string that heads the list of what is browsed for
int HeadingOf(BROWSE browse, MEDIA::TYPE itemType)
{
  switch (browse)
  {
    case BROWSE::GENRES:
      return 515;
    case BROWSE::SOURCES:
      return 39030;
    case BROWSE::ROLES:
      return 38033;
    case BROWSE::COUNTRIES:
      return 574;
    case BROWSE::ARTISTS:
    case BROWSE::ALBUM_ARTISTS:
      return 557;
    case BROWSE::ALBUMS:
      return 558;
    case BROWSE::ACTORS:
      return 20337;
    case BROWSE::YEARS:
      return 562;
    case BROWSE::ORIGINAL_YEARS:
      return 38078;
    case BROWSE::DIRECTORS:
      return 20339;
    case BROWSE::STUDIOS:
      return 572;
    case BROWSE::WRITERS:
      return 20417;
    case BROWSE::TVSHOWS:
      return 20343;
    case BROWSE::TITLES:
    {
      const std::optional<LIBRARY_QUERY::TYPE> type{LIBRARY_QUERY::TypeOf(itemType)};
      return type ? LIBRARY_QUERY::LabelOf(*type) : 0;
    }
    case BROWSE::SETS:
      return 20434;
    case BROWSE::TAGS:
      return 20459;
    case BROWSE::PLAYLISTS:
      return 559;
    default:
      return 0;
  }
}
} // unnamed namespace

void CGUIDialogSmartPlaylistRule::OnBrowse()
{
  auto& localizeStrings{CServiceBroker::GetResourcesComponent().GetLocalizeStrings()};
  const Field field{m_rule.m_field};
  const BROWSE browse{MEDIA::CLibraryQueryRule::BrowseOf(field)};
  const auto sources{LIBRARY_QUERY::SourcesOf(m_type)};

  if (browse == BROWSE::NOTHING)
  {
    return;
  }

  if (browse == BROWSE::PATHS)
  {
    std::vector<CMediaSource> mediaSources{LIBRARY_QUERY::PathSourcesOf(m_type)};
    CServiceBroker::GetMediaManager().GetLocalDrives(mediaSources);

    std::string path = m_rule.GetParameter();
    CGUIDialogFileBrowser::ShowAndGetDirectory(mediaSources, localizeStrings.Get(657), path, false);
    if (!m_rule.m_parameter.empty())
      m_rule.m_parameter.clear();
    if (!path.empty())
      m_rule.m_parameter.emplace_back(std::move(path));

    UpdateButtons();
    return;
  }

  CFileItemList items;
  int iLabel = HeadingOf(browse, MEDIA::TYPE::NONE);
  if (browse == BROWSE::PLAYLISTS)
  {
    // use filebrowser to grab another smart playlist
    for (const std::string& folder : MEDIA::PlaylistFoldersOf(m_type))
    {
      CFileItemList items2;
      XFILE::CDirectory::GetDirectory(folder, items2, ".xsp", XFILE::DIR_FLAG_NO_FILE_DIRS);
      items.Append(items2);
    }

    for (int i = 0; i < items.Size(); i++)
    {
      CFileItemPtr item = items[i];
      MEDIA::CLibraryQuery playlist;
      // don't list unloadable smartplaylists or any referenceable smartplaylists
      // which do not match the type of the current smartplaylist
      if (!playlist.Load(item->GetPath()) ||
          (field == Field::PLAYLIST &&
           (!LIBRARY_QUERY::CanReference(m_type, playlist.GetType()) ||
            (playlist.GetGroup() != LIBRARY_QUERY::GROUP::DEFAULT || playlist.IsGroupMixed()))))
      {
        items.Remove(i);
        i -= 1;
        continue;
      }

      if (!playlist.GetName().empty())
        item->SetLabel(playlist.GetName());
    }
  }
  else
  {
    // Each library the type lists from gives what it has
    for (const LIBRARY_QUERY::Source& source : sources)
    {
      CFileItemList values;
      LIBRARY_QUERY::BrowseValues(source, browse, "", nullptr, false, values);
      items.Append(values);
      iLabel = HeadingOf(browse, source.itemType);
    }
  }

  // sort the items
  items.Sort(SortBy::LABEL, SortOrder::ASCENDING,
             CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
                 CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING)
                 ? SortAttributeIgnoreArticle
                 : SortAttributeNone);

  CGUIDialogSelect* pDialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
  pDialog->Reset();
  pDialog->SetItems(items);
  std::string strHeading =
      StringUtils::Format(localizeStrings.Get(13401), localizeStrings.Get(iLabel));
  pDialog->SetHeading(CVariant{std::move(strHeading)});
  pDialog->SetMultiSelection(field != Field::PLAYLIST && field != Field::VIRTUAL_FOLDER);

  if (!m_rule.m_parameter.empty())
    pDialog->SetSelected(m_rule.m_parameter);

  pDialog->Open();
  if (pDialog->IsConfirmed())
  {
    m_rule.m_parameter.clear();
    for (int i : pDialog->GetSelectedItems())
      m_rule.m_parameter.push_back(items.Get(i)->GetLabel());

    UpdateButtons();
  }
  pDialog->Reset();
}

namespace
{
std::pair<std::string, CDatabaseQueryRule::SearchOperator> OperatorLabel(
    CDatabaseQueryRule::SearchOperator op)
{
  return std::make_pair(MEDIA::CLibraryQueryRule::GetLocalizedOperator(op), op);
}
} // unnamed namespace

std::vector<std::pair<std::string, CDatabaseQueryRule::SearchOperator>>
CGUIDialogSmartPlaylistRule::GetValidOperators(const MEDIA::CLibraryQueryRule& rule) const
{
  using enum CDatabaseQueryRule::SearchOperator;
  std::vector<std::pair<std::string, CDatabaseQueryRule::SearchOperator>> labels;
  if (MEDIA::CLibraryQueryRule::IsPlaylistField(rule.m_field))
  {
    labels.push_back(OperatorLabel(OPERATOR_EQUALS));
    labels.push_back(OperatorLabel(OPERATOR_DOES_NOT_EQUAL));
    return labels;
  }

  switch (rule.GetFieldType(rule.m_field))
  {
    case TEXT_FIELD:
      // text fields - add the usual comparisons
      labels.push_back(OperatorLabel(OPERATOR_EQUALS));
      labels.push_back(OperatorLabel(OPERATOR_DOES_NOT_EQUAL));
      labels.push_back(OperatorLabel(OPERATOR_CONTAINS));
      labels.push_back(OperatorLabel(OPERATOR_DOES_NOT_CONTAIN));
      labels.push_back(OperatorLabel(OPERATOR_STARTS_WITH));
      labels.push_back(OperatorLabel(OPERATOR_ENDS_WITH));
      break;

    case REAL_FIELD:
    case NUMERIC_FIELD:
    case SECONDS_FIELD:
      // numerical fields - less than greater than
      labels.push_back(OperatorLabel(OPERATOR_EQUALS));
      labels.push_back(OperatorLabel(OPERATOR_DOES_NOT_EQUAL));
      labels.push_back(OperatorLabel(OPERATOR_GREATER_THAN));
      labels.push_back(OperatorLabel(OPERATOR_LESS_THAN));
      break;

    case DATE_FIELD:
      // date field
      labels.push_back(OperatorLabel(OPERATOR_AFTER));
      labels.push_back(OperatorLabel(OPERATOR_BEFORE));
      labels.push_back(OperatorLabel(OPERATOR_IN_THE_LAST));
      labels.push_back(OperatorLabel(OPERATOR_NOT_IN_THE_LAST));
      break;

    case TEXTIN_FIELD:
      labels.push_back(OperatorLabel(OPERATOR_EQUALS));
      labels.push_back(OperatorLabel(OPERATOR_DOES_NOT_EQUAL));
      break;

    case BOOLEAN_FIELD:
      labels.push_back(OperatorLabel(OPERATOR_TRUE));
      labels.push_back(OperatorLabel(OPERATOR_FALSE));
      break;
  }
  return labels;
}

void CGUIDialogSmartPlaylistRule::OnCancel()
{
  m_cancelled = true;
  Close();
}

void CGUIDialogSmartPlaylistRule::OnField()
{
  const auto fields = MEDIA::CLibraryQueryRule::GetFields(m_type);
  CGUIDialogSelect* dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
  dialog->Reset();
  dialog->SetHeading(CVariant{20427});
  int selected = -1;
  for (auto field = fields.begin(); field != fields.end(); field++)
  {
    dialog->Add(MEDIA::CLibraryQueryRule::GetLocalizedField(*field));
    if (*field == m_rule.m_field)
      selected = std::distance(fields.begin(), field);
  }
  if (selected > -1)
    dialog->SetSelected(selected);
  dialog->Open();
  int newSelected = dialog->GetSelectedItem();
  // check if selection has changed
  if (!dialog->IsConfirmed() || newSelected < 0 || newSelected == selected)
    return;

  m_rule.m_field = fields[newSelected];
  // check if operator is still valid. if not, reset to first valid one
  const std::vector<std::pair<std::string, CDatabaseQueryRule::SearchOperator>> validOperators =
      GetValidOperators(m_rule);
  bool isValid = false;
  for (auto op : validOperators)
    if (std::get<0>(op) == std::get<0>(OperatorLabel(m_rule.m_operator)))
      isValid = true;
  if (!isValid)
    m_rule.m_operator = std::get<1>(validOperators[0]);

  m_rule.SetParameter("");
  UpdateButtons();
}

void CGUIDialogSmartPlaylistRule::OnOperator()
{
  const auto labels = GetValidOperators(m_rule);
  CGUIDialogSelect* dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
  dialog->Reset();
  dialog->SetHeading(CVariant{ 16023 });
  for (auto label : labels)
    dialog->Add(std::get<0>(label));
  dialog->SetSelected(MEDIA::CLibraryQueryRule::GetLocalizedOperator(m_rule.m_operator));
  dialog->Open();
  int newSelected = dialog->GetSelectedItem();
  // check if selection has changed
  if (!dialog->IsConfirmed() || newSelected < 0)
    return;

  m_rule.m_operator = std::get<1>(labels[newSelected]);
  UpdateButtons();
}

void CGUIDialogSmartPlaylistRule::UpdateButtons()
{
  if (m_rule.m_field == Field::NONE)
    m_rule.m_field = MEDIA::CLibraryQueryRule::GetFields(m_type)[0];
  SET_CONTROL_LABEL(CONTROL_FIELD, MEDIA::CLibraryQueryRule::GetLocalizedField(m_rule.m_field));

  const CDatabaseQueryRule::FieldType fieldType = m_rule.GetFieldType(m_rule.m_field);

  CONTROL_ENABLE_ON_CONDITION(CONTROL_VALUE, fieldType != BOOLEAN_FIELD);
  CONTROL_ENABLE_ON_CONDITION(CONTROL_BROWSE,
                              MEDIA::CLibraryQueryRule::IsFieldBrowseable(m_rule.m_field));

  SET_CONTROL_LABEL(CONTROL_OPERATOR, std::get<0>(OperatorLabel(m_rule.m_operator)));

  // update label2 appropriately
  SET_CONTROL_LABEL2(CONTROL_VALUE, m_rule.GetParameter());
  CGUIEditControl::INPUT_TYPE type = CGUIEditControl::INPUT_TYPE_TEXT;

  switch (fieldType)
  {
    case TEXT_FIELD:
    case TEXTIN_FIELD:
    case REAL_FIELD:
    case NUMERIC_FIELD:
      type = CGUIEditControl::INPUT_TYPE_TEXT;
      break;
    case DATE_FIELD:
      if (m_rule.m_operator == CDatabaseQueryRule::SearchOperator::OPERATOR_IN_THE_LAST ||
          m_rule.m_operator == CDatabaseQueryRule::SearchOperator::OPERATOR_NOT_IN_THE_LAST)
        type = CGUIEditControl::INPUT_TYPE_TEXT;
      else
        type = CGUIEditControl::INPUT_TYPE_DATE;
      break;
    case SECONDS_FIELD:
      type = CGUIEditControl::INPUT_TYPE_SECONDS;
      break;
    case BOOLEAN_FIELD:
      type = CGUIEditControl::INPUT_TYPE_NUMBER;
      break;
  }
  SendMessage(GUI_MSG_SET_TYPE, CONTROL_VALUE, type, 21420);
}

void CGUIDialogSmartPlaylistRule::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  UpdateButtons();

  CGUIEditControl *editControl = dynamic_cast<CGUIEditControl*>(GetControl(CONTROL_VALUE));
  if (editControl != NULL)
    editControl->SetInputValidation(MEDIA::CLibraryQueryRule::Validate, &m_rule);
}

void CGUIDialogSmartPlaylistRule::OnDeinitWindow(int nextWindowID)
{
  CGUIDialog::OnDeinitWindow(nextWindowID);

  // reset field spincontrolex
  SendMessage(GUI_MSG_LABEL_RESET, CONTROL_FIELD);
  // reset operator spincontrolex
  SendMessage(GUI_MSG_LABEL_RESET, CONTROL_OPERATOR);
}

bool CGUIDialogSmartPlaylistRule::EditRule(MEDIA::CLibraryQueryRule& rule, LIBRARY_QUERY::TYPE type)
{
  CGUIDialogSmartPlaylistRule *editor = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSmartPlaylistRule>(WINDOW_DIALOG_SMART_PLAYLIST_RULE);
  if (!editor) return false;

  editor->m_rule = rule;
  editor->m_type = type;
  editor->Open();
  rule = editor->m_rule;
  return !editor->m_cancelled;
}

