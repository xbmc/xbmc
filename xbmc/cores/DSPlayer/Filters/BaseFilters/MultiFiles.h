/*
 * $Id: MultiFiles.h 1647 2010-02-09 20:12:31Z xhmikosr $
 *
 * (C) 2006-2010 see AUTHORS
 *
 * This file is part of mplayerc.
 *
 * Mplayerc is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mplayerc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#pragma once


#include "DShowUtil/DShowUtil.h"

class CMultiFiles : public CObject
{
	DECLARE_DYNAMIC(CMultiFiles)

public:
// Flag values
	enum OpenFlags {
		modeRead =         (int) 0x00000,
		modeWrite =        (int) 0x00001,
		modeReadWrite =    (int) 0x00002,
		shareCompat =      (int) 0x00000,
		shareExclusive =   (int) 0x00010,
		shareDenyWrite =   (int) 0x00020,
		shareDenyRead =    (int) 0x00030,
		shareDenyNone =    (int) 0x00040,
		modeNoInherit =    (int) 0x00080,
		modeCreate =       (int) 0x01000,
		modeNoTruncate =   (int) 0x02000,
		typeText =         (int) 0x04000, // typeText and typeBinary are
		typeBinary =       (int) 0x08000, // used in derived classes only
		osNoBuffer =       (int) 0x10000,
		osWriteThrough =   (int) 0x20000,
		osRandomAccess =   (int) 0x40000,
		osSequentialScan = (int) 0x80000,
		};

	enum Attribute {
		normal =    0x00,
		readOnly =  0x01,
		hidden =    0x02,
		system =    0x04,
		volume =    0x08,
		directory = 0x10,
		archive =   0x20
		};

	enum SeekPosition { begin = 0x0, current = 0x1, end = 0x2 };

// Constructors
	CMultiFiles();

	CStdString			m_strFileName;

// Operations
	virtual BOOL Open(LPCTSTR lpszFileName, UINT nOpenFlags);
	virtual BOOL OpenFiles(CAtlList<CHdmvClipInfo::PlaylistItem>& files, UINT nOpenFlags);


	virtual ULONGLONG Seek(LONGLONG lOff, UINT nFrom);
	virtual ULONGLONG GetLength() const;

	virtual UINT Read(void* lpBuf, UINT nCount);
	virtual void Close();

// Implementation
public:
	virtual ~CMultiFiles();

protected:
	REFERENCE_TIME*				m_pCurrentPTSOffset;
	std::vector<CStdString>			m_strFiles;
	std::vector<ULONGLONG>		m_FilesSize;
	std::vector<REFERENCE_TIME>	m_rtPtsOffsets;
	HANDLE						m_hFile;
	int							m_nCurPart;
	ULONGLONG					m_llTotalLength;

	BOOL						OpenPart(int nPart);
	void						ClosePart();
	ULONGLONG					GetAbsolutePosition(LONGLONG lOff, UINT nFrom);
	void						Reset();
};
