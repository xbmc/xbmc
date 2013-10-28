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


// Acknowledgements:
// Thanks to Adam Szulc for his initial CString code.

////////////////////////////////////////////////////////
// cstring.h
//  Declaration of the cstring.h

// This class is intended to provide a simple alternative to the MFC/ATL 
// CString class that ships with Microsoft compilers. The CString class
// specified here is compatible with other compilers such as Borland 5.5
// and MinGW.

// Differences between this class and the MFC/ATL CString class
// ------------------------------------------------------------
// 1) The constructors for this class accepts only TCHARs. The various text conversion
//    functions can be used to convert from other character types to TCHARs.
//
// 2) This class is not reference counted, so these CStrings should be passed as 
//    references or const references when used as function arguments. As a result there 
//    is no need for functions like LockBuffer and UnLockBuffer.
//
// 3) The Format functions only accepts POD (Plain Old Data) arguments. It does not
//    accept arguments which are class or struct objects. In particular it does not
//    accept CString objects, unless these are cast to LPCTSTR.
//    This is demonstrates valid and invalid usage:
//      CString string1(_T("Hello World"));
//      CString string2;
//
//      // This is invalid, and produces undefined behaviour.
//      string2.Format(_T("String1 is: %s"), string1); // No! you can't do this
//
//      // This is ok
//      string2.Format(_T("String1 is: %s"), (LPCTSTR)string1); // Yes, this is correct
//
//    Note: The MFC/ATL CString class uses a non portable hack to make its CString class 
//          behave like a POD. Other compilers (such as the MinGW compiler) specifically
//          prohibit the use of non POD types for functions with variable argument lists.
//
// 4) This class provides a few additional functions:
//       b_str			Returns a BSTR string. This an an alternative for casting to BSTR.
//       c_str			Returns a const TCHAR string. This is an alternative for casting to LPCTSTR.
//       GetErrorString	Assigns CString to the error string for the specified System Error Code 
//                      (from ::GetLastErrror() for example).
//       GetString		Returns a reference to the underlying std::basic_string<TCHAR>. This 
//						reference can be used to modify the string directly.



#ifndef _WIN32XX_CSTRING_H_
#define _WIN32XX_CSTRING_H_


#include "wincore.h"


namespace Win32xx
{

	class CString 
	{
		// friend functions allow the left hand side to be something other than CString
		friend CString operator + (const CString& string1, const CString& string2);
		friend CString operator + (const CString& string, LPCTSTR pszText);	
		friend CString operator + (const CString& string, TCHAR ch);
		friend CString operator + (LPCTSTR pszText, const CString& string);
		friend CString operator + (TCHAR ch, const CString& string);		

	public:
		CString();
		~CString();
		CString(const CString& str);
		CString(LPCTSTR pszText);
		CString(TCHAR ch, int nLength = 1);
		CString(LPCTSTR pszText, int nLength);

		CString& operator = (const CString& str);
		CString& operator = (const TCHAR ch);
		CString& operator = (LPCTSTR pszText);
		BOOL     operator == (LPCTSTR pszText);
		BOOL     operator != (LPCTSTR pszText);
		BOOL	 operator < (LPCTSTR pszText);
		BOOL	 operator > (LPCTSTR pszText);
		BOOL	 operator <= (LPCTSTR pszText);
		BOOL	 operator >= (LPCTSTR pszText);
				 operator LPCTSTR() const;
				 operator BSTR() const;
		TCHAR&   operator [] (int nIndex);
		CString& operator += (const CString& str);

		// Attributes
		BSTR     b_str() const		{ return T2W(m_str.c_str()); }	// alternative for casting to BSTR
		LPCTSTR	 c_str() const		{ return m_str.c_str(); }		// alternative for casting to LPCTSTR
		tString& GetString()		{ return m_str; }				// returns a reference to the underlying std::basic_string<TCHAR>
		int      GetLength() const	{ return (int)m_str.length(); }		// returns the length in characters

		// Operations
		BSTR     AllocSysString() const;
		void	 AppendFormat(LPCTSTR pszFormat,...);
		void	 AppendFormat(UINT nFormatID, ...);
		int      Compare(LPCTSTR pszText) const;
		int      CompareNoCase(LPCTSTR pszText) const;
		int      Delete(int nIndex, int nCount = 1);
		int		 Find(TCHAR ch, int nIndex = 0 ) const;
		int      Find(LPCTSTR pszText, int nStart = 0) const;
		int		 FindOneOf(LPCTSTR pszText) const;
		void	 Format(UINT nID, ...);
		void     Format(LPCTSTR pszFormat,...);
		void     FormatV(LPCTSTR pszFormat, va_list args);
		void	 FormatMessage(LPCTSTR pszFormat,...);
		void	 FormatMessageV(LPCTSTR pszFormat, va_list args);
		TCHAR	 GetAt(int nIndex) const;
		LPTSTR	 GetBuffer(int nMinBufLength);
		void	 GetErrorString(DWORD dwError);
		void     Empty();
		int      Insert(int nIndex, TCHAR ch);
		int      Insert(int nIndex, const CString& str);
		BOOL     IsEmpty() const;
		CString  Left(int nCount) const;
		BOOL	 LoadString(UINT nID);
		void     MakeLower();
		void	 MakeReverse();
		void     MakeUpper();
		CString	 Mid(int nFirst) const;
		CString  Mid(int nFirst, int nCount) const;
		void	 ReleaseBuffer( int nNewLength = -1 );
		int      Remove(LPCTSTR pszText);
		int      Replace(TCHAR chOld, TCHAR chNew);
		int      Replace(const LPCTSTR pszOld, LPCTSTR pszNew);
		int      ReverseFind(LPCTSTR pszText, int nStart = -1) const;
		CString  Right(int nCount) const;
		void	 SetAt(int nIndex, TCHAR ch);
		BSTR	 SetSysString(BSTR* pBstr) const;
		CString	 SpanExcluding(LPCTSTR pszText) const;
		CString	 SpanIncluding(LPCTSTR pszText) const;
		CString	 Tokenize(LPCTSTR pszTokens, int& iStart) const;
		void	 Trim();
		void	 TrimLeft();
		void	 TrimLeft(TCHAR chTarget);
		void	 TrimLeft(LPCTSTR pszTargets);
		void	 TrimRight();
		void	 TrimRight(TCHAR chTarget);
		void	 TrimRight(LPCTSTR pszTargets);
		void     Truncate(int nNewLength);

#ifndef _WIN32_WCE
		int      Collate(LPCTSTR pszText) const;
		int		 CollateNoCase(LPCTSTR pszText) const;
		BOOL	 GetEnvironmentVariable(LPCTSTR pszVar);
#endif

	private:
		tString m_str;
		std::vector<TCHAR> m_buf;
	};

	inline CString::CString()
	{
	}

	inline CString::~CString()
	{
	}

	inline CString::CString(const CString& str)
	{
		m_str.assign(str);
	}

	inline CString::CString(LPCTSTR pszText)
	{
		m_str.assign(pszText);
	}

	inline CString::CString(TCHAR ch, int nLength)
	{
		m_str.assign(nLength, ch);
	}
	
	inline CString::CString(LPCTSTR pszText, int nLength)
	{
		m_str.assign(pszText, nLength);	
	}

	inline CString& CString::operator = (const CString& str)
	{
		m_str.assign(str);
		return *this;
	}

	inline CString& CString::operator = (const TCHAR ch)
	{
		m_str.assign(1, ch);
		return *this;
	}

	inline CString& CString::operator = (LPCTSTR pszText)
	{
		m_str.assign(pszText);
		return *this;
	}

	inline BOOL CString::operator == (LPCTSTR pszText)
	// Returns TRUE if the strings have the same content
	{
		assert(pszText);
		return (0 == Compare(pszText));
	}

	inline BOOL CString::operator != (LPCTSTR pszText)
	// Returns TRUE if the strings have a different content
	{
		assert(pszText);
        return Compare(pszText) != 0;
	}

	inline BOOL CString::operator < (LPCTSTR pszText)
	{
		assert(pszText);
		return Compare(pszText) < 0;
	}

	inline BOOL CString::operator > (LPCTSTR pszText)
	{
		assert(pszText);
		return Compare(pszText) > 0;
	}

	inline BOOL CString::operator <= (LPCTSTR pszText)
	{
		assert(pszText);
		return Compare(pszText) <= 0;
	}

	inline BOOL CString::operator >= (LPCTSTR pszText)
	{
		assert(pszText);
		return Compare(pszText) >= 0;
	}

	inline CString::operator LPCTSTR() const
	{
		return m_str.c_str();
	}

	inline TCHAR& CString::operator [] (int nIndex)
	{
		assert(nIndex >= 0);
		assert(nIndex < GetLength());
		return m_str[nIndex];
	}

	inline CString& CString::operator += (const CString& str)
	{
		m_str.append(str);
		return *this;
	}

	inline BSTR CString::AllocSysString() const
	// Allocates a BSTR from the CString content.
	{
		return ::SysAllocStringLen(T2W(m_str.c_str()), (UINT)m_str.size());
	}

	inline void CString::AppendFormat(LPCTSTR pszFormat,...)
	// Appends formatted data to an the CString content.
	{
		CString str;
		str.Format(pszFormat);
		m_str.append(str);
	}

	inline void CString::AppendFormat(UINT nFormatID, ...)
	// Appends formatted data to an the CString content.
	{
		CString str1;
		CString str2;
		if (str1.LoadString(nFormatID))
		{
			str2.Format(str1);
			m_str.append(str2);
		}
	}

#ifndef _WIN32_WCE
	inline int CString::Collate(LPCTSTR pszText) const
	// Performs a case sensitive comparison of the two strings using locale-specific information.
	{
		assert(pszText);
		return _tcscoll(m_str.c_str(), pszText);
	}

	inline int CString::CollateNoCase(LPCTSTR pszText) const
	// Performs a case insensitive comparison of the two strings using locale-specific information.
	{
		assert(pszText);
		return _tcsicoll(m_str.c_str(), pszText);
	}
#endif	// _WIN32_WCE

	inline int CString::Compare(LPCTSTR pszText) const
	// Performs a case sensitive comparison of the two strings.
	{
		assert(pszText);
		return m_str.compare(pszText);
	}

	inline int CString::CompareNoCase(LPCTSTR pszText) const
	// Performs a case insensitive comparison of the two strings.
	{
		assert(pszText);
		return _tcsicmp(m_str.data(), pszText);
	}

	inline int CString::Delete(int nIndex, int nCount /* = 1 */)
	// Deletes a character or characters from the string.
	{
		assert(nIndex >= 0);
		assert(nCount >= 0);

		m_str.erase(nIndex, nCount);
		return (int)m_str.size();
	}

	inline void CString::Empty()
	// Erases the contents of the string.
	{
		m_str.erase();
	}

	inline int CString::Find(TCHAR ch, int nIndex /* = 0 */) const
	// Finds a character in the string.
	{
		assert(nIndex >= 0);
		return (int)m_str.find(ch, nIndex);
	}

	inline int CString::Find(LPCTSTR pszText, int nIndex /* = 0 */) const
	// Finds a substring within the string. 
	{
		assert(pszText);
		assert(nIndex >= 0);
		return (int)m_str.find(pszText, nIndex);
	}

	inline int CString::FindOneOf(LPCTSTR pszText) const
	// Finds the first matching character from a set.
	{
		assert(pszText);
		return (int)m_str.find_first_of(pszText);
	}

	inline void CString::Format(LPCTSTR pszFormat,...)
	// Formats the string as sprintf does.
	{
		va_list args;
		va_start(args, pszFormat);
		FormatV(pszFormat, args);
		va_end(args);
	}

	inline void CString::Format(UINT nID, ...)
	// Formats the string as sprintf does.
	{
		Empty();
		CString str;
		if (str.LoadString(nID))
			Format(str);
	}

	inline void CString::FormatV(LPCTSTR pszFormat, va_list args)
	// Formats the string using a variable list of arguments.
	{
		if (pszFormat)
		{
			int nResult = -1, nLength = 256;

			// A vector is used to store the TCHAR array
			std::vector<TCHAR> vBuffer;( nLength+1, _T('\0') );

			while (-1 == nResult)
			{
				vBuffer.assign( nLength+1, _T('\0') );
				nResult = _vsntprintf(&vBuffer[0], nLength, pszFormat, args);
				nLength *= 2;
			}
			m_str.assign(&vBuffer[0]);
		}
	}

	inline void CString::FormatMessage(LPCTSTR pszFormat,...)
	// Formats a message string.
	{
		va_list args;
		va_start(args, pszFormat);
		FormatMessageV(pszFormat, args);
		va_end(args);
	}

	inline void CString::FormatMessageV(LPCTSTR pszFormat, va_list args)
	// Formats a message string using a variable argument list.
	{
		LPTSTR pszTemp = 0;
		if (pszFormat)
		{
			DWORD dwResult = ::FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ALLOCATE_BUFFER, pszFormat, 0, 0, pszTemp, 0, &args);

			if (0 == dwResult || 0 == pszTemp )
				throw std::bad_alloc();

			m_str = pszTemp;
			LocalFree(pszTemp);
		}
	}

	inline TCHAR CString::GetAt(int nIndex) const
	// Returns the character at the specified location within the string.
	{
		assert(nIndex >= 0);
		assert(nIndex < GetLength());
		return m_str[nIndex];
	}

	inline LPTSTR CString::GetBuffer(int nMinBufLength)
	// Creates a buffer of nMinBufLength charaters (+1 extra for NULL termination) and returns 
	// a pointer to this buffer. This buffer can be used by any function which accepts a LPTSTR.
	// Care must be taken not to exceed the length of the buffer. Use ReleaseBuffer to safely 
	// copy this buffer back to the CString object.
	//
	// Note: The buffer uses a vector. Vectors are required to be contiguous in memory under
	//       the current standard, whereas std::strings do not have this requirement.
	{
		assert (nMinBufLength >= 0);
		
		m_buf.assign(nMinBufLength + 1, _T('\0'));
		tString::iterator it_end;

		if (m_str.length() >= (size_t)nMinBufLength)
		{
			it_end = m_str.begin();
			std::advance(it_end, nMinBufLength);
		}
		else
			it_end = m_str.end();
		
		std::copy(m_str.begin(), it_end, m_buf.begin());

		return &m_buf[0];
	}

#ifndef _WIN32_WCE
	inline BOOL CString::GetEnvironmentVariable(LPCTSTR pszVar)
	// Sets the string to the value of the specified environment variable.
	{
		assert(pszVar);
		Empty();

		int nLength = ::GetEnvironmentVariable(pszVar, NULL, 0);
		if (nLength > 0)
		{
			std::vector<TCHAR> vBuffer( nLength+1, _T('\0') );
			::GetEnvironmentVariable(pszVar, &vBuffer[0], nLength);
			m_str = &vBuffer[0];
		}

		return (BOOL)nLength;
	}
#endif // _WIN32_WCE

	inline void CString::GetErrorString(DWORD dwError)
	// Returns the error string for the specified System Error Code (e.g from GetLastErrror).
	{
		m_str.erase();
		
		if (dwError != 0)
		{
			TCHAR* pTemp = 0;
			DWORD dwFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
			::FormatMessage(dwFlags, NULL, dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&pTemp, 1, NULL);
			m_str.assign(pTemp);
			::LocalFree(pTemp);
		}
	}
	
	inline int CString::Insert(int nIndex, TCHAR ch)
	// Inserts a single character or a substring at the given index within the string.
	{
		assert(nIndex >= 0);
		assert(ch);

		m_str.insert(nIndex, &ch, 1);
		return (int)m_str.size();
	}

	inline int CString::Insert(int nIndex, const CString& str)
	// Inserts a single character or a substring at the given index within the string.
	{
		assert(nIndex >= 0);

		m_str.insert(nIndex, str);
		return (int)m_str.size();
	}

	inline BOOL CString::IsEmpty() const
	// Returns TRUE if the string is empty
	{
		return m_str.empty();
	}

	inline CString CString::Left(int nCount) const
	// Extracts the left part of a string.
	{
		assert(nCount >= 0);

		CString str;
		str.m_str.assign(c_str(), 0, nCount);
		return str;
	}

	inline BOOL CString::LoadString(UINT nID)
	// Loads the string from a Windows resource.
	{
		assert (GetApp());

		int nSize = 64;
		TCHAR* pTCharArray = 0;
		std::vector<TCHAR> vString;
		int nTChars = nSize;

		Empty();

		// Increase the size of our array in a loop until we load the entire string
		// The ANSI and _UNICODE versions of LoadString behave differently. This technique works for both.
		while ( nSize-1 <= nTChars )
		{
			nSize = nSize * 4;
			vString.assign(nSize+1, _T('\0'));
			pTCharArray = &vString[0];
			nTChars = ::LoadString (GetApp()->GetResourceHandle(), nID, pTCharArray, nSize);
		}

		if (nTChars > 0)
			m_str.assign(pTCharArray);

		return (nTChars != 0);
	}

	inline void CString::MakeLower()
	// Converts all the characters in this string to lowercase characters.
	{
		std::transform(m_str.begin(), m_str.end(), m_str.begin(), &::tolower);
	}

	inline void CString::MakeReverse()
	// Reverses the string.
	{
		std::reverse(m_str.begin(), m_str.end());
	}

	inline void CString::MakeUpper()
	// Converts all the characters in this string to uppercase characters.
	{
		std::transform(m_str.begin(), m_str.end(), m_str.begin(), &::toupper);
	}

	inline CString CString::Mid(int nFirst) const
	// Extracts the middle part of a string.
	{
		return Mid(nFirst, GetLength());
	}

	inline CString CString::Mid(int nFirst, int nCount) const
	// Extracts the middle part of a string.
	{
		assert(nFirst >= 0);
		assert(nCount >= 0);

		CString str;
		str.m_str.assign(c_str(), nFirst, nFirst + nCount);
		return str;
	}

	inline int CString::ReverseFind(LPCTSTR pszText, int nIndex /* = -1 */) const
	// Search for a substring within the string, starting from the end.
	{
		assert(pszText);
		return (int)m_str.rfind(pszText, nIndex);
	}

	inline void CString::SetAt(int nIndex, TCHAR ch)
	// Sets the character at the specificed position to the specified value.
	{
		assert(nIndex >= 0);
		assert(nIndex < GetLength());
		m_str[nIndex] = ch;
	}

	inline void CString::ReleaseBuffer( int nNewLength /*= -1*/ )
	// This copies the contents of the buffer (acquired by GetBuffer) to this CString,
	// and releases the contents of the buffer. The default length of -1 copies from the
	// buffer until a null terminator is reached. If the buffer doesn't contain a null
	// terminator, you must specify the buffer's length.
	{
		assert (nNewLength > 0 || -1 == nNewLength);
		assert (nNewLength < (int)m_buf.size());

		if (-1 == nNewLength)
			nNewLength = lstrlen(&m_buf[0]);
		m_str.assign(nNewLength+1, _T('\0'));

		std::vector<TCHAR>::iterator it_end = m_buf.begin();
		std::advance(it_end, nNewLength);
		
		std::copy(m_buf.begin(), it_end, m_str.begin());
		m_buf.clear();
	}

	inline int CString::Remove(LPCTSTR pszText)
	// Removes each occurrence of the specified substring from the string.
	{
		assert(pszText);

		int nCount = 0;
		size_t pos = 0;
		while ((pos = m_str.find(pszText, pos)) != std::string::npos)
		{
			m_str.erase(pos, lstrlen(pszText));
			++nCount;
		}
		return nCount;
	}

	inline int CString::Replace(TCHAR chOld, TCHAR chNew)
	// Replaces each occurance of the old character with the new character.
	{
		int nCount = 0;
		tString::iterator it = m_str.begin();
		while (it != m_str.end())
		{
			if (*it == chOld)
			{
				*it = chNew;
				++nCount;
			}
			++it;
		}
		return nCount;
	}

	inline int CString::Replace(LPCTSTR pszOld, LPCTSTR pszNew)
	// Replaces each occurance of the old substring with the new substring.
	{
		assert(pszOld);
		assert(pszNew);

		int nCount = 0;
		size_t pos = 0;
		while ((pos = m_str.find(pszOld, pos)) != std::string::npos)
		{
			m_str.replace(pos, lstrlen(pszOld), pszNew);
			pos += lstrlen(pszNew);
			++nCount;
		}
		return nCount;
	}

	inline CString CString::Right(int nCount) const
	// Extracts the right part of a string.
	{
		assert(nCount >= 0);

		CString str;
		str.m_str.assign(c_str(), m_str.size() - nCount, nCount);
		return str;
	}

	inline BSTR CString::SetSysString(BSTR* pBstr) const
	// Sets an existing BSTR object to the string.
	{
		assert(pBstr);

		if ( !::SysReAllocStringLen(pBstr, T2W(m_str.c_str()), (UINT)m_str.length()) )
			throw std::bad_alloc();

		return *pBstr;
	}

	inline CString CString::SpanExcluding(LPCTSTR pszText) const
	// Extracts characters from the string, starting with the first character, 
	// that are not in the set of characters identified by pszCharSet.
	{
		assert (pszText);

		CString str;
		size_t pos = 0;

		while ((pos = m_str.find_first_not_of(pszText, pos)) != std::string::npos)
		{
			str.m_str.append(1, m_str[pos++]);
		}

		return str;
	}

	inline CString CString::SpanIncluding(LPCTSTR pszText) const
	// Extracts a substring that contains only the characters in a set.
	{
		assert (pszText);

		CString str;
		size_t pos = 0;

		while ((pos = m_str.find_first_of(pszText, pos)) != std::string::npos)
		{
			str.m_str.append(1, m_str[pos++]);
		}

		return str;
	}

	inline CString CString::Tokenize(LPCTSTR pszTokens, int& iStart) const
	// Extracts specified tokens in a target string.
	{
		assert(pszTokens);
		assert(iStart >= 0);

		CString str;
		size_t pos1 = m_str.find_first_not_of(pszTokens, iStart);
		size_t pos2 = m_str.find_first_of(pszTokens, pos1);

		iStart = (int)pos2 + 1;
		if (pos2 == m_str.npos)
			iStart = -1;

		if (pos1 != m_str.npos)
			str.m_str = m_str.substr(pos1, pos2-pos1);

		return str;
	}

	inline void CString::Trim()
	// Trims all leading and trailing whitespace characters from the string.
	{
		TrimLeft();
		TrimRight();
	}

	inline void CString::TrimLeft()
	// Trims leading whitespace characters from the string.
	{
		// This method is supported by the Borland 5.5 compiler
		tString::iterator iter;
		for (iter = m_str.begin(); iter < m_str.end(); ++iter)
		{
			if (!isspace(*iter))
				break;
		}

		m_str.erase(m_str.begin(), iter);
	}

	inline void CString::TrimLeft(TCHAR chTarget)
	// Trims the specified character from the beginning of the string.
	{
		m_str.erase(0, m_str.find_first_not_of(chTarget));
	}

	inline void CString::TrimLeft(LPCTSTR pszTargets)
	// Trims the specified set of characters from the beginning of the string. 
	{
		assert(pszTargets);
		m_str.erase(0, m_str.find_first_not_of(pszTargets));
	}

	inline void CString::TrimRight()
	// Trims trailing whitespace characters from the string.
	{
		// This method is supported by the Borland 5.5 compiler
		tString::reverse_iterator riter;
		for (riter = m_str.rbegin(); riter < m_str.rend(); ++riter)
		{
			if (!isspace(*riter))
				break;
		}

		m_str.erase(riter.base(), m_str.end());
	}

	inline void CString::TrimRight(TCHAR chTarget)
	// Trims the specified character from the end of the string.
	{
		size_t pos = m_str.find_last_not_of(chTarget);
		if (pos != std::string::npos)
			m_str.erase(++pos);
	}

	inline void CString::TrimRight(LPCTSTR pszTargets)
	// Trims the specified set of characters from the end of the string.
	{
		assert(pszTargets);

		size_t pos = m_str.find_last_not_of(pszTargets);
		if (pos != std::string::npos)
			m_str.erase(++pos);
	}

	inline void CString::Truncate(int nNewLength)
	// Reduces the length of the string to the specified amount.
	{
		if (nNewLength < GetLength())
		{
			assert(nNewLength >= 0);
			m_str.erase(nNewLength);
		}
	}

	
	///////////////////////////////////
	// Global Functions
	//

	// friend functions of CString
	inline CString operator + (const CString& string1, const CString& string2)
	{
		CString str(string1);
		str.m_str.append(string2.m_str);
		return str;
	}

	inline CString operator + (const CString& string, LPCTSTR pszText)
	{
		CString str(string);
		str.m_str.append(pszText);
		return str;
	}
	
	inline CString operator + (const CString& string, TCHAR ch)
	{
		CString str(string);
		str.m_str.append(1, ch);
		return str;
	}
	
	inline CString operator + (LPCTSTR pszText, const CString& string)
	{
		CString str(pszText);
		str.m_str.append(string);
		return str;
	}
	
	inline CString operator + (TCHAR ch, const CString& string)
	{
		CString str(ch);
		str.m_str.append(string);
		return str;
	}	

	// Global LoadString
	inline CString LoadString(UINT nID)
	{
		CString str;
		str.LoadString(nID);
		return str;
	}


}	// namespace Win32xx

#endif//_WIN32XX_CSTRING_H_
