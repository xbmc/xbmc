#include "pch.h"
#include "HTTPFile.h"

using namespace XFILE;

CHTTPFile::CHTTPFile(void)
{
}


CHTTPFile::~CHTTPFile(void)
{
}

bool CHTTPFile::OpenForWrite(const CURL& url, bool bOverWrite)
{
  // real Open is delayed until we receive the POST data
  m_urlforwrite = url;
  m_openedforwrite = true;
  return true;
}

int CHTTPFile::Write(const void* lpBuf, int64_t uiBufSize)
{
  // Although we can not verify much, try to catch errors where we can
  if (!m_openedforwrite)
    return -1;

  CStdString myPostData((char*) lpBuf);
  if (myPostData.length() != uiBufSize)
    return -1;

  // If we get here, we (most likely) satisfied the pre-conditions that we used OpenForWrite and passed a string as postdata
  // we mimic 'post(..)' but do not read any data
  m_postdata = myPostData;
  m_postdataset = true;
  m_openedforwrite = false;
  SetMimeType("application/json");
  if (!Open(m_urlforwrite))
    return -1;

  // Finally (and this is a clumsy hack) return the http response code
  return (int) m_httpresponse;
}

