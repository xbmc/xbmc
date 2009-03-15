#pragma once
/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "IFile.h"
#include "RingBuffer.h"
#include <map>
#include "utils/HttpHeader.h"

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
      virtual __int64  Seek(__int64 iFilePosition, int iWhence=SEEK_SET);
      virtual __int64 GetPosition();
      virtual __int64  GetLength();
      virtual int  Stat(const CURL& url, struct __stat64* buffer);
      virtual void Close();
      virtual bool ReadString(char *szLine, int iLineLength)     { return m_state->ReadString(szLine, iLineLength); }
      virtual unsigned int Read(void* lpBuf, __int64 uiBufSize)  { return m_state->Read(lpBuf, uiBufSize); }
      virtual CStdString GetContent()                            { return m_state->m_httpheader.GetContentType(); }

      bool Post(const CStdString& strURL, const CStdString& strPostData, CStdString& strHTML);
      bool Get(const CStdString& strURL, CStdString& strHTML);
      bool ReadData(CStdString& strHTML);
      bool Download(const CStdString& strURL, const CStdString& strFileName, LPDWORD pdwSize = NULL);
      bool IsInternet(bool checkDNS = true);
      void Cancel();
      void Reset();
      void SetUserAgent(CStdString sUserAgent)                   { m_userAgent = sUserAgent; }
      void SetProxy(CStdString &proxy)                           { m_proxy = proxy; }
      void SetProxyUserPass(CStdString &proxyuserpass)           { m_proxyuserpass = proxyuserpass; }
      void SetCustomRequest(CStdString &request)                 { m_customrequest = request; }
      void UseOldHttpVersion(bool bUse)                          { m_useOldHttpVersion = bUse; }
      void SetContentEncoding(CStdString encoding)               { m_contentencoding = encoding; }
      void SetTimeout(int connecttimeout)                        { m_connecttimeout = connecttimeout; }
      void SetLowSpeedTime(int lowspeedtime)                     { m_lowspeedtime = lowspeedtime; }
      void SetPostData(CStdString postdata)                      { m_postdata = postdata; }
      void SetReferer(CStdString referer)                        { m_referer = referer; }
      void SetContentType(CStdString contenttype)                { SetRequestHeader("Content-Type", m_contenttype); }
      void SetRequestHeader(CStdString header, CStdString value);
      void SetRequestHeader(CStdString header, long value);

      void ClearRequestHeaders();
      void SetBufferSize(unsigned int size);

      const CHttpHeader& GetHttpHeader() { return m_state->m_httpheader; }

      /* static function that will get content type of a file */
      static bool GetHttpHeader(const CURL &url, CHttpHeader &headers);
      static bool GetContent(const CURL &url, CStdString &content, CStdString useragent="");

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
          __int64         m_fileSize;
          __int64         m_filePos;

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
      bool Service(const CStdString& strURL, const CStdString& strPostData, CStdString& strHTML);

    private:
      CReadState*     m_state;
      unsigned int    m_bufferSize;

      CStdString      m_url;
      CStdString      m_userAgent;
      CStdString      m_proxy;
      CStdString      m_proxyuserpass;
      CStdString      m_customrequest;
      CStdString      m_contentencoding;
      CStdString      m_ftpauth;
      CStdString      m_ftpport;
      CStdString      m_binary;
      CStdString      m_postdata;
      CStdString      m_referer;
      CStdString      m_contenttype;
      bool            m_ftppasvip;
      int             m_connecttimeout;
      int             m_lowspeedtime;
      bool            m_opened;
      bool            m_useOldHttpVersion;
      bool            m_seekable;
      bool            m_multisession;

      CRingBuffer     m_buffer;           // our ringhold buffer
      char *          m_overflowBuffer;   // in the rare case we would overflow the above buffer
      unsigned int    m_overflowSize;     // size of the overflow buffer

      int             m_stillRunning;     // Is background url fetch still in progress?

      struct XCURL::curl_slist* m_curlAliasList;
      struct XCURL::curl_slist* m_curlHeaderList;

      typedef std::map<CStdString, CStdString> MAPHTTPHEADERS;
      MAPHTTPHEADERS m_requestheaders;
  };
}


