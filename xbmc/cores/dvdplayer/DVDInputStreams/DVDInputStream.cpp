
#include "../../../stdafx.h"
#include "DVDInputStream.h"


CDVDInputStream::CDVDInputStream()
{
  m_strFileName = NULL;
  m_bEOF = true;
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
  m_bEOF = true;
}

const char* CDVDInputStream::GetFileName()
{
  return m_strFileName;
}

bool CDVDInputStream::HasExtension(char* sExtension)
{
  char* ext;
  
  if (m_strFileName)
  {
    ext = strrchr(m_strFileName, '.');
    if (ext && (ext + 1) != 0)
    {
      ext += 1;
      return (stricmp(sExtension, ext) == 0);
    }
  }
  return false;
}
