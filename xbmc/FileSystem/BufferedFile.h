
#pragma once

#include "IFile.h"
using namespace XFILE;

namespace XFILE
{
  class CBufferedFile : public IFile
  {
  public:
    CBufferedFile(IFile* pFile, int iBufferSize);
    CBufferedFile(IFile* pFile);
    virtual ~CBufferedFile();

    bool Init(IFile* pFile, int iBufferSize);

    virtual bool Open(const CURL& url, bool bBinary = true);
    virtual bool OpenForWrite(const CURL& url, bool bBinary, bool bOverWrite) { Close(); return false; };
    virtual bool Exists(const CURL& url)                                      { return m_pFile->Exists(url); }
    virtual int Stat(const CURL& url, struct __stat64* buffer)                { return m_pFile->Stat(url, buffer); }
    
    virtual unsigned int Read(void* lpBuf, __int64 uiBufSize);
    virtual int Write(const void* lpBuf, __int64 uiBufSize)                   { return -1;}
    virtual bool ReadString(char *szLine, int iLineLength);
    virtual __int64 Seek(__int64 iFilePosition, int iWhence = SEEK_SET);
    virtual void Close();
    virtual __int64 GetPosition();
    virtual __int64 GetLength()                                               { return m_pFile->GetLength(); }
    virtual bool CanSeek()                                                    { return m_pFile->CanSeek(); }
    virtual char GetDirectorySeperator()                                      { return m_pFile->GetDirectorySeperator(); }
    virtual void Flush();
    virtual int GetChunkSize()                                                { return m_iMaxBufferSize; }
    virtual bool SkipNext()                                                   { return m_pFile->SkipNext();}

    virtual bool Delete(const char* strFileName)                              { return m_pFile->Delete(strFileName); }
    virtual bool Rename(const char* strFileName, const char* strNewFileName)  { return m_pFile->Rename(strFileName, strNewFileName); }
    
    IFile* GetFile()                                                          { return m_pFile; }
    
  private:
    unsigned int ReadChunks(BYTE* buffer, int iSize);
    unsigned int FillBuffer();
    unsigned int ReadFromBuffer(BYTE* pBuffer, int iSize);
    
    IFile* m_pFile;
    
    BYTE* m_buffer;
    int   m_iMaxBufferSize; // maximum buffer size
    int   m_iBufferSize;    // size of data in buffer
    int   m_iBufferPos;     // current read position in buffer
    int   m_iChunkSize;     // optimal read size
  };
};


