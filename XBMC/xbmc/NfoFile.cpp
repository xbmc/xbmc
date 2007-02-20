// NfoFile.cpp: implementation of the CNfoFile class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NfoFile.h"
#include "utils/ScraperParser.h"
#include "FileSystem/Directory.h"
#include "util.h"


using namespace DIRECTORY;
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CNfoFile::CNfoFile(const CStdString& strContent)
{
  CNfoFile::strContent = strContent;
}

CNfoFile::~CNfoFile()
{
}

HRESULT CNfoFile::Create(const CStdString& strPath)
{
  if (FAILED(Load(strPath.c_str())))
    return E_FAIL;

  CDirectory dir;
  CFileItemList items;
  dir.GetDirectory("q:\\system\\scrapers\\video",items,".xml",false);
  for (int i=0;i<items.Size();++i)
  {
    if (!items[i]->m_bIsFolder)
    {
      
      if(!FAILED(Scrape(CUtil::GetFileName(items[i]->m_strPath).c_str()))) break;
    }
  }
  Close();

  return (m_strImDbUrl.size() > 0) ? S_OK : E_FAIL;
}
HRESULT CNfoFile::Scrape(const CStdString& strScraperPath)
{
  CScraperParser m_parser;
  if (!m_parser.Load("Q:\\system\\scrapers\\video\\"+strScraperPath))
    return E_FAIL;

  if(m_parser.GetContent() !=  strContent.c_str() )
    return E_FAIL;
  m_parser.m_param[0] = m_doc;
  m_strImDbUrl = m_parser.Parse("NfoUrl");
  if(m_strImDbUrl.size() > 0)
  {
    m_strScraper = strScraperPath;
    return S_OK;
  }
  else
  {
    return E_FAIL;
  }
}
HRESULT CNfoFile::Load(const CStdString& strFile)
{
  FILE* hFile;

  hFile = fopen(strFile.c_str(), "rb");
  if (hFile == NULL)
  {
    OutputDebugString("No such file: ");
    OutputDebugString(strFile.c_str());
    OutputDebugString("\n");
    return E_FAIL;
  }

  fseek(hFile, 0, SEEK_END);
  m_size = ftell(hFile);

  fseek(hFile, 0, SEEK_SET);

  m_doc = (char*) malloc(m_size + 1);
  if (!m_doc)
  {
    m_size = 0;
    fclose(hFile);
    return E_FAIL;
  }

  if (fread(m_doc, m_size, 1, hFile) <= 0)
  {
    delete m_doc;
    m_doc = 0;
    m_size = 0;
    fclose(hFile);
    return E_FAIL;
  }

  m_doc[m_size] = 0;
  fclose(hFile);
  return S_OK;
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

