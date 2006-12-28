#include "stdafx.h"
#include "GUIWindowPointer.h"


#define ID_POINTER 10

CGUIWindowPointer::CGUIWindowPointer(void)
    : CGUIWindow(105, "Pointer.xml")
{
  m_dwPointer = 0;
  m_loadOnDemand = false;
  m_needsScaling = false;
}

CGUIWindowPointer::~CGUIWindowPointer(void)
{}

void CGUIWindowPointer::Move(int x, int y)
{
  float posX = m_posX + x;
  float posY = m_posY + y;
  if (posX < 0) posX = 0;
  if (posY < 0) posY = 0;
  if (posX > g_graphicsContext.GetWidth()) posX = (float)g_graphicsContext.GetWidth();
  if (posY > g_graphicsContext.GetHeight()) posY = (float)g_graphicsContext.GetHeight();
  SetPosition(posX, posY);
}

void CGUIWindowPointer::SetPointer(DWORD dwPointer)
{
  if (m_dwPointer == dwPointer) return ;
  // set the new pointer visible
  CGUIControl *pControl = (CGUIControl *)GetControl(dwPointer);
  if (pControl)
  {
    pControl->SetVisible(true);
    // disable the old pointer
    pControl = (CGUIControl *)GetControl(m_dwPointer);
    if (pControl) pControl->SetVisible(false);
    // set pointer to the new one
    m_dwPointer = dwPointer;
  }
}

void CGUIWindowPointer::OnWindowLoaded()
{ // set all our pointer images invisible
  for (ivecControls i = m_vecControls.begin();i != m_vecControls.end(); ++i)
  {
    CGUIControl* pControl = *i;
    pControl->SetVisible(false);
  }
  CGUIWindow::OnWindowLoaded();
  DynamicResourceAlloc(false);
}

void CGUIWindowPointer::Render()
{
  SetPosition(g_Mouse.posX, g_Mouse.posY);
  SetPointer(g_Mouse.GetState());
  CGUIWindow::Render();
}
