#include "include.h"
#include "GUICheckMarkControl.h"
#include "../xbmc/utils/CharsetConverter.h"
#include "GUIFontManager.h"


CGUICheckMarkControl::CGUICheckMarkControl(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strTextureCheckMark, const CStdString& strTextureCheckMarkNF, DWORD dwCheckWidth, DWORD dwCheckHeight, const CLabelInfo &labelInfo)
    : CGUIControl(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight)
    , m_imgCheckMark(dwParentID, dwControlId, iPosX, iPosY, dwCheckWidth, dwCheckHeight, strTextureCheckMark)
    , m_imgCheckMarkNoFocus(dwParentID, dwControlId, iPosX, iPosY, dwCheckWidth, dwCheckHeight, strTextureCheckMarkNF)
{
  m_strLabel = L"";
  m_label = labelInfo;
  m_bSelected = false;
  ControlType = GUICONTROL_CHECKMARK;
}

CGUICheckMarkControl::~CGUICheckMarkControl(void)
{}

void CGUICheckMarkControl::Render()
{
  if (!UpdateEffectState()) return ;
  int iTextPosX = m_iPosX;
  int iTextPosY = m_iPosY;
  int iCheckMarkPosX = m_iPosX;
  if (m_label.font)
  {
    CStdStringW strLabelUnicode;
    g_charsetConverter.stringCharsetToFontCharset(m_strLabel, strLabelUnicode);

    float fTextHeight, fTextWidth;
    m_label.font->GetTextExtent( strLabelUnicode.c_str(), &fTextWidth, &fTextHeight);
    m_dwWidth = (DWORD)fTextWidth + 5 + m_imgCheckMark.GetWidth();
    m_dwHeight = m_imgCheckMark.GetHeight();

    if (fTextHeight < m_imgCheckMark.GetHeight())
    {
      iTextPosY += (m_imgCheckMark.GetHeight() - (int)fTextHeight) / 2;
    }

    if (!(m_label.align & (XBFONT_RIGHT | XBFONT_CENTER_X)))
    {
      iCheckMarkPosX += ( (DWORD)(fTextWidth) + 5);
    }
    else
    {
      iTextPosX += m_imgCheckMark.GetWidth() + 5;
    }

    if (IsDisabled() )
    {
      m_label.font->DrawText((float)iTextPosX, (float)iTextPosY, m_label.disabledColor, m_label.shadowColor, strLabelUnicode.c_str());
    }
    else
    {
      if (HasFocus())
      {
        m_label.font->DrawText((float)iTextPosX, (float)iTextPosY, m_label.textColor, m_label.shadowColor, strLabelUnicode.c_str());
      }
      else
      {
        m_label.font->DrawText((float)iTextPosX, (float)iTextPosY, m_label.disabledColor, m_label.shadowColor, strLabelUnicode.c_str());
      }
    }
  }
  if (m_bSelected)
  {
    m_imgCheckMark.SetPosition(iCheckMarkPosX, m_iPosY);
    m_imgCheckMark.Render();
  }
  else
  {
    m_imgCheckMarkNoFocus.SetPosition(iCheckMarkPosX, m_iPosY);
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
    g_graphicsContext.SendMessage(msg);
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

void CGUICheckMarkControl::OnMouseClick(DWORD dwButton)
{
  if (dwButton != MOUSE_LEFT_BUTTON) return ;
  g_Mouse.SetState(MOUSE_STATE_CLICK);
  CAction action;
  action.wID = ACTION_SELECT_ITEM;
  OnAction(action);
}

void CGUICheckMarkControl::SetText(const CStdString &aLabel)
{
  WCHAR wszText[1024];
  swprintf(wszText, L"%S", aLabel.c_str());
  m_strLabel = wszText;
}

void CGUICheckMarkControl::SetText(const wstring &label)
{
  m_strLabel = label;
}

void CGUICheckMarkControl::PythonSetLabel(const CStdString &strFont, const wstring &strText, DWORD dwTextColor)
{
  m_label.font = g_fontManager.GetFont(strFont);
  m_label.textColor = dwTextColor;
  m_strLabel = strText;
}

void CGUICheckMarkControl::PythonSetDisabledColor(DWORD dwDisabledColor)
{
  m_label.disabledColor = dwDisabledColor;
}
