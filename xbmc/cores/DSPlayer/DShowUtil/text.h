#pragma once
#include <list>
#include "boost/foreach.hpp"

// extern CString ExplodeMin(CString str, CAtlList<CString>& sl, TCHAR sep, int limit = 0);
// extern CString Explode(CString str, CAtlList<CString>& sl, TCHAR sep, int limit = 0);
// extern CString Implode(CAtlList<CString>& sl, TCHAR sep);

//template<class T, typename SEP>
using namespace std;
CStdString Explode(CStdString str, list<CStdString>& sl, TCHAR sep, int limit = 0)
{
  while (!sl.empty())
    sl.pop_back();

	for(int i = 0, j = 0; ; i = j+1)
	{
		j = str.Find(sep, i);

		if(j < 0 || sl.size() == limit-1)
		{
			sl.push_back(str.Mid(i).Trim());
			break;
		}
		else
		{
			sl.push_back(str.Mid(i, j-i).Trim());
		}		
	}
  std::list<CStdString>::iterator it = sl.begin();
  CStdString strit;
  strit = it->c_str();
  return strit;//GetHead();
}

//template<class T, typename SEP>
/*CStdString ExplodeMin(CStdString str, list<CStdString>& sl, TCHAR sep, int limit = 0)
{
	Explode(str, sl, sep, limit);
  BOOST_FOREACH(CStdString ss,sl)
  {
    
  }
	POSITION pos = sl.GetHeadPosition();
	while(pos) 
	{
		POSITION tmp = pos;
		if(sl.GetNext(pos).IsEmpty())
			sl.RemoveAt(tmp);
	}
	if(sl.IsEmpty()) sl.AddTail(T()); // eh

	return sl.GetHead();
}*/

//template<class T, typename SEP>
CStdString Implode(list<CStdString>& sl, TCHAR sep)
{
	CStdString ret;
  BOOST_FOREACH(CStdString ss,sl)
  {
    ret += ss;
    ret += sep;
  }
  
	/*POSITION pos = sl.GetHeadPosition();
	while(pos)
	{
		ret += sl.GetNext(pos);
		if(pos) ret += sep;
	}*/
	return(ret);
}

//mfc required for CMapStringToString
//extern CStdString ExtractTag(CStdString tag, CMapStringToString& attribs, bool& fClosing);
extern CStdStringA ConvertMBCS(CStdStringA str, DWORD SrcCharSet, DWORD DstCharSet);
extern CStdStringA UrlEncode(CStdStringA str, bool fRaw = false);
extern CStdStringA UrlDecode(CStdStringA str, bool fRaw = false);
extern DWORD CharSetToCodePage(DWORD dwCharSet);
extern std::list<CStdString>& MakeLower(std::list<CStdString>& sl);
extern std::list<CStdString>& MakeUpper(std::list<CStdString>& sl);
extern std::list<CStdString>& RemoveStrings(std::list<CStdString>& sl, int minlen, int maxlen);

