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

// stdafx.cpp : Quelltextdatei, die nur die Standard-Includes einbindet
//	FileZilla server.pch ist die vorcompilierte Header-Datei
//	stdafx.obj enthält die vorcompilierte Typinformation

#include "./stdafx.h"



HWND hMainWnd = NULL;

CCriticalSectionWrapper::CCriticalSectionWrapper()
{
	m_bInitialized = TRUE;
	InitializeCriticalSection(&m_criticalSection);
}

CCriticalSectionWrapper::~CCriticalSectionWrapper()
{
	if (m_bInitialized)
		DeleteCriticalSection(&m_criticalSection);
	m_bInitialized = FALSE;
}

void CCriticalSectionWrapper::Lock()
{
	if (m_bInitialized)
		EnterCriticalSection(&m_criticalSection);
}

void CCriticalSectionWrapper::Unlock()
{
	if (m_bInitialized)
		LeaveCriticalSection(&m_criticalSection);
}
