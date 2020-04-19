/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CurlFile.h"

#include "File.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "Util.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "threads/SystemClock.h"
#include "utils/Base64.h"
#include "utils/XTimeUtils.h"

#include <algorithm>
#include <cassert>
#include <climits>
#include <system_error>
#include <vector>

#ifdef TARGET_POSIX
#include "platform/posix/ConvUtils.h"
#include "platform/posix/XFileUtils.h"

#include <errno.h>
#include <inttypes.h>
#endif

#include "ShoutcastFile.h"
#include "platform/Curl.h"
#include "utils/CharsetConverter.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

using namespace XFILE;
using namespace KODI::PLATFORM;

#define FITS_INT(a) (((a) <= INT_MAX) && ((a) >= INT_MIN))

CurlProxy ProxyType2CurlProxyType(CCurlFile::ProxyType proxy) noexcept
{
  switch (proxy)
  {
    case CCurlFile::ProxyType::HTTP:
      return CurlProxy::HTTP;
    case CCurlFile::ProxyType::SOCKS4:
      return CurlProxy::SOCKS4;
    case CCurlFile::ProxyType::SOCKS4A:
      return CurlProxy::SOCKS4A;
    case CCurlFile::ProxyType::SOCKS5:
      return CurlProxy::SOCKS5;
    case CCurlFile::ProxyType::SOCKS5_REMOTE:
      return CurlProxy::SOCKS5_HOSTNAME;
    default:
      return CurlProxy::HTTP;
  }
}

constexpr int FILLBUFFER_OK = 0;
constexpr int FILLBUFFER_NO_DATA = 1;
constexpr int FILLBUFFER_FAIL = 2;

/* curl calls this routine to get more data */
extern "C" size_t write_callback(char* buffer, size_t size, size_t nitems, void* userp)
{
  if (userp == NULL)
    return 0;

  CCurlFile::CReadState* state = (CCurlFile::CReadState*)userp;
  return state->WriteCallback(buffer, size, nitems);
}

extern "C" size_t read_callback(char* buffer, size_t size, size_t nitems, void* userp)
{
  if (userp == NULL)
    return 0;

  CCurlFile::CReadState* state = (CCurlFile::CReadState*)userp;
  return state->ReadCallback(buffer, size, nitems);
}

extern "C" size_t header_callback(void* ptr, size_t size, size_t nmemb, void* stream)
{
  CCurlFile::CReadState* state = (CCurlFile::CReadState*)stream;
  return state->HeaderCallback(ptr, size, nmemb);
}

using curl_off_t = long long;
/* used only by CCurlFile::Stat to bail out of unwanted transfers */
extern "C" int transfer_abort_callback(
    void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
  if (dlnow > 0)
    return 1;
  else
    return 0;
}

/* fix for silly behavior of realloc */
static inline void* realloc_simple(void* ptr, size_t size)
{
  void* ptr2 = realloc(ptr, size);
  if (ptr && !ptr2 && size > 0)
  {
    free(ptr);
    return NULL;
  }
  else
    return ptr2;
}

size_t CCurlFile::CReadState::HeaderCallback(void* ptr, size_t size, size_t nmemb)
{
  std::string inString;
  // libcurl doc says that this info is not always \0 terminated
  const char* strBuf = (const char*)ptr;
  const size_t iSize = size * nmemb;
  if (strBuf[iSize - 1] == 0)
    inString.assign(strBuf, iSize - 1); // skip last char if it's zero
  else
    inString.append(strBuf, iSize);

  m_httpheader.Parse(inString);

  return iSize;
}

size_t CCurlFile::CReadState::ReadCallback(char* buffer, size_t size, size_t nitems)
{
  constexpr size_t CURL_READFUNC_PAUSE = 0x10000001;

  if (m_fileSize == 0)
    return 0;

  if (m_filePos >= m_fileSize)
  {
    m_isPaused = true;
    return CURL_READFUNC_PAUSE;
  }

  int64_t retSize = std::min(m_fileSize - m_filePos, int64_t(nitems * size));
  memcpy(buffer, m_readBuffer + m_filePos, retSize);
  m_filePos += retSize;

  return retSize;
}

size_t CCurlFile::CReadState::WriteCallback(char* buffer, size_t size, size_t nitems)
{
  size_t amount = size * nitems;
  if (m_overflowSize)
  {
    // we have our overflow buffer - first get rid of as much as we can
    auto maxWriteable = std::min(static_cast<size_t>(m_buffer.getMaxWriteSize()), m_overflowSize);
    if (maxWriteable)
    {
      if (!m_buffer.WriteData(m_overflowBuffer, maxWriteable))
      {
        CLog::Log(LOGERROR, "CCurlFile::WriteCallback - Unable to write to buffer - what's up?");
        return 0;
      }

      if (maxWriteable < m_overflowSize)
      {
        // still have some more - copy it down
        memmove(m_overflowBuffer, m_overflowBuffer + maxWriteable, m_overflowSize - maxWriteable);
      }
      m_overflowSize -= maxWriteable;

      // Shrink memory:
      m_overflowBuffer = (char*)realloc_simple(m_overflowBuffer, m_overflowSize);
    }
  }
  // ok, now copy the data into our ring buffer
  auto maxWriteable = std::min(static_cast<size_t>(m_buffer.getMaxWriteSize()), amount);
  if (maxWriteable)
  {
    if (!m_buffer.WriteData(buffer, maxWriteable))
    {
      CLog::Log(LOGERROR,
                "CCurlFile::WriteCallback - Unable to write to buffer with %i bytes - what's up?",
                maxWriteable);
      return 0;
    }
    else
    {
      amount -= maxWriteable;
      buffer += maxWriteable;
    }
  }
  if (amount)
  {
    //! @todo Limit max. amount of the overflowbuffer
    m_overflowBuffer = (char*)realloc_simple(m_overflowBuffer, amount + m_overflowSize);
    if (m_overflowBuffer == NULL)
    {
      CLog::Log(
          LOGWARNING,
          "CCurlFile::WriteCallback - Failed to grow overflow buffer from %i bytes to %i bytes",
          m_overflowSize, amount + m_overflowSize);
      return 0;
    }
    memcpy(m_overflowBuffer + m_overflowSize, buffer, amount);
    m_overflowSize += amount;
  }
  return size * nitems;
}

CCurlFile::CReadState::~CReadState()
{
  Disconnect();
}

bool CCurlFile::CReadState::Seek(int64_t pos)
{
  if (pos == m_filePos)
    return true;

  if (FITS_INT(pos - m_filePos) && m_buffer.SkipBytes((int)(pos - m_filePos)))
  {
    m_filePos = pos;
    return true;
  }

  if (pos > m_filePos && pos < m_filePos + m_bufferSize)
  {
    auto len = m_buffer.getMaxReadSize();
    m_filePos += len;
    m_buffer.SkipBytes(static_cast<int>(len));
    if (FillBuffer(m_bufferSize) != FILLBUFFER_OK)
    {
      if (!m_buffer.SkipBytes(-static_cast<int>(len)))
        CLog::Log(LOGERROR, "%s - Failed to restore position after failed fill", __FUNCTION__);
      else
        m_filePos -= len;
      return false;
    }

    if (!FITS_INT(pos - m_filePos) || !m_buffer.SkipBytes((int)(pos - m_filePos)))
    {
      CLog::Log(LOGERROR, "%s - Failed to skip to position after having filled buffer",
                __FUNCTION__);
      if (!m_buffer.SkipBytes(-static_cast<int>(len)))
        CLog::Log(LOGERROR, "%s - Failed to restore position after failed seek", __FUNCTION__);
      else
        m_filePos -= len;
      return false;
    }
    m_filePos = pos;
    return true;
  }
  return false;
}

void CCurlFile::CReadState::SetResume(void)
{
  /*
   * Explicitly set RANGE header when filepos=0 as some http servers require us to always send the range
   * request header. If we don't the server may provide different content causing seeking to fail.
   * This only affects HTTP-like items, for FTP it's a null operation.
   */
  if (m_sendRange && m_filePos == 0)
    m_curl.SetResumeRange("0-", m_filePos);
  else
  {
    m_curl.SetResumeRange(nullptr, m_filePos);
    m_sendRange = false;
  }
}

long CCurlFile::CReadState::Connect(unsigned int size)
{
  if (m_filePos != 0)
    CLog::Log(LOGDEBUG, "CurlFile::CReadState::Connect - Resume from position %" PRId64, m_filePos);

  SetResume();
  m_curl.UseMulti();

  m_bufferSize = size;
  m_buffer.Destroy();
  m_buffer.Create(size * 3);
  m_httpheader.Clear();

  // read some data in to try and obtain the length
  // maybe there's a better way to get this info??
  m_stillRunning = 1;

  std::error_code ec;
  // (Try to) fill buffer
  if (FillBuffer(1) != FILLBUFFER_OK)
  {
    // Check response code
    long response = m_curl.GetResponseCode(ec);
    if (ec)
      return -1;
    else
      return response;
  }

  auto length = m_curl.GetContentLength(ec);
  if (!ec)
  {
    if (length < 0)
      length = 0;
    m_fileSize = m_filePos + length;
  }

  long response = m_curl.GetResponseCode(ec);
  if (!ec)
    return response;

  return -1;
}

void CCurlFile::CReadState::Disconnect()
{
  m_buffer.Clear();
  free(m_overflowBuffer);
  m_overflowBuffer = NULL;
  m_overflowSize = 0;
  m_filePos = 0;
  m_fileSize = 0;
  m_bufferSize = 0;
  m_readBuffer = 0;
}


CCurlFile::~CCurlFile()
{
  Close();
  delete m_state;
  delete m_oldState;
}

CCurlFile::CCurlFile()
{
  m_state = new CReadState();
  m_acceptCharset = "UTF-8,*;q=0.8"; /* prefer UTF-8 if available */
  m_acceptencoding = "all"; /* Accept all supported encoding by default */
}

//Has to be called before Open()
void CCurlFile::SetBufferSize(unsigned int size)
{
  m_bufferSize = size;
}

void CCurlFile::Close()
{
  if (m_opened && m_forWrite && !m_inError)
    Write(NULL, 0);

  m_state->Disconnect();
  delete m_oldState;
  m_oldState = NULL;

  m_url.clear();
  m_referer.clear();
  m_cookie.clear();

  m_opened = false;
  m_forWrite = false;
  m_inError = false;
}

void CCurlFile::SetCommonOptions(CReadState* state, bool failOnError /* = true */)
{
  auto& curl = state->m_curl;

  curl.EasyReset();

  curl.SetCallbacks(static_cast<void*>(state), write_callback, read_callback, header_callback);

  // set username and password for current handle
  if (m_username.length() > 0 && m_password.length() > 0)
    curl.SetUsernamePassword(m_username, m_password);

  curl.DisableFtpEPSV(); // turn off epsv

  // Allow us to follow redirects
  curl.FollowRedirects(m_redirectlimit);

  // Enable cookie engine for current handle
  curl.EnableCookies();

  // Set custom cookie if requested
  if (!m_cookie.empty())
    curl.SetCookies(m_cookie);

  curl.FlushCookies();

  // When using multiple threads you should set the CURLOPT_NOSIGNAL option to
  // TRUE for all handles. Everything will work fine except that timeouts are not
  // honored during the DNS lookup - which you can work around by building libcurl
  // with c-ares support. c-ares is a library that provides asynchronous name
  // resolves. Unfortunately, c-ares does not yet support IPv6.
  curl.DisableSignals();

  if (failOnError)
  {
    // not interested in failed requests
    curl.FailOnError();
  }

  // enable support for icecast / shoutcast streams
  curl.SetAlias("ICY 200 OK");

  if (!m_verifyPeer)
    curl.DisableSslVerifypeer();

  curl.SetUrl(m_url);
  curl.DisableTransferText();

  // setup POST data if it is set (and it may be empty)
  if (m_postdataset)
    curl.SetPostData(m_postdata);

  // setup Referer header if needed
  curl.SetOrClearReferer(m_referer);

  // setup any requested authentication
  if (!m_ftpauth.empty())
  {
    CurlFtpAuth auth{CurlFtpAuth::DEFAULT};
    if (m_ftpauth == "any")
      auth = CurlFtpAuth::DEFAULT;
    else if (m_ftpauth == "ssl")
      auth = CurlFtpAuth::SSL;
    else if (m_ftpauth == "tls")
      auth = CurlFtpAuth::TLS;
    curl.SetFtpAuth(auth);
  }

  // setup requested http authentication method
  if (!m_httpauth.empty())
  {
    if (m_httpauth == "any")
      curl.SetHttpAuth(CurlHttpAuth::ANY);
    else if (m_httpauth == "anysafe")
      curl.SetHttpAuth(CurlHttpAuth::ANYSAFE);
    else if (m_httpauth == "digest")
      curl.SetHttpAuth(CurlHttpAuth::DIGEST);
    else if (m_httpauth == "ntlm")
      curl.SetHttpAuth(CurlHttpAuth::NTLM);
  }

  // allow passive mode for ftp
  curl.SetFtpPort(m_ftpport);

  // allow curl to not use the ip address in the returned pasv response
  curl.SetSkipFtpPassiveIp(m_ftppasvip);

  // setup Accept-Encoding if requested
  if (!m_acceptencoding.empty())
    curl.SetAcceptEncoding(m_acceptencoding);

  if (!m_acceptCharset.empty())
    SetRequestHeader("Accept-Charset", m_acceptCharset);

  auto settings = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();

  if (!m_userAgent.empty())
    curl.SetUserAgent(m_userAgent);
  else /* set some default agent as shoutcast doesn't return proper stuff otherwise */
    curl.SetUserAgent(settings->m_userAgent);

  if (settings->m_curlDisableIPV6)
    curl.DisableIPV6();

  if (!m_proxyhost.empty())
  {
    curl.SetProxyOptions(ProxyType2CurlProxyType(m_proxytype), m_proxyhost, m_proxyport,
                         m_proxyuser, m_proxypassword);
  }
  if (!m_customrequest.empty())
    curl.SetCustomRequest(m_customrequest);

  if (m_connecttimeout == 0)
    m_connecttimeout =
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_curlconnecttimeout;

  // set our timeouts, we abort connection after m_timeout, and reads after no data for m_timeout seconds
  curl.SetConnectTimeout(m_connecttimeout);


  if (m_lowspeedtime == 0)
    m_lowspeedtime =
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_curllowspeedtime;

  // We abort in case we transfer less than 1byte/second
  // Set the lowspeed time very low as it seems Curl takes much longer to detect a lowspeed condition
  curl.SetLowSpeedLimit(1, m_lowspeedtime);

  curl.IgnoreContentLength();
  // Setup allowed TLS/SSL ciphers. New versions of cURL may deprecate things that are still in use.
  if (!m_cipherlist.empty())
    curl.SetCipherList(m_cipherlist);

  if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_curlDisableHTTP2)
    curl.SetHttpVersion(CurlHttpVersion::VERSION_1_1);
  else
    // enable HTTP2 support. default: CURL_HTTP_VERSION_1_1. Curl >= 7.62.0 defaults to CURL_HTTP_VERSION_2TLS
    curl.SetHttpVersion(CurlHttpVersion::VERSION_2_0);
}

void CCurlFile::SetRequestHeaders(CReadState* state)
{
  state->m_curl.SetHeaders(m_requestheaders);
}

void CCurlFile::SetCorrectHeaders(CReadState* state)
{
  CHttpHeader& h = state->m_httpheader;
  /* workaround for shoutcast server wich doesn't set content type on standard mp3 */
  if (h.GetMimeType().empty())
  {
    if (!h.GetValue("icy-notice1").empty() || !h.GetValue("icy-name").empty() ||
        !h.GetValue("icy-br").empty())
      h.AddParam("Content-Type", "audio/mpeg");
  }

  /* hack for google video */
  if (StringUtils::EqualsNoCase(h.GetMimeType(), "text/html") &&
      !h.GetValue("Content-Disposition").empty())
  {
    std::string strValue = h.GetValue("Content-Disposition");
    if (strValue.find("filename=") != std::string::npos &&
        strValue.find(".flv") != std::string::npos)
      h.AddParam("Content-Type", "video/flv");
  }
}

void CCurlFile::ParseAndCorrectUrl(CURL& url2)
{
  std::string strProtocol = url2.GetTranslatedProtocol();
  url2.SetProtocol(strProtocol);

  if (url2.IsProtocol("ftp") || url2.IsProtocol("ftps"))
  {
    // we was using url options for urls, keep the old code work and warning
    if (!url2.GetOptions().empty())
    {
      CLog::Log(LOGWARNING,
                "%s: ftp url option is deprecated, please switch to use protocol option (change "
                "'?' to '|'), url: [%s]",
                __FUNCTION__, url2.GetRedacted().c_str());
      url2.SetProtocolOptions(url2.GetOptions().substr(1));
      /* ftp has no options */
      url2.SetOptions("");
    }

    /* this is uggly, depending on from where   */
    /* we get the link it may or may not be     */
    /* url encoded. if handed from ftpdirectory */
    /* it won't be so let's handle that case    */

    std::string filename(url2.GetFileName());
    std::vector<std::string> array;

    // if server sent us the filename in non-utf8, we need send back with same encoding.
    if (url2.GetProtocolOption("utf8") == "0")
      g_charsetConverter.utf8ToStringCharset(filename);

    //! @todo create a tokenizer that doesn't skip empty's
    StringUtils::Tokenize(filename, array, "/");
    filename.clear();
    for (std::vector<std::string>::iterator it = array.begin(); it != array.end(); it++)
    {
      if (it != array.begin())
        filename += "/";

      filename += CURL::Encode(*it);
    }

    /* make sure we keep slashes */
    if (StringUtils::EndsWith(url2.GetFileName(), "/"))
      filename += "/";

    url2.SetFileName(filename);

    m_ftpauth.clear();
    if (url2.HasProtocolOption("auth"))
    {
      m_ftpauth = url2.GetProtocolOption("auth");
      StringUtils::ToLower(m_ftpauth);
      if (m_ftpauth.empty())
        m_ftpauth = "any";
    }
    m_ftpport = "";
    if (url2.HasProtocolOption("active"))
    {
      m_ftpport = url2.GetProtocolOption("active");
      if (m_ftpport.empty())
        m_ftpport = "-";
    }
    if (url2.HasProtocolOption("verifypeer"))
    {
      if (url2.GetProtocolOption("verifypeer") == "false")
        m_verifyPeer = false;
    }
    m_ftppasvip = url2.HasProtocolOption("pasvip") && url2.GetProtocolOption("pasvip") != "0";
  }
  else if (url2.IsProtocol("http") || url2.IsProtocol("https"))
  {
    std::shared_ptr<CSettings> s = CServiceBroker::GetSettingsComponent()->GetSettings();
    if (!s)
      return;

    if (!url2.IsLocalHost() && m_proxyhost.empty() &&
        s->GetBool(CSettings::SETTING_NETWORK_USEHTTPPROXY) &&
        !s->GetString(CSettings::SETTING_NETWORK_HTTPPROXYSERVER).empty() &&
        s->GetInt(CSettings::SETTING_NETWORK_HTTPPROXYPORT) > 0)
    {
      m_proxytype = (ProxyType)s->GetInt(CSettings::SETTING_NETWORK_HTTPPROXYTYPE);
      m_proxyhost = s->GetString(CSettings::SETTING_NETWORK_HTTPPROXYSERVER);
      m_proxyport = s->GetInt(CSettings::SETTING_NETWORK_HTTPPROXYPORT);
      m_proxyuser = s->GetString(CSettings::SETTING_NETWORK_HTTPPROXYUSERNAME);
      m_proxypassword = s->GetString(CSettings::SETTING_NETWORK_HTTPPROXYPASSWORD);
      CLog::Log(LOGDEBUG, "Using proxy %s, type %d", m_proxyhost.c_str(),
                static_cast<int>(ProxyType2CurlProxyType(m_proxytype)));
    }

    // get username and password
    m_username = url2.GetUserName();
    m_password = url2.GetPassWord();

    // handle any protocol options
    std::map<std::string, std::string> options;
    url2.GetProtocolOptions(options);
    if (!options.empty())
    {
      // set xbmc headers
      for (const auto& it : options)
      {
        std::string name = it.first;
        StringUtils::ToLower(name);
        const std::string& value = it.second;

        if (name == "auth")
        {
          m_httpauth = value;
          StringUtils::ToLower(m_httpauth);
          if (m_httpauth.empty())
            m_httpauth = "any";
        }
        else if (name == "referer")
          SetReferer(value);
        else if (name == "user-agent")
          SetUserAgent(value);
        else if (name == "cookie")
          SetCookie(value);
        else if (name == "acceptencoding" || name == "encoding")
          SetAcceptEncoding(value);
        else if (name == "noshout" && value == "true")
          m_skipshout = true;
        else if (name == "seekable" && value == "0")
          m_seekable = false;
        else if (name == "accept-charset")
          SetAcceptCharset(value);
        else if (name == "sslcipherlist")
          m_cipherlist = value;
        else if (name == "connection-timeout")
          m_connecttimeout = strtol(value.c_str(), NULL, 10);
        else if (name == "failonerror")
          m_failOnError = value == "true";
        else if (name == "redirect-limit")
          m_redirectlimit = strtol(value.c_str(), NULL, 10);
        else if (name == "postdata")
        {
          m_postdata = Base64::Decode(value);
          m_postdataset = true;
        }
        else if (name == "active-remote") // needed for DACP!
        {
          SetRequestHeader(it.first, value);
        }
        else if (name == "customrequest")
        {
          SetCustomRequest(value);
        }
        else if (name == "verifypeer")
        {
          if (value == "false")
            m_verifyPeer = false;
        }
        else
        {
          if (name.length() > 0 && name[0] == '!')
          {
            SetRequestHeader(it.first.substr(1), value);
            CLog::Log(
                LOGDEBUG,
                "CurlFile::ParseAndCorrectUrl() adding custom header option '%s: ***********'",
                it.first.substr(1).c_str());
          }
          else
          {
            SetRequestHeader(it.first, value);
            if (name == "authorization")
              CLog::Log(
                  LOGDEBUG,
                  "CurlFile::ParseAndCorrectUrl() adding custom header option '%s: ***********'",
                  it.first.c_str());
            else
              CLog::Log(LOGDEBUG,
                        "CurlFile::ParseAndCorrectUrl() adding custom header option '%s: %s'",
                        it.first.c_str(), value.c_str());
          }
        }
      }
    }
  }

  // Unset the protocol options to have an url without protocol options
  url2.SetProtocolOptions("");

  if (m_username.length() > 0 && m_password.length() > 0)
    m_url = url2.GetWithoutUserDetails();
  else
    m_url = url2.Get();
}

bool CCurlFile::Post(const std::string& strURL,
                     const std::string& strPostData,
                     std::string& strHTML)
{
  m_postdata = strPostData;
  m_postdataset = true;
  return Service(strURL, strHTML);
}

bool CCurlFile::Get(const std::string& strURL, std::string& strHTML)
{
  m_postdata = "";
  m_postdataset = false;
  return Service(strURL, strHTML);
}

bool CCurlFile::Service(const std::string& strURL, std::string& strHTML)
{
  const CURL pathToUrl(strURL);
  if (Open(pathToUrl))
  {
    if (ReadData(strHTML))
    {
      Close();
      return true;
    }
  }
  Close();
  return false;
}

bool CCurlFile::ReadData(std::string& strHTML)
{
  ssize_t size_read = 0ll;
  ssize_t data_size = 0ll;
  strHTML = "";
  char buffer[16384];
  while ((size_read = Read(buffer, sizeof(buffer) - 1)) > 0)
  {
    buffer[size_read] = 0;
    strHTML.append(buffer, size_read);
    data_size += size_read;
  }
  if (m_state->m_cancelled)
    return false;
  return true;
}

bool CCurlFile::Download(const std::string& strURL,
                         const std::string& strFileName,
                         ssize_t* pdwSize)
{
  CLog::Log(LOGINFO, "CCurlFile::Download - %s->%s", strURL.c_str(), strFileName.c_str());

  std::string strData;
  if (!Get(strURL, strData))
    return false;

  XFILE::CFile file;
  if (!file.OpenForWrite(strFileName, true))
  {
    CLog::Log(LOGERROR, "CCurlFile::Download - Unable to open file %s: %u", strFileName.c_str(),
              GetLastError());
    return false;
  }
  ssize_t written = 0;
  if (!strData.empty())
    written = file.Write(strData.c_str(), strData.size());

  if (pdwSize != NULL)
    *pdwSize = written > 0 ? written : 0;

  return written == static_cast<ssize_t>(strData.size());
}

// Detect whether we are "online" or not! Very simple and dirty!
bool CCurlFile::IsInternet()
{
  CURL url("http://www.msftncsi.com/ncsi.txt");
  bool found = Exists(url);
  if (!found)
  {
    // fallback
    Close();
    url.Parse("http://www.w3.org/");
    found = Exists(url);
  }
  Close();

  return found;
}

void CCurlFile::Cancel()
{
  m_state->m_cancelled = true;
  while (m_opened)
    KODI::TIME::Sleep(1);
}

void CCurlFile::Reset()
{
  m_state->m_cancelled = false;
}

void CCurlFile::SetProxy(const std::string& type,
                         const std::string& host,
                         uint16_t port,
                         const std::string& user,
                         const std::string& password)
{
  m_proxytype = ProxyType::HTTP;
  if (type == "http")
    m_proxytype = ProxyType::HTTP;
  else if (type == "socks4")
    m_proxytype = ProxyType::SOCKS4;
  else if (type == "socks4a")
    m_proxytype = ProxyType::SOCKS4A;
  else if (type == "socks5")
    m_proxytype = ProxyType::SOCKS5;
  else if (type == "socks5-remote")
    m_proxytype = ProxyType::SOCKS5_REMOTE;
  else
    CLog::Log(LOGERROR, "Invalid proxy type \"%s\"", type.c_str());
  m_proxyhost = host;
  m_proxyport = port;
  m_proxyuser = user;
  m_proxypassword = password;
}

bool CCurlFile::Open(const CURL& url)
{
  m_opened = true;
  m_seekable = true;

  CURL url2(url);
  ParseAndCorrectUrl(url2);

  std::string redactPath = CURL::GetRedacted(m_url);
  CLog::Log(LOGDEBUG, "CurlFile::Open(%p) %s", (void*)this, redactPath.c_str());

  // setup common curl options
  SetCommonOptions(
      m_state,
      m_failOnError &&
          !CServiceBroker::GetLogging().CanLogComponent(LOGCURL));
  SetRequestHeaders(m_state);
  m_state->m_sendRange = m_seekable;
  m_state->m_bRetry = m_allowRetry;

  m_httpresponse = m_state->Connect(m_bufferSize);

  if (m_httpresponse <= 0 || (m_failOnError && m_httpresponse >= 400))
  {
    std::string error;
    if (m_httpresponse >= 400 &&
        CServiceBroker::GetLogging().CanLogComponent(LOGCURL))
    {
      error.resize(4096);
      ReadString(&error[0], 4095);
    }

    CLog::Log(LOGERROR, "CCurlFile::Open failed with code %li for %s:\n%s", m_httpresponse,
              redactPath.c_str(), error.c_str());

    return false;
  }

  SetCorrectHeaders(m_state);

  // since we can't know the stream size up front if we're gzipped/deflated
  // flag the stream with an unknown file size rather than the compressed
  // file size.
  if (!m_state->m_httpheader.GetValue("Content-Encoding").empty() &&
      !StringUtils::EqualsNoCase(m_state->m_httpheader.GetValue("Content-Encoding"), "identity"))
    m_state->m_fileSize = 0;

  // check if this stream is a shoutcast stream. sometimes checking the protocol line is not enough so examine other headers as well.
  // shoutcast streams should be handled by FileShoutcast.
  if ((m_state->m_httpheader.GetProtoLine().substr(0, 3) == "ICY" ||
       !m_state->m_httpheader.GetValue("icy-notice1").empty() ||
       !m_state->m_httpheader.GetValue("icy-name").empty() ||
       !m_state->m_httpheader.GetValue("icy-br").empty()) &&
      !m_skipshout)
  {
    CLog::Log(LOGDEBUG, "CCurlFile::Open - File <%s> is a shoutcast stream. Re-opening",
              redactPath.c_str());
    throw new CRedirectException(new CShoutcastFile);
  }

  m_multisession = false;
  if (url2.IsProtocol("http") || url2.IsProtocol("https"))
  {
    m_multisession = true;
    if (m_state->m_httpheader.GetValue("Server").find("Portable SDK for UPnP devices") !=
        std::string::npos)
    {
      CLog::Log(LOGWARNING,
                "CCurlFile::Open - Disabling multi session due to broken libupnp server");
      m_multisession = false;
    }
  }

  if (StringUtils::EqualsNoCase(m_state->m_httpheader.GetValue("Transfer-Encoding"), "chunked"))
    m_state->m_fileSize = 0;

  if (m_state->m_fileSize <= 0)
    m_seekable = false;
  if (m_seekable)
  {
    if (url2.IsProtocol("http") || url2.IsProtocol("https"))
    {
      // if server says explicitly it can't seek, respect that
      if (StringUtils::EqualsNoCase(m_state->m_httpheader.GetValue("Accept-Ranges"), "none"))
        m_seekable = false;
    }
  }

  std::error_code ec;
  auto efurl = m_state->m_curl.GetEffectiveUrl(ec);

  if (!ec && !efurl.empty())
  {
    if (m_url != efurl)
    {
      std::string redactEfpath = CURL::GetRedacted(efurl);
      CLog::Log(LOGDEBUG, "CCurlFile::Open - effective URL: <%s>", redactEfpath.c_str());
    }
    m_url = efurl;
  }

  return true;
}

bool CCurlFile::OpenForWrite(const CURL& url, bool bOverWrite)
{
  if (m_opened)
    return false;

  if (Exists(url) && !bOverWrite)
    return false;

  CURL url2(url);
  ParseAndCorrectUrl(url2);

  CLog::Log(LOGDEBUG, "CCurlFile::OpenForWrite(%p) %s", (void*)this,
            CURL::GetRedacted(m_url).c_str());

  // setup common curl options
  SetCommonOptions(m_state);
  SetRequestHeaders(m_state);

  std::error_code ec;
  auto efurl = m_state->m_curl.GetEffectiveUrl(ec);
  if (!ec && !efurl.empty())
    m_url = efurl;

  m_opened = true;
  m_forWrite = true;
  m_inError = false;
  m_writeOffset = 0;

  m_state->m_curl.EnableUpload();

  m_state->m_curl.UseMulti();

  m_state->SetReadBuffer(NULL, 0);

  return true;
}

ssize_t CCurlFile::Write(const void* lpBuf, size_t uiBufSize)
{
  if (!(m_opened && m_forWrite) || m_inError)
    return -1;

  std::error_code ec;

  m_state->SetReadBuffer(lpBuf, uiBufSize);
  m_state->m_isPaused = false;
  ec = m_state->m_curl.EasyResume();

  std::error_code result;

  m_stillRunning = 1;
  while (m_stillRunning && !m_state->m_isPaused)
  {
    while ((result = m_state->m_curl.MultiPerform(m_stillRunning)) == CurlMCode::CALL_MULTI_PERFORM)
      ;

    if (!m_stillRunning)
      break;

    if (result)
    {
      long code = m_state->m_curl.GetResponseCode(ec);
      if (!ec) // No point in writing the error code if fetching it failed
        CLog::Log(LOGERROR, "%s - Unable to write curl resource (%s) - %ld", __FUNCTION__,
                  CURL::GetRedacted(m_url).c_str(), code);
      m_inError = true;
      return -1;
    }
  }

  m_writeOffset += m_state->m_filePos;
  return m_state->m_filePos;
}

bool CCurlFile::CReadState::ReadString(char* szLine, int iLineLength)
{
  unsigned int want = (unsigned int)iLineLength;

  if ((m_fileSize == 0 || m_filePos < m_fileSize) && FillBuffer(want) != FILLBUFFER_OK)
    return false;

  // ensure only available data is considered
  want = std::min(m_buffer.getMaxReadSize(), want);

  /* check if we finished prematurely */
  if (!m_stillRunning && (m_fileSize == 0 || m_filePos != m_fileSize) && !want)
  {
    if (m_fileSize != 0)
      CLog::Log(LOGWARNING,
                "%s - Transfer ended before entire file was retrieved pos %" PRId64
                ", size %" PRId64,
                __FUNCTION__, m_filePos, m_fileSize);

    return false;
  }

  char* pLine = szLine;
  do
  {
    if (!m_buffer.ReadData(pLine, 1))
      break;

    pLine++;
  } while (((pLine - 1)[0] != '\n') && ((unsigned int)(pLine - szLine) < want));
  pLine[0] = 0;
  m_filePos += (pLine - szLine);
  return (pLine - szLine) > 0;
}

bool CCurlFile::ReOpen(const CURL& url)
{
  Close();
  return Open(url);
}

bool CCurlFile::Exists(const CURL& url)
{
  // if file is already running, get info from it
  if (m_opened)
  {
    CLog::Log(LOGWARNING, "CCurlFile::Exists - Exist called on open file %s",
              url.GetRedacted().c_str());
    return true;
  }

  std::error_code ec;
  CURL url2(url);
  ParseAndCorrectUrl(url2);

  SetCommonOptions(m_state);
  SetRequestHeaders(m_state);
  m_state->m_curl.SetTimeout(CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_curlconnecttimeout);
  m_state->m_curl.DisableBody();
  m_state->m_curl.WriteData(nullptr);

  if (url2.IsProtocol("ftp") || url2.IsProtocol("ftps"))
  {
    m_state->m_curl.EnableFileTime();
    // nocwd is less standard, will return empty list for non-existed remote dir on some ftp server, avoid it.
    if (StringUtils::EndsWith(url2.GetFileName(), "/"))
      m_state->m_curl.SetFtpFileMethod(CurlFtpMethod::SINGLECWD);
    else
      m_state->m_curl.SetFtpFileMethod(CurlFtpMethod::NOCWD);
  }

  auto result = m_state->m_curl.EasyPerform();

  if (!result || result == CurlCode::WRITE_ERROR)
  {
    return true;
  }

  if (result == CurlCode::HTTP_RETURNED_ERROR)
  {
    long code = m_state->m_curl.GetResponseCode(ec);
    if (!ec && code != 404)
    {
      if (code == 405)
      {
        // If we get a Method Not Allowed response, retry with a GET Request
        m_state->m_curl.EnableBody();
        m_state->m_curl.EnableFileTime();
        m_state->m_curl.SetProgressCallback(transfer_abort_callback);

        m_state->m_curl.AddHeader("Range: bytes=0-1");

        result = m_state->m_curl.EasyPerform();
        if (!result || result == CurlCode::WRITE_ERROR)
        {
          return true;
        }

        if (result && result == CurlCode::HTTP_RETURNED_ERROR)
        {
          long code = m_state->m_curl.GetResponseCode(result);
          if (!result && code != 404)
            CLog::Log(LOGERROR, "CCurlFile::Exists - Failed: HTTP returned error %ld for %s", code,
                      url.GetRedacted().c_str());
        }
      }
      else
        CLog::Log(LOGERROR, "CCurlFile::Exists - Failed: HTTP returned error %ld for %s", code,
                  url.GetRedacted().c_str());
    }
  }
  else if (result != CurlCode::REMOTE_FILE_NOT_FOUND && result != CurlCode::FTP_COULDNT_RETR_FILE)
  {
    CLog::Log(LOGERROR, "CCurlFile::Exists - Failed: %s(%d) for %s", result.message().c_str(),
              result.value(), url.GetRedacted().c_str());
  }

  errno = ENOENT;
  return false;
}

int64_t CCurlFile::Seek(int64_t iFilePosition, int iWhence)
{
  int64_t nextPos = m_state->m_filePos;

  if (!m_seekable)
    return -1;

  switch (iWhence)
  {
    case SEEK_SET:
      nextPos = iFilePosition;
      break;
    case SEEK_CUR:
      nextPos += iFilePosition;
      break;
    case SEEK_END:
      if (m_state->m_fileSize)
        nextPos = m_state->m_fileSize + iFilePosition;
      else
        return -1;
      break;
    default:
      return -1;
  }

  // We can't seek beyond EOF
  if (m_state->m_fileSize && nextPos > m_state->m_fileSize)
    return -1;

  if (m_state->Seek(nextPos))
    return nextPos;

  if (m_multisession)
  {
    if (!m_oldState)
    {
      CURL url(m_url);
      m_oldState = m_state;
      m_state = new CReadState();
      m_state->m_fileSize = m_oldState->m_fileSize;
    }
    else
    {
      CReadState* tmp;
      tmp = m_state;
      m_state = m_oldState;
      m_oldState = tmp;

      if (m_state->Seek(nextPos))
        return nextPos;

      m_state->Disconnect();
    }
  }
  else
    m_state->Disconnect();

  // re-setup common curl options
  SetCommonOptions(m_state);

  /* caller might have changed some headers (needed for daap)*/
  //! @todo daap is gone. is this needed for something else?
  SetRequestHeaders(m_state);

  m_state->m_filePos = nextPos;
  m_state->m_sendRange = true;
  m_state->m_bRetry = m_allowRetry;

  long response = m_state->Connect(m_bufferSize);
  if (response < 0 && (m_state->m_fileSize == 0 || m_state->m_fileSize != m_state->m_filePos))
  {
    if (m_multisession)
    {
      if (m_oldState)
      {
        delete m_state;
        m_state = m_oldState;
        m_oldState = NULL;
      }
      // Retry without multisession
      m_multisession = false;
      return Seek(iFilePosition, iWhence);
    }
    else
    {
      m_seekable = false;
      return -1;
    }
  }

  SetCorrectHeaders(m_state);

  return m_state->m_filePos;
}

int64_t CCurlFile::GetLength()
{
  if (!m_opened)
    return 0;
  return m_state->m_fileSize;
}

int64_t CCurlFile::GetPosition()
{
  if (!m_opened)
    return 0;
  return m_state->m_filePos;
}

int CCurlFile::Stat(const CURL& url, struct __stat64* buffer)
{
  // if file is already running, get info from it
  if (m_opened)
  {
    CLog::Log(LOGWARNING, "CCurlFile::Stat - Stat called on open file %s",
              url.GetRedacted().c_str());
    if (buffer)
    {
      memset(buffer, 0, sizeof(struct __stat64));
      buffer->st_size = GetLength();
      buffer->st_mode = _S_IFREG;
    }
    return 0;
  }

  std::error_code ec;
  CURL url2(url);
  ParseAndCorrectUrl(url2);

  SetCommonOptions(m_state);
  SetRequestHeaders(m_state);
  m_state->m_curl.SetTimeout(
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_curlconnecttimeout);
  m_state->m_curl.DisableBody();
  m_state->m_curl.EnableFileTime();

  if (url2.IsProtocol("ftp"))
  {
    // nocwd is less standard, will return empty list for non-existed remote dir on some ftp server, avoid it.
    if (StringUtils::EndsWith(url2.GetFileName(), "/"))
      m_state->m_curl.SetFtpFileMethod(CurlFtpMethod::SINGLECWD);
    else
      m_state->m_curl.SetFtpFileMethod(CurlFtpMethod::NOCWD);
  }

  auto result = m_state->m_curl.EasyPerform();

  if (result == CurlCode::HTTP_RETURNED_ERROR)
  {
    auto code = m_state->m_curl.GetResponseCode(ec);
    if (!ec && code == 404)
    {
      errno = ENOENT;
      return -1;
    }
  }

  if (result == CurlCode::GOT_NOTHING || result == CurlCode::HTTP_RETURNED_ERROR ||
      result == CurlCode::RECV_ERROR /* some silly shoutcast servers */)
  {
    /* some http servers and shoutcast servers don't give us any data on a head request */
    /* request normal and just bail out via progress meter callback after we received data */
    /* somehow curl doesn't reset CURLOPT_NOBODY properly so reset everything */
    SetCommonOptions(m_state);
    SetRequestHeaders(m_state);
    m_state->m_curl.SetTimeout(
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_curlconnecttimeout);
    m_state->m_curl.EnableFileTime();
    m_state->m_curl.SetProgressCallback(transfer_abort_callback);

    result = m_state->m_curl.EasyPerform();
  }

  if (result && result != CurlCode::ABORTED_BY_CALLBACK)
  {
    errno = ENOENT;
    CLog::Log(LOGERROR, "CCurlFile::Stat - Failed: %s(%d) for %s", result.message().c_str(),
              result.value(), url.GetRedacted().c_str());
    return -1;
  }

  auto length = m_state->m_curl.GetContentLength(ec);
  if (ec || length < 0ll)
  {
    if (url.IsProtocol("ftp"))
    {
      CLog::Log(LOGNOTICE, "CCurlFile::Stat - Content length failed: %s(%d) for %s",
                ec.message().c_str(), ec.value(), url.GetRedacted().c_str());
      errno = ENOENT;
      return -1;
    }
    else
      length = 0;
  }

  SetCorrectHeaders(m_state);

  if (buffer)
  {
    auto content = m_state->m_curl.GetContentType(ec);
    if (ec)
    {
      CLog::Log(LOGNOTICE, "CCurlFile::Stat - Content type failed: %s(%d) for %s",
                ec.message().c_str(), ec.value(), url.GetRedacted().c_str());
      errno = ENOENT;
      return -1;
    }

    memset(buffer, 0, sizeof(struct __stat64));
    buffer->st_size = length;
    if (!content.empty() && content.compare("text/html") == 0) //consider html files directories
      buffer->st_mode = _S_IFDIR;
    else
      buffer->st_mode = _S_IFREG;

    auto filetime = m_state->m_curl.GetFileTime(ec);
    if (ec)
      CLog::Log(LOGNOTICE, "CCurlFile::Stat - Filetime failed: %s(%d) for %s", ec.message().c_str(),
                ec.value(), url.GetRedacted().c_str());
    else if (filetime != -1)
      buffer->st_mtime = filetime;
  }

  return 0;
}

ssize_t CCurlFile::CReadState::Read(void* lpBuf, size_t uiBufSize)
{
  /* only request 1 byte, for truncated reads (only if not eof) */
  if (m_fileSize == 0 || m_filePos < m_fileSize)
  {
    int8_t result = FillBuffer(1);
    if (result == FILLBUFFER_FAIL)
      return -1; // Fatal error

    if (result == FILLBUFFER_NO_DATA)
      return 0;
  }

  /* ensure only available data is considered */
  unsigned int want = std::min<unsigned int>(m_buffer.getMaxReadSize(), uiBufSize);

  /* xfer data to caller */
  if (m_buffer.ReadData((char*)lpBuf, want))
  {
    m_filePos += want;
    return want;
  }

  /* check if we finished prematurely */
  if (!m_stillRunning && (m_fileSize == 0 || m_filePos != m_fileSize))
  {
    CLog::Log(LOGWARNING,
              "%s - Transfer ended before entire file was retrieved pos %" PRId64 ", size %" PRId64,
              __FUNCTION__, m_filePos, m_fileSize);
    return -1;
  }

  return 0;
}

/* use to attempt to fill the read buffer up to requested number of bytes */
int8_t CCurlFile::CReadState::FillBuffer(unsigned int want)
{
  int retry = 0;
  fd_set fdread;
  fd_set fdwrite;
  fd_set fdexcep;

  // only attempt to fill buffer if transactions still running and buffer
  // doesnt exceed required size already
  while (m_buffer.getMaxReadSize() < want && m_buffer.getMaxWriteSize() > 0)
  {
    if (m_cancelled)
      return FILLBUFFER_NO_DATA;

    /* if there is data in overflow buffer, try to use that first */
    if (m_overflowSize)
    {
      auto amount = std::min(static_cast<size_t>(m_buffer.getMaxWriteSize()), m_overflowSize);
      m_buffer.WriteData(m_overflowBuffer, amount);

      if (amount < m_overflowSize)
        memmove(m_overflowBuffer, m_overflowBuffer + amount, m_overflowSize - amount);

      m_overflowSize -= amount;
      // Shrink memory:
      m_overflowBuffer = (char*)realloc_simple(m_overflowBuffer, m_overflowSize);
      continue;
    }

    auto result = m_curl.MultiPerform(m_stillRunning);
    if (!m_stillRunning)
    {
      if (!result)
      {
        /* if we still have stuff in buffer, we are fine */
        if (m_buffer.getMaxReadSize())
          return FILLBUFFER_OK;

        // check for errors
        std::error_code ec;
        int msgs;
        bool bRetryNow = true;
        bool bError = false;
        while (!(ec = m_curl.GetMultiMessage(msgs)))
        {
          if (!ec)
            return FILLBUFFER_OK;

          long httpCode = 0;
          if (ec == CurlCode::HTTP_RETURNED_ERROR)
          {
            std::error_code ec2;
            httpCode = m_curl.GetResponseCode(ec2);

            // Don't log 404 not-found errors to prevent log-spam
            if (httpCode != 404)
              CLog::Log(LOGERROR, "CCurlFile::FillBuffer - Failed: HTTP returned error %ld",
                        httpCode);
          }
          else
          {
            CLog::Log(LOGERROR, "CCurlFile::FillBuffer - Failed: %s(%d)", ec.message().c_str(),
                      ec.value());
          }

          if ((ec == CurlCode::OPERATION_TIMEDOUT || ec == CurlCode::PARTIAL_FILE ||
               ec == CurlCode::COULDNT_CONNECT || ec == CurlCode::RECV_ERROR) &&
              !m_bFirstLoop)
          {
            bRetryNow = false; // Leave it to caller whether the operation is retried
            bError = true;
          }
          else if (
              (ec == CurlCode::RANGE_ERROR ||
               httpCode == 416 /* = Requested Range Not Satisfiable */ ||
               httpCode ==
                   406 /* = Not Acceptable (fixes issues with non compliant HDHomerun servers */) &&
              m_bFirstLoop && m_filePos == 0 && m_sendRange)
          {
            // If server returns a (possible) range error, disable range and retry (handled below)
            bRetryNow = true;
            bError = true;
            m_sendRange = false;
          }
          else
          {
            // For all other errors, abort the operation
            return FILLBUFFER_FAIL;
          }
        }

        // Check for an actual error, if not, just return no-data
        if (!bError && !m_bLastError)
          return FILLBUFFER_NO_DATA;

        // Close handle
        if (m_curl.IsMulti())
          m_curl.RemoveMulti();

        // Reset all the stuff like we would in Disconnect()
        m_buffer.Clear();
        free(m_overflowBuffer);
        m_overflowBuffer = NULL;
        m_overflowSize = 0;
        m_bLastError = true; // Flag error for the next run

        // Retry immediately or leave it up to the caller?
        if ((m_bRetry &&
             retry <
                 CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_curlretries) ||
            (bRetryNow && retry == 0))
        {
          retry++;

          // Connect + seek to current position (again)
          SetResume();
          m_curl.UseMulti();

          CLog::Log(LOGWARNING, "CCurlFile::FillBuffer - Reconnect, (re)try %i", retry);

          // Return to the beginning of the loop:
          continue;
        }

        return FILLBUFFER_NO_DATA; // We failed but flag no data to caller, so it can retry the operation
      }
      return FILLBUFFER_FAIL;
    }

    // We've finished out first loop
    if (m_bFirstLoop && m_buffer.getMaxReadSize() > 0)
      m_bFirstLoop = false;

    // No error this run
    m_bLastError = false;

    switch (static_cast<CurlMCode>(result.value()))
    {
      case CurlMCode::OK:
      {
        int maxfd = -1;
        FD_ZERO(&fdread);
        FD_ZERO(&fdwrite);
        FD_ZERO(&fdexcep);

        // get file descriptors from the transfers
        m_curl.MultiFdSet(&fdread, &fdwrite, &fdexcep, &maxfd);

        std::error_code ec;
        auto timeout = m_curl.GetTimeout(ec);
        if (ec || timeout == -1 || timeout < 200)
          timeout = 200;

        XbmcThreads::EndTime endTime(timeout);
        int rc;

        do
        {
          /* On success the value of maxfd is guaranteed to be >= -1. We call
           * select(maxfd + 1, ...); specially in case of (maxfd == -1) there are
           * no fds ready yet so we call select(0, ...) --or Sleep() on Windows--
           * to sleep 100ms, which is the minimum suggested value in the
           * curl_multi_fdset() doc.
           */
          if (maxfd == -1)
          {
#ifdef TARGET_WINDOWS
            /* Windows does not support using select() for sleeping without a dummy
             * socket. Instead use Windows' Sleep() and sleep for 100ms which is the
             * minimum suggested value in the curl_multi_fdset() doc.
             */
            KODI::TIME::Sleep(100);
            rc = 0;
#else
            /* Portable sleep for platforms other than Windows. */
            struct timeval wait = {0, 100 * 1000}; /* 100ms */
            rc = select(0, NULL, NULL, NULL, &wait);
#endif
          }
          else
          {
            unsigned int time_left = endTime.MillisLeft();
            struct timeval wait = {(int)time_left / 1000, ((int)time_left % 1000) * 1000};
            rc = select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &wait);
          }
#ifdef TARGET_WINDOWS
        } while (rc == SOCKET_ERROR && WSAGetLastError() == WSAEINTR);
#else
        } while (rc == SOCKET_ERROR && errno == EINTR);
#endif

        if (rc == SOCKET_ERROR)
        {
#ifdef TARGET_WINDOWS
          char buf[256];
          strerror_s(buf, 256, WSAGetLastError());
          CLog::Log(LOGERROR, "CCurlFile::FillBuffer - Failed with socket error:%s", buf);
#else
          char const* str = strerror(errno);
          CLog::Log(LOGERROR, "CCurlFile::FillBuffer - Failed with socket error:%s", str);
#endif

          return FILLBUFFER_FAIL;
        }
      }
      break;
      case CurlMCode::CALL_MULTI_PERFORM:
      {
        // we don't keep calling here as that can easily overwrite our buffer which we want to avoid
        // docs says we should call it soon after, but aslong as we are reading data somewhere
        // this aught to be soon enough. should stay in socket otherwise
        continue;
      }
      break;
      default:
      {
        CLog::Log(LOGERROR, "CCurlFile::FillBuffer - Multi perform failed with code %d, aborting",
                  result.value());
        return FILLBUFFER_FAIL;
      }
      break;
    }
  }
  return FILLBUFFER_OK;
}

void CCurlFile::CReadState::SetReadBuffer(const void* lpBuf, int64_t uiBufSize)
{
  m_readBuffer = const_cast<char*>((const char*)lpBuf);
  m_fileSize = uiBufSize;
  m_filePos = 0;
}

void CCurlFile::ClearRequestHeaders()
{
  m_requestheaders.clear();
}

void CCurlFile::SetRequestHeader(const std::string& header, const std::string& value)
{
  m_requestheaders[header] = value;
}

void CCurlFile::SetRequestHeader(const std::string& header, long value)
{
  m_requestheaders[header] = StringUtils::Format("%ld", value);
}

std::string CCurlFile::GetURL(void)
{
  return m_url;
}

std::string CCurlFile::GetRedirectURL()
{
  std::error_code ec;
  auto redirectUrl = m_state->m_curl.GetRedirectURL(ec);
  if (ec)
  {
    CLog::Log(LOGERROR, "Info string request for type redirect_url failed with result code {}",
              ec.value());
    return "";
  }
  return redirectUrl;
}

/* STATIC FUNCTIONS */
bool CCurlFile::GetHttpHeader(const CURL& url, CHttpHeader& headers)
{
  try
  {
    CCurlFile file;
    if (file.Stat(url, NULL) == 0)
    {
      headers = file.GetHttpHeader();
      return true;
    }
    return false;
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - Exception thrown while trying to retrieve header url: %s",
              __FUNCTION__, url.GetRedacted().c_str());
    return false;
  }
}

bool CCurlFile::GetMimeType(const CURL& url, std::string& content, const std::string& useragent)
{
  CCurlFile file;
  if (!useragent.empty())
    file.SetUserAgent(useragent);

  struct __stat64 buffer;
  std::string redactUrl = url.GetRedacted();
  if (file.Stat(url, &buffer) == 0)
  {
    if (buffer.st_mode == _S_IFDIR)
      content = "x-directory/normal";
    else
      content = file.GetProperty(XFILE::FILE_PROPERTY_MIME_TYPE);
    CLog::Log(LOGDEBUG, "CCurlFile::GetMimeType - %s -> %s", redactUrl.c_str(), content.c_str());
    return true;
  }
  CLog::Log(LOGDEBUG, "CCurlFile::GetMimeType - %s -> failed", redactUrl.c_str());
  content.clear();
  return false;
}

bool CCurlFile::GetContentType(const CURL& url, std::string& content, const std::string& useragent)
{
  CCurlFile file;
  if (!useragent.empty())
    file.SetUserAgent(useragent);

  struct __stat64 buffer;
  std::string redactUrl = url.GetRedacted();
  if (file.Stat(url, &buffer) == 0)
  {
    if (buffer.st_mode == _S_IFDIR)
      content = "x-directory/normal";
    else
      content = file.GetProperty(XFILE::FILE_PROPERTY_CONTENT_TYPE, "");
    CLog::Log(LOGDEBUG, "CCurlFile::GetContentType - %s -> %s", redactUrl.c_str(), content.c_str());
    return true;
  }
  CLog::Log(LOGDEBUG, "CCurlFile::GetContentType - %s -> failed", redactUrl.c_str());
  content.clear();
  return false;
}

bool CCurlFile::GetCookies(const CURL& url, std::string& cookies)
{
  CCurl curl;
  std::error_code ec;

  auto str = curl.GetCookies(ec);
  if (ec || str.empty())
    return false;

  cookies = str;
  return true;
}

int CCurlFile::IoControl(EIoControl request, void* param)
{
  if (request == IOCTRL_SEEK_POSSIBLE)
    return m_seekable ? 1 : 0;

  if (request == IOCTRL_SET_RETRY)
  {
    m_allowRetry = *(bool*)param;
    return 0;
  }

  return -1;
}

const std::string CCurlFile::GetProperty(XFILE::FileProperty type, const std::string& name) const
{
  switch (type)
  {
    case FILE_PROPERTY_RESPONSE_PROTOCOL:
      return m_state->m_httpheader.GetProtoLine();
    case FILE_PROPERTY_RESPONSE_HEADER:
      return m_state->m_httpheader.GetValue(name);
    case FILE_PROPERTY_CONTENT_TYPE:
      return m_state->m_httpheader.GetValue("content-type");
    case FILE_PROPERTY_CONTENT_CHARSET:
      return m_state->m_httpheader.GetCharset();
    case FILE_PROPERTY_MIME_TYPE:
      return m_state->m_httpheader.GetMimeType();
    case FILE_PROPERTY_EFFECTIVE_URL:
    {
      std::error_code ec;
      // We don't care about an error here as it returns an empty string on any errors
      return m_state->m_curl.GetEffectiveUrl(ec);
    }
    default:
      return "";
  }
}

const std::vector<std::string> CCurlFile::GetPropertyValues(XFILE::FileProperty type,
                                                            const std::string& name) const
{
  if (type == FILE_PROPERTY_RESPONSE_HEADER)
  {
    return m_state->m_httpheader.GetValues(name);
  }
  std::vector<std::string> values;
  std::string value = GetProperty(type, name);
  if (!value.empty())
  {
    values.emplace_back(value);
  }
  return values;
}

double CCurlFile::GetDownloadSpeed()
{
  std::error_code ec;
  return m_state->m_curl.GetDownloadSpeed(ec);
}
