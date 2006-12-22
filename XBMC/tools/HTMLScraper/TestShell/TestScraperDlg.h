// TestScraperDlg.h : header file
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"


// CTestScraperDlg dialog
class CTestScraperDlg : public CDialog
{
// Construction
public:
	CTestScraperDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_TEST_SCRAPER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;
	int GetURL(const CString &str, CString &strHTML);
	// RGB2YUV stuff
	void RunRGBTest(int nSkip);
	LONG yuv2rgb_ps1(BYTE Y, BYTE U, BYTE V);
	LONG yuv2rgb_ps2(BYTE Y, BYTE U, BYTE V);
	LONG yuv2rgb_ps3(BYTE Y, BYTE U, BYTE V);
	float nine_bits(float in);
	LONG rgb_exact(float *r, float *g, float *b);
	// RGB2YUV stuff
	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnEnChangeSearch();
	afx_msg void OnBnClickedSearchBtn();
	CListCtrl m_searchResults;
	afx_msg void OnNMDblclkSearchResults(NMHDR *pNMHDR, LRESULT *pResult);
};
