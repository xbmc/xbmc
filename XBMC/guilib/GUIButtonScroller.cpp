
#include "include.h"
#include "GUIButtonScroller.h"
#include "LocalizeStrings.h"
#include "GUIWindowManager.h"
#include "../xbmc/utils/CharsetConverter.h"
#include "../xbmc/Util.h"
#include "../xbmc/utils/GUIInfoManager.h"
#include "SkinInfo.h"

#define SCROLL_SPEED 6.0f
#define ANALOG_SCROLL_START 0.8f

CGUIButtonScroller::CGUIButtonScroller(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, float gap, int iSlots, int iDefaultSlot, int iMovementRange, bool bHorizontal, int iAlpha, bool bWrapAround, bool bSmoothScrolling, const CImage& textureFocus, const CImage& textureNoFocus, const CLabelInfo& labelInfo)
    : CGUIControl(dwParentID, dwControlId, posX, posY, width, height)
    , m_imgFocus(dwParentID, dwControlId, posX, posY, width, height, textureFocus)
    , m_imgNoFocus(dwParentID, dwControlId, posX, posY, width, height, textureNoFocus)
{
  m_iXMLNumSlots = iSlots;
  m_iXMLDefaultSlot = iDefaultSlot - 1;
  m_xmlPosX = posX;
  m_xmlPosY = posY;
  m_xmlWidth = width;
  m_xmlHeight = height;
  m_buttonGap = gap;
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
  m_scrollOffset = 0;
  m_bScrollUp = false;
  m_bScrollDown = false;
  m_bMoveUp = false;
  m_bMoveDown = false;
  m_fAnalogScrollSpeed = 0;
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
    vector<CStdString> actions = m_vecButtons[GetActiveButton()]->clickActions;
    for (unsigned int i = 0; i < actions.size(); i++)
    {
      CGUIMessage message(GUI_MSG_EXECUTE, GetID(), GetParentID());
      // find our currently highlighted item
      message.SetStringParam(actions[i]);
      g_graphicsContext.SendMessage(message);
    }
    return true;
  }
  if (action.wID == ACTION_CONTEXT_MENU)
  { // send a click message to our parent
    SEND_CLICK_MESSAGE(GetID(), GetParentID(), action.wID);
    return true;
  }
  // smooth scrolling (for analog controls)
  if (action.wID == ACTION_SCROLL_UP)
  {
    m_fAnalogScrollSpeed += action.fAmount1 * action.fAmount1;
    bool handled = false;
    while (m_fAnalogScrollSpeed > ANALOG_SCROLL_START)
    {
      handled = true;
      m_fAnalogScrollSpeed -= ANALOG_SCROLL_START;
      if (!m_bWrapAround && m_iOffset + m_iCurrentSlot == 0)
        break;
      DoUp();
    }
    return handled;
  }
  if (action.wID == ACTION_SCROLL_DOWN)
  {
    m_fAnalogScrollSpeed += action.fAmount1 * action.fAmount1;
    bool handled = false;
    while (m_fAnalogScrollSpeed > ANALOG_SCROLL_START)
    {
      handled = true;
      m_fAnalogScrollSpeed -= ANALOG_SCROLL_START;
      if (!m_bWrapAround && m_iOffset + m_iCurrentSlot == (int) m_vecButtons.size() - 1)
        break;
      DoDown();
    }
    return handled;
  }
  return CGUIControl::OnAction(action);
}

bool CGUIButtonScroller::OnMessage(CGUIMessage &message)
{
  if (message.GetMessage() == GUI_MSG_ITEM_SELECT)
  {
    SetActiveButton(message.GetParam1());
  }
  else if (message.GetMessage() == GUI_MSG_ITEM_SELECTED)
  {
    message.SetParam1(GetActiveButton());
    return true;
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

void CGUIButtonScroller::LoadButtons(TiXmlNode *node)
{
  // run through and find all <button> tags
  // Format is:
  // <button id="1">
  //    <label>
  //    <execute>
  //    <texturefocus>
  //    <texturenofocus>
  // </button>

  // TODO: UTF-8 - what if the XML encoding is in UTF-8?
  TiXmlElement *buttons = node->FirstChildElement("buttons");
  if (!buttons) return;

  // resolve includes
  g_SkinInfo.ResolveIncludes(buttons);

  TiXmlElement *buttonNode = buttons->FirstChildElement("button");
  while (buttonNode)
  {
    // resolve includes
    g_SkinInfo.ResolveIncludes(buttonNode);
    CButton *button = new CButton;
    buttonNode->Attribute("id", &button->id);
    const TiXmlNode *childNode = buttonNode->FirstChild("label");
    if (childNode && childNode->FirstChild())
    {
      CStdString strLabel = childNode->FirstChild()->Value();
      if (StringUtils::IsNaturalNumber(strLabel))
        button->strLabel = g_localizeStrings.Get(atoi(strLabel.c_str()));
      else
      { // convert to UTF-8
        CStdString utf8String;
        g_charsetConverter.stringCharsetToUtf8(strLabel, utf8String);
        button->strLabel = utf8String;
      }
    }
    // get info
    childNode = buttonNode->FirstChild("info");
    if (childNode && childNode->FirstChild())
      button->info = g_infoManager.TranslateString(childNode->FirstChild()->Value());
    childNode = buttonNode->FirstChild("execute");
    if (childNode && childNode->FirstChild())
      button->clickActions.push_back(childNode->FirstChild()->Value());
    childNode = buttonNode->FirstChild("onclick");
    while (childNode && childNode->FirstChild())
    {
      button->clickActions.push_back(childNode->FirstChild()->Value());
      childNode = childNode->NextSibling("onclick");
    }
    childNode = buttonNode->FirstChild("texturefocus");
    if (childNode && childNode->FirstChild())
      button->imageFocus = new CGUIImage(GetParentID(), GetID(), m_posX, m_posY, m_width, m_height, (CStdString)childNode->FirstChild()->Value());
    childNode = buttonNode->FirstChild("texturenofocus");
    if (childNode && childNode->FirstChild())
      button->imageNoFocus = new CGUIImage(GetParentID(), GetID(), m_posX, m_posY, m_width, m_height, (CStdString)childNode->FirstChild()->Value());
    m_vecButtons.push_back(button);
    buttonNode = buttonNode->NextSiblingElement("button");
  }
}

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
  // calculate our correct width and height
  if (m_bHorizontal)
  {
    m_xmlWidth = (m_iXMLNumSlots * (m_imgFocus.GetWidth() + m_buttonGap) - m_buttonGap);
    m_xmlHeight = m_imgFocus.GetHeight();
  }
  else
  {
    m_xmlWidth = m_imgFocus.GetWidth();
    m_xmlHeight = (m_iXMLNumSlots * (m_imgFocus.GetHeight() + m_buttonGap) - m_buttonGap);
  }
  m_width = m_xmlWidth;
  m_height = m_xmlHeight;
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
  float posX = m_posX;
  float posY = m_posY;
  // set our viewport
  g_graphicsContext.SetClipRegion(posX, posY, m_width, m_height);
  // if we're scrolling, update our scroll offset
  if (m_bScrollUp || m_bScrollDown)
  {
    float maxScroll = m_bHorizontal ? m_imgFocus.GetWidth() : m_imgFocus.GetHeight();
    maxScroll += m_buttonGap;
    m_scrollOffset += (int)(maxScroll / m_fScrollSpeed) + 1;
    if (m_scrollOffset > maxScroll || !m_bSmoothScrolling)
    {
      m_scrollOffset = 0;
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
    }
    else
    {
      if (m_bScrollUp)
      {
        if (m_bHorizontal)
          posX -= m_scrollOffset;
        else
          posY -= m_scrollOffset;
      }
      else
      {
        if (m_bHorizontal)
          posX += m_scrollOffset - maxScroll;
        else
          posY += m_scrollOffset - maxScroll;
      }
    }
  }
  float posX3 = posX;
  float posY3 = posY;
  // ok, now check if we're scrolling down
  int iOffset = m_iOffset;
  if (m_bScrollDown)
  {
    iOffset = GetPrevious(iOffset);
    RenderItem(posX, posY, iOffset, false);
  }
  // ok, now render the main block
  for (int i = 0; i < m_iNumSlots; i++)
    RenderItem(posX, posY, iOffset, false);
  // ok, now check if we're scrolling up
  if (m_bScrollUp)
    RenderItem(posX, posY, iOffset, false);
  // ok, now render the background slot...
  if (HasFocus())
  {
    posX = m_posX;
    posY = m_posY;
    // check if we're moving up or down
    if (m_bMoveUp || m_bMoveDown)
    {
      float maxScroll = m_bHorizontal ? m_imgFocus.GetWidth() : m_imgFocus.GetHeight();
      maxScroll += m_buttonGap;
      m_scrollOffset += maxScroll / SCROLL_SPEED + 1;
      if (m_scrollOffset > maxScroll || !m_bSmoothScrolling)
      {
        m_scrollOffset = 0;
        if (m_bMoveUp)
        {
          if (m_iCurrentSlot > 0)
            m_iCurrentSlot--;
        }
        else
        {
          if (m_iCurrentSlot + 1 < m_iNumSlots)
            m_iCurrentSlot++;
        }
        m_bMoveUp = false;
        m_bMoveDown = false;
      }
      else
      {
        if (m_bMoveUp)
        {
          if (m_bHorizontal)
            posX -= m_scrollOffset;
          else
            posY -= m_scrollOffset;
        }
        else
        {
          if (m_bHorizontal)
            posX += m_scrollOffset;
          else
            posY += m_scrollOffset;
        }
      }
    }
    if (m_bHorizontal)
      posX += m_iCurrentSlot * ((int)m_imgFocus.GetWidth() + m_buttonGap);
    else
      posY += m_iCurrentSlot * ((int)m_imgFocus.GetHeight() + m_buttonGap);
    // check if we have a skinner-defined icon image
    CGUIImage *pImage = m_vecButtons[GetActiveButton()]->imageFocus;
    if (pImage && (m_bScrollUp || m_bScrollDown))
      pImage = NULL;
    else if (!pImage)
      pImage = &m_imgFocus;
    if (pImage)
    {
      pImage->SetPosition(posX, posY);
      pImage->SetVisible(true);
      pImage->SetWidth(m_imgFocus.GetWidth());
      pImage->SetHeight(m_imgFocus.GetHeight());
      pImage->Render();
    }
  }
  // Now render the text
  iOffset = m_iOffset;
  posX = posX3;
  posY = posY3;
  if (m_bScrollDown)
  {
    iOffset = GetPrevious(iOffset);
    RenderItem(posX, posY, iOffset, true);
  }
  // ok, now render the main block
  for (int i = 0; i < m_iNumSlots; i++)
    RenderItem(posX, posY, iOffset, true);
  // ok, now check if we're scrolling up
  if (m_bScrollUp)
    RenderItem(posX, posY, iOffset, true);

  // reset the viewport
  g_graphicsContext.RestoreClipRegion();
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
      return ;
    }
    int iMaxButton = m_iDefaultSlot + m_iMovementRange;
    if (iMaxButton >= m_iNumSlots) iMaxButton = m_iNumSlots - 1;
    if (iButton > iMaxButton)
    {
      m_iOffset = iButton - iMaxButton;
      m_iCurrentSlot = iMaxButton;
      return ;
    }
    // now change our current slot so that it all fits nicely
    // lastly, make sure we fill the number of slots that we have (if possible)
    int iNumButtonsToShow = m_vecButtons.size() - iButton + m_iCurrentSlot;
    if (iNumButtonsToShow < m_iNumSlots && iNumButtonsToShow < (int)m_vecButtons.size())
    { // we have empty space - try and fill it up
      while (iNumButtonsToShow < (int)m_vecButtons.size() && m_iCurrentSlot + 1 < m_iNumSlots)
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
  else if (!m_bWrapAround && (unsigned int) (m_iOffset + m_iCurrentSlot) == m_vecButtons.size() - 1)
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
  else if (!m_bWrapAround && (unsigned int) (m_iOffset + m_iCurrentSlot) == m_vecButtons.size() - 1 && m_dwControlRight != GetID())
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
        m_scrollOffset = 0;
        m_iOffset = GetPrevious(m_iOffset);
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
        m_scrollOffset = 0;
        if (m_iCurrentSlot > 0) m_iCurrentSlot--;
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
        m_scrollOffset = 0;
        if (GetNext(m_iOffset) != -1) m_iOffset = GetNext(m_iOffset);
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
        m_scrollOffset = 0;
        if (m_iCurrentSlot + 1 < m_iNumSlots) m_iCurrentSlot++;
      }
      m_bMoveDown = true;
    }
  }
}

void CGUIButtonScroller::RenderItem(float &posX, float &posY, int &iOffset, bool bText)
{
  if (iOffset < 0) return ;
  float fStartAlpha, fEndAlpha;
  GetScrollZone(fStartAlpha, fEndAlpha);
  if (bText)
  {
    if (!m_label.font) return ;
    float fPosX = posX + m_label.offsetX;
    float fPosY = posY + m_label.offsetY;
    if (m_label.align & XBFONT_RIGHT)
      fPosX = posX + m_imgFocus.GetWidth() - m_label.offsetX;
    if (m_label.align & XBFONT_CENTER_X)
      fPosX = posX + m_imgFocus.GetWidth() / 2;
    if (m_label.align & XBFONT_CENTER_Y)
      fPosY = posY + m_imgFocus.GetHeight() / 2;

    // label is from <info> tag first, and if that's blank,
    // we use the <label> tag
    CStdString label = g_infoManager.GetLabel(m_vecButtons[iOffset]->info);
    if (label.IsEmpty())
      label = m_vecButtons[iOffset]->strLabel;

    CStdStringW strLabelUnicode;
    g_charsetConverter.utf8ToW(label, strLabelUnicode);

    float fAlpha = 255.0f;
    if (m_bHorizontal)
    {
      if (fPosX < fStartAlpha)
        fAlpha -= (fStartAlpha - fPosX) / (fStartAlpha - m_posX) * m_iAlpha * 2.55f;
      if (fPosX > fEndAlpha)
        fAlpha -= (fPosX - fEndAlpha) / (m_posX + m_width - fEndAlpha) * m_iAlpha * 2.55f;
    }
    else
    {
      if (fPosY < fStartAlpha)
        fAlpha -= (fStartAlpha - fPosY) / (fStartAlpha - m_posY) * m_iAlpha * 2.55f;
      if (fPosY > fEndAlpha)
        fAlpha -= (fPosY - fEndAlpha) / (m_posY + m_height - fEndAlpha) * m_iAlpha * 2.55f;
    }
    if (fAlpha < 1) fAlpha = 1; // don't quite go all the way transparent,
                                // as any shadow colour will not be rendered transparent if
                                // it's defined in the font class
    if (fAlpha > 255) fAlpha = 255.0f;
    DWORD dwAlpha = (DWORD)(fAlpha + 0.5f);
    DWORD color = (m_label.focusedColor && iOffset == GetActiveButton()) ? m_label.focusedColor : m_label.textColor;
    DWORD alpha = (dwAlpha * ((color & 0xff000000) >> 24)) / 255;
    DWORD dwColor = (alpha << 24) | (color & 0xFFFFFF);
    alpha = (dwAlpha * ((m_label.shadowColor & 0xff000000) >> 24)) / 255;
    DWORD dwShadowColor = (alpha << 24) | (m_label.shadowColor & 0xFFFFFF);
    m_label.font->DrawText( fPosX, fPosY, dwColor, dwShadowColor, strLabelUnicode.c_str(), m_label.align);
  }
  else
  {
    float fAlpha = 255.0f;
    float fAlpha1 = 255.0f;
    // check if we have a skinner-defined texture...
    CGUIImage *pImage = m_vecButtons[iOffset]->imageNoFocus;
    if (!pImage) pImage = &m_imgNoFocus;
    pImage->SetAlpha(0xFF);
    pImage->SetVisible(true);
    if (m_bHorizontal)
    {
      if (posX < fStartAlpha)
      {
        fAlpha -= (fStartAlpha - posX) / (fStartAlpha - m_posX) * m_iAlpha * 2.55f;
        fAlpha1 -= (fStartAlpha - (posX + m_imgFocus.GetWidth() + m_buttonGap)) / (fStartAlpha - m_posX) * m_iAlpha * 2.55f;
      }
      if (posX >= fEndAlpha)
      {
        fAlpha -= (posX - fEndAlpha) / (m_posX + m_width - fEndAlpha) * m_iAlpha * 2.55f;
        fAlpha1 -= ((posX + m_imgFocus.GetWidth() + m_buttonGap) - fEndAlpha) / (m_posX + m_width - fEndAlpha) * m_iAlpha * 2.55f;
      }
      if (fAlpha < 0) fAlpha = 0;
      if (fAlpha1 < 0) fAlpha1 = 0;
      if (fAlpha > 255) fAlpha = 255.0f;
      if (fAlpha1 > 255) fAlpha1 = 255.0f;
      pImage->SetAlpha((unsigned char)(fAlpha + 0.5f), (unsigned char)(fAlpha1 + 0.5f),
                       (unsigned char)(fAlpha1 + 0.5f), (unsigned char)(fAlpha + 0.5f));
    }
    else
    {
      if (posY < fStartAlpha)
      {
        fAlpha -= (fStartAlpha - posY) / (fStartAlpha - m_posY) * m_iAlpha * 2.55f;
        fAlpha1 -= (fStartAlpha - (posY + m_imgFocus.GetHeight() + m_buttonGap)) / (fStartAlpha - m_posY) * m_iAlpha * 2.55f;
      }
      if (posY > fEndAlpha)
      {
        fAlpha -= (posY - fEndAlpha) / (m_posY + m_height - fEndAlpha) * m_iAlpha * 2.55f;
        fAlpha1 -= ((posY + m_imgFocus.GetHeight() + m_buttonGap) - fEndAlpha) / (m_posY + m_height - fEndAlpha) * m_iAlpha * 2.55f;
      }
      if (fAlpha < 0) fAlpha = 0;
      if (fAlpha1 < 0) fAlpha1 = 0;
      if (fAlpha > 255) fAlpha = 255.0f;
      if (fAlpha1 > 255) fAlpha1 = 255.0f;
      pImage->SetAlpha((unsigned char)(fAlpha + 0.5f), (unsigned char)(fAlpha + 0.5f),
                       (unsigned char)(fAlpha1 + 0.5f), (unsigned char)(fAlpha1 + 0.5f));
    }
    pImage->SetPosition(posX, posY);
    pImage->SetWidth(m_imgNoFocus.GetWidth());
    pImage->SetHeight(m_imgNoFocus.GetHeight());
    pImage->Render();
  }
  iOffset = GetNext(iOffset);
  if (m_bHorizontal)
    posX += m_imgFocus.GetWidth() + m_buttonGap;
  else
    posY += m_imgFocus.GetHeight() + m_buttonGap;
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
    m_width = m_iNumSlots * (m_imgFocus.GetWidth() + m_buttonGap) - m_buttonGap;
    m_height = m_imgFocus.GetHeight();
    m_posX = m_xmlPosX + (m_xmlWidth - m_width) * 0.5f;
    m_posY = m_xmlPosY;
  }
  else
  {
    m_width = m_imgFocus.GetWidth();
    m_height = m_iNumSlots * (m_imgFocus.GetHeight() + m_buttonGap) - m_buttonGap;
    m_posX = m_xmlPosX;
    m_posY = m_xmlPosY + (m_xmlHeight - m_height) * 0.5f;
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
    fStartAlpha = m_posX + iMinSlot * (m_imgFocus.GetWidth() + m_buttonGap);
    fEndAlpha = m_posX + iMaxSlot * (m_imgFocus.GetWidth() + m_buttonGap) - m_buttonGap;
  }
  else
  {
    fStartAlpha = m_posY + iMinSlot * (m_imgFocus.GetHeight() + m_buttonGap);
    fEndAlpha = m_posY + iMaxSlot * (m_imgFocus.GetHeight() + m_buttonGap) - m_buttonGap;
  }
}

bool CGUIButtonScroller::OnMouseOver(const CPoint &point)
{
  float fStartAlpha, fEndAlpha;
  GetScrollZone(fStartAlpha, fEndAlpha);
  if (m_bHorizontal)
  {
    if (point.x < fStartAlpha) // scroll down
    {
      m_bScrollUp = false;
      if (m_iSlowScrollCount > 10) m_iSlowScrollCount = 0;
      if (m_bSmoothScrolling || m_iSlowScrollCount == 0)
        m_bScrollDown = true;
      else
        m_bScrollDown = false;
      m_iSlowScrollCount++;
      m_fScrollSpeed = 50.0f + SCROLL_SPEED - (point.x - fStartAlpha) / (m_posX - fStartAlpha) * 50.0f;
    }
    else if (point.x > fEndAlpha - 1) // scroll up
    {
      m_bScrollDown = false;
      if (m_iSlowScrollCount > 10) m_iSlowScrollCount = 0;
      if (m_bSmoothScrolling || m_iSlowScrollCount == 0)
        m_bScrollUp = true;
      else
        m_bScrollUp = false;
      m_fScrollSpeed = 50.0f + SCROLL_SPEED - (point.x - fEndAlpha) / (m_posX + m_width - fEndAlpha) * 50.0f;
    }
    else // call base class
    { // select the appropriate item, and call the base class (to set focus)
      m_iCurrentSlot = (int)((point.x - m_posX) / (m_imgFocus.GetWidth() + m_buttonGap));
    }
  }
  else
  {
    if (point.y < fStartAlpha) // scroll down
    {
      m_bScrollUp = false;
      if (m_iSlowScrollCount > 10) m_iSlowScrollCount = 0;
      if (m_bSmoothScrolling || m_iSlowScrollCount == 0)
        m_bScrollDown = true;
      else
        m_bScrollDown = false;
      m_iSlowScrollCount++;
      m_fScrollSpeed = 50.0f + SCROLL_SPEED - (point.y - fStartAlpha) / (m_posY - fStartAlpha) * 50.0f;
    }
    else if (point.y > fEndAlpha - 1) // scroll up
    {
      m_bScrollDown = false;
      if (m_iSlowScrollCount > 10) m_iSlowScrollCount = 0;
      if (m_bSmoothScrolling || m_iSlowScrollCount == 0)
        m_bScrollUp = true;
      else
        m_bScrollUp = false;
      m_iSlowScrollCount++; m_fScrollSpeed = 50.0f + SCROLL_SPEED - (point.y - fEndAlpha) / (m_posY + m_height - fEndAlpha) * 50.0f;
    }
    else
    { // select the appropriate item, and call the base class (to set focus)
      m_iCurrentSlot = (int)((point.y - m_posY) / (m_imgFocus.GetHeight() + m_buttonGap));
    }
  }
  return CGUIControl::OnMouseOver(point);
}

bool CGUIButtonScroller::OnMouseClick(DWORD dwButton, const CPoint &point)
{
  if (dwButton != MOUSE_LEFT_BUTTON && dwButton != MOUSE_RIGHT_BUTTON) return false;
  // check if we are in the clickable button zone
  float fStartAlpha, fEndAlpha;
  GetScrollZone(fStartAlpha, fEndAlpha);
  if (m_bHorizontal)
  {
    if (point.x >= fStartAlpha && point.x <= fEndAlpha)
    { // click the appropriate item
      m_iCurrentSlot = (int)((point.x - m_posX) / (m_imgFocus.GetWidth() + m_buttonGap));
      CAction action;
      if (dwButton == MOUSE_LEFT_BUTTON)
        action.wID = ACTION_SELECT_ITEM;
      if (dwButton == MOUSE_RIGHT_BUTTON)
        action.wID = ACTION_CONTEXT_MENU;
      OnAction(action);
      return true;
    }
  }
  else
  {
    if (point.y >= fStartAlpha && point.y <= fEndAlpha)
    {
      m_iCurrentSlot = (int)((point.y - m_posY) / (m_imgFocus.GetHeight() + m_buttonGap));
      CAction action;
      if (dwButton == MOUSE_LEFT_BUTTON)
        action.wID = ACTION_SELECT_ITEM;
      if (dwButton == MOUSE_RIGHT_BUTTON)
        action.wID = ACTION_CONTEXT_MENU;
      OnAction(action);
      return true;
    }
  }
  return false;
}

bool CGUIButtonScroller::OnMouseWheel(char wheel, const CPoint &point)
{
  // check if we are within the clickable button zone
  float fStartAlpha, fEndAlpha;
  GetScrollZone(fStartAlpha, fEndAlpha);
  if ((m_bHorizontal && point.x >= fStartAlpha && point.x <= fEndAlpha) ||
      (!m_bHorizontal && point.y >= fStartAlpha && point.y <= fEndAlpha))
  {
    if (wheel > 0)
      m_bScrollDown = true;
    else
      m_bScrollUp = true;
    m_fScrollSpeed = SCROLL_SPEED;
    return true;
  }
  return false;
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

void CGUIButtonScroller::SaveStates(vector<CControlState> &states)
{
  states.push_back(CControlState(GetID(), GetActiveButton()));
}
