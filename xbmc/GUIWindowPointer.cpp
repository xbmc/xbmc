#include "stdafx.h"
#include "GUIWindowPointer.h"

#define ID_POINTER 10

CGUIWindowPointer::CGUIWindowPointer(void)
:CGUIWindow(0)
{
	m_dwPointer = 0;
}

CGUIWindowPointer::~CGUIWindowPointer(void)
{
}

void CGUIWindowPointer::Move(int x, int y)
{
	int iPosX = m_iPosX + x;
	int iPosY = m_iPosY + y;
	if (iPosX<0) iPosX = 0;
	if (iPosY<0) iPosY = 0;
	if (iPosX>g_graphicsContext.GetWidth()) iPosX = g_graphicsContext.GetWidth();
	if (iPosY>g_graphicsContext.GetHeight()) iPosY = g_graphicsContext.GetHeight();
	SetPosition(iPosX, iPosY);
}

void CGUIWindowPointer::SetPointer(DWORD dwPointer)
{
	if (m_dwPointer == dwPointer) return;
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
{	// set all our pointer images invisible
	for (ivecControls i=m_vecControls.begin();i != m_vecControls.end(); ++i)
	{
		CGUIControl* pControl= *i;
		pControl->SetVisible(false);
	}
}

void CGUIWindowPointer::Render()
{
	SetPosition(g_Mouse.iPosX, g_Mouse.iPosY);
	SetPointer(g_Mouse.GetState());
	CGUIWindow::Render();
}