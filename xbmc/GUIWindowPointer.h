#pragma once
#include "GUIWindow.h"

class CGUIWindowPointer :
	public CGUIWindow
{
public:
	CGUIWindowPointer(void);
	virtual ~CGUIWindowPointer(void);
	void Move(int x, int y);
	void SetPointer(DWORD dwPointer);
	virtual void Render();
protected:
	virtual void OnWindowLoaded();
private:
	DWORD m_dwPointer;
};
