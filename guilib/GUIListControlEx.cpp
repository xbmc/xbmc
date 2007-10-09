#include "include.h"
#include "GUIListControlEx.h"


#define CONTROL_LIST  0
#define CONTROL_UPDOWN 9998
CGUIListControlEx::CGUIListControlEx(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height,
                                     float spinWidth, float spinHeight,
                                     const CImage& textureUp, const CImage& textureDown,
                                     const CImage& textureUpFocus, const CImage& textureDownFocus,
                                     const CLabelInfo& spinInfo, float spinX, float spinY,
                                     const CLabelInfo& labelInfo, const CLabelInfo& labelInfo2,
                                     const CImage& textureButton, const CImage& textureButtonFocus)
    : CGUIControl(dwParentID, dwControlId, posX, posY, width, height)
    , m_upDown(dwControlId, CONTROL_UPDOWN, 0, 0, spinWidth, spinHeight, textureUp, textureDown, textureUpFocus, textureDownFocus, spinInfo, SPIN_CONTROL_TYPE_INT)
    , m_imgButton(dwControlId, 0, posX, posY, width, height, textureButtonFocus, textureButton, labelInfo)
{
  m_upDown.SetSpinAlign(XBFONT_CENTER_Y | XBFONT_RIGHT, 0);
  m_pList = NULL;
  m_iOffset = 0;
  m_iItemsPerPage = 10;
  m_itemHeight = 10;
  m_label = labelInfo;
  m_label2 = labelInfo2;
  m_iSelect = CONTROL_LIST;
  m_iCursorY = 0;
  m_strSuffix = "|";
  m_imageWidth = 16;
  m_imageHeight = 16;
  m_spaceBetweenItems = 4;
  m_bUpDownVisible = true; // show the spin control by default

  m_spinPosX = spinX;
  m_spinPosY = spinY;
  ControlType = GUICONTROL_LISTEX;
}

CGUIListControlEx::~CGUIListControlEx(void)
{
}

void CGUIListControlEx::Render()
{
  if ( m_pList && m_label.font )
  {
    float posY = m_posY;
    CGUIList::GUILISTITEMS& list = m_pList->Lock();

    for (int i = 0; i < m_iItemsPerPage; i++)
    {
      float posX = m_posX;
      if (i + m_iOffset < (int)list.size() )
      {
        CGUIItem* pItem = list[i + m_iOffset];

        // create a list item rendering context
        CGUIListExItem::RenderContext context;
        context.m_positionX = posX;
        context.m_positionY = posY;
        context.m_bFocused = (i == m_iCursorY && HasFocus() && m_iSelect == CONTROL_LIST);
        context.m_bActive = (i == m_iCursorY);
        context.m_pButton = &m_imgButton;
        context.m_label = m_label;

        // render the list item
        pItem->OnPaint(&context);

        posY += m_itemHeight + m_spaceBetweenItems;
      }
    }
    if (m_bUpDownVisible)
    {
      int iPages = list.size() / m_iItemsPerPage;

      if (list.size() % m_iItemsPerPage)
      {
        iPages++;
      }
      m_upDown.SetPosition(m_posX + m_spinPosX, m_posY + m_spinPosY);
      m_upDown.SetRange(1, iPages);
      m_upDown.SetValue(GetPage(list.size()));
      m_upDown.Render();
    }
    m_pList->Release();
  }
  CGUIControl::Render();
}

// returns which page we are on
int CGUIListControlEx::GetPage(int listSize)
{
  if (m_iOffset >= listSize - m_iItemsPerPage)
  {
    m_iOffset = listSize - m_iItemsPerPage;
    if (m_iOffset <= 0)
    {
      m_iOffset = 0;
      return 1;
    }
    if (listSize % m_iItemsPerPage)
      return listSize / m_iItemsPerPage + 1;
    else
      return listSize / m_iItemsPerPage;
  }
  else
    return m_iOffset / m_iItemsPerPage + 1;
}

bool CGUIListControlEx::OnAction(const CAction &action)
{
  // track state before action
  int iOldItem = m_iCursorY + m_iOffset;

  switch (action.wID)
  {
  case ACTION_PAGE_UP:
    OnPageUp();
    return true;
    break;

  case ACTION_PAGE_DOWN:
    OnPageDown();
    return true;
    break;

    /* TODO: doesn't bloody work does it ;-)
             if you scroll up/down it often crashes and locks up XBMC

    // smooth scrolling (for analog controls)
    case ACTION_SCROLL_UP:
    {
     m_fSmoothScrollOffset+=action.fAmount1*action.fAmount1;
     while (m_fSmoothScrollOffset>0.4)
     {
      m_fSmoothScrollOffset-=0.4f;
      if (m_iOffset>0 && m_iCursorY<=m_iItemsPerPage/2)
      {
       m_iOffset--;
      }
      else if (m_iCursorY>0)
      {
       m_iCursorY--;
      }
     }
    }
    break;
    case ACTION_SCROLL_DOWN:
    {
     CGUIList::GUILISTITEMS& list = m_pList->Lock();
     m_fSmoothScrollOffset+=action.fAmount1*action.fAmount1;
     while (m_fSmoothScrollOffset>0.4)
     {
      m_fSmoothScrollOffset-=0.4f;
      if (m_iOffset + m_iItemsPerPage < (int)list.size() && m_iCursorY>=m_iItemsPerPage/2)
      {
       m_iOffset++;
      }
      else if (m_iCursorY<m_iItemsPerPage-1 && m_iOffset + m_iCursorY < (int)list.size()-1)
      {
       m_iCursorY++;
      }
     }
    }
    break;
    */

  case ACTION_SELECT_ITEM:
    {
      // generate a control clicked event
      if (m_iSelect == CONTROL_LIST)
      { // Don't know what to do, so send to our parent window.
        CGUIMessage msg(GUI_MSG_CLICKED, GetID(), GetParentID(), action.wID);
        return SendWindowMessage(msg);
      }
      else
      { // send action to the page control
        return m_upDown.OnAction(action);
      }
    }
    break;
  }

  // call the base class
  bool handled = CGUIControl::OnAction(action);

  // post event notifications
  switch (action.wID)
  {
  case ACTION_MOVE_UP:
  case ACTION_MOVE_DOWN:
    {
      int iNewItem = m_iCursorY + m_iOffset;

      // determine of the item selection within list control has changed
      if (iOldItem != iNewItem)
      {
        // generate control selection changed event
        CGUIMessage msg(GUI_MSG_SELCHANGED, GetID(), GetParentID(), iOldItem, iNewItem);
        return SendWindowMessage(msg);
      }
      break;
    }
  }
  return handled;
}

bool CGUIListControlEx::OnMessage(CGUIMessage& message)
{
  if (message.GetControlId() == GetID() )
  {
    if (message.GetSenderId() == CONTROL_UPDOWN)
    {
      if (message.GetMessage() == GUI_MSG_CLICKED)
      {
        CGUIList::GUILISTITEMS& list = m_pList->Lock();

        m_iOffset = (m_upDown.GetValue() - 1) * m_iItemsPerPage;
        while (m_iOffset + m_iCursorY >= (int)list.size()) m_iCursorY--;

        m_pList->Release();
      }
    }

    if (message.GetMessage() == GUI_MSG_LOSTFOCUS ||
        message.GetMessage() == GUI_MSG_SETFOCUS)
    {
      m_iSelect = CONTROL_LIST;
    }

    if (message.GetMessage() == GUI_MSG_LABEL_BIND)
    {
      CGUIList* pNewList = (CGUIList*) message.GetLPVOID();
      if (!m_pList)
        m_pList = pNewList;
      else
      {
        CGUIList* pOldList = m_pList;
        pOldList->Lock();
        m_pList = pNewList;
        pOldList->Release();
      }
    }

    if (message.GetMessage() == GUI_MSG_ITEM_SELECTED)
    {
      if (!m_pList)
        return false;

      CGUIList::GUILISTITEMS& list = m_pList->Lock();

      if ((int)list.size() > m_iCursorY + m_iOffset)
      {
        message.SetParam1(m_iCursorY + m_iOffset);
        message.SetLPVOID(list[m_iCursorY + m_iOffset]);
      }
      else
      {
        m_iCursorY = m_iOffset = 0;
      }

      m_pList->Release();
    }

    if (message.GetMessage() == GUI_MSG_ITEM_SELECT)
    {
      if (!m_pList)
        return false;

      CGUIList::GUILISTITEMS& list = m_pList->Lock();

      if (message.GetParam1() >= 0 && message.GetParam1() < list.size())
      {
        int iPage = 1;
        m_iOffset = 0;
        m_iCursorY = message.GetParam1();

        while (m_iCursorY >= m_iItemsPerPage)
        {
          iPage++;
          m_iOffset += m_iItemsPerPage;
          m_iCursorY -= m_iItemsPerPage;
        }

        m_upDown.SetValue(iPage);
      }

      m_pList->Release();
    }
  }

  if ( CGUIControl::OnMessage(message) )
  {
    return true;
  }

  return false;
}

void CGUIListControlEx::PreAllocResources()
{
  if (!m_label.font) return ;
  CGUIControl::PreAllocResources();
  m_upDown.PreAllocResources();
  m_imgButton.PreAllocResources();
}

void CGUIListControlEx::AllocResources()
{
  if (!m_label.font)
  {
    return ;
  }

  CGUIControl::AllocResources();
  m_upDown.AllocResources();

  m_imgButton.AllocResources();
  SetWidth(m_width);
  SetHeight(m_height);
}

void CGUIListControlEx::FreeResources()
{
  CGUIControl::FreeResources();
  m_upDown.FreeResources();
  m_imgButton.FreeResources();
}

void CGUIListControlEx::DynamicResourceAlloc(bool bOnOff)
{
  CGUIControl::DynamicResourceAlloc(bOnOff);
  m_upDown.DynamicResourceAlloc(bOnOff);
  //  Image will be reloaded all the time
  //m_imgButton.DynamicResourceAlloc(bOnOff);
}

void CGUIListControlEx::OnRight()
{
  if (m_iSelect == CONTROL_LIST)
  {
    if (m_upDown.GetMaximum() > 1)
    {
      m_iSelect = CONTROL_UPDOWN;
      m_upDown.SetFocus(true);
    }
    else
    {
      CGUIControl::OnRight();
    }
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

void CGUIListControlEx::OnLeft()
{
  if (m_iSelect == CONTROL_LIST)
    CGUIControl::OnLeft();
  else if (m_upDown.IsFocusedOnUp())
    m_upDown.OnLeft();
  else
  {
    m_iSelect = CONTROL_LIST;
    m_upDown.SetFocus(false);
  }
}

void CGUIListControlEx::OnUp()
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
    else if (m_pList && (m_dwControlUp == 0 || m_dwControlUp == GetID()))
    {
      // move 2 last item in list

      /* No longer done, on recommendation of Sollie
      CGUIList::GUILISTITEMS& list = m_pList->Lock();
      CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), GetID(), list.size() - 1);
      OnMessage(msg);
      m_pList->Release();
      */
    }
    else
    {
      CGUIControl::OnUp();
    }
  }
  else
  { // focus the list again
    m_upDown.SetFocus(false);
    m_iSelect = CONTROL_LIST;
  }
}

void CGUIListControlEx::OnDown()
{
  if ((m_iSelect == CONTROL_LIST) && (m_pList))
  {
    CGUIList::GUILISTITEMS& list = m_pList->Lock();

    if (m_iCursorY + 1 < m_iItemsPerPage)
    {
      if (m_iOffset + 1 + m_iCursorY < (int)list.size())
      {
        m_iCursorY++;
      }
      else if(m_dwControlDown == 0 || m_dwControlDown == GetID())
      {
        /* No longer done, on recommendation of Sollie
        // move first item in list
        CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), GetID(), 0);
        OnMessage(msg);
        */
      }
      else
      {
        CGUIControl::OnDown();
      }
    }
    else
    {
      if (m_iOffset + 1 + m_iCursorY < (int)list.size())
      {
        m_iOffset++;

        int iPage = 1;
        int iSel = m_iOffset + m_iCursorY;
        while (iSel >= m_iItemsPerPage)
        {
          iPage++;
          iSel -= m_iItemsPerPage;
        }
        m_upDown.SetValue(iPage);
      }
      else if(m_dwControlDown == 0 || m_dwControlDown == GetID())
      {
        /* No longer done, on recommendation of Sollie
        // move first item in list
        CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), GetID(), 0);
        OnMessage(msg);
        */
      }
      else
      {
        CGUIControl::OnDown();
      }

    }
    m_pList->Release();
  }
  else
  {
    // move down off our control
    m_upDown.SetFocus(false);
    CGUIControl::OnDown();
  }
}

void CGUIListControlEx::SetScrollySuffix(const CStdString& strSuffix)
{
  m_strSuffix = strSuffix;
}


void CGUIListControlEx::OnPageUp()
{
  int iPage = m_upDown.GetValue();
  if (iPage > 1)
  {
    iPage--;
    m_upDown.SetValue(iPage);
    m_iOffset = (m_upDown.GetValue() - 1) * m_iItemsPerPage;
  }
  else
  {
    // already on page 1, then select the 1st item
    m_iCursorY = 0;
  }
}

void CGUIListControlEx::OnPageDown()
{
  if (!m_pList)
  {
    return ;
  }

  CGUIList::GUILISTITEMS& list = m_pList->Lock();

  int iPages = list.size() / m_iItemsPerPage;
  if (list.size() % m_iItemsPerPage)
    iPages++;

  int iPage = m_upDown.GetValue();
  if (iPage + 1 <= iPages)
  {
    iPage++;
    m_upDown.SetValue(iPage);
    m_iOffset = (m_upDown.GetValue() - 1) * m_iItemsPerPage;
  }
  else
  {
    // already on last page, move 2 last item in list
    CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), GetID(), list.size() - 1);
    OnMessage(msg);
  }

  if (m_iOffset + m_iCursorY >= (int)list.size() )
  {
    m_iCursorY = (int)(list.size() - m_iOffset) - 1;
  }

  m_pList->Release();
}




void CGUIListControlEx::SetImageDimensions(float width, float height)
{
  m_imageWidth = width;
  m_imageHeight = height;
}

void CGUIListControlEx::SetItemHeight(float height)
{
  m_itemHeight = height;
}

void CGUIListControlEx::SetSpaceBetweenItems(float spaceBetweenItems)
{
  m_spaceBetweenItems = spaceBetweenItems;
}

CStdString CGUIListControlEx::GetDescription() const
{
  if (!m_pList)
    return "";

  CGUIList::GUILISTITEMS& list = m_pList->Lock();

  CStdString strLabel;
  int iItem = m_iCursorY + m_iOffset;
  if (iItem >= 0 && iItem < (int)list.size())
  {
    CGUIItem* pItem = list[iItem];
    strLabel = pItem->GetName();
  }

  m_pList->Release();

  return strLabel;
}

void CGUIListControlEx::SetPageControlVisible(bool bVisible)
{
  m_bUpDownVisible = bVisible;
  return ;
}

//TODO: Add OnMouseOver() and possibly HitTest() and SelectItemFromPoint from
// CGUIListControl
bool CGUIListControlEx::OnMouseOver(const CPoint &point)
{
  return CGUIControl::OnMouseOver(point);
}

bool CGUIListControlEx::OnMouseClick(DWORD dwButton, const CPoint &point)
{
  return CGUIControl::OnMouseClick(dwButton, point);
}

bool CGUIListControlEx::CanFocus() const
{
  //if (!m_pList)
  //  return false;

  //if (m_pList->Size()<=0)
  //  return false;

  return CGUIControl::CanFocus();
}

void CGUIListControlEx::SetPosition(float posX, float posY)
{
  // offset our spin control by the appropriate amount
  float spinOffsetX = m_upDown.GetXPosition() - GetXPosition();
  float spinOffsetY = m_upDown.GetYPosition() - GetYPosition();
  CGUIControl::SetPosition(posX, posY);
  m_upDown.SetPosition(GetXPosition() + spinOffsetX, GetYPosition() + spinOffsetY);
}

void CGUIListControlEx::SetWidth(float width)
{
  float spinOffsetX = m_upDown.GetXPosition() - GetXPosition() - GetWidth();
  CGUIControl::SetWidth(width);
  m_upDown.SetPosition(GetXPosition() + GetWidth() + spinOffsetX, m_upDown.GetYPosition());

  m_imgButton.SetWidth(m_width);
}

void CGUIListControlEx::SetHeight(float height)
{
  float spinOffsetY = m_upDown.GetYPosition() - GetYPosition() - GetHeight();
  CGUIControl::SetHeight(height);
  m_upDown.SetPosition(m_upDown.GetXPosition(), GetYPosition() + GetHeight() + spinOffsetY);

  m_imgButton.SetHeight(m_itemHeight);
  float fActualItemHeight = m_itemHeight + m_spaceBetweenItems;
  float fTotalHeight = m_height - m_upDown.GetHeight() - 5;
  m_iItemsPerPage = (int)(fTotalHeight / fActualItemHeight );
  m_upDown.SetRange(1, 1);
  m_upDown.SetValue(1);
}


void CGUIListControlEx::SetPulseOnSelect(bool pulse)
{
  m_imgButton.SetPulseOnSelect(pulse);
  m_upDown.SetPulseOnSelect(pulse);
  CGUIControl::SetPulseOnSelect(pulse);
}

void CGUIListControlEx::SetNavigation(DWORD dwUp, DWORD dwDown, DWORD dwLeft, DWORD dwRight)
{
  CGUIControl::SetNavigation(dwUp, dwDown, dwLeft, dwRight);
  m_upDown.SetNavigation(GetID(), dwDown, GetID(), dwRight);
}

