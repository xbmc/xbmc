
#include "stdafx.h"
#include "DVDSubtitleStream.h"
#include "../DVDInputStreams/DVDFactoryInputStream.h"
#include "../DVDInputStreams/DVDInputStream.h"

using namespace std;

char* strnchr(char *s, size_t len, char c)
{
  size_t pos;

  for (pos = 0; pos < len; pos++)
  {
    if (s[pos] == c) return s + pos;
  }

  return 0;
}

char* strnrchr(const char *sp, int c, size_t n)
{
	const char *r = 0;

	while (n-- > 0 && *sp)
	{
		if (*sp == c)	r = sp;
		sp++;
	}

	return ((char *)r);
}


CDVDSubtitleStream::CDVDSubtitleStream()
{
  m_pInputStream = NULL;
  m_buffer = NULL;
  m_iMaxBufferSize = 0;
  m_iBufferSize = 0;
  m_iBufferPos = 0;
}

CDVDSubtitleStream::~CDVDSubtitleStream()
{
  if (m_buffer) delete[] m_buffer;
  if (m_pInputStream) delete m_pInputStream;
}

bool CDVDSubtitleStream::Open(const string& strFile)
{
  m_pInputStream = CDVDFactoryInputStream::CreateInputStream(NULL, strFile, "");
  if (m_pInputStream && m_pInputStream->Open(strFile.c_str(), ""))
  {
    // set up our buffers
    int iBlockSize = m_pInputStream->GetBlockSize();
    m_buffer = new BYTE[iBlockSize];
    
    if (m_buffer)
    {
      m_iMaxBufferSize = iBlockSize;
      m_iBufferSize = 0;
      m_iBufferPos = 0;
      
      return true;
    }
  }
  
  return false;
}

void CDVDSubtitleStream::Close()
{
  if (m_pInputStream)
    SAFE_DELETE(m_pInputStream);
  if (m_buffer) 
    SAFE_DELETE_ARRAY(m_buffer);
  
  m_iMaxBufferSize = 0;
}

int CDVDSubtitleStream::Read(BYTE* buf, int buf_size)
{
  if (m_buffer && m_pInputStream)
  {
    if (m_iBufferSize == 0)
    {
      // need to read more data, buffer is empty
      m_iBufferPos = 0;
      m_iBufferSize = 0;
      
      int iRes = m_pInputStream->Read(m_buffer, m_iMaxBufferSize);
      if (iRes > 0)
      {
        m_iBufferSize = iRes;
      }
      else
      {
        // error
        return iRes;
      }
    }
    
    int iBytesLeft = m_iBufferSize - m_iBufferPos;
    int iBytesToRead = buf_size;
    
    if (iBytesToRead > iBytesLeft) iBytesToRead = iBytesLeft;
    
    memcpy(buf, m_buffer + m_iBufferPos, iBytesToRead);
    m_iBufferPos += iBytesToRead;
    m_iBufferSize -= iBytesToRead;
    
    return iBytesToRead;
  }
  
  return 0;
}

__int64 CDVDSubtitleStream::Seek(__int64 offset, int whence)
{
  if (m_pInputStream)
  {
    __int64 iPos = m_pInputStream->Seek(0LL, SEEK_CUR);
    
    switch (whence)
    {
      case SEEK_CUR:
      {
        if ((m_iBufferPos + offset) > m_iBufferSize)
        {
          m_iBufferPos = 0;
          m_iBufferSize = 0;
          
          return m_pInputStream->Seek(offset, whence);
        }
        else
        {
          m_iBufferPos += (int)offset;
          m_iBufferSize -= (int)offset;
          
          return iPos + m_iBufferPos;
        }
        break;
      }
      case SEEK_END:
      {
        // just drop buffer
        m_iBufferPos = 0;
        m_iBufferSize = 0;
        
        return m_pInputStream->Seek(offset, whence);
      }
      case SEEK_SET:
      {
        if ((offset - iPos) > m_iBufferSize)
        {
          m_iBufferPos = 0;
          m_iBufferSize = 0;
          
          return m_pInputStream->Seek(offset, whence);
        }
        else
        {
          m_iBufferPos += (int)(offset - iPos);
          m_iBufferSize -= (int)(offset - iPos);
          
          return offset;
        }
        break;
      }
    }
  }
  return -1;
}

char* CDVDSubtitleStream::ReadLine(char* buf, int iLen)
{
  char* pBuffer = buf;
  
  pBuffer[0] = '\0';
  
  if (m_buffer)
  {
    while (iLen > 0)
    {
      if (m_iBufferSize == 0)
      {
        // need to read more data, buffer is empty
        m_iBufferPos = 0;
        m_iBufferSize = 0;
        
        int iRes = m_pInputStream->Read(m_buffer, m_iMaxBufferSize);
        if (iRes > 0)
        {
          m_iBufferSize = iRes;
        }
        else
        {
          // error
          return NULL;
        }
      }
      
      int iBytesToRead = iLen;
      if (iBytesToRead > m_iBufferSize) iBytesToRead = m_iBufferSize;    
      
      // now find end of string
      char* pLineStart = (char*)(m_buffer + m_iBufferPos);
      char* pLineEnd = strnchr(pLineStart, m_iBufferSize, '\n');
      if (pLineEnd)
      {
        pLineEnd[0] = '\0';
        if ((pLineEnd - pLineStart > 0) && pLineEnd[-1] == '\r') pLineEnd[-1] = '\0';
        
        // we have a line
        int iStringLength = (pLineEnd + 1) - pLineStart; // + 1 for '\n'
        if (iStringLength > iLen)
        {
          // unable to copy string
          return NULL;
        }
        
        m_iBufferPos += iStringLength;
        m_iBufferSize -= iStringLength;
        
        memcpy(pBuffer, pLineStart, iStringLength);

        pBuffer += iStringLength;
        pBuffer[0] = 0;
        return buf;
      }
      else
      {
        // didn't find new line, copy all data to buffer and try again
        if (m_iBufferSize > iLen)
        {
          // buffer is to small
          return NULL;
        }
        
        memcpy(pBuffer, m_buffer + m_iBufferPos, m_iBufferSize);
        m_iBufferPos += m_iBufferSize;
        m_iBufferSize -= m_iBufferSize;
        pBuffer += m_iBufferSize;
      }
    }
  }
  return buf;
}
