#if !defined(AFX_NEWUSERDLG_H__0487F0AE_1C8C_4658_A466_5EF412B12982__INCLUDED_)
#define AFX_NEWUSERDLG_H__0487F0AE_1C8C_4658_A466_5EF412B12982__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NewUserDlg.h : Header-Datei
//

/////////////////////////////////////////////////////////////////////////////
// Dialogfeld CNewUserDlg 

class CNewUserDlg : public CDialog
{
// Konstruktion
public:
	CNewUserDlg(CWnd* pParent = NULL);   // Standardkonstruktor
	std::list<CString> m_GroupList;

// Dialogfelddaten
	//{{AFX_DATA(CNewUserDlg)
	enum { IDD = IDD_NEWUSER };
	CButton	m_cOkCtrl;
	CComboBox	m_cGroup;
	CString	m_Name;
	CString m_Group;
	//}}AFX_DATA


// Überschreibungen
	// Vom Klassen-Assistenten generierte virtuelle Funktionsüberschreibungen
	//{{AFX_VIRTUAL(CNewUserDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV-Unterstützung
	//}}AFX_VIRTUAL

// Implementierung
protected:

	// Generierte Nachrichtenzuordnungsfunktionen
	//{{AFX_MSG(CNewUserDlg)
	afx_msg void OnChangeNewuserName();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ fügt unmittelbar vor der vorhergehenden Zeile zusätzliche Deklarationen ein.

#endif // AFX_NEWUSERDLG_H__0487F0AE_1C8C_4658_A466_5EF412B12982__INCLUDED_
