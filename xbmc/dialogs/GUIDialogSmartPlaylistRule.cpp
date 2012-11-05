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
#include "settings/Settings.h"
#include "storage/MediaManager.h"

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
        CStdString parameter;
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
  if (m_type.Equals("songs") || m_type.Equals("albums") || m_type.Equals("artists") || m_type.Equals("mixed"))
    basePath = "musicdb://";
  else
    basePath = "videodb://";

  VIDEODB_CONTENT_TYPE type = VIDEODB_CONTENT_MOVIES;
  if (m_type.Equals("movies"))
    basePath += "1/";
  else if (m_type.Equals("tvshows"))
  {
    type = VIDEODB_CONTENT_TVSHOWS;
    basePath += "2/";
  }
  else if (m_type.Equals("musicvideos"))
  {
    type = VIDEODB_CONTENT_MUSICVIDEOS;
    basePath += "3/";
  }
  else if (m_type.Equals("episodes"))
  {
    if (m_rule.m_field == FieldGenre || m_rule.m_field == FieldYear ||
        m_rule.m_field == FieldStudio)
      type = VIDEODB_CONTENT_TVSHOWS;
    else
      type = VIDEODB_CONTENT_EPISODES;
    basePath += "2/";
  }

  int iLabel = 0;
  if (m_rule.m_field == FieldGenre)
  {
    if (m_type.Equals("tvshows") || m_type.Equals("episodes") || m_type.Equals("movies"))
      videodatabase.GetGenresNav(basePath + "1/", items, type);
    else if (m_type.Equals("songs") || m_type.Equals("albums") || m_type.Equals("artists") || m_type.Equals("mixed"))
      database.GetGenresNav("musicdb://1/",items);
    if (m_type.Equals("musicvideos") || m_type.Equals("mixed"))
    {
      CFileItemList items2;
      videodatabase.GetGenresNav("videodb://3/1/",items2,VIDEODB_CONTENT_MUSICVIDEOS);
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
    if (m_type.Equals("songs") || m_type.Equals("mixed") || m_type.Equals("albums") || m_type.Equals("artists"))
      database.GetArtistsNav("musicdb://2/", items, m_rule.m_field == FieldAlbumArtist, -1);
    if (m_type.Equals("musicvideos") || m_type.Equals("mixed"))
    {
      CFileItemList items2;
      videodatabase.GetMusicVideoArtistsByName("", items2);
      items.Append(items2);
    }
    iLabel = 557;
  }
  else if (m_rule.m_field == FieldAlbum)
  {
    if (m_type.Equals("songs") || m_type.Equals("mixed") || m_type.Equals("albums"))
      database.GetAlbumsNav("musicdb://3/", items);
    if (m_type.Equals("musicvideos") || m_type.Equals("mixed"))
    {
      CFileItemList items2;
      videodatabase.GetMusicVideoAlbumsByName("", items2);
      items.Append(items2);
    }
    iLabel = 558;
  }
  else if (m_rule.m_field == FieldActor)
  {
    videodatabase.GetActorsNav(basePath + "4/",items,type);
    iLabel = 20337;
  }
  else if (m_rule.m_field == FieldYear)
  {
    if (m_type.Equals("songs") || m_type.Equals("mixed") || m_type.Equals("albums"))
      database.GetYearsNav("musicdb://9/", items);
    if (!m_type.Equals("songs") && !m_type.Equals("albums"))
    {
      CFileItemList items2;
      videodatabase.GetYearsNav(basePath + "3/", items2, type);
      items.Append(items2);
    }
    iLabel = 562;
  }
  else if (m_rule.m_field == FieldDirector)
  {
    videodatabase.GetDirectorsNav(basePath + "5/", items, type);
    iLabel = 20339;
  }
  else if (m_rule.m_field == FieldStudio)
  {
    if (m_type.Equals("movies"))
      basePath += "6/";
    else if (m_type.Equals("musicvideos") || m_type.Equals("mixed"))
      basePath += "7/";
    else
      basePath += "5/";

    videodatabase.GetStudiosNav(basePath, items, type);
    iLabel = 572;
  }
  else if (m_rule.m_field == FieldWriter)
  {
    videodatabase.GetWritersNav(basePath, items, type);
    iLabel = 20417;
  }
  else if (m_rule.m_field == FieldTvShowTitle)
  {
    videodatabase.GetTvShowsNav("videodb://2/2/",items);
    iLabel = 20343;
  }
  else if (m_rule.m_field == FieldPlaylist)
  {
    // use filebrowser to grab another smart playlist

    // Note: This can cause infinite loops (playlist that refers to the same playlist) but I don't
    //       think there's any decent way to deal with this, as the infinite loop may be an arbitrary
    //       number of playlists deep, eg playlist1 -> playlist2 -> playlist3 ... -> playlistn -> playlist1
    CStdString path = "special://videoplaylists/";
    if (m_type.Equals("songs") || m_type.Equals("albums"))
      path = "special://musicplaylists/";
    XFILE::CDirectory::GetDirectory(path, items, ".xsp", XFILE::DIR_FLAG_NO_FILE_DIRS);
    for (int i = 0; i < items.Size(); i++)
    {
      CFileItemPtr item = items[i];
      CSmartPlaylist playlist;
      if (playlist.OpenAndReadName(item->GetPath()))
        item->SetLabel(playlist.GetName());
    }
    iLabel = 559;
  }
  else if (m_rule.m_field == FieldPath)
  {
    VECSOURCES sources;
    if (m_type == "songs" || m_type == "mixed")
      sources = *g_settings.GetSourcesFromType("music");
    if (m_type != "songs")
    {
      VECSOURCES sources2 = *g_settings.GetSourcesFromType("video");
      sources.insert(sources.end(),sources2.begin(),sources2.end());
    }
    g_mediaManager.GetLocalDrives(sources);
    
    CStdString path = m_rule.GetParameter();
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
    videodatabase.GetSetsNav("videodb://1/7/", items, VIDEODB_CONTENT_MOVIES);
    iLabel = 20434;
  }
  else if (m_rule.m_field == FieldTag)
  {
    if (m_type == "movies")
      videodatabase.GetTagsNav(basePath + "9/", items, VIDEODB_CONTENT_MOVIES);
    else
      return;
    iLabel = 20459;
  }
  else
  { // TODO: Add browseability in here.
    assert(false);
  }

  // sort the items
  items.Sort(SORT_METHOD_LABEL, SortOrderAscending);

  CGUIDialogSelect* pDialog = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
  pDialog->Reset();
  pDialog->SetItems(&items);
  CStdString strHeading;
  strHeading.Format(g_localizeStrings.Get(13401),g_localizeStrings.Get(iLabel));
  pDialog->SetHeading(strHeading);
  pDialog->SetMultiSelection(true);

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
  m_rule.m_operator = (CSmartPlaylistRule::SEARCH_OPERATOR)msg.GetParam1();

  UpdateButtons();
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
  CONTROL_DISABLE(CONTROL_BROWSE);
  switch (CSmartPlaylistRule::GetFieldType(m_rule.m_field))
  {
  case CSmartPlaylistRule::BROWSEABLE_FIELD:
    CONTROL_ENABLE(CONTROL_BROWSE);
    // fall through...
  case CSmartPlaylistRule::TEXT_FIELD:
    // text fields - add the usual comparisons
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_EQUALS);
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_DOES_NOT_EQUAL);
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_CONTAINS);
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_DOES_NOT_CONTAIN);
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_STARTS_WITH);
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_ENDS_WITH);
    break;

  case CSmartPlaylistRule::BROWSEABLE_NUMERIC_FIELD:
      CONTROL_ENABLE(CONTROL_BROWSE);
      // fall through...
  case CSmartPlaylistRule::NUMERIC_FIELD:
  case CSmartPlaylistRule::SECONDS_FIELD:
    // numerical fields - less than greater than
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_EQUALS);
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_DOES_NOT_EQUAL);
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_GREATER_THAN);
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_LESS_THAN);
    break;

  case CSmartPlaylistRule::DATE_FIELD:
    // date field
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_AFTER);
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_BEFORE);
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_IN_THE_LAST);
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_NOT_IN_THE_LAST);
    break;

  case CSmartPlaylistRule::PLAYLIST_FIELD:
    CONTROL_ENABLE(CONTROL_BROWSE);
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_EQUALS);
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_DOES_NOT_EQUAL);
    break;

  case CSmartPlaylistRule::BOOLEAN_FIELD:
    CONTROL_DISABLE(CONTROL_VALUE);
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_TRUE);
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_FALSE);
    break;

  case CSmartPlaylistRule::TEXTIN_FIELD:
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_EQUALS);
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_DOES_NOT_EQUAL);
    break;
  }

  // check our operator is valid, and update if not
  SendMessage(GUI_MSG_ITEM_SELECT, CONTROL_OPERATOR, m_rule.m_operator);
  CGUIMessage selected(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_OPERATOR);
  OnMessage(selected);
  m_rule.m_operator = (CSmartPlaylistRule::SEARCH_OPERATOR)selected.GetParam1();

  // update the parameter edit control appropriately
  SET_CONTROL_LABEL2(CONTROL_VALUE, m_rule.GetParameter());
  CGUIEditControl::INPUT_TYPE type = CGUIEditControl::INPUT_TYPE_TEXT;
  CSmartPlaylistRule::FIELD_TYPE fieldType = CSmartPlaylistRule::GetFieldType(m_rule.m_field);
  switch (fieldType)
  {
  case CSmartPlaylistRule::BROWSEABLE_FIELD:
  case CSmartPlaylistRule::BROWSEABLE_NUMERIC_FIELD:
  case CSmartPlaylistRule::TEXT_FIELD:
  case CSmartPlaylistRule::PLAYLIST_FIELD:
  case CSmartPlaylistRule::TEXTIN_FIELD:
  case CSmartPlaylistRule::NUMERIC_FIELD:
    type = CGUIEditControl::INPUT_TYPE_TEXT;
    break;
  case CSmartPlaylistRule::DATE_FIELD:
    if (m_rule.m_operator == CSmartPlaylistRule::OPERATOR_IN_THE_LAST ||
        m_rule.m_operator == CSmartPlaylistRule::OPERATOR_NOT_IN_THE_LAST)
      type = CGUIEditControl::INPUT_TYPE_TEXT;
    else
      type = CGUIEditControl::INPUT_TYPE_DATE;
    break;
  case CSmartPlaylistRule::SECONDS_FIELD:
    type = CGUIEditControl::INPUT_TYPE_SECONDS;
    break;
  case CSmartPlaylistRule::BOOLEAN_FIELD:
    type = CGUIEditControl::INPUT_TYPE_NUMBER;
    break;
  }
  SendMessage(GUI_MSG_SET_TYPE, CONTROL_VALUE, type, 21420);
}

void CGUIDialogSmartPlaylistRule::AddOperatorLabel(CSmartPlaylistRule::SEARCH_OPERATOR op)
{
  CGUIMessage select(GUI_MSG_LABEL_ADD, GetID(), CONTROL_OPERATOR, op);
  select.SetLabel(CSmartPlaylistRule::GetLocalizedOperator(op));
  OnMessage(select);
}

void CGUIDialogSmartPlaylistRule::OnWindowLoaded()
{
  CGUIWindow::OnWindowLoaded();
  ChangeButtonToEdit(CONTROL_VALUE, true); // true for single label
}

void CGUIDialogSmartPlaylistRule::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  SendMessage(GUI_MSG_LABEL_RESET, CONTROL_FIELD);
  // add the fields to the field spincontrol
  vector<Field> fields = CSmartPlaylistRule::GetFields(m_type);
  for (unsigned int i = 0; i < fields.size(); i++)
  {
    CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_FIELD, fields[i]);
    msg.SetLabel(CSmartPlaylistRule::GetLocalizedField(fields[i]));
    OnMessage(msg);
  }
  UpdateButtons();
}

void CGUIDialogSmartPlaylistRule::OnDeinitWindow(int nextWindowID)
{
  CGUIDialog::OnDeinitWindow(nextWindowID);

  // reset field spincontrolex
  SendMessage(GUI_MSG_LABEL_RESET, CONTROL_FIELD);
  // reset operator spincontrolex
  SendMessage(GUI_MSG_LABEL_RESET, CONTROL_OPERATOR);
}

bool CGUIDialogSmartPlaylistRule::EditRule(CSmartPlaylistRule &rule, const CStdString& type)
{
  CGUIDialogSmartPlaylistRule *editor = (CGUIDialogSmartPlaylistRule *)g_windowManager.GetWindow(WINDOW_DIALOG_SMART_PLAYLIST_RULE);
  if (!editor) return false;

  editor->m_rule = rule;
  editor->m_type = type == "mixed" ? "songs" : type;
  editor->DoModal(g_windowManager.GetActiveWindow());
  rule = editor->m_rule;
  return !editor->m_cancelled;
}

