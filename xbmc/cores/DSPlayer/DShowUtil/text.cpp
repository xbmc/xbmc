//#include "stdafx.h"
#include "text.h"

/*
CString Explode(CString str, CAtlList<CString>& sl, TCHAR sep, int limit)
{
	sl.RemoveAll();

	if(limit == 1) {sl.AddTail(str); return _T("");}

	if(!str.IsEmpty() && str[str.GetLength()-1] != sep)
		str += sep;

	for(int i = 0, j = 0; (j = str.Find(sep, i)) >= 0; i = j+1)
	{
		CString tmp = str.Mid(i, j-i);
		tmp.TrimLeft(sep); tmp.TrimRight(sep);
		tmp.TrimLeft(); tmp.TrimRight();
		sl.AddTail(tmp);
		if(limit > 0 && sl.GetCount() == limit-1)
		{
			if(j+1 < str.GetLength()) 
			{
				CString tmp = str.Mid(j+1);
				tmp.TrimLeft(sep); tmp.TrimRight(sep);
				tmp.TrimLeft(); tmp.TrimRight();
				sl.AddTail(tmp);
			}
			break;
		}
	}

	if(sl.IsEmpty())
	{
		str.TrimLeft(sep); str.TrimRight(sep);
		str.TrimLeft(); str.TrimRight();
		sl.AddTail(str);
	}

	return sl.GetHead();
}

CString ExplodeMin(CString str, CAtlList<CString>& sl, TCHAR sep, int limit)
{
	Explode(str, sl, sep, limit);
	POSITION pos = sl.GetHeadPosition();
	while(pos) 
	{
		POSITION tmp = pos;
		if(sl.GetNext(pos).IsEmpty())
			sl.RemoveAt(tmp);
	}
	if(sl.IsEmpty()) sl.AddTail(CString()); // eh

	return sl.GetHead();
}

CString Implode(CAtlList<CString>& sl, TCHAR sep)
{
	CString ret;
	POSITION pos = sl.GetHeadPosition();
	while(pos)
	{
		ret += sl.GetNext(pos);
		if(pos) ret += sep;
	}
	return(ret);
}
*/

DWORD CharSetToCodePage(DWORD dwCharSet)
{
	if(dwCharSet == CP_UTF8) return CP_UTF8;
	if(dwCharSet == CP_UTF7) return CP_UTF7;
	CHARSETINFO cs={0};
	::TranslateCharsetInfo((DWORD *)dwCharSet, &cs, TCI_SRCCHARSET);
	return cs.ciACP;
}

CStringA ConvertMBCS(CStringA str, DWORD SrcCharSet, DWORD DstCharSet)
{
	WCHAR* utf16 = DNew WCHAR[str.GetLength()+1];
	memset(utf16, 0, (str.GetLength()+1)*sizeof(WCHAR));

	CHAR* mbcs = DNew CHAR[str.GetLength()*6+1];
	memset(mbcs, 0, str.GetLength()*6+1);

	int len = MultiByteToWideChar(
		CharSetToCodePage(SrcCharSet), 0, 
		str.GetBuffer(str.GetLength()), str.GetLength(), 
		utf16, (str.GetLength()+1)*sizeof(WCHAR));

	len = WideCharToMultiByte(
		CharSetToCodePage(DstCharSet), 0, 
		utf16, len, 
		mbcs, str.GetLength()*6,
		NULL, NULL);

	str = mbcs;

	delete [] utf16;
	delete [] mbcs;

	return str;
}

CStringA UrlEncode(CStringA str, bool fRaw)
{
	CStringA urlstr;

	for(int i = 0; i < str.GetLength(); i++)
	{
		CHAR c = str[i];
		if(fRaw && c == '+') urlstr += "%2B";
		else if(c > 0x20 && c < 0x7f && c != '&') urlstr += c;
		else if(c == 0x20) urlstr += fRaw ? ' ' : '+';
		else {CStringA tmp; tmp.Format("%%%02x", (BYTE)c); urlstr += tmp;}
	}

	return urlstr;
}

CStringA UrlDecode(CStringA str, bool fRaw)
{
	str.Replace("&amp;", "&");

	CHAR* s = str.GetBuffer(str.GetLength());
	CHAR* e = s + str.GetLength();
	CHAR* s1 = s;
	CHAR* s2 = s;
	while(s1 < e)
	{
		CHAR s11 = (s1 < e-1) ? (__isascii(s1[1]) && isupper(s1[1]) ? tolower(s1[1]) : s1[1]) : 0;
		CHAR s12 = (s1 < e-2) ? (__isascii(s1[2]) && isupper(s1[2]) ? tolower(s1[2]) : s1[2]) : 0;

		if(*s1 == '%' && s1 < e-2
		&& (s1[1] >= '0' && s1[1] <= '9' || s11 >= 'a' && s11 <= 'f')
		&& (s1[2] >= '0' && s1[2] <= '9' || s12 >= 'a' && s12 <= 'f'))
		{
			s1[1] = s11;
			s1[2] = s12;
			*s2 = 0;
			if(s1[1] >= '0' && s1[1] <= '9') *s2 |= s1[1]-'0';
			else if(s1[1] >= 'a' && s1[1] <= 'f') *s2 |= s1[1]-'a'+10;
			*s2 <<= 4;
			if(s1[2] >= '0' && s1[2] <= '9') *s2 |= s1[2]-'0';
			else if(s1[2] >= 'a' && s1[2] <= 'f') *s2 |= s1[2]-'a'+10;
			s1 += 2;
		}
		else 
		{
			*s2 = *s1 == '+' && !fRaw ? ' ' : *s1;
		}

		s1++;
		s2++;
	}

	str.ReleaseBuffer(s2 - s);

	return str;
}

CString ExtractTag(CString tag, CMapStringToString& attribs, bool& fClosing)
{
	tag.Trim();
	attribs.RemoveAll();

	fClosing = !tag.IsEmpty() ? tag[0] == '/' : false;
	tag.TrimLeft('/');

	int i = tag.Find(' ');
	if(i < 0) i = tag.GetLength();
	CString type = tag.Left(i).MakeLower();
	tag = tag.Mid(i).Trim();

	while((i = tag.Find('=')) > 0)
	{
		CString attrib = tag.Left(i).Trim().MakeLower();
		tag = tag.Mid(i+1);
		for(i = 0; i < tag.GetLength() && _istspace(tag[i]); i++);
		tag = i < tag.GetLength() ? tag.Mid(i) : _T("");
		if(!tag.IsEmpty() && tag[0] == '\"') {tag = tag.Mid(1); i = tag.Find('\"');}
		else i = tag.Find(' ');
		if(i < 0) i = tag.GetLength();
		CString param = tag.Left(i).Trim();
		if(!param.IsEmpty())
			attribs[attrib] = param;
		tag = i+1 < tag.GetLength() ? tag.Mid(i+1) : _T("");
	}

	return(type);
}

CAtlList<CString>& MakeLower(CAtlList<CString>& sl)
{
	POSITION pos = sl.GetHeadPosition();
	while(pos) sl.GetNext(pos).MakeLower();
	return sl;
}

CAtlList<CString>& MakeUpper(CAtlList<CString>& sl)
{
	POSITION pos = sl.GetHeadPosition();
	while(pos) sl.GetNext(pos).MakeUpper();
	return sl;
}

CAtlList<CString>& RemoveStrings(CAtlList<CString>& sl, int minlen, int maxlen)
{
	POSITION pos = sl.GetHeadPosition();
	while(pos)
	{
		POSITION tmp = pos;
		CString& str = sl.GetNext(pos);
		int len = str.GetLength();
		if(len < minlen || len > maxlen) sl.RemoveAt(tmp);
	}
	return sl;
}
