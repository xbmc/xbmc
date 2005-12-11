// NfoFile.cpp: implementation of the CNfoFile class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "NfoFile.h"
#include "utils/RegExp.h"

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

  CRegExp reg;
  reg.RegComp("imdb.com/Title\\?[0-9]*");
  if (reg.RegFind(m_doc) > -1)
  {
    char *src = reg.GetReplaceString("\\0");
    m_strImDbUrl = "http://www.";
    m_strImDbUrl += src;
    free(src);
  }
  reg.RegComp("imdb.com/title/tt[0-9]*");
  if (reg.RegFind(m_doc) > -1)
  {
    char *src = reg.GetReplaceString("\\0");
    m_strImDbUrl = "http://www.";
    m_strImDbUrl += src;
    free(src);
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

