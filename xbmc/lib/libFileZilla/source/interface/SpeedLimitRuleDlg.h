#if !defined(AFX_SPEEDLIMITRULEDLG_H__8C304309_0F82_42CC_9833_2FAF0A6A83C9__INCLUDED_)
#define AFX_SPEEDLIMITRULEDLG_H__8C304309_0F82_42CC_9833_2FAF0A6A83C9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SpeedLimitRuleDlg.h : header file
//
#include "..\SpeedLimit.h"

/////////////////////////////////////////////////////////////////////////////
// CSpeedLimitRuleDlg dialog

class CSpeedLimitRuleDlg : public CDialog
{
// Construction
public:
	void FillFromSpeedLimit(const CSpeedLimit &sl);
	CSpeedLimit GetSpeedLimit();
	CSpeedLimitRuleDlg(CWnd* pParent = NULL);   // standard constructor

	BOOL	m_Day[ 7];
// Dialog Data
	//{{AFX_DATA(CSpeedLimitRuleDlg)
	enum { IDD = IDD_SPEEDLIMIT_RULE_DIALOG };
	CDateTimeCtrl	m_ToCtrl;
	CDateTimeCtrl	m_FromCtrl;
	CDateTimeCtrl	m_DateCtrl;
	BOOL	m_DateCheck;
	CTime	m_Date;
	BOOL	m_FromCheck;
	CTime	m_FromTime;
	BOOL	m_ToCheck;
	CTime	m_ToTime;
	int		m_Speed;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSpeedLimitRuleDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSpeedLimitRuleDlg)
	afx_msg void OnDateCheck();
	afx_msg void OnToCheck();
	afx_msg void OnFromCheck();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SPEEDLIMITRULEDLG_H__8C304309_0F82_42CC_9833_2FAF0A6A83C9__INCLUDED_)
