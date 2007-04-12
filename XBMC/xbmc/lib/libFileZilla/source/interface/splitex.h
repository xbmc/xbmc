////////////////////////////////////////////////////////////////////////////
//
// splitex.h
// Based upon code from Oleg G. Galkin
// Modified to handle multiple hidden rows/columns

#pragma once
class CSplitterWndEx : public CSplitterWnd
{
protected:

	int *m_arr;  // array the keeps track of actual row/columns versus perceived row/column
	int m_length; // length of above array

	int Id_short(int row, int col);

public:
	void GetRowInfoEx( int row, int& cyCur, int& cyMin );
	void GetColumnInfoEx( int row, int& cyCur, int& cyMin );
	BOOL IsRowHidden(int row);
	BOOL IsColumnHidden(int row);
	CWnd* GetPane(int row,int col);
    CSplitterWndEx();

	virtual ~CSplitterWndEx();


	void ShowRow(int row);
	void ShowColumn(int row);

	int IdFromRowCol(int row, int col) const;
    void HideRow(int rowHide,int rowToResize=-1);
	void HideColumn(int rowHide,int rowToResize=-1);

	BOOL IsChildPane(CWnd* pWnd, int* pRow, int* pCol);
	CWnd *GetActivePane(int* pRow, int* pCol);

// ClassWizard generated virtual function overrides
     //{{AFX_VIRTUAL(CSplitterWndEx)
     //}}AFX_VIRTUAL

// Generated message map functions
protected:
     //{{AFX_MSG(CSplitterWndEx)
      // NOTE - the ClassWizard will add and remove member functions here.
     //}}AFX_MSG

    DECLARE_MESSAGE_MAP()
};

