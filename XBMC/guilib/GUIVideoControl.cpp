#include "guivideocontrol.h"
#include "guifontmanager.h"


CGUIVideoControl::CGUIVideoControl(DWORD dwParentID, DWORD dwControlId, DWORD dwPosX, DWORD dwPosY, DWORD dwWidth, DWORD dwHeight)
:CGUIControl(dwParentID, dwControlId, dwPosX, dwPosY,dwWidth, dwHeight)
{
}

CGUIVideoControl::~CGUIVideoControl(void)
{
}


void CGUIVideoControl::Render()
{
	RECT rc;
	rc.top    = m_dwPosX;
	rc.left   = m_dwPosY;
	rc.right	= m_dwPosX+m_dwWidth;
	rc.bottom = m_dwPosY+m_dwHeight;
	g_graphicsContext.SetViewWindow(rc);
}


