// DetailsDialog.cpp : implementation file
//

#include "stdafx.h"
#include "TestScraper.h"
#include "DetailsDialog.h"

// CDetailsDialog dialog

IMPLEMENT_DYNAMIC(CDetailsDialog, CDialog)
CDetailsDialog::CDetailsDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CDetailsDialog::IDD, pParent)
{
}

CDetailsDialog::~CDetailsDialog()
{
}

void CDetailsDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CDetailsDialog, CDialog)
//	ON_WM_SETFOCUS()
ON_WM_SHOWWINDOW()
//ON_EN_CHANGE(IDC_DETAILS, OnEnChangeDetails)
END_MESSAGE_MAP()


// CDetailsDialog message handlers

BOOL CDetailsDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	// set window title
	SetWindowText(GetString("Title") + " - Details");
	OutputDebugString(m_strXML);
	// find the data in the xml string
	CString strValue;
	strValue += FormatField("Tagline");
	strValue += FormatField("Director");
	strValue += FormatField("Year");
	strValue += FormatField("Genre");
	strValue += FormatField("RunTime");
	strValue += FormatField("Outline");
	strValue += FormatField("Plot");
	// cast is a special case
	int start = m_strXML.Find("<cast>") + 6;
	int end = m_strXML.Find("</cast>", start);
	CString strCast = m_strXML.Mid(start, end-start);
	strCast.Replace("\n", "\r\n\t");
	strValue += "Cast:\t" + strCast + "\r\n\r\n";
	// continue as before
	strValue += FormatField("Rating");
	strValue += FormatField("Votes");
	strValue += FormatField("MPAA");
	strValue += FormatField("Credits");
	strValue += FormatField("Thumb");
	strValue += FormatField("Top250");
	GetDlgItem(IDC_DETAILS)->SetWindowText(strValue);
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

CString CDetailsDialog::GetString(const CString &strField)
{
	CString field = strField;
	field.MakeLower();
	CString strFind;
	strFind.Format("<%s>", field);
	int start = m_strXML.Find(strFind) + strFind.GetLength();
	strFind.Format("</%s>", field);
	int end = m_strXML.Find(strFind, start);
	return m_strXML.Mid(start, end-start);
}

CString CDetailsDialog::FormatField(const CString &strField)
{
  return strField + ":\t" + GetString(strField) + "\r\n\r\n";
}

void CDetailsDialog::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialog::OnShowWindow(bShow, nStatus);
	CEdit *pEdit = (CEdit *)GetDlgItem(IDC_DETAILS);
	pEdit->SetSel(0,0);
}

//void CDetailsDialog::OnEnChangeDetails()
//{
//	// TODO:  If this is a RICHEDIT control, the control will not
//	// send this notification unless you override the CDialog::OnInitDialog()
//	// function and call CRichEditCtrl().SetEventMask()
//	// with the ENM_CHANGE flag ORed into the mask.
//
//	// TODO:  Add your control notification handler code here
//}
