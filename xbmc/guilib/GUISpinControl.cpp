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

#include "GUISpinControl.h"
#include "utils/CharsetConverter.h"
#include "Key.h"

using namespace std;

#define SPIN_BUTTON_DOWN 1
#define SPIN_BUTTON_UP   2

CGUISpinControl::CGUISpinControl(int parentID, int controlID, float posX, float posY, float width, float height, const CTextureInfo& textureUp, const CTextureInfo& textureDown, const CTextureInfo& textureUpFocus, const CTextureInfo& textureDownFocus, const CLabelInfo &labelInfo, int iType)
    : CGUIControl(parentID, controlID, posX, posY, width, height)
    , m_imgspinUp(posX, posY, width, height, textureUp)
    , m_imgspinDown(posX, posY, width, height, textureDown)
    , m_imgspinUpFocus(posX, posY, width, height, textureUpFocus)
    , m_imgspinDownFocus(posX, posY, width, height, textureDownFocus)
    , m_label(posX, posY, width, height, labelInfo)
{
  m_bReverse = false;
  m_iStart = 0;
  m_iEnd = 100;
  m_fStart = 0.0f;
  m_fEnd = 1.0f;
  m_fInterval = 0.1f;
  m_iValue = 0;
  m_fValue = 0.0;
  m_iType = iType;
  m_iSelect = SPIN_BUTTON_DOWN;
  m_bShowRange = false;
  m_iTypedPos = 0;
  strcpy(m_szTyped, "");
  ControlType = GUICONTROL_SPIN;
  m_currentItem = 0;
  m_numItems = 10;
  m_itemsPerPage = 10;
  m_showOnePage = true;
}

CGUISpinControl::~CGUISpinControl(void)
{}

bool CGUISpinControl::OnAction(const CAction &action)
{
  switch (action.GetID())
  {
  case REMOTE_0:
  case REMOTE_1:
  case REMOTE_2:
  case REMOTE_3:
  case REMOTE_4:
  case REMOTE_5:
  case REMOTE_6:
  case REMOTE_7:
  case REMOTE_8:
  case REMOTE_9:
    {
      if (strlen(m_szTyped) >= 3)
      {
        m_iTypedPos = 0;
        strcpy(m_szTyped, "");
      }
      int iNumber = action.GetID() - REMOTE_0;

      m_szTyped[m_iTypedPos] = iNumber + '0';
      m_iTypedPos++;
      m_szTyped[m_iTypedPos] = 0;
      int iValue;
      sscanf(m_szTyped, "%i", &iValue);
      switch (m_iType)
      {
      case SPIN_CONTROL_TYPE_INT:
        {
          if (iValue < m_iStart || iValue > m_iEnd)
          {
            m_iTypedPos = 0;
            m_szTyped[m_iTypedPos] = iNumber + '0';
            m_iTypedPos++;
            m_szTyped[m_iTypedPos] = 0;
            sscanf(m_szTyped, "%i", &iValue);
            if (iValue < m_iStart || iValue > m_iEnd)
            {
              m_iTypedPos = 0;
              strcpy(m_szTyped, "");
              return true;
            }
          }
          m_iValue = iValue;
          CGUIMessage msg(GUI_MSG_CLICKED, GetID(), GetParentID());
          SendWindowMessage(msg);
        }
        break;

      case SPIN_CONTROL_TYPE_TEXT:
        {
          if (iValue < 0 || iValue >= (int)m_vecLabels.size())
          {
            m_iTypedPos = 0;
            m_szTyped[m_iTypedPos] = iNumber + '0';
            m_iTypedPos++;
            m_szTyped[m_iTypedPos] = 0;
            sscanf(m_szTyped, "%i", &iValue);
            if (iValue < 0 || iValue >= (int)m_vecLabels.size())
            {
              m_iTypedPos = 0;
              strcpy(m_szTyped, "");
              return true;
            }
          }
          m_iValue = iValue;
          CGUIMessage msg(GUI_MSG_CLICKED, GetID(), GetParentID());
          SendWindowMessage(msg);
        }
        break;

      }
      return true;
    }
    break;
  case ACTION_PAGE_UP:
    if (!m_bReverse)
      PageDown();
    else
      PageUp();
    return true;
    break;
  case ACTION_PAGE_DOWN:
    if (!m_bReverse)
      PageUp();
    else
      PageDown();
    return true;
    break;
  case ACTION_SELECT_ITEM:
    if (m_iSelect == SPIN_BUTTON_UP)
    {
      MoveUp();
      return true;
    }
    if (m_iSelect == SPIN_BUTTON_DOWN)
    {
      MoveDown();
      return true;
    }
    break;
  }
/*  static float m_fSmoothScrollOffset = 0.0f;
  if (action.GetID() == ACTION_SCROLL_UP)
  {
    m_fSmoothScrollOffset += action.GetAmount() * action.GetAmount();
    bool handled = false;
    while (m_fSmoothScrollOffset > 0.4)
    {
      handled = true;
      m_fSmoothScrollOffset -= 0.4f;
      MoveDown();
    }
    return handled;
  }*/
  return CGUIControl::OnAction(action);
}

void CGUISpinControl::OnLeft()
{
  if (m_iSelect == SPIN_BUTTON_UP)
  {
    // select the down button
    m_iSelect = SPIN_BUTTON_DOWN;
    MarkDirtyRegion();
  }
  else
  { // base class
    CGUIControl::OnLeft();
  }
}

void CGUISpinControl::OnRight()
{
  if (m_iSelect == SPIN_BUTTON_DOWN)
  {
    // select the up button
    m_iSelect = SPIN_BUTTON_UP;
    MarkDirtyRegion();
  }
  else
  { // base class
    CGUIControl::OnRight();
  }
}

void CGUISpinControl::Clear()
{
  m_vecLabels.erase(m_vecLabels.begin(), m_vecLabels.end());
  m_vecValues.erase(m_vecValues.begin(), m_vecValues.end());
  SetValue(0);
}

bool CGUISpinControl::OnMessage(CGUIMessage& message)
{
  if (CGUIControl::OnMessage(message) )
    return true;
  if (message.GetControlId() == GetID() )
  {
    switch (message.GetMessage())
    {
    case GUI_MSG_ITEM_SELECT:
      if (SPIN_CONTROL_TYPE_PAGE == m_iType)
      {
        m_currentItem = message.GetParam1();
        return true;
      }
      SetValue( message.GetParam1());
      if (message.GetParam2() == SPIN_BUTTON_DOWN || message.GetParam2() == SPIN_BUTTON_UP)
        m_iSelect = message.GetParam2();
      return true;
      break;

    case GUI_MSG_LABEL_RESET:
      if (SPIN_CONTROL_TYPE_PAGE == m_iType)
      {
        m_itemsPerPage = message.GetParam1();
        m_numItems = message.GetParam2();
        return true;
      }
      {
        Clear();
        return true;
      }
      break;

    case GUI_MSG_SHOWRANGE:
      if (message.GetParam1() )
        m_bShowRange = true;
      else
        m_bShowRange = false;
      break;

    case GUI_MSG_LABEL_ADD:
      {
        AddLabel(message.GetLabel(), message.GetParam1());
        return true;
      }
      break;

    case GUI_MSG_ITEM_SELECTED:
      {
        message.SetParam1( GetValue() );
        message.SetParam2(m_iSelect);

        if (m_iType == SPIN_CONTROL_TYPE_TEXT)
        {
          if ( m_iValue >= 0 && m_iValue < (int)m_vecLabels.size() )
            message.SetLabel( m_vecLabels[m_iValue]);
        }
        return true;
      }

    case GUI_MSG_PAGE_UP:
      if (CanMoveUp())
        MoveUp();
      return true;

    case GUI_MSG_PAGE_DOWN:
      if (CanMoveDown())
        MoveDown();
      return true;

    case GUI_MSG_MOVE_OFFSET:
      {
        int count = (int)message.GetParam1();
        while (count < 0)
        {
          MoveUp();
          count++;
        }
        while (count > 0)
        {
          MoveDown();
          count--;
        }
        return true;
      }

    }
  }
  return false;
}

void CGUISpinControl::AllocResources()
{
  CGUIControl::AllocResources();
  m_imgspinUp.AllocResources();
  m_imgspinUpFocus.AllocResources();
  m_imgspinDown.AllocResources();
  m_imgspinDownFocus.AllocResources();

  m_imgspinDownFocus.SetPosition(m_posX, m_posY);
  m_imgspinDown.SetPosition(m_posX, m_posY);
  m_imgspinUp.SetPosition(m_posX + m_imgspinDown.GetWidth(), m_posY);
  m_imgspinUpFocus.SetPosition(m_posX + m_imgspinDownFocus.GetWidth(), m_posY);
}

void CGUISpinControl::FreeResources(bool immediately)
{
  CGUIControl::FreeResources(immediately);
  m_imgspinUp.FreeResources(immediately);
  m_imgspinUpFocus.FreeResources(immediately);
  m_imgspinDown.FreeResources(immediately);
  m_imgspinDownFocus.FreeResources(immediately);
  m_iTypedPos = 0;
  strcpy(m_szTyped, "");
}

void CGUISpinControl::DynamicResourceAlloc(bool bOnOff)
{
  CGUIControl::DynamicResourceAlloc(bOnOff);
  m_imgspinUp.DynamicResourceAlloc(bOnOff);
  m_imgspinUpFocus.DynamicResourceAlloc(bOnOff);
  m_imgspinDown.DynamicResourceAlloc(bOnOff);
  m_imgspinDownFocus.DynamicResourceAlloc(bOnOff);
}

void CGUISpinControl::SetInvalid()
{
  CGUIControl::SetInvalid();
  m_label.SetInvalid();
  m_imgspinUp.SetInvalid();
  m_imgspinUpFocus.SetInvalid();
  m_imgspinDown.SetInvalid();
  m_imgspinDownFocus.SetInvalid();
}

void CGUISpinControl::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  bool changed = false;

  if (!HasFocus())
  {
    m_iTypedPos = 0;
    strcpy(m_szTyped, "");
  }

  CStdString text;

  if (m_iType == SPIN_CONTROL_TYPE_INT)
  {
    if (m_bShowRange)
    {
      text.Format("%i/%i", m_iValue, m_iEnd);
    }
    else
    {
      text.Format("%i", m_iValue);
    }
  }
  else if (m_iType == SPIN_CONTROL_TYPE_PAGE)
  {
    // work out number of pages and current page
    int numPages = (m_numItems + m_itemsPerPage - 1) / m_itemsPerPage;
    int currentPage = m_currentItem / m_itemsPerPage + 1;
    if (m_currentItem >= m_numItems - m_itemsPerPage)
      currentPage = numPages;
    text.Format("%i/%i", currentPage, numPages);
  }
  else if (m_iType == SPIN_CONTROL_TYPE_FLOAT)
  {
    if (m_bShowRange)
    {
      text.Format("%02.2f/%02.2f", m_fValue, m_fEnd);
    }
    else
    {
      text.Format("%02.2f", m_fValue);
    }
  }
  else
  {
    if (m_iValue >= 0 && m_iValue < (int)m_vecLabels.size() )
    {
      if (m_bShowRange)
      {
        text.Format("(%i/%i) %s", m_iValue + 1, (int)m_vecLabels.size(), CStdString(m_vecLabels[m_iValue]).c_str() );
      }
      else
      {
        text.Format("%s", CStdString(m_vecLabels[m_iValue]).c_str() );
      }
    }
    else text.Format("?%i?", m_iValue);

  }

  changed |= m_label.SetText(text);

  const float space = 5;
  float textWidth = m_label.GetTextWidth() + 2 * m_label.GetLabelInfo().offsetX;
  // Position the arrows
  bool arrowsOnRight(0 != (m_label.GetLabelInfo().align & (XBFONT_RIGHT | XBFONT_CENTER_X)));
  if (!arrowsOnRight)
  {
    changed |= m_imgspinDownFocus.SetPosition(m_posX + textWidth + space, m_posY);
    changed |= m_imgspinDown.SetPosition(m_posX + textWidth + space, m_posY);
    changed |= m_imgspinUpFocus.SetPosition(m_posX + textWidth + space + m_imgspinDown.GetWidth(), m_posY);
    changed |= m_imgspinUp.SetPosition(m_posX + textWidth + space + m_imgspinDown.GetWidth(), m_posY);
  }

  changed |= m_imgspinDownFocus.Process(currentTime);
  changed |= m_imgspinDown.Process(currentTime);
  changed |= m_imgspinUp.Process(currentTime);
  changed |= m_imgspinUpFocus.Process(currentTime);

  if (changed)
    MarkDirtyRegion();

  CGUIControl::Process(currentTime, dirtyregions);
}

void CGUISpinControl::Render()
{
  if ( HasFocus() )
  {
    if (m_iSelect == SPIN_BUTTON_UP)
      m_imgspinUpFocus.Render();
    else
      m_imgspinUp.Render();

    if (m_iSelect == SPIN_BUTTON_DOWN)
      m_imgspinDownFocus.Render();
    else
      m_imgspinDown.Render();
  }
  else
  {
    m_imgspinUp.Render();
    m_imgspinDown.Render();
  }

  if (m_label.GetLabelInfo().font)
  {
    const float space = 5;
    float textWidth = m_label.GetTextWidth() + 2 * m_label.GetLabelInfo().offsetX;
    // Position the arrows
    bool arrowsOnRight(0 != (m_label.GetLabelInfo().align & (XBFONT_RIGHT | XBFONT_CENTER_X)));

    if (arrowsOnRight)
      RenderText(m_posX - space - textWidth, m_posY, textWidth, m_height);
    else
      RenderText(m_posX + m_imgspinDown.GetWidth() + m_imgspinUp.GetWidth() + space, m_posY, textWidth, m_height);

    // set our hit rectangle for MouseOver events
    m_hitRect = m_label.GetRenderRect();
  }
  CGUIControl::Render();
}

void CGUISpinControl::RenderText(float posX, float posY, float width, float height)
{
  m_label.SetMaxRect(posX, posY, width, height);
  m_label.SetColor(GetTextColor());
  m_label.Render();
}

CGUILabel::COLOR CGUISpinControl::GetTextColor() const
{
  if (IsDisabled())
    return CGUILabel::COLOR_DISABLED;
  else if (HasFocus())
    return CGUILabel::COLOR_FOCUSED;
  return CGUILabel::COLOR_TEXT;
}

void CGUISpinControl::SetRange(int iStart, int iEnd)
{
  m_iStart = iStart;
  m_iEnd = iEnd;
}


void CGUISpinControl::SetFloatRange(float fStart, float fEnd)
{
  m_fStart = fStart;
  m_fEnd = fEnd;
}

void CGUISpinControl::SetValueFromLabel(const CStdString &label)
{
  if (m_iType == SPIN_CONTROL_TYPE_TEXT)
  {
    m_iValue = 0;
    for (unsigned int i = 0; i < m_vecLabels.size(); i++)
      if (label == m_vecLabels[i])
        m_iValue = i;
  }
  else
    m_iValue = atoi(label.c_str());
}

void CGUISpinControl::SetValue(int iValue)
{
  if (m_iType == SPIN_CONTROL_TYPE_TEXT)
  {
    m_iValue = 0;
    for (unsigned int i = 0; i < m_vecValues.size(); i++)
      if (iValue == m_vecValues[i])
        m_iValue = i;
  }
  else
    m_iValue = iValue;

  SetInvalid();
}

void CGUISpinControl::SetFloatValue(float fValue)
{
  m_fValue = fValue;
}

int CGUISpinControl::GetValue() const
{
  if (m_iType == SPIN_CONTROL_TYPE_TEXT)
  {
    if (m_iValue >= 0 && m_iValue < (int)m_vecValues.size())
      return m_vecValues[m_iValue];
  }
  return m_iValue;
}

float CGUISpinControl::GetFloatValue() const
{
  return m_fValue;
}


void CGUISpinControl::AddLabel(const string& strLabel, int iValue)
{
  m_vecLabels.push_back(strLabel);
  m_vecValues.push_back(iValue);
}

const string CGUISpinControl::GetLabel() const
{
  if (m_iValue >= 0 && m_iValue < (int)m_vecLabels.size())
  {
    return m_vecLabels[ m_iValue];
  }
  return "";
}

void CGUISpinControl::SetPosition(float posX, float posY)
{
  CGUIControl::SetPosition(posX, posY);

  m_imgspinDownFocus.SetPosition(posX, posY);
  m_imgspinDown.SetPosition(posX, posY);

  m_imgspinUp.SetPosition(m_posX + m_imgspinDown.GetWidth(), m_posY);
  m_imgspinUpFocus.SetPosition(m_posX + m_imgspinDownFocus.GetWidth(), m_posY);

}

float CGUISpinControl::GetWidth() const
{
  return m_imgspinDown.GetWidth() * 2 ;
}

bool CGUISpinControl::CanMoveUp(bool bTestReverse)
{
  // test for reverse...
  if (bTestReverse && m_bReverse) return CanMoveDown(false);

  switch (m_iType)
  {
  case SPIN_CONTROL_TYPE_PAGE:
    return m_currentItem > 0;
  case SPIN_CONTROL_TYPE_INT:
    {
      if (m_iValue - 1 >= m_iStart)
        return true;
      return false;
    }
    break;

  case SPIN_CONTROL_TYPE_FLOAT:
    {
      if (m_fValue - m_fInterval >= m_fStart)
        return true;
      return false;
    }
    break;

  case SPIN_CONTROL_TYPE_TEXT:
    {
      if (m_iValue - 1 >= 0)
        return true;
      return false;
    }
    break;
  }
  return false;
}

bool CGUISpinControl::CanMoveDown(bool bTestReverse)
{
  // test for reverse...
  if (bTestReverse && m_bReverse) return CanMoveUp(false);
  switch (m_iType)
  {
  case SPIN_CONTROL_TYPE_PAGE:
    return m_currentItem < m_numItems;
  case SPIN_CONTROL_TYPE_INT:
    {
      if (m_iValue + 1 <= m_iEnd)
        return true;
      return false;
    }
    break;

  case SPIN_CONTROL_TYPE_FLOAT:
    {
      if (m_fValue + m_fInterval <= m_fEnd)
        return true;
      return false;
    }
    break;

  case SPIN_CONTROL_TYPE_TEXT:
    {
      if (m_iValue + 1 < (int)m_vecLabels.size())
        return true;
      return false;
    }
    break;
  }
  return false;
}

void CGUISpinControl::PageUp()
{
  switch (m_iType)
  {
  case SPIN_CONTROL_TYPE_INT:
    {
      if (m_iValue - 10 >= m_iStart)
        m_iValue -= 10;
      else
        m_iValue = m_iStart;
      CGUIMessage msg(GUI_MSG_CLICKED, GetID(), GetParentID());
      SendWindowMessage(msg);
      return ;
    }
    break;
  case SPIN_CONTROL_TYPE_PAGE:
    ChangePage(-10);
    break;
  case SPIN_CONTROL_TYPE_TEXT:
    {
      if (m_iValue - 10 >= 0)
        m_iValue -= 10;
      else
        m_iValue = 0;
      CGUIMessage msg(GUI_MSG_CLICKED, GetID(), GetParentID());
      SendWindowMessage(msg);
      return ;
    }
    break;
  }

}

void CGUISpinControl::PageDown()
{
  switch (m_iType)
  {
  case SPIN_CONTROL_TYPE_INT:
    {
      if (m_iValue + 10 <= m_iEnd)
        m_iValue += 10;
      else
        m_iValue = m_iEnd;
      CGUIMessage msg(GUI_MSG_CLICKED, GetID(), GetParentID());
      SendWindowMessage(msg);
      return ;
    }
    break;
  case SPIN_CONTROL_TYPE_PAGE:
    ChangePage(10);
    break;
  case SPIN_CONTROL_TYPE_TEXT:
    {
      if (m_iValue + 10 < (int)m_vecLabels.size() )
        m_iValue += 10;
      CGUIMessage msg(GUI_MSG_CLICKED, GetID(), GetParentID());
      SendWindowMessage(msg);
    }
    break;
  }
}

void CGUISpinControl::MoveUp(bool bTestReverse)
{
  if (bTestReverse && m_bReverse)
  { // actually should move down.
    MoveDown(false);
    return ;
  }
  switch (m_iType)
  {
  case SPIN_CONTROL_TYPE_INT:
    {
      if (m_iValue - 1 >= m_iStart)
        m_iValue--;
      else if (m_iValue == m_iStart)
        m_iValue = m_iEnd;
      CGUIMessage msg(GUI_MSG_CLICKED, GetID(), GetParentID());
      SendWindowMessage(msg);
      return ;
    }
    break;

  case SPIN_CONTROL_TYPE_PAGE:
    ChangePage(-1);
    break;

  case SPIN_CONTROL_TYPE_FLOAT:
    {
      if (m_fValue - m_fInterval >= m_fStart)
        m_fValue -= m_fInterval;
      else if (m_fValue - m_fInterval < m_fStart)
        m_fValue = m_fEnd;
      CGUIMessage msg(GUI_MSG_CLICKED, GetID(), GetParentID());
      SendWindowMessage(msg);
      return ;
    }
    break;

  case SPIN_CONTROL_TYPE_TEXT:
    {
      if (m_iValue - 1 >= 0)
        m_iValue--;
      else if (m_iValue == 0)
        m_iValue = (int)m_vecLabels.size() - 1;
      CGUIMessage msg(GUI_MSG_CLICKED, GetID(), GetParentID());
      SendWindowMessage(msg);
      return ;
    }
    break;
  }
}

void CGUISpinControl::MoveDown(bool bTestReverse)
{
  if (bTestReverse && m_bReverse)
  { // actually should move up.
    MoveUp(false);
    return ;
  }
  switch (m_iType)
  {
  case SPIN_CONTROL_TYPE_INT:
    {
      if (m_iValue + 1 <= m_iEnd)
        m_iValue++;
      else if (m_iValue == m_iEnd)
        m_iValue = m_iStart;
      CGUIMessage msg(GUI_MSG_CLICKED, GetID(), GetParentID());
      SendWindowMessage(msg);
      return ;
    }
    break;

  case SPIN_CONTROL_TYPE_PAGE:
    ChangePage(1);
    break;

  case SPIN_CONTROL_TYPE_FLOAT:
    {
      if (m_fValue + m_fInterval <= m_fEnd)
        m_fValue += m_fInterval;
      else if (m_fValue + m_fInterval > m_fEnd)
        m_fValue = m_fStart;
      CGUIMessage msg(GUI_MSG_CLICKED, GetID(), GetParentID());
      SendWindowMessage(msg);
      return ;
    }
    break;

  case SPIN_CONTROL_TYPE_TEXT:
    {
      if (m_iValue + 1 < (int)m_vecLabels.size() )
        m_iValue++;
      else if (m_iValue == (int)m_vecLabels.size() - 1)
        m_iValue = 0;
      CGUIMessage msg(GUI_MSG_CLICKED, GetID(), GetParentID());
      SendWindowMessage(msg);
      return ;
    }
    break;
  }
}
void CGUISpinControl::SetReverse(bool bReverse)
{
  m_bReverse = bReverse;
}

void CGUISpinControl::SetFloatInterval(float fInterval)
{
  m_fInterval = fInterval;
}

void CGUISpinControl::SetShowRange(bool bOnoff)
{
  m_bShowRange = bOnoff;
}

int CGUISpinControl::GetMinimum() const
{
  switch (m_iType)
  {
  case SPIN_CONTROL_TYPE_PAGE:
    return 0;
  case SPIN_CONTROL_TYPE_INT:
    return m_iStart;
    break;

  case SPIN_CONTROL_TYPE_TEXT:
    return 1;
    break;

  case SPIN_CONTROL_TYPE_FLOAT:
    return (int)(m_fStart*10.0f);
    break;
  }
  return 0;
}

int CGUISpinControl::GetMaximum() const
{
  switch (m_iType)
  {
  case SPIN_CONTROL_TYPE_PAGE:
    return m_numItems;
  case SPIN_CONTROL_TYPE_INT:
    return m_iEnd;
    break;

  case SPIN_CONTROL_TYPE_TEXT:
    return (int)m_vecLabels.size();
    break;

  case SPIN_CONTROL_TYPE_FLOAT:
    return (int)(m_fEnd*10.0f);
    break;
  }
  return 100;
}

bool CGUISpinControl::HitTest(const CPoint &point) const
{
  if (m_imgspinUpFocus.HitTest(point) || m_imgspinDownFocus.HitTest(point))
    return true;
  return CGUIControl::HitTest(point);
}

bool CGUISpinControl::OnMouseOver(const CPoint &point)
{
  int select = m_iSelect;
  if (m_imgspinDownFocus.HitTest(point))
    m_iSelect = SPIN_BUTTON_DOWN;
  else
    m_iSelect = SPIN_BUTTON_UP;

  if (select != m_iSelect)
    MarkDirtyRegion();

  return CGUIControl::OnMouseOver(point);
}

EVENT_RESULT CGUISpinControl::OnMouseEvent(const CPoint &point, const CMouseEvent &event)
{
  if (event.m_id == ACTION_MOUSE_LEFT_CLICK)
  {
    if (m_imgspinUpFocus.HitTest(point))
      MoveUp();
    else if (m_imgspinDownFocus.HitTest(point))
      MoveDown();
    return EVENT_RESULT_HANDLED;
  }
  else if (event.m_id == ACTION_MOUSE_WHEEL_UP)
  {
    MoveUp();
    return EVENT_RESULT_HANDLED;
  }
  else if (event.m_id == ACTION_MOUSE_WHEEL_DOWN)
  {
    MoveDown();
    return EVENT_RESULT_HANDLED;
  }
  return EVENT_RESULT_UNHANDLED;
}

CStdString CGUISpinControl::GetDescription() const
{
  CStdString strLabel;
  strLabel.Format("%i/%i", 1 + GetValue(), GetMaximum());
  return strLabel;
}

bool CGUISpinControl::IsFocusedOnUp() const
{
  return (m_iSelect == SPIN_BUTTON_UP);
}

void CGUISpinControl::ChangePage(int amount)
{
  m_currentItem += amount * m_itemsPerPage;
  if (m_currentItem > m_numItems - m_itemsPerPage)
    m_currentItem = m_numItems - m_itemsPerPage;
  if (m_currentItem < 0)
    m_currentItem = 0;
  CGUIMessage message(GUI_MSG_NOTIFY_ALL, GetParentID(), GetID(), GUI_MSG_PAGE_CHANGE, m_currentItem);
  SendWindowMessage(message);
}

bool CGUISpinControl::UpdateColors()
{
  bool changed = CGUIControl::UpdateColors();
  changed |= m_label.UpdateColors();
  changed |= m_imgspinDownFocus.SetDiffuseColor(m_diffuseColor);
  changed |= m_imgspinDown.SetDiffuseColor(m_diffuseColor);
  changed |= m_imgspinUp.SetDiffuseColor(m_diffuseColor);
  changed |= m_imgspinUpFocus.SetDiffuseColor(m_diffuseColor);

  return changed;
}

bool CGUISpinControl::IsVisible() const
{
  // page controls can be optionally disabled if the number of pages is 1
  if (m_iType == SPIN_CONTROL_TYPE_PAGE && m_numItems <= m_itemsPerPage && !m_showOnePage)
    return false;
  return CGUIControl::IsVisible();
}
