// TestScraperDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TestScraper.h"
#include "TestScraperDlg.h"
#include "DetailsDialog.h"

#include "../HTMLScraper.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CTestScraperDlg dialog



CTestScraperDlg::CTestScraperDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTestScraperDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CTestScraperDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SEARCH_RESULTS, m_searchResults);
}

BEGIN_MESSAGE_MAP(CTestScraperDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_EN_CHANGE(IDC_SEARCH, OnEnChangeSearch)
	ON_BN_CLICKED(IDC_SEARCH_BTN, OnBnClickedSearchBtn)
//	ON_NOTIFY(LVN_ITEMCHANGED, IDC_SEARCH_RESULTS, OnLvnItemchangedSearchResults)
	ON_NOTIFY(NM_DBLCLK, IDC_SEARCH_RESULTS, OnNMDblclkSearchResults)
END_MESSAGE_MAP()


// CTestScraperDlg message handlers

BOOL CTestScraperDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	GetDlgItem(IDC_SEARCH_BTN)->EnableWindow(false);

	RECT rect;
	m_searchResults.GetWindowRect(&rect);
	int iWidth = (int)((rect.right-rect.left)*0.5f);
	int iWidth2 = (rect.right-rect.left) - iWidth - 4;
	m_searchResults.InsertColumn(0, "Title", LVCFMT_LEFT, iWidth, 0);
	m_searchResults.InsertColumn(1, "URL", LVCFMT_LEFT, iWidth2, 1);
	return TRUE;  // return TRUE  unless you set the focus to a control
}

int CTestScraperDlg::GetURL(const CString &str, CString &strHTML)
{
	int nRetCode = 0;
	const TCHAR szHeaders[] = _T("Accept: text/*\r\nUser-Agent: MFC_Tear_Sample\r\n");

	CInternetSession session(_T("TEAR - MFC Sample App"), PRE_CONFIG_INTERNET_ACCESS);
	CHttpConnection* pServer = NULL;
	CHttpFile* pFile = NULL;
	strHTML = "";
	try
	{
		// check to see if this is a reasonable URL

		CString strServerName;
		CString strObject;
		INTERNET_PORT nPort;
		DWORD dwServiceType;

		if (!AfxParseURL(str, dwServiceType, strServerName, strObject, nPort) ||
			dwServiceType != INTERNET_SERVICE_HTTP)
		{
			return 1;
		}

		pServer = session.GetHttpConnection(strServerName, nPort);

		pFile = pServer->OpenRequest(CHttpConnection::HTTP_VERB_GET,
			strObject, NULL, 1, NULL, NULL, INTERNET_FLAG_EXISTING_CONNECT);
		pFile->AddRequestHeaders(szHeaders);
		pFile->SendRequest();

		DWORD dwRet;
		pFile->QueryInfoStatusCode(dwRet);

		// if access was denied, prompt the user for the password

		if (dwRet == HTTP_STATUS_DENIED)
		{
			DWORD dwPrompt;
			dwPrompt = pFile->ErrorDlg(NULL, ERROR_INTERNET_INCORRECT_PASSWORD,
				FLAGS_ERROR_UI_FLAGS_GENERATE_DATA | FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS, NULL);

			// if the user cancelled the dialog, bail out

			if (dwPrompt != ERROR_INTERNET_FORCE_RETRY)
			{
				return 1;
			}

			pFile->SendRequest();
			pFile->QueryInfoStatusCode(dwRet);
		}

		CString strNewLocation;
		pFile->QueryInfo(HTTP_QUERY_RAW_HEADERS_CRLF, strNewLocation);

		TCHAR sz[1024];
		while (pFile->ReadString(sz, 1023))
		{
			strHTML += sz;
		}

		pFile->Close();
		pServer->Close();
	}
	catch (CInternetException* pEx)
	{
		// catch errors from WinINet

		TCHAR szErr[1024];
		pEx->GetErrorMessage(szErr, 1024);

		nRetCode = 2;
		pEx->Delete();
	}

	if (pFile != NULL)
		delete pFile;
	if (pServer != NULL)
		delete pServer;
	session.Close();

	return nRetCode;
}

void CTestScraperDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CTestScraperDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CTestScraperDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CTestScraperDlg::OnEnChangeSearch()
{
	CString strSearch;
	GetDlgItem(IDC_SEARCH)->GetWindowText(strSearch);
	GetDlgItem(IDC_SEARCH_BTN)->EnableWindow(strSearch.GetLength()>0);
}

void CTestScraperDlg::OnBnClickedSearchBtn()
{
	CString strSearch;
	GetDlgItem(IDC_SEARCH)->GetWindowText(strSearch);
	CString strURL;
	CString strHTML;
	strURL.Format("http://akas.imdb.com/Tsearch?title=%s", strSearch);
	if (GetURL(strURL, strHTML) == 0)
	{
		char *szXML = new char[60000];
    int len;
		if (len = IMDbGetSearchResults(szXML, strHTML, strURL))
		{
			CString strXML = szXML;
			m_searchResults.DeleteAllItems();
			OutputDebugString(strXML);
			int iStart = strXML.Find("<movie>") + 7;
			int iEnd = strXML.Find("</movie>", iStart);
			int iNumItems = 0;
			while (iStart > 7)
			{
				CString strMovie = strXML.Mid(iStart, iEnd-iStart);
				// add stuff to the listcontrol
				int start = strMovie.Find("<title>") + 7;
				int end = strMovie.Find("</title>", start);
				CString strTitle = strMovie.Mid(start, end-start);
				m_searchResults.InsertItem(iNumItems, strTitle);
				start = strMovie.Find("<url>") + 5;
				end = strMovie.Find("</url>", start);
				CString strURL = strMovie.Mid(start, end-start);
				m_searchResults.SetItemText(iNumItems,1,strURL);
				iStart = strXML.Find("<movie>", iEnd) + 7;
				iEnd = strXML.Find("</movie>", iStart);
				iNumItems++;
			}
		}
		delete [] szXML;
	}
}

void CTestScraperDlg::OnNMDblclkSearchResults(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	int item = pNMLV->iItem;
	if (item < 0 || item >= m_searchResults.GetItemCount())
		return;
	CString strTitle = m_searchResults.GetItemText(item, 0);
	CString strURL = m_searchResults.GetItemText(item, 1);
	CString strHTML;
	CString strPlotHTML;
	GetURL(strURL, strHTML);
	GetURL(strURL + "plotsummary", strPlotHTML);
  char szXML[60000];
	int iDetails = IMDbGetDetails(szXML, strHTML, strPlotHTML);
	if (iDetails)
	{
		CDetailsDialog dlg;
		dlg.m_strXML = szXML;
		dlg.DoModal();
	}
	*pResult = 0;
}
