/////////////////////////////
// Button.h


#ifndef BUTTON_H
#define BUTTON_H

#include "wincore.h"


class CButton : public CWnd
{
public:
	CButton() {}
	virtual ~CButton();
	virtual void PreCreate(CREATESTRUCT &cs);

protected:
	virtual LRESULT WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

};



#endif //BUTTON_H


