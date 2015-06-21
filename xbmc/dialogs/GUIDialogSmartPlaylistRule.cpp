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

#include "GUIDialogSmartPlaylistRule.h"
#include "GUIDialogFileBrowser.h"
#include "music/MusicDatabase.h"
#include "video/VideoDatabase.h"
#include "guilib/GUIWindowManager.h"
#include "GUIDialogSelect.h"
#include "filesystem/Directory.h"
#include "FileItem.h"
#include "guilib/GUIEditControl.h"
#include "guilib/LocalizeStrings.h"
#include "settings/MediaSourceSettings.h"
#include "storage/MediaManager.h"
#include "utils/LabelFormatter.h"
#include "utils/StringUtils.h"
#include "settings/Settings.h"

#define CONTROL_FIELD           15
#define CONTROL_OPERATOR        16
#define CONTROL_VALUE           17
#define CONTROL_OK              18
#define CONTROL_CANCEL          19
#define CONTROL_BROWSE          20

using namespace std;

CGUIDialogSmartPlaylistRule::CGUIDialogSmartPlaylistRule(void)
    : CGUIDialog(WINDOW_DIALOG_SMART_PLAYLIST_RULE, "SmartPlaylistRule.xml")
{
  m_cancelled = false;
  m_loadType = KEEP_IN_MEMORY;
}

CGUIDialogSmartPlaylistRule::~CGUIDialogSmartPlaylistRule()
{
}

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

void CGUIDialogSmartPlaylistRule::OnBrowse()
{
  CFileItemList items;
  CMusicDatabase database;
  database.Open();
  CVideoDatabase videodatabase;
  videodatabase.Open();

  std::string basePath;
  if (CSmartPlaylist::IsMusicType(m_type))
    basePath = "musicdb://";
  else
    basePath = "videodb://";

  VIDEODB_CONTENT_TYPE type = VIDEODB_CONTENT_MOVIES;
  if (m_type == "movies")
    basePath += "movies/";
  else if (m_type == "tvshows")
  {
    type = VIDEODB_CONTENT_TVSHOWS;
    basePath += "tvshows/";
  }
  else if (m_type == "musicvideos")
  {
    type = VIDEODB_CONTENT_MUSICVIDEOS;
    basePath += "musicvideos/";
  }
  else if (m_type == "episodes")
  {
    if (m_rule.m_field == FieldGenre || m_rule.m_field == FieldYear ||
        m_rule.m_field == FieldStudio)
      type = VIDEODB_CONTENT_TVSHOWS;
    else
      type = VIDEODB_CONTENT_EPISODES;
    basePath += "tvshows/";
  }

  int iLabel = 0;
  if (m_rule.m_field == FieldGenre)
  {
    if (m_type == "tvshows" ||
        m_type == "episodes" ||
        m_type == "movies")
      videodatabase.GetGenresNav(basePath + "genres/", items, type);
    else if (m_type == "songs" ||
             m_type == "albums" ||
             m_type == "artists" ||
             m_type == "mixed")
      database.GetGenresNav("musicdb://genres/",items);
    if (m_type == "musicvideos" ||
        m_type == "mixed")
    {
      CFileItemList items2;
      videodatabase.GetGenresNav("videodb://musicvideos/genres/",items2,VIDEODB_CONTENT_MUSICVIDEOS);
      items.Append(items2);
    }
    iLabel = 515;
  }
  else if (m_rule.m_field == FieldCountry)
  {
    videodatabase.GetCountriesNav(basePath, items, type);
    iLabel = 574;
  }
  else if (m_rule.m_field == FieldArtist || m_rule.m_field == FieldAlbumArtist)
  {
    if (CSmartPlaylist::IsMusicType(m_type))
      database.GetArtistsNav("musicdb://artists/", items, m_rule.m_field == FieldAlbumArtist, -1);
    if (m_type == "musicvideos" ||
        m_type == "mixed")
    {
      CFileItemList items2;
      videodatabase.GetMusicVideoArtistsByName("", items2);
      items.Append(items2);
    }
    iLabel = 557;
  }
  else if (m_rule.m_field == FieldAlbum)
  {
    if (CSmartPlaylist::IsMusicType(m_type))
      database.GetAlbumsNav("musicdb://albums/", items);
    if (m_type == "musicvideos" ||
        m_type == "mixed")
    {
      CFileItemList items2;
      videodatabase.GetMusicVideoAlbumsByName("", items2);
      items.Append(items2);
    }
    iLabel = 558;
  }
  else if (m_rule.m_field == FieldActor)
  {
    videodatabase.GetActorsNav(basePath + "actors/",items,type);
    iLabel = 20337;
  }
  else if (m_rule.m_field == FieldYear)
  {
    if (CSmartPlaylist::IsMusicType(m_type))
      database.GetYearsNav("musicdb://years/", items);
    if (CSmartPlaylist::IsVideoType(m_type))
    {
      CFileItemList items2;
      videodatabase.GetYearsNav(basePath + "years/", items2, type);
      items.Append(items2);
    }
    iLabel = 562;
  }
  else if (m_rule.m_field == FieldDirector)
  {
    videodatabase.GetDirectorsNav(basePath + "directors/", items, type);
    iLabel = 20339;
  }
  else if (m_rule.m_field == FieldStudio)
  {
    videodatabase.GetStudiosNav(basePath + "studios/", items, type);
    iLabel = 572;
  }
  else if (m_rule.m_field == FieldWriter)
  {
    videodatabase.GetWritersNav(basePath, items, type);
    iLabel = 20417;
  }
  else if (m_rule.m_field == FieldTvShowTitle ||
          (m_type == "tvshows" && m_rule.m_field == FieldTitle))
  {
    videodatabase.GetTvShowsNav(basePath + "titles/", items);
    iLabel = 20343;
  }
  else if (m_rule.m_field == FieldTitle)
  {
    if (m_type == "songs" || m_type == "mixed")
    {
      database.GetSongsNav("musicdb://songs/", items, -1, -1, -1);
      iLabel = 134;
    }
    if (m_type == "movies")
    {
      videodatabase.GetMoviesNav(basePath + "titles/", items);
      iLabel = 20342;
    }
    if (m_type == "episodes")
    {
      videodatabase.GetEpisodesNav(basePath + "titles/-1/-1/", items);
      // we need to replace the db label (<season>x<episode> <title>) with the title only
      CLabelFormatter format("%T", "");
      for (int i = 0; i < items.Size(); i++)
        format.FormatLabel(items[i].get());
      iLabel = 20360;
    }
    if (m_type == "musicvideos" || m_type == "mixed")
    {
      videodatabase.GetMusicVideosNav(basePath + "titles/", items);
      iLabel = 20389;
    }
  }
  else if (m_rule.m_field == FieldPlaylist || m_rule.m_field == FieldVirtualFolder)
  {
    // use filebrowser to grab another smart playlist

    // Note: This can cause infinite loops (playlist that refers to the same playlist) but I don't
    //       think there's any decent way to deal with this, as the infinite loop may be an arbitrary
    //       number of playlists deep, eg playlist1 -> playlist2 -> playlist3 ... -> playlistn -> playlist1
    if (CSmartPlaylist::IsVideoType(m_type))
      XFILE::CDirectory::GetDirectory("special://videoplaylists/", items, ".xsp", XFILE::DIR_FLAG_NO_FILE_DIRS);
    if (CSmartPlaylist::IsMusicType(m_type))
    {
      CFileItemList items2;
      XFILE::CDirectory::GetDirectory("special://musicplaylists/", items2, ".xsp", XFILE::DIR_FLAG_NO_FILE_DIRS);
      items.Append(items2);
    }

    for (int i = 0; i < items.Size(); i++)
    {
      CFileItemPtr item = items[i];
      CSmartPlaylist playlist;
      // don't list unloadable smartplaylists or any referencable smartplaylists
      // which do not match the type of the current smartplaylist
      if (!playlist.Load(item->GetPath()) ||
         (m_rule.m_field == FieldPlaylist &&
         (!CSmartPlaylist::CheckTypeCompatibility(m_type, playlist.GetType()) ||
         (!playlist.GetGroup().empty() || playlist.IsGroupMixed()))))
      {
        items.Remove(i);
        i -= 1;
        continue;
      }

      if (!playlist.GetName().empty())
        item->SetLabel(playlist.GetName());
    }
    iLabel = 559;
  }
  else if (m_rule.m_field == FieldPath)
  {
    VECSOURCES sources;
    if (m_type == "songs" || m_type == "mixed")
      sources = *CMediaSourceSettings::Get().GetSources("music");
    if (CSmartPlaylist::IsVideoType(m_type))
    {
      VECSOURCES sources2 = *CMediaSourceSettings::Get().GetSources("video");
      sources.insert(sources.end(),sources2.begin(),sources2.end());
    }
    g_mediaManager.GetLocalDrives(sources);
    
    std::string path = m_rule.GetParameter();
    CGUIDialogFileBrowser::ShowAndGetDirectory(sources, g_localizeStrings.Get(657), path, false);
    if (m_rule.m_parameter.size() > 0)
      m_rule.m_parameter.clear();
    if (!path.empty())
      m_rule.m_parameter.push_back(path);

    UpdateButtons();
    return;
  }
  else if (m_rule.m_field == FieldSet)
  {
    videodatabase.GetSetsNav("videodb://movies/sets/", items, VIDEODB_CONTENT_MOVIES);
    iLabel = 20434;
  }
  else if (m_rule.m_field == FieldTag)
  {
    VIDEODB_CONTENT_TYPE type = VIDEODB_CONTENT_MOVIES;
    if (m_type == "tvshows" ||
        m_type == "episodes")
      type = VIDEODB_CONTENT_TVSHOWS;
    else if (m_type == "musicvideos")
      type = VIDEODB_CONTENT_MUSICVIDEOS;
    else if (m_type != "movies")
      return;

    videodatabase.GetTagsNav(basePath + "tags/", items, type);
    iLabel = 20459;
  }
  else
  { // TODO: Add browseability in here.
    assert(false);
  }

  // sort the items
  items.Sort(SortByLabel, SortOrderAscending, CSettings::Get().GetBool("filelists.ignorethewhensorting") ? SortAttributeIgnoreArticle : SortAttributeNone);

  CGUIDialogSelect* pDialog = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
  pDialog->Reset();
  pDialog->SetItems(&items);
  std::string strHeading = StringUtils::Format(g_localizeStrings.Get(13401).c_str(), g_localizeStrings.Get(iLabel).c_str());
  pDialog->SetHeading(strHeading);
  pDialog->SetMultiSelection(m_rule.m_field != FieldPlaylist && m_rule.m_field != FieldVirtualFolder);

  if (!m_rule.m_parameter.empty())
    pDialog->SetSelected(m_rule.m_parameter);

  pDialog->DoModal();
  if (pDialog->IsConfirmed())
  {
    const CFileItemList &items = pDialog->GetSelectedItems();
    m_rule.m_parameter.clear();
    for (int index = 0; index < items.Size(); index++)
      m_rule.m_parameter.push_back(items[index]->GetLabel());

    UpdateButtons();
  }
  pDialog->Reset();
}

void CGUIDialogSmartPlaylistRule::OnCancel()
{
  m_cancelled = true;
  Close();
}

void CGUIDialogSmartPlaylistRule::OnField()
{
  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_FIELD);
  OnMessage(msg);
  m_rule.m_field = (Field)msg.GetParam1();

  UpdateButtons();
}

void CGUIDialogSmartPlaylistRule::OnOperator()
{
  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_OPERATOR);
  OnMessage(msg);
  m_rule.m_operator = (CDatabaseQueryRule::SEARCH_OPERATOR)msg.GetParam1();

  UpdateButtons();
}

std::pair<std::string, int> OperatorLabel(CDatabaseQueryRule::SEARCH_OPERATOR op)
{
  return make_pair(CSmartPlaylistRule::GetLocalizedOperator(op), op);
}

void CGUIDialogSmartPlaylistRule::UpdateButtons()
{
  // update the field control
  SendMessage(GUI_MSG_ITEM_SELECT, CONTROL_FIELD, m_rule.m_field);
  CGUIMessage msg2(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_FIELD);
  OnMessage(msg2);
  m_rule.m_field = (Field)msg2.GetParam1();

  // and now update the operator set
  SendMessage(GUI_MSG_LABEL_RESET, CONTROL_OPERATOR);

  CONTROL_ENABLE(CONTROL_VALUE);
  if (CSmartPlaylistRule::IsFieldBrowseable(m_rule.m_field))
    CONTROL_ENABLE(CONTROL_BROWSE);
  else
    CONTROL_DISABLE(CONTROL_BROWSE);

  std::vector< std::pair<std::string, int> > labels;
  switch (m_rule.GetFieldType(m_rule.m_field))
  {
  case CDatabaseQueryRule::TEXT_FIELD:
    // text fields - add the usual comparisons
    labels.push_back(OperatorLabel(CDatabaseQueryRule::OPERATOR_EQUALS));
    labels.push_back(OperatorLabel(CDatabaseQueryRule::OPERATOR_DOES_NOT_EQUAL));
    labels.push_back(OperatorLabel(CDatabaseQueryRule::OPERATOR_CONTAINS));
    labels.push_back(OperatorLabel(CDatabaseQueryRule::OPERATOR_DOES_NOT_CONTAIN));
    labels.push_back(OperatorLabel(CDatabaseQueryRule::OPERATOR_STARTS_WITH));
    labels.push_back(OperatorLabel(CDatabaseQueryRule::OPERATOR_ENDS_WITH));
    break;

  case CDatabaseQueryRule::NUMERIC_FIELD:
  case CDatabaseQueryRule::SECONDS_FIELD:
    // numerical fields - less than greater than
    labels.push_back(OperatorLabel(CDatabaseQueryRule::OPERATOR_EQUALS));
    labels.push_back(OperatorLabel(CDatabaseQueryRule::OPERATOR_DOES_NOT_EQUAL));
    labels.push_back(OperatorLabel(CDatabaseQueryRule::OPERATOR_GREATER_THAN));
    labels.push_back(OperatorLabel(CDatabaseQueryRule::OPERATOR_LESS_THAN));
    break;

  case CDatabaseQueryRule::DATE_FIELD:
    // date field
    labels.push_back(OperatorLabel(CDatabaseQueryRule::OPERATOR_AFTER));
    labels.push_back(OperatorLabel(CDatabaseQueryRule::OPERATOR_BEFORE));
    labels.push_back(OperatorLabel(CDatabaseQueryRule::OPERATOR_IN_THE_LAST));
    labels.push_back(OperatorLabel(CDatabaseQueryRule::OPERATOR_NOT_IN_THE_LAST));
    break;

  case CDatabaseQueryRule::PLAYLIST_FIELD:
    CONTROL_ENABLE(CONTROL_BROWSE);
    labels.push_back(OperatorLabel(CDatabaseQueryRule::OPERATOR_EQUALS));
    labels.push_back(OperatorLabel(CDatabaseQueryRule::OPERATOR_DOES_NOT_EQUAL));
    break;

  case CDatabaseQueryRule::BOOLEAN_FIELD:
    CONTROL_DISABLE(CONTROL_VALUE);
    labels.push_back(OperatorLabel(CDatabaseQueryRule::OPERATOR_TRUE));
    labels.push_back(OperatorLabel(CDatabaseQueryRule::OPERATOR_FALSE));
    break;

  case CDatabaseQueryRule::TEXTIN_FIELD:
    labels.push_back(OperatorLabel(CDatabaseQueryRule::OPERATOR_EQUALS));
    labels.push_back(OperatorLabel(CDatabaseQueryRule::OPERATOR_DOES_NOT_EQUAL));
    break;
  }

  SET_CONTROL_LABELS(CONTROL_OPERATOR, m_rule.m_operator, &labels);

  // check our operator is valid, and update if not
  CGUIMessage selected(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_OPERATOR);
  OnMessage(selected);
  m_rule.m_operator = (CDatabaseQueryRule::SEARCH_OPERATOR)selected.GetParam1();

  // update the parameter edit control appropriately
  SET_CONTROL_LABEL2(CONTROL_VALUE, m_rule.GetParameter());
  CGUIEditControl::INPUT_TYPE type = CGUIEditControl::INPUT_TYPE_TEXT;
  CDatabaseQueryRule::FIELD_TYPE fieldType = m_rule.GetFieldType(m_rule.m_field);
  switch (fieldType)
  {
  case CDatabaseQueryRule::TEXT_FIELD:
  case CDatabaseQueryRule::PLAYLIST_FIELD:
  case CDatabaseQueryRule::TEXTIN_FIELD:
  case CDatabaseQueryRule::NUMERIC_FIELD:
    type = CGUIEditControl::INPUT_TYPE_TEXT;
    break;
  case CDatabaseQueryRule::DATE_FIELD:
    if (m_rule.m_operator == CDatabaseQueryRule::OPERATOR_IN_THE_LAST ||
        m_rule.m_operator == CDatabaseQueryRule::OPERATOR_NOT_IN_THE_LAST)
      type = CGUIEditControl::INPUT_TYPE_TEXT;
    else
      type = CGUIEditControl::INPUT_TYPE_DATE;
    break;
  case CDatabaseQueryRule::SECONDS_FIELD:
    type = CGUIEditControl::INPUT_TYPE_SECONDS;
    break;
  case CDatabaseQueryRule::BOOLEAN_FIELD:
    type = CGUIEditControl::INPUT_TYPE_NUMBER;
    break;
  }
  SendMessage(GUI_MSG_SET_TYPE, CONTROL_VALUE, type, 21420);
}

void CGUIDialogSmartPlaylistRule::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  // add the fields to the field spincontrol
  std::vector< std::pair<std::string, int> > labels;
  vector<Field> fields = CSmartPlaylistRule::GetFields(m_type);
  for (unsigned int i = 0; i < fields.size(); i++)
    labels.push_back(make_pair(CSmartPlaylistRule::GetLocalizedField(fields[i]), fields[i]));

  SET_CONTROL_LABELS(CONTROL_FIELD, 0, &labels);

  UpdateButtons();

  CGUIEditControl *editControl = dynamic_cast<CGUIEditControl*>(GetControl(CONTROL_VALUE));
  if (editControl != NULL)
    editControl->SetInputValidation(CSmartPlaylistRule::Validate, &m_rule);
}

void CGUIDialogSmartPlaylistRule::OnDeinitWindow(int nextWindowID)
{
  CGUIDialog::OnDeinitWindow(nextWindowID);

  // reset field spincontrolex
  SendMessage(GUI_MSG_LABEL_RESET, CONTROL_FIELD);
  // reset operator spincontrolex
  SendMessage(GUI_MSG_LABEL_RESET, CONTROL_OPERATOR);
}

bool CGUIDialogSmartPlaylistRule::EditRule(CSmartPlaylistRule &rule, const std::string& type)
{
  CGUIDialogSmartPlaylistRule *editor = (CGUIDialogSmartPlaylistRule *)g_windowManager.GetWindow(WINDOW_DIALOG_SMART_PLAYLIST_RULE);
  if (!editor) return false;

  editor->m_rule = rule;
  editor->m_type = type;
  editor->DoModal(g_windowManager.GetActiveWindow());
  rule = editor->m_rule;
  return !editor->m_cancelled;
}

