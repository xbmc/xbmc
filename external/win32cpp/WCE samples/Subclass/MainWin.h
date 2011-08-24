///////////////////////////////
// MainWin.h


#ifndef MAINWIN_H
#define MAINWIN_H


#include "Button.h"


class CMainWin : public CWnd
{
public:
	CMainWin();
	~CMainWin() {}

protected:
	LRESULT WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void OnCreate();

private:
	CButton m_Button;
};



#endif //MAINWIN_H

