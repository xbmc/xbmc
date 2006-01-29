
#include "include.h"
#include "GUIButtonScroller.h"
#include "LocalizeStrings.h"
#include "GUIWindowManager.h"
#include "../xbmc/utils/CharsetConverter.h"
#include "../xbmc/util.h"
#ifdef PRE_SKIN_VERSION_2_0_COMPATIBILITY
#include "SkinInfo.h"
#endif

#define SCROLL_SPEED 6.0f

CGUIButtonScroller::CGUIButtonScroller(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, int iGap, int iSlots, int iDefaultSlot, int iMovementRange, bool bHorizontal, int iAlpha, bool bWrapAround, bool bSmoothScrolling, const CStdString& strTextureFocus, const CStdString& strTextureNoFocus, const CLabelInfo& labelInfo)
    : CGUIControl(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight)
    , m_imgFocus(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight, strTextureFocus)
    , m_imgNoFocus(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight, strTextureNoFocus)
{
  m_iXMLNumSlots = iSlots;
  m_iXMLDefaultSlot = iDefaultSlot - 1;
  m_iXMLPosX = iPosX;
  m_iXMLPosY = iPosY;
  m_dwXMLWidth = dwWidth;
  m_dwXMLHeight = dwHeight;
  m_iButtonGap = iGap;
  m_iNumSlots = iSlots;
  m_bHorizontal = bHorizontal;
  m_iDefaultSlot = iDefaultSlot - 1;
  m_iMovementRange = iMovementRange;
  m_iAlpha = iAlpha;
  m_bWrapAround = bWrapAround;
  m_bSmoothScrolling = bSmoothScrolling;
  m_iSlowScrollCount = 0;
  if (!m_bWrapAround)
  { // force the amount of movement to allow for the number of slots
    if (m_iMovementRange < m_iDefaultSlot) m_iMovementRange = m_iDefaultSlot;
    if (m_iMovementRange < m_iNumSlots - 1 - m_iDefaultSlot) m_iMovementRange = m_iNumSlots - 1 - m_iDefaultSlot;
  }
  // reset other variables to the defaults
  m_iCurrentSlot = -1;
  m_iOffset = 0;
  m_iScrollOffset = 0;
  m_bScrollUp = false;
  m_bScrollDown = false;
  m_bMoveUp = false;
  m_bMoveDown = false;
  ControlType = GUICONTROL_BUTTONBAR;
  //  m_dwFrameCounter = 0;
  m_label = labelInfo;
}

CGUIButtonScroller::~CGUIButtonScroller(void)
{}

bool CGUIButtonScroller::OnAction(const CAction &action)
{
  if (action.wID == ACTION_SELECT_ITEM)
  {
    // send the appropriate message to the parent window
    CGUIMessage message(GUI_MSG_EXECUTE, GetID(), GetParentID());
    // find our currently highlighted item
    message.SetStringParam(m_vecButtons[GetActiveButton()]->strExecute.c_str());
    g_graphicsContext.SendMessage(message);
    return true;
  }
  if (action.wID == ACTION_CONTEXT_MENU)
  { // send a click message to our parent
    SEND_CLICK_MESSAGE(GetID(), GetParentID(), action.wID);
    return true;
  }
  return CGUIControl::OnAction(action);
}

bool CGUIButtonScroller::OnMessage(CGUIMessage &message)
{
  if (message.GetMessage() == GUI_MSG_SETFOCUS)
  {
    if (message.GetSenderId() == GetParentID())
    {
      OnChangeFocus();
    }
  }
  return CGUIControl::OnMessage(message);
}

void CGUIButtonScroller::ClearButtons()
{
  // destroy our buttons (if we have them from a previous viewing)
  for (int i = 0; i < (int)m_vecButtons.size(); ++i)
  {
    CButton* pButton = m_vecButtons[i];
    delete pButton;
  }
  m_vecButtons.erase(m_vecButtons.begin(), m_vecButtons.end());
}

void CGUIButtonScroller::LoadButtons(const TiXmlNode *node)
{
#ifdef PRE_SKIN_VERSION_2_0_COMPATIBILITY
  if (g_SkinInfo.GetVersion() < 1.8)
  { // load from xboxmediacenter.xml instead
    for (int i = 0; i < (int)g_settings.m_buttonSettings.m_vecButtons.size(); i++)
    {
      if (g_settings.m_buttonSettings.m_vecButtons[i]->m_dwLabel != -1)
      { // grab the required label from our strings
        AddButton(g_localizeStrings.Get(g_settings.m_buttonSettings.m_vecButtons[i]->m_dwLabel),
                             g_settings.m_buttonSettings.m_vecButtons[i]->m_strExecute,
                             g_settings.m_buttonSettings.m_vecButtons[i]->m_iIcon);
      }
      else
      {
        AddButton(g_settings.m_buttonSettings.m_vecButtons[i]->m_strLabel,
                             g_settings.m_buttonSettings.m_vecButtons[i]->m_strExecute,
                             g_settings.m_buttonSettings.m_vecButtons[i]->m_iIcon);
      }
    }
    SetActiveButton(g_settings.m_buttonSettings.m_iDefaultButton);
    return;
  }
#endif
  // run through and find all <button> tags
  // Format is:
  // <button id="1">
  //    <label>
  //    <execute>
  //    <texturefocus>
  //    <texturenofocus>
  // </button>
  TiXmlNode *buttons = node->FirstChild("buttons");
  if (!buttons) return;
  TiXmlElement *buttonNode = buttons->FirstChildElement("button");
  while (buttonNode)
  {
    CButton *button = new CButton;
    buttonNode->Attribute("id", &button->id);
    TiXmlNode *childNode = buttonNode->FirstChild("label");
    if (childNode && childNode->FirstChild())
    {
      CStdString strLabel = childNode->FirstChild()->Value();
      if (CUtil::IsNaturalNumber(strLabel))
        button->strLabel = g_localizeStrings.Get(atoi(strLabel.c_str()));
      else
      {
        WCHAR label[1024];
        swprintf(label, L"%S", strLabel.c_str());
        button->strLabel = label;
      }
    }
    childNode = buttonNode->FirstChild("execute");
    if (childNode && childNode->FirstChild())
      button->strExecute = childNode->FirstChild()->Value();
    childNode = buttonNode->FirstChild("onclick");
    if (childNode && childNode->FirstChild())
      button->strExecute = childNode->FirstChild()->Value();
    childNode = buttonNode->FirstChild("texturefocus");
    if (childNode && childNode->FirstChild())
      button->imageFocus = new CGUIImage(GetParentID(), GetID(), m_iPosX, m_iPosY, m_dwWidth, m_dwHeight, childNode->FirstChild()->Value());
    childNode = buttonNode->FirstChild("texturenofocus");
    if (childNode && childNode->FirstChild())
      button->imageNoFocus = new CGUIImage(GetParentID(), GetID(), m_iPosX, m_iPosY, m_dwWidth, m_dwHeight, childNode->FirstChild()->Value());
    m_vecButtons.push_back(button);
    buttonNode = buttonNode->NextSiblingElement("button");
  }
}

#ifdef PRE_SKIN_VERSION_2_0_COMPATIBILITY
void CGUIButtonScroller::AddButton(const wstring &strLabel, const CStdString &strExecute, const int iID)
{
  // add a button to our control
  CButton *pButton = new CButton;
  if (pButton)
  {
    pButton->strLabel = strLabel;
    pButton->strExecute = strExecute;
    pButton->id = iID;
    m_vecButtons.push_back(pButton);
  }
  // update the number of filled slots etc.
  if ((int)m_vecButtons.size() < m_iXMLNumSlots)
  {
    m_iNumSlots = m_vecButtons.size();
    m_iDefaultSlot = (int)((float)m_iXMLDefaultSlot / ((float)m_iXMLNumSlots - 1) * ((float)m_iNumSlots - 1));
    Update();
  }
  else
  {
    m_iNumSlots = m_iXMLNumSlots;
    m_iDefaultSlot = m_iXMLDefaultSlot;
    Update();
  }
}
#endif

void CGUIButtonScroller::PreAllocResources()
{
  CGUIControl::PreAllocResources();
  m_imgFocus.PreAllocResources();
  m_imgNoFocus.PreAllocResources();
}

void CGUIButtonScroller::AllocResources()
{
  CGUIControl::AllocResources();
  //  m_dwFrameCounter=0;
  m_imgFocus.AllocResources();
  m_imgNoFocus.AllocResources();
  if (g_SkinInfo.GetVersion() < 1.8)
  {  // grab our button scroller images
    for (unsigned int i = 0; i < m_vecButtons.size(); ++i)
    {
      CButton *button = m_vecButtons[i];
      CGUIImage *image = (CGUIImage *)(m_gWindowManager.GetWindow(GetParentID())->GetControl(button->id + 20));
      if (image) button->imageFocus = new CGUIImage(*image);
      image = (CGUIImage *)(m_gWindowManager.GetWindow(GetParentID())->GetControl(button->id + 40));
      if (image) button->imageNoFocus = new CGUIImage(*image);
    }
  }
  // calculate our correct width and height
  if (m_bHorizontal)
  {
    m_dwXMLWidth = (DWORD)(m_iXMLNumSlots * (m_imgFocus.GetWidth() + m_iButtonGap) - m_iButtonGap);
    m_dwXMLHeight = m_imgFocus.GetHeight();
  }
  else
  {
    m_dwXMLWidth = m_imgFocus.GetWidth();
    m_dwXMLHeight = (DWORD)(m_iXMLNumSlots * (m_imgFocus.GetHeight() + m_iButtonGap) - m_iButtonGap);
  }
  m_dwWidth = m_dwXMLWidth;
  m_dwHeight = m_dwXMLHeight;
  // update the number of filled slots etc.
  if ((int)m_vecButtons.size() < m_iXMLNumSlots)
  {
    m_iNumSlots = m_vecButtons.size();
    m_iDefaultSlot = (int)((float)m_iXMLDefaultSlot / ((float)m_iXMLNumSlots - 1) * ((float)m_iNumSlots - 1));
    Update();
  }
  else
  {
    m_iNumSlots = m_iXMLNumSlots;
    m_iDefaultSlot = m_iXMLDefaultSlot;
    Update();
  }
  SetActiveButton(0);
}

void CGUIButtonScroller::FreeResources()
{
  CGUIControl::FreeResources();
  m_imgFocus.FreeResources();
  m_imgNoFocus.FreeResources();
  ClearButtons();
}

void CGUIButtonScroller::DynamicResourceAlloc(bool bOnOff)
{
  CGUIControl::DynamicResourceAlloc(bOnOff);
  m_imgFocus.DynamicResourceAlloc(bOnOff);
  m_imgNoFocus.DynamicResourceAlloc(bOnOff);
}

void CGUIButtonScroller::Render()
{
	if (!UpdateEffectState())
		return;
  int iPosX = m_iPosX;
  int iPosY = m_iPosY;
  // set our viewport
  int iPosX2 = iPosX;
  int iPosY2 = iPosY;
  g_graphicsContext.SetViewPort( (float)iPosX, (float)iPosY, (float)m_dwWidth, (float)m_dwHeight);
  // if we're scrolling, update our scroll offset
  if (m_bScrollUp || m_bScrollDown)
  {
    int iMaxScroll = m_bHorizontal ? (int)m_imgFocus.GetWidth() : (int)m_imgFocus.GetHeight();
    iMaxScroll += m_iButtonGap;
    m_iScrollOffset += (int)((float)iMaxScroll / m_fScrollSpeed) + 1;
    if (m_iScrollOffset > iMaxScroll || !m_bSmoothScrolling)
    {
      m_iScrollOffset = 0;
      if (m_bScrollUp)
      {
        if (GetNext(m_iOffset) != -1) m_iOffset = GetNext(m_iOffset);
      }
      else
      {
        if (GetPrevious(m_iOffset) != -1) m_iOffset = GetPrevious(m_iOffset);
      }
      // check for wraparound...
      if (!m_bWrapAround)
      {
        if (m_iOffset + m_iNumSlots > (int)m_vecButtons.size())
          m_iOffset = GetPrevious(m_iOffset);
      }
      m_bScrollUp = false;
      m_bScrollDown = false;
      OnChangeFocus();
    }
    else
    {
      if (m_bScrollUp)
      {
        if (m_bHorizontal)
          iPosX -= m_iScrollOffset;
        else
          iPosY -= m_iScrollOffset;
      }
      else
      {
        if (m_bHorizontal)
          iPosX += m_iScrollOffset - iMaxScroll;
        else
          iPosY += m_iScrollOffset - iMaxScroll;
      }
    }
  }
  int iPosX3 = iPosX;
  int iPosY3 = iPosY;
  // ok, now check if we're scrolling down
  int iOffset = m_iOffset;
  if (m_bScrollDown)
  {
    iOffset = GetPrevious(iOffset);
    RenderItem(iPosX, iPosY, iOffset, false);
  }
  // ok, now render the main block
  for (int i = 0; i < m_iNumSlots; i++)
    RenderItem(iPosX, iPosY, iOffset, false);
  // ok, now check if we're scrolling up
  if (m_bScrollUp)
    RenderItem(iPosX, iPosY, iOffset, false);
  // ok, now render the background slot...
  if (HasFocus())
  {
    iPosX = m_iPosX;
    iPosY = m_iPosY;
    // check if we're moving up or down
    if (m_bMoveUp || m_bMoveDown)
    {
      int iMaxScroll = m_bHorizontal ? (int)m_imgFocus.GetWidth() : (int)m_imgFocus.GetHeight();
      iMaxScroll += m_iButtonGap;
      m_iScrollOffset += (int)((float)iMaxScroll / SCROLL_SPEED) + 1;
      if (m_iScrollOffset > iMaxScroll || !m_bSmoothScrolling)
      {
        m_iScrollOffset = 0;
        if (m_bMoveUp)
          m_iCurrentSlot--;
        else
          m_iCurrentSlot++;
        m_bMoveUp = false;
        m_bMoveDown = false;
        OnChangeFocus();
      }
      else
      {
        if (m_bMoveUp)
        {
          if (m_bHorizontal)
            iPosX -= m_iScrollOffset;
          else
            iPosY -= m_iScrollOffset;
        }
        else
        {
          if (m_bHorizontal)
            iPosX += m_iScrollOffset;
          else
            iPosY += m_iScrollOffset;
        }
      }
    }
    if (m_bHorizontal)
      iPosX += m_iCurrentSlot * ((int)m_imgFocus.GetWidth() + m_iButtonGap);
    else
      iPosY += m_iCurrentSlot * ((int)m_imgFocus.GetHeight() + m_iButtonGap);
    // check if we have a skinner-defined icon image
    CGUIImage *pImage = m_vecButtons[GetActiveButton()]->imageFocus;
    if (!pImage) pImage = &m_imgFocus;
    pImage->SetPosition(iPosX, iPosY);
    pImage->SetVisible(true);
    pImage->SetWidth(m_imgFocus.GetWidth());
    pImage->SetHeight(m_imgFocus.GetHeight());
    pImage->Render();
  }
  // Now render the text
  iOffset = m_iOffset;
  iPosX = iPosX3;
  iPosY = iPosY3;
  if (m_bScrollDown)
  {
    iOffset = GetPrevious(iOffset);
    RenderItem(iPosX, iPosY, iOffset, true);
  }
  // ok, now render the main block
  for (int i = 0; i < m_iNumSlots; i++)
    RenderItem(iPosX, iPosY, iOffset, true);
  // ok, now check if we're scrolling up
  if (m_bScrollUp)
    RenderItem(iPosX, iPosY, iOffset, true);

  // reset the viewport
  g_graphicsContext.RestoreViewPort();
  CGUIControl::Render();
}

int CGUIButtonScroller::GetNext(int iCurrent) const
{
  if (iCurrent + 1 >= (int)m_vecButtons.size())
  {
    if (m_bWrapAround)
      return 0;
    else
      return -1;
  }
  else
    return iCurrent + 1;
}

int CGUIButtonScroller::GetPrevious(int iCurrent)
{
  if (iCurrent - 1 < 0)
  {
    if (m_bWrapAround)
      return m_vecButtons.size() - 1;
    else
      return -1;
  }
  else
    return iCurrent -1;
}

int CGUIButtonScroller::GetButton(int iOffset)
{
  return iOffset % ((int)m_vecButtons.size());
}

void CGUIButtonScroller::SetActiveButton(int iButton)
{
  if (iButton >= (int)m_vecButtons.size())
    iButton = 0;
  // set the highlighted button
  m_iCurrentSlot = m_iDefaultSlot;
  // and the appropriate offset
  if (!m_bWrapAround)
  { // check whether there is no wiggle room
    int iMinButton = m_iDefaultSlot - m_iMovementRange;
    if (iMinButton < 0) iMinButton = 0;
    if (iButton < iMinButton)
    {
      m_iOffset = 0;
      m_iCurrentSlot = iMinButton;
      OnChangeFocus();
      return ;
    }
    int iMaxButton = m_iDefaultSlot + m_iMovementRange;
    if (iMaxButton >= m_iNumSlots) iMaxButton = m_iNumSlots - 1;
    if (iButton > iMaxButton)
    {
      m_iOffset = iButton - iMaxButton;
      m_iCurrentSlot = iMaxButton;
      OnChangeFocus();
      return ;
    }
    // now change our current slot so that it all fits nicely
    // lastly, make sure we fill the number of slots that we have (if possible)
    int iNumButtonsToShow = m_vecButtons.size() - iButton + m_iCurrentSlot;
    if (iNumButtonsToShow < m_iNumSlots && iNumButtonsToShow < (int)m_vecButtons.size())
    { // we have empty space - try and fill it up
      while (iNumButtonsToShow < (int)m_vecButtons.size() && m_iCurrentSlot < m_iNumSlots)
      {
        m_iCurrentSlot++;
        iNumButtonsToShow = m_vecButtons.size() - iButton + m_iCurrentSlot;
      }
    }
  }
  m_iOffset = 0;
  for (int i = 0; i < (int)m_vecButtons.size(); i++)
  {
    int iItem = i;
    for (int j = 0; j < m_iCurrentSlot; j++)
      if (GetNext(iItem) != -1) iItem = GetNext(iItem);
    if (iItem == iButton)
    {
      m_iOffset = i;
      break;
    }
  }
  OnChangeFocus();
}

void CGUIButtonScroller::OnUp()
{
  if (m_bHorizontal)
    CGUIControl::OnUp();
  else if (!m_bWrapAround && m_iOffset + m_iCurrentSlot == 0)
  {
    if (m_dwControlUp != GetID())
      CGUIControl::OnUp();  // not wrapping around, and we're up the top + our next control is different
    else
      SetActiveButton((int)m_vecButtons.size() - 1);   // move to the last button in the list
  }
  else
    DoUp();
}

void CGUIButtonScroller::OnDown()
{
  if (m_bHorizontal)
    CGUIControl::OnDown();
  else if (!m_bWrapAround && m_iOffset + m_iCurrentSlot == m_vecButtons.size() - 1)
  {
    if (m_dwControlUp != GetID())
      CGUIControl::OnDown();  // not wrapping around, and we're down the bottom + our next control is different
    else
      SetActiveButton(0);   // move to the first button in the list
  }
  else
    DoDown();
}

void CGUIButtonScroller::OnLeft()
{
  if (!m_bHorizontal)
    CGUIControl::OnLeft();
  else if (!m_bWrapAround && m_iOffset + m_iCurrentSlot == 0 && m_dwControlLeft != GetID())
    CGUIControl::OnLeft();  // not wrapping around, and we're at the left + our next control is different
  else
    DoUp();
}

void CGUIButtonScroller::OnRight()
{
  if (!m_bHorizontal)
    CGUIControl::OnRight();
  else if (!m_bWrapAround && m_iOffset + m_iCurrentSlot == m_vecButtons.size() - 1 && m_dwControlRight != GetID())
    CGUIControl::OnRight();  // not wrapping around, and we're at the right + our next control is different
  else
    DoDown();
}

void CGUIButtonScroller::DoUp()
{
  if (!m_bScrollUp)
  {
    if (m_iCurrentSlot - 1 < m_iDefaultSlot - m_iMovementRange || m_iCurrentSlot - 1 < 0)
    {
      if (m_bScrollDown)
      { // finish scroll for higher speed
        m_bScrollDown = false;
        m_iScrollOffset = 0;
        m_iOffset = GetPrevious(m_iOffset);
        OnChangeFocus();
      }
      else
      {
        m_bScrollDown = true;
        m_fScrollSpeed = SCROLL_SPEED;
      }
    }
    else
    {
      if (m_bMoveUp)
      {
        m_bMoveUp = false;
        m_iScrollOffset = 0;
        m_iCurrentSlot--;
        OnChangeFocus();
      }
      m_bMoveUp = true;
    }
  }
}

void CGUIButtonScroller::DoDown()
{
  if (!m_bScrollDown)
  {
    if (m_iCurrentSlot + 1 > m_iDefaultSlot + m_iMovementRange || m_iCurrentSlot + 1 >= m_iNumSlots)
      if (m_bScrollUp)
      { // finish scroll for higher speed
        m_bScrollUp = false;
        m_iScrollOffset = 0;
        if (GetNext(m_iOffset) != -1) m_iOffset = GetNext(m_iOffset);
        OnChangeFocus();
      }
      else
      {
        m_bScrollUp = true;
        m_fScrollSpeed = SCROLL_SPEED;
      }
    else
    {
      if (m_bMoveDown)
      {
        m_bMoveDown = false;
        m_iScrollOffset = 0;
        m_iCurrentSlot++;
        OnChangeFocus();
      }
      m_bMoveDown = true;
    }
  }
}

void CGUIButtonScroller::RenderItem(int &iPosX, int &iPosY, int &iOffset, bool bText)
{
  if (iOffset < 0) return ;
  float fStartAlpha, fEndAlpha;
  GetScrollZone(fStartAlpha, fEndAlpha);
  if (bText)
  {
    if (!m_label.font) return ;
    float fPosX = (float)iPosX + m_label.offsetX;
    float fPosY = (float)iPosY + m_label.offsetY;
    if (m_label.align & XBFONT_RIGHT)
      fPosX = (float)iPosX + m_imgFocus.GetWidth() - m_label.offsetX;
    if (m_label.align & XBFONT_CENTER_X)
      fPosX = (float)iPosX + m_imgFocus.GetWidth() / 2;
    if (m_label.align & XBFONT_CENTER_Y)
      fPosY = (float)iPosY + m_imgFocus.GetHeight() / 2;

    CStdStringW strLabelUnicode;
    g_charsetConverter.stringCharsetToFontCharset(m_vecButtons[iOffset]->strLabel, strLabelUnicode);

    float fAlpha = 255.0f;
    if (m_bHorizontal)
    {
      if (fPosX < fStartAlpha)
        fAlpha -= (fStartAlpha - fPosX) / (fStartAlpha - m_iPosX) * m_iAlpha * 2.55f;
      if (fPosX > fEndAlpha)
        fAlpha -= (fPosX - fEndAlpha) / (m_iPosX + (int)m_dwWidth - fEndAlpha) * m_iAlpha * 2.55f;
    }
    else
    {
      if (fPosY < fStartAlpha)
        fAlpha -= (fStartAlpha - fPosY) / (fStartAlpha - m_iPosY) * m_iAlpha * 2.55f;
      if (fPosY > fEndAlpha)
        fAlpha -= (fPosY - fEndAlpha) / (m_iPosY + (int)m_dwHeight - fEndAlpha) * m_iAlpha * 2.55f;
    }
    if (fAlpha < 1) fAlpha = 1; // don't quite go all the way transparent,
                                // as any shadow colour will not be rendered transparent if
                                // it's defined in the font class
    if (fAlpha > 255) fAlpha = 255.0f;
    DWORD dwAlpha = (DWORD)(fAlpha + 0.5f);
    DWORD dwColor = (dwAlpha << 24) | (m_label.textColor & 0xFFFFFF);
    DWORD dwShadowColor = (dwAlpha << 24) | (m_label.shadowColor & 0xFFFFFF);
    m_label.font->DrawText( fPosX, fPosY, dwColor, dwShadowColor, strLabelUnicode.c_str(), m_label.align);
  }
  else
  {
    float fAlpha = 255.0f;
    float fAlpha1 = 255.0f;
    // check if we have a skinner-defined texture...
    CGUIImage *pImage = m_vecButtons[iOffset]->imageNoFocus;
    if (!pImage) pImage = &m_imgNoFocus;
    pImage->SetCornerAlpha(0xFF, 0xFF, 0xFF, 0xFF);
    pImage->SetVisible(true);
    if (m_bHorizontal)
    {
      if (iPosX < fStartAlpha)
      {
        fAlpha -= (fStartAlpha - iPosX) / (fStartAlpha - m_iPosX) * m_iAlpha * 2.55f;
        fAlpha1 -= (fStartAlpha - (iPosX + m_imgFocus.GetWidth() + m_iButtonGap)) / (fStartAlpha - m_iPosX) * m_iAlpha * 2.55f;
      }
      if (iPosX >= fEndAlpha)
      {
        fAlpha -= ((float)iPosX - fEndAlpha) / (m_iPosX + (int)m_dwWidth - fEndAlpha) * m_iAlpha * 2.55f;
        fAlpha1 -= ((float)(iPosX + m_imgFocus.GetWidth() + m_iButtonGap) - fEndAlpha) / (m_iPosX + (int)m_dwWidth - fEndAlpha) * m_iAlpha * 2.55f;
      }
      if (fAlpha < 0) fAlpha = 0;
      if (fAlpha1 < 0) fAlpha1 = 0;
      if (fAlpha > 255) fAlpha = 255.0f;
      if (fAlpha1 > 255) fAlpha1 = 255.0f;
      pImage->SetCornerAlpha((DWORD)(fAlpha + 0.5f), (DWORD)(fAlpha1 + 0.5f), (DWORD)(fAlpha + 0.5f), (DWORD)(fAlpha1 + 0.5f));
    }
    else
    {
      if (iPosY < fStartAlpha)
      {
        fAlpha -= (fStartAlpha - iPosY) / (fStartAlpha - m_iPosY) * m_iAlpha * 2.55f;
        fAlpha1 -= (fStartAlpha - (iPosY + m_imgFocus.GetHeight() + m_iButtonGap)) / (fStartAlpha - m_iPosY) * m_iAlpha * 2.55f;
      }
      if (iPosY > fEndAlpha)
      {
        fAlpha -= ((float)iPosY - fEndAlpha) / (m_iPosY + (int)m_dwHeight - fEndAlpha) * m_iAlpha * 2.55f;
        fAlpha1 -= ((float)(iPosY + m_imgFocus.GetHeight() + m_iButtonGap) - fEndAlpha) / (m_iPosY + (int)m_dwHeight - fEndAlpha) * m_iAlpha * 2.55f;
      }
      if (fAlpha < 0) fAlpha = 0;
      if (fAlpha1 < 0) fAlpha1 = 0;
      if (fAlpha > 255) fAlpha = 255.0f;
      if (fAlpha1 > 255) fAlpha1 = 255.0f;
      pImage->SetCornerAlpha((DWORD)(fAlpha + 0.5f), (DWORD)(fAlpha + 0.5f), (DWORD)(fAlpha1 + 0.5f), (DWORD)(fAlpha1 + 0.5f));
    }
    pImage->SetPosition(iPosX, iPosY);
    pImage->SetWidth(m_imgNoFocus.GetWidth());
    pImage->SetHeight(m_imgNoFocus.GetHeight());
    pImage->Render();
  }
  iOffset = GetNext(iOffset);
  if (m_bHorizontal)
    iPosX += m_imgFocus.GetWidth() + m_iButtonGap;
  else
    iPosY += m_imgFocus.GetHeight() + m_iButtonGap;
}

void CGUIButtonScroller::OnChangeFocus()
{
  // send a message to our parent that our focused button has changed...
  if (!GetActiveButtonID()) return;
  CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), GetID(), GetActiveButtonID());
  g_graphicsContext.SendMessage(msg);
}

int CGUIButtonScroller::GetActiveButtonID() const
{
  int iButton = GetActiveButton();
  if (iButton < 0 || iButton >= (int)m_vecButtons.size()) return 0;
  return m_vecButtons[iButton]->id;
}

int CGUIButtonScroller::GetActiveButton() const
{
  if (m_iCurrentSlot < 0) return -1;
  int iCurrentItem = m_iOffset;
  for (int i = 0; i < m_iCurrentSlot; i++)
    if (GetNext(iCurrentItem) != -1) iCurrentItem = GetNext(iCurrentItem);
  return iCurrentItem;
}

void CGUIButtonScroller::Update()
{
  if (m_bHorizontal)
  {
    m_dwWidth = m_iNumSlots * ((int)m_imgFocus.GetWidth() + m_iButtonGap) - m_iButtonGap;
    m_dwHeight = m_imgFocus.GetHeight();
    m_iPosX = m_iXMLPosX + ((int)m_dwXMLWidth - (int)m_dwWidth) / 2;
    m_iPosY = m_iXMLPosY;
  }
  else
  {
    m_dwWidth = m_imgFocus.GetWidth();
    m_dwHeight = m_iNumSlots * ((int)m_imgFocus.GetHeight() + m_iButtonGap) - m_iButtonGap;
    m_iPosX = m_iXMLPosX;
    m_iPosY = m_iXMLPosY + ((int)m_dwXMLHeight - (int)m_dwHeight) / 2;
  }
}

void CGUIButtonScroller::GetScrollZone(float &fStartAlpha, float &fEndAlpha)
{
  // check if we are in the scrollable zone (alpha fade area)
  // calculate our alpha amount
  int iMinSlot = m_iDefaultSlot - m_iMovementRange;
  if (iMinSlot < 0) iMinSlot = 0;
  int iMaxSlot = m_iDefaultSlot + m_iMovementRange + 1;
  if (iMaxSlot > m_iNumSlots) iMaxSlot = m_iNumSlots;
  // calculate the amount of pixels between 0 and iMinSlot
  if (m_bHorizontal)
  {
    fStartAlpha = (float)m_iPosX + iMinSlot * (m_imgFocus.GetWidth() + m_iButtonGap);
    fEndAlpha = (float)m_iPosX + iMaxSlot * (m_imgFocus.GetWidth() + m_iButtonGap);
  }
  else
  {
    fStartAlpha = (float)m_iPosY + (float)iMinSlot * (m_imgFocus.GetHeight() + m_iButtonGap);
    fEndAlpha = (float)m_iPosY + (float)iMaxSlot * (m_imgFocus.GetHeight() + m_iButtonGap);
  }
}

void CGUIButtonScroller::OnMouseOver()
{
  float fStartAlpha, fEndAlpha;
  GetScrollZone(fStartAlpha, fEndAlpha);
  if (m_bHorizontal)
  {
    if (g_Mouse.iPosX < fStartAlpha) // scroll down
    {
      m_bScrollUp = false;
      if (m_iSlowScrollCount > 10) m_iSlowScrollCount = 0;
      if (m_bSmoothScrolling || m_iSlowScrollCount == 0)
        m_bScrollDown = true;
      else
        m_bScrollDown = false;
      m_iSlowScrollCount++;
      m_fScrollSpeed = 50.0f + SCROLL_SPEED - ((float)g_Mouse.iPosX - fStartAlpha) / ((float)m_iPosX - fStartAlpha) * 50.0f;
    }
    else if (g_Mouse.iPosX > fEndAlpha - 1) // scroll up
    {
      m_bScrollDown = false;
      if (m_iSlowScrollCount > 10) m_iSlowScrollCount = 0;
      if (m_bSmoothScrolling || m_iSlowScrollCount == 0)
        m_bScrollUp = true;
      else
        m_bScrollUp = false;
      m_fScrollSpeed = 50.0f + SCROLL_SPEED - ((float)g_Mouse.iPosX - fEndAlpha) / ((float)m_iPosX + (float)m_dwWidth - fEndAlpha) * 50.0f;
    }
    else // call base class
    { // select the appropriate item, and call the base class (to set focus)
      m_iCurrentSlot = (g_Mouse.iPosX - m_iPosX) / (m_imgFocus.GetWidth() + m_iButtonGap);
    }
  }
  else
  {
    if (g_Mouse.iPosY < fStartAlpha) // scroll down
    {
      m_bScrollUp = false;
      if (m_iSlowScrollCount > 10) m_iSlowScrollCount = 0;
      if (m_bSmoothScrolling || m_iSlowScrollCount == 0)
        m_bScrollDown = true;
      else
        m_bScrollDown = false;
      m_iSlowScrollCount++;
      m_fScrollSpeed = 50.0f + SCROLL_SPEED - ((float)g_Mouse.iPosY - fStartAlpha) / ((float)m_iPosY - fStartAlpha) * 50.0f;
    }
    else if (g_Mouse.iPosY > fEndAlpha - 1) // scroll up
    {
      m_bScrollDown = false;
      if (m_iSlowScrollCount > 10) m_iSlowScrollCount = 0;
      if (m_bSmoothScrolling || m_iSlowScrollCount == 0)
        m_bScrollUp = true;
      else
        m_bScrollUp = false;
      m_iSlowScrollCount++; m_fScrollSpeed = 50.0f + SCROLL_SPEED - ((float)g_Mouse.iPosY - fEndAlpha) / ((float)m_iPosY + (float)m_dwHeight - fEndAlpha) * 50.0f;
    }
    else
    { // select the appropriate item, and call the base class (to set focus)
      m_iCurrentSlot = (g_Mouse.iPosY - m_iPosY) / (m_imgFocus.GetHeight() + m_iButtonGap);
    }
  }
  CGUIControl::OnMouseOver();
}

void CGUIButtonScroller::OnMouseClick(DWORD dwButton)
{
  if (dwButton != MOUSE_LEFT_BUTTON && dwButton != MOUSE_RIGHT_BUTTON) return ;
  // check if we are in the clickable button zone
  float fStartAlpha, fEndAlpha;
  GetScrollZone(fStartAlpha, fEndAlpha);
  if (m_bHorizontal)
  {
    if (g_Mouse.iPosX >= fStartAlpha && g_Mouse.iPosX <= fEndAlpha)
    { // click the appropriate item
      m_iCurrentSlot = (g_Mouse.iPosX - m_iPosX) / (m_imgFocus.GetWidth() + m_iButtonGap);
      CAction action;
      if (dwButton == MOUSE_LEFT_BUTTON)
        action.wID = ACTION_SELECT_ITEM;
      if (dwButton == MOUSE_RIGHT_BUTTON)
        action.wID = ACTION_CONTEXT_MENU;
      OnAction(action);
    }
  }
  else
  {
    if (g_Mouse.iPosY >= fStartAlpha && g_Mouse.iPosY <= fEndAlpha)
    {
      m_iCurrentSlot = (g_Mouse.iPosY - m_iPosY) / (m_imgFocus.GetHeight() + m_iButtonGap);
      CAction action;
      if (dwButton == MOUSE_LEFT_BUTTON)
        action.wID = ACTION_SELECT_ITEM;
      if (dwButton == MOUSE_RIGHT_BUTTON)
        action.wID = ACTION_CONTEXT_MENU;
      OnAction(action);
    }
  }
}

void CGUIButtonScroller::OnMouseWheel()
{
  // check if we are within the clickable button zone
  float fStartAlpha, fEndAlpha;
  GetScrollZone(fStartAlpha, fEndAlpha);
  if ((m_bHorizontal && g_Mouse.iPosX >= fStartAlpha && g_Mouse.iPosX <= fEndAlpha) ||
      (!m_bHorizontal && g_Mouse.iPosY >= fStartAlpha && g_Mouse.iPosY <= fEndAlpha))
  {
    if (g_Mouse.cWheel > 0)
      m_bScrollDown = true;
    else
      m_bScrollUp = true;
    m_fScrollSpeed = SCROLL_SPEED;
  }
}

CStdString CGUIButtonScroller::GetDescription() const
{
  if (GetActiveButton() >= 0)
  {
    CStdString strLabel = m_vecButtons[GetActiveButton()]->strLabel;
    return strLabel;
  }
  return "";
}

void CGUIButtonScroller::SetPosition(int iPosX, int iPosY)
{
  CGUIControl::SetPosition(iPosX, iPosY);
}

void CGUIButtonScroller::SetWidth(int iWidth)
{
  CGUIControl::SetWidth(iWidth);
}

void CGUIButtonScroller::SetHeight(int iHeight)
{
  CGUIControl::SetHeight(iHeight);
}
