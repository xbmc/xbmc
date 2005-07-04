#include "stdafx.h"
#include "GUIButtonControl.h"
#include "GUIFontManager.h"
#include "GUIWindowManager.h"
#include "GUIDialog.h"
#include "../xbmc/utils/CharsetConverter.h"

CGUIButtonControl::CGUIButtonControl(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strTextureFocus, const CStdString& strTextureNoFocus, DWORD dwTextXOffset, DWORD dwTextYOffset, DWORD dwAlign)
    : CGUIControl(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight)
    , m_imgFocus(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight, strTextureFocus)
    , m_imgNoFocus(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight, strTextureNoFocus)
{
  m_bSelected = false;
  m_bTabButton = false;
  m_dwFocusCounter = 0;
  m_dwFlickerCounter = 0;
  m_dwFrameCounter = 0;
  m_strLabel = L"";
  m_strLabel2 = L"";
  m_dwTextColor = 0xFFFFFFFF;
  m_dwDisabledColor = 0xFF606060;
  m_dwTextOffsetX = dwTextXOffset;
  m_dwTextOffsetY = dwTextYOffset;
  m_dwTextAlignment = dwAlign;
  m_pFont = NULL;
  m_lHyperLinkWindowID = WINDOW_INVALID;
  m_strExecuteAction = "";
  m_bPulsing = true;
  ControlType = GUICONTROL_BUTTON;
}

CGUIButtonControl::~CGUIButtonControl(void)
{}

void CGUIButtonControl::SetDisabledColor(D3DCOLOR color)
{
  m_dwDisabledColor = color;
}

void CGUIButtonControl::Render()
{
  if (!UpdateVisibility())
  {
    return ;
  }

  m_dwFrameCounter++;

  if (m_bTabButton)
  {
    m_imgFocus.SetVisible(m_bSelected);
    m_imgNoFocus.SetVisible(!m_bSelected);
  }
  else if (HasFocus())
  {
    if (m_pulseOnSelect)
    {
      DWORD dwAlphaCounter = m_dwFocusCounter + 2;
      DWORD dwAlphaChannel;
      if ((dwAlphaCounter % 128) >= 64)
        dwAlphaChannel = dwAlphaCounter % 64;
      else
        dwAlphaChannel = 63 - (dwAlphaCounter % 64);

      dwAlphaChannel += 192;
      dwAlphaChannel = DWORD((float)m_dwAlpha * (float)dwAlphaChannel / 255.0f);
      m_imgFocus.SetAlpha(dwAlphaChannel);
    }
    m_imgFocus.SetVisible(true);
    m_imgNoFocus.SetVisible(false);
    m_dwFocusCounter++;
  }
  else
  {
    m_imgFocus.SetVisible(false);
    m_imgNoFocus.SetVisible(true);
  }
  // render both so the visibility settings cause the frame counter to resetcorrectly
  m_imgFocus.Render();
  m_imgNoFocus.Render();

  if (!m_pFont) return ;

  // if we're flickering then we may not need to render text
  bool bRenderText = (m_dwFlickerCounter > 0) ? (m_dwFrameCounter % 60 > 30) : true;
  m_dwFlickerCounter = (m_dwFlickerCounter > 0) ? (m_dwFlickerCounter - 1) : 0;

  if (m_strLabel.size() > 0 && bRenderText)
  {
    float fPosX = (float)m_iPosX + m_dwTextOffsetX;
    float fPosY = (float)m_iPosY + m_dwTextOffsetY;

    if (m_dwTextAlignment & XBFONT_RIGHT)
      fPosX = (float)m_iPosX + m_dwWidth - m_dwTextOffsetX;

    if (m_dwTextAlignment & XBFONT_CENTER_X)
      fPosX = (float)m_iPosX + m_dwWidth / 2;

    if (m_dwTextAlignment & XBFONT_CENTER_Y)
      fPosY = (float)m_iPosY + m_dwHeight / 2;

    CStdStringW strLabelUnicode;
    g_charsetConverter.stringCharsetToFontCharset(m_strLabel, strLabelUnicode);

    m_pFont->Begin();
    if (IsDisabled())
      m_pFont->DrawText( fPosX, fPosY, m_dwDisabledColor, strLabelUnicode.c_str(), m_dwTextAlignment);
    else
      m_pFont->DrawText( fPosX, fPosY, m_dwTextColor, strLabelUnicode.c_str(), m_dwTextAlignment);

    // render the second label if it exists
    if (m_strLabel2.size() > 0)
    {
      fPosX = (float)m_iPosX + m_dwWidth - m_dwTextOffsetX;
      DWORD dwAlign = XBFONT_RIGHT | (m_dwTextAlignment & XBFONT_CENTER_Y);

      g_charsetConverter.stringCharsetToFontCharset(m_strLabel2, strLabelUnicode);

      if (IsDisabled() )
        m_pFont->DrawText( fPosX, fPosY, m_dwDisabledColor, strLabelUnicode.c_str(), dwAlign);
      else
        m_pFont->DrawText( fPosX, fPosY, m_dwTextColor, strLabelUnicode.c_str(), dwAlign);
    }
    m_pFont->End();
  }

}

bool CGUIButtonControl::OnAction(const CAction &action)
{
  if (action.wID == ACTION_SELECT_ITEM)
  {
    // Save values, SEND_CLICK_MESSAGE may deactivate the window
    long lHyperLinkWindowID = m_lHyperLinkWindowID;
    CStdString strExecuteAction = m_strExecuteAction;

    // button selected, send a message
    SEND_CLICK_MESSAGE(GetID(), GetParentID(), 0);

    if (strExecuteAction.length() > 0)
    {
      CGUIMessage message(GUI_MSG_EXECUTE, GetID(), GetParentID());
      message.SetStringParam(strExecuteAction);
      g_graphicsContext.SendMessage(message);
      return true;
    }

    if (lHyperLinkWindowID != WINDOW_INVALID)
    {
      CGUIWindow *pWindow = m_gWindowManager.GetWindow(lHyperLinkWindowID);
      if (pWindow && pWindow->IsDialog())
      {
        CGUIDialog *pDialog = (CGUIDialog *)pWindow;
        pDialog->DoModal(m_gWindowManager.GetActiveWindow());
      }
      else
      {
        m_gWindowManager.ActivateWindow(lHyperLinkWindowID);
      }
    }
    return true;
  }
  return CGUIControl::OnAction(action);
}

bool CGUIButtonControl::OnMessage(CGUIMessage& message)
{
  if (message.GetControlId() == GetID())
  {
    if (message.GetMessage() == GUI_MSG_LABEL_SET)
    {
      m_strLabel = message.GetLabel() ;
      return true;
    }
  }

  return CGUIControl::OnMessage(message);
}

void CGUIButtonControl::PreAllocResources()
{
  CGUIControl::PreAllocResources();
  m_imgFocus.PreAllocResources();
  m_imgNoFocus.PreAllocResources();
}

void CGUIButtonControl::AllocResources()
{
  CGUIControl::AllocResources();
  m_dwFocusCounter = 0;
  m_imgFocus.AllocResources();
  m_imgNoFocus.AllocResources();
  m_dwWidth = m_imgFocus.GetWidth();
  m_dwHeight = m_imgFocus.GetHeight();
}

void CGUIButtonControl::FreeResources()
{
  CGUIControl::FreeResources();
  m_imgFocus.FreeResources();
  m_imgNoFocus.FreeResources();
}

void CGUIButtonControl::DynamicResourceAlloc(bool bOnOff)
{
  CGUIControl::DynamicResourceAlloc(bOnOff);
  m_imgFocus.DynamicResourceAlloc(bOnOff);
  m_imgNoFocus.DynamicResourceAlloc(bOnOff);
}

void CGUIButtonControl::SetText(const CStdString &aLabel)
{
  WCHAR wszText[1024];
  swprintf(wszText, L"%S", aLabel.c_str());
  m_strLabel = wszText;
}

void CGUIButtonControl::SetText(const wstring &label)
{
  m_strLabel = label;
}

void CGUIButtonControl::SetText2(const CStdString &aLabel2)
{
  WCHAR wszText[1024];
  swprintf(wszText, L"%S", aLabel2.c_str());
  m_strLabel2 = wszText;
}

void CGUIButtonControl::SetText2(const wstring &label2)
{
  m_strLabel2 = label2;
}

void CGUIButtonControl::SetLabel(const CStdString& strFontName, const CStdString& strLabel, D3DCOLOR dwColor)
{
  WCHAR wszText[1024];
  swprintf(wszText, L"%S", strLabel.c_str());
  m_strLabel = wszText;
  m_dwTextColor = dwColor;
  m_pFont = g_fontManager.GetFont(strFontName);
}

void CGUIButtonControl::SetLabel(const CStdString& strFontName, const wstring& strLabel, D3DCOLOR dwColor)
{
  m_strLabel = strLabel;
  m_dwTextColor = dwColor;
  m_pFont = g_fontManager.GetFont(strFontName);
}


void CGUIButtonControl::Update()
{
  CGUIControl::Update();

  m_imgFocus.SetWidth(m_dwWidth);
  m_imgFocus.SetHeight(m_dwHeight);

  m_imgNoFocus.SetWidth(m_dwWidth);
  m_imgNoFocus.SetHeight(m_dwHeight);
}

void CGUIButtonControl::SetPosition(int iPosX, int iPosY)
{
  CGUIControl::SetPosition(iPosX, iPosY);

  m_imgFocus.SetPosition(iPosX, iPosY);
  m_imgNoFocus.SetPosition(iPosX, iPosY);
}
void CGUIButtonControl::SetAlpha(DWORD dwAlpha)
{
  CGUIControl::SetAlpha(dwAlpha);

  m_imgFocus.SetAlpha(dwAlpha);
  m_imgNoFocus.SetAlpha(dwAlpha);
}

void CGUIButtonControl::SetColourDiffuse(D3DCOLOR colour)
{
  CGUIControl::SetColourDiffuse(colour);

  m_imgFocus.SetColourDiffuse(colour);
  m_imgNoFocus.SetColourDiffuse(colour);
}


void CGUIButtonControl::SetHyperLink(long dwWindowID)
{
  m_lHyperLinkWindowID = dwWindowID;
}

void CGUIButtonControl::SetExecuteAction(const CStdString& strExecuteAction)
{
  m_strExecuteAction = strExecuteAction;
}

void CGUIButtonControl::OnMouseClick(DWORD dwButton)
{
  if (dwButton == MOUSE_LEFT_BUTTON)
  {
    g_Mouse.SetState(MOUSE_STATE_CLICK);
    CAction action;
    action.wID = ACTION_SELECT_ITEM;
    OnAction(action);
  }
}

void CGUIButtonControl::Flicker(bool bFlicker)
{
  m_dwFlickerCounter = bFlicker ? 240 : 0;
}
