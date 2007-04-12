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

// SpeedLimitRuleDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SpeedLimitRuleDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSpeedLimitRuleDlg dialog


CSpeedLimitRuleDlg::CSpeedLimitRuleDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSpeedLimitRuleDlg::IDD, pParent)
{
	m_Day[ 0] = TRUE;
	m_Day[ 1] = TRUE;
	m_Day[ 2] = TRUE;
	m_Day[ 3] = TRUE;
	m_Day[ 4] = TRUE;
	m_Day[ 5] = TRUE;
	m_Day[ 6] = TRUE;
	//{{AFX_DATA_INIT(CSpeedLimitRuleDlg)
	m_DateCheck = FALSE;
	m_Date = CTime::GetCurrentTime();
	m_FromCheck = FALSE;
	m_FromTime = CTime::GetCurrentTime();
	m_ToCheck = FALSE;
	m_ToTime = CTime::GetCurrentTime();
	m_Speed = 8;
	//}}AFX_DATA_INIT
}


void CSpeedLimitRuleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSpeedLimitRuleDlg)
	DDX_Control(pDX, IDC_TO_TIME, m_ToCtrl);
	DDX_Control(pDX, IDC_FROM_TIME, m_FromCtrl);
	DDX_Control(pDX, IDC_DATE_VALUE, m_DateCtrl);
	DDX_Check(pDX, IDC_DATE_CHECK, m_DateCheck);
	DDX_DateTimeCtrl(pDX, IDC_DATE_VALUE, m_Date);
	DDX_Check(pDX, IDC_FROM_CHECK, m_FromCheck);
	DDX_DateTimeCtrl(pDX, IDC_FROM_TIME, m_FromTime);
	DDX_Check(pDX, IDC_TO_CHECK, m_ToCheck);
	DDX_DateTimeCtrl(pDX, IDC_TO_TIME, m_ToTime);
	DDX_Text(pDX, IDC_SPEED, m_Speed);
	DDV_MinMaxInt(pDX, m_Speed, 1, 1000000000);
	//}}AFX_DATA_MAP
	DDX_Check(pDX, IDC_CHECK_DAY1, m_Day[0]);
	DDX_Check(pDX, IDC_CHECK_DAY2, m_Day[1]);
	DDX_Check(pDX, IDC_CHECK_DAY3, m_Day[2]);
	DDX_Check(pDX, IDC_CHECK_DAY4, m_Day[3]);
	DDX_Check(pDX, IDC_CHECK_DAY5, m_Day[4]);
	DDX_Check(pDX, IDC_CHECK_DAY6, m_Day[5]);
	DDX_Check(pDX, IDC_CHECK_DAY7, m_Day[6]);
}


BEGIN_MESSAGE_MAP(CSpeedLimitRuleDlg, CDialog)
	//{{AFX_MSG_MAP(CSpeedLimitRuleDlg)
	ON_BN_CLICKED(IDC_DATE_CHECK, OnDateCheck)
	ON_BN_CLICKED(IDC_TO_CHECK, OnToCheck)
	ON_BN_CLICKED(IDC_FROM_CHECK, OnFromCheck)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpeedLimitRuleDlg message handlers

void CSpeedLimitRuleDlg::OnDateCheck() 
{
	// TODO: Add your control notification handler code here
	UpdateData();

	m_DateCtrl.EnableWindow( m_DateCheck);
}

void CSpeedLimitRuleDlg::OnToCheck() 
{
	// TODO: Add your control notification handler code here
	UpdateData();

	m_ToCtrl.EnableWindow( m_ToCheck);
}

void CSpeedLimitRuleDlg::OnFromCheck() 
{
	// TODO: Add your control notification handler code here
	UpdateData();

	m_FromCtrl.EnableWindow( m_FromCheck);
}

BOOL CSpeedLimitRuleDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	m_DateCtrl.EnableWindow( m_DateCheck);
	m_ToCtrl.EnableWindow( m_ToCheck);
	m_FromCtrl.EnableWindow( m_FromCheck);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

CSpeedLimit CSpeedLimitRuleDlg::GetSpeedLimit()
{
	CSpeedLimit res;
	
	res.m_DateCheck = m_DateCheck;
	res.m_Date.y = m_Date.GetYear();
	res.m_Date.m = m_Date.GetMonth();
	res.m_Date.d = m_Date.GetDay();
	res.m_FromCheck = m_FromCheck;
	res.m_ToCheck = m_ToCheck;
	res.m_FromTime.h = m_FromTime.GetHour();
	res.m_FromTime.m = m_FromTime.GetMinute();
	res.m_FromTime.s = m_FromTime.GetSecond();
	res.m_ToTime.h = m_ToTime.GetHour();
	res.m_ToTime.m = m_ToTime.GetMinute();
	res.m_ToTime.s = m_ToTime.GetSecond();
	res.m_Speed = m_Speed;

	res.m_Day = 0;
	for (int i = 0; i < 7; i++)
		if (m_Day[i])
			res.m_Day |= 1 << i;

	return res;
}

void CSpeedLimitRuleDlg::FillFromSpeedLimit(const CSpeedLimit &sl)
{
	m_DateCheck = sl.m_DateCheck;
	m_Date = CTime(sl.m_Date.y, sl.m_Date.m, sl.m_Date.d, 0, 0, 0);
	m_FromCheck = sl.m_FromCheck;
	m_FromTime = CTime(2011, 1, 1, sl.m_FromTime.h, sl.m_FromTime.m, sl.m_FromTime.s);
	m_ToCheck = sl.m_ToCheck;
	m_ToTime = CTime(2011, 1, 1, sl.m_ToTime.h, sl.m_ToTime.m, sl.m_ToTime.s);;
	m_Speed = sl.m_Speed;

	for (int i = 0; i < 7; i++)
		m_Day[i] = (sl.m_Day >> i) % 2;
}

