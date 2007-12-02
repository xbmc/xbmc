#include "include.h"
#include "GUISpinControl.h"
#include "../xbmc/utils/CharsetConverter.h"

#define SPIN_BUTTON_DOWN 1
#define SPIN_BUTTON_UP   2

CGUISpinControl::CGUISpinControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CImage& textureUp, const CImage& textureDown, const CImage& textureUpFocus, const CImage& textureDownFocus, const CLabelInfo &labelInfo, int iType)
    : CGUIControl(dwParentID, dwControlId, posX, posY, width, height)
    , m_imgspinUp(dwParentID, dwControlId, posX, posY, width, height, textureUp)
    , m_imgspinDown(dwParentID, dwControlId, posX, posY, width, height, textureDown)
    , m_imgspinUpFocus(dwParentID, dwControlId, posX, posY, width, height, textureUpFocus)
    , m_imgspinDownFocus(dwParentID, dwControlId, posX, posY, width, height, textureDownFocus)
    , m_textLayout(labelInfo.font, false)
{
  m_bReverse = false;
  m_iStart = 0;
  m_iEnd = 100;
  m_fStart = 0.0f;
  m_fEnd = 1.0f;
  m_fInterval = 0.1f;
  m_iValue = 0;
  m_label = labelInfo;
  m_label.align |= XBFONT_CENTER_Y;
  m_fValue = 0.0;
  m_iType = iType;
  m_iSelect = SPIN_BUTTON_DOWN;
  m_bShowRange = false;
  m_iTypedPos = 0;
  strcpy(m_szTyped, "");
  ControlType = GUICONTROL_SPIN;
  m_numItems = 10;
  m_itemsPerPage = 10;
  m_showOnePage = true;
}

CGUISpinControl::~CGUISpinControl(void)
{}


bool CGUISpinControl::OnAction(const CAction &action)
{
  switch (action.wID)
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
      int iNumber = action.wID - REMOTE_0;

      m_szTyped[m_iTypedPos] = iNumber + '0';
      m_iTypedPos++;
      m_szTyped[m_iTypedPos] = 0;
      int iValue;
      sscanf(m_szTyped, "%i", &iValue);
      switch (m_iType)
      {
      case SPIN_CONTROL_TYPE_INT:
      case SPIN_CONTROL_TYPE_PAGE:
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
  if (action.wID == ACTION_SCROLL_UP)
  {
    m_fSmoothScrollOffset += action.fAmount1 * action.fAmount1;
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
        // the page number...
        int offset = message.GetParam1() / m_itemsPerPage + 1;
        if ((int)message.GetParam1() >= m_numItems - m_itemsPerPage)
          offset = m_iEnd;
        SetValue(offset);
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
        int numPages = (m_numItems + m_itemsPerPage - 1) / m_itemsPerPage;
        SetRange(1, numPages);
        m_iValue = 1;
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

    }
  }
  return false;
}

void CGUISpinControl::PreAllocResources()
{
  CGUIControl::PreAllocResources();
  m_imgspinUp.PreAllocResources();
  m_imgspinUpFocus.PreAllocResources();
  m_imgspinDown.PreAllocResources();
  m_imgspinDownFocus.PreAllocResources();
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

void CGUISpinControl::FreeResources()
{
  CGUIControl::FreeResources();
  m_imgspinUp.FreeResources();
  m_imgspinUpFocus.FreeResources();
  m_imgspinDown.FreeResources();
  m_imgspinDownFocus.FreeResources();
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

void CGUISpinControl::Render()
{
  if (!HasFocus())
  {
    m_iTypedPos = 0;
    strcpy(m_szTyped, "");
  }

  float posX = m_posX;
  CStdString text;
  CStdStringW strTextUnicode;

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
    text.Format("%i/%i", m_iValue, m_iEnd);
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

  m_textLayout.Update(text);
  // Calculate the size of our text (for use in HitTest)
  float fTextWidth, fTextHeight;
  m_textLayout.GetTextExtent(fTextWidth, fTextHeight);
  // Position the arrows
  if ( !(m_label.align & (XBFONT_RIGHT | XBFONT_CENTER_X)) )
  {
    m_imgspinUpFocus.SetPosition(fTextWidth + 5 + posX + m_imgspinDown.GetWidth(), m_posY);
    m_imgspinUp.SetPosition(fTextWidth + 5 + posX + m_imgspinDown.GetWidth(), m_posY);
    m_imgspinDownFocus.SetPosition(fTextWidth + 5 + posX, m_posY);
    m_imgspinDown.SetPosition(fTextWidth + 5 + posX, m_posY);
  }

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

  if (m_label.font)
  {
    float fPosY;
    if (m_label.align & XBFONT_CENTER_Y)
      fPosY = m_posY + m_height * 0.5f;
    else
      fPosY = m_posY + m_label.offsetY;

    float fPosX = m_posX + m_label.offsetX - 3;
    if ( !IsDisabled() /*HasFocus()*/ )
      m_textLayout.Render(fPosX, fPosY, 0, m_label.textColor, m_label.shadowColor, m_label.align, 0);
    else if (HasFocus() && m_label.focusedColor)
      m_textLayout.Render(fPosX, fPosY, 0, m_label.focusedColor, m_label.shadowColor, m_label.align, 0);
    else
      m_textLayout.Render(fPosX, fPosY, 0, m_label.disabledColor, m_label.shadowColor, m_label.align, 0, true);

    // set our hit rectangle for MouseOver events
    if (!(m_label.align & (XBFONT_RIGHT | XBFONT_CENTER_X)))
      m_hitRect.SetRect(fPosX, fPosY, fPosX + fTextWidth, fPosY + fTextHeight);
    else
      m_hitRect.SetRect(fPosX - fTextWidth, fPosY, fPosX, fPosY + fTextHeight);
  }
  CGUIControl::Render();
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
  case SPIN_CONTROL_TYPE_INT:
  case SPIN_CONTROL_TYPE_PAGE:
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
  case SPIN_CONTROL_TYPE_INT:
  case SPIN_CONTROL_TYPE_PAGE:
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
    {
      if (m_iValue - 10 >= m_iStart)
        m_iValue -= 10;
      else
        m_iValue = m_iStart;
      SendPageChange();
    }
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
    {
      if (m_iValue + 10 <= m_iEnd)
        m_iValue += 10;
      else
        m_iValue = m_iEnd;
      SendPageChange();
    }
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
    {
      if (m_iValue - 1 >= m_iStart)
        m_iValue--;
      else if (m_iValue == m_iStart)
        m_iValue = m_iEnd;
      SendPageChange();
      return ;
    }
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
    {
      if (m_iValue + 1 <= m_iEnd)
        m_iValue++;
      else if (m_iValue == m_iEnd)
        m_iValue = m_iStart;
      SendPageChange();
    }
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
  case SPIN_CONTROL_TYPE_INT:
  case SPIN_CONTROL_TYPE_PAGE:
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
  case SPIN_CONTROL_TYPE_INT:
  case SPIN_CONTROL_TYPE_PAGE:
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
  if (m_imgspinUpFocus.HitTest(point))
  {
    CGUIControl::OnMouseOver(point);
    m_iSelect = SPIN_BUTTON_UP;
  }
  else if (m_imgspinDownFocus.HitTest(point))
  {
    CGUIControl::OnMouseOver(point);
    m_iSelect = SPIN_BUTTON_DOWN;
  }
  else
  {
    CGUIControl::OnMouseOver(point);
    m_iSelect = SPIN_BUTTON_UP;
  }
  return true;
}

bool CGUISpinControl::OnMouseClick(DWORD dwButton, const CPoint &point)
{ // only left button handled
  if (dwButton != MOUSE_LEFT_BUTTON) return false;
  if (m_imgspinUpFocus.HitTest(point))
  {
    MoveUp();
  }
  if (m_imgspinDownFocus.HitTest(point))
  {
    MoveDown();
  }
  return true;
}

bool CGUISpinControl::OnMouseWheel(char wheel, const CPoint &point)
{
  for (int i = 0; i < abs(wheel); i++)
  {
    if (wheel > 0)
    {
      MoveUp();
    }
    else
    {
      MoveDown();
    }
  }
  return true;
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

void CGUISpinControl::SendPageChange()
{
  CGUIMessage message(GUI_MSG_NOTIFY_ALL, GetParentID(), GetID(), GUI_MSG_PAGE_CHANGE, (m_iValue - 1) * m_itemsPerPage);
  SendWindowMessage(message);
}

void CGUISpinControl::SetColorDiffuse(D3DCOLOR color)
{
  CGUIControl::SetColorDiffuse(color);
  m_imgspinDownFocus.SetColorDiffuse(color);
  m_imgspinDown.SetColorDiffuse(color);
  m_imgspinUp.SetColorDiffuse(color);
  m_imgspinUpFocus.SetColorDiffuse(color);
}

bool CGUISpinControl::IsVisible() const
{
  // page controls can be optionally disabled if the number of pages is 1
  if (m_iType == SPIN_CONTROL_TYPE_PAGE && m_numItems <= m_itemsPerPage && !m_showOnePage)
    return false;
  return CGUIControl::IsVisible();
}
