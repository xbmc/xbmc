#include "stdafx.h"
#include "guivideocontrol.h"
#include "guifontmanager.h"
#include "guiwindowmanager.h"


CGUIVideoControl::CGUIVideoControl(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight)
:CGUIControl(dwParentID, dwControlId, iPosX, iPosY,dwWidth, dwHeight)
{
}

CGUIVideoControl::~CGUIVideoControl(void)
{
}


void CGUIVideoControl::Render()
{
	RECT rc;
	float x = (float)m_iPosX;
	float y = (float)m_iPosY;

	if (!IsVisible()) return;

	g_graphicsContext.Correct(x, y);

	rc.left    = (DWORD)x;
	rc.top    = (DWORD)y;
	rc.right	= rc.left+m_dwWidth;
	rc.bottom = rc.top+m_dwHeight;
	g_graphicsContext.SetViewWindow(rc);

	extern void xbox_video_render_update();
	xbox_video_render_update();
}

void CGUIVideoControl::OnMouseClick(DWORD dwButton)
{	// mouse has clicked in the video control
	// switch to fullscreen video
	if (dwButton == MOUSE_LEFT_BUTTON)
	{
		CGUIMessage message(GUI_MSG_FULLSCREEN, GetID(), GetParentID());
		g_graphicsContext.SendMessage(message);
	}
	if (dwButton == MOUSE_RIGHT_BUTTON)
	{	// toggle the playlist window
		if (m_gWindowManager.GetActiveWindow() == WINDOW_VIDEO_PLAYLIST)
			m_gWindowManager.PreviousWindow();
		else
			m_gWindowManager.ActivateWindow(WINDOW_VIDEO_PLAYLIST);
		// reset the mouse button.
		g_Mouse.bClick[MOUSE_RIGHT_BUTTON] = false;
	}
}
