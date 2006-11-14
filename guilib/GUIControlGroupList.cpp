#include "include.h"
#include "GUIControlGroupList.h"

CGUIControlGroupList::CGUIControlGroupList(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, float itemGap, DWORD pageControl)
: CGUIControlGroup(dwParentID, dwControlId, posX, posY, width, height)
{
  m_itemGap = itemGap;
  m_pageControl = pageControl;
  m_offset = 0;
  m_totalSize = 10;
  ControlType = GUICONTROL_GROUPLIST;
}

CGUIControlGroupList::~CGUIControlGroupList(void)
{
}

void CGUIControlGroupList::Render()
{
  if (IsVisible())
  {
    ValidateOffset();
    if (m_pageControl)
    {
      CGUIMessage message(GUI_MSG_LABEL_RESET, GetParentID(), m_pageControl, (DWORD)m_height, (DWORD)m_totalSize);
      SendWindowMessage(message);
      CGUIMessage message2(GUI_MSG_ITEM_SELECT, GetParentID(), m_pageControl, (DWORD)m_offset);
      SendWindowMessage(message2);
    }
    // we run through the controls, rendering as we go
    float posX = 0;
    float posY = 0;
    for (iControls it = m_children.begin(); it != m_children.end(); ++it)
    {
      CGUIControl *control = *it;
      control->UpdateEffectState(m_renderTime);
      if (control->IsVisible())
      {
        if (posY + control->GetHeight() > m_offset && posY < m_offset + m_height)
        { // we can render
          control->SetPosition(m_posX + posX, m_posY + posY - m_offset);
          control->Render();
        }
        posY += control->GetHeight() + m_itemGap;
      }
    }
    CGUIControl::Render();
  }
  g_graphicsContext.RemoveGroupTransform();
}

bool CGUIControlGroupList::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage() )
  {
  case GUI_MSG_FOCUSED:
    { // a control has been focused
      // scroll if we need to and update our page control
      ValidateOffset();
      float offset = 0;
      bool done = false;
      for (iControls it = m_children.begin(); it != m_children.end(); ++it)
      {
        CGUIControl *control = *it;
        if (!control->IsVisible())
          continue;
        if (control->GetID() == message.GetControlId())
        {
          if (offset < m_offset)
          { // start of this control is below the current offset
            m_offset = offset;
          }
          else if (offset + control->GetHeight() > m_offset + m_height)
          { // end of this control is below the current offset
            m_offset = offset + control->GetHeight() - m_height;
          }
          break;
        }
        offset += control->GetHeight() + m_itemGap;
      }
    }
    break;
  case GUI_MSG_SETFOCUS:
    {
      // we've been asked to focus.  We focus the last control if it's on this page,
      // else we'll focus the first focusable control from our offset (after verifying it)
      ValidateOffset();
      // now check the focusControl's offset
      float offset = 0;
      for (iControls it = m_children.begin(); it != m_children.end(); ++it)
      {
        CGUIControl *control = *it;
        if (!control->IsVisible())
          continue;
        if (control->GetID() == m_focusedControl)
        {
          if (offset >= m_offset && offset + control->GetHeight() < m_offset + m_height)
          { // control is in our range
            return CGUIControlGroup::OnMessage(message);
          }
          break;
        }
        offset += control->GetHeight() + m_itemGap;
      }
      // find the first control on this page
      offset = 0;
      for (iControls it = m_children.begin(); it != m_children.end(); ++it)
      {
        CGUIControl *control = *it;
        if (!control->IsVisible())
          continue;
        if (control->CanFocus() && offset >= m_offset && offset + control->GetHeight() < m_offset + m_height)
        {
          m_focusedControl = control->GetID();
          break;
        }
        offset += control->GetHeight() + m_itemGap;
      }
    }
    break;
  case GUI_MSG_PAGE_CHANGE:
    {
      if (message.GetSenderId() == m_pageControl)
      { // it's from our page control
        m_offset = (float)message.GetParam1();
        return true;
      }
    }
    break;
  }
  return CGUIControlGroup::OnMessage(message);
}

void CGUIControlGroupList::ValidateOffset()
{
  // calculate how many items we have on this page
  m_totalSize = 0;
  for (iControls it = m_children.begin(); it != m_children.end(); ++it)
  {
    CGUIControl *control = *it;
    if (!control->IsVisible()) continue;
    m_totalSize += control->GetHeight() + m_itemGap;
  }
  // check our m_offset range
  if (m_offset > m_totalSize - m_height) m_offset = m_totalSize - m_height;
  if (m_offset < 0) m_offset = 0;
}