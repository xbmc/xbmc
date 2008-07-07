/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
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
    CStdString strDoc;
    if (document)
      strDoc = document;
    else
      strDoc = m_doc;
    // try to load using string charset
    if (strDoc.Find("encoding=") == -1)
      g_charsetConverter.stringCharsetToUtf8(strDoc.Mid(0),strDoc);

    doc.Parse(strDoc.c_str());
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
