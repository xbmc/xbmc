#include "stdafx.h"
#include "FileZip.h"
#include <sys/stat.h>

#ifndef __GNUC__
#ifndef _XBOX
#pragma comment (lib,"../../xbmc/lib/zlib/zlib.lib")
#endif
#endif

using namespace XFILE;

CFileZip::CFileZip() : m_bUseProgressBar(false), m_dlgProgress(NULL)
{
  m_szStringBuffer = NULL;
  m_szStartOfStringBuffer = NULL;
  m_iDataInStringBuffer = 0;
}

CFileZip::~CFileZip()
{
  if (m_szStringBuffer)
    delete[] m_szStringBuffer;
  Close();
}

bool CFileZip::Open(const CURL&url, bool bBinary)
{
  CStdString strPath;
  url.GetURL(strPath);
  if (!g_ZipManager.GetZipEntry(strPath,mZipItem))
    return false;
  
  if ((mZipItem.flags & 64) == 64)
  {
    CLog::Log(LOGERROR,"FileZip: encrypted file, not supported!");
    return false;
  }
  
  if ((mZipItem.method != 8) && (mZipItem.method != 0))
  {
    CLog::Log(LOGERROR,"FileZip: unsupported compression method!");
    return false;
  }

  if (!mFile.Open(url.GetHostName(),true)) // this is the zip-file, always open binary
  {
    CLog::Log(LOGERROR,"FileZip: unable to open zip file %s!",url.GetHostName().c_str());
    return false;
  }
  mFile.Seek(mZipItem.offset,SEEK_SET);
  m_iFilePos = 0;
  m_iZipFilePos = 0;
  m_iAvailBuffer = 0;
  m_bFlush = false;
  m_ZStream.zalloc = Z_NULL;
  m_ZStream.zfree = Z_NULL;
  m_ZStream.opaque = Z_NULL;
  if( mZipItem.method != 0 )
    if (inflateInit2(&m_ZStream,-MAX_WBITS) != Z_OK)
    { 
      CLog::Log(LOGERROR,"FileZip: error initializing zlib!");
      return false;
    }
  m_ZStream.next_in = (Bytef*)m_szBuffer;
  m_ZStream.avail_in = 0;
  m_ZStream.total_out = 0;
  return true;
}

__int64 CFileZip::GetLength()
{
  return mZipItem.usize;
}

__int64 CFileZip::GetPosition()
{
  return m_iFilePos;
}

__int64 CFileZip::Seek(__int64 iFilePosition, int iWhence)
{
  if (mZipItem.method == 0) // this is easy
  {
    __int64 iResult;
    switch (iWhence)
    {
    case SEEK_SET:
      if (iFilePosition > mZipItem.usize)
        return -1;
      m_iFilePos = iFilePosition;
      m_iZipFilePos = m_iFilePos;
      iResult = mFile.Seek(iFilePosition+mZipItem.offset,SEEK_SET)-mZipItem.offset;
      return iResult;
      break;

    case SEEK_CUR:
      if (m_iFilePos+iFilePosition > mZipItem.usize)
        return -1;
      m_iFilePos += iFilePosition;
      m_iZipFilePos = m_iFilePos;
      iResult = mFile.Seek(iFilePosition,SEEK_CUR)-mZipItem.offset;
      return iResult;
      break;

    case SEEK_END:
      if (iFilePosition > mZipItem.usize)
        return -1;
      m_iFilePos = mZipItem.usize+iFilePosition;
      m_iZipFilePos = m_iFilePos;
      iResult = mFile.Seek(mZipItem.offset+mZipItem.usize+iFilePosition,SEEK_SET)-mZipItem.offset;
      return iResult;
      break;
    default:
      return -1;

    }
  }
  // here goes the stupid part..
  if (mZipItem.method == 8)
  {
    char temp[131072];
    __int64 iStartPos = m_iFilePos;
    switch (iWhence)
    {
    case SEEK_SET:
      if (iFilePosition == m_iFilePos)
        return m_iFilePos; // mp3reader does this lots-of-times
      if (iFilePosition > mZipItem.usize || iFilePosition < 0)
        return -1;
      // read until position in 128k blocks.. only way to do it due to format. 
      // can't start in the middle of data since then we'd have no clue where 
      // we are in uncompressed data..
      if (iFilePosition < m_iFilePos) 
      {
        m_iFilePos = 0; 
        m_iZipFilePos = 0;
        inflateEnd(&m_ZStream);
        inflateInit2(&m_ZStream,-MAX_WBITS); // simply restart zlib
        mFile.Seek(mZipItem.offset,SEEK_SET);
        m_ZStream.next_in = (Bytef*)m_szBuffer;
        m_ZStream.avail_in = 0;
        m_ZStream.total_out = 0;
        if (iFilePosition > 1024*1024) // 1 MB seek
        {
          StartProgressBar();
          m_bUseProgressBar = true;
        }
        while (m_iFilePos < iFilePosition)
        {
          unsigned int iToRead = (iFilePosition-m_iFilePos)>131072?131072:(int)(iFilePosition-m_iFilePos);
          if (Read(temp,iToRead) != iToRead)
            return -1;
          if (m_bUseProgressBar)
          {
            m_dlgProgress->SetPercentage(static_cast<int>(static_cast<float>(m_iFilePos)/static_cast<float>(iFilePosition)*100));
            m_dlgProgress->Progress();
          }
        }
        if( m_bUseProgressBar) 
        {
          StopProgressBar();
          m_bUseProgressBar = false;
        }
        return m_iFilePos;
      }
      else // seek forward
        return Seek(iFilePosition-m_iFilePos,SEEK_CUR);
      break;

    case SEEK_CUR:
      if (iFilePosition < 0)
        return Seek(m_iFilePos+iFilePosition,SEEK_SET); // can't rewind stream
      // read until requested position, drop data
      if (m_iFilePos+iFilePosition > mZipItem.usize)
        return -1;
      iFilePosition += m_iFilePos;
      if (iFilePosition-m_iFilePos > 1024*1024) // 1 MB seek
      {
        StartProgressBar();
        m_bUseProgressBar = true;
      }
      while (m_iFilePos < iFilePosition)
      {
        unsigned int iToRead = (iFilePosition-m_iFilePos)>131072?131072:(int)(iFilePosition-m_iFilePos);
        if (Read(temp,iToRead) != iToRead)
          return -1;
        if (m_bUseProgressBar)
        {
          m_dlgProgress->SetPercentage(static_cast<int>(static_cast<float>(m_iFilePos)/static_cast<float>(iFilePosition)*100));
          m_dlgProgress->Progress();
        }
      }
      if( m_bUseProgressBar) 
        {
          StopProgressBar();
          m_bUseProgressBar = false;
        }
      return m_iFilePos;
      break;

    case SEEK_END: 
      // now this is a nasty bastard, possibly takes lotsoftime
      // uncompress, minding m_ZStream.total_out
            
      if ((GetLength()+iFilePosition)-m_iFilePos > 1024*1024) // 1 MB seek
      {
        StartProgressBar();
        m_bUseProgressBar = true;
      }

      while( m_ZStream.total_out < mZipItem.usize+iFilePosition)
      {
        unsigned int iToRead = (mZipItem.usize+iFilePosition-m_ZStream.total_out > 131072)?131072:(int)(mZipItem.usize+iFilePosition-m_ZStream.total_out);
        if (Read(temp,iToRead) != iToRead)
          return -1;
        if (m_bUseProgressBar)
        {
          m_dlgProgress->SetPercentage(static_cast<int>(static_cast<float>(m_iFilePos-iStartPos)/static_cast<float>(mZipItem.usize+iFilePosition)*100));
          m_dlgProgress->Progress();
        }
      }
      if( m_bUseProgressBar) 
      {
          StopProgressBar();
          m_bUseProgressBar = false;
      }
      return m_iFilePos;
      break;
    default:
      return -1;
    }
  }
  return -1;
}

bool CFileZip::Exists(const CURL& url)
{
  SZipEntry item;
  CStdString strPath;
  url.GetURL(strPath);
  if (g_ZipManager.GetZipEntry(strPath,item))
    return true;
  return false;
}

int CFileZip::Stat(const CURL& url, struct __stat64* buffer)
{ 
  CStdString strPath;
  url.GetURL(strPath);
  if (!g_ZipManager.GetZipEntry(strPath,mZipItem))
    return -1;

  buffer->st_gid = 0;
#ifndef _LINUX
  buffer->st_atime = buffer->st_ctime = mZipItem.mod_time;
#else
  buffer->_st_atime = buffer->_st_ctime = mZipItem.mod_time;
#endif
  buffer->st_size = mZipItem.usize;
  return -1;
}

unsigned int CFileZip::Read(void* lpBuf, __int64 uiBufSize)
{
  // flush what might be left in the string buffer
  if (m_iDataInStringBuffer > 0)
  {
    size_t iMax = static_cast<size_t>((uiBufSize>m_iDataInStringBuffer?m_iDataInStringBuffer:uiBufSize));
    memcpy(lpBuf,m_szStartOfStringBuffer,iMax);
    uiBufSize -= iMax;
    m_iDataInStringBuffer -= iMax;
  }
  if (mZipItem.method == 8) // deflated
  {
    uLong iDecompressed = 0;
    uLong prevOut = m_ZStream.total_out;
    while ((iDecompressed < uiBufSize) && ((m_iZipFilePos < mZipItem.csize) || (m_bFlush)))
    {
      m_ZStream.next_out = (Bytef*)(lpBuf)+iDecompressed;
      m_ZStream.avail_out = static_cast<uInt>(uiBufSize-iDecompressed);
      if (m_bFlush) // need to flush buffer !
      {        
        int iMessage = inflate(&m_ZStream,Z_SYNC_FLUSH);        
        m_bFlush = ((iMessage == Z_OK) && (m_ZStream.avail_out == 0))?true:false;
        if (!m_ZStream.avail_out) // flush filled buffer, get out of here
        {
          iDecompressed = m_ZStream.total_out-prevOut;
          break;
        }
      }

      if (!m_ZStream.avail_in)
      {
        if (!FillBuffer()) // eof! 
        {
          iDecompressed = m_ZStream.total_out-prevOut;
          break;
        }
      }

      int iMessage = inflate(&m_ZStream,Z_SYNC_FLUSH);
      if (iMessage < 0)
      {
        Close();
        return 0; // READ ERROR
      }

      m_bFlush = ((iMessage == Z_OK) && (m_ZStream.avail_out == 0))?true:false; // more info in input buffer
      
      iDecompressed = m_ZStream.total_out-prevOut;
    }
    m_iFilePos += iDecompressed;
    return static_cast<unsigned int>(iDecompressed);
  }
  else if (mZipItem.method == 0) // uncompressed. just read from file, but mind our boundaries.
  {
    if (uiBufSize+m_iFilePos > mZipItem.csize)
      uiBufSize = mZipItem.csize-m_iFilePos;
    if (uiBufSize < 0)
    {
      return 0; // we are past eof, this shouldn't happen but test anyway
    }
    unsigned int iResult = mFile.Read(lpBuf,uiBufSize);
    m_iZipFilePos += iResult;
    m_iFilePos += iResult;
    return iResult;
  }
  else
    return false; // shouldn't happen. compression method checked in open
}

void CFileZip::Close()
{
  if (mZipItem.method == 8)
    inflateEnd(&m_ZStream);
  
  mFile.Close();
}
/* CHANGED: JM - moved to CFile
bool CFileZip::ReadString(char* szLine, int iLineLength)
{
  if (!m_szStringBuffer)
  {
    m_szStringBuffer = new char[1024]; // 1024 byte long strings per read
    m_szStartOfStringBuffer = m_szStringBuffer;
    m_iDataInStringBuffer = 0;
    m_iRead = 0;
  }
  
  bool bEof = m_iDataInStringBuffer==0;
  while ((iLineLength > 1) && (m_iRead > -1))
  {
    if (m_iDataInStringBuffer > 0)
    {
      bEof = false;
      m_iRead = 1;
      int iMax = (iLineLength<m_iDataInStringBuffer?iLineLength-1:m_iDataInStringBuffer-1);
      for( int i=0;i<iMax;++i )
      {
        if (m_szStartOfStringBuffer[i] == '\r') // mac or win32 endings
        {
          strncpy(szLine,m_szStartOfStringBuffer,i);
          szLine[i] = '\0';
          m_iDataInStringBuffer -= i+1;
          m_szStartOfStringBuffer += i+1;
          if( m_szStartOfStringBuffer[0] == '\n') // win32 endings
          { 
            m_szStartOfStringBuffer++;
            m_iDataInStringBuffer--;
          }
          return true;
        }
        else if (m_szStartOfStringBuffer[i] == '\n') // unix or fucked up win32 endings
        {
          strncpy(szLine,m_szStartOfStringBuffer,i);
          szLine[i] = '\0';
          m_iDataInStringBuffer -= i+1;
          m_szStartOfStringBuffer += i+1;
          if (m_szStartOfStringBuffer[0] == '\r')
          {
            m_szStartOfStringBuffer++;
            m_iDataInStringBuffer--;
          }
          return true;
        } 
      }
      strncpy(szLine,m_szStartOfStringBuffer,iMax);
      szLine += iMax;
      iLineLength -= iMax;
      m_iDataInStringBuffer -= iMax;
    }
    
    if (m_iRead == 1 && (m_iDataInStringBuffer == 1))
    {
      m_szStringBuffer[0] = m_szStringBuffer[1023]; // need to make sure we don't loose any '\r\n' between buffers
      m_iDataInStringBuffer = Read(m_szStringBuffer+1,1023); 
    }
    else
      m_iDataInStringBuffer = Read(m_szStringBuffer,1024); 
    m_szStartOfStringBuffer = m_szStringBuffer;
    if (m_iDataInStringBuffer)
      m_iRead = 1;
    else
      m_iRead = -1;
  }
  szLine[0] = '\0';
  return !bEof;
}*/

bool CFileZip::FillBuffer()
{
  unsigned int sToRead = 65535;
  if (m_iZipFilePos+65535 > mZipItem.csize)
    sToRead = static_cast<int>(mZipItem.csize-m_iZipFilePos);

  if (sToRead <= 0)
    return false; // eof!
  
  if (mFile.Read(m_szBuffer,sToRead) != sToRead)
    return false;
  m_ZStream.avail_in = sToRead;
  m_ZStream.next_in = (Bytef*)m_szBuffer;
  m_iZipFilePos += sToRead;
  return true;
}

void CFileZip::DestroyBuffer(void* lpBuffer, int iBufSize)
{
  if (!m_bFlush)
    return;
  int iMessage = Z_STREAM_END; // whatever != Z_OK
  while ((iMessage == Z_OK) && (m_ZStream.avail_out == 0))
  {
    m_ZStream.next_out = (Bytef*)lpBuffer;
    m_ZStream.avail_out = iBufSize;
    iMessage = inflate(&m_ZStream,Z_SYNC_FLUSH);
  }
  m_bFlush = false;
}

void CFileZip::StartProgressBar()
{
  if (!m_dlgProgress)
    m_dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  m_dlgProgress->StartModal();
  m_dlgProgress->SetPercentage(0);
  m_dlgProgress->SetHeading(773);
  m_dlgProgress->SetLine(0,"");
  m_dlgProgress->SetLine(1,"");
  m_dlgProgress->SetLine(2,"");
  m_dlgProgress->ShowProgressBar(true);
}

void CFileZip::StopProgressBar()
{
  m_dlgProgress->Close();
}
