// NfoFile.h: interface for the CNfoFile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_NfoFile_H__641CCF68_6D2A_426E_9204_C0E4BEF12D00__INCLUDED_)
#define AFX_NfoFile_H__641CCF68_6D2A_426E_9204_C0E4BEF12D00__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "tinyXML/tinyxml.h"

class CVideoInfoTag;

class CNfoFile
{
public:
  CNfoFile(const CStdString&);
  virtual ~CNfoFile();

  HRESULT Create(const CStdString&);
  template<class T>
    bool GetDetails(T& details,const char* document=NULL)
  {
    TiXmlDocument doc;
    if (document)
      doc.Parse(document);
    else
      doc.Parse(m_doc);
    if (details.Load(doc.RootElement()))
      return true;
    CLog::Log(LOGDEBUG, "Not a proper xml nfo file (%s, col %i, row %i)", doc.ErrorDesc(), doc.ErrorCol(), doc.ErrorRow());
    return false;
  }

  CStdString m_strScraper;
  CStdString m_strImDbUrl;
  CStdString m_strImDbNr;
private:
  HRESULT Load(const CStdString&);
  HRESULT Scrape(const CStdString&, const CStdString& strURL="");
  void Close();
private:
  char* m_doc;
  int m_size;
  CStdString m_strContent;
};

#endif // !defined(AFX_NfoFile_H__641CCF68_6D2A_426E_9204_C0E4BEF12D00__INCLUDED_)
