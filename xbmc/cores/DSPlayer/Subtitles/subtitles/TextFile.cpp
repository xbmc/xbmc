/* 
 *	Copyright (C) 2003-2006 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "TextFile.h"

CTextFile::CTextFile(enc e)
{
	m_encoding = m_defaultencoding = e;
	m_offset = 0;
}

bool CTextFile::Open(LPCTSTR lpszFileName)
{
	if(! __super::Open(lpszFileName))
		return(false);

  m_strFileName = lpszFileName;

	m_encoding = m_defaultencoding;
	m_offset = 0;

  if(__super::GetSize() >= 2)
	{
		WORD w;
		if(sizeof(w) != Read((char *)&w, sizeof(w)))
			return Close(), false;

		if(w == 0xfeff)
		{
			m_encoding = LE16;
			m_offset = 2;
		}
		else if(w == 0xfffe)
		{
			m_encoding = BE16;
			m_offset = 2;
		}
    else if(w == 0xbbef && __super::GetSize() >= 3)
		{
			BYTE b;
			if(sizeof(b) != Read((char *)&b, sizeof(b)))
				return Close(), false;

			if(b == 0xbf)
			{
				m_encoding = UTF8;
				m_offset = 3;
			}
		}
	}

	if(m_encoding == m_defaultencoding)
	{
    __super::Close(); // CWebTextFile::Close() would delete the temp file if we called it...
		if(!__super::Open(lpszFileName))
			return(false);
	}

	return(true);
}

bool CTextFile::Save(LPCTSTR lpszFileName, enc e)
{
	if(!__super::Open(lpszFileName))
		return(false);

	if(e == UTF8)
	{
		BYTE b[3] = {0xef,0xbb,0xbf};
		Write((const char*)b, sizeof(b));
	}
	else if(e == LE16)
	{
		BYTE b[2] = {0xff,0xfe};
		Write((const char*)b, sizeof(b));
	}
	else if(e == BE16)
	{
		BYTE b[2] = {0xfe,0xff};
		Write((const char*)b, sizeof(b));
	}

	m_encoding = e;

	return true;
}

void CTextFile::SetEncoding(enc e)
{
	m_encoding = e;
}

CTextFile::enc CTextFile::GetEncoding()
{
	return m_encoding;
}

bool CTextFile::IsUnicode()
{
	return m_encoding == UTF8 || m_encoding == LE16 || m_encoding == BE16;
}

// CFile

CStdString CTextFile::GetFilePath() const
{
	// to avoid a CException coming from CTime
	return m_strFileName; // __super::GetFilePath();
}

// CStdioFile

ULONGLONG CTextFile::GetPosition()
{
  return(ATL::CFileT<true>::GetPosition() - m_offset);
}

ULONGLONG CTextFile::GetLength()
{
	return(ATL::CFileT<true>::GetSize() - m_offset);
}

ULONGLONG CTextFile::Seek(LONGLONG lOff, UINT nFrom)
{
	ULONGLONG pos = GetPosition();
	ULONGLONG len = GetLength();

	switch(nFrom)
	{
	default:
  case std::ios_base::beg: lOff = lOff; break;
	case std::ios_base::cur: lOff = pos + lOff; break;
	case std::ios_base::end: lOff = len - lOff; break;
	}

	lOff = max(min(lOff, len), 0) + m_offset;

	pos = ATL::CFileT<true>::Seek(lOff, SEEK_SET) - m_offset;

	return(pos);
}

void CTextFile::WriteString(LPCSTR lpsz/*CStdStringA str*/)
{
	CStdStringA str(lpsz);
  CStdString foo;
  CStdStringW fooW;

	if(m_encoding == ASCII)
	{
    foo = AToT(str);
    ATL::CFileT<true>::Write(foo.c_str(), sizeof(foo) * foo.length());
	}
	else if(m_encoding == ANSI)
	{
		str.Replace("\n", "\r\n");
		Write((LPCSTR)str, str.GetLength());
	}
	else if(m_encoding == UTF8 || m_encoding == LE16 || m_encoding == BE16)
	{
    fooW = AToW(str);
		Write(fooW, sizeof(fooW) * fooW.length());
	}
}

void CTextFile::WriteString(LPCWSTR lpsz/*CStdStringW str*/)
{
	CStdStringW str(lpsz);

	if(m_encoding == ASCII)
	{
		WriteString(WToT(str));
	}
	else if(m_encoding == ANSI)
	{
		str.Replace(L"\n", L"\r\n");
		CStdStringA stra = CStdStringA(CStdString(str)); // TODO: codepage
		Write((LPCSTR)stra, stra.GetLength());
	}
	else if(m_encoding == UTF8)
	{
		str.Replace(L"\n", L"\r\n");
		for(int i = 0; i < str.GetLength(); i++)
		{
			DWORD c = (WORD)str[i];

			if(0 <= c && c < 0x80) // 0xxxxxxx
			{
				Write(&c, 1);
			}
			else if(0x80 <= c && c < 0x800) // 110xxxxx 10xxxxxx
			{
				c = 0xc080|((c<<2)&0x1f00)|(c&0x003f);
				Write((BYTE*)&c+1, 1);
				Write(&c, 1);
			}
			else if(0x800 <= c && c < 0xFFFF) // 1110xxxx 10xxxxxx 10xxxxxx
			{
				c = 0xe08080|((c<<4)&0x0f0000)|((c<<2)&0x3f00)|(c&0x003f);
				Write((BYTE*)&c+2, 1);
				Write((BYTE*)&c+1, 1);
				Write(&c, 1);
			}
			else
			{
				c = '?';
				Write(&c, 1);
			}
		}
	}
	else if(m_encoding == LE16)
	{
		str.Replace(L"\n", L"\r\n");
		Write((LPCWSTR)str, str.GetLength()*2);
	}
	else if(m_encoding == BE16)
	{
		str.Replace(L"\n", L"\r\n");
		for(int i = 0; i < str.GetLength(); i++)
			str.SetAt(i, ((str[i]>>8)&0x00ff)|((str[i]<<8)&0xff00));
		Write((LPCWSTR)str, str.GetLength()*2);
	}
}

BOOL CTextFile::ReadString(CStdStringA& str)
{
	bool fEOF = true;

	str.Empty();

	/*if(m_encoding == ASCII)
	{
		CStdString s;
		fEOF = !__super::ReadString(s);
		str = TToA(s);
	}*/
	if(m_encoding == ANSI || m_encoding == ASCII)
	{
		char c;
		while(Read(&c, sizeof(c)) == sizeof(c))
		{
			fEOF = false;
			if(c == '\r') continue;
			if(c == '\n') break;
			str += c;
		}
	}
	else if(m_encoding == UTF8)
	{
		BYTE b;
		while(Read(&b, sizeof(b)) == sizeof(b))
		{
			fEOF = false;
			char c = '?';
			if(!(b&0x80)) // 0xxxxxxx
			{
				c = b&0x7f;
			}
			else if((b&0xe0) == 0xc0) // 110xxxxx 10xxxxxx
			{
				if(Read(&b, sizeof(b)) != sizeof(b)) break;
			}
			else if((b&0xf0) == 0xe0) // 1110xxxx 10xxxxxx 10xxxxxx
			{
				if(Read(&b, sizeof(b)) != sizeof(b)) break;
				if(Read(&b, sizeof(b)) != sizeof(b)) break;
			}
			if(c == '\r') continue;
			if(c == '\n') break;
			str += c;
		}
	}
	else if(m_encoding == LE16)
	{
		WORD w;
		while(Read(&w, sizeof(w)) == sizeof(w))
		{
			fEOF = false;
			char c = '?';
			if(!(w&0xff00)) c = w&0xff;
			if(c == '\r') continue;
			if(c == '\n') break;
			str += c;
		}
	}
	else if(m_encoding == BE16)
	{
		WORD w;
		while(Read(&w, sizeof(w)) == sizeof(w))
		{
			fEOF = false;
			char c = '?';
			if(!(w&0xff)) c = w>>8;
			if(c == '\r') continue;
			if(c == '\n') break;
			str += c;
		}
	}

	return(!fEOF);
}

BOOL CTextFile::ReadString(CStdStringW& str)
{
	bool fEOF = true;

	str.Empty();

	/*if(m_encoding == ASCII)
	{
		CStdString s;
		fEOF = !__super::ReadString(s);
		str = TToW(s);
	}
	else */if(m_encoding == ANSI || m_encoding == ASCII)
	{
		CStdStringA stra;
		char c;
		while(Read(&c, sizeof(c)) == sizeof(c))
		{
			fEOF = false;
			if(c == '\r') continue;
			if(c == '\n') break;
			stra += c;
		}
		str = CStdStringW(CStdString(stra)); // TODO: codepage
	}
	else if(m_encoding == UTF8)
	{
		BYTE b;
		while(Read(&b, sizeof(b)) == sizeof(b))
		{
			fEOF = false;
			WCHAR c = '?';
			if(!(b&0x80)) // 0xxxxxxx
			{
				c = b&0x7f;
			}
			else if((b&0xe0) == 0xc0) // 110xxxxx 10xxxxxx
			{
				c = (b&0x1f)<<6;
				if(Read(&b, sizeof(b)) != sizeof(b)) break;
				c |= (b&0x3f);
			}
			else if((b&0xf0) == 0xe0) // 1110xxxx 10xxxxxx 10xxxxxx
			{
				c = (b&0x0f)<<12;
				if(Read(&b, sizeof(b)) != sizeof(b)) break;
				c |= (b&0x3f)<<6;
				if(Read(&b, sizeof(b)) != sizeof(b)) break;
				c |= (b&0x3f);
			}
			if(c == '\r') continue;
			if(c == '\n') break;
			str += c;
		}
	}
	else if(m_encoding == LE16)
	{
		WCHAR wc;
		while(Read(&wc, sizeof(wc)) == sizeof(wc))
		{
			fEOF = false;
			if(wc == '\r') continue;
			if(wc == '\n') break;
			str += wc;
		}
	}
	else if(m_encoding == BE16)
	{
		WCHAR wc;
		while(Read(&wc, sizeof(wc)) == sizeof(wc))
		{
			fEOF = false;
			wc = ((wc>>8)&0x00ff)|((wc<<8)&0xff00);
			if(wc == '\r') continue;
			if(wc == '\n') break;
			str += wc;
		}
	}

	return(!fEOF);
}

//
// CWebTextFile
//

CWebTextFile::CWebTextFile(LONGLONG llMaxSize)
	: m_llMaxSize(llMaxSize)
{
}

bool CWebTextFile::Open(LPCTSTR lpszFileName)
{
	CStdString fn(lpszFileName);

	if(fn.Find(_T("http://")) != 0)
		return __super::Open(lpszFileName);

  ATL::CFileT<true> f;    

	if(!f.Open(fn)) return(false);

	TCHAR path[MAX_PATH];
	GetTempPath(MAX_PATH, path);

	fn = path + fn.Mid(fn.ReverseFind('/')+1);
	int i = fn.Find(_T("?"));
	if(i > 0) fn = fn.Left(i);
  ATL::CFileT<true> temp;
	if(!temp.Open(fn))
	{
		f.Close();
		return(false);
	}

	BYTE buff[1024];
	int len, total = 0;
	while((len = f.Read(buff, 1024)) == 1024 && (m_llMaxSize < 0 || (total+=1024) < m_llMaxSize))
		temp.Write(buff, len);
	if(len > 0) temp.Write(buff, len);

	m_tempfn = fn;

	f.Close(); // must close it because the desctructor doesn't seem to do it and we will get an exception when "is" is destroying

	return __super::Open(m_tempfn);
}

bool CWebTextFile::Save(LPCTSTR lpszFileName, enc e)
{
	// CWebTextFile is read-only...
	ASSERT(0);
	return(false);
}

void CWebTextFile::Close()
{
	__super::Close();

	if(!m_tempfn.IsEmpty())
	{
		_tremove(m_tempfn);
		m_tempfn.Empty();
	}
}

///////////////////////////////////////////////////////////////

CStdStringW AToW(CStdStringA str)
{
	CStdStringW ret;
	for(int i = 0, j = str.GetLength(); i < j; i++)
		ret += (WCHAR)(BYTE)str[i];
	return(ret);
}

CStdStringA WToA(CStdStringW str)
{
	CStdStringA ret;
	for(int i = 0, j = str.GetLength(); i < j; i++)
		ret += (CHAR)(WORD)str[i];
	return(ret);
}

CStdString AToT(CStdStringA str)
{
	CStdString ret;
	for(int i = 0, j = str.GetLength(); i < j; i++)
		ret += (TCHAR)(BYTE)str[i];
	return(ret);
}

CStdString WToT(CStdStringW str)
{
	CStdString ret;
	for(int i = 0, j = str.GetLength(); i < j; i++)
		ret += (TCHAR)(WORD)str[i];
	return(ret);
}

CStdStringA TToA(CStdString str)
{
	CStdStringA ret;
#ifdef UNICODE
	for(int i = 0, j = str.GetLength(); i < j; i++)
		ret += (CHAR)(BYTE)str[i];
#else
	ret = str;
#endif
	return(ret);
}

CStdStringW TToW(CStdString str)
{
	CStdStringW ret;
#ifdef UNICODE
	ret = str;
#else
	for(int i = 0, j = str.GetLength(); i < j; i++)
		ret += (WCHAR)(BYTE)str[i];
#endif
	return(ret);
}
