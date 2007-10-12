#pragma once
#include "IFile.h"
#include "../cores/paplayer/RingHoldBuffer.h"
#include <map>
#include "../utils/HttpHeader.h"

class IHttpHeaderCallback
{
public:
  virtual void ParseHeaderData(CStdString strData) = 0;
  virtual ~IHttpHeaderCallback() { }
};

namespace XCURL
{
  typedef void CURL_HANDLE;
  typedef void CURLM;
  struct curl_slist;
};

class CHttpHeader;

namespace XFILE
{
	class CFileCurl : public IFile  
	{
    public:
	    CFileCurl();
	    virtual ~CFileCurl();
	    virtual bool Open(const CURL& url, bool bBinary = true);
	    virtual bool Exists(const CURL& url);
      virtual bool ReadString(char *szLine, int iLineLength);
	    virtual __int64	Seek(__int64 iFilePosition, int iWhence=SEEK_SET);
      virtual __int64 GetPosition();
	    virtual __int64	GetLength();
      virtual int	Stat(const CURL& url, struct __stat64* buffer);
	    virtual void Close();
      virtual unsigned int Read(void* lpBuf, __int64 uiBufSize);
      virtual CStdString GetContent()                            { return m_httpheader.GetContentType(); }
      
      size_t WriteCallback(char *buffer, size_t size, size_t nitems);
      size_t HeaderCallback(void *ptr, size_t size, size_t nmemb);
      
      void SetHttpHeaderCallback(IHttpHeaderCallback* pCallback) { m_pHeaderCallback = pCallback; }
      void SetUserAgent(CStdString sUserAgent)                   { m_userAgent = sUserAgent; }
      void SetProxy(CStdString &proxy)                           { m_proxy = proxy; }
      void SetCustomRequest(CStdString &request)                 { m_customrequest = request; }
      void UseOldHttpVersion(bool bUse)                          { m_useOldHttpVersion = bUse; }
      void SetContentEncoding(CStdString encoding)               { m_contentencoding = encoding; }
      void SetTimeout(int timeout)                               { m_timeout = timeout; }

      void SetRequestHeader(CStdString header, CStdString value);
      void SetRequestHeader(CStdString header, long value);      

      void ClearRequestHeaders();
      void SetBufferSize(unsigned int size);

      const CHttpHeader& GetHttpHeader() { return m_httpheader; }

      /* static function that will get content type of a file */      
      static bool GetHttpHeader(const CURL &url, CHttpHeader &headers);
      static bool GetContent(const CURL &url, CStdString &content);
    protected:
      void ParseAndCorrectUrl(CURL &url);
      void SetCommonOptions();
      void SetRequestHeaders();

      bool FillBuffer(unsigned int want);

    private:
      
      XCURL::CURL_HANDLE*    m_easyHandle;
      XCURL::CURLM*          m_multiHandle;
      CStdString      m_url;
      CStdString      m_userAgent;
      CStdString      m_proxy;
      CStdString      m_customrequest;
      CStdString      m_contentencoding;
      CStdString      m_ftpauth;
      CStdString      m_ftpport;
      bool            m_ftppasvip;
      int             m_timeout;

      __int64         m_fileSize;
      __int64	      m_filePos;
      bool            m_opened;
      bool            m_useOldHttpVersion;
      bool            m_seekable;

      CRingHoldBuffer m_buffer;           // our ringhold buffer
      char *          m_overflowBuffer;   // in the rare case we would overflow the above buffer
      unsigned int    m_overflowSize;     // size of the overflow buffer
      unsigned int    m_bufferSize;

      int             m_stillRunning; /* Is background url fetch still in progress */

      struct XCURL::curl_slist* m_curlAliasList;
      struct XCURL::curl_slist* m_curlHeaderList;
      IHttpHeaderCallback* m_pHeaderCallback;
      
      typedef std::map<CStdString, CStdString> MAPHTTPHEADERS;
      MAPHTTPHEADERS m_requestheaders;

      /* returned http header */
      CHttpHeader m_httpheader;
  };
};

