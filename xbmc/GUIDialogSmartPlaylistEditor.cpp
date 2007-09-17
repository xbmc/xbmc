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
#include "GUIDialogSmartPlaylistEditor.h"
#include "GUIDialogKeyboard.h"
#include "Util.h"
#include "GUIDialogSmartPlaylistRule.h"

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

#define CONTROL_OK              20
#define CONTROL_CANCEL          21
#define CONTROL_TYPE            22

using namespace PLAYLIST;

CGUIDialogSmartPlaylistEditor::CGUIDialogSmartPlaylistEditor(void)
    : CGUIDialog(WINDOW_DIALOG_SMART_PLAYLIST_EDITOR, "SmartPlaylistEditor.xml")
{
  m_cancelled = false;
}

CGUIDialogSmartPlaylistEditor::~CGUIDialogSmartPlaylistEditor()
{
}

bool CGUIDialogSmartPlaylistEditor::OnAction(const CAction &action)
{
  if (action.wID == ACTION_PREVIOUS_MENU)
    m_cancelled = true;
  return CGUIDialog::OnAction(action);
}

bool CGUIDialogSmartPlaylistEditor::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      int iAction = message.GetParam1();
      if (iControl == CONTROL_RULE_LIST && iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
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
      else
        return CGUIDialog::OnMessage(message);
      return true;
    }
    break;
  case GUI_MSG_WINDOW_INIT:
    {
      m_cancelled = false;
      UpdateButtons();
    }
    break;
  case GUI_MSG_WINDOW_DEINIT:
    {
      CGUIDialog::OnMessage(message);
      // clear the rule list
      CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_RULE_LIST);
      OnMessage(msg);
      m_ruleLabels.Clear();
    }
    break;
  case GUI_MSG_FOCUSED:
    if (message.GetControlId() == CONTROL_RULE_REMOVE ||
        message.GetControlId() == CONTROL_RULE_EDIT)
      HighlightItem(GetSelectedItem());
    else
      HighlightItem(-1);
    break;
  }
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogSmartPlaylistEditor::OnRuleList(int item)
{
  if (item < 0 || item >= (int)m_playlist.m_playlistRules.size()) return;

  CSmartPlaylistRule rule = m_playlist.m_playlistRules[item];

  if (CGUIDialogSmartPlaylistRule::EditRule(rule,m_playlist.GetType()))
    m_playlist.m_playlistRules[item] = rule;

  UpdateButtons();
}

void CGUIDialogSmartPlaylistEditor::OnName()
{
  CGUIDialogKeyboard::ShowAndGetInput(m_playlist.m_playlistName, g_localizeStrings.Get(1022), false);
  UpdateButtons();
}

void CGUIDialogSmartPlaylistEditor::OnOK()
{
  // save our playlist
  if (m_path.IsEmpty())
  {
    CStdString filename(m_playlist.m_playlistName);
    CStdString path;
    if (CGUIDialogKeyboard::ShowAndGetInput(filename, g_localizeStrings.Get(16013), false))
    {
      CStdString strTmp;
      CUtil::AddFileToFolder(m_playlist.m_playlistType,filename,strTmp);
      CUtil::AddFileToFolder(g_guiSettings.GetString("system.playlistspath"),strTmp,path);
    }
    else
      return;
    if (CUtil::GetExtension(path) != ".xsp")
      path += ".xsp";

    // should we check whether we should overwrite?
    m_path = path;
  }
  else
  {
    if (m_path.Left(g_guiSettings.GetString("system.playlistspath").size()).Equals(g_guiSettings.GetString("system.playlistspath"))) // fugly, well aware
    {
      CStdString filename = CUtil::GetFileName(m_path);
      CStdString strType = m_path.Mid(g_guiSettings.GetString("system.playlistspath").size(),m_path.size()-filename.size()-g_guiSettings.GetString("system.playlistspath").size()-1);
      if (!strType.Equals(m_playlist.m_playlistType))
      {
        XFILE::CFile::Delete(m_path);
        CStdString strTmp;
        CUtil::AddFileToFolder(m_playlist.m_playlistType,filename,strTmp);
        CUtil::AddFileToFolder(g_guiSettings.GetString("system.playlistspath"),strTmp,m_path);
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
  m_playlist.m_matchAllRules = (msg.GetParam1() == 0);
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
  if (msg.GetParam1() == 1)
    m_playlist.SetType("music");
  if (msg.GetParam1() == 2)
    m_playlist.SetType("video");
  if (msg.GetParam1() == 3)
    m_playlist.SetType("mixed");
  UpdateButtons();
}

void CGUIDialogSmartPlaylistEditor::OnOrder()
{
  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_ORDER_FIELD);
  OnMessage(msg);
  m_playlist.m_orderField = (CSmartPlaylistRule::DATABASE_FIELD)msg.GetParam1();
  UpdateButtons();
}

void CGUIDialogSmartPlaylistEditor::OnOrderDirection()
{
  m_playlist.m_orderAscending = !m_playlist.m_orderAscending;
  UpdateButtons();
}

void CGUIDialogSmartPlaylistEditor::UpdateButtons()
{
  if (m_playlist.m_playlistRules.size() == 0 || m_playlist.m_playlistRules[0].m_field == CSmartPlaylistRule::FIELD_NONE)
  {
    CONTROL_DISABLE(CONTROL_OK)
  }
  else
  {
    CONTROL_ENABLE(CONTROL_OK)
  }

  if (m_playlist.m_playlistRules.size() <= 1)
  {
    CONTROL_DISABLE(CONTROL_RULE_REMOVE)
    CONTROL_DISABLE(CONTROL_MATCH)
  }
  else
  {
    CONTROL_ENABLE(CONTROL_RULE_REMOVE)
    CONTROL_ENABLE(CONTROL_MATCH)
  }
  // name
  SET_CONTROL_LABEL(CONTROL_NAME, m_playlist.m_playlistName)

  int currentItem = GetSelectedItem();
  CGUIMessage msgReset(GUI_MSG_LABEL_RESET, GetID(), CONTROL_RULE_LIST);
  OnMessage(msgReset);
  m_ruleLabels.Clear();
  for (unsigned int i = 0; i < m_playlist.m_playlistRules.size(); i++)
  {
    CFileItem* item = new CFileItem("", false);
    if (m_playlist.m_playlistRules[i].m_field == CSmartPlaylistRule::FIELD_NONE)
      item->SetLabel(g_localizeStrings.Get(21423));
    else
      item->SetLabel(m_playlist.m_playlistRules[i].GetLocalizedRule());
    m_ruleLabels.Add(item);
    CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_RULE_LIST, 0, 0, (void*)item);
    OnMessage(msg);
  }
  CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_RULE_LIST, currentItem);
  OnMessage(msg);

  if (m_playlist.m_orderAscending)
  {
    CONTROL_SELECT(CONTROL_ORDER_DIRECTION);
  }
  else
  {
    CONTROL_DESELECT(CONTROL_ORDER_DIRECTION);
  }
}

void CGUIDialogSmartPlaylistEditor::OnWindowLoaded()
{
  CGUIDialog::OnWindowLoaded();
  // setup the match spinner
  {
    CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_MATCH, 0);
    msg.SetLabel(21425);
    OnMessage(msg);
  }
  {
    CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_MATCH, 1);
    msg.SetLabel(21426);
    OnMessage(msg);
  }
  {
    CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_MATCH, m_playlist.m_matchAllRules ? 0 : 1);
    OnMessage(msg);
  }
  // and now the limit spinner
  {
    CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_LIMIT, 0);
    msg.SetLabel(21428);
    OnMessage(msg);
  }
  const int limits[] = { 10, 25, 50, 100, 250, 500, 1000 };
  float amountOut = 1.0f;
  for (int i = 0; i < sizeof(limits) / sizeof(int); i++)
  {
    CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_LIMIT, limits[i]);
    CStdString label; label.Format(g_localizeStrings.Get(21436).c_str(), limits[i]);
    msg.SetLabel(label);
    OnMessage(msg);
  }
  {
    CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_LIMIT, m_playlist.m_limit);
    OnMessage(msg);
  }
  // and the order by spinner
  for (int field = CSmartPlaylistRule::FIELD_NONE; field <= CSmartPlaylistRule::FIELD_RANDOM; field++)
  {
    CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_ORDER_FIELD, field);
    msg.SetLabel(CSmartPlaylistRule::GetLocalizedField((CSmartPlaylistRule::DATABASE_FIELD)field));
    OnMessage(msg);
  }
  {
    CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_ORDER_FIELD, m_playlist.m_orderField);
    OnMessage(msg);
  }
  // type
  {
    CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_TYPE, 1);
    CStdString label = g_localizeStrings.Get(2);
    msg.SetLabel(label);
    OnMessage(msg);
  }
  {
    CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_TYPE, 2);
    CStdString label = g_localizeStrings.Get(3);
    msg.SetLabel(label);
    OnMessage(msg);
  }
  {
    CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_TYPE, 3);
    CStdString label= g_localizeStrings.Get(20395);
    msg.SetLabel(label);
    OnMessage(msg);
  }
  {
    int iType=1;
    CStdString type = m_playlist.GetType();
    if (m_playlist.GetType() == "video")
      iType = 2;
    if (m_playlist.GetType() == "mixed")
      iType = 3;
    CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_TYPE, iType);
    OnMessage(msg);
  }

}

int CGUIDialogSmartPlaylistEditor::GetSelectedItem()
{
  CGUIMessage message(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_RULE_LIST);
  OnMessage(message);
  return message.GetParam1();
}

void CGUIDialogSmartPlaylistEditor::HighlightItem(int item)
{
  for (int i = 0; i < m_ruleLabels.Size(); i++)
    m_ruleLabels[i]->Select(false);
  if (item >= 0 && item < m_ruleLabels.Size())
    m_ruleLabels[item]->Select(true);
  CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_RULE_LIST, item);
  OnMessage(msg);
}

void CGUIDialogSmartPlaylistEditor::OnRuleRemove(int item)
{
  if (item < 0 || item >= (int)m_playlist.m_playlistRules.size()) return;
  m_playlist.m_playlistRules.erase(m_playlist.m_playlistRules.begin() + item);

  UpdateButtons();
  if (item >= m_ruleLabels.Size())
    HighlightItem(m_ruleLabels.Size() - 1);
  else
    HighlightItem(item);
  if (m_ruleLabels.Size() <= 1)
  {
    SET_CONTROL_FOCUS(CONTROL_RULE_ADD, 0);
  }
}

void CGUIDialogSmartPlaylistEditor::OnRuleAdd()
{
  CSmartPlaylistRule rule;
  if (CGUIDialogSmartPlaylistRule::EditRule(rule,m_playlist.GetType()))
  {
    if (m_playlist.m_playlistRules.size() == 1 && m_playlist.m_playlistRules[0].m_field == CSmartPlaylistRule::FIELD_NONE)
      m_playlist.m_playlistRules[0] = rule;
    else
      m_playlist.m_playlistRules.push_back(rule);
  }
  UpdateButtons();
}

bool CGUIDialogSmartPlaylistEditor::NewPlaylist(const CStdString &type)
{
  CGUIDialogSmartPlaylistEditor *editor = (CGUIDialogSmartPlaylistEditor *)m_gWindowManager.GetWindow(WINDOW_DIALOG_SMART_PLAYLIST_EDITOR);
  if (!editor) return false;

  editor->m_path = "";
  editor->m_playlist = CSmartPlaylist();
  editor->m_playlist.m_playlistRules.push_back(CSmartPlaylistRule());
  editor->m_playlist.SetType(type);
  editor->Initialize();
  editor->DoModal(m_gWindowManager.GetActiveWindow());
  return !editor->m_cancelled;
}

bool CGUIDialogSmartPlaylistEditor::EditPlaylist(const CStdString &path)
{
  CGUIDialogSmartPlaylistEditor *editor = (CGUIDialogSmartPlaylistEditor *)m_gWindowManager.GetWindow(WINDOW_DIALOG_SMART_PLAYLIST_EDITOR);
  if (!editor) return false;

  CSmartPlaylist playlist;
  if (!playlist.Load(path)) return false;

  editor->m_playlist = playlist;
  editor->m_path = path;
  editor->Initialize();
  editor->DoModal(m_gWindowManager.GetActiveWindow());
  return !editor->m_cancelled;
}
