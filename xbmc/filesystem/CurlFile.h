#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "IFile.h"
#include "utils/RingBuffer.h"
#include <map>
#include "utils/HttpHeader.h"

namespace XCURL
{
  typedef void CURL_HANDLE;
  typedef void CURLM;
  struct curl_slist;
}

namespace XFILE
{
  class CCurlFile : public IFile
  {
    public:
      typedef enum
      {
        PROXY_HTTP = 0,
        PROXY_SOCKS4,
        PROXY_SOCKS4A,
        PROXY_SOCKS5,
        PROXY_SOCKS5_REMOTE,
      } ProxyType;
    
      CCurlFile();
      virtual ~CCurlFile();
      virtual bool Open(const CURL& url);
      virtual bool OpenForWrite(const CURL& url, bool bOverWrite = false);
      virtual bool Exists(const CURL& url);
      virtual int64_t  Seek(int64_t iFilePosition, int iWhence=SEEK_SET);
      virtual int64_t GetPosition();
      virtual int64_t  GetLength();
      virtual int  Stat(const CURL& url, struct __stat64* buffer);
      virtual void Close();
      virtual bool ReadString(char *szLine, int iLineLength)     { return m_state->ReadString(szLine, iLineLength); }
      virtual unsigned int Read(void* lpBuf, int64_t uiBufSize)  { return m_state->Read(lpBuf, uiBufSize); }
      virtual int Write(const void* lpBuf, int64_t uiBufSize);
      virtual CStdString GetMimeType()                           { return m_state->m_httpheader.GetMimeType(); }
      virtual CStdString GetContent()                            { return GetMimeType(); }
      virtual int IoControl(EIoControl request, void* param);
      virtual std::string GetContentCharset(void)                { return GetServerReportedCharset(); }

      bool Post(const CStdString& strURL, const CStdString& strPostData, CStdString& strHTML);
      bool Get(const CStdString& strURL, CStdString& strHTML);
      bool ReadData(CStdString& strHTML);
      bool Download(const CStdString& strURL, const CStdString& strFileName, LPDWORD pdwSize = NULL);
      bool IsInternet();
      void Cancel();
      void Reset();
      void SetUserAgent(CStdString sUserAgent)                   { m_userAgent = sUserAgent; }
      void SetProxy(CStdString &proxy)                           { m_proxy = proxy; }
      void SetProxyUserPass(CStdString &proxyuserpass)           { m_proxyuserpass = proxyuserpass; }
      void SetProxyType(ProxyType proxytype)                     { m_proxytype = proxytype; }
      void SetStreamProxy(const CStdString &proxy, ProxyType type);
      void SetCustomRequest(CStdString &request)                 { m_customrequest = request; }
      void UseOldHttpVersion(bool bUse)                          { m_useOldHttpVersion = bUse; }
      void SetContentEncoding(CStdString encoding)               { m_contentencoding = encoding; }
      void SetAcceptCharset(const std::string& charset)          { m_acceptCharset = charset; }
      void SetTimeout(int connecttimeout)                        { m_connecttimeout = connecttimeout; }
      void SetLowSpeedTime(int lowspeedtime)                     { m_lowspeedtime = lowspeedtime; }
      void SetPostData(CStdString postdata)                      { m_postdata = postdata; }
      void SetReferer(CStdString referer)                        { m_referer = referer; }
      void SetCookie(CStdString cookie)                          { m_cookie = cookie; }
      void SetMimeType(CStdString mimetype)                      { SetRequestHeader("Content-Type", mimetype); }
      void SetRequestHeader(CStdString header, CStdString value);
      void SetRequestHeader(CStdString header, long value);

      void ClearRequestHeaders();
      void SetBufferSize(unsigned int size);

      const CHttpHeader& GetHttpHeader() { return m_state->m_httpheader; }
      std::string GetServerReportedCharset(void);

      /* static function that will get content type of a file */
      static bool GetHttpHeader(const CURL &url, CHttpHeader &headers);
      static bool GetMimeType(const CURL &url, CStdString &content, CStdString useragent="");

      /* static function that will get cookies stored by CURL in RFC 2109 format */
      static bool GetCookies(const CURL &url, std::string &cookies);

      class CReadState
      {
      public:
          CReadState();
          ~CReadState();
          XCURL::CURL_HANDLE*    m_easyHandle;
          XCURL::CURLM*          m_multiHandle;

          CRingBuffer     m_buffer;           // our ringhold buffer
          unsigned int    m_bufferSize;

          char *          m_overflowBuffer;   // in the rare case we would overflow the above buffer
          unsigned int    m_overflowSize;     // size of the overflow buffer
          int             m_stillRunning;     // Is background url fetch still in progress
          bool            m_cancelled;
          int64_t         m_fileSize;
          int64_t         m_filePos;
          bool            m_bFirstLoop;
          bool            m_isPaused;
          bool            m_sendRange;

          char*           m_readBuffer;

          /* returned http header */
          CHttpHeader m_httpheader;
          bool        IsHeaderDone(void)
          { return m_httpheader.IsHeaderDone(); }

          struct XCURL::curl_slist* m_curlHeaderList;
          struct XCURL::curl_slist* m_curlAliasList;

          size_t ReadCallback(char *buffer, size_t size, size_t nitems);
          size_t WriteCallback(char *buffer, size_t size, size_t nitems);
          size_t HeaderCallback(void *ptr, size_t size, size_t nmemb);

          bool         Seek(int64_t pos);
          unsigned int Read(void* lpBuf, int64_t uiBufSize);
          bool         ReadString(char *szLine, int iLineLength);
          bool         FillBuffer(unsigned int want);
          void         SetReadBuffer(const void* lpBuf, int64_t uiBufSize);

          void         SetResume(void);
          long         Connect(unsigned int size);
          void         Disconnect();
      };

    protected:
      void ParseAndCorrectUrl(CURL &url);
      void SetCommonOptions(CReadState* state);
      void SetRequestHeaders(CReadState* state);
      void SetCorrectHeaders(CReadState* state);
      bool Service(const CStdString& strURL, CStdString& strHTML);

    protected:
      CReadState*     m_state;
      CReadState*     m_oldState;
      unsigned int    m_bufferSize;
      int64_t         m_writeOffset;

      CStdString      m_url;
      CStdString      m_userAgent;
      CStdString      m_proxy;
      CStdString      m_proxyuserpass;
      ProxyType       m_proxytype;
      CStdString      m_customrequest;
      CStdString      m_contentencoding;
      std::string     m_acceptCharset;
      CStdString      m_ftpauth;
      CStdString      m_ftpport;
      CStdString      m_binary;
      CStdString      m_postdata;
      CStdString      m_referer;
      CStdString      m_cookie;
      CStdString      m_username;
      CStdString      m_password;
      CStdString      m_httpauth;
      CStdString      m_cipherlist;
      bool            m_ftppasvip;
      int             m_connecttimeout;
      int             m_lowspeedtime;
      bool            m_opened;
      bool            m_forWrite;
      bool            m_inError;
      bool            m_useOldHttpVersion;
      bool            m_seekable;
      bool            m_multisession;
      bool            m_skipshout;
      bool            m_postdataset;

      CRingBuffer     m_buffer;           // our ringhold buffer
      char *          m_overflowBuffer;   // in the rare case we would overflow the above buffer
      unsigned int    m_overflowSize;     // size of the overflow buffer

      int             m_stillRunning;     // Is background url fetch still in progress?

      typedef std::map<CStdString, CStdString> MAPHTTPHEADERS;
      MAPHTTPHEADERS m_requestheaders;

      long            m_httpresponse;
  };
}
