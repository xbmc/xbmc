#if !defined(AFX_OPTIONSGSSPAGE_H__E3184429_C324_46EF_85F7_5F12C16A6FA6__INCLUDED_)
#define AFX_OPTIONSGSSPAGE_H__E3184429_C324_46EF_85F7_5F12C16A6FA6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// OptionsGSSPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// COptionsGSSPage dialog

class COptionsGSSPage : public CSAPrefsSubDlg
{

// Construction
public:
	COptionsGSSPage(CWnd* pParent = NULL);


// Dialog Data
	//{{AFX_DATA(COptionsGSSPage)
	enum { IDD = IDD_OPTIONS_GSS };
	BOOL	m_bPromptPassword;
	BOOL	m_bUseGSS;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(COptionsGSSPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(COptionsGSSPage)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OPTIONSGSSPAGE_H__E3184429_C324_46EF_85F7_5F12C16A6FA6__INCLUDED_)
