
#include "MyDialog.h"
#include "resource.h"



CMyDialog::CMyDialog(UINT nResID, CWnd* pParent)
	: CDialog(nResID, pParent)
{
}

CMyDialog::~CMyDialog()
{
}


BOOL CMyDialog::OnInitDialog()
{
	// This function is called before the dialog is displayed.


	//Set the Icon
	SetIconLarge(IDW_MAIN);
	SetIconSmall(IDW_MAIN);

	return true;
}

void CMyDialog::OnOK()
{
	::MessageBox(NULL, TEXT("DONE Button Pressed.  Program will exit now."), TEXT("Button"), MB_OK);
	CDialog::OnOK();
}

INT_PTR CMyDialog::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
// Add any messages to be handled inside the switch statement


//	switch (uMsg)
//	{
//
//	} // switch(uMsg)
	
	return DialogProcDefault(uMsg, wParam, lParam);
	
} // INT_PTR CALLBACK DialogProc(...)


