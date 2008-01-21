// NfoFile.cpp: implementation of the CNfoFile class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NfoFile.h"
#include "utils/ScraperParser.h"
#include "utils/IMDB.h"
#include "FileSystem/File.h"
#include "FileSystem/Directory.h"
#include "Util.h"


using namespace DIRECTORY;
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CNfoFile::CNfoFile(const CStdString& strContent)
{
  m_strContent = strContent;
  m_doc = NULL;
}

CNfoFile::~CNfoFile()
{
  Close();
}

bool CNfoFile::GetDetails(CVideoInfoTag &details, const char* document)
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

HRESULT CNfoFile::Create(const CStdString& strPath)
{
  if (FAILED(Load(strPath)))
    return E_FAIL;

  // first check if it's an XML file with the info we need
  CVideoInfoTag details;
  if (GetDetails(details))
  {
    m_strScraper = "NFO";
    return S_OK;
  }

  CDirectory dir;
  CFileItemList items;
  dir.GetDirectory("q:\\system\\scrapers\\video",items,".xml",false);
  for (int i=0;i<items.Size();++i)
  {
    if (!items[i]->m_bIsFolder && !FAILED(Scrape(items[i]->m_strPath)))
      break;
  }

  return (m_strImDbUrl.size() > 0) ? S_OK : E_FAIL;
}

HRESULT CNfoFile::Scrape(const CStdString& strScraperPath)
{
  CScraperParser m_parser;
  if (!m_parser.Load(strScraperPath))
    return E_FAIL;
  if (m_parser.GetContent() !=  m_strContent)
    return E_FAIL;

  m_parser.m_param[0] = m_doc;
  m_strImDbUrl = m_parser.Parse("NfoScrape");
  TiXmlDocument doc;
  doc.Parse(m_strImDbUrl.c_str());
  if (doc.RootElement())
  {
    CVideoInfoTag details;
    if (GetDetails(details,m_strImDbUrl.c_str()))
    {
      m_strScraper = "NFO";
      Close();
      m_size = m_strImDbUrl.size();
      m_doc = new char[m_size+1];
      strcpy(m_doc,m_strImDbUrl.c_str());
      return S_OK;
    }
  }
  m_parser.m_param[0] = m_doc;
  m_strImDbUrl = m_parser.Parse("NfoUrl");
  doc.Parse(m_strImDbUrl.c_str());
  TiXmlElement* pId = doc.FirstChildElement("id");
  if (pId && pId->FirstChild())
    m_strImDbNr = pId->FirstChild()->Value();
  
  if (m_strImDbUrl.size() > 0)
  {
    m_strScraper = CUtil::GetFileName(strScraperPath);
    return S_OK;
  }
  else
    return E_FAIL;
}

HRESULT CNfoFile::Load(const CStdString& strFile)
{
  Close();
  XFILE::CFile file;
  if (file.Open(strFile, true))
  {
    m_size = (int)file.GetLength();
    m_doc = new char[m_size+1];
    if (!m_doc)
    {
      file.Close();
      return E_FAIL;
    }
    file.Read(m_doc, m_size);
    m_doc[m_size] = 0;
    file.Close();
    return S_OK;
  }
  return E_FAIL;
}

void CNfoFile::Close()
{
  if (m_doc != NULL)
  {
    delete m_doc;
    m_doc = 0;
  }

  m_size = 0;
}
