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
#define OPTION_CUSTOMPASVENABLE 12
#define OPTION_CUSTOMPASVIP 13
#define OPTION_CUSTOMPASVMINPORT 14
#define OPTION_CUSTOMPASVMAXPORT 15
#define OPTION_WELCOMEMESSAGE 16
#define OPTION_ADMINPORT 17

#define OPTIONS_NUM 17

struct t_Option
{
	char name[30];
	int nType;
};

static const t_Option m_Options[OPTIONS_NUM]={	"Serverport",				1,	"Number of Threads",		1,
												"Maximum user count",		1,	"Timeout",					1,
												"No Transfer Timeout",		1,	"Allow Incoming FXP",		1,
												"Allow outgoing FXP",		1,	"No Strict In FXP",			1,
												"No Strict Out FXP",		1,	"Login Timeout",			1,
												"Show Pass in Log",			1,	"Custom PASV Enable",		1,
												"Custom PASV IP",			0,	"Custom PASV min port",		1,
												"Custom PASV max port",		1,	"Initial Welcome Message",	0,
												"Admin port",				1};

#endif // OPTION_TYPES_INCLUDED
