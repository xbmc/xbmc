#include "stdafx.h"
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
	float x = (float)m_dwPosX;
	float y = (float)m_dwPosY;
	g_graphicsContext.Correct(x, y);

	rc.left    = (DWORD)x;
	rc.top    = (DWORD)y;
	rc.right	= rc.left+m_dwWidth;
	rc.bottom = rc.top+m_dwHeight;
	g_graphicsContext.SetViewWindow(rc);

	extern void xbox_video_render_update();
	xbox_video_render_update();
}


