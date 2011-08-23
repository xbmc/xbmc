// Win32++   Version 7.2
// Released: 5th AUgust 2011
//
//      David Nash
//      email: dnash@bigpond.net.au
//      url: https://sourceforge.net/projects/win32-framework
//
//
// Copyright (c) 2005-2011  David Nash
//
// Permission is hereby granted, free of charge, to
// any person obtaining a copy of this software and
// associated documentation files (the "Software"),
// to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify,
// merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom
// the Software is furnished to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice
// shall be included in all copies or substantial portions
// of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
// ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
// TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
// PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
// SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
// ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
// OR OTHER DEALINGS IN THE SOFTWARE.
//
////////////////////////////////////////////////////////


#ifndef _WIN32XX_FILE_H_
#define _WIN32XX_FILE_H_


#include "wincore.h"

namespace Win32xx
{

	class CFile
	{
	public:
		CFile();
		CFile(HANDLE hFile);
		CFile(LPCTSTR pszFileName, UINT nOpenFlags);
		~CFile();
		operator HANDLE() const;

		BOOL Close();
		BOOL Flush();
		HANDLE GetHandle() const;
		ULONGLONG GetLength() const;
		const CString& GetFileName() const;
		const CString& GetFilePath() const;
		const CString& GetFileTitle() const;
		ULONGLONG GetPosition() const;
		BOOL LockRange(ULONGLONG Pos, ULONGLONG Count);
		BOOL Open(LPCTSTR pszFileName, UINT nOpenFlags);
		CString OpenFileDialog(LPCTSTR pszFilePathName = NULL,
						DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, LPCTSTR pszFilter = NULL,
						CWnd* pOwnerWnd = NULL);
		UINT Read(void* pBuf, UINT nCount);
		static BOOL Remove(LPCTSTR pszFileName);
		static BOOL Rename(LPCTSTR pszOldName, LPCTSTR pszNewName);
		CString SaveFileDialog(LPCTSTR pszFilePathName = NULL,
						DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, LPCTSTR pszFilter = NULL,
						LPCTSTR pszDefExt = NULL, CWnd* pOwnerWnd = NULL);
		ULONGLONG Seek(LONGLONG lOff, UINT nFrom);
		void SeekToBegin();
		ULONGLONG SeekToEnd();
		void SetFilePath(LPCTSTR pszNewName);
		BOOL SetLength(ULONGLONG NewLen);
		BOOL UnlockRange(ULONGLONG Pos, ULONGLONG Count);
		BOOL Write(const void* pBuf, UINT nCount);

	private:
		CFile(const CFile&);				// Disable copy construction
		CFile& operator = (const CFile&);	// Disable assignment operator
		CString m_FileName;
		CString m_FilePath;
		CString m_FileTitle;
		HANDLE m_hFile;
	};

}



namespace Win32xx
{
	inline CFile::CFile() : m_hFile(0)
	{
	}

	inline CFile::CFile(HANDLE hFile) : m_hFile(hFile)
	{
	}

	inline CFile::CFile(LPCTSTR pszFileName, UINT nOpenFlags) : m_hFile(0)
	{
		assert(pszFileName);
		Open(pszFileName, nOpenFlags);
		assert(m_hFile);
	}
	
	inline CFile::~CFile()
	{
		Close();
	}

	inline CFile::operator HANDLE() const
	{
		return m_hFile;
	}

	inline BOOL CFile::Close()
	// Closes the file associated with this object. Closed file can no longer be read or written to.
	{
		BOOL bResult = TRUE;
		if (m_hFile)
			bResult = CloseHandle(m_hFile);

		m_hFile = 0;
		return bResult;
	}

	inline BOOL CFile::Flush()
	// Causes any remaining data in the file buffer to be written to the file.
	{
		assert(m_hFile);
		return FlushFileBuffers(m_hFile);
	}
	
	inline HANDLE CFile::GetHandle() const
	{
		return m_hFile;
	}

	inline ULONGLONG CFile::GetLength( ) const
	// Returns the length of the file in bytes.
	{
		assert(m_hFile);

		LONG High = 0;
		DWORD LowPos = SetFilePointer(m_hFile, 0, &High, FILE_END);

		ULONGLONG Result = ((ULONGLONG)High << 32) + LowPos;
		return Result;
	}

	inline const CString& CFile::GetFileName() const
	// Returns the filename of the file associated with this object.
	{
		return (const CString&)m_FileName;
	}

	inline const CString& CFile::GetFilePath() const
	// Returns the full filename including the directory of the file associated with this object.
	{
		return (const CString&)m_FilePath;
	}

	inline const CString& CFile::GetFileTitle() const
	// Returns the filename of the file associated with this object, excluding the path and the file extension
	{
		return (const CString&)m_FileTitle;
	}

	inline ULONGLONG CFile::GetPosition() const
	// Returns the current value of the file pointer, which can be used in subsequent calls to Seek.
	{
		assert(m_hFile);
		LONG High = 0;
		DWORD LowPos = SetFilePointer(m_hFile, 0, &High, FILE_CURRENT);

		ULONGLONG Result = ((ULONGLONG)High << 32) + LowPos;
		return Result;
	}

	inline BOOL CFile::LockRange(ULONGLONG Pos, ULONGLONG Count)
	// Locks a range of bytes in and open file.
	{
		assert(m_hFile);

		DWORD dwPosHigh = (DWORD)(Pos >> 32);
		DWORD dwPosLow = (DWORD)(Pos & 0xFFFFFFFF);
		DWORD dwCountHigh = (DWORD)(Count >> 32);
		DWORD dwCountLow = (DWORD)(Count & 0xFFFFFFFF);

		return ::LockFile(m_hFile, dwPosLow, dwPosHigh, dwCountLow, dwCountHigh);
	}

	inline BOOL CFile::Open(LPCTSTR pszFileName, UINT nOpenFlags)
	// Prepares a file to be written to or read from.
	{
		if (m_hFile) Close();

		m_hFile = ::CreateFile(pszFileName, GENERIC_READ | GENERIC_WRITE, 0, NULL, nOpenFlags, FILE_ATTRIBUTE_NORMAL, NULL);

		if (INVALID_HANDLE_VALUE == m_hFile)
		{
			TRACE(_T("Failed\n"));
			m_hFile = 0;
		}

		if (m_hFile)
		{
			SetFilePath(pszFileName);
		}

		return (m_hFile != 0);
	}

	inline CString CFile::OpenFileDialog(LPCTSTR pszFilePathName, DWORD dwFlags, LPCTSTR pszFilter, CWnd* pOwnerWnd)
	// Displays the file open dialog. 
	// Returns a CString containing either the selected file name or an empty CString.
	{
		CString str;
		if (pszFilePathName)
			str = pszFilePathName;

		OPENFILENAME ofn = {0};
		ofn.lStructSize = sizeof(OPENFILENAME);

#if defined OPENFILENAME_SIZE_VERSION_400
		if (GetWinVersion() < 2500)
			ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
#endif

		ofn.hwndOwner = pOwnerWnd? pOwnerWnd->GetHwnd() : NULL;
		ofn.hInstance = GetApp()->GetInstanceHandle();
		ofn.lpstrFilter = pszFilter;
		ofn.lpstrTitle = _T("Open File");
		ofn.Flags = dwFlags;
		ofn.nMaxFile = _MAX_PATH;
		
		ofn.lpstrFile = (LPTSTR)str.GetBuffer(_MAX_PATH);
		::GetOpenFileName(&ofn);
		str.ReleaseBuffer();

		return str;
	}

	inline UINT CFile::Read(void* pBuf, UINT nCount)
	// Reads from the file, storing the contents in the specified buffer.
	{
		assert(m_hFile);
		DWORD dwRead = 0;

		if (!::ReadFile(m_hFile, pBuf, nCount, &dwRead, NULL))
			dwRead = 0;

		return dwRead;
	}

	inline BOOL CFile::Rename(LPCTSTR pszOldName, LPCTSTR pszNewName)
	// Renames the specified file.
	{
		return ::MoveFile(pszOldName, pszNewName);
	}

	inline BOOL CFile::Remove(LPCTSTR pszFileName)
	// Deletes the specified file.
	{
		return ::DeleteFile(pszFileName);
	}

	inline CString CFile::SaveFileDialog(LPCTSTR pszFilePathName, DWORD dwFlags, LPCTSTR pszFilter, LPCTSTR pszDefExt, CWnd* pOwnerWnd)
	// Displays the SaveFileDialog.
	// Returns a CString containing either the selected file name or an empty CString
	{
		CString str;
		if (pszFilePathName)
			str = pszFilePathName;

		OPENFILENAME ofn = {0};
		ofn.lStructSize = sizeof(OPENFILENAME);

#if defined OPENFILENAME_SIZE_VERSION_400
		if (GetWinVersion() < 2500)
			ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
#endif

		ofn.hwndOwner = pOwnerWnd? pOwnerWnd->GetHwnd() : NULL;
		ofn.hInstance = GetApp()->GetInstanceHandle();
		ofn.lpstrFilter = pszFilter;
		ofn.lpstrFile = (LPTSTR)pszFilePathName;
		ofn.lpstrFileTitle = (LPTSTR)pszFilePathName;
		ofn.lpstrDefExt = pszDefExt;
		ofn.nMaxFile = lstrlen(pszFilePathName);
		ofn.lpstrTitle = _T("Save File");
		ofn.Flags = dwFlags;
		ofn.nMaxFile = _MAX_PATH;
		ofn.lpstrFile = (LPTSTR)str.GetBuffer(_MAX_PATH);
		::GetSaveFileName(&ofn);
		str.ReleaseBuffer();

		return str;
	}

	inline ULONGLONG CFile::Seek(LONGLONG lOff, UINT nFrom)
	// Positions the current file pointer.
	// Permitted values for nFrom are: FILE_BEGIN, FILE_CURRENT, or FILE_END.
	{
		assert(m_hFile);
		assert(nFrom == FILE_BEGIN || nFrom == FILE_CURRENT || nFrom == FILE_END);

		LONG High = LONG(lOff >> 32);
		LONG Low = (LONG)(lOff & 0xFFFFFFFF);

		DWORD LowPos = SetFilePointer(m_hFile, Low, &High, nFrom);

		ULONGLONG Result = ((ULONGLONG)High << 32) + LowPos;
		return Result;
	}

	inline void CFile::SeekToBegin()
	// Sets the current file pointer to the beginning of the file.
	{
		assert(m_hFile);
		Seek(0, FILE_BEGIN);
	}

	inline ULONGLONG CFile::SeekToEnd()
	// Sets the current file pointer to the end of the file.
	{
		assert(m_hFile);
		return Seek(0, FILE_END);
	}

	inline void CFile::SetFilePath(LPCTSTR pszFileName)
	// Specifies the full file name, including its path
	{
		TCHAR* pFileName = NULL;
		int nBuffSize = ::GetFullPathName(pszFileName, 0, 0, 0);
		if (nBuffSize > 0)
		{
			TCHAR* pBuff = m_FilePath.GetBuffer(nBuffSize);
			::GetFullPathName(pszFileName, nBuffSize, pBuff, &pFileName);
			m_FilePath.ReleaseBuffer();
			m_FileName = pFileName;
			int nPos = m_FileName.ReverseFind(_T("."));
			if (nPos >= 0)
				m_FileTitle = m_FileName.Left(nPos);
		}
	}

	inline BOOL CFile::SetLength(ULONGLONG NewLen)
	// Changes the length of the file to the specified value.
	{
		assert(m_hFile);

		Seek(NewLen, FILE_BEGIN);
		return ::SetEndOfFile(m_hFile);
	}

	inline BOOL CFile::UnlockRange(ULONGLONG Pos, ULONGLONG Count)
	// Unlocks a range of bytes in an open file.
	{
		assert(m_hFile);

		DWORD dwPosHigh = (DWORD)(Pos >> 32);
		DWORD dwPosLow = (DWORD)(Pos & 0xFFFFFFFF);
		DWORD dwCountHigh = (DWORD)(Count >> 32);
		DWORD dwCountLow = (DWORD)(Count & 0xFFFFFFFF);

		return ::UnlockFile(m_hFile, dwPosLow, dwPosHigh, dwCountLow, dwCountHigh);
	}

	inline BOOL CFile::Write(const void* pBuf, UINT nCount)
	// Writes the specified buffer to the file.
	{
		assert(m_hFile);
		DWORD dwWritten = 0;
		BOOL bResult = ::WriteFile(m_hFile, pBuf, nCount, &dwWritten, NULL);
		if (dwWritten != nCount)
			bResult = FALSE;

		return bResult;
	}


}	// namespace Win32xx

#endif
