
#include "MyDialog.h"
#include "resource.h"
#include "windowsx.h"


CMyDialog::CMyDialog(UINT nResID, CWnd* pParent)
	: CDialog(nResID, pParent), m_nCounter(0)
{
}

CMyDialog::~CMyDialog()
{
}

void CMyDialog::AddToButton()
{
	//get the control window
	HWND hwButton = ::GetDlgItem(m_hWnd, IDC_BUTTON1);
	
	//set text to show in control
	TCHAR szBufW[16];
	wsprintf(szBufW, L"Button %d", m_nCounter);
	::Button_SetText(hwButton, szBufW);
	return;
}

void CMyDialog::AddToComboBox()
{
	//get the control window
	HWND hwComboBox = ::GetDlgItem(m_hWnd, IDC_COMBO1);
	
	//set text to show in control
	TCHAR szBufW[16];
	wsprintf(szBufW, L"ComboBox %d", m_nCounter);
	if (m_nCounter)
	{
		ComboBox_AddString(hwComboBox, szBufW);
		::ComboBox_SetText(hwComboBox, szBufW);
		ComboBox_SetCurSel(hwComboBox, m_nCounter-1);
	}
	else 
	{
		ComboBox_ResetContent(hwComboBox); 
		ComboBox_ShowDropdown(hwComboBox, FALSE);
	}
}

void CMyDialog::AddToEdit()
{
	//get the control window
	HWND hwEdit = ::GetDlgItem(m_hWnd, IDC_EDIT1 ); 
	
	//set text to show in control
	TCHAR szBufW[16];
	wsprintf(szBufW, L"Edit %d\r\n", m_nCounter);
	if (m_nCounter)
		Edit_ReplaceSel(hwEdit, szBufW); 
	else
		::SetWindowText(hwEdit, L""); 
}

void CMyDialog::AddToListBox()
{
	//get the control window
	HWND hwListBox = ::GetDlgItem(m_hWnd, IDC_LIST1 ); 
	
	//set text to show in control
	TCHAR szBufW[16];
	wsprintf(szBufW, L"ListBox %d", m_nCounter);
	if (m_nCounter)
		ListBox_AddString(hwListBox, szBufW);
	else
		ListBox_ResetContent(hwListBox); 
}

void CMyDialog::AddToProgressBar()
{
	//get the control window
	HWND hwProgressBar = ::GetDlgItem(m_hWnd, IDC_PROGRESS1);
	
	//set progress bar position
	SendMessage(hwProgressBar, PBM_SETPOS, (WPARAM)m_nCounter * 10, 0L);
}

void CMyDialog::AddToScrollBars()
{
	//get the control window
	HWND hwScrollBarH = ::GetDlgItem(m_hWnd, IDC_SCROLLBAR1);
	HWND hwScrollBarV = ::GetDlgItem(m_hWnd, IDC_SCROLLBAR2);
	
	//set scroll bar range
	ScrollBar_SetRange(hwScrollBarH, 0, 10, FALSE);
	ScrollBar_SetRange(hwScrollBarV, 0, 10, FALSE);
	
	//set scroll bar position
	ScrollBar_SetPos(hwScrollBarH, m_nCounter, TRUE);
	ScrollBar_SetPos(hwScrollBarV, m_nCounter, TRUE);
}

void CMyDialog::AddToSlider()
{
	//get the control window
	HWND hwSlider = ::GetDlgItem(m_hWnd, IDC_SLIDER1);
	
	//set slider position
	SendMessage(hwSlider, TBM_SETPOS, TRUE, (WPARAM)m_nCounter * 10);
}

BOOL CMyDialog::OnInitDialog()
{
	//Set the Icon
	SetIconLarge(IDW_MAIN);
	SetIconSmall(IDW_MAIN);

	// Set a timer to animate the controls on the dialog window
	SetTimer(ID_TIMER, 500, NULL);

	return true;
}

void CMyDialog::OnOK()
{
	::MessageBox(NULL, TEXT("DONE Button Pressed.  Program will exit now."), TEXT("Button"), MB_OK);
	CDialog::OnOK();
}

INT_PTR CMyDialog::DialogProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
	case WM_TIMER:
		m_nCounter > 9 ? m_nCounter = 0 : m_nCounter++;
		AddToEdit();
		AddToListBox();
		AddToScrollBars();
		AddToProgressBar();
		AddToSlider();
		AddToComboBox();
		AddToButton();
    break;

    } // switch(uMsg)
	
	return DialogProcDefault(uMsg, wParam, lParam);
	
} // INT_PTR CALLBACK DialogProc(...)


