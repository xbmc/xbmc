#include "include.h"
#include "GUISliderControl.h"
#include "GUIFontManager.h"
#include "../xbmc/utils/GUIInfoManager.h"


CGUISliderControl::CGUISliderControl(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strBackGroundTexture, const CStdString& strMidTexture, const CStdString& strMidTextureFocus, int iType)
    : CGUIControl(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight)
    , m_guiBackground(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight, strBackGroundTexture)
    , m_guiMid(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight, strMidTexture)
    , m_guiMidFocus(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight, strMidTextureFocus)
{
  m_iType = iType;
  m_iPercent = 0;
  m_iStart = 0;
  m_iEnd = 100;
  m_fStart = 0.0f;
  m_fEnd = 1.0f;
  m_fInterval = 0.1f;
  m_iValue = 0;
  m_fValue = 0.0;
  m_iControlOffsetX = 60;
  m_iControlOffsetY = 0;
  ControlType = GUICONTROL_SLIDER;
  m_iInfoCode = 0;
}

CGUISliderControl::~CGUISliderControl(void)
{}


void CGUISliderControl::Render()
{
  CGUIFont* pFont13 = g_fontManager.GetFont("font13");
  float fRange, fPos, fPercent;

  if (!UpdateFadeState()) return ;
  if (IsDisabled()) return ;

  switch (m_iType)
  {
  case SPIN_CONTROL_TYPE_FLOAT:
    if(m_iInfoCode) m_fValue = (float)g_infoManager.GetInt(m_iInfoCode);

    if (m_renderText && pFont13)
    {
      WCHAR strValue[128];
      swprintf(strValue, L"%2.2f", m_fValue);
      pFont13->DrawShadowText( (float)m_iPosX, (float)m_iPosY, 0xffffffff,
                               strValue, 0,
                               0,
                               2,
                               2,
                               0xFF020202);
    }
    m_guiBackground.SetPosition(m_iPosX + m_iControlOffsetX, m_iPosY + m_iControlOffsetY);

    fRange = (float)(m_fEnd - m_fStart);
    fPos = (float)(m_fValue - m_fStart);
    fPercent = (fPos / fRange) * 100.0f;
    m_iPercent = (int) fPercent;
    break;

  case SPIN_CONTROL_TYPE_INT:
    if(m_iInfoCode) m_iValue = g_infoManager.GetInt(m_iInfoCode);

    if (m_renderText && pFont13)
    {
      WCHAR strValue[128];
      swprintf(strValue, L"%i/%i", m_iValue, m_iEnd);
      pFont13->DrawShadowText( (float)m_iPosX, (float)m_iPosY, 0xffffffff,
                               strValue, 0,
                               0,
                               2,
                               2,
                               0xFF020202);
    }
    m_guiBackground.SetPosition(m_iPosX + m_iControlOffsetX, m_iPosY + m_iControlOffsetY);

    fRange = (float)(m_iEnd - m_iStart);
    fPos = (float)(m_iValue - m_iStart);
    m_iPercent = (int) ((fPos / fRange) * 100.0f);
    break;
  default:
    if(m_iInfoCode) m_iPercent = g_infoManager.GetInt(m_iInfoCode);
  }

  float fScaleX, fScaleY;
  fScaleY = m_dwHeight == 0 ? 1.0f : m_dwHeight/(float)m_guiBackground.GetTextureHeight();
  fScaleX = m_dwWidth == 0 ? 1.0f : m_dwWidth/(float)m_guiBackground.GetTextureWidth();

  m_guiBackground.SetHeight(m_dwHeight);
  m_guiBackground.SetWidth(m_dwWidth);
  m_guiBackground.Render();

  float fWidth = (float)(m_guiBackground.GetTextureWidth() - m_guiMid.GetTextureWidth())*fScaleX;

  fPos = (float)m_iPercent;
  fPos /= 100.0f;
  fPos *= fWidth;
  fPos += (float) m_guiBackground.GetXPosition();
  //fPos += 10.0f;
  if ((int)fWidth > 1)
  {
    if (m_bHasFocus)
    {
      m_guiMidFocus.SetPosition((int)fPos, m_guiBackground.GetYPosition() );
      m_guiMidFocus.SetWidth((int)(m_guiMidFocus.GetTextureWidth()*fScaleX));
      m_guiMidFocus.SetHeight((int)(m_guiMidFocus.GetTextureHeight()*fScaleY));
      m_guiMidFocus.Render();
    }
    else
    {
      m_guiMid.SetPosition((int)fPos, m_guiBackground.GetYPosition() );
      m_guiMid.SetWidth((int)(m_guiMid.GetTextureWidth()*fScaleX));
      m_guiMid.SetHeight((int)(m_guiMid.GetTextureHeight()*fScaleY));
      m_guiMid.Render();
    }
  }
}

/*
bool CGUISliderControl::CanFocus() const
{
 //return false;
 return true;
}
*/

bool CGUISliderControl::OnMessage(CGUIMessage& message)
{
  if (message.GetControlId() == GetID() )
  {
    switch (message.GetMessage())
    {
    case GUI_MSG_ITEM_SELECT:
      SetPercentage( message.GetParam1() );
      return true;
      break;

    case GUI_MSG_LABEL_RESET:
      {
        SetPercentage(0);
        return true;
      }
      break;
    }
  }

  return CGUIControl::OnMessage(message);
}

bool CGUISliderControl::OnAction(const CAction &action)
{
  switch ( action.wID )
  {
  case ACTION_MOVE_LEFT:
    //case ACTION_OSD_SHOW_VALUE_MIN:
    Move( -1);
    return true;
    break;

  case ACTION_MOVE_RIGHT:
    //case ACTION_OSD_SHOW_VALUE_PLUS:
    Move(1);
    return true;
    break;
  default:
    return CGUIControl::OnAction(action);
  }
}

void CGUISliderControl::Move(int iNumSteps)
{
  switch (m_iType)
  {
  case SPIN_CONTROL_TYPE_FLOAT:
    m_fValue += m_fInterval * iNumSteps;
    if (m_fValue < m_fStart) m_fValue = m_fStart;
    if (m_fValue > m_fEnd) m_fValue = m_fEnd;
    break;

  case SPIN_CONTROL_TYPE_INT:
    m_iValue += iNumSteps;
    if (m_iValue < m_iStart) m_iValue = m_iStart;
    if (m_iValue > m_iEnd) m_iValue = m_iEnd;
    break;

  default:
    m_iPercent += iNumSteps;
    if (m_iPercent < 0) m_iPercent = 0;
    if (m_iPercent > 100) m_iPercent = 100;
    break;
  }
  SEND_CLICK_MESSAGE(GetID(), GetParentID(), 0);
}

void CGUISliderControl::SetPercentage(int iPercent)
{
  if (iPercent > 100) iPercent = 100;
  if (iPercent < 0) iPercent = 0;
  m_iPercent = iPercent;
}

int CGUISliderControl::GetPercentage() const
{
  return m_iPercent;
}

void CGUISliderControl::SetIntValue(int iValue)
{
  if (m_iType == SPIN_CONTROL_TYPE_FLOAT)
    m_fValue = (float)iValue;
  else if (m_iType == SPIN_CONTROL_TYPE_INT)
    m_iValue = iValue;
  else
    SetPercentage(iValue);
}

int CGUISliderControl::GetIntValue() const
{
  if (m_iType == SPIN_CONTROL_TYPE_FLOAT)
    return (int)m_fValue;
  else if (m_iType == SPIN_CONTROL_TYPE_INT)
    return m_iValue;
  else
    return m_iPercent;
}

void CGUISliderControl::SetFloatValue(float fValue)
{
  if (m_iType == SPIN_CONTROL_TYPE_FLOAT)
    m_fValue = fValue;
  else if (m_iType == SPIN_CONTROL_TYPE_INT)
    m_iValue = (int)fValue;
  else
    SetPercentage((int)fValue);
}

float CGUISliderControl::GetFloatValue() const
{
  if (m_iType == SPIN_CONTROL_TYPE_FLOAT)
    return m_fValue;
  else if (m_iType == SPIN_CONTROL_TYPE_INT)
    return (float)m_iValue;
  else
    return (float)m_iPercent;
}

void CGUISliderControl::SetFloatInterval(float fInterval)
{
  m_fInterval = fInterval;
}

void CGUISliderControl::SetRange(int iStart, int iEnd)
{
  if (m_iType == SPIN_CONTROL_TYPE_FLOAT)
    SetFloatRange((float)iStart,(float)iEnd);
  else
  {
    m_iStart = iStart;
    m_iEnd = iEnd;
  }
}

void CGUISliderControl::SetFloatRange(float fStart, float fEnd)
{
  if (m_iType == SPIN_CONTROL_TYPE_INT)
    SetRange((int)fStart, (int)fEnd);
  else
  {
    m_fStart = fStart;
    m_fEnd = fEnd;
  }
}

int CGUISliderControl::GetType() const
{
  return m_iType;
}

void CGUISliderControl::FreeResources()
{
  CGUIControl::FreeResources();
  m_guiBackground.FreeResources();
  m_guiMid.FreeResources();
  m_guiMidFocus.FreeResources();
}

void CGUISliderControl::DynamicResourceAlloc(bool bOnOff)
{
  CGUIControl::DynamicResourceAlloc(bOnOff);
  m_guiBackground.DynamicResourceAlloc(bOnOff);
  m_guiMid.DynamicResourceAlloc(bOnOff);
  m_guiMidFocus.DynamicResourceAlloc(bOnOff);
}

void CGUISliderControl::PreAllocResources()
{
  CGUIControl::PreAllocResources();
  m_guiBackground.PreAllocResources();
  m_guiMid.PreAllocResources();
  m_guiMidFocus.PreAllocResources();
}

void CGUISliderControl::AllocResources()
{
  CGUIControl::AllocResources();
  m_guiBackground.AllocResources();
  m_guiMid.AllocResources();
  m_guiMidFocus.AllocResources();
}

void CGUISliderControl::Update()
{
  m_guiBackground.SetPosition( GetXPosition(), GetYPosition());
}

bool CGUISliderControl::HitTest(int iPosX, int iPosY) const
{
  if (m_guiBackground.HitTest(iPosX, iPosY)) return true;
  if (m_guiMid.HitTest(iPosX, iPosY)) return true;
  return false;
}

void CGUISliderControl::SetFromPosition(int iPosX, int iPosY)
{
  float fPercent = (float)(g_Mouse.iPosX - m_guiBackground.GetXPosition()) / ((float)m_guiBackground.GetWidth());
  if (fPercent < 0) fPercent = 0;
  if (fPercent > 1) fPercent = 1;
  switch (m_iType)
  {
  case SPIN_CONTROL_TYPE_FLOAT:
    m_fValue = m_fStart + (m_fEnd - m_fStart) * fPercent;
    break;

  case SPIN_CONTROL_TYPE_INT:
    m_iValue = (int)(m_iStart + (float)(m_iEnd - m_iStart) * fPercent + 0.49f);
    break;

  default:
    m_iPercent = (int)(fPercent * 100 + 0.49f);
    break;
  }
  SEND_CLICK_MESSAGE(GetID(), GetParentID(), 0);
}

void CGUISliderControl::OnMouseClick(DWORD dwButton)
{
  g_Mouse.SetState(MOUSE_STATE_CLICK);
  // turn off any exclusive access, if it's on...
  g_Mouse.EndExclusiveAccess(GetID(), GetParentID());
  if (m_guiBackground.HitTest(g_Mouse.iPosX, g_Mouse.iPosY))
  { // set the position
    SetFromPosition(g_Mouse.iPosX, g_Mouse.iPosY);
  }
}

void CGUISliderControl::OnMouseDrag()
{
  g_Mouse.SetState(MOUSE_STATE_DRAG);
  // get exclusive access to the mouse
  g_Mouse.SetExclusiveAccess(GetID(), GetParentID());
  // get the position of the mouse
  SetFromPosition(g_Mouse.iPosX, g_Mouse.iPosY);
}

void CGUISliderControl::OnMouseWheel()
{ // move the slider 10 steps in the appropriate direction
  Move(g_Mouse.cWheel*10);
}

void CGUISliderControl::SetInfo(int iInfo)
{
  m_iInfoCode = iInfo;
}

CStdString CGUISliderControl::GetDescription() const
{
  CStdString description;
  if (m_iType == SPIN_CONTROL_TYPE_FLOAT)
    description.Format("%2.2f", m_fValue);
  else if (m_iType == SPIN_CONTROL_TYPE_INT)
    description.Format("%i", m_iValue);
  else
    description.Format("%i%%", m_iPercent);
  return description;
}