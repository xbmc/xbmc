// FileZilla Server - a Windows ftp server

// Copyright (C) 2002 - Tim Kosse <tim.kosse@gmx.de>

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

// FileZilla server.cpp : Legt das Klassenverhalten für die Anwendung fest.
//

#include "stdafx.h"
#include "FileZilla server.h"
#include "version.h"
#include "misc\hyperlink.h"
#include "options.h"

#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFileZillaserverApp

BEGIN_MESSAGE_MAP(CFileZillaserverApp, CWinApp)
	//{{AFX_MSG_MAP(CFileZillaserverApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
		// HINWEIS - Hier werden Mapping-Makros vom Klassen-Assistenten eingefügt und entfernt.
		//    Innerhalb dieser generierten Quelltextabschnitte NICHTS VERÄNDERN!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFileZillaserverApp Konstruktion

CFileZillaserverApp::CFileZillaserverApp()
{
	// ZU ERLEDIGEN: Hier Code zur Konstruktion einfügen
	// Alle wichtigen Initialisierungen in InitInstance platzieren
}

CFileZillaserverApp::~CFileZillaserverApp()
{
	WSACleanup();
}

/////////////////////////////////////////////////////////////////////////////
// Das einzige CFileZillaserverApp-Objekt

CFileZillaserverApp theApp;


/////////////////////////////////////////////////////////////////////////////
// CFileZillaserverApp Initialisierung

BOOL CFileZillaserverApp::InitInstance()
{
	COptions *pOptions = new COptions;

	if (m_lpCmdLine && _tcslen(m_lpCmdLine) >= 11 && !_tcsncmp(m_lpCmdLine, "/adminport ", 11))
	{
		int nAdminPort = _ttoi(m_lpCmdLine + 11);

		if (nAdminPort > 1 && nAdminPort < 65535 && pOptions->GetOption(IOPTION_LASTSERVERADDRESS) == _T("127.0.0.1"))
			pOptions->SetOption(IOPTION_LASTSERVERPORT, nAdminPort);
		delete pOptions;
		return FALSE;
	}
	
	// initialize Winsock library
	BOOL res=TRUE;
	WSADATA wsaData;
	
	WORD wVersionRequested = MAKEWORD(1, 1);
	int nResult = WSAStartup(wVersionRequested, &wsaData);
	if (nResult != 0)
		res=FALSE;
	else if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1)
	{
		WSACleanup();
		res=FALSE;
	}
	
	if(!res)
		return FALSE;
	
	// Standardinitialisierung
	// Wenn Sie diese Funktionen nicht nutzen und die Größe Ihrer fertigen 
	//  ausführbaren Datei reduzieren wollen, sollten Sie die nachfolgenden
	//  spezifischen Initialisierungsroutinen, die Sie nicht benötigen, entfernen.

#ifdef _AFXDLL
	Enable3dControls();			// Diese Funktion bei Verwendung von MFC in gemeinsam genutzten DLLs aufrufen
#else
	Enable3dControlsStatic();	// Diese Funktion bei statischen MFC-Anbindungen aufrufen
#endif

	// Dieser Code erstellt ein neues Rahmenfensterobjekt und setzt dies
	// dann als das Hauptfensterobjekt der Anwendung, um das Hauptfenster zu erstellen.

	CMainFrame* pFrame = new CMainFrame(pOptions);
	m_pMainWnd = pFrame;

	// Rahmen mit Ressourcen erstellen und laden

	pFrame->LoadFrame(IDR_MAINFRAME,
		WS_OVERLAPPEDWINDOW , NULL,
		NULL);



	// Das einzige Fenster ist initialisiert und kann jetzt angezeigt und aktualisiert werden.
	if (pOptions->GetOptionVal(IOPTION_STARTMINIMIZED))
		pFrame->ShowWindow(SW_HIDE);
	else
		pFrame->ShowWindow(SW_SHOW);
	pFrame->UpdateWindow();

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CFileZillaserverApp Nachrichten-Handler





/////////////////////////////////////////////////////////////////////////////
// CAboutDlg-Dialog für Info über Anwendung

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialogdaten
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//CStatic	m_cDonate;
	CStatic	m_cVersion;
	CHyperLink m_mail;
	CHyperLink m_homepage;
	//}}AFX_DATA

	CHyperLink m_cDonate;
	
	// Überladungen für virtuelle Funktionen, die vom Anwendungs-Assistenten erzeugt wurden
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV-Unterstützung
	//}}AFX_VIRTUAL

// Implementierung
protected:
	//{{AFX_MSG(CAboutDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	DDX_Control(pDX, IDC_VERSION, m_cVersion);
	DDX_Control(pDX, IDC_MAIL, m_mail);
	DDX_Control(pDX, IDC_HOMEPAGE, m_homepage);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// Anwendungsbefehl zum Ausführen des Dialogfelds
void CFileZillaserverApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CFileZillaserverApp-Nachrichtenbehandlungsroutinen

int CFileZillaserverApp::ExitInstance() 
{
	return CWinApp::ExitInstance();
}

BOOL CAboutDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_homepage.ModifyLinkStyle(0, CHyperLink::StyleUseHover);
	m_homepage.SetColors(0xFF0000, 0xFF0000, 
				   0xFF0000, 0xFF);
	m_mail.ModifyLinkStyle(0, CHyperLink::StyleUseHover);
	m_mail.SetColors(0xFF0000, 0xFF0000, 
				   0xFF0000, 0xFF);
	m_mail.SetURL("mailto:Tim.Kosse@gmx.de");

	m_cDonate.SubclassDlgItem(IDC_DONATE, this, "https://www.paypal.com/xclick/business=Tim.Kosse%40gmx.de&item_name=FileZilla&cn=Enter+your+comments+here&tax=0&currency_code=USD");
	
	m_cVersion.SetWindowText(GetVersionString());
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX-Eigenschaftenseiten sollten FALSE zurückgeben
}





















