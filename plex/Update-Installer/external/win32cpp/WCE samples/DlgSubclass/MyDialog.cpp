///////////////////////////////////////
// MyDialog.cpp

#include "MyDialog.h"
#include "Hyperlink.h"
#include "resource.h"


// Definitions for the CMyDialog class
CMyDialog::CMyDialog(UINT nResID, CWnd* pParent)
	: CDialog(nResID, pParent)
{
}

CMyDialog::CMyDialog(LPCTSTR lpszResName, CWnd* pParent)
	: CDialog(lpszResName, pParent)
{
}

CMyDialog::~CMyDialog()
{
}

INT_PTR CMyDialog::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
//	switch (uMsg)
//	{
		//Additional messages to be handled go here
//	}


	//Use the dialogframe default message handling for remaining messages
	return DialogProcDefault(uMsg, wParam, lParam);
}

BOOL CMyDialog::OnCommand(UINT nID)
{
//	switch (nID)
//   {

//   } //switch (nID)

	return TRUE;
}

BOOL CMyDialog::OnInitDialog()
{
	// Set the Icon
	SetIconLarge(IDW_MAIN);
	SetIconSmall(IDW_MAIN);

	// Put some text in the edit boxes
	::SetDlgItemText(GetHwnd(), IDC_EDIT1, TEXT("Edit Control"));

	// Turn our button into a MyButton object
	m_Button.AttachDlgItem(IDC_BUTTON2, this);

	// Turn our static control into a hyperlink
	m_Hyperlink.AttachDlgItem(IDC_STATIC4, this);

	return true;
}

void CMyDialog::OnOK()
{
	::MessageBox(NULL, TEXT("OK Button Pressed.  Program will exit now."), TEXT("Button"), MB_OK);
	CDialog::OnOK();
}



void CMyDialog::SetStatic(LPCTSTR szString)
{
	::SetDlgItemText(GetHwnd(), IDC_EDIT1, szString);
}

