#pragma once


// CDetailsDialog dialog

class CDetailsDialog : public CDialog
{
	DECLARE_DYNAMIC(CDetailsDialog)

public:
	CDetailsDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDetailsDialog();

// Dialog Data
	enum { IDD = IDD_DETAILS_DIALOG };

	CString m_strTitle;
	CString m_strXML;

protected:
	CString GetString(const CString &strField);
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
//	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
//	afx_msg void OnEnChangeDetails();
};
