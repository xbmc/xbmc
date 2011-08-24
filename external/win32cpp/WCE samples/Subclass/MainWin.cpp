//////////////////////////////
// MainWin.Cpp

#include "MainWin.h"


CMainWin::CMainWin()
{
	::InitCommonControls();
}

void CMainWin::OnCreate()
{
	m_Button.Create(this);
}

LRESULT CMainWin::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_DESTROY:
	 		// Terminate the application.
			::PostQuitMessage(0);
			return 0L;
	}

	// Pass unhandled messages on for default processing
	return WndProcDefault(uMsg, wParam, lParam);
}
