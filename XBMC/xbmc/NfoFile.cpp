// NfoFile.cpp: implementation of the CNfoFile class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NfoFile.h"
#include "utils/ScraperParser.h"
#include "FileSystem/File.h"
#include "FileSystem/Directory.h"
#include "util.h"


using namespace DIRECTORY;
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CNfoFile::CNfoFile(const CStdString& strContent)
{
  CNfoFile::strContent = strContent;
  m_doc = NULL;
}

CNfoFile::~CNfoFile()
{
}

HRESULT CNfoFile::Create(const CStdString& strPath)
{
  if (FAILED(Load(strPath)))
    return E_FAIL;

  CDirectory dir;
  CFileItemList items;
  dir.GetDirectory("q:\\system\\scrapers\\video",items,".xml",false);
  for (int i=0;i<items.Size();++i)
  {
    if (!items[i]->m_bIsFolder)
    {
      
      if(!FAILED(Scrape(items[i]->m_strPath))) break;
    }
  }
  Close();

  return (m_strImDbUrl.size() > 0) ? S_OK : E_FAIL;
}

HRESULT CNfoFile::Scrape(const CStdString& strScraperPath)
{
  CScraperParser m_parser;
  if (!m_parser.Load(strScraperPath))
    return E_FAIL;

  if(m_parser.GetContent() !=  strContent )
    return E_FAIL;
  m_parser.m_param[0] = m_doc;
  m_strImDbUrl = m_parser.Parse("NfoUrl");
  if(m_strImDbUrl.size() > 0)
  {
    m_strScraper = CUtil::GetFileName(strScraperPath);
    return S_OK;
  }
  else
  {
    return E_FAIL;
  }
}
HRESULT CNfoFile::Load(const CStdString& strFile)
{
  Close();
  XFILE::CFile file;
  if (file.Open(strFile, true))
  {
    m_size = (int)file.GetLength();
    m_doc = (char*) malloc(m_size + 1);
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

