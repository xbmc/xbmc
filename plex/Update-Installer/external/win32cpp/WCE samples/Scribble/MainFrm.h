#ifndef VIEW_H
#define VIEW_H


#include "wincore.h"
#include "wceframe.h"
#include "dialog.h"
#include <vector>


class CMainFrame : public CWceFrame
{
public:
	CMainFrame();
	virtual ~CMainFrame() {}
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual void OnInitialUpdate();
	virtual void SetPen(COLORREF Color);

protected:
	virtual void DrawLine(short x, short y);
	virtual void OnDraw(CDC* pDC);	
	virtual void OnLButtonDown(WPARAM wParam, LPARAM lParam);
	virtual void OnLButtonUp(WPARAM wParam, LPARAM lParam);
	virtual void OnMouseMove(WPARAM wParam, LPARAM lParam);
	virtual void StorePoint(int x, int y, bool PenDown);
	virtual LRESULT WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	struct PlotPoint
	{
		short x;
		short y;
		bool PenDown;
		COLORREF color;
	};

	std::vector<PlotPoint> m_points;	// Points of lines to draw
	COLORREF m_PenColor;
};

#endif   //VIEW_H
