/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "XhrFile.h"

#include "URL.h"
#include "utils/Base64.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>

#include <emscripten.h>

using namespace XFILE;

namespace
{
constexpr int DEFAULT_REQUEST_TIMEOUT_SECONDS = 30;
constexpr int MAX_EAGER_RESPONSE_BYTES = 1024 * 1024 * 1024;
constexpr int64_t RANGE_WINDOW_BYTES = 2 * 1024 * 1024;

// Synchronous XHR on a pthread serializes requests per thread, so one store
// per module suffices between a request and the copies that follow it.
// clang-format off
EM_JS(int, kodi_xhr_request,
      (const char* methodPtr, const char* urlPtr, const char* bodyPtr, int bodyLen,
       const char* headersPtr, const char* userAgentPtr, const char* refererPtr,
       int timeoutSeconds, int includeBody), {
  var method = UTF8ToString(methodPtr) || "GET";
  var url = UTF8ToString(urlPtr);
  var body = bodyLen > 0 ? HEAPU8.slice(bodyPtr, bodyPtr + bodyLen) : null;
  var headers = headersPtr ? UTF8ToString(headersPtr) : "";
  var userAgent = userAgentPtr ? UTF8ToString(userAgentPtr) : "";
  var referer = refererPtr ? UTF8ToString(refererPtr) : "";

  // Browsers quote the URL, credentials included, in their error messages.
  var redact = function(s) { return String(s).replace(new RegExp("//[^/@ ]*@", "g"), "//"); };

  var store = Module.kodiXhrStore = Module.kodiXhrStore || {};
  store.status = 0;
  store.redirected = false;
  store.responseURL = "";
  store.rawHeaders = "";
  store.body = null;
  store.contentLength = -1;
  store.error = "";

  try {
    var xhr = new XMLHttpRequest();
    xhr.open(method, url, false);
    if (includeBody)
      xhr.responseType = "arraybuffer";
    if (timeoutSeconds > 0)
      xhr.timeout = timeoutSeconds * 1000;
    // headers is "name:value\nname:value\n..."
    if (headers) {
      var lines = headers.split("\n");
      for (var i = 0; i < lines.length; ++i) {
        var sep = lines[i].indexOf(":");
        if (sep > 0) {
          try { xhr.setRequestHeader(lines[i].substr(0, sep),
                                     lines[i].substr(sep + 1)); }
          catch (_) {} // setRequestHeader throws on forbidden header names
        }
      }
    }
    if (userAgent) { try { xhr.setRequestHeader("User-Agent", userAgent); } catch (_) {} }
    if (referer)   { try { xhr.setRequestHeader("Referer",    referer);   } catch (_) {} }
    xhr.send(body);

    store.status = xhr.status | 0;
    var plain = function(u) { u = new URL(u); u.hash = u.username = u.password = ""; return u.href; };
    store.redirected = !!xhr.responseURL && plain(xhr.responseURL) !== plain(url);
    store.responseURL = xhr.responseURL || url;
    store.rawHeaders = xhr.getAllResponseHeaders() || "";
    var clen = xhr.getResponseHeader("Content-Length");
    store.contentLength = clen ? Number(clen) : -1;
    if (includeBody && xhr.response)
      store.body = new Uint8Array(xhr.response);
    if (store.status === 0)
      store.error = "no response (blocked by the browser: CORS, mixed content or network)";
    return store.status;
  } catch (e) {
    store.error = redact((e && e.name ? e.name + ": " : "") + (e && e.message ? e.message : String(e)));
    console.warn("[kodi][xhr] request failed:", method, store.error);
    return 0;
  }
});

EM_JS(int, kodi_xhr_body_size, (), {
  var s = Module.kodiXhrStore || {};
  return s.body ? (s.body.length | 0) : 0;
});

// Hands the body over to the heap: the store keeps no copy, so a large response
// is not held twice until the thread's next request.
EM_JS(int, kodi_xhr_copy_body, (char* outPtr, int outSize), {
  var s = Module.kodiXhrStore || {};
  var body = s.body;
  s.body = null;
  if (!body || !outPtr || outSize <= 0) return 0;
  var n = Math.min(body.length, outSize);
  HEAPU8.set(body.subarray(0, n), outPtr);
  return n | 0;
});

EM_JS(int, kodi_xhr_redirected, (), {
  var s = Module.kodiXhrStore || {};
  return s.redirected ? 1 : 0;
});

EM_JS(double, kodi_xhr_content_length, (), {
  var s = Module.kodiXhrStore || {};
  return typeof s.contentLength === "number" ? s.contentLength : -1;
});

// fieldId: 0=rawHeaders 1=responseURL 2=error. The caller frees the result.
EM_JS(char*, kodi_xhr_string, (int fieldId), {
  var s = Module.kodiXhrStore || {};
  var v = fieldId === 0 ? s.rawHeaders : fieldId === 1 ? s.responseURL : s.error;
  return stringToNewUTF8(v || "");
});
// clang-format on
EM_JS_DEPS(kodi_xhr, "$stringToNewUTF8");

std::string FetchString(int fieldId)
{
  char* value = kodi_xhr_string(fieldId);
  std::string out(value ? value : "");
  std::free(value);
  return out;
}

void ParseResponseHeaders(const std::string& raw, CHttpHeader& out)
{
  out.Clear();
  size_t pos = 0;
  while (pos < raw.size())
  {
    size_t nl = raw.find('\n', pos);
    std::string line = raw.substr(pos, nl == std::string::npos ? std::string::npos : nl - pos);
    pos = nl == std::string::npos ? raw.size() : nl + 1;
    if (!line.empty() && line.back() == '\r')
      line.pop_back();
    if (!line.empty())
      out.Parse(line + "\r\n");
  }
  // CHttpHeader parses a line only once the next one arrives.
  out.Parse("\r\n");
}

// Content-Range: bytes <first>-<last>/<length>
bool ParseContentRange(const std::string& value, int64_t& first, int64_t& last, int64_t& length)
{
  long long f = 0;
  long long l = 0;
  long long len = 0;
  if (std::sscanf(value.c_str(), "bytes %lld-%lld/%lld", &f, &l, &len) != 3 || f < 0 || l < f ||
      len <= l)
    return false;
  first = f;
  last = l;
  length = len;
  return true;
}

bool IsHttp(const CURL& url)
{
  return url.IsProtocol("http") || url.IsProtocol("https");
}

bool HasHeader(const std::map<std::string, std::string>& headers, const char* name)
{
  return std::any_of(headers.begin(), headers.end(),
                     [name](const auto& kv) { return StringUtils::EqualsNoCase(kv.first, name); });
}
} // namespace

struct CXhrFile::Request
{
  std::string url;
  std::string headers;
  std::string userAgent;
  std::string referer;
  int timeout{DEFAULT_REQUEST_TIMEOUT_SECONDS};
};

struct CXhrFile::Response
{
  int status{0};
  std::string error;
  std::string rawHeaders;
  std::string responseUrl;
  bool redirected{false};
  std::unique_ptr<char[]> body;
  size_t bodySize{0};
  // Set for a 206 answering a range: first is the offset of body[0] in the
  // resource, total the resource length, or -1 when Content-Range is hidden.
  bool partial{false};
  int64_t first{0};
  int64_t total{0};
};

CXhrFile::CXhrFile() = default;

CXhrFile::~CXhrFile()
{
  Close();
}

CXhrFile::Request CXhrFile::BuildRequest() const
{
  Request request;
  request.url = m_url;
  for (const auto& [name, value] : m_requestHeaders)
  {
    if (!name.empty())
      request.headers += name + ':' + value + '\n';
  }
  if (!m_cookie.empty())
    request.headers += "Cookie:" + m_cookie + "\n";
  if (!m_acceptEncoding.empty())
    request.headers += "Accept-Encoding:" + m_acceptEncoding + "\n";
  if (!m_acceptCharset.empty())
    request.headers += "Accept-Charset:" + m_acceptCharset + "\n";
  request.userAgent = m_userAgent;
  request.referer = m_referer;
  request.timeout = m_timeout > 0 ? m_timeout : DEFAULT_REQUEST_TIMEOUT_SECONDS;
  return request;
}

// Applies Kodi's protocol options as CCurlFile does and returns the URL without
// them. Options only libcurl can honour, such as verifypeer, are dropped.
std::string CXhrFile::ApplyProtocolOptions(const CURL& url)
{
  CURL target(url);
  std::map<std::string, std::string> options;
  target.GetProtocolOptions(options);
  for (const auto& [option, value] : options)
  {
    const std::string name = StringUtils::ToLower(option);
    if (name == "referer")
      m_referer = value;
    else if (name == "user-agent")
      m_userAgent = value;
    else if (name == "cookie")
      m_cookie = value;
    else if (name == "acceptencoding" || name == "encoding")
      m_acceptEncoding = value;
    else if (name == "accept-charset")
      m_acceptCharset = value;
    else if (name == "customrequest")
      m_customRequest = value;
    else if (name == "redirect-limit")
      m_noRedirect = value == "0";
    else if (name == "postdata")
    {
      m_postData = Base64::Decode(value);
      m_postDataSet = true;
    }
    else if (name == "auth" || name == "noshout" || name == "seekable" || name == "sslcipherlist" ||
             name == "connection-timeout" || name == "failonerror" || name == "verifypeer")
      continue;
    else if (!name.empty())
      m_requestHeaders[name[0] == '!' ? option.substr(1) : option] = value;
  }
  target.SetProtocolOptions("");
  return target.Get();
}

// Fetches [rangeStart, rangeStart + rangeLength) of the resource, or all of it
// when rangeLength is 0. A server that ignores the range answers 200 with the
// whole body.
CXhrFile::Response CXhrFile::Fetch(const Request& request,
                                   const std::string& method,
                                   const std::string& body,
                                   int64_t rangeStart,
                                   int64_t rangeLength) const
{
  Response out;
  if (m_cancelled)
  {
    out.error = "cancelled";
    return out;
  }

  std::string headers = request.headers;
  if (rangeLength > 0)
    headers += StringUtils::Format("Range:bytes={}-{}\n", rangeStart, rangeStart + rangeLength - 1);

  out.status =
      kodi_xhr_request(method.c_str(), request.url.c_str(), body.empty() ? nullptr : body.data(),
                       static_cast<int>(body.size()), headers.empty() ? nullptr : headers.c_str(),
                       request.userAgent.c_str(), request.referer.c_str(), request.timeout, 1);
  if (out.status <= 0)
  {
    out.error = FetchString(2);
    return out;
  }

  out.rawHeaders = FetchString(0);
  out.responseUrl = FetchString(1);
  out.redirected = kodi_xhr_redirected() != 0;
  const int bodySize = kodi_xhr_body_size();
  if (bodySize > MAX_EAGER_RESPONSE_BYTES)
  {
    kodi_xhr_copy_body(nullptr, 0); // drops the body
    out.status = 0;
    out.error = StringUtils::Format("refusing to keep a {:.1f} MiB response in the WASM heap",
                                    static_cast<double>(bodySize) / (1024.0 * 1024.0));
    return out;
  }
  if (bodySize > 0)
  {
    out.body.reset(new char[bodySize]);
    out.bodySize = static_cast<size_t>(bodySize);
    kodi_xhr_copy_body(out.body.get(), bodySize);
  }
  if (out.status == 206)
  {
    CHttpHeader header;
    ParseResponseHeaders(out.rawHeaders, header);
    const std::string contentRange = header.GetValue("content-range");
    int64_t last = 0;
    if (!contentRange.empty())
    {
      out.partial = ParseContentRange(contentRange, out.first, last, out.total) &&
                    last - out.first + 1 == static_cast<int64_t>(out.bodySize);
    }
    else if (rangeLength > 0)
    {
      // A cross-origin server that does not expose Content-Range still answers
      // the range that was asked for.
      out.partial = true;
      out.first = rangeStart;
      out.total = -1;
    }
  }
  return out;
}

bool CXhrFile::Open(const CURL& url)
{
  Close();

  if (!IsHttp(url))
  {
    CLog::LogF(LOGERROR, "unsupported protocol in <{}>", url.GetRedacted());
    return false;
  }

  m_url = ApplyProtocolOptions(url);
  const std::string method =
      !m_customRequest.empty() ? m_customRequest : (m_postDataSet ? "POST" : "GET");
  const std::string& body = m_postDataSet ? m_postData : std::string();
  Request request = BuildRequest();
  // libcurl posts a form by default; XHR would send the bytes with no type.
  if (m_postDataSet && !HasHeader(m_requestHeaders, "Content-Type"))
    request.headers += "Content-Type:application/x-www-form-urlencoded\n";

  // Only a plain GET is read in windows; other requests are taken whole.
  const bool windowed = method == "GET" && !HasHeader(m_requestHeaders, "Range");
  Response response = Fetch(request, method, body, 0, windowed ? RANGE_WINDOW_BYTES : 0);
  // Content-Length, which CORS always exposes, gives the length that a hidden
  // Content-Range would have.
  if (windowed && response.partial && response.total < 0)
  {
    const int status = Head(CURL(m_url));
    const double length = kodi_xhr_content_length();
    if (status > 0 && status < 400 && length >= static_cast<double>(response.bodySize))
      response.total = static_cast<int64_t>(length);
  }
  // An empty resource answers a range request with 416.
  if (windowed && (response.status == 416 ||
                   (response.status == 206 &&
                    (!response.partial || response.first != 0 || response.total < 0))))
    response = Fetch(request, method, body, 0, 0);
  m_httpResponse = response.status;

  if (m_httpResponse <= 0)
  {
    CLog::LogF(LOGERROR, "{} <{}> failed: {}", method, url.GetRedacted(), response.error);
    return false;
  }

  // XHR follows redirects before any header is visible. A caller that asked not
  // to gets the target as REDIRECT_URL without the target's headers, which it
  // would take for the redirect's: a repository checksum from the mirror.
  if (m_noRedirect && response.redirected)
  {
    m_redirectUrl = response.responseUrl;
    response.rawHeaders.clear();
  }
  ParseResponseHeaders(response.rawHeaders, m_httpHeader);
  if (!response.responseUrl.empty())
    m_url = response.responseUrl;

  m_rangeSupported = windowed && response.partial;
  m_fileSize = m_rangeSupported ? response.total : static_cast<int64_t>(response.bodySize);
  m_filePos = 0;
  m_windowStart = 0;
  m_windowSize = response.bodySize;
  m_window = std::move(response.body);

  if (m_httpResponse >= 400)
  {
    CLog::LogF(LOGERROR, "<{}> failed with code {}", url.GetRedacted(), m_httpResponse);
    return false;
  }

  m_opened = true;
  return true;
}

// Returns the HTTP status, or 0 without a response. The headers are kept for a
// successful response only.
int CXhrFile::Head(const CURL& url)
{
  if (!IsHttp(url))
    return 0;
  const std::string target = ApplyProtocolOptions(url);
  const Request request = BuildRequest();
  const int status =
      kodi_xhr_request("HEAD", target.c_str(), nullptr, 0,
                       request.headers.empty() ? nullptr : request.headers.c_str(),
                       request.userAgent.c_str(), request.referer.c_str(), request.timeout, 0);
  if (status > 0 && status < 400)
    ParseResponseHeaders(FetchString(0), m_httpHeader);
  return status;
}

bool CXhrFile::Exists(const CURL& url)
{
  return Stat(url, nullptr) == 0;
}

int CXhrFile::Stat(const CURL& url, struct __stat64* buffer)
{
  if (m_opened)
  {
    if (buffer)
    {
      *buffer = {};
      buffer->st_size = m_fileSize;
      buffer->st_mode = _S_IFREG;
    }
    return 0;
  }

  int64_t size = 0;
  const int status = Head(url);
  if (status > 0 && status < 400)
  {
    const double contentLength = kodi_xhr_content_length();
    size = contentLength >= 0.0 ? static_cast<int64_t>(contentLength) : 0;
  }
  // Some servers reject HEAD; CCurlFile retries those with a GET too.
  else if (status >= 400 && status != 404 && Open(url))
  {
    size = m_fileSize;
    // The response headers stay for the mime type the callers read next.
    DropWindow();
    m_opened = false;
  }
  else
  {
    errno = ENOENT;
    return -1;
  }

  if (buffer)
  {
    *buffer = {};
    buffer->st_size = size;
    // consider html files directories, as CCurlFile does
    buffer->st_mode =
        m_httpHeader.GetMimeType().find("text/html") != std::string::npos ? _S_IFDIR : _S_IFREG;
  }
  return 0;
}

void CXhrFile::DropWindow()
{
  m_window.reset();
  m_windowSize = 0;
  m_windowStart = 0;
}

// Moves the window so that it starts at `position` and covers at least `want`
// bytes.
bool CXhrFile::FillWindow(int64_t position, size_t want)
{
  if (!m_rangeSupported)
    return false;

  const int64_t length = std::max<int64_t>(static_cast<int64_t>(want), RANGE_WINDOW_BYTES);
  Response response = Fetch(BuildRequest(), "GET", {}, position, length);
  const bool usable =
      response.status == 206 ? response.partial : response.status > 0 && response.status < 400;
  const int64_t first = response.partial ? response.first : 0;
  if (!usable || position < first || position >= first + static_cast<int64_t>(response.bodySize))
  {
    CLog::LogF(LOGERROR, "range {}+{} of <{}> failed: HTTP {} {}", position, length,
               CURL::GetRedacted(m_url), response.status, response.error);
    return false;
  }

  if (!response.partial)
  {
    m_fileSize = static_cast<int64_t>(response.bodySize);
    m_rangeSupported = false;
  }
  else if (response.total >= 0)
  {
    m_fileSize = response.total;
  }
  m_window = std::move(response.body);
  m_windowSize = response.bodySize;
  m_windowStart = first;
  return true;
}

ssize_t CXhrFile::Read(void* lpBuf, size_t uiBufSize)
{
  if (!m_opened || !lpBuf || uiBufSize == 0)
    return 0;
  if (m_filePos >= m_fileSize)
    return 0;
  if (m_filePos < m_windowStart || m_filePos >= m_windowStart + static_cast<int64_t>(m_windowSize))
  {
    if (!FillWindow(m_filePos, uiBufSize))
      return -1;
  }
  const size_t offset = static_cast<size_t>(m_filePos - m_windowStart);
  const size_t n = std::min(uiBufSize, m_windowSize - offset);
  std::memcpy(lpBuf, m_window.get() + offset, n);
  m_filePos += static_cast<int64_t>(n);
  return static_cast<ssize_t>(n);
}

IFile::ReadLineResult CXhrFile::ReadLine(char* buffer, std::size_t bufferSize)
{
  if (!m_opened || !buffer || bufferSize == 0)
    return {ReadLineResult::FAILURE, 0};

  size_t n = 0;
  bool sawNewline = false;
  while (n < bufferSize - 1 && !sawNewline && m_filePos < m_fileSize)
  {
    if (m_filePos < m_windowStart ||
        m_filePos >= m_windowStart + static_cast<int64_t>(m_windowSize))
    {
      if (!FillWindow(m_filePos, 1))
      {
        if (n == 0)
          return {ReadLineResult::FAILURE, 0};
        break;
      }
    }
    const size_t offset = static_cast<size_t>(m_filePos - m_windowStart);
    const char* src = m_window.get() + offset;
    const size_t scan = std::min(m_windowSize - offset, bufferSize - 1 - n);
    const char* nl = static_cast<const char*>(std::memchr(src, '\n', scan));
    const size_t take = nl ? static_cast<size_t>(nl - src) + 1 : scan;
    std::memcpy(buffer + n, src, take);
    n += take;
    m_filePos += static_cast<int64_t>(take);
    sawNewline = nl != nullptr;
  }
  buffer[n] = '\0';
  if (n == 0)
    return {ReadLineResult::FAILURE, 0};
  if (!sawNewline && n == bufferSize - 1 && m_filePos < m_fileSize)
    return {ReadLineResult::TRUNCATED, n};
  return {ReadLineResult::OK, n};
}

int64_t CXhrFile::Seek(int64_t iFilePosition, int iWhence)
{
  if (!m_opened)
    return -1;
  int64_t next = m_filePos;
  switch (iWhence)
  {
    case SEEK_SET:
      next = iFilePosition;
      break;
    case SEEK_CUR:
      next += iFilePosition;
      break;
    case SEEK_END:
      next = m_fileSize + iFilePosition;
      break;
    default:
      return -1;
  }
  if (next < 0 || next > m_fileSize)
    return -1;
  m_filePos = next;
  return m_filePos;
}

int64_t CXhrFile::GetPosition()
{
  return m_opened ? m_filePos : 0;
}

int64_t CXhrFile::GetLength()
{
  return m_opened ? m_fileSize : 0;
}

void CXhrFile::Close()
{
  DropWindow();
  m_httpHeader.Clear();
  m_fileSize = 0;
  m_filePos = 0;
  m_rangeSupported = false;
  m_opened = false;
  m_httpResponse = -1;
  m_url.clear();
  m_redirectUrl.clear();
}

const std::string CXhrFile::GetProperty(XFILE::FileProperty type, const std::string& name) const
{
  switch (type)
  {
    case FileProperty::RESPONSE_PROTOCOL:
      return m_httpHeader.GetProtoLine();
    case FileProperty::RESPONSE_HEADER:
      return m_httpHeader.GetValue(name);
    case FileProperty::CONTENT_TYPE:
      return m_httpHeader.GetValue("content-type");
    case FileProperty::CONTENT_CHARSET:
      return m_httpHeader.GetCharset();
    case FileProperty::MIME_TYPE:
      return m_httpHeader.GetMimeType();
    case FileProperty::EFFECTIVE_URL:
      return m_url;
    case FileProperty::REDIRECT_URL:
      return m_redirectUrl;
    default:
      return {};
  }
}

const std::vector<std::string> CXhrFile::GetPropertyValues(XFILE::FileProperty type,
                                                           const std::string& name) const
{
  if (type == FileProperty::RESPONSE_HEADER)
    return m_httpHeader.GetValues(name);
  std::string value = GetProperty(type, name);
  if (value.empty())
    return {};
  return {std::move(value)};
}

int CXhrFile::IoControl(IOControl request, void* param)
{
  if (request == IOControl::SEEK_POSSIBLE)
    return 1;
  return IFile::IoControl(request, param);
}

bool CXhrFile::Get(const std::string& url, std::string& body)
{
  m_postDataSet = false;
  if (!Open(CURL(url)))
    return false;
  return ReadData(body);
}

bool CXhrFile::Post(const std::string& url, const std::string& postData, std::string& body)
{
  m_postData = postData;
  m_postDataSet = true;
  const bool opened = Open(CURL(url));
  m_postDataSet = false;
  m_postData.clear();
  return opened && ReadData(body);
}

bool CXhrFile::ReadData(std::string& body)
{
  body.clear();
  if (!m_opened)
    return false;
  while (m_filePos < m_fileSize)
  {
    if (m_filePos < m_windowStart ||
        m_filePos >= m_windowStart + static_cast<int64_t>(m_windowSize))
    {
      if (!FillWindow(m_filePos, static_cast<size_t>(std::min<int64_t>(
                                     m_fileSize - m_filePos, std::numeric_limits<int>::max()))))
        return false;
    }
    const size_t offset = static_cast<size_t>(m_filePos - m_windowStart);
    body.append(m_window.get() + offset, m_windowSize - offset);
    m_filePos = m_windowStart + static_cast<int64_t>(m_windowSize);
  }
  return true;
}

bool CXhrFile::IsInternet()
{
  Close();
  return Exists(CURL("https://www.msftconnecttest.com/connecttest.txt")) ||
         Exists(CURL("https://www.w3.org/"));
}

// The url is driven directly rather than through CFile, so stored credentials
// (passwords.xml) have to be applied here or an authenticated source is probed
// anonymously and answers 401.
bool CXhrFile::GetHttpHeader(const CURL& url, CHttpHeader& headers)
{
  CXhrFile probe;
  if (probe.Stat(URIUtils::AddCredentials(url), nullptr) != 0)
    return false;
  headers = probe.m_httpHeader;
  return true;
}

bool CXhrFile::GetMimeType(const CURL& url, std::string& content, const std::string& useragent)
{
  CXhrFile probe;
  probe.SetUserAgent(useragent);
  struct __stat64 buffer;
  if (probe.Stat(URIUtils::AddCredentials(url), &buffer) != 0)
  {
    content.clear();
    return false;
  }
  content = buffer.st_mode == _S_IFDIR ? "x-directory/normal" : probe.m_httpHeader.GetMimeType();
  return true;
}

bool CXhrFile::GetContentType(const CURL& url, std::string& content, const std::string& useragent)
{
  CXhrFile probe;
  probe.SetUserAgent(useragent);
  struct __stat64 buffer;
  if (probe.Stat(URIUtils::AddCredentials(url), &buffer) != 0)
  {
    content.clear();
    return false;
  }
  content = buffer.st_mode == _S_IFDIR ? "x-directory/normal"
                                       : probe.m_httpHeader.GetValue("content-type");
  return true;
}
