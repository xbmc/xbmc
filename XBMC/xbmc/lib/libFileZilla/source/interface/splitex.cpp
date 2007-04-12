////////////////////////////////////////////////////////////////////////////
//
// splitex.cpp
// Based upon code from Oleg G. Galkin
// Modified to handle multiple hidden rows

#include "stdafx.h"
#include "splitex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

////////////////////////////////////////////////////////////////////////////
//
// CSplitterWndEx

CSplitterWndEx::CSplitterWndEx()
{
	m_arr = 0;
}

CSplitterWndEx::~CSplitterWndEx()
{
	if (m_arr)
		delete [] m_arr;
}

int CSplitterWndEx::Id_short(int row, int col)
{
	return AFX_IDW_PANE_FIRST + row * 16 + col;
}

void CSplitterWndEx::ShowRow(int r)
{
    ASSERT_VALID(this);
    ASSERT(m_nRows < m_nMaxRows);

	ASSERT(m_arr);
	ASSERT(r < m_length);
	ASSERT(m_arr[r] >= m_nRows);
	ASSERT(m_arr[r] < m_length);

	int rowNew = r;
	int cyNew = m_pRowInfo[m_arr[r]].nCurSize;
	int cyIdealNew = m_pRowInfo[m_arr[r]].nIdealSize;
	
	int new_val = 0;

	for (int i = rowNew - 1; i >= 0; i--)
		if (m_arr[i] < m_nRows)	// not hidden
		{
			new_val = m_arr[i] + 1;
			break;
		}

	int old_val = m_arr[rowNew];

    m_nRows++;  // add a row

    // fill the hided row
    int row;
    for (int col = 0; col < m_nCols; col++)
    {
		CWnd* pPaneShow = GetDlgItem(
			Id_short(old_val, col));
        ASSERT(pPaneShow != NULL);
        pPaneShow->ShowWindow(SW_SHOWNA);

		for (row = m_length - 1; row >= 0; row--)
        {
			if ((m_arr[row] >= new_val) &&
				(m_arr[row] < old_val))
			{
				CWnd* pPane = CSplitterWnd::GetPane(m_arr[row], col);
				ASSERT(pPane != NULL);
				pPane->SetDlgCtrlID(Id_short(m_arr[row] + 1, col));
			}
        }
		pPaneShow->SetDlgCtrlID(Id_short(new_val, col));
    }

	for (row = 0; row < m_length; row++)
		if ((m_arr[row] >= new_val) &&
			(m_arr[row] < old_val))
			m_arr[row]++;

	m_arr[rowNew] = new_val;

    //new panes have been created -- recalculate layout
	for (row = new_val + 1; row < m_length; row++)
	{
		if (m_arr[row]<m_nRows)
		{
			m_pRowInfo[m_arr[row]].nIdealSize = m_pRowInfo[m_arr[row-1]].nCurSize;
			m_pRowInfo[m_arr[row]].nCurSize = m_pRowInfo[m_arr[row-1]].nCurSize;
		}
	}
	if (cyNew>=0x10000)
	{
		int rowToResize=(cyNew>>16)-1;
		cyNew%=0x10000;
		cyIdealNew%=0x10000;
		m_pRowInfo[m_arr[rowToResize]].nCurSize-=cyNew+m_cxSplitter;
		m_pRowInfo[m_arr[rowToResize]].nIdealSize=m_pRowInfo[m_arr[rowToResize]].nCurSize;//-=cyIdealNew+m_cxSplitter;
	}

	m_pRowInfo[new_val].nIdealSize = cyNew;
	m_pRowInfo[new_val].nCurSize = cyNew;
    RecalcLayout();
}

void CSplitterWndEx::ShowColumn(int c)
{
    ASSERT_VALID(this);
    ASSERT(m_nCols < m_nMaxCols);

	ASSERT(m_arr);
	ASSERT(c < m_length);
	ASSERT(m_arr[c] >= m_nRows);
	ASSERT(m_arr[c] < m_length);

	int colNew = c;
	int cxNew = m_pColInfo[m_arr[c]].nCurSize;
	int cxIdealNew = m_pColInfo[m_arr[c]].nIdealSize;
	
	int new_val = 0;

	for (int i = colNew - 1; i >= 0; i--)
		if (m_arr[i] < m_nCols)	// not hidden
		{
			new_val = m_arr[i] + 1;
			break;
		}

	int old_val = m_arr[colNew];

    m_nCols++;  // add a col

    // fill the hided col
    int col;
    for (int row = 0; row < m_nRows; row++)
    {
		CWnd* pPaneShow = GetDlgItem(
			Id_short(row, old_val));
        ASSERT(pPaneShow != NULL);
        pPaneShow->ShowWindow(SW_SHOWNA);

		for (col = m_length - 1; col >= 0; col--)
        {
			if ((m_arr[col] >= new_val) &&
				(m_arr[col] < old_val))
			{
				CWnd* pPane = CSplitterWnd::GetPane(row, m_arr[col]);
				ASSERT(pPane != NULL);
				pPane->SetDlgCtrlID(Id_short(row, m_arr[col]+1));
			}
        }
		pPaneShow->SetDlgCtrlID(Id_short(row, new_val));
    }

	for (col = 0; col < m_length; col++)
		if ((m_arr[col] >= new_val) &&
			(m_arr[col] < old_val))
			m_arr[col]++;

	m_arr[colNew] = new_val;

    //new panes have been created -- recalculate layout
	for (col = new_val + 1; col < m_length; col++)
	{
		if (m_arr[col]<m_nCols)
		{
			m_pColInfo[m_arr[col]].nIdealSize = m_pColInfo[m_arr[col-1]].nCurSize;
			m_pColInfo[m_arr[col]].nCurSize = m_pColInfo[m_arr[col-1]].nCurSize;
		}
	}
	if (cxNew>=0x10000)
	{
		int colToResize=(cxNew>>16)-1;
		cxNew%=0x10000;
		cxIdealNew%=0x10000;
		m_pColInfo[m_arr[colToResize]].nCurSize-=cxNew+m_cySplitter;
		m_pColInfo[m_arr[colToResize]].nIdealSize=m_pColInfo[m_arr[colToResize]].nCurSize;//-=cxIdealNew+m_cySplitter;
	}

	m_pColInfo[new_val].nIdealSize = cxNew;
	m_pColInfo[new_val].nCurSize = cxNew;
    RecalcLayout();
}

void CSplitterWndEx::HideRow(int rowHide,int rowToResize)
{
    ASSERT_VALID(this);
    ASSERT(m_nRows > 1);

	if (m_arr)
		ASSERT(m_arr[rowHide] < m_nRows);

    // if the row has an active window -- change it
    int rowActive, colActive;

	if (!m_arr)
	{
		m_arr = new int[m_nRows];
		for (int i = 0; i < m_nRows; i++)
			m_arr[i] = i;
		m_length = m_nRows;
	}

	if (GetActivePane(&rowActive, &colActive) != NULL &&
        rowActive == rowHide) //colActive == rowHide)
    {
        if (++rowActive >= m_nRows)
			rowActive = 0;
        //SetActivePane(rowActive, colActive);

		SetActivePane(rowActive, colActive);
    }

    // hide all row panes
    for (int col = 0; col < m_nCols; col++)
    {
        CWnd* pPaneHide = CSplitterWnd::GetPane(m_arr[rowHide], col);
        ASSERT(pPaneHide != NULL);
	    pPaneHide->ShowWindow(SW_HIDE);

		for (int row = rowHide + 1; row < m_length; row++)
        {
			if (m_arr[row] < m_nRows )
			{
				CWnd* pPane = CSplitterWnd::GetPane(m_arr[row], col);
				ASSERT(pPane != NULL);
				pPane->SetDlgCtrlID(Id_short(row-1, col));
				m_arr[row]--;
			}
        }
        pPaneHide->SetDlgCtrlID(
			Id_short(m_nRows -1 , col));
    }

	int oldsize=m_pRowInfo[m_arr[rowHide]].nCurSize;
	int oldidealsize=m_pRowInfo[m_arr[rowHide]].nIdealSize;
	for (int row=rowHide;row<(m_length-1);row++)
	{
		if (m_arr[row+1] < m_nRows )
		{
			m_pRowInfo[m_arr[row]].nCurSize=m_pRowInfo[m_arr[row+1]].nCurSize;
			m_pRowInfo[m_arr[row]].nIdealSize=m_pRowInfo[m_arr[row+1]].nCurSize;		
		}
	}
	if (rowToResize!=-1)
	{
		m_pRowInfo[m_arr[rowToResize]].nCurSize+=oldsize+m_cySplitter;
		m_pRowInfo[m_arr[rowToResize]].nIdealSize+=oldsize+m_cySplitter;
		oldsize+=0x10000*(rowToResize+1);
		oldidealsize+=0x10000*(rowToResize+1);
	}

	m_pRowInfo[m_nRows - 1].nCurSize =oldsize;
	m_pRowInfo[m_nRows - 1].nIdealSize =oldsize;
	
	m_arr[rowHide] = m_nRows-1;
	

    m_nRows--;
	RecalcLayout();
}

void CSplitterWndEx::HideColumn(int colHide, int colToResize)
{
    ASSERT_VALID(this);
    ASSERT(m_nCols > 1);

	if (m_arr)
		ASSERT(m_arr[colHide] < m_nCols);

    // if the col has an active window -- change it
    int colActive, rowActive;

	if (!m_arr)
	{
		m_arr = new int[m_nCols];
		for (int i = 0; i < m_nCols; i++)
			m_arr[i] = i;
		m_length = m_nCols;
	}

	if (GetActivePane(&rowActive, &colActive) != NULL &&
        colActive == colHide)
    {
        if (++colActive >= m_nCols)
			colActive = 0;
        SetActivePane(rowActive, colActive);
    }

    // hide all row panes
    for (int row = 0; row < m_nRows; row++)
    {
        CWnd* pPaneHide = CSplitterWnd::GetPane(row, m_arr[colHide]);
        ASSERT(pPaneHide != NULL);
	    pPaneHide->ShowWindow(SW_HIDE);

		for (int col = colHide + 1; col < m_length; col++)
        {
			if (m_arr[col] < m_nCols )
			{
				CWnd* pPane = CSplitterWnd::GetPane(row, m_arr[col]);
				ASSERT(pPane != NULL);
				pPane->SetDlgCtrlID(Id_short(row, col-1));
				m_arr[col]--;
			}
        }
        pPaneHide->SetDlgCtrlID(
			Id_short(row, m_nCols -1));
    }

	int oldsize=m_pColInfo[m_arr[colHide]].nCurSize;
	int oldidealsize=m_pColInfo[m_arr[colHide]].nIdealSize;
	for (int col=colHide; col<(m_length-1); col++)
	{
		if (m_arr[col+1] < m_nCols )
		{
			m_pColInfo[m_arr[col]].nCurSize=m_pColInfo[m_arr[col+1]].nCurSize;
			m_pColInfo[m_arr[col]].nIdealSize=m_pColInfo[m_arr[col+1]].nCurSize;		
		}
	}
	if (colToResize!=-1)
	{
		m_pColInfo[m_arr[colToResize]].nCurSize+=oldsize+m_cxSplitter;
		m_pColInfo[m_arr[colToResize]].nIdealSize+=oldsize+m_cxSplitter;
		oldsize+=0x10000*(colToResize+1);
		oldidealsize+=0x10000*(colToResize+1);
	}

	m_pColInfo[m_nCols - 1].nCurSize =oldsize;
	m_pColInfo[m_nCols - 1].nIdealSize =oldsize;
	
	m_arr[colHide] = m_nCols-1;

    m_nCols--;
	RecalcLayout();
}

BEGIN_MESSAGE_MAP(CSplitterWndEx, CSplitterWnd)
//{{AFX_MSG_MAP(CSplitterWndEx)
  // NOTE - the ClassWizard will add and remove mapping macros here.
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CWnd* CSplitterWndEx::GetPane(int row, int col)
{
	if (!m_arr)
		return CSplitterWnd::GetPane(row,col);
	else
	{
		ASSERT_VALID(this);

		CWnd* pView = GetDlgItem(IdFromRowCol(m_arr[row], col));
		ASSERT(pView != NULL);  // panes can be a CWnd, but are usually CViews
		return pView;
	}
}

int CSplitterWndEx::IdFromRowCol(int row, int col) const
{
	ASSERT_VALID(this);
	ASSERT(row >= 0);
	ASSERT(row < (m_arr?m_length:m_nRows));
	ASSERT(col >= 0);
	ASSERT(col < m_nCols);

	return AFX_IDW_PANE_FIRST + row * 16 + col;
}

BOOL CSplitterWndEx::IsChildPane(CWnd* pWnd, int* pRow, int* pCol)
{
	ASSERT_VALID(this);
	ASSERT_VALID(pWnd);

	UINT nID = ::GetDlgCtrlID(pWnd->m_hWnd);
	if (IsChild(pWnd) && nID >= AFX_IDW_PANE_FIRST && nID <= AFX_IDW_PANE_LAST)
	{
		if (pWnd->GetParent()!=this)
			return FALSE;
		if (pRow != NULL)
			*pRow = (nID - AFX_IDW_PANE_FIRST) / 16;
		if (pCol != NULL)
			*pCol = (nID - AFX_IDW_PANE_FIRST) % 16;
		ASSERT(pRow == NULL || *pRow < (m_arr?m_length:m_nRows));
		ASSERT(pCol == NULL || *pCol < m_nCols);
		return TRUE;
	}
	else
	{
		if (pRow != NULL)
			*pRow = -1;
		if (pCol != NULL)
			*pCol = -1;
		return FALSE;
	}
}

CWnd* CSplitterWndEx::GetActivePane(int* pRow, int* pCol)
	// return active view, NULL when no active view
{
	ASSERT_VALID(this);

	// attempt to use active view of frame window
	CWnd* pView = NULL;
	CFrameWnd* pFrameWnd = GetParentFrame();
	ASSERT_VALID(pFrameWnd);
	pView = pFrameWnd->GetActiveView();

	// failing that, use the current focus
	if (pView == NULL)
		pView = GetFocus();

	// make sure the pane is a child pane of the splitter
	if (pView != NULL && !IsChildPane(pView, pRow, pCol))
		pView = NULL;

	return pView;
}

BOOL CSplitterWndEx::IsRowHidden(int row)
{
	return m_arr[row]>=m_nRows;

}

void CSplitterWndEx::GetRowInfoEx(int row, int &cyCur, int &cyMin)
{
	if (!m_arr)
		GetRowInfo(row,cyCur,cyMin);
	else
	{
		if (m_pRowInfo[m_arr[row]].nCurSize>0x10000)
			cyCur=m_pRowInfo[m_arr[row]].nCurSize/0x10000;
		else
			cyCur=m_pRowInfo[m_arr[row]].nCurSize%0x10000;
		cyMin=0;
	}
}
