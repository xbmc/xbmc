// ChildView.h : Schnittstelle der Klasse CChildView
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_CHILDVIEW_H__876AD783_C002_4B64_BB20_59EA097CD4EC__INCLUDED_)
#define AFX_CHILDVIEW_H__876AD783_C002_4B64_BB20_59EA097CD4EC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CChildView-Fenster

class CChildView : public CWnd
{
// Konstruktion
public:
	CChildView();

// Attribute
public:

// Operationen
public:

// Überladungen
	// Vom Klassen-Assistenten erstellte virtuelle Funktionsüberladungen
	//{{AFX_VIRTUAL(CChildView)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementierung
public:
	virtual ~CChildView();

	// Generierte Funktionen für die Nachrichtentabellen
protected:
	//{{AFX_MSG(CChildView)
	afx_msg void OnPaint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ fügt unmittelbar vor der vorherigen Zeile zusätzliche Deklarationen ein.

#endif // !defined(AFX_CHILDVIEW_H__876AD783_C002_4B64_BB20_59EA097CD4EC__INCLUDED_)
