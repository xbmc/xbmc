#include "include.h"
#include "GUIListControl.h"
#include "../xbmc/utils/CharsetConverter.h"


#define CONTROL_LIST  0
#define CONTROL_UPDOWN 1
CGUIListControl::CGUIListControl(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight,
                                 DWORD dwSpinWidth, DWORD dwSpinHeight,
                                 const CStdString& strUp, const CStdString& strDown,
                                 const CStdString& strUpFocus, const CStdString& strDownFocus,
                                 DWORD dwSpinColor, int iSpinX, int iSpinY,
                                 const CLabelInfo& labelInfo, const CLabelInfo& labelInfo2,
                                 const CStdString& strButton, const CStdString& strButtonFocus)
    : CGUIControl(dwParentID, dwControlId, iPosX, iPosY, dwWidth, dwHeight)
    , m_upDown(dwControlId, 0, 0, 0, dwSpinWidth, dwSpinHeight, strUp, strDown, strUpFocus, strDownFocus, labelInfo, dwSpinColor, SPIN_CONTROL_TYPE_INT)
    , m_imgButton(dwControlId, 0, iPosX, iPosY, dwWidth, dwHeight, strButtonFocus, strButton, labelInfo)
{
  m_label = labelInfo;
  m_label2 = labelInfo2;
  m_upDown.SetSpinAlign(XBFONT_CENTER_Y | XBFONT_RIGHT, 0);
  m_iOffset = 0;
  m_fSmoothScrollOffset = 0;
  m_iItemsPerPage = 10;
  m_iItemHeight = 10;
  m_iSelect = CONTROL_LIST;
  m_iCursorY = 0;
  m_strSuffix = L"|";
  m_iSpinPosX = iSpinX;
  m_iSpinPosY = iSpinY;
  m_iImageWidth = 16;
  m_iImageHeight = 16;
  m_iSpaceBetweenItems = 4;
  m_bUpDownVisible = true;   // show the spin control by default
  m_upDown.SetShowRange(true); // show the range by default
  ControlType = GUICONTROL_LIST;
}

CGUIListControl::~CGUIListControl(void)
{
}

void CGUIListControl::Render()
{
  if (!UpdateEffectState()) return ;

  // Free memory not used on screen at the moment, do this first so there's more memory for the new items.
  if (m_iOffset < 30000)
  {
    for (int i = 0; i < m_iOffset; ++i)
    {
      CGUIListItem *pItem = m_vecItems[i];
      if (pItem)
      {
        pItem->FreeMemory();
      }
    }
  }
  for (int i = m_iOffset + m_iItemsPerPage; i < (int)m_vecItems.size(); ++i)
  {
    CGUIListItem *pItem = m_vecItems[i];
    if (pItem)
    {
      pItem->FreeMemory();
    }
  }

  // Loop through the list 3 times
  // 1. Render buttons & icons
  // 2. Render all text for font m_pFont
  // 3. Render all text for font m_pFont2
  // This is slightly faster than looping once through the list, render item, render m_pFont, render m_pFont2, render item, render m_pFont... etc
  // Text-rendering is generally slow and batching between Begin() End() makes it a bit faster. (XPR fonts)
  //Render buttons and icons
  int iPosY = m_iPosY;
  for (int i = 0; i < m_iItemsPerPage; i++)
  {
    int iPosX = m_iPosX;
    if (i + m_iOffset < (int)m_vecItems.size())
    {
      CGUIListItem *pItem = m_vecItems[i + m_iOffset];
      // focused line
      m_imgButton.SetFocus(i == m_iCursorY && HasFocus() && m_iSelect == CONTROL_LIST);
      m_imgButton.SetPosition(m_iPosX, iPosY);
      m_imgButton.Render();

      // render the icon
      if (pItem->HasThumbnail())
      {
        CStdString strThumb = pItem->GetThumbnailImage();
        //if (strThumb.Right(4) == ".tbn" || strThumb.Right(10) == "folder.jpg" || strThumb.Find("\\imdb\\imdb"))
        { // copy the thumb -> icon
          pItem->SetIconImage(strThumb);
        }
      }
      if (pItem->HasIcon() )
      {
        // show icon
        CGUIImage* pImage = pItem->GetIcon();
        if (!pImage)
        {
          if (m_iImageWidth * m_iImageHeight > 32*32 && !pItem->HasThumbnail())
          { // use large version of the icon
            CStdString strLargeIcon = pItem->GetIconImage();
            strLargeIcon.Insert(strLargeIcon.Find("."), "Big");
            pImage = new CGUIImage(0, 0, 0, 0, m_iImageWidth, m_iImageHeight, strLargeIcon, 0x0);
          }
          else
            pImage = new CGUIImage(0, 0, 0, 0, m_iImageWidth, m_iImageHeight, pItem->GetIconImage(), 0x0);
          pImage->SetKeepAspectRatio(true);
          pImage->AllocResources();
          pItem->SetIcon(pImage);
        }

        if (pImage)
        {
          pImage->SetWidth(m_iImageWidth);
          pImage->SetHeight(m_iImageHeight);
          // center vertically
          pImage->SetPosition(iPosX + 8 + (m_iImageWidth - pImage->GetRenderWidth()) / 2, iPosY + (m_iItemHeight - pImage->GetRenderHeight()) / 2);
          pImage->Render();
        }
      }
      iPosY += m_iItemHeight + m_iSpaceBetweenItems;
    }
  }

  //--------------------------------------------------------
  //Batch together all textrendering for m_pFont
  iPosY = m_iPosY;
  if (m_label.font)
  {
    m_label.font->Begin();
    for (int i = 0; i < m_iItemsPerPage; i++)
    {
      int iPosX = m_iPosX;
      if (i + m_iOffset < (int)m_vecItems.size() )
      {
        CGUIListItem *pItem = m_vecItems[i + m_iOffset];
        CStdString strLabel2 = pItem->GetLabel2();
        iPosX += m_iImageWidth + m_label.offsetX + 10;

        DWORD dwColor = m_label.textColor;
        if (pItem->IsSelected())
        {
          dwColor = m_label.selectedColor;
        }

        bool bSelected(i == m_iCursorY && HasFocus() && m_iSelect == CONTROL_LIST);

        DWORD dMaxWidth = (m_dwWidth - m_iImageWidth - 16);
        if ( strLabel2.size() > 0 && m_label2.font)
        {
          CStdStringW labelUnicode2;
          g_charsetConverter.stringCharsetToFontCharset(strLabel2, labelUnicode2);
          if ( m_label.offsetY == m_label2.offsetY )
          {
            float fTextHeight = 0;
            float fTextWidth = 0;
            m_label2.font->GetTextExtent( labelUnicode2.c_str(), &fTextWidth, &fTextHeight);
            dMaxWidth -= (DWORD)(fTextWidth + 20);
          }
        }

        CStdStringW labelUnicode;
        g_charsetConverter.stringCharsetToFontCharset(pItem->GetLabel(), labelUnicode);
        float fPosY = (float)iPosY + m_label.offsetY;
        if (m_label.align & XBFONT_CENTER_Y)
        {
          float fTextHeight = 0;
          float fTextWidth = 0;
          m_label.font->GetTextExtent( labelUnicode.c_str(), &fTextWidth, &fTextHeight);
          fPosY = (float)iPosY + (m_iItemHeight - fTextHeight) / 2;
        }
        RenderText((float)iPosX, fPosY, (float)dMaxWidth, dwColor, (WCHAR*)labelUnicode.c_str(), bSelected);
        iPosY += m_iItemHeight + m_iSpaceBetweenItems;
      }
    }
    m_label.font->End();
  }
  //------------------------------------------
  //Batch together all textrendering for m_pFont2
  iPosY = m_iPosY;
  if (m_label2.font)
  {
    m_label2.font->Begin();
    for (int i = 0; i < m_iItemsPerPage; i++)
    {
      int iPosX = m_iPosX;
      if (i + m_iOffset < (int)m_vecItems.size())
      {
        CGUIListItem *pItem = m_vecItems[i + m_iOffset];
        CStdString strLabel2 = pItem->GetLabel2();

        iPosX += m_iImageWidth + m_label.offsetX + 10;
        if (strLabel2.size() > 0 && m_label2.font)
        {
          CStdStringW labelUnicode2;
          g_charsetConverter.stringCharsetToFontCharset(strLabel2, labelUnicode2);
          DWORD dwColor = m_label2.textColor;
          if (pItem->IsSelected())
          {
            dwColor = m_label2.selectedColor;
          }
          if (!m_label2.offsetX)
            iPosX = m_iPosX + m_dwWidth - 16;
          else
            iPosX = m_iPosX + m_label2.offsetX;

          float fPosY = (float)iPosY + m_label2.offsetY;
          if (m_label.align & XBFONT_CENTER_Y)
          {
            fPosY = (float)iPosY + m_iItemHeight * 0.5f;
          }
          m_label2.font->DrawText((float)iPosX, fPosY, dwColor, m_label2.shadowColor, labelUnicode2.c_str(), (m_label.align & XBFONT_CENTER_Y) | XBFONT_RIGHT);
        }
        iPosY += m_iItemHeight + m_iSpaceBetweenItems;
      }
    }
    m_label2.font->End();
  }

  if (m_bUpDownVisible && m_upDown.GetMaximum() > 1)
  {
    m_upDown.SetPosition(m_iPosX + m_iSpinPosX, m_iPosY + m_iSpinPosY);
    m_upDown.SetValue(GetPage());
    m_upDown.Render();
  }
  CGUIControl::Render();
}

void CGUIListControl::RenderText(float fPosX, float fPosY, float fMaxWidth, DWORD dwTextColor, WCHAR* wszText, bool bScroll )
{
  if (!m_label.font)
    return ;

  static int iLastItem = -1;

  float fTextHeight = 0;
  float fTextWidth = 0;
  m_label.font->GetTextExtent(wszText, &fTextWidth, &fTextHeight);

  if (!bScroll)
  {
    m_label.font->DrawTextWidth(fPosX, fPosY, dwTextColor, m_label.shadowColor, wszText, fMaxWidth);
    return ;
  }
  else
  {
    if (fTextWidth <= fMaxWidth)
    { // don't need to scroll
      m_label.font->DrawTextWidth(fPosX, fPosY, dwTextColor, m_label.shadowColor, wszText, fMaxWidth);
      m_scrollInfo.Reset();
      return ;
    }
    // scroll
    CStdStringW scrollString = wszText;
    scrollString += L" ";
    scrollString += m_strSuffix;
    int iItem = m_iCursorY + m_iOffset;
    m_label.font->End(); // need to deinit the font before setting viewport
    if (iLastItem != iItem)
    {
      m_scrollInfo.Reset();
      iLastItem = iItem;
    }
    m_label.font->DrawScrollingText(fPosX, fPosY, &dwTextColor, 1, m_label.shadowColor, scrollString, fMaxWidth, m_scrollInfo);
    m_label.font->Begin(); // resume fontbatching
  }
}

bool CGUIListControl::OnAction(const CAction &action)
{
  switch (action.wID)
  {
  case ACTION_PAGE_UP:
    {
      if (m_iOffset == 0)
      { // already on the first page, so move to the first item
        m_iCursorY = 0;
        if (m_iCursorY < 0) m_iCursorY = 0;
      }
      else
      { // scroll up to the previous page
        Scroll( -m_iItemsPerPage);
      }
      return true;
    }
    break;
  case ACTION_PAGE_DOWN:
    {
      if (m_iOffset == (int)m_vecItems.size() - m_iItemsPerPage || (int)m_vecItems.size() < m_iItemsPerPage)
      { // already at the last page, so move to the last item.
        m_iCursorY = m_vecItems.size() - m_iOffset - 1;
        if (m_iCursorY < 0) m_iCursorY = 0;
      }
      else
      { // scroll down to the next page
        Scroll(m_iItemsPerPage);
      }
      return true;
    }
    break;
    // smooth scrolling (for analog controls)
  case ACTION_SCROLL_UP:
    {
      m_fSmoothScrollOffset += action.fAmount1 * action.fAmount1;
      bool handled = false;
      while (m_fSmoothScrollOffset > 0.4)
      {
        handled = true;
        m_fSmoothScrollOffset -= 0.4f;
        if (m_iOffset > 0 && m_iCursorY <= m_iItemsPerPage / 2)
        {
          Scroll( -1);
        }
        else if (m_iCursorY > 0)
        {
          m_iCursorY--;
        }
      }
      return handled;
    }
    break;
  case ACTION_SCROLL_DOWN:
    {
      m_fSmoothScrollOffset += action.fAmount1 * action.fAmount1;
      bool handled = false;
      while (m_fSmoothScrollOffset > 0.4)
      {
        handled = true;
        m_fSmoothScrollOffset -= 0.4f;
        if (m_iOffset + m_iItemsPerPage < (int)m_vecItems.size() && m_iCursorY >= m_iItemsPerPage / 2)
        {
          Scroll(1);
        }
        else if (m_iCursorY < m_iItemsPerPage - 1 && m_iOffset + m_iCursorY < (int)m_vecItems.size() - 1)
        {
          m_iCursorY++;
        }
      }
      return handled;
    }
    break;

  case ACTION_MOVE_LEFT:
  case ACTION_MOVE_RIGHT:
  case ACTION_MOVE_DOWN:
  case ACTION_MOVE_UP:
    { // use base class implementation
      return CGUIControl::OnAction(action);
    }
    break;

  default:
    {
      if (m_iSelect == CONTROL_LIST && action.wID)
      { // Don't know what to do, so send to our parent window.
        CGUIMessage msg(GUI_MSG_CLICKED, GetID(), GetParentID(), action.wID);
        return g_graphicsContext.SendMessage(msg);
      }
      else
      { // send action to the page control
        return m_upDown.OnAction(action);
      }
    }
  }
}

bool CGUIListControl::OnMessage(CGUIMessage& message)
{
  if (message.GetControlId() == GetID() )
  {
    if (message.GetSenderId() == 0) // page spin control
    {
      if (message.GetMessage() == GUI_MSG_CLICKED)
      {
        m_iOffset = (m_upDown.GetValue() - 1) * m_iItemsPerPage;
        while (m_iOffset + m_iCursorY >= (int)m_vecItems.size()) m_iCursorY--;
        // moving to the last page
        if (m_iOffset + m_iItemsPerPage > (int)m_vecItems.size() && (int)m_vecItems.size() >= m_iItemsPerPage)
        {
          m_iOffset = m_vecItems.size() - m_iItemsPerPage;
          m_iCursorY = m_iItemsPerPage - 1;
        }
      }
    }
    if (message.GetMessage() == GUI_MSG_LOSTFOCUS ||
        message.GetMessage() == GUI_MSG_SETFOCUS)
    {
      m_iSelect = CONTROL_LIST;
    }
    if (message.GetMessage() == GUI_MSG_LABEL_ADD)
    {
      CGUIListItem* pItem = (CGUIListItem*)message.GetLPVOID();
      m_vecItems.push_back( pItem);
      int iPages = m_vecItems.size() / m_iItemsPerPage;
      if (m_vecItems.size() % m_iItemsPerPage) iPages++;
      m_upDown.SetRange(1, iPages);
      m_upDown.SetValue(1);
    }

    if (message.GetMessage() == GUI_MSG_LABEL_RESET)
    {
      m_iCursorY = 0;
      m_iOffset = 0;
      m_vecItems.erase(m_vecItems.begin(), m_vecItems.end());
      m_upDown.SetRange(1, 1);
      m_upDown.SetValue(1);
    }

    if (message.GetMessage() == GUI_MSG_ITEM_SELECTED)
    {
      message.SetParam1(m_iCursorY + m_iOffset);
    }
    if (message.GetMessage() == GUI_MSG_ITEM_SELECT)
    {
      if (message.GetParam1() >= 0 && message.GetParam1() < (int)m_vecItems.size())
      {
        // Check that m_iOffset is valid
        if (m_iOffset > (int)m_vecItems.size() - m_iItemsPerPage) m_iOffset = m_vecItems.size() - m_iItemsPerPage;
        if (m_iOffset < 0) m_iOffset = 0;
        // Select the item requested
        int iItem = message.GetParam1();
        if (iItem >= m_iOffset && iItem < m_iOffset + m_iItemsPerPage)
        { // the item is on the current page, so don't change it.
          m_iCursorY = iItem - m_iOffset;
        }
        else if (iItem < m_iOffset)
        { // item is on a previous page - make it the first item on the page
          m_iCursorY = 0;
          m_iOffset = iItem;
        }
        else // (iItem >= m_iOffset+m_iItemsPerPage)
        { // item is on a later page - make it the last item on the page
          m_iCursorY = m_iItemsPerPage - 1;
          m_iOffset = iItem - m_iCursorY;
        }
      }
    }
  }

  if ( CGUIControl::OnMessage(message) ) return true;

  return false;
}

void CGUIListControl::PreAllocResources()
{
  if (!m_label.font) return ;
  CGUIControl::PreAllocResources();
  m_upDown.PreAllocResources();
  m_imgButton.PreAllocResources();
}

void CGUIListControl::AllocResources()
{
  if (!m_label.font) return ;
  CGUIControl::AllocResources();
  m_upDown.AllocResources();

  m_imgButton.AllocResources();

  SetWidth(m_dwWidth);
  SetHeight(m_dwHeight);
}

void CGUIListControl::FreeResources()
{
  CGUIControl::FreeResources();
  m_upDown.FreeResources();
  m_imgButton.FreeResources();
}

void CGUIListControl::DynamicResourceAlloc(bool bOnOff)
{
  CGUIControl::DynamicResourceAlloc(bOnOff);
  m_upDown.DynamicResourceAlloc(bOnOff);
  //  Image will be reloaded all the time
  //m_imgButton.DynamicResourceAlloc(bOnOff);
}

void CGUIListControl::OnRight()
{
  if (m_iSelect == CONTROL_LIST)
  { // Only move to up/down control if we have move than 1 page
    if (m_bUpDownVisible && m_upDown.GetMaximum() > 1)
    { // Move to updown control
      m_iSelect = CONTROL_UPDOWN;
      m_upDown.SetFocus(true);
    }
    else
    {
      CGUIControl::OnRight();
    }
  }
  else
    m_upDown.OnRight();
  if (!m_upDown.HasFocus())
    m_iSelect = CONTROL_LIST;
}

void CGUIListControl::OnLeft()
{
  if (m_iSelect == CONTROL_LIST)
    CGUIControl::OnLeft();
  else
    m_upDown.OnLeft();
  if (!m_upDown.HasFocus())
    m_iSelect = CONTROL_LIST;
}

void CGUIListControl::OnUp()
{
  if (m_iSelect == CONTROL_LIST)
  {
    if (m_iCursorY > 0)
    {
      m_iCursorY--;
    }
    else if (m_iCursorY == 0 && m_iOffset)
    {
      m_iOffset--;
    }
    else
    {
      if (m_vecItems.size() > 0)
      {
        // move 2 last item in list
        m_iOffset = m_vecItems.size() - m_iItemsPerPage;
        if (m_iOffset < 0) m_iOffset = 0;
        m_iCursorY = m_vecItems.size() - m_iOffset - 1;
      }
    }
  }
  else
  {
    m_upDown.OnUp();
    if (!m_upDown.HasFocus())
      m_iSelect = CONTROL_LIST;
  }
}

void CGUIListControl::OnDown()
{
  if (m_iSelect == CONTROL_LIST)
  {
    if (m_iCursorY + 1 < m_iItemsPerPage)
    {
      if (m_iOffset + 1 + m_iCursorY < (int)m_vecItems.size())
      {
        m_iCursorY++;
      }
      else
      {
        // move first item in list
        m_iOffset = 0;
        m_iCursorY = 0;
      }
    }
    else
    {
      if (m_iOffset + 1 + m_iCursorY < (int)m_vecItems.size())
      {
        m_iOffset++;
      }
      else
      {
        // move first item in list
        m_iOffset = 0;
        m_iCursorY = 0;
      }
    }
  }
  else
  {
    m_upDown.OnDown();
    if (!m_upDown.HasFocus())
    {
      CGUIControl::OnDown();
    }
  }
}

void CGUIListControl::SetScrollySuffix(const CStdString& wstrSuffix)
{
  WCHAR wsSuffix[128];
  swprintf(wsSuffix, L"%S", wstrSuffix.c_str());
  m_strSuffix = wsSuffix;
}

// scrolls the said amount
void CGUIListControl::Scroll(int iAmount)
{
  // increase or decrease the offset
  m_iOffset += iAmount;
  if (m_iOffset > (int)m_vecItems.size() - m_iItemsPerPage)
  {
    m_iOffset = m_vecItems.size() - m_iItemsPerPage;
  }
  if (m_iOffset < 0) m_iOffset = 0;
}

// returns which page we are on
int CGUIListControl::GetPage()
{
  if (m_iOffset >= (int)m_vecItems.size() - m_iItemsPerPage)
  {
    m_iOffset = m_vecItems.size() - m_iItemsPerPage;
    if (m_iOffset <= 0)
    {
      m_iOffset = 0;
      return 1;
    }
    if (m_vecItems.size() % m_iItemsPerPage)
      return m_vecItems.size() / m_iItemsPerPage + 1;
    else
      return m_vecItems.size() / m_iItemsPerPage;
  }
  else
    return m_iOffset / m_iItemsPerPage + 1;
}

void CGUIListControl::SetImageDimensions(int iWidth, int iHeight)
{
  m_iImageWidth = iWidth;
  m_iImageHeight = iHeight;
}

void CGUIListControl::SetItemHeight(int iHeight)
{
  m_iItemHeight = iHeight;
}
void CGUIListControl::SetSpace(int iHeight)
{
  m_iSpaceBetweenItems = iHeight;
}

int CGUIListControl::GetSelectedItem() const
{
  return m_iCursorY + m_iOffset;
}

bool CGUIListControl::SelectItemFromPoint(int iPosX, int iPosY)
{
  int iRow = iPosY / (m_iItemHeight + m_iSpaceBetweenItems);
  if (iRow >= 0 && iRow < m_iItemsPerPage && iRow + m_iOffset < (int)m_vecItems.size())
  {
    m_iCursorY = iRow;
    return true;
  }
  return false;
}

void CGUIListControl::GetPointFromItem(int &iPosX, int &iPosY)
{
  iPosY = m_iCursorY * (m_iItemHeight + m_iSpaceBetweenItems) + m_iItemHeight / 2;
  iPosX = m_dwWidth / 2;
}

void CGUIListControl::SetPageControlVisible(bool bVisible)
{
  m_bUpDownVisible = bVisible;
  return ;
}

bool CGUIListControl::HitTest(int iPosX, int iPosY) const
{
  if (m_upDown.HitTest(iPosX, iPosY))
    return true;
  return CGUIControl::HitTest(iPosX, iPosY);
}

void CGUIListControl::OnMouseOver()
{
  // check if we are near the spin control
  if (m_upDown.HitTest(g_Mouse.iPosX, g_Mouse.iPosY))
  {
    m_upDown.OnMouseOver();
  }
  else
  {
    m_upDown.SetFocus(false);
    // select the item under the pointer
    if (SelectItemFromPoint(g_Mouse.iPosX - m_iPosX, g_Mouse.iPosY - m_iPosY))
      CGUIControl::OnMouseOver();
  }
}

void CGUIListControl::OnMouseClick(DWORD dwButton)
{
  if (m_upDown.HitTest(g_Mouse.iPosX, g_Mouse.iPosY))
  {
    m_upDown.OnMouseClick(dwButton);
  }
  else
  {
    if (SelectItemFromPoint(g_Mouse.iPosX - m_iPosX, g_Mouse.iPosY - m_iPosY))
    { // send click message to window
      SEND_CLICK_MESSAGE(GetID(), GetParentID(), ACTION_MOUSE_CLICK + dwButton);
    }
  }
}

void CGUIListControl::OnMouseDoubleClick(DWORD dwButton)
{
  if (m_upDown.HitTest(g_Mouse.iPosX, g_Mouse.iPosY))
  {
    m_upDown.OnMouseDoubleClick(dwButton);
  }
  else
  {
    if (SelectItemFromPoint(g_Mouse.iPosX - m_iPosX, g_Mouse.iPosY - m_iPosY))
    { // send double click message to window
      SEND_CLICK_MESSAGE(GetID(), GetParentID(), ACTION_MOUSE_DOUBLE_CLICK + dwButton);
    }
  }
}

void CGUIListControl::OnMouseWheel()
{
  if (m_upDown.HitTest(g_Mouse.iPosX, g_Mouse.iPosY))
  {
    m_upDown.OnMouseWheel();
  }
  else
  { // scroll
    Scroll( -g_Mouse.cWheel);
  }
}

bool CGUIListControl::CanFocus() const
{
  //if (m_vecItems.size()<=0)
  //  return false;

  return CGUIControl::CanFocus();
}

void CGUIListControl::SetNavigation(DWORD dwUp, DWORD dwDown, DWORD dwLeft, DWORD dwRight)
{
  CGUIControl::SetNavigation(dwUp, dwDown, dwLeft, dwRight);
  m_upDown.SetNavigation(0, 0, 0, dwRight);
}

void CGUIListControl::SetPosition(int iPosX, int iPosY)
{
  // offset our spin control by the appropriate amount
  int iSpinOffsetX = m_upDown.GetXPosition() - GetXPosition();
  int iSpinOffsetY = m_upDown.GetYPosition() - GetYPosition();
  CGUIControl::SetPosition(iPosX, iPosY);
  m_upDown.SetPosition(GetXPosition() + iSpinOffsetX, GetYPosition() + iSpinOffsetY);
}

void CGUIListControl::SetWidth(int iWidth)
{
  int iSpinOffsetX = m_upDown.GetXPosition() - GetXPosition() - GetWidth();
  CGUIControl::SetWidth(iWidth);
  m_imgButton.SetWidth(m_dwWidth);
  m_upDown.SetPosition(GetXPosition() + GetWidth() + iSpinOffsetX, m_upDown.GetYPosition());
}

void CGUIListControl::SetHeight(int iHeight)
{
  int iSpinOffsetY = m_upDown.GetYPosition() - GetYPosition() - GetHeight();
  CGUIControl::SetHeight(iHeight);
  m_imgButton.SetHeight(m_iItemHeight);
  m_upDown.SetPosition(m_upDown.GetXPosition(), GetYPosition() + GetHeight() + iSpinOffsetY);

  float fHeight = (float)m_iItemHeight + (float)m_iSpaceBetweenItems;
  float fTotalHeight = (float)(m_dwHeight - m_upDown.GetHeight() - 5);
  m_iItemsPerPage = (int)(fTotalHeight / fHeight );

  int iPages = m_vecItems.size() / m_iItemsPerPage;
  if (m_vecItems.size() % m_iItemsPerPage) iPages++;
  m_upDown.SetRange(1, iPages);
}

void CGUIListControl::SetPulseOnSelect(bool pulse)
{
  m_imgButton.SetPulseOnSelect(pulse);
  m_upDown.SetPulseOnSelect(pulse);
  CGUIControl::SetPulseOnSelect(pulse);
}

CStdString CGUIListControl::GetDescription() const
{
  CStdString strLabel;
  int iItem = m_iCursorY + m_iOffset;
  if (iItem >= 0 && iItem < (int)m_vecItems.size())
  {
    CGUIListItem *pItem = m_vecItems[iItem];
    strLabel = pItem->GetLabel();
    if (pItem->m_bIsFolder)
    {
      strLabel.Format("[%s]", pItem->GetLabel().c_str());
    }
  }
  return strLabel;
}