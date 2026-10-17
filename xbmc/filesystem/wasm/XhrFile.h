/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "filesystem/IFile.h"
#include "utils/HttpHeader.h"

#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace XFILE
{
/*! \brief HTTP(S) file backed by the browser's XMLHttpRequest.
 *
 * Used by the WASM build when no BSD sockets are available, so libcurl cannot
 * connect. Requests are synchronous XHRs issued from the calling pthread; the
 * browser handles TLS, CORS (with the dev proxy in tools/wasm/dev_proxy_pre.js),
 * redirects and cookies. GET bodies are read through ranged windows so seeking in a
 * large file does not download all of it.
 */
class CXhrFile : public IFile
{
public:
  CXhrFile();
  ~CXhrFile() override;

  bool Open(const CURL& url) override;
  bool OpenForWrite(const CURL& url, bool bOverWrite = false) override { return false; }
  bool ReOpen(const CURL& url) override { return Open(url); }
  bool Exists(const CURL& url) override;
  int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET) override;
  int64_t GetPosition() override;
  int64_t GetLength() override;
  int Stat(const CURL& url, struct __stat64* buffer) override;
  void Close() override;
  ReadLineResult ReadLine(char* buffer, std::size_t bufferSize) override;
  ssize_t Read(void* lpBuf, size_t uiBufSize) override;
  ssize_t Write(const void* lpBuf, size_t uiBufSize) override { return -1; }
  const std::string GetProperty(XFILE::FileProperty type,
                                const std::string& name = "") const override;
  const std::vector<std::string> GetPropertyValues(XFILE::FileProperty type,
                                                   const std::string& name = "") const override;
  int IoControl(IOControl request, void* param) override;
  double GetDownloadSpeed() override { return 0.0; }

  bool Get(const std::string& url, std::string& body);
  bool Post(const std::string& url, const std::string& postData, std::string& body);
  bool IsInternet();
  // A synchronous XHR cannot be aborted from another thread; Cancel() fails the
  // requests that follow.
  void Cancel() { m_cancelled = true; }
  void Reset() { m_cancelled = false; }

  void SetUserAgent(const std::string& userAgent) { m_userAgent = userAgent; }
  void SetReferer(const std::string& referer) { m_referer = referer; }
  void SetTimeout(int connectTimeoutSeconds) { m_timeout = connectTimeoutSeconds; }

  static bool GetHttpHeader(const CURL& url, CHttpHeader& headers);
  static bool GetMimeType(const CURL& url, std::string& content, const std::string& useragent = "");
  static bool GetContentType(const CURL& url,
                             std::string& content,
                             const std::string& useragent = "");

private:
  struct Request;
  struct Response;

  std::string ApplyProtocolOptions(const CURL& url);
  bool ReadData(std::string& body);
  Request BuildRequest() const;
  Response Fetch(const Request& request,
                 const std::string& method,
                 const std::string& body,
                 int64_t rangeStart,
                 int64_t rangeLength) const;
  int Head(const CURL& url);
  bool FillWindow(int64_t position, size_t want);
  void DropWindow();

  std::string m_url;
  std::map<std::string, std::string> m_requestHeaders;
  std::string m_userAgent;
  std::string m_referer;
  std::string m_cookie;
  std::string m_customRequest;
  bool m_noRedirect{false};
  std::string m_acceptEncoding;
  std::string m_acceptCharset;
  std::string m_postData;
  bool m_postDataSet{false};
  int m_timeout{0};
  std::atomic<bool> m_cancelled{false};

  bool m_opened{false};
  int m_httpResponse{-1};
  CHttpHeader m_httpHeader;
  std::string m_redirectUrl;

  // The window is the part of the body currently held in memory: bytes
  // [m_windowStart, m_windowStart + m_windowSize) of the resource.
  std::unique_ptr<char[]> m_window;
  size_t m_windowSize{0};
  int64_t m_windowStart{0};
  int64_t m_filePos{0};
  int64_t m_fileSize{0};
  bool m_rangeSupported{false};
};
} // namespace XFILE
