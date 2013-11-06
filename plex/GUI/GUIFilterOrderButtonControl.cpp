#include "GUIFilterOrderButtonControl.h"


///////////////////////////////////////////////////////////////////////////////////////////////////
CGUIFilterOrderButtonControl::CGUIFilterOrderButtonControl(int parentID, int controlID,
                                                           float posX, float posY,
                                                           float width, float height,
                                                           const CTextureInfo &textureFocus, const CTextureInfo &textureNoFocus,
                                                           const CLabelInfo &labelInfo,
                                                           const CTextureInfo &off, const CTextureInfo &ascending, const CTextureInfo &descending)
  : CGUIButtonControl(parentID, controlID, posX, posY, width, height, textureFocus, textureNoFocus, labelInfo),
    m_off(posX, posY, 16, 16, off),
    m_ascending(posX, posY, 16, 16, ascending),
    m_descending(posX, posY, 16, 16, descending)
{
  m_state = OFF;
  ControlType = GUICONTROL_FILTERORDER;
  m_off.SetAspectRatio(CAspectRatio::AR_KEEP);
  m_ascending.SetAspectRatio(CAspectRatio::AR_KEEP);
  m_descending.SetAspectRatio(CAspectRatio::AR_KEEP);
  m_startState = DESCENDING;

  m_radioPosX = 0.0;
  m_radioPosY = 0.0;
}

void CGUIFilterOrderButtonControl::SetTristate(CGUIFilterOrderButtonControl::FilterOrderButtonState state)
{
  m_state = state;
  MarkDirtyRegion();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIFilterOrderButtonControl::Render()
{
  CGUIButtonControl::Render();

  if (IsDisabled() || m_state == OFF)
    m_off.Render();
  else if (m_state == ASCENDING)
    m_ascending.Render();
  else if (m_state == DESCENDING)
    m_descending.Render();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIFilterOrderButtonControl::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  m_off.Process(currentTime);
  m_ascending.Process(currentTime);
  m_descending.Process(currentTime);

  CGUIButtonControl::Process(currentTime, dirtyregions);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIFilterOrderButtonControl::AllocResources()
{
  CGUIButtonControl::AllocResources();
  m_off.AllocResources();
  m_ascending.AllocResources();
  m_descending.AllocResources();

  SetPosition(m_posX, m_posY);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIFilterOrderButtonControl::FreeResources(bool immediately)
{
  CGUIButtonControl::FreeResources(immediately);
  m_off.FreeResources(immediately);
  m_ascending.FreeResources(immediately);
  m_descending.FreeResources(immediately);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIFilterOrderButtonControl::DynamicResourceAlloc(bool bOnOff)
{
  CGUIControl::DynamicResourceAlloc(bOnOff);
  m_off.DynamicResourceAlloc(bOnOff);
  m_ascending.DynamicResourceAlloc(bOnOff);
  m_descending.DynamicResourceAlloc(bOnOff);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIFilterOrderButtonControl::SetInvalid()
{
  CGUIButtonControl::SetInvalid();
  m_off.SetInvalid();
  m_ascending.SetInvalid();
  m_descending.SetInvalid();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIFilterOrderButtonControl::SetPosition(float posX, float posY)
{
  CGUIButtonControl::SetPosition(posX, posY);
  float radioPosX = m_radioPosX ? m_posX + m_radioPosX : (m_posX + m_width - 8) - m_off.GetWidth();
  float radioPosY = m_radioPosY ? m_posY + m_radioPosY : m_posY + (m_height - m_off.GetHeight()) / 2;
  m_off.SetPosition(radioPosX, radioPosY);
  m_ascending.SetPosition(radioPosX, radioPosY);
  m_descending.SetPosition(radioPosX, radioPosY);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIFilterOrderButtonControl::SetRadioDimensions(float posX, float posY, float width, float height)
{
  m_radioPosX = posX;
  m_radioPosY = posY;
  if (width)
  {
    m_off.SetWidth(width);
    m_ascending.SetWidth(width);
    m_descending.SetWidth(width);
  }
  if (height)
  {
    m_off.SetHeight(height);
    m_ascending.SetHeight(height);
    m_descending.SetHeight(height);
  }
  SetPosition(GetXPosition(), GetYPosition());
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIFilterOrderButtonControl::SetWidth(float width)
{
  CGUIButtonControl::SetWidth(width);
  SetPosition(GetXPosition(), GetYPosition());
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIFilterOrderButtonControl::SetHeight(float height)
{
  CGUIButtonControl::SetHeight(height);
  SetPosition(GetXPosition(), GetYPosition());
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIFilterOrderButtonControl::UpdateColors()
{
  bool changed = CGUIButtonControl::UpdateColors();

  changed |= m_off.SetDiffuseColor(m_diffuseColor);
  changed |= m_ascending.SetDiffuseColor(m_diffuseColor);
  changed |= m_descending.SetDiffuseColor(m_diffuseColor);

  return changed;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIFilterOrderButtonControl::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_SELECT_ITEM)
  {
    if (m_state == OFF)
      m_state = m_startState;
    else
      m_state = m_state == DESCENDING ? ASCENDING : DESCENDING;

    MarkDirtyRegion();
  }
  return CGUIButtonControl::OnAction(action);
}
