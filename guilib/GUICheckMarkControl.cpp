#include "include.h"
#include "GUICheckMarkControl.h"
#include "../xbmc/utils/CharsetConverter.h"
#include "GUIFontManager.h"


CGUICheckMarkControl::CGUICheckMarkControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CImage& textureCheckMark, const CImage& textureCheckMarkNF, float checkWidth, float checkHeight, const CLabelInfo &labelInfo)
    : CGUIControl(dwParentID, dwControlId, posX, posY, width, height)
    , m_imgCheckMark(dwParentID, dwControlId, posX, posY, checkWidth, checkHeight, textureCheckMark)
    , m_imgCheckMarkNoFocus(dwParentID, dwControlId, posX, posY, checkWidth, checkHeight, textureCheckMarkNF)
    , m_textLayout(labelInfo.font, false)
{
  m_strLabel = "";
  m_label = labelInfo;
  m_bSelected = false;
  ControlType = GUICONTROL_CHECKMARK;
}

CGUICheckMarkControl::~CGUICheckMarkControl(void)
{}

void CGUICheckMarkControl::Render()
{
  m_textLayout.Update(m_strLabel);

  float fTextHeight, fTextWidth;
  m_textLayout.GetTextExtent(fTextWidth, fTextHeight);
  m_width = (DWORD)fTextWidth + 5 + m_imgCheckMark.GetWidth();
  m_height = m_imgCheckMark.GetHeight();

  float textPosX = m_posX;
  float textPosY = m_posY + m_height * 0.5f;
  float checkMarkPosX = m_posX;

  if (m_label.align & (XBFONT_RIGHT | XBFONT_CENTER_X))
    textPosX += m_imgCheckMark.GetWidth() + 5;
  else
    checkMarkPosX += fTextWidth + 5;

  if (IsDisabled() )
    m_textLayout.Render(textPosX, textPosY, 0, m_label.disabledColor, m_label.shadowColor, XBFONT_CENTER_Y, 0, true);
  else if (HasFocus() && m_label.focusedColor)
    m_textLayout.Render(textPosX, textPosY, 0, m_label.focusedColor, m_label.shadowColor, XBFONT_CENTER_Y, 0);
  else
    m_textLayout.Render(textPosX, textPosY, 0, m_label.textColor, m_label.shadowColor, XBFONT_CENTER_Y, 0);

  if (m_bSelected)
  {
    m_imgCheckMark.SetPosition(checkMarkPosX, m_posY);
    m_imgCheckMark.Render();
  }
  else
  {
    m_imgCheckMarkNoFocus.SetPosition(checkMarkPosX, m_posY);
    m_imgCheckMarkNoFocus.Render();
  }
  CGUIControl::Render();
}

bool CGUICheckMarkControl::OnAction(const CAction &action)
{
  if (action.wID == ACTION_SELECT_ITEM)
  {
    m_bSelected = !m_bSelected;
    CGUIMessage msg(GUI_MSG_CLICKED, GetID(), GetParentID(), action.wID);
    SendWindowMessage(msg);
    return true;
  }
  return CGUIControl::OnAction(action);
}

bool CGUICheckMarkControl::OnMessage(CGUIMessage& message)
{
  if ( message.GetControlId() == GetID() )
  {
    if (message.GetMessage() == GUI_MSG_LABEL_SET)
    {
      m_strLabel = message.GetLabel();
      return true;
    }
  }
  if (CGUIControl::OnMessage(message)) return true;
  return false;
}

void CGUICheckMarkControl::PreAllocResources()
{
  CGUIControl::PreAllocResources();
  m_imgCheckMark.PreAllocResources();
  m_imgCheckMarkNoFocus.PreAllocResources();
}

void CGUICheckMarkControl::AllocResources()
{
  CGUIControl::AllocResources();
  m_imgCheckMark.AllocResources();
  m_imgCheckMarkNoFocus.AllocResources();
}

void CGUICheckMarkControl::FreeResources()
{
  CGUIControl::FreeResources();
  m_imgCheckMark.FreeResources();
  m_imgCheckMarkNoFocus.FreeResources();
}

void CGUICheckMarkControl::DynamicResourceAlloc(bool bOnOff)
{
  CGUIControl::DynamicResourceAlloc(bOnOff);
  m_imgCheckMark.DynamicResourceAlloc(bOnOff);
  m_imgCheckMarkNoFocus.DynamicResourceAlloc(bOnOff);
}

void CGUICheckMarkControl::SetSelected(bool bOnOff)
{
  m_bSelected = bOnOff;
}

bool CGUICheckMarkControl::GetSelected() const
{
  return m_bSelected;
}

bool CGUICheckMarkControl::OnMouseClick(DWORD dwButton, const CPoint &point)
{
  if (dwButton != MOUSE_LEFT_BUTTON) return false;
  g_Mouse.SetState(MOUSE_STATE_CLICK);
  CAction action;
  action.wID = ACTION_SELECT_ITEM;
  OnAction(action);
  return true;
}

void CGUICheckMarkControl::SetLabel(const string &label)
{
  m_strLabel = label;
}

void CGUICheckMarkControl::PythonSetLabel(const CStdString &strFont, const string &strText, DWORD dwTextColor)
{
  m_label.font = g_fontManager.GetFont(strFont);
  m_label.textColor = dwTextColor;
  m_label.focusedColor = dwTextColor;
  m_strLabel = strText;
}

void CGUICheckMarkControl::PythonSetDisabledColor(DWORD dwDisabledColor)
{
  m_label.disabledColor = dwDisabledColor;
}

void CGUICheckMarkControl::SetColorDiffuse(D3DCOLOR color)
{
  CGUIControl::SetColorDiffuse(color);
  m_imgCheckMark.SetColorDiffuse(color);
  m_imgCheckMarkNoFocus.SetColorDiffuse(color);
}
