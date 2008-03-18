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

#if !defined(OPTION_TYPES_INCLUDED)
#define OPTION_TYPES_INCLUDED

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// Options.h : Header-Datei
//
#define OPTION_SERVERPORT 1
#define OPTION_THREADNUM 2
#define OPTION_MAXUSERS 3
#define OPTION_TIMEOUT 4
#define OPTION_NOTRANSFERTIMEOUT 5
#define OPTION_INFXP 6
#define OPTION_OUTFXP 7
#define OPTION_NOINFXPSTRICT 8
#define OPTION_NOOUTFXPSTRICT 9
#define OPTION_LOGINTIMEOUT 10
#define OPTION_LOGSHOWPASS 11
#define OPTION_CUSTOMPASVIPTYPE 12
#define OPTION_CUSTOMPASVIP 13
#define OPTION_CUSTOMPASVMINPORT 14
#define OPTION_CUSTOMPASVMAXPORT 15
#define OPTION_WELCOMEMESSAGE 16
#define OPTION_ADMINPORT 17
#define OPTION_ADMINPASS 18
#define OPTION_ADMINIPBINDINGS 19
#define OPTION_ADMINIPADDRESSES 20
#define OPTION_ENABLELOGGING 21
#define OPTION_LOGLIMITSIZE 22
#define OPTION_LOGTYPE 23
#define OPTION_LOGDELETETIME 24
#define OPTION_USEGSS 25
#define OPTION_GSSPROMPTPASSWORD 26
#define OPTION_DOWNLOADSPEEDLIMITTYPE 27
#define OPTION_UPLOADSPEEDLIMITTYPE 28
#define OPTION_DOWNLOADSPEEDLIMIT 29
#define OPTION_UPLOADSPEEDLIMIT 30
#define OPTION_BUFFERSIZE 31
#define OPTION_CUSTOMPASVIPSERVER 32
#define OPTION_USECUSTOMPASVPORT 33

#define OPTIONS_NUM 33

struct t_Option
{
	TCHAR name[30];
	int nType;
	BOOL bOnlyLocal; //If TRUE, setting can only be changed from local connections
};

const DWORD SERVER_VERSION = 0x00080800;
const DWORD PROTOCOL_VERSION = 0x00010400;


static const t_Option m_Options[OPTIONS_NUM]={	_T("Serverport"),				1,	FALSE,
												_T("Number of Threads"),		1,	FALSE,
												_T("Maximum user count"),		1,	FALSE,
												_T("Timeout"),					1,	FALSE,
												_T("No Transfer Timeout"),		1,	FALSE,
												_T("Allow Incoming FXP"),		1,	FALSE,
												_T("Allow outgoing FXP"),		1,	FALSE,
												_T("No Strict In FXP"),			1,	FALSE,
												_T("No Strict Out FXP"),		1,	FALSE,
												_T("Login Timeout"),			1,	FALSE,
												_T("Show Pass in Log"),			1,	FALSE,
												_T("Custom PASV IP type"),		1,	FALSE,
												_T("Custom PASV IP"),			0,	FALSE,
												_T("Custom PASV min port"),		1,	FALSE,
												_T("Custom PASV max port"),		1,	FALSE,
												_T("Initial Welcome Message"),	0,	FALSE,
												_T("Admin port"),				1,	TRUE,
												_T("Admin Password"),			0,	TRUE,
												_T("Admin IP Bindings"),		0,	TRUE,
												_T("Admin IP Addresses"),		0,	TRUE,
												_T("Enable logging"),			1,	FALSE,
												_T("Logsize limit"),			1,	FALSE,
												_T("Logfile type"),				1,	FALSE,
												_T("Logfile delete time"),		1,	FALSE, 
												_T("Use GSS Support"),			1,	FALSE, 
												_T("GSS Prompt for Password"),	1,	FALSE,
												_T("Download Speedlimit Type"),	1,	FALSE,
												_T("Upload Speedlimit Type"),	1,	FALSE,
												_T("Download Speedlimit"),		1,	FALSE,
												_T("Upload Speedlimit"),		1,	FALSE,
												_T("Buffer Size"),				1,	FALSE,
												_T("Custom PASV IP server"),	0,	FALSE,
												_T("Use custom PASV ports"),	1,	FALSE
											};

#endif // OPTION_TYPES_INCLUDED
