// NfoFile.cpp: implementation of the CNfoFile class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NfoFile.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CNfoFile::CNfoFile()
{
}

CNfoFile::~CNfoFile()
{
}

HRESULT CNfoFile::Create(LPSTR szPath)
{
  if (FAILED(Load(szPath)))
    return E_FAIL;

  CHAR* szUrl = strstr(m_doc, "http://us.imdb");
  if (!szUrl)
  {
    szUrl = strstr(m_doc, "http://www.imdb.com");
  }
  if (!szUrl)
  {
    szUrl = strstr(m_doc, "http://akas.imdb.com");
  }
  if (szUrl)
  {
    char szScrape[1024];

    for (int i = 0; i < 1024; i++)
    {
      char c = szUrl[i];
      if ( (c == 0) || (c == 32) || (c == 10) || (c == 13) )
      {
        szScrape[i] = 0;
        break;
      }

      szScrape[i] = c;
    }

    m_strImDbUrl = szScrape;
  }

  Close();

  return (m_strImDbUrl.size() > 0) ? S_OK : E_FAIL;
}

HRESULT CNfoFile::Load(char* szFile)
{
  FILE* hFile;

  hFile = fopen(szFile, "rb");
  if (hFile == NULL)
  {
    OutputDebugString("No such file: ");
    OutputDebugString(szFile);
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

