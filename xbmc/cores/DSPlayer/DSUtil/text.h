#pragma once

//extern CStdString ExplodeMin(CStdString str, std::list<CStdString>& sl, TCHAR sep, int limit = 0);
//extern CStdString Explode(CStdString str, std::list<CStdString>& sl, TCHAR sep, int limit = 0);
//extern CStdString Implode(std::list<CStdString>& sl, TCHAR sep);

template<class T, typename SEP>
T Explode(T str, std::list<T>& sl, SEP sep, int limit = 0)
{
  sl.clear();

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

  return sl.front();
}

template<class T, typename SEP>
T ExplodeMin(T str, std::list<T>& sl, SEP sep, int limit = 0)
{
  Explode(str, sl, sep, limit);
  POSITION pos = sl.GetHeadPosition();
  while(pos) 
  {
    POSITION tmp = pos;
    if(sl.GetNext(pos).IsEmpty())
      sl.RemoveAt(tmp);
  }
  if(sl.IsEmpty()) sl.push_back(T()); // eh

  return sl.GetHead();
}

template<class T, typename SEP>
T Implode(std::list<T>& sl, SEP sep)
{
  T ret;
  std::list<T>::iterator it = sl.begin();
  for(; it != sl.end(); ++it)
  {
    ret += *it;
    if (it != sl.end()) ret += sep;
  }
  return(ret);
}

extern CStdString ExtractTag(CStdString tag, std::map<CStdString, CStdString>& attribs, bool& fClosing);
extern CStdStringA ConvertMBCS(CStdStringA str, DWORD SrcCharSet, DWORD DstCharSet);
extern CStdStringA UrlEncode(CStdStringA str, bool fRaw = false);
extern CStdStringA UrlDecode(CStdStringA str, bool fRaw = false);
extern DWORD CharSetToCodePage(DWORD dwCharSet);
extern std::list<CStdString>& MakeLower(std::list<CStdString>& sl);
extern std::list<CStdString>& MakeUpper(std::list<CStdString>& sl);
extern std::list<CStdString>& RemoveStrings(std::list<CStdString>& sl, int minlen, int maxlen);

