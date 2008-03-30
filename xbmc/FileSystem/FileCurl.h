#pragma once
#include "IFile.h"
#include "RingBuffer.h"
#include <map>
#include "../utils/HttpHeader.h"

namespace XCURL
{
  typedef void CURL_HANDLE;
  typedef void CURLM;
  struct curl_slist;
}

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
	    virtual __int64	Seek(__int64 iFilePosition, int iWhence=SEEK_SET);
      virtual __int64 GetPosition();
	    virtual __int64	GetLength();
      virtual int	Stat(const CURL& url, struct __stat64* buffer);
	    virtual void Close();
      virtual bool ReadString(char *szLine, int iLineLength)     { return m_state->ReadString(szLine, iLineLength); }
      virtual unsigned int Read(void* lpBuf, __int64 uiBufSize)  { return m_state->Read(lpBuf, uiBufSize); }
      virtual CStdString GetContent()                            { return m_state->m_httpheader.GetContentType(); }
            
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

      const CHttpHeader& GetHttpHeader() { return m_state->m_httpheader; }

      /* static function that will get content type of a file */      
      static bool GetHttpHeader(const CURL &url, CHttpHeader &headers);
      static bool GetContent(const CURL &url, CStdString &content);

      class CReadState
      {
      public:
          CReadState();
          ~CReadState();
          XCURL::CURL_HANDLE*    m_easyHandle;
          XCURL::CURLM*          m_multiHandle;

          CRingBuffer m_buffer;           // our ringhold buffer
          unsigned int    m_bufferSize;

          char *          m_overflowBuffer;   // in the rare case we would overflow the above buffer
          unsigned int    m_overflowSize;     // size of the overflow buffer
          int             m_stillRunning; /* Is background url fetch still in progress */

          __int64         m_fileSize;
          __int64	        m_filePos;

          bool            m_connected;

          /* returned http header */
          CHttpHeader m_httpheader;

          size_t WriteCallback(char *buffer, size_t size, size_t nitems);
          size_t HeaderCallback(void *ptr, size_t size, size_t nmemb);

          bool         Seek(__int64 pos);
          unsigned int Read(void* lpBuf, __int64 uiBufSize);
          bool         ReadString(char *szLine, int iLineLength);
          bool         FillBuffer(unsigned int want);

          long         Connect(unsigned int size);
          void         Disconnect();
      };

    protected:
      void ParseAndCorrectUrl(CURL &url);
      void SetCommonOptions(CReadState* state);
      void SetRequestHeaders(CReadState* state);
      void SetCorrectHeaders(CReadState* state);

    private:
      CReadState*     m_state;
      unsigned int    m_bufferSize;

      CStdString      m_url;
      CStdString      m_userAgent;
      CStdString      m_proxy;
      CStdString      m_customrequest;
      CStdString      m_contentencoding;
      CStdString      m_ftpauth;
      CStdString      m_ftpport;
      bool            m_ftppasvip;
      int             m_timeout;
      bool            m_opened;
      bool            m_useOldHttpVersion;
      bool            m_seekable;
      bool            m_binary;

      CRingBuffer     m_buffer;           // our ringhold buffer
      char *          m_overflowBuffer;   // in the rare case we would overflow the above buffer
      unsigned int    m_overflowSize;     // size of the overflow buffer

      int             m_stillRunning; /* Is background url fetch still in progress */

      struct XCURL::curl_slist* m_curlAliasList;
      struct XCURL::curl_slist* m_curlHeaderList;
      
      typedef std::map<CStdString, CStdString> MAPHTTPHEADERS;
      MAPHTTPHEADERS m_requestheaders;
  };
}


