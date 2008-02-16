// KaiVector.h: interface for the CKaiVector class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_KaiVector_H__641CCF68_6D2A_426E_9204_C0E4BEF12D00__INCLUDED_)
#define AFX_KaiVector_H__641CCF68_6D2A_426E_9204_C0E4BEF12D00__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CKaiVector
{
public:
  CKaiVector();
  virtual ~CKaiVector();

  void Load(const CStdString& strPath);
  void Save(const CStdString& strPath);

  static CKaiVector* From(CStdString& strUrl);

  void AddTitle(DWORD aTitleId, CStdString& aVector);
  bool GetTitle(DWORD aTitleId, CStdString& aVector);
  bool ContainsTitle(DWORD aTitleId);
  bool IsEmpty();

protected:

  typedef std::map<DWORD, CStdString> TITLEVECTORMAP;
  TITLEVECTORMAP m_mapTitles;
  CRITICAL_SECTION m_critical;
  bool m_bDirty;
};

#endif // !defined(AFX_KaiVector_H__641CCF68_6D2A_426E_9204_C0E4BEF12D00__INCLUDED_)
