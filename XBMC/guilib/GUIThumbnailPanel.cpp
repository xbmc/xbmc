#include "include.h"
#include "GUIThumbnailPanel.h"
#include "../xbmc/utils/CharsetConverter.h"
#include "..\xbmc\settings.h"
#include "GUIWindowManager.h"

#define CONTROL_LIST  0
#define CONTROL_UPDOWN 9998

/*
total width/height for an item including space between items:
  <itemWidth>128</itemWidth>
 <itemHeight>128</itemHeight>
  
width/height of folder icon:
  <textureWidth>80</textureWidth>
  <textureHeight>80</textureHeight> 
 
width/height of thumbnail
  <thumbWidth>128</thumbWidth>
  <thumbHeight>100</thumbHeight>
 
relative position of thumbnail in the folder icon
  <thumbPosX>4</thumbPosX>
  <thumbPosY>16</thumbPosY>
*/

CGUIThumbnailPanel::CGUIThumbnailPanel(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height,
                                       const CImage& imageIcon,
                                       const CImage& imageIconFocus,
                                       float spinWidth, float spinHeight,
                                       const CImage& textureUp, const CImage& textureDown,
                                       const CImage& textureUpFocus, const CImage& textureDownFocus,
                                       const CLabelInfo& spinInfo, float spinX, float spinY,
                                       const CLabelInfo &labelInfo)
    : CGUIControl(dwParentID, dwControlId, posX, posY, width, height)
    , m_imgFolder(dwParentID, dwControlId, posX, posY, 0, 0, imageIcon)
    , m_imgFolderFocus(dwParentID, dwControlId, posX, posY, 0, 0, imageIconFocus)
    , m_upDown(dwControlId, CONTROL_UPDOWN, 0, 0, spinWidth, spinHeight, textureUp, textureDown, textureUpFocus, textureDownFocus, spinInfo, SPIN_CONTROL_TYPE_INT)
    , m_scrollInfo(0)
{
  m_label = labelInfo;
  m_upDown.SetSpinAlign(XBFONT_CENTER_Y | XBFONT_RIGHT, 0);
  m_itemWidth = 0;
  m_itemHeight = 0;
  m_iRowOffset = 0;
  m_fSmoothScrollOffset = 0;
  m_iSelect = CONTROL_LIST;
  m_iCursorY = 0;
  m_iCursorX = 0;
  m_spinPosX = spinX;
  m_spinPosY = spinY;
  m_strSuffix = "|";
  m_bScrollUp = false;
  m_bScrollDown = false;
  m_iScrollCounter = 0;
  m_iLastItem = -1;
  m_textureWidth = 80;
  m_textureHeight = 80;
  m_thumbAlign = 0;
  m_thumbWidth = 64;
  m_thumbHeight = 64;
  m_thumbXPos = 8;
  m_thumbYPos = 8;
  m_labelState = SHOW_ALL;
  m_pageControlVisible = true;   // show the spin control by default
  m_pageControl = 0;
  m_usingBigIcons = false;
  m_upDown.SetShowRange(true); // show the range by default
  m_aspectRatio = CGUIImage::ASPECT_RATIO_KEEP;
  ControlType = GUICONTROL_THUMBNAIL;
}

CGUIThumbnailPanel::~CGUIThumbnailPanel(void)
{}

void CGUIThumbnailPanel::RenderItem(bool bFocus, float posX, float posY, CGUIListItem* pItem, int iStage)
{
  float textPosY = posY + m_textureHeight;
  float centeredPosX = posX + (m_itemWidth - m_textureWidth)*0.5f;

  if (iStage == 0) //render images
  {
    if (bFocus && HasFocus() && m_iSelect == CONTROL_LIST )
    {
      m_imgFolderFocus.SetPosition(centeredPosX, posY);
      m_imgFolderFocus.Render();
    }
    else
    {
      m_imgFolder.SetPosition(centeredPosX, posY);
      m_imgFolder.Render();
    }
    CStdString strThumb = pItem->GetThumbnailImage();
    if (strThumb.IsEmpty() && pItem->HasIcon())
    { // no thumbnail, but it does have an icon
      strThumb = pItem->GetIconImage();
      strThumb.Insert(strThumb.Find("."), "Big");
    }
    if (!strThumb.IsEmpty())
    {
      CGUIImage *thumb = pItem->GetThumbnail();
      if (!thumb )
      {
        thumb = new CGUIImage(0, 0, m_thumbXPos + centeredPosX, m_thumbYPos + posY, m_thumbWidth, m_thumbHeight, strThumb, 0x0);
        thumb->SetAspectRatio(m_aspectRatio);
        pItem->SetThumbnail(thumb);
      }

      if (thumb)
      {
        float xOff = 0;
        float yOff = 0;
        //only supports center yet, 0 is default meaning use x/y position
        if (m_thumbAlign != 0)
        {
          xOff += (m_textureWidth - m_thumbWidth) * 0.5f;
          yOff += (m_textureHeight - m_thumbHeight) * 0.5f;
          //if thumbPosX or thumbPosX != 0 the thumb will be bumped off-center
        }
        // set file name to make sure it's always up to date (does nothing if it is)
        thumb->SetFileName(strThumb);
        if (!thumb->IsAllocated())
          thumb->AllocResources();
        thumb->SetPosition(m_thumbXPos + centeredPosX + xOff, m_thumbYPos + posY + yOff);
        thumb->Render();

        // Add the overlay image
        CGUIImage *overlay = pItem->GetOverlay();
        if (!overlay && pItem->HasOverlay())
        {
          overlay = new CGUIImage(0, 0, 0, 0, 0, 0, pItem->GetOverlayImage(), 0x0);
          overlay->SetAspectRatio(m_aspectRatio);
          overlay->AllocResources();
          pItem->SetOverlay(overlay);
        }
        // Render the image
        if (overlay)
        {
          float x, y;
          thumb->GetBottomRight(x, y);
          if (m_usingBigIcons)
          {
            overlay->SetWidth((float)overlay->GetTextureWidth());
            overlay->SetHeight((float)overlay->GetTextureWidth());
          }
          else
          {
            float scale = (m_thumbHeightBig) ? m_thumbHeight / m_thumbHeightBig : 1.0f;
            overlay->SetWidth(overlay->GetTextureWidth() * scale);
            overlay->SetHeight(overlay->GetTextureHeight() * scale);
          }
          // if we haven't yet rendered, make sure we update our sizing
          if (!overlay->HasRendered())
            overlay->CalculateSize();
          overlay->SetPosition(x - overlay->GetRenderWidth(), y - overlay->GetRenderHeight());
          overlay->Render();
        }
      }
    }
  }
  if (iStage == 1) //render text
  {
    // hide filenames in thumbnail panel
    if (m_labelState == HIDE_ALL)
      return;
    if (m_labelState == HIDE_FILES && !pItem->m_bIsFolder)
      return;
    if (m_labelState == HIDE_FOLDERS && pItem->m_bIsFolder)
      return;

    CStdStringW strItemLabelUnicode;
    g_charsetConverter.utf8ToUTF16(pItem->GetLabel().c_str(), strItemLabelUnicode);

    DWORD dwColor = m_label.textColor;
    if (pItem->IsSelected())
      dwColor = m_label.selectedColor;
    else if (bFocus && HasFocus() && m_label.focusedColor)
      dwColor = m_label.focusedColor;
    if (bFocus && HasFocus() && m_iSelect == CONTROL_LIST )
    {
      RenderText(posX, textPosY, dwColor, (WCHAR*) strItemLabelUnicode.c_str(), true);
    }
    else
    {
      RenderText(posX, textPosY, dwColor, (WCHAR*) strItemLabelUnicode.c_str(), false);
    }
  }
}

void CGUIThumbnailPanel::Render()
{
  if (!IsVisible()) return;

  if (!ValidItem(m_iCursorX, m_iCursorY) )
  {
    m_iCursorX = 0;
    m_iCursorY = 0;
    m_iRowOffset = 0;
  }

  float scrollYOffset = 0;
  if (m_bScrollDown)
  {
    scrollYOffset = -(m_itemHeight - m_iScrollCounter);
  }
  if (m_bScrollUp)
  {
    scrollYOffset = m_itemHeight - m_iScrollCounter;
  }

  g_graphicsContext.SetViewPort(m_posX, m_posY, m_iColumns*m_itemWidth, m_iRows*m_itemHeight);

  //free memory of thumbs that are not going to be displayed
  int iStartItem = m_iRowOffset * m_iColumns;
  if (m_bScrollUp)
  {
    iStartItem -= m_iColumns;
  }
  int iEndItem = (m_iRowOffset + m_iRows) * m_iColumns - 1;
  if (m_bScrollDown)
  {
    iEndItem += m_iColumns;
  }
  if (iStartItem < 30000)
  {
    for (int i = 0; i < iStartItem;++i)
    {
      CGUIListItem *pItem = m_vecItems[i];
      if (pItem)
      {
        pItem->FreeMemory();
      }
    }
  }
  for (int i = iEndItem + 1; i < (int)m_vecItems.size(); ++i)
  {
    CGUIListItem *pItem = m_vecItems[i];
    if (pItem)
    {
      pItem->FreeMemory();
    }
  }

  if (m_label.font)
  {
    //render in 2 loops, first the images, second all text (batched between Begin()/End())
    for (int iStage = 0; iStage <= 1; iStage++)
    {
      if (iStage == 1) //text rendering
      {
        m_label.font->Begin();
      }

      if (m_bScrollUp && m_iRowOffset > 0)
      {
        // render item on top
        float posY = m_posY - m_itemHeight + scrollYOffset;
        m_iRowOffset --;
        for (int iCol = 0; iCol < m_iColumns; iCol++)
        {
          float posX = m_posX + iCol * m_itemWidth;
          int iItem = iCol + m_iRowOffset * m_iColumns;
          if (iItem >= 0 && iItem < (int)m_vecItems.size())
          {
            CGUIListItem *pItem = m_vecItems[iItem];
            RenderItem(false, posX, posY, pItem, iStage);
          }
        }
        m_iRowOffset++;
      }

      // render main panel
      for (int iRow = 0; iRow < m_iRows; iRow++)
      {
        float posY = m_posY + iRow * m_itemHeight + scrollYOffset;
        for (int iCol = 0; iCol < m_iColumns; iCol++)
        {
          float posX = m_posX + iCol * m_itemWidth;
          int iItem = (iRow + m_iRowOffset) * m_iColumns + iCol;
          if (iItem < (int)m_vecItems.size())
          {
            CGUIListItem *pItem = m_vecItems[iItem];
            bool bFocus = (m_iCursorX == iCol && m_iCursorY == iRow );
            RenderItem(bFocus, posX, posY, pItem, iStage);
          }
        }
      }

      if (m_bScrollDown)
      {
        // render item on bottom
        float posY = m_posY + m_iRows * m_itemHeight + scrollYOffset;
        for (int iCol = 0; iCol < m_iColumns; iCol++)
        {
          float posX = m_posX + iCol * m_itemWidth;
          int iItem = (iRow + m_iRowOffset) * m_iColumns + iCol;
          if (iItem < (int)m_vecItems.size())
          {
            CGUIListItem *pItem = m_vecItems[iItem];
            RenderItem(false, posX, posY, pItem, iStage);
          }
        }
      }
      if (iStage == 1)
      { //end text rendering
        m_label.font->End();
      }
    }
  }

  g_graphicsContext.RestoreViewPort();

  //
  int iFrames = 12;
  int iStep = (int)m_itemHeight / iFrames;
  if (!iStep) iStep = 1;
  if (m_bScrollDown)
  {
    m_iScrollCounter -= iStep;
    if (m_iScrollCounter <= 0)
    {
      m_bScrollDown = false;
      m_iRowOffset++;
      // Check if we need to update our position
      if (!ValidItem(m_iCursorX, m_iCursorY))
      { // select the last item available
        int iPos = m_vecItems.size() - 1 - m_iRowOffset * m_iColumns;
        m_iCursorY = iPos / m_iColumns;
        m_iCursorX = iPos % m_iColumns;
      }
      // Update the page counter
      UpdatePageControl();
    }
  }
  if (m_bScrollUp)
  {
    m_iScrollCounter -= iStep;
    if (m_iScrollCounter <= 0 && m_iRowOffset > 0)
    {
      m_bScrollUp = false;
      m_iRowOffset--;
      UpdatePageControl();
    }
  }
  if (m_pageControlVisible && m_upDown.GetMaximum() > 1)
  {
    m_upDown.SetPosition(m_posX + m_spinPosX, m_posY + m_spinPosY);
    m_upDown.Render();
  }
  CGUIControl::Render();
}

bool CGUIThumbnailPanel::OnAction(const CAction &action)
{
  switch (action.wID)
  {
  case ACTION_PAGE_UP:
    OnPageUp();
    return false;
    break;

  case ACTION_PAGE_DOWN:
    OnPageDown();
    return false;
    break;

  case ACTION_SCROLL_UP:
    {
      m_fSmoothScrollOffset += action.fAmount1 * action.fAmount1;
      while (m_fSmoothScrollOffset > 10.0f / m_iRows)
      {
        m_fSmoothScrollOffset -= 10.0f / m_iRows;
        if (m_iRowOffset > 0 && m_iCursorX <= m_iColumns / 2 && m_iCursorY <= m_iRows / 2)
        {
          ScrollUp();
        }
        else if (m_iCursorX > 0)
        {
          m_iCursorX--;
        }
        else if (m_iCursorY > 0)
        {
          m_iCursorY--;
          m_iCursorX = m_iColumns - 1;
        }
      }
      return false;
    }
    break;
  case ACTION_SCROLL_DOWN:
    {
      int iItemsPerPage = m_iRows * m_iColumns;
      m_fSmoothScrollOffset += action.fAmount1 * action.fAmount1;
      while (m_fSmoothScrollOffset > 10.0f / m_iRows)
      {
        m_fSmoothScrollOffset -= 10.0f / m_iRows;
        if (m_iRowOffset*m_iColumns + iItemsPerPage < (int)m_vecItems.size() && m_iCursorX >= m_iColumns / 2 && m_iCursorY >= m_iRows / 2)
        {
          ScrollDown();
        }
        else if (m_iCursorX < m_iColumns - 1 && (m_iRowOffset + m_iCursorY)*m_iColumns + m_iCursorX < (int)m_vecItems.size() - 1)
        {
          m_iCursorX++;
        }
        else if (m_iCursorY < m_iRows - 1 && (m_iRowOffset + m_iCursorY)*m_iColumns + m_iCursorX < (int)m_vecItems.size() - 1)
        {
          m_iCursorY++;
          m_iCursorX = 0;
        }
      }
      return false;
    }
    break;

  case ACTION_MOVE_DOWN:
  case ACTION_MOVE_UP:
  case ACTION_MOVE_LEFT:
  case ACTION_MOVE_RIGHT:
    { // use the base class
      return CGUIControl::OnAction(action);
    }
    break;

  default:
    {
      if (m_iSelect == CONTROL_LIST && action.wID)
      {
        CGUIMessage msg(GUI_MSG_CLICKED, GetID(), GetParentID(), action.wID);
        return g_graphicsContext.SendMessage(msg);
      }
      else
      {
        return m_upDown.OnAction(action);
      }
    }
  }
}

bool CGUIThumbnailPanel::OnMessage(CGUIMessage& message)
{
  if (message.GetControlId() == GetID() )
  {
    if (message.GetSenderId() == CONTROL_UPDOWN)
    {
      if (message.GetMessage() == GUI_MSG_CLICKED)
      { // Page Control
        GetOffsetFromPage();
      }
    }
    if (message.GetMessage() == GUI_MSG_LOSTFOCUS ||
        message.GetMessage() == GUI_MSG_SETFOCUS)
    {
      m_iSelect = CONTROL_LIST;
    }
    if (message.GetMessage() == GUI_MSG_LABEL_ADD)
    {
      m_vecItems.push_back( (CGUIListItem*) message.GetLPVOID() );
      int iItemsPerPage = m_iRows * m_iColumns;
      int iPages = m_vecItems.size() / iItemsPerPage;
      if (m_vecItems.size() % iItemsPerPage) iPages++;
      m_upDown.SetRange(1, iPages);
      m_upDown.SetValue(1);
      if (m_pageControl)
      {
        int totalRows = (m_vecItems.size() + m_iColumns - 1) / m_iColumns;
        CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), m_pageControl, m_iRows, totalRows);
        SendWindowMessage(msg);
      }
      return true;
    }

    if (message.GetMessage() == GUI_MSG_LABEL_RESET)
    {
      m_vecItems.erase(m_vecItems.begin(), m_vecItems.end());
      m_upDown.SetRange(1, 1);
      m_upDown.SetValue(1);
      if (m_pageControl)
      {
        int totalRows = (m_vecItems.size() + m_iColumns - 1) / m_iColumns;
        CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), m_pageControl, m_iRows, totalRows);
        SendWindowMessage(msg);
      }
      m_iCursorX = m_iCursorY = 0;
      // don't reset our row offset here - it's taken care of in SELECT
      //m_iRowOffset = 0;
      m_bScrollUp = false;
      m_bScrollDown = false;
      return true;
    }
    if (message.GetMessage() == GUI_MSG_ITEM_SELECTED)
    {
      message.SetParam1((m_iRowOffset + m_iCursorY) * m_iColumns + m_iCursorX);
      return true;
    }
    if (message.GetMessage() == GUI_MSG_ITEM_SELECT)
    {
      SetSelectedItem(message.GetParam1());
      return true;
    }
    if (message.GetMessage() == GUI_MSG_PAGE_CHANGE)
    {
      if (message.GetSenderId() == m_pageControl)
      { // update our page number
        m_iRowOffset = message.GetParam1();
        return true;
      }
    }
  }

  return CGUIControl::OnMessage(message);
}

void CGUIThumbnailPanel::PreAllocResources()
{
  if (!m_label.font) return;
  CGUIControl::PreAllocResources();
  m_upDown.PreAllocResources();
  m_imgFolder.PreAllocResources();
  m_imgFolderFocus.PreAllocResources();
}

void CGUIThumbnailPanel::Calculate(bool resetItem)
{
  m_imgFolder.SetWidth(m_textureWidth);
  m_imgFolder.SetHeight(m_textureHeight);
  m_imgFolderFocus.SetWidth(m_textureWidth);
  m_imgFolderFocus.SetHeight(m_textureHeight);
  m_iLastItem = -1;
  float fWidth, fHeight;

  fWidth = m_itemWidth;
  fHeight = m_itemHeight;
  float fTotalHeight = m_height - 5;
  m_iRows = (int)(fTotalHeight / fHeight);
  m_iColumns = (int) (m_width / fWidth );

  // calculate the number of pages
  int iItemsPerPage = m_iRows * m_iColumns;
  int iPages = m_vecItems.size() / iItemsPerPage;
  if (m_vecItems.size() % iItemsPerPage) iPages++;
  m_upDown.SetRange(1, iPages);
  if (resetItem)
  {
    m_upDown.SetValue(1);
    int iItem = (m_iRowOffset + m_iCursorY) * m_iColumns + m_iCursorX;
    SetSelectedItem(iItem);
  }
  if (m_pageControl)
  {
    int totalRows = (m_vecItems.size() + m_iColumns - 1) / m_iColumns;
    CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), m_pageControl, m_iRows, totalRows);
    SendWindowMessage(msg);
  }
}

void CGUIThumbnailPanel::AllocResources()
{
  if (!m_label.font) return ;
  CGUIControl::AllocResources();
  m_upDown.AllocResources();
  m_imgFolder.AllocResources();
  m_imgFolderFocus.AllocResources();
  if (!m_itemHeight) m_itemHeight = m_itemHeightLow ? m_itemHeightLow : 100;
  if (!m_itemWidth) m_itemWidth = m_itemWidthLow ? m_itemWidthLow : 100;
  Calculate(true);
}

void CGUIThumbnailPanel::FreeResources()
{
  CGUIControl::FreeResources();
  m_upDown.FreeResources();
  m_imgFolder.FreeResources();
  m_imgFolderFocus.FreeResources();
}

void CGUIThumbnailPanel::DynamicResourceAlloc(bool bOnOff)
{
  CGUIControl::DynamicResourceAlloc(bOnOff);
  m_upDown.DynamicResourceAlloc(bOnOff);
  m_imgFolder.DynamicResourceAlloc(bOnOff);
  m_imgFolderFocus.DynamicResourceAlloc(bOnOff);
}

bool CGUIThumbnailPanel::ValidItem(int iX, int iY)
{
  if (iX >= m_iColumns) return false;
  if (iY >= m_iRows) return false;
  if ((m_iRowOffset + iY)*m_iColumns + iX < (int)m_vecItems.size() ) return true;
  return false;
}
void CGUIThumbnailPanel::OnRight()
{
  if (m_iSelect == CONTROL_LIST)
  {
    if (m_iCursorX + 1 < m_iColumns && ValidItem(m_iCursorX + 1, m_iCursorY) )
    {
      m_iCursorX++;
      return ;
    }

    if (m_pageControlVisible && m_upDown.GetMaximum() > 1)
    {
      m_iSelect = CONTROL_UPDOWN;
      m_upDown.SetFocus(true);
    }
    else
      CGUIControl::OnRight();
  }
  else if (!m_upDown.IsFocusedOnUp())
    m_upDown.OnRight();
  else
  { // focus on our list and do the base move right
    m_upDown.SetFocus(false);
    m_iSelect = CONTROL_LIST;
    CGUIControl::OnRight();
  }
}

void CGUIThumbnailPanel::OnLeft()
{
  if (m_iSelect == CONTROL_LIST)
  {
    if (m_iCursorX > 0)
    {
      m_iCursorX--;
      return ;
    }
    CGUIControl::OnLeft();
  }
  else if (m_upDown.IsFocusedOnUp())
    m_upDown.OnLeft();
  else
  {
    m_iSelect = CONTROL_LIST;
    m_upDown.SetFocus(false);
  }
}

void CGUIThumbnailPanel::OnUp()
{
  if (m_iSelect == CONTROL_LIST)
  {
    if (m_iCursorY > 0)
    {
      m_iCursorY--;
    }
    else if (m_iCursorY == 0 && m_iRowOffset > 0)
    {
      ScrollUp();
    }
    else
    {
      if (m_dwControlUp == GetID() && m_vecItems.size() > 0)
      { // move 2 last item in list
        SetSelectedItem(m_vecItems.size() - 1);
      }
      else
        CGUIControl::OnUp();
    }
  }
  else
  { // focus the list again
    m_upDown.SetFocus(false);
    m_iSelect = CONTROL_LIST;
  }
}

void CGUIThumbnailPanel::OnDown()
{
  if (m_iSelect == CONTROL_LIST)
  {
    if (m_iCursorY + 1 == m_iRows && ScrollDown())
    {
      return;
    }
    if ( ValidItem(m_iCursorX, m_iCursorY + 1) )
    {
      m_iCursorY++;
    }
    else
    {
      if (m_dwControlDown == GetID() && m_vecItems.size())
        SetSelectedItem(0);
      else
        CGUIControl::OnDown();
    }
  }
  else
  {
    // move down off our control
    m_upDown.SetFocus(false);
    CGUIControl::OnDown();
  }
}

void CGUIThumbnailPanel::RenderText(float fPosX, float fPosY, DWORD dwTextColor, WCHAR* wszText, bool bScroll )
{
  if (!m_label.font) return ;
  static int iLastItem = -1;

  float fTextHeight, fTextWidth;
  m_label.font->GetTextExtent( wszText, &fTextWidth, &fTextHeight);
  float fMaxWidth = m_itemWidth * 0.9f;
  fPosX += (m_itemWidth - fMaxWidth) * 0.5f;
  if (!bScroll)
  {
    // Center text to make it look nicer...
    if (fTextWidth <= fMaxWidth)
      fPosX += (fMaxWidth - fTextWidth) * 0.5f;
    m_label.font->DrawTextWidth(fPosX, fPosY, dwTextColor, m_label.shadowColor, wszText, fMaxWidth);
    return ;
  }
  else
  {
    if (fTextWidth <= fMaxWidth)
    { // Center text to make it look nicer...
      fPosX += (fMaxWidth - fTextWidth) / 2;
      m_label.font->DrawTextWidth(fPosX, fPosY, dwTextColor, m_label.shadowColor, wszText, fMaxWidth);
      iLastItem = -1; // reset scroller
      return ;
    }

    // scroll
    int iItem = m_iCursorX + (m_iCursorY + m_iRowOffset) * m_iColumns;
    CStdStringW scrollString = wszText;
    scrollString += L" ";
    scrollString += m_strSuffix;

    m_label.font->End(); //deinit fontbatching before setting a new viewport
    if (iLastItem != iItem)
    {
      m_scrollInfo.Reset();
      iLastItem = iItem;
    }
    m_label.font->DrawScrollingText(fPosX, fPosY, &dwTextColor, 1, m_label.shadowColor, scrollString, fMaxWidth, m_scrollInfo);
    m_label.font->Begin(); //resume fontbatching
  }
}

void CGUIThumbnailPanel::SetScrollySuffix(const CStdString &strSuffix)
{
  m_strSuffix = strSuffix;
}

void CGUIThumbnailPanel::OnPageUp()
{
  m_iRowOffset -= m_iRows;
  if (m_iRowOffset < 0) m_iRowOffset = 0;
  UpdatePageControl();
}

void CGUIThumbnailPanel::OnPageDown()
{
  int totalRows = (m_vecItems.size() + m_iColumns - 1) / m_iColumns;
  m_iRowOffset += m_iRows;
  if (m_iRowOffset >= totalRows - m_iRows)
    m_iRowOffset = totalRows - m_iRows;
  UpdatePageControl();
}

void CGUIThumbnailPanel::GetOffsetFromPage()
{
  m_iRowOffset = (m_upDown.GetValue() - 1) * m_iRows;
  // make sure we have a full screen on the last page.
  int iRowsToGo = m_vecItems.size() / m_iColumns - m_iRowOffset + 1;
  while (iRowsToGo < m_iRows && m_iRowOffset > 0)
  {
    iRowsToGo++;
    m_iRowOffset--;
  }
  while (m_iCursorX > 0 && (m_iRowOffset + m_iCursorY)*m_iColumns + m_iCursorX >= (int) m_vecItems.size() )
  {
    m_iCursorX--;
  }
  while (m_iCursorY > 0 && (m_iRowOffset + m_iCursorY)*m_iColumns + m_iCursorX >= (int) m_vecItems.size() )
  {
    m_iCursorY--;
  }
}

void CGUIThumbnailPanel::SetThumbAlign(int align)
{
  m_thumbAlign = align;
}

void CGUIThumbnailPanel::SetThumbDimensions(float posX, float posY, float width, float height)
{
  m_thumbWidth = width;
  m_thumbHeight = height;
  m_thumbXPos = posX;
  m_thumbYPos = posY;
}

void CGUIThumbnailPanel::SetItemWidth(float width)
{
  m_itemWidth = width;
  FreeResources();
  AllocResources();
}

void CGUIThumbnailPanel::SetItemHeight(float height)
{
  m_itemHeight = height;
  FreeResources();
  AllocResources();
}

void CGUIThumbnailPanel::SetSelectedItem(int iItem)
{
  // check the location of our row offset first
  if (m_iRowOffset > (int)m_vecItems.size() / m_iColumns) m_iRowOffset = m_vecItems.size() / m_iColumns;
  if (m_iRowOffset < 0) m_iRowOffset = 0;

  // now check our item is in the valid range
  if (iItem < 0 || iItem >= (int)m_vecItems.size())
    return;

  // move to the appropriate page on the thumb control
  if (iItem < m_iRowOffset * (m_iRows * m_iColumns))
  { // before the page we need to be on - make it in the first row
    m_iRowOffset = iItem / m_iColumns;
  }
  else if (iItem >= (m_iRowOffset + m_iRows) * m_iColumns)
  { // after the page we are on - make it the last row
    m_iRowOffset = max(iItem / m_iColumns - m_iRows + 1, 0);
  }
  int item = iItem - m_iRowOffset * m_iColumns;
  m_iCursorY = item / m_iColumns;
  m_iCursorX = item % m_iColumns;
  UpdatePageControl();
}

void CGUIThumbnailPanel::ShowBigIcons(bool bOnOff)
{
  m_usingBigIcons = bOnOff;
  if (bOnOff)
  {
    m_itemWidth = m_itemWidthBig;
    m_itemHeight = m_itemHeightBig;
    m_textureWidth = m_textureWidthBig;
    m_textureHeight = m_textureHeightBig;
    SetThumbDimensions(m_thumbXPosBig, m_thumbYPosBig, m_thumbWidthBig, m_thumbHeightBig);
  }
  else
  {
    m_itemWidth = m_itemWidthLow;
    m_itemHeight = m_itemHeightLow;
    m_textureWidth = m_textureWidthLow;
    m_textureHeight = m_textureHeightLow;
    SetThumbDimensions(m_thumbXPosLow, m_thumbYPosLow, m_thumbWidthLow, m_thumbHeightLow);
  }
  Calculate(true);
}

bool CGUIThumbnailPanel::HitTest(float posX, float posY) const
{
  if (m_upDown.HitTest(posX, posY))
    return true;
  return CGUIControl::HitTest(posX, posY);
}

bool CGUIThumbnailPanel::OnMouseOver()
{
  // check if we are near the spin control
  if (m_upDown.HitTest(g_Mouse.posX, g_Mouse.posY))
  {
    if (m_upDown.OnMouseOver())
      m_upDown.SetFocus(true);
  }
  else
  {
    m_upDown.SetFocus(false);
    // select the item under the pointer
    if (SelectItemFromPoint(g_Mouse.posX - m_posX, g_Mouse.posY - m_posY))
      return CGUIControl::OnMouseOver();
  }
  return false;
}

bool CGUIThumbnailPanel::OnMouseClick(DWORD dwButton)
{
  if (m_upDown.HitTest(g_Mouse.posX, g_Mouse.posY))
  {
    return m_upDown.OnMouseClick(dwButton);
  }
  else
  {
    if (SelectItemFromPoint(g_Mouse.posX - m_posX, g_Mouse.posY - m_posY))
    {
      SEND_CLICK_MESSAGE(GetID(), GetParentID(), ACTION_MOUSE_CLICK + dwButton);
      return true;
    }
  }
  return false;
}

bool CGUIThumbnailPanel::OnMouseDoubleClick(DWORD dwButton)
{
  if (m_upDown.HitTest(g_Mouse.posX, g_Mouse.posY))
  {
    return m_upDown.OnMouseClick(dwButton);
  }
  else
  {
    if (SelectItemFromPoint(g_Mouse.posX - m_posX, g_Mouse.posY - m_posY))
    {
      SEND_CLICK_MESSAGE(GetID(), GetParentID(), ACTION_MOUSE_DOUBLE_CLICK + dwButton);
      return true;
    }
  }
  return false;
}

bool CGUIThumbnailPanel::OnMouseWheel()
{
  if (m_upDown.HitTest(g_Mouse.posX, g_Mouse.posY))
  {
    return m_upDown.OnMouseWheel();
  }
  else
  {
    if (g_Mouse.cWheel > 0)
      ScrollUp();
    else
      ScrollDown();
    return true;
  }
}

bool CGUIThumbnailPanel::SelectItemFromPoint(float posX, float posY)
{
  int x = (int)(posX / m_itemWidth);
  int y = (int)(posY / m_itemHeight);
  if (ValidItem(x, y))
  { // valid item - check width constraints
    if (posX > x*m_itemWidth + (m_itemWidth - m_textureWidth) / 2 && posX < x*m_itemWidth + m_textureWidth + (m_itemWidth - m_textureWidth) / 2)
    { // width ok - check height constraints
      float fTextHeight, fTextWidth;
      m_label.font->GetTextExtent( L"Yy", &fTextWidth, &fTextHeight);
      if (posY > y*m_itemHeight && posY < y*m_itemHeight + m_textureHeight + fTextHeight)
      { // height ok - good to go!
        m_iCursorX = x;
        m_iCursorY = y;
        return true;
      }
    }
  }
  // invalid item, or not over a thumb
  return false;
}

bool CGUIThumbnailPanel::ScrollDown()
{
  // Check if we are already scrolling, and stop if we are (to speed up fast scrolls)
  if (m_bScrollDown)
  {
    m_bScrollDown = false;
    m_iRowOffset++;
    // Check if we need to update our position
    if (!ValidItem(m_iCursorX, m_iCursorY))
    { // select the last item available
      int iPos = m_vecItems.size() - 1 - m_iRowOffset * m_iColumns;
      m_iCursorY = iPos / m_iColumns;
      m_iCursorX = iPos % m_iColumns;
    }
    UpdatePageControl();
  }
  // Now scroll down, if we can
  if ((m_iRowOffset + m_iRows)*m_iColumns < (int)m_vecItems.size())
  {
    m_iScrollCounter = (int)m_itemHeight;
    m_bScrollDown = true;
    return true;
  }
  return false;
}

void CGUIThumbnailPanel::ScrollUp()
{
  // If we are already scrolling, then stop (to speed up fast scrolls)
  if (m_bScrollUp)
  {
    m_iScrollCounter = 0;
    m_bScrollUp = false;
    m_iRowOffset --;
    UpdatePageControl();
  }
  // scroll up, if possible
  if (m_iRowOffset > 0)
  {
    m_iScrollCounter = (int)m_itemHeight;
    m_bScrollUp = true;
  }
}

bool CGUIThumbnailPanel::CanFocus() const
{
  //if (m_vecItems.size()<=0)
  //  return false;

  return CGUIControl::CanFocus();
}

void CGUIThumbnailPanel::SetNavigation(DWORD dwUp, DWORD dwDown, DWORD dwLeft, DWORD dwRight)
{
  CGUIControl::SetNavigation(dwUp, dwDown, dwLeft, dwRight);
  m_upDown.SetNavigation(GetID(), dwDown, GetID(), dwRight);
}

void CGUIThumbnailPanel::SetPosition(float posX, float posY)
{
  // offset our spin control by the appropriate amount
  float spinOffsetX = m_upDown.GetXPosition() - GetXPosition();
  float spinOffsetY = m_upDown.GetYPosition() - GetYPosition();
  CGUIControl::SetPosition(posX, posY);
  m_upDown.SetPosition(GetXPosition() + spinOffsetX, GetYPosition() + spinOffsetY);
}

void CGUIThumbnailPanel::SetWidth(float width)
{
  float spinOffsetX = m_upDown.GetXPosition() - GetXPosition() - GetWidth();
  CGUIControl::SetWidth(width);
  m_upDown.SetPosition(GetXPosition() + GetWidth() + spinOffsetX, m_upDown.GetYPosition());
  Calculate(false);
}

void CGUIThumbnailPanel::SetHeight(float height)
{
  float spinOffsetY = m_upDown.GetYPosition() - GetYPosition() - GetHeight();
  CGUIControl::SetHeight(height);
  m_upDown.SetPosition(m_upDown.GetXPosition(), GetYPosition() + GetHeight() + spinOffsetY);
  Calculate(false);
}

void CGUIThumbnailPanel::SetPulseOnSelect(bool pulse)
{
  m_upDown.SetPulseOnSelect(pulse);
  CGUIControl::SetPulseOnSelect(pulse);
}

CStdString CGUIThumbnailPanel::GetDescription() const
{
  CStdString strLabel;
  int iItem = (m_iRowOffset + m_iCursorY) * m_iColumns + m_iCursorX;
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

void CGUIThumbnailPanel::SaveStates(vector<CControlState> &states)
{
  states.push_back(CControlState(GetID(), (m_iRowOffset + m_iCursorY) * m_iColumns + m_iCursorX));
}

void CGUIThumbnailPanel::SetPageControl(DWORD id)
{
  m_pageControl = id;
  if (m_pageControl)
    SetPageControlVisible(false);
}

void CGUIThumbnailPanel::UpdatePageControl()
{
  int iPage = m_iRowOffset / m_iRows + 1;
  if ((m_iRowOffset + m_iRows)*m_iColumns >= (int)m_vecItems.size() && iPage < m_upDown.GetMaximum())
    iPage++; // last page
  m_upDown.SetValue(iPage);
  if (m_pageControl)
  { // tell our pagecontrol (scrollbar or whatever) to update
    CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), m_pageControl, m_iRowOffset);
    SendWindowMessage(msg);
  }
}