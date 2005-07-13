
#include "../../../stdafx.h"
#include "DVDInputStream.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CDVDInputStream::CDVDInputStream()
{
  m_strFileName = NULL;
}

CDVDInputStream::~CDVDInputStream()
{
  if (m_strFileName) delete m_strFileName;
}

bool CDVDInputStream::Open(const char* strFile)
{
  if (m_strFileName) delete m_strFileName;
  m_strFileName = strdup(strFile);

  return true;
}

void CDVDInputStream::Close()
{
  if (m_strFileName) delete m_strFileName;
  m_strFileName = NULL;
}

const char* CDVDInputStream::GetFileName()
{
  return m_strFileName;
}
