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

#include "GUIMultiSelectText.h"
#include "GUIWindowManager.h"
#include "Key.h"
#include "utils/log.h"

using namespace std;

CGUIMultiSelectTextControl::CSelectableString::CSelectableString(CGUIFont *font, const CStdString &text, bool selectable, const CStdString &clickAction)
 : m_text(font, false)
{
  m_selectable = selectable;
  m_clickAction = clickAction;
  m_clickAction.TrimLeft(" =");
  m_clickAction.TrimRight(" ");
  m_text.Update(text);
  float height;
  m_text.GetTextExtent(m_length, height);
}

CGUIMultiSelectTextControl::CGUIMultiSelectTextControl(int parentID, int controlID, float posX, float posY, float width, float height, const CTextureInfo& textureFocus, const CTextureInfo& textureNoFocus, const CLabelInfo& labelInfo, const CGUIInfoLabel &content)
    : CGUIControl(parentID, controlID, posX, posY, width, height)
    , m_button(parentID, controlID, posX, posY, width, height, textureFocus, textureNoFocus, labelInfo)
{
  m_info = content;
  m_label = labelInfo;
  m_selectedItem = 0;
  m_offset = 0;
  m_totalWidth = 0;
  m_scrollOffset = 0;
  m_scrollSpeed = 0;
  m_scrollLastTime = 0;
  m_renderTime = 0;
  m_label.align &= ~3; // we currently ignore all x alignment
}

CGUIMultiSelectTextControl::~CGUIMultiSelectTextControl(void)
{
}

bool CGUIMultiSelectTextControl::UpdateColors()
{
  bool changed = CGUIControl::UpdateColors();
  changed |= m_label.UpdateColors();

  return changed;
}

void CGUIMultiSelectTextControl::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  m_renderTime = currentTime;

  // check our selected item is in range
  unsigned int numSelectable = GetNumSelectable();
  if (!numSelectable)
    SetFocus(false);
  else if (m_selectedItem >= numSelectable)
    m_selectedItem = numSelectable - 1;

  // and validate our offset
  if (m_offset + m_width > m_totalWidth)
    m_offset = m_totalWidth - m_width;
  if (m_offset < 0) m_offset = 0;

  // handle scrolling
  m_scrollOffset += m_scrollSpeed * (m_renderTime - m_scrollLastTime);
  if ((m_scrollSpeed < 0 && m_scrollOffset < m_offset) ||
      (m_scrollSpeed > 0 && m_scrollOffset > m_offset))
  {
    m_scrollOffset = m_offset;
    m_scrollSpeed = 0;
  }
  m_scrollLastTime = m_renderTime;

  g_graphicsContext.SetOrigin(-m_scrollOffset, 0);

  // process the buttons
  for (unsigned int i = 0; i < m_buttons.size(); i++)
  {
    m_buttons[i].SetFocus(HasFocus() && i == m_selectedItem);
    m_buttons[i].DoProcess(currentTime, dirtyregions);
  }

  g_graphicsContext.RestoreOrigin();

  CGUIControl::Process(currentTime, dirtyregions);
}

void CGUIMultiSelectTextControl::Render()
{
  // clip and set our scrolling origin
  bool clip(m_width < m_totalWidth);
  if (clip)
  { // need to crop
    if (!g_graphicsContext.SetClipRegion(m_posX, m_posY, m_width, m_height))
      return; // nothing to render??
  }
  g_graphicsContext.SetOrigin(-m_scrollOffset, 0);

  // render the buttons
  for (unsigned int i = 0; i < m_buttons.size(); i++)
    m_buttons[i].DoRender();

  // position the text - we center vertically if applicable, and use the offsets.
  // all x-alignment is ignored for now (see constructor)
  float posX = m_posX;
  float posY = m_posY + m_label.offsetY;
  if (m_label.align & XBFONT_CENTER_Y)
    posY = m_posY + m_height * 0.5f;

  if (m_items.size() && m_items[0].m_selectable)
    posX += m_label.offsetX;

  // render the text
  unsigned int num_selectable = 0;
  for (unsigned int i = 0; i < m_items.size(); i++)
  {
    CSelectableString &string = m_items[i];
    if (IsDisabled()) // all text is rendered with disabled color
      string.m_text.Render(posX, posY, 0, m_label.disabledColor, m_label.shadowColor, m_label.align, 0, true);
    else if (HasFocus() && string.m_selectable && num_selectable == m_selectedItem) // text is rendered with focusedcolor
      string.m_text.Render(posX, posY, 0, m_label.focusedColor, m_label.shadowColor, m_label.align, 0);
    else // text is rendered with textcolor
      string.m_text.Render(posX, posY, 0, m_label.textColor, m_label.shadowColor, m_label.align, 0);
    posX += string.m_length;
    if (string.m_selectable)
      num_selectable++;
  }

  g_graphicsContext.RestoreOrigin();
  if (clip)
    g_graphicsContext.RestoreClipRegion();

  CGUIControl::Render();
}

void CGUIMultiSelectTextControl::UpdateInfo(const CGUIListItem *item)
{
  if (m_info.IsEmpty())
    return; // nothing to do

  if (item)
    UpdateText(m_info.GetItemLabel(item));
  else
    UpdateText(m_info.GetLabel(m_parentID));
}

bool CGUIMultiSelectTextControl::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_SELECT_ITEM)
  {
    // item is clicked - see if we have a clickaction
    CStdString clickAction;
    unsigned int selected = 0;
    for (unsigned int i = 0; i < m_items.size(); i++)
    {
      if (m_items[i].m_selectable)
      {
        if (m_selectedItem == selected)
          clickAction = m_items[i].m_clickAction;
        selected++;
      }
    }
    if (!clickAction.IsEmpty())
    { // have a click action -> perform it
      CGUIMessage message(GUI_MSG_EXECUTE, m_controlID, m_parentID);
      message.SetStringParam(clickAction);
      g_windowManager.SendMessage(message);
    }
    else
    { // no click action, just send a message to the window
      CGUIMessage msg(GUI_MSG_CLICKED, m_controlID, m_parentID, m_selectedItem);
      SendWindowMessage(msg);
    }
    return true;
  }
  return CGUIControl::OnAction(action);
}

void CGUIMultiSelectTextControl::OnLeft()
{
  if (MoveLeft())
    return;
  CGUIControl::OnLeft();
}

void CGUIMultiSelectTextControl::OnRight()
{
  if (MoveRight())
    return;
  CGUIControl::OnRight();
}

// movement functions (callable from lists)
bool CGUIMultiSelectTextControl::MoveLeft()
{
  if (m_selectedItem > 0)
    ScrollToItem(m_selectedItem - 1);
  else if (GetNumSelectable() && m_actionLeft.GetNavigation() && m_actionLeft.GetNavigation() == m_controlID)
    ScrollToItem(GetNumSelectable() - 1);
  else
    return false;
  return true;
}

bool CGUIMultiSelectTextControl::MoveRight()
{
  if (GetNumSelectable() && m_selectedItem < GetNumSelectable() - 1)
    ScrollToItem(m_selectedItem + 1);
  else if (m_actionRight.GetNavigation() && m_actionRight.GetNavigation() == m_controlID)
    ScrollToItem(0);
  else
    return false;
  return true;
}

void CGUIMultiSelectTextControl::SelectItemFromPoint(const CPoint &point)
{
  int item = GetItemFromPoint(point);
  if (item != -1)
  {
    ScrollToItem(item);
    SetFocus(true);
  }
  else
    SetFocus(false);
}

bool CGUIMultiSelectTextControl::HitTest(const CPoint &point) const
{
  return (GetItemFromPoint(point) != -1);
}

bool CGUIMultiSelectTextControl::OnMouseOver(const CPoint &point)
{
  ScrollToItem(GetItemFromPoint(point));
  return CGUIControl::OnMouseOver(point);
}

EVENT_RESULT CGUIMultiSelectTextControl::OnMouseEvent(const CPoint &point, const CMouseEvent &event)
{
  if (event.m_id == ACTION_MOUSE_LEFT_CLICK)
  {
    m_selectedItem = GetItemFromPoint(point);
    OnAction(CAction(ACTION_SELECT_ITEM));
    return EVENT_RESULT_HANDLED;
  }
  return EVENT_RESULT_UNHANDLED;
}

int CGUIMultiSelectTextControl::GetItemFromPoint(const CPoint &point) const
{
  if (!m_label.font) return -1;
  float posX = m_posX;
  unsigned int selectable = 0;
  for (unsigned int i = 0; i < m_items.size(); i++)
  {
    const CSelectableString &string = m_items[i];
    if (string.m_selectable)
    {
      CRect rect(posX, m_posY, posX + string.m_length, m_posY + m_height);
      if (rect.PtInRect(point))
        return selectable;
      selectable++;
    }
    posX += string.m_length;
  }
  return -1;
}

void CGUIMultiSelectTextControl::UpdateText(const CStdString &text)
{
  if (text == m_oldText)
    return;

  m_items.clear();

  // parse our text into clickable blocks
  // format is [ONCLICK <action>] [/ONCLICK]
  size_t startClickable = text.Find("[ONCLICK");
  size_t startUnclickable = 0;

  // add the first unclickable block
  if (startClickable != CStdString::npos)
    AddString(text.Mid(startUnclickable, startClickable - startUnclickable), false);
  else
    AddString(text.Mid(startUnclickable), false);
  while (startClickable != CStdString::npos)
  {
    // grep out the action and the end of the string
    size_t endAction = text.Find(']', startClickable + 8);
    size_t endClickable = text.Find("[/ONCLICK]", startClickable + 8);
    if (endAction != CStdString::npos && endClickable != CStdString::npos)
    { // success - add the string, and move the start of our next unclickable portion along
      AddString(text.Mid(endAction + 1, endClickable - endAction - 1), true, text.Mid(startClickable + 8, endAction - startClickable - 8));
      startUnclickable = endClickable + 10;
    }
    else
    {
      CLog::Log(LOGERROR, "Invalid multiselect string %s", text.c_str());
      break;
    }
    startClickable = text.Find("[ONCLICK", startUnclickable);
    // add the unclickable portion
    if (startClickable != CStdString::npos)
      AddString(text.Mid(startUnclickable, startClickable - startUnclickable), false);
    else
      AddString(text.Mid(startUnclickable), false);
  }

  m_oldText = text;

  // finally, position our buttons
  PositionButtons();
}

void CGUIMultiSelectTextControl::AddString(const CStdString &text, bool selectable, const CStdString &clickAction)
{
  if (!text.IsEmpty())
    m_items.push_back(CSelectableString(m_label.font, text, selectable, clickAction));
}

void CGUIMultiSelectTextControl::PositionButtons()
{
  m_buttons.clear();

  // add new buttons
  m_totalWidth = 0;
  if (m_items.size() && m_items.front().m_selectable)
    m_totalWidth += m_label.offsetX;

  for (unsigned int i = 0; i < m_items.size(); i++)
  {
    const CSelectableString &text = m_items[i];
    if (text.m_selectable)
    {
      CGUIButtonControl button(m_button);
      button.SetPosition(m_posX + m_totalWidth - m_label.offsetX, m_posY);
      button.SetWidth(text.m_length + 2 * m_label.offsetX);
      m_buttons.push_back(button);
    }
    m_totalWidth += text.m_length;
  }

  if (m_items.size() && m_items.back().m_selectable)
    m_totalWidth += m_label.offsetX;
}

CStdString CGUIMultiSelectTextControl::GetDescription() const
{
  // We currently just return the entire string - should we bother returning the
  // particular subitems of this?
  CStdString strLabel(m_info.GetLabel(m_parentID));
  return strLabel;
}

unsigned int CGUIMultiSelectTextControl::GetNumSelectable() const
{
  unsigned int selectable = 0;
  for (unsigned int i = 0; i < m_items.size(); i++)
    if (m_items[i].m_selectable)
      selectable++;
  return selectable;
}

unsigned int CGUIMultiSelectTextControl::GetFocusedItem() const
{
  if (GetNumSelectable())
    return m_selectedItem + 1;
  return 0;
}

void CGUIMultiSelectTextControl::SetFocusedItem(unsigned int item)
{
  SetFocus(item > 0);
  if (item > 0)
    ScrollToItem(item - 1);
}

bool CGUIMultiSelectTextControl::CanFocus() const
{
  if (!GetNumSelectable()) return false;
  return CGUIControl::CanFocus();
}

void CGUIMultiSelectTextControl::SetFocus(bool focus)
{
  for (unsigned int i = 0; i < m_buttons.size(); i++)
    m_buttons[i].SetFocus(focus);
  CGUIControl::SetFocus(focus);
}

// overrides to allow anims to translate down to the focus image
void CGUIMultiSelectTextControl::SetAnimations(const vector<CAnimation> &animations)
{
  // send any focus animations down to the focus image only
  m_animations.clear();
  vector<CAnimation> focusAnims;
  for (unsigned int i = 0; i < animations.size(); i++)
  {
    const CAnimation &anim = animations[i];
    if (anim.GetType() == ANIM_TYPE_FOCUS)
      focusAnims.push_back(anim);
    else
      m_animations.push_back(anim);
  }
  m_button.SetAnimations(focusAnims);
}

void CGUIMultiSelectTextControl::ScrollToItem(unsigned int item)
{
  static const unsigned int time_to_scroll = 200;
  if (item >= m_buttons.size()) return;
  // grab our button
  const CGUIButtonControl &button = m_buttons[item];
  float left = button.GetXPosition();
  float right = left + button.GetWidth();
  // make sure that we scroll so that this item is on screen
  m_scrollOffset = m_offset;
  if (left < m_posX + m_offset)
    m_offset = left - m_posX;
  else if (right > m_posX + m_offset + m_width)
    m_offset = right - m_width - m_posX;
  m_scrollSpeed = (m_offset - m_scrollOffset) / time_to_scroll;
  m_selectedItem = item;
}

