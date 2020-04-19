/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Curl.h"

#include "ServiceBroker.h"
#include "commons/ilog.h" // for LOGERROR, LOGCURL, LOGDEBUG, LOG_LEVEL_DEBUG
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <cassert>
#include <map> // for map
#include <memory> // for unique_ptr
#include <string> // for char_traits
#include <system_error> // for error_code, error_category
#include <type_traits> // for move
#include <utility> // for pair
#include <vector> // for vector

// Needed for now because our CURL is not namespaced
// and is included in the pch on Windows
#define CURL CURL_HANDLE
#include <curl/curl.h>
#undef CURL

namespace
{
struct SCurlEasyErrorCategory : std::error_category
{
  const char* name() const noexcept override { return "curl_easy_error"; }

  std::string message(int ev) const override
  {
    return curl_easy_strerror(static_cast<CURLcode>(ev));
  }
};

struct SCurlMultiErrorCategory : std::error_category
{
  const char* name() const noexcept override { return "curl_multi_error"; }

  std::string message(int ev) const override
  {
    return curl_multi_strerror(static_cast<CURLMcode>(ev));
  }
};

const SCurlEasyErrorCategory g_curlEasyErrorCategory{};
const SCurlMultiErrorCategory g_curlMultiErrorCategory{};

} // namespace

// curl calls this routine to debug
extern "C" int debug_callback(
    CURL_HANDLE* handle, curl_infotype info, char* output, size_t size, void* data)
{
  if (info == CURLINFO_DATA_IN || info == CURLINFO_DATA_OUT)
    return 0;

  if (!CServiceBroker::GetLogging().CanLogComponent(LOGCURL))
    return 0;

  std::string strLine;
  strLine.append(output, size);
  std::vector<std::string> vecLines;
  StringUtils::Tokenize(strLine, vecLines, "\r\n");

  const char* infotype;
  switch (info)
  {
    case CURLINFO_TEXT:
      infotype = "TEXT: ";
      break;
    case CURLINFO_HEADER_IN:
      infotype = "HEADER_IN: ";
      break;
    case CURLINFO_HEADER_OUT:
      infotype = "HEADER_OUT: ";
      break;
    case CURLINFO_SSL_DATA_IN:
      infotype = "SSL_DATA_IN: ";
      break;
    case CURLINFO_SSL_DATA_OUT:
      infotype = "SSL_DATA_OUT: ";
      break;
    case CURLINFO_END:
      infotype = "END: ";
      break;
    default:
      infotype = "";
      break;
  }

  for (const auto& it : vecLines)
  {
    CLog::Log(LOGDEBUG, "Curl::Debug - %s%s", infotype, it.c_str());
  }
  return 0;
}

namespace KODI
{
namespace PLATFORM
{

std::error_code make_error_code(CURLcode code)
{
  return {static_cast<int>(code), g_curlEasyErrorCategory};
}

std::error_code make_error_code(CurlCode code)
{
  return {static_cast<int>(code), g_curlEasyErrorCategory};
}

std::error_code make_error_code(CURLMcode code)
{
  return {static_cast<int>(code), g_curlMultiErrorCategory};
}

std::error_code make_error_code(CurlMCode code)
{
  return {static_cast<int>(code), g_curlMultiErrorCategory};
}
struct CCurl::SSession
{
  SSession() { m_easy = curl_easy_init(); }

  ~SSession()
  {
    if (m_multi && m_easy)
      curl_multi_remove_handle(m_multi, m_easy);
    if (m_easy)
      curl_easy_cleanup(m_easy);
    if (m_multi)
      curl_multi_cleanup(m_multi);

    curl_slist_free_all(m_header);
    curl_slist_free_all(m_alias);
  }

  SSession(const SSession&) = delete;
  SSession(SSession&&) = default;

  SSession& operator=(const SSession&) = delete;
  SSession& operator=(SSession&&) = default;

  CURL_HANDLE* m_easy;
  CURLM* m_multi{nullptr};
  curl_slist* m_header{nullptr};
  curl_slist* m_alias{nullptr};
};

static constexpr int CURL_OFF = 0L;
static constexpr int CURL_ON = 1L;

CCurl::CCurl() : m_session{new SSession()}
{
}

CCurl::~CCurl() = default;

std::error_code CCurl::EasyPerform()
{
  return make_error_code(curl_easy_perform(m_session->m_easy));
}

std::error_code CCurl::EasyPause(CurlPause type)
{
  return make_error_code(curl_easy_pause(m_session->m_easy, static_cast<int>(type)));
}

std::error_code CCurl::EasyResume()
{
  return make_error_code(curl_easy_pause(m_session->m_easy, CURLPAUSE_CONT));
}

void CCurl::EasyReset()
{
  curl_easy_reset(m_session->m_easy);

  curl_easy_setopt(m_session->m_easy, CURLOPT_DEBUGFUNCTION, debug_callback);

  if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_logLevel >= LOG_LEVEL_DEBUG)
    curl_easy_setopt(m_session->m_easy, CURLOPT_VERBOSE, CURL_ON);
  else
    curl_easy_setopt(m_session->m_easy, CURLOPT_VERBOSE, CURL_OFF);
}

std::error_code CCurl::MultiPerform(int& running_handles)
{
  return make_error_code(curl_multi_perform(m_session->m_multi, &running_handles));
}

std::error_code CCurl::MultiFdSet(fd_set* read_fd_set,
                                  fd_set* write_fd_set,
                                  fd_set* exc_fd_set,
                                  int* max_fd)
{
  return make_error_code(
      curl_multi_fdset(m_session->m_multi, read_fd_set, write_fd_set, exc_fd_set, max_fd));
}

int64_t CCurl::GetContentLength(std::error_code& ec)
{
  double length;
  ec = make_error_code(
      curl_easy_getinfo(m_session->m_easy, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &length));

  return static_cast<int64_t>(length);
}

std::string CCurl::GetContentType(std::error_code& ec)
{
  char* content;

  ec = make_error_code(curl_easy_getinfo(m_session->m_easy, CURLINFO_CONTENT_TYPE, &content));

  return content != nullptr ? std::string(content) : std::string();
}

long CCurl::GetFileTime(std::error_code& ec)
{
  long time = 0;

  ec = make_error_code(curl_easy_getinfo(m_session->m_easy, CURLINFO_FILETIME, &time));

  return time;
}

long CCurl::GetResponseCode(std::error_code& ec)
{
  long response;

  ec = make_error_code(curl_easy_getinfo(m_session->m_easy, CURLINFO_RESPONSE_CODE, &response));

  return response;
}

std::string CCurl::GetEffectiveUrl(std::error_code& ec)
{
  char* url = nullptr;

  ec = make_error_code(curl_easy_getinfo(m_session->m_easy, CURLINFO_EFFECTIVE_URL, &url));

  return url != nullptr ? std::string(url) : std::string();
}

std::string CCurl::GetRedirectURL(std::error_code& ec)
{
  char* url = nullptr;

  ec = make_error_code(curl_easy_getinfo(m_session->m_easy, CURLINFO_REDIRECT_URL, &url));

  return url != nullptr ? std::string(url) : std::string();
}

std::string CCurl::GetCookies(std::error_code& ec)
{
  std::string cookiesStr;
  struct curl_slist* curlCookies;

  ec = make_error_code(curl_easy_getinfo(m_session->m_easy, CURLINFO_COOKIELIST, &curlCookies));
  if (ec)
    return std::string();

  // iterate over each cookie and format it into an RFC 2109 formatted Set-Cookie string
  auto curlCookieIter = curlCookies;
  while (curlCookieIter)
  {
    // tokenize the CURL cookie string
    auto valuesVec = StringUtils::Split(curlCookieIter->data, "\t");

    // ensure the length is valid
    if (valuesVec.size() < 7)
    {
      CLog::Log(LOGERROR, "CCurlFile::GetCookies - invalid cookie: '%s'", curlCookieIter->data);
      curlCookieIter = curlCookieIter->next;
      continue;
    }

    // create a http-header formatted cookie string
    std::string cookieStr =
        valuesVec[5] + "=" + valuesVec[6] + "; path=" + valuesVec[2] + "; domain=" + valuesVec[0];

    // append this cookie to the string containing all cookies
    if (!cookiesStr.empty())
      cookiesStr += "\n";
    cookiesStr += cookieStr;

    // move on to the next cookie
    curlCookieIter = curlCookieIter->next;
  }

  // free the curl cookies
  curl_slist_free_all(curlCookies);

  return cookiesStr;
}

double CCurl::GetDownloadSpeed(std::error_code& ec)
{
  double speed = 0.0;
#if LIBCURL_VERSION_NUM >= 0x073a00 // 0.7.58.0
  ec = make_error_code(curl_easy_getinfo(m_session->m_easy, CURLINFO_SPEED_DOWNLOAD, &speed));
#else
  double time = 0.0, size = 0.0;
  ec = make_error_code(curl_easy_getinfo(m_session->m_easy, CURLINFO_TOTAL_TIME, &time));
  if (!ec)
  {
    ec = make_error_code(curl_easy_getinfo(m_session->m_easy, CURLINFO_SIZE_DOWNLOAD, &size));
    if (!ec && time > 0.0)
      speed = size / time;
  }
#endif

  return speed;
}

long CCurl::GetTimeout(std::error_code& ec)
{
  long timeout = 0;
  ec = make_error_code(curl_multi_timeout(m_session->m_multi, &timeout));

  return timeout;
}

std::error_code CCurl::GetMultiMessage(int& messages)
{
  // Curl docs says msg->msg is always DONE so we can safely ignore it
  // We don't use the whatever union member so we can ignore that as well.
  // We only use one handle per multi so we can safely ignore the handle param as well.

  auto msg = curl_multi_info_read(m_session->m_multi, &messages);
  if (!msg || msg->msg == CURLMSG_DONE)
    return make_error_code(CurlMCode::OK);

  return make_error_code(msg->data.result);
}

void CCurl::UseMulti()
{
  if (!m_session->m_multi)
    m_session->m_multi = curl_multi_init();

  curl_multi_add_handle(m_session->m_multi, m_session->m_easy);
}

void CCurl::RemoveMulti()
{
  curl_multi_remove_handle(m_session->m_multi, m_session->m_easy);
}

bool CCurl::IsMulti()
{
  return m_session->m_multi != nullptr;
}

std::error_code CCurl::DisableBody()
{
  return make_error_code(curl_easy_setopt(m_session->m_easy, CURLOPT_NOBODY, 1));
}

std::error_code CCurl::EnableBody()
{
  return make_error_code(curl_easy_setopt(m_session->m_easy, CURLOPT_NOBODY, 0));
}

std::error_code CCurl::DisableFtpEPSV()
{
  return make_error_code(curl_easy_setopt(m_session->m_easy, CURLOPT_FTP_USE_EPSV, 0));
}

std::error_code CCurl::DisableSignals()
{
  return make_error_code(curl_easy_setopt(m_session->m_easy, CURLOPT_NOSIGNAL, CURL_ON));
}

std::error_code CCurl::DisableSslVerifypeer()
{
  return make_error_code(curl_easy_setopt(m_session->m_easy, CURLOPT_SSL_VERIFYPEER, CURL_OFF));
}

std::error_code CCurl::DisableTransferText()
{
  return make_error_code(curl_easy_setopt(m_session->m_easy, CURLOPT_TRANSFERTEXT, CURL_OFF));
}

std::error_code CCurl::DisableIPV6()
{
  return make_error_code(curl_easy_setopt(m_session->m_easy, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4));
}

std::error_code CCurl::WriteData(void* data)
{
  return make_error_code(curl_easy_setopt(m_session->m_easy, CURLOPT_WRITEDATA, data));
}

std::error_code CCurl::FlushCookies()
{
  return make_error_code(curl_easy_setopt(m_session->m_easy, CURLOPT_COOKIELIST, "FLUSH"));
}

std::error_code CCurl::FailOnError()
{
  return make_error_code(curl_easy_setopt(m_session->m_easy, CURLOPT_FAILONERROR, 1));
}

std::error_code CCurl::IgnoreContentLength()
{
  return make_error_code(curl_easy_setopt(m_session->m_easy, CURLOPT_IGNORE_CONTENT_LENGTH, 1));
}

std::error_code CCurl::EnableFileTime()
{
  return make_error_code(curl_easy_setopt(m_session->m_easy, CURLOPT_FILETIME, 1));
}

std::error_code CCurl::EnableUpload()
{
  return make_error_code(curl_easy_setopt(m_session->m_easy, CURLOPT_UPLOAD, 1));
}

std::error_code CCurl::EnableCookies()
{
  return make_error_code(curl_easy_setopt(m_session->m_easy, CURLOPT_COOKIEFILE, ""));
}

std::error_code CCurl::FollowRedirects(int max)
{
  int enable = max != 0 ? 1 : 0;

  auto ec = make_error_code(curl_easy_setopt(m_session->m_easy, CURLOPT_FOLLOWLOCATION, enable));
  if (ec)
    return ec;

  return make_error_code(curl_easy_setopt(m_session->m_easy, CURLOPT_MAXREDIRS, max));
}

std::error_code CCurl::SetTimeout(int timeout)
{
  return make_error_code(curl_easy_setopt(m_session->m_easy, CURLOPT_TIMEOUT, timeout));
}

std::error_code CCurl::SetConnectTimeout(int timeout)
{
  return make_error_code(curl_easy_setopt(m_session->m_easy, CURLOPT_CONNECTTIMEOUT, timeout));
}

std::error_code CCurl::SetFtpFileMethod(CurlFtpMethod method)
{
  return make_error_code(curl_easy_setopt(m_session->m_easy, CURLOPT_FTP_FILEMETHOD,
                                          static_cast<curl_ftpmethod>(method)));
}

std::error_code CCurl::SetProgressCallback(ProgressCallback cb)
{
  std::error_code ec;
#if LIBCURL_VERSION_NUM >= 0x072000 // 0.7.32
  ec = make_error_code(curl_easy_setopt(m_session->m_easy, CURLOPT_XFERINFOFUNCTION, cb));
#else
  ec = make_error_code(curl_easy_setopt(m_session->m_easy, CURLOPT_PROGRESSFUNCTION, cb));
#endif
  if (ec)
    return ec;

  return make_error_code(curl_easy_setopt(m_session->m_easy, CURLOPT_NOPROGRESS, 0));
}

std::error_code CCurl::SetCallbacks(void* state,
                                    ReadWriteCallback write,
                                    ReadWriteCallback read,
                                    HeaderCallback header)
{
  std::error_code ec;

  ec = make_error_code(curl_easy_setopt(m_session->m_easy, CURLOPT_WRITEDATA, state));
  if (ec)
    return ec;

  ec = make_error_code(curl_easy_setopt(m_session->m_easy, CURLOPT_WRITEFUNCTION, write));
  if (ec)
    return ec;

  ec = make_error_code(curl_easy_setopt(m_session->m_easy, CURLOPT_READDATA, state));
  if (ec)
    return ec;

  ec = make_error_code(curl_easy_setopt(m_session->m_easy, CURLOPT_READFUNCTION, read));
  if (ec)
    return ec;

  ec = make_error_code(curl_easy_setopt(m_session->m_easy, CURLOPT_HEADERDATA, state));
  if (ec)
    return ec;

  ec = make_error_code(curl_easy_setopt(m_session->m_easy, CURLOPT_HEADERFUNCTION, header));
  if (ec)
    return ec;

  return make_error_code(curl_easy_setopt(m_session->m_easy, CURLOPT_HEADER, CURL_OFF));
}

std::error_code CCurl::SetUsernamePassword(const std::string& user, const std::string& pass)
{
  assert(!user.empty() && !pass.empty());

  auto up = user + ":" + pass;
  return make_error_code(curl_easy_setopt(m_session->m_easy, CURLOPT_USERPWD, up.c_str()));
}

std::error_code CCurl::SetCookies(const std::string& cookies)
{
  return make_error_code(curl_easy_setopt(m_session->m_easy, CURLOPT_COOKIE, cookies.c_str()));
}

std::error_code CCurl::SetAlias(const std::string& alias)
{
  if (m_session->m_alias)
  {
    curl_slist_free_all(m_session->m_alias);
    m_session->m_alias = nullptr;
  }

  m_session->m_alias = curl_slist_append(m_session->m_alias, alias.c_str());

  return make_error_code(
      curl_easy_setopt(m_session->m_easy, CURLOPT_HTTP200ALIASES, m_session->m_alias));
}

std::error_code CCurl::SetUrl(const std::string& url)
{
  return make_error_code(curl_easy_setopt(m_session->m_easy, CURLOPT_URL, url.c_str()));
}

std::error_code CCurl::SetPostData(const std::string& data)
{
  auto ec = make_error_code(curl_easy_setopt(m_session->m_easy, CURLOPT_POST, 1));
  if (ec)
    return ec;

  ec = make_error_code(curl_easy_setopt(m_session->m_easy, CURLOPT_POSTFIELDSIZE, data.length()));
  if (ec)
    return ec;

  return make_error_code(curl_easy_setopt(m_session->m_easy, CURLOPT_POSTFIELDS, data.c_str()));
}

std::error_code CCurl::SetOrClearReferer(const std::string& referer)
{
  if (!referer.empty())
    return make_error_code(curl_easy_setopt(m_session->m_easy, CURLOPT_REFERER, referer.c_str()));

  auto ec = make_error_code(curl_easy_setopt(m_session->m_easy, CURLOPT_REFERER, NULL));
  if (ec)
    return ec;

  return make_error_code(curl_easy_setopt(m_session->m_easy, CURLOPT_AUTOREFERER, CURL_ON));
}

std::error_code CCurl::SetFtpAuth(CurlFtpAuth auth)
{
  auto ec = make_error_code(curl_easy_setopt(m_session->m_easy, CURLOPT_USE_SSL, CURLUSESSL_TRY));
  if (ec)
    return ec;

  return make_error_code(
      curl_easy_setopt(m_session->m_easy, CURLOPT_FTPSSLAUTH, static_cast<curl_ftpauth>(auth)));
}

std::error_code CCurl::SetHttpAuth(CurlHttpAuth auth)
{
  return make_error_code(
      curl_easy_setopt(m_session->m_easy, CURLOPT_HTTPAUTH, static_cast<unsigned long>(auth)));
}
std::error_code CCurl::SetHttpVersion(CurlHttpVersion version)
{
  return make_error_code(
      curl_easy_setopt(m_session->m_easy, CURLOPT_HTTP_VERSION, static_cast<int>(version)));
}

std::error_code CCurl::SetFtpPort(const std::string& port)
{
  return make_error_code(
      curl_easy_setopt(m_session->m_easy, CURLOPT_FTPPORT, port.empty() ? nullptr : port.c_str()));
}

std::error_code CCurl::SetSkipFtpPassiveIp(bool skip)
{
  return make_error_code(
      curl_easy_setopt(m_session->m_easy, CURLOPT_FTP_SKIP_PASV_IP, skip ? 1 : 0));
}

std::error_code CCurl::SetAcceptEncoding(const std::string& encoding)
{
  return make_error_code(
      curl_easy_setopt(m_session->m_easy, CURLOPT_ACCEPT_ENCODING, encoding.c_str()));
}

std::error_code CCurl::SetUserAgent(const std::string& useragent)
{
  return make_error_code(curl_easy_setopt(m_session->m_easy, CURLOPT_USERAGENT, useragent.c_str()));
}

std::error_code CCurl::SetLowSpeedLimit(int bps, int time)
{
  auto ec = make_error_code(curl_easy_setopt(m_session->m_easy, CURLOPT_LOW_SPEED_LIMIT, bps));
  if (ec)
    return ec;

  return make_error_code(curl_easy_setopt(m_session->m_easy, CURLOPT_LOW_SPEED_TIME, time));
}

std::error_code CCurl::SetCipherList(const std::string& ciphers)
{
  return make_error_code(
      curl_easy_setopt(m_session->m_easy, CURLOPT_SSL_CIPHER_LIST, ciphers.c_str()));
}

std::error_code CCurl::SetProxyOptions(CurlProxy proxy,
                                       const std::string& host,
                                       int port,
                                       const std::string& user,
                                       const std::string& password)
{
  auto ec = make_error_code(
      curl_easy_setopt(m_session->m_easy, CURLOPT_PROXYTYPE, static_cast<curl_proxytype>(proxy)));
  if (ec)
    return ec;

  const auto hostport = StringUtils::Format("%s:%d", host, port);
  ec = make_error_code(curl_easy_setopt(m_session->m_easy, CURLOPT_PROXY, hostport.c_str()));
  if (ec)
    return ec;

  const auto up = user + ":" + password;
  return make_error_code(curl_easy_setopt(m_session->m_easy, CURLOPT_PROXYUSERPWD, up.c_str()));
}

std::error_code CCurl::SetCustomRequest(const std::string& request)
{
  return make_error_code(
      curl_easy_setopt(m_session->m_easy, CURLOPT_CUSTOMREQUEST, request.c_str()));
}

std::error_code CCurl::SetResumeRange(const char* start, int64_t end)
{
  auto ec = make_error_code(curl_easy_setopt(m_session->m_easy, CURLOPT_RANGE, start));
  if (ec)
    return ec;

  return make_error_code(curl_easy_setopt(m_session->m_easy, CURLOPT_RESUME_FROM_LARGE, end));
}

std::error_code CCurl::SetHeaders(const std::map<std::string, std::string>& headers)
{
  if (m_session->m_header)
  {
    curl_slist_free_all(m_session->m_header);
    m_session->m_header = nullptr;
  }

  for (const auto& it : headers)
  {
    m_session->m_header =
        curl_slist_append(m_session->m_header, std::string(it.first + ":" + it.second).c_str());
  }

  // add user defined headers
  return make_error_code(
      curl_easy_setopt(m_session->m_easy, CURLOPT_HTTPHEADER, m_session->m_header));
}
std::error_code CCurl::AddHeader(const std::string& header)
{
  if (m_session->m_header)
    curl_slist_append(m_session->m_header, header.c_str());
  else
    m_session->m_header = curl_slist_append(nullptr, header.c_str());

  return make_error_code(
      curl_easy_setopt(m_session->m_easy, CURLOPT_HTTPHEADER, m_session->m_header));
}

} // namespace PLATFORM
} // namespace KODI
