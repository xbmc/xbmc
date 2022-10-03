/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IFile.h"
#include "utils/HttpHeader.h"
#include "utils/RingBuffer.h"

#include <map>
#include <string>

typedef void CURL_HANDLE;
typedef void CURLM;
struct curl_slist;

namespace XFILE
{
  class CCurlFile : public IFile
  {
    private:
      typedef enum
      {
        PROXY_HTTP = 0,
        PROXY_SOCKS4,
        PROXY_SOCKS4A,
        PROXY_SOCKS5,
        PROXY_SOCKS5_REMOTE,
        PROXY_HTTPS,
      } ProxyType;

    public:
      CCurlFile();
      ~CCurlFile() override;
      bool Open(const CURL& url) override;
      bool OpenForWrite(const CURL& url, bool bOverWrite = false) override;
      bool ReOpen(const CURL& url) override;
      bool Exists(const CURL& url) override;
      int64_t Seek(int64_t iFilePosition, int iWhence=SEEK_SET) override;
      int64_t GetPosition() override;
      int64_t GetLength() override;
      int Stat(const CURL& url, struct __stat64* buffer) override;
      void Close() override;
      bool ReadString(char *szLine, int iLineLength) override { return m_state->ReadString(szLine, iLineLength); }
      ssize_t Read(void* lpBuf, size_t uiBufSize) override { return m_state->Read(lpBuf, uiBufSize); }
      ssize_t Write(const void* lpBuf, size_t uiBufSize) override;
      const std::string GetProperty(XFILE::FileProperty type, const std::string &name = "") const override;
      const std::vector<std::string> GetPropertyValues(XFILE::FileProperty type, const std::string &name = "") const override;
      int IoControl(EIoControl request, void* param) override;
      double GetDownloadSpeed() override;

      bool Post(const std::string& strURL, const std::string& strPostData, std::string& strHTML);
      bool Get(const std::string& strURL, std::string& strHTML);
      bool ReadData(std::string& strHTML);
      bool Download(const std::string& strURL, const std::string& strFileName, unsigned int* pdwSize = NULL);
      bool IsInternet();
      void Cancel();
      void Reset();
      void SetUserAgent(const std::string& sUserAgent) { m_userAgent = sUserAgent; }
      void SetProxy(const std::string &type, const std::string &host, uint16_t port,
                    const std::string &user, const std::string &password);
      void SetCustomRequest(const std::string &request) { m_customrequest = request; }
      void SetAcceptEncoding(const std::string& encoding) { m_acceptencoding = encoding; }
      void SetAcceptCharset(const std::string& charset) { m_acceptCharset = charset; }
      void SetTimeout(int connecttimeout) { m_connecttimeout = connecttimeout; }
      void SetLowSpeedTime(int lowspeedtime) { m_lowspeedtime = lowspeedtime; }
      void SetPostData(const std::string& postdata) { m_postdata = postdata; }
      void SetReferer(const std::string& referer) { m_referer = referer; }
      void SetCookie(const std::string& cookie) { m_cookie = cookie; }
      void SetMimeType(const std::string& mimetype) { SetRequestHeader("Content-Type", mimetype); }
      void SetRequestHeader(const std::string& header, const std::string& value);
      void SetRequestHeader(const std::string& header, long value);

      void ClearRequestHeaders();
      void SetBufferSize(unsigned int size);

      const CHttpHeader& GetHttpHeader() const { return m_state->m_httpheader; }
      std::string GetURL(void);
      std::string GetRedirectURL();

      /* static function that will get content type of a file */
      static bool GetHttpHeader(const CURL &url, CHttpHeader &headers);
      static bool GetMimeType(const CURL &url, std::string &content, const std::string &useragent="");
      static bool GetContentType(const CURL &url, std::string &content, const std::string &useragent = "");

      /* static function that will get cookies stored by CURL in RFC 2109 format */
      static bool GetCookies(const CURL &url, std::string &cookies);

      class CReadState
      {
      public:
          CReadState();
          ~CReadState();
          CURL_HANDLE* m_easyHandle;
          CURLM* m_multiHandle;

          CRingBuffer m_buffer; // our ringhold buffer
          unsigned int m_bufferSize;

          char* m_overflowBuffer; // in the rare case we would overflow the above buffer
          unsigned int m_overflowSize; // size of the overflow buffer
          int m_stillRunning; // Is background url fetch still in progress
          bool m_cancelled;
          int64_t m_fileSize;
          int64_t m_filePos;
          bool m_bFirstLoop;
          bool m_isPaused;
          bool m_sendRange;
          bool m_bLastError;
          bool m_bRetry;

          char* m_readBuffer;

          /* returned http header */
          CHttpHeader m_httpheader;
          bool IsHeaderDone(void) { return m_httpheader.IsHeaderDone(); }

          curl_slist* m_curlHeaderList;
          curl_slist* m_curlAliasList;

          size_t ReadCallback(char *buffer, size_t size, size_t nitems);
          size_t WriteCallback(char *buffer, size_t size, size_t nitems);
          size_t HeaderCallback(void *ptr, size_t size, size_t nmemb);

          bool Seek(int64_t pos);
          ssize_t Read(void* lpBuf, size_t uiBufSize);
          bool ReadString(char *szLine, int iLineLength);
          int8_t FillBuffer(unsigned int want);
          void SetReadBuffer(const void* lpBuf, int64_t uiBufSize);

          void SetResume(void);
          long Connect(unsigned int size);
          void Disconnect();
      };

    protected:
      void ParseAndCorrectUrl(CURL &url);
      void SetCommonOptions(CReadState* state, bool failOnError = true);
      void SetRequestHeaders(CReadState* state);
      void SetCorrectHeaders(CReadState* state);
      bool Service(const std::string& strURL, std::string& strHTML);
      std::string GetInfoString(int infoType);

    protected:
      CReadState* m_state;
      CReadState* m_oldState;
      unsigned int m_bufferSize;
      int64_t m_writeOffset = 0;

      std::string m_url;
      std::string m_userAgent;
      ProxyType m_proxytype = PROXY_HTTP;
      std::string m_proxyhost;
      uint16_t m_proxyport = 3128;
      std::string m_proxyuser;
      std::string m_proxypassword;
      std::string m_customrequest;
      std::string m_acceptencoding;
      std::string m_acceptCharset;
      std::string m_ftpauth;
      std::string m_ftpport;
      std::string m_binary;
      std::string m_postdata;
      std::string m_referer;
      std::string m_cookie;
      std::string m_username;
      std::string m_password;
      std::string m_httpauth;
      std::string m_cipherlist;
      bool m_ftppasvip;
      int m_connecttimeout;
      int m_redirectlimit;
      int m_lowspeedtime;
      bool m_opened;
      bool m_forWrite;
      bool m_inError;
      bool m_seekable;
      bool m_multisession;
      bool m_skipshout;
      bool m_postdataset;
      bool m_allowRetry;
      bool m_verifyPeer = true;
      bool m_failOnError = true;
      curl_slist* m_dnsCacheList = nullptr;

      CRingBuffer m_buffer; // our ringhold buffer
      char* m_overflowBuffer; // in the rare case we would overflow the above buffer
      unsigned int m_overflowSize = 0; // size of the overflow buffer

      int  m_stillRunning; // Is background url fetch still in progress?

      typedef std::map<std::string, std::string> MAPHTTPHEADERS;
      MAPHTTPHEADERS m_requestheaders;

      long m_httpresponse;
  };
}
