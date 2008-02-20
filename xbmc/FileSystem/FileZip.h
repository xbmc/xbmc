#ifndef FILE_ZIP_H_
#define FILE_ZIP_H_

#include "IFile.h"
#include "ZipManager.h"
#include "../lib/zlib/zlib.h"
#include "../utils/log.h"
#include "../../guilib/GUIWindowManager.h"
#include "../GUIDialogProgress.h"

namespace XFILE
{
  class CFileZip : public IFile
  {
  public:
    CFileZip();
    virtual ~CFileZip();
  
    virtual __int64 GetPosition();
    virtual __int64 GetLength();
    virtual bool Open(const CURL& url, bool bBinary = true);
    virtual bool Exists(const CURL& url);
    virtual int Stat(const CURL& url, struct __stat64* buffer);
    virtual unsigned int Read(void* lpBuf, __int64 uiBufSize);
    //virtual bool ReadString(char *szLine, int iLineLength);
    virtual __int64 Seek(__int64 iFilePosition, int iWhence = SEEK_SET);
    virtual void Close();

    int UnpackFromMemory(std::string& strDest, const std::string& strInput);
  private:
    bool InitDecompress();
    bool FillBuffer();
    void DestroyBuffer(void* lpBuffer, int iBufSize);
    void StartProgressBar();
    void StopProgressBar();
    CFile mFile;
    SZipEntry mZipItem;
    __int64 m_iFilePos; // position in _uncompressed_ data read
    __int64 m_iZipFilePos; // position in _compressed_ data
    int m_iAvailBuffer;
    z_stream m_ZStream;
    char m_szBuffer[65535];     // 64k buffer for compressed data
    char* m_szStringBuffer;
    char* m_szStartOfStringBuffer; // never allocated!
    int m_iDataInStringBuffer;
    int m_iRead;
    bool m_bFlush;
    bool m_bUseProgressBar;
    CGUIDialogProgress* m_dlgProgress; // used if seeking is required..
  };
}

#endif
