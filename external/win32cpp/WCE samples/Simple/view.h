#ifndef VIEW_H
#define VIEW_H

#include "wincore.h"


class CView : public CWnd
{
public:
	CView() {}
	virtual ~CView() {}
	virtual void OnDraw(CDC* pDC);
	virtual	LRESULT WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
};

#endif   //VIEW_H
