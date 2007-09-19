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
#include "GUIDialogSmartPlaylistRule.h"
#include "GUIDialogKeyboard.h"
#include "GUIDialogNumeric.h"
#include "GUIDialogFileBrowser.h"
#include "Util.h"
#include "MusicDatabase.h"
#include "VideoDatabase.h"

#define CONTROL_FIELD           15
#define CONTROL_OPERATOR        16
#define CONTROL_VALUE           17
#define CONTROL_OK              18
#define CONTROL_CANCEL          19
#define CONTROL_BROWSE          20

using namespace PLAYLIST;

CGUIDialogSmartPlaylistRule::CGUIDialogSmartPlaylistRule(void)
    : CGUIDialog(WINDOW_DIALOG_SMART_PLAYLIST_RULE, "SmartPlaylistRule.xml")
{
  m_cancelled = false;
}

CGUIDialogSmartPlaylistRule::~CGUIDialogSmartPlaylistRule()
{
}

bool CGUIDialogSmartPlaylistRule::OnAction(const CAction &action)
{
  if (action.wID == ACTION_PREVIOUS_MENU)
    m_cancelled = true;
  return CGUIDialog::OnAction(action);
}

bool CGUIDialogSmartPlaylistRule::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      int iAction = message.GetParam1();
      if (iControl == CONTROL_OK)
        OnOK();
      else if (iControl == CONTROL_CANCEL)
        OnCancel();
      else if (iControl == CONTROL_VALUE)
        OnValue();
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

  int iLabel;
  if (m_rule.m_field == CSmartPlaylistRule::SONG_GENRE)
  {
    if (m_type.Equals("music") || m_type.Equals("mixed"))
      database.GetGenresNav("musicdb://4/",items);
    if (m_type.Equals("video") || m_type.Equals("mixed"))
    {
      CFileItemList items2;
      videodatabase.GetGenresNav("videodb://3/1/",items2,3);
      items.Append(items2);
    }
    iLabel = 515;
  }
  if (m_rule.m_field == CSmartPlaylistRule::SONG_ARTIST || m_rule.m_field == CSmartPlaylistRule::SONG_ALBUM_ARTIST)
  {
    if (m_type.Equals("music") || m_type.Equals("mixed"))
      database.GetArtistsNav("musicdb://5/",items,-1,m_rule.m_field == CSmartPlaylistRule::SONG_ALBUM_ARTIST);
    if (m_type.Equals("video") || m_type.Equals("mixed"))
    {
      CFileItemList items2;
      videodatabase.GetMusicVideoArtistsByName("",items2);
      items.Append(items2);
    }
    iLabel = 484;
  }
  if (m_rule.m_field == CSmartPlaylistRule::SONG_ALBUM)
  {
    if (m_type.Equals("music") || m_type.Equals("mixed"))
      database.GetAlbumsNav("musicdb://6/",items,-1,-1);
    if (m_type.Equals("video") || m_type.Equals("mixed"))
    {
      CFileItemList items2;
      videodatabase.GetMusicVideoAlbumsByName("",items2);
      items.Append(items2);
    }
    iLabel = 483;
  }

  CGUIDialogSelect* pDialog = (CGUIDialogSelect*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SELECT);
  pDialog->Reset();
  pDialog->SetItems(&items);
  CStdString strHeading;
  strHeading.Format(g_localizeStrings.Get(13401),g_localizeStrings.Get(iLabel));
  pDialog->SetHeading(strHeading);
  pDialog->DoModal();
  if (pDialog->GetSelectedLabel() > -1)
  {
    m_rule.m_parameter = pDialog->GetSelectedLabelText();
    UpdateButtons();
  }
  pDialog->Reset();
}

void CGUIDialogSmartPlaylistRule::OnCancel()
{
  m_cancelled = true;
  Close();
}

void CGUIDialogSmartPlaylistRule::OnValue()
{
  CStdString value(m_rule.m_parameter);
  switch (GetFieldType(m_rule.m_field))
  {
  case TEXT_FIELD:
    if (CGUIDialogKeyboard::ShowAndGetInput(value, g_localizeStrings.Get(21420), false))
      m_rule.m_parameter = value;
    break;
  case DATE_FIELD:
    if (m_rule.m_operator == CSmartPlaylistRule::OPERATOR_IN_THE_LAST)
    {
      if (CGUIDialogKeyboard::ShowAndGetInput(value, g_localizeStrings.Get(21420), false))
        m_rule.m_parameter = value;
    }
    else
    {
      CDateTime dateTime;
      dateTime.SetFromDBDate(m_rule.m_parameter);
      if (dateTime < CDateTime(2000,1, 1, 0, 0, 0))
        dateTime = CDateTime(2000, 1, 1, 0, 0, 0);
      SYSTEMTIME date;
      dateTime.GetAsSystemTime(date);
      if (CGUIDialogNumeric::ShowAndGetDate(date, g_localizeStrings.Get(21420)))
      {
        dateTime = CDateTime(date);
        m_rule.m_parameter = dateTime.GetAsDBDate();
      }
    }
    break;
  case SECONDS_FIELD:
    if (CGUIDialogNumeric::ShowAndGetSeconds(value, g_localizeStrings.Get(21420)))
      m_rule.m_parameter = value;
    break;
  case NUMERIC_FIELD:
    if (CGUIDialogNumeric::ShowAndGetNumber(value, g_localizeStrings.Get(21420)))
      m_rule.m_parameter = value;
    break;
  case PLAYLIST_FIELD:
    // use filebrowser to grab another smart playlist

    // Note: This can cause infinite loops (playlist that refers to the same playlist) but I don't
    //       think there's any decent way to deal with this, as the infinite loop may be an arbitrary
    //       number of playlists deep, eg playlist1 -> playlist2 -> playlist3 ... -> playlistn -> playlist1
    CStdString path = "special://musicplaylists/";
    if (m_type.Equals("video"))
      path = "special://videoplaylists/";
    if (CGUIDialogFileBrowser::ShowAndGetFile(path, ".xsp", g_localizeStrings.Get(656), path))
    {
      CSmartPlaylist playlist;
      if (playlist.Load(path))
        m_rule.m_parameter = !playlist.GetName().IsEmpty() ? playlist.GetName() : CUtil::GetTitleFromPath(path);
    }
    break;
  }
  UpdateButtons();
}

void CGUIDialogSmartPlaylistRule::OnField()
{
  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_FIELD);
  OnMessage(msg);
  m_rule.m_field = (CSmartPlaylistRule::DATABASE_FIELD)msg.GetParam1();

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
  if (m_rule.m_parameter.size() == 0)
  {
    CONTROL_DISABLE(CONTROL_OK)
  }
  else
  {
    CONTROL_ENABLE(CONTROL_OK)
  }

  // update the field control
  CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_FIELD, m_rule.m_field);
  OnMessage(msg);
  CGUIMessage msg2(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_FIELD);
  OnMessage(msg2);
  m_rule.m_field = (CSmartPlaylistRule::DATABASE_FIELD)msg2.GetParam1();

  // and now update the operator set
  CGUIMessage reset(GUI_MSG_LABEL_RESET, GetID(), CONTROL_OPERATOR);
  OnMessage(reset);

  switch (GetFieldType(m_rule.m_field))
  {
  case TEXT_FIELD:
    // text fields - add the usual comparisons
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_EQUALS);
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_DOES_NOT_EQUAL);
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_CONTAINS);
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_DOES_NOT_CONTAIN);
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_STARTS_WITH);
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_ENDS_WITH);
    break;

  case NUMERIC_FIELD:
  case SECONDS_FIELD:
    // numerical fields - less than greater than
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_EQUALS);
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_DOES_NOT_EQUAL);
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_GREATER_THAN);
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_LESS_THAN);
    break;

  case DATE_FIELD:
    // date field
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_AFTER);
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_BEFORE);
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_IN_THE_LAST);
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_NOT_IN_THE_LAST);
    break;

  case PLAYLIST_FIELD:
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_EQUALS);
    AddOperatorLabel(CSmartPlaylistRule::OPERATOR_DOES_NOT_EQUAL);
    break;
  }

  CGUIMessage select(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_OPERATOR, m_rule.m_operator);
  OnMessage(select);
  CGUIMessage selected(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_OPERATOR);
  OnMessage(selected);
  m_rule.m_operator = (CSmartPlaylistRule::SEARCH_OPERATOR)selected.GetParam1();
  if ((m_rule.m_field == CSmartPlaylistRule::SONG_ALBUM || m_rule.m_field == CSmartPlaylistRule::SONG_ARTIST || m_rule.m_field == CSmartPlaylistRule::SONG_GENRE || m_rule.m_field == CSmartPlaylistRule::SONG_ALBUM_ARTIST) && (m_rule.m_operator == CSmartPlaylistRule::OPERATOR_EQUALS || m_rule.m_operator == CSmartPlaylistRule::OPERATOR_DOES_NOT_EQUAL))
  {
    CONTROL_ENABLE(CONTROL_BROWSE)
  }
  else
  {
    CONTROL_DISABLE(CONTROL_BROWSE)
  }

  SET_CONTROL_LABEL(CONTROL_VALUE, m_rule.m_parameter);
}

void CGUIDialogSmartPlaylistRule::AddOperatorLabel(CSmartPlaylistRule::SEARCH_OPERATOR op)
{
  CGUIMessage select(GUI_MSG_LABEL_ADD, GetID(), CONTROL_OPERATOR, op);
  select.SetLabel(CSmartPlaylistRule::GetLocalizedOperator(op));
  OnMessage(select);
}

void CGUIDialogSmartPlaylistRule::OnInitWindow()
{
  CGUIDialog::OnInitWindow();
  // add the fields to the field spincontrol
  for (int field = CSmartPlaylistRule::FIELD_NONE + 1; field < CSmartPlaylistRule::FIELD_RANDOM; field++)
  {
    if (field == CSmartPlaylistRule::SONG_DATEADDED)
      continue;   // TODO: We don't have dateadded field in the database, so can't filter on this yet
    if (m_type.Equals("video"))
    {
      if (field == CSmartPlaylistRule::SONG_COMMENT || field == CSmartPlaylistRule::SONG_TRACKNUMBER || field == CSmartPlaylistRule::SONG_ALBUM_ARTIST || field == CSmartPlaylistRule::SONG_PLAYCOUNT || field == CSmartPlaylistRule::SONG_LASTPLAYED || field == CSmartPlaylistRule::SONG_TIME || field == CSmartPlaylistRule::SONG_RATING)
        continue;
    }
    CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_FIELD, field);
    msg.SetLabel(CSmartPlaylistRule::GetLocalizedField((CSmartPlaylistRule::DATABASE_FIELD)field));
    OnMessage(msg);
  }
  UpdateButtons();
}

bool CGUIDialogSmartPlaylistRule::EditRule(CSmartPlaylistRule &rule, const CStdString& type)
{
  CGUIDialogSmartPlaylistRule *editor = (CGUIDialogSmartPlaylistRule *)m_gWindowManager.GetWindow(WINDOW_DIALOG_SMART_PLAYLIST_RULE);
  if (!editor) return false;
  
  editor->m_rule = rule;
  editor->m_type = type;
  editor->DoModal(m_gWindowManager.GetActiveWindow());
  rule = editor->m_rule;
  return !editor->m_cancelled;
}

CGUIDialogSmartPlaylistRule::FIELD CGUIDialogSmartPlaylistRule::GetFieldType(CSmartPlaylistRule::DATABASE_FIELD field)
{
  switch (field)
  {
  case CSmartPlaylistRule::SONG_GENRE:
  case CSmartPlaylistRule::SONG_ALBUM:
  case CSmartPlaylistRule::SONG_ARTIST:
  case CSmartPlaylistRule::SONG_ALBUM_ARTIST:
  case CSmartPlaylistRule::SONG_TITLE:
  case CSmartPlaylistRule::SONG_FILENAME:
  case CSmartPlaylistRule::SONG_COMMENT:
    return TEXT_FIELD;

  case CSmartPlaylistRule::SONG_YEAR:
  case CSmartPlaylistRule::SONG_TRACKNUMBER:
  case CSmartPlaylistRule::SONG_PLAYCOUNT:
  case CSmartPlaylistRule::SONG_RATING:
    return NUMERIC_FIELD;

  case CSmartPlaylistRule::SONG_TIME:
    return SECONDS_FIELD;

  case CSmartPlaylistRule::SONG_LASTPLAYED:
    return DATE_FIELD;

  case CSmartPlaylistRule::FIELD_PLAYLIST:
    return PLAYLIST_FIELD;
  }
  return TEXT_FIELD;
}

