
#include "stdafx.h"
#include "DVDInputStream.h"


CDVDInputStream::CDVDInputStream(DVDStreamType streamType)
{
  m_streamType = streamType;
  m_strFileName = NULL;
}

CDVDInputStream::~CDVDInputStream()
{
  if (m_strFileName) 
    free(m_strFileName);
}

bool CDVDInputStream::Open(const char* strFile, const std::string &content)
{
  if (m_strFileName)
    free(m_strFileName);

  m_strFileName = strdup(strFile);
  m_content = content;
  return true;
}

void CDVDInputStream::Close()
{
  if (m_strFileName) 
     free(m_strFileName);
  m_strFileName = NULL;
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
