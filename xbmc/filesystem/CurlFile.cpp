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
#include "filesystem/SpecialProtocol.h"
#include "network/DNSNameCache.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "threads/SystemClock.h"
#include "utils/Base64.h"
#include "utils/XTimeUtils.h"

#include <algorithm>
#include <cassert>
#include <climits>
#include <vector>

#ifdef TARGET_POSIX
#include <errno.h>
#include <inttypes.h>
#include "platform/posix/ConvUtils.h"
#endif

#include "DllLibCurl.h"
#include "ShoutcastFile.h"
#include "utils/CharsetConverter.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

using namespace XFILE;
using namespace XCURL;

using namespace std::chrono_literals;

#define FITS_INT(a) (((a) <= INT_MAX) && ((a) >= INT_MIN))

curl_proxytype proxyType2CUrlProxyType[] = {
    CURLPROXY_HTTP,   CURLPROXY_SOCKS4,          CURLPROXY_SOCKS4A,
    CURLPROXY_SOCKS5, CURLPROXY_SOCKS5_HOSTNAME, CURLPROXY_HTTPS,
};

#define FILLBUFFER_OK         0
#define FILLBUFFER_NO_DATA    1
#define FILLBUFFER_FAIL       2

// curl calls this routine to debug
extern "C" int debug_callback(CURL_HANDLE *handle, curl_infotype info, char *output, size_t size, void *data)
{
  if (info == CURLINFO_DATA_IN || info == CURLINFO_DATA_OUT)
    return 0;

  if (!CServiceBroker::GetLogging().CanLogComponent(LOGCURL))
    return 0;

  std::string strLine;
  strLine.append(output, size);
  std::vector<std::string> vecLines;
  StringUtils::Tokenize(strLine, vecLines, "\r\n");
  std::vector<std::string>::const_iterator it = vecLines.begin();

  const char *infotype;
  switch(info)
  {
    case CURLINFO_TEXT         : infotype = "TEXT: "; break;
    case CURLINFO_HEADER_IN    : infotype = "HEADER_IN: "; break;
    case CURLINFO_HEADER_OUT   : infotype = "HEADER_OUT: "; break;
    case CURLINFO_SSL_DATA_IN  : infotype = "SSL_DATA_IN: "; break;
    case CURLINFO_SSL_DATA_OUT : infotype = "SSL_DATA_OUT: "; break;
    case CURLINFO_END          : infotype = "END: "; break;
    default                    : infotype = ""; break;
  }

  while (it != vecLines.end())
  {
    CLog::Log(LOGDEBUG, "Curl::Debug - {}{}", infotype, (*it));
    ++it;
  }
  return 0;
}

/* curl calls this routine to get more data */
extern "C" size_t write_callback(char *buffer,
               size_t size,
               size_t nitems,
               void *userp)
{
  if(userp == NULL) return 0;

  CCurlFile::CReadState *state = (CCurlFile::CReadState *)userp;
  return state->WriteCallback(buffer, size, nitems);
}

extern "C" size_t read_callback(char *buffer,
               size_t size,
               size_t nitems,
               void *userp)
{
  if(userp == NULL) return 0;

  CCurlFile::CReadState *state = (CCurlFile::CReadState *)userp;
  return state->ReadCallback(buffer, size, nitems);
}

extern "C" size_t header_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
  CCurlFile::CReadState *state = (CCurlFile::CReadState *)stream;
  return state->HeaderCallback(ptr, size, nmemb);
}

/* used only by CCurlFile::Stat to bail out of unwanted transfers */
extern "C" int transfer_abort_callback(void *clientp,
               curl_off_t dltotal,
               curl_off_t dlnow,
               curl_off_t ultotal,
               curl_off_t ulnow)
{
  if(dlnow > 0)
    return 1;
  else
    return 0;
}

/* fix for silly behavior of realloc */
static inline void* realloc_simple(void *ptr, size_t size)
{
  void *ptr2 = realloc(ptr, size);
  if(ptr && !ptr2 && size > 0)
  {
    free(ptr);
    return NULL;
  }
  else
    return ptr2;
}

static constexpr int CURL_OFF = 0L;
static constexpr int CURL_ON = 1L;

size_t CCurlFile::CReadState::HeaderCallback(void *ptr, size_t size, size_t nmemb)
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

size_t CCurlFile::CReadState::ReadCallback(char *buffer, size_t size, size_t nitems)
{
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

size_t CCurlFile::CReadState::WriteCallback(char *buffer, size_t size, size_t nitems)
{
  unsigned int amount = size * nitems;
  if (m_overflowSize)
  {
    // we have our overflow buffer - first get rid of as much as we can
    unsigned int maxWriteable = std::min(m_buffer.getMaxWriteSize(), m_overflowSize);
    if (maxWriteable)
    {
      if (!m_buffer.WriteData(m_overflowBuffer, maxWriteable))
      {
        CLog::Log(LOGERROR, "CCurlFile::CReadState::{} - ({}) Unable to write to buffer",
                  __FUNCTION__, fmt::ptr(this));
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
  unsigned int maxWriteable = std::min(m_buffer.getMaxWriteSize(), amount);
  if (maxWriteable)
  {
    if (!m_buffer.WriteData(buffer, maxWriteable))
    {
      CLog::Log(LOGERROR,
                "CCurlFile::CReadState::{} - ({}) Unable to write to buffer with {} bytes",
                __FUNCTION__, fmt::ptr(this), maxWriteable);
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
    if(m_overflowBuffer == NULL)
    {
      CLog::Log(LOGWARNING,
                "CCurlFile::CReadState::{} - ({}) Failed to grow overflow buffer from {} bytes to "
                "{} bytes",
                __FUNCTION__, fmt::ptr(this), m_overflowSize, amount + m_overflowSize);
      return 0;
    }
    memcpy(m_overflowBuffer + m_overflowSize, buffer, amount);
    m_overflowSize += amount;
  }
  return size * nitems;
}

CCurlFile::CReadState::CReadState()
{
  m_easyHandle = NULL;
  m_multiHandle = NULL;
  m_overflowBuffer = NULL;
  m_overflowSize = 0;
  m_stillRunning = 0;
  m_filePos = 0;
  m_fileSize = 0;
  m_bufferSize = 0;
  m_cancelled = false;
  m_bFirstLoop = true;
  m_sendRange = true;
  m_bLastError = false;
  m_readBuffer = 0;
  m_isPaused = false;
  m_bRetry = true;
  m_curlHeaderList = NULL;
  m_curlAliasList = NULL;
}

CCurlFile::CReadState::~CReadState()
{
  Disconnect();

  if(m_easyHandle)
    g_curlInterface.easy_release(&m_easyHandle, &m_multiHandle);
}

bool CCurlFile::CReadState::Seek(int64_t pos)
{
  if(pos == m_filePos)
    return true;

  if(FITS_INT(pos - m_filePos) && m_buffer.SkipBytes((int)(pos - m_filePos)))
  {
    m_filePos = pos;
    return true;
  }

  if(pos > m_filePos && pos < m_filePos + m_bufferSize)
  {
    int len = m_buffer.getMaxReadSize();
    m_filePos += len;
    m_buffer.SkipBytes(len);
    if (FillBuffer(m_bufferSize) != FILLBUFFER_OK)
    {
      if(!m_buffer.SkipBytes(-len))
        CLog::Log(LOGERROR,
                  "CCurlFile::CReadState::{} - ({}) Failed to restore position after failed fill",
                  __FUNCTION__, fmt::ptr(this));
      else
        m_filePos -= len;
      return false;
    }

    if(!FITS_INT(pos - m_filePos) || !m_buffer.SkipBytes((int)(pos - m_filePos)))
    {
      CLog::Log(
          LOGERROR,
          "CCurlFile::CReadState::{} - ({}) Failed to skip to position after having filled buffer",
          __FUNCTION__, fmt::ptr(this));
      if(!m_buffer.SkipBytes(-len))
        CLog::Log(LOGERROR,
                  "CCurlFile::CReadState::{} - ({}) Failed to restore position after failed seek",
                  __FUNCTION__, fmt::ptr(this));
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
    g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_RANGE, "0-");
  else
  {
    g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_RANGE, NULL);
    m_sendRange = false;
  }

  g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_RESUME_FROM_LARGE, m_filePos);
}

long CCurlFile::CReadState::Connect(unsigned int size)
{
  if (m_filePos != 0)
    CLog::Log(LOGDEBUG, "CurlFile::CReadState::{} - ({}) Resume from position {}", __FUNCTION__,
              fmt::ptr(this), m_filePos);

  SetResume();
  g_curlInterface.multi_add_handle(m_multiHandle, m_easyHandle);

  m_bufferSize = size;
  m_buffer.Destroy();
  m_buffer.Create(size * 3);
  m_httpheader.Clear();

  // read some data in to try and obtain the length
  // maybe there's a better way to get this info??
  m_stillRunning = 1;

  // (Try to) fill buffer
  if (FillBuffer(1) != FILLBUFFER_OK)
  {
    // Check response code
    long response;
    if (CURLE_OK == g_curlInterface.easy_getinfo(m_easyHandle, CURLINFO_RESPONSE_CODE, &response))
      return response;
    else
      return -1;
  }

#if LIBCURL_VERSION_NUM >= 0x073700 // CURL_AT_LEAST_VERSION(0, 7, 55)
  curl_off_t length;
  if (CURLE_OK ==
      g_curlInterface.easy_getinfo(m_easyHandle, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &length))
#else
  double length;
  if (CURLE_OK == g_curlInterface.easy_getinfo(m_easyHandle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &length))
#endif
  {
    if (length < 0)
      length = 0.0;
    m_fileSize = m_filePos + (int64_t)length;
  }

  long response;
  if (CURLE_OK == g_curlInterface.easy_getinfo(m_easyHandle, CURLINFO_RESPONSE_CODE, &response))
    return response;

  return -1;
}

void CCurlFile::CReadState::Disconnect()
{
  if(m_multiHandle && m_easyHandle)
    g_curlInterface.multi_remove_handle(m_multiHandle, m_easyHandle);

  m_buffer.Clear();
  free(m_overflowBuffer);
  m_overflowBuffer = NULL;
  m_overflowSize = 0;
  m_filePos = 0;
  m_fileSize = 0;
  m_bufferSize = 0;
  m_readBuffer = 0;

  /* cleanup */
  if( m_curlHeaderList )
    g_curlInterface.slist_free_all(m_curlHeaderList);
  m_curlHeaderList = NULL;

  if( m_curlAliasList )
    g_curlInterface.slist_free_all(m_curlAliasList);
  m_curlAliasList = NULL;
}


CCurlFile::~CCurlFile()
{
  Close();
  delete m_state;
  delete m_oldState;
}

CCurlFile::CCurlFile()
 : m_overflowBuffer(NULL)
{
  m_opened = false;
  m_forWrite = false;
  m_inError = false;
  m_multisession  = true;
  m_seekable = true;
  m_connecttimeout = 0;
  m_redirectlimit = 5;
  m_lowspeedtime = 0;
  m_ftppasvip = false;
  m_bufferSize = 32768;
  m_postdataset = false;
  m_state = new CReadState();
  m_oldState = NULL;
  m_skipshout = false;
  m_httpresponse = -1;
  m_acceptCharset = "UTF-8,*;q=0.8"; /* prefer UTF-8 if available */
  m_allowRetry = true;
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

  if (m_dnsCacheList)
    g_curlInterface.slist_free_all(m_dnsCacheList);
  m_dnsCacheList = nullptr;
}

void CCurlFile::SetCommonOptions(CReadState* state, bool failOnError /* = true */)
{
  CURL_HANDLE* h = state->m_easyHandle;

  g_curlInterface.easy_reset(h);

  g_curlInterface.easy_setopt(h, CURLOPT_DEBUGFUNCTION, debug_callback);

  if( CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_logLevel >= LOG_LEVEL_DEBUG )
    g_curlInterface.easy_setopt(h, CURLOPT_VERBOSE, CURL_ON);
  else
    g_curlInterface.easy_setopt(h, CURLOPT_VERBOSE, CURL_OFF);

  g_curlInterface.easy_setopt(h, CURLOPT_WRITEDATA, state);
  g_curlInterface.easy_setopt(h, CURLOPT_WRITEFUNCTION, write_callback);

  g_curlInterface.easy_setopt(h, CURLOPT_READDATA, state);
  g_curlInterface.easy_setopt(h, CURLOPT_READFUNCTION, read_callback);

  // use DNS cache
  g_curlInterface.easy_setopt(h, CURLOPT_RESOLVE, m_dnsCacheList);

  // make sure headers are separated from the data stream
  g_curlInterface.easy_setopt(h, CURLOPT_WRITEHEADER, state);
  g_curlInterface.easy_setopt(h, CURLOPT_HEADERFUNCTION, header_callback);
  g_curlInterface.easy_setopt(h, CURLOPT_HEADER, CURL_OFF);

  g_curlInterface.easy_setopt(h, CURLOPT_FTP_USE_EPSV, 0); // turn off epsv

  // Allow us to follow redirects
  g_curlInterface.easy_setopt(h, CURLOPT_FOLLOWLOCATION, m_redirectlimit != 0);
  g_curlInterface.easy_setopt(h, CURLOPT_MAXREDIRS, m_redirectlimit);

  // Enable cookie engine for current handle
  g_curlInterface.easy_setopt(h, CURLOPT_COOKIEFILE, "");

  // Set custom cookie if requested
  if (!m_cookie.empty())
    g_curlInterface.easy_setopt(h, CURLOPT_COOKIE, m_cookie.c_str());

  g_curlInterface.easy_setopt(h, CURLOPT_COOKIELIST, "FLUSH");

  // When using multiple threads you should set the CURLOPT_NOSIGNAL option to
  // TRUE for all handles. Everything will work fine except that timeouts are not
  // honored during the DNS lookup - which you can work around by building libcurl
  // with c-ares support. c-ares is a library that provides asynchronous name
  // resolves. Unfortunately, c-ares does not yet support IPv6.
  g_curlInterface.easy_setopt(h, CURLOPT_NOSIGNAL, CURL_ON);

  if (failOnError)
  {
    // not interested in failed requests
    g_curlInterface.easy_setopt(h, CURLOPT_FAILONERROR, 1);
  }

  // enable support for icecast / shoutcast streams
  if ( NULL == state->m_curlAliasList )
    // m_curlAliasList is used only by this one place, but SetCommonOptions can
    // be called multiple times, only append to list if it's empty.
    state->m_curlAliasList = g_curlInterface.slist_append(state->m_curlAliasList, "ICY 200 OK");
  g_curlInterface.easy_setopt(h, CURLOPT_HTTP200ALIASES, state->m_curlAliasList);

  if (!m_verifyPeer)
    g_curlInterface.easy_setopt(h, CURLOPT_SSL_VERIFYPEER, 0);

  g_curlInterface.easy_setopt(m_state->m_easyHandle, CURLOPT_URL, m_url.c_str());
  g_curlInterface.easy_setopt(m_state->m_easyHandle, CURLOPT_TRANSFERTEXT, CURL_OFF);

  // setup POST data if it is set (and it may be empty)
  if (m_postdataset)
  {
    g_curlInterface.easy_setopt(h, CURLOPT_POST, 1 );
    g_curlInterface.easy_setopt(h, CURLOPT_POSTFIELDSIZE, m_postdata.length());
    g_curlInterface.easy_setopt(h, CURLOPT_POSTFIELDS, m_postdata.c_str());
  }

  // setup Referer header if needed
  if (!m_referer.empty())
    g_curlInterface.easy_setopt(h, CURLOPT_REFERER, m_referer.c_str());
  else
  {
    g_curlInterface.easy_setopt(h, CURLOPT_REFERER, NULL);
    // Do not send referer header on redirects (same behaviour as ffmpeg and browsers)
    g_curlInterface.easy_setopt(h, CURLOPT_AUTOREFERER, CURL_OFF);
  }

  // setup any requested authentication
  if( !m_ftpauth.empty() )
  {
    g_curlInterface.easy_setopt(h, CURLOPT_FTP_SSL, CURLFTPSSL_TRY);
    if( m_ftpauth == "any" )
      g_curlInterface.easy_setopt(h, CURLOPT_FTPSSLAUTH, CURLFTPAUTH_DEFAULT);
    else if( m_ftpauth == "ssl" )
      g_curlInterface.easy_setopt(h, CURLOPT_FTPSSLAUTH, CURLFTPAUTH_SSL);
    else if( m_ftpauth == "tls" )
      g_curlInterface.easy_setopt(h, CURLOPT_FTPSSLAUTH, CURLFTPAUTH_TLS);
  }

  // setup requested http authentication method
  bool bAuthSet = false;
  if(!m_httpauth.empty())
  {
    bAuthSet = true;
    if( m_httpauth == "any" )
      g_curlInterface.easy_setopt(h, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
    else if( m_httpauth == "anysafe" )
      g_curlInterface.easy_setopt(h, CURLOPT_HTTPAUTH, CURLAUTH_ANYSAFE);
    else if( m_httpauth == "digest" )
      g_curlInterface.easy_setopt(h, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
    else if( m_httpauth == "ntlm" )
      g_curlInterface.easy_setopt(h, CURLOPT_HTTPAUTH, CURLAUTH_NTLM);
    else
      bAuthSet = false;
  }

  // set username and password for current handle
  if (!m_username.empty())
  {
    g_curlInterface.easy_setopt(h, CURLOPT_USERNAME, m_username.c_str());
    if (!m_password.empty())
      g_curlInterface.easy_setopt(h, CURLOPT_PASSWORD, m_password.c_str());

    if (!bAuthSet)
      g_curlInterface.easy_setopt(h, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
  }

  // allow passive mode for ftp
  if( m_ftpport.length() > 0 )
    g_curlInterface.easy_setopt(h, CURLOPT_FTPPORT, m_ftpport.c_str());
  else
    g_curlInterface.easy_setopt(h, CURLOPT_FTPPORT, NULL);

  // allow curl to not use the ip address in the returned pasv response
  if( m_ftppasvip )
    g_curlInterface.easy_setopt(h, CURLOPT_FTP_SKIP_PASV_IP, 0);
  else
    g_curlInterface.easy_setopt(h, CURLOPT_FTP_SKIP_PASV_IP, 1);

  // setup Accept-Encoding if requested
  if (m_acceptencoding.length() > 0)
    g_curlInterface.easy_setopt(h, CURLOPT_ACCEPT_ENCODING, m_acceptencoding == "all" ? "" : m_acceptencoding.c_str());

  if (!m_acceptCharset.empty())
    SetRequestHeader("Accept-Charset", m_acceptCharset);

  if (m_userAgent.length() > 0)
    g_curlInterface.easy_setopt(h, CURLOPT_USERAGENT, m_userAgent.c_str());
  else /* set some default agent as shoutcast doesn't return proper stuff otherwise */
    g_curlInterface.easy_setopt(h, CURLOPT_USERAGENT, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_userAgent.c_str());

  if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_curlDisableIPV6)
    g_curlInterface.easy_setopt(h, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);

  if (!m_proxyhost.empty())
  {
    g_curlInterface.easy_setopt(h, CURLOPT_PROXYTYPE, proxyType2CUrlProxyType[m_proxytype]);

    const std::string hostport = m_proxyhost + StringUtils::Format(":{}", m_proxyport);
    g_curlInterface.easy_setopt(h, CURLOPT_PROXY, hostport.c_str());

    std::string userpass;

    if (!m_proxyuser.empty() && !m_proxypassword.empty())
      userpass = CURL::Encode(m_proxyuser) + ":" + CURL::Encode(m_proxypassword);

    if (!userpass.empty())
      g_curlInterface.easy_setopt(h, CURLOPT_PROXYUSERPWD, userpass.c_str());
  }
  if (m_customrequest.length() > 0)
    g_curlInterface.easy_setopt(h, CURLOPT_CUSTOMREQUEST, m_customrequest.c_str());

  if (m_connecttimeout == 0)
    m_connecttimeout = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_curlconnecttimeout;

  // set our timeouts, we abort connection after m_timeout, and reads after no data for m_timeout seconds
  g_curlInterface.easy_setopt(h, CURLOPT_CONNECTTIMEOUT, m_connecttimeout);

  // We abort in case we transfer less than 1byte/second
  g_curlInterface.easy_setopt(h, CURLOPT_LOW_SPEED_LIMIT, 1);

  if (m_lowspeedtime == 0)
    m_lowspeedtime = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_curllowspeedtime;

  // Set the lowspeed time very low as it seems Curl takes much longer to detect a lowspeed condition
  g_curlInterface.easy_setopt(h, CURLOPT_LOW_SPEED_TIME, m_lowspeedtime);

  // enable tcp keepalive
  if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_curlKeepAliveInterval > 0)
  {
    g_curlInterface.easy_setopt(h, CURLOPT_TCP_KEEPALIVE, 1L);
    g_curlInterface.easy_setopt(
        h, CURLOPT_TCP_KEEPIDLE,
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_curlKeepAliveInterval / 2);
    g_curlInterface.easy_setopt(
        h, CURLOPT_TCP_KEEPINTVL,
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_curlKeepAliveInterval);
  }

  // Setup allowed TLS/SSL ciphers. New versions of cURL may deprecate things that are still in use.
  if (!m_cipherlist.empty())
    g_curlInterface.easy_setopt(h, CURLOPT_SSL_CIPHER_LIST, m_cipherlist.c_str());

  if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_curlDisableHTTP2)
    g_curlInterface.easy_setopt(h, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
  else
    // enable HTTP2 support. default: CURL_HTTP_VERSION_1_1. Curl >= 7.62.0 defaults to CURL_HTTP_VERSION_2TLS
    g_curlInterface.easy_setopt(h, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2TLS);

  // set CA bundle file
  std::string caCert = CSpecialProtocol::TranslatePath(
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_caTrustFile);
#ifdef TARGET_WINDOWS_STORE
  // UWP Curl - Setting CURLOPT_CAINFO with a valid cacert file path is required for UWP
  g_curlInterface.easy_setopt(h, CURLOPT_CAINFO, "system\\certs\\cacert.pem");
#endif
  if (!caCert.empty() && XFILE::CFile::Exists(caCert))
    g_curlInterface.easy_setopt(h, CURLOPT_CAINFO, caCert.c_str());
}

void CCurlFile::SetRequestHeaders(CReadState* state)
{
  if(state->m_curlHeaderList)
  {
    g_curlInterface.slist_free_all(state->m_curlHeaderList);
    state->m_curlHeaderList = NULL;
  }

  for (const auto& it : m_requestheaders)
  {
    std::string buffer = it.first + ": " + it.second;
    state->m_curlHeaderList = g_curlInterface.slist_append(state->m_curlHeaderList, buffer.c_str());
  }

  // add user defined headers
  if (state->m_easyHandle)
    g_curlInterface.easy_setopt(state->m_easyHandle, CURLOPT_HTTPHEADER, state->m_curlHeaderList);
}

void CCurlFile::SetCorrectHeaders(CReadState* state)
{
  CHttpHeader& h = state->m_httpheader;
  /* workaround for shoutcast server which doesn't set content type on standard mp3 */
  if( h.GetMimeType().empty() )
  {
    if( !h.GetValue("icy-notice1").empty()
    || !h.GetValue("icy-name").empty()
    || !h.GetValue("icy-br").empty() )
      h.AddParam("Content-Type", "audio/mpeg");
  }

  /* hack for google video */
  if (StringUtils::EqualsNoCase(h.GetMimeType(),"text/html")
  &&  !h.GetValue("Content-Disposition").empty() )
  {
    std::string strValue = h.GetValue("Content-Disposition");
    if (strValue.find("filename=") != std::string::npos &&
        strValue.find(".flv") != std::string::npos)
      h.AddParam("Content-Type", "video/flv");
  }
}

void CCurlFile::ParseAndCorrectUrl(CURL &url2)
{
  std::string strProtocol = url2.GetTranslatedProtocol();
  url2.SetProtocol(strProtocol);

  // lookup host in DNS cache
  std::string resolvedHost;
  if (CDNSNameCache::GetCached(url2.GetHostName(), resolvedHost))
  {
    struct curl_slist* tempCache;
    int entryPort = url2.GetPort();

    if (entryPort == 0)
    {
      if (strProtocol == "http")
        entryPort = 80;
      else if (strProtocol == "https")
        entryPort = 443;
      else if (strProtocol == "ftp")
        entryPort = 21;
      else if (strProtocol == "ftps")
        entryPort = 990;
    }

    std::string entryString =
        url2.GetHostName() + ":" + std::to_string(entryPort) + ":" + resolvedHost;
    tempCache = g_curlInterface.slist_append(m_dnsCacheList, entryString.c_str());

    if (tempCache)
      m_dnsCacheList = tempCache;
  }

  if( url2.IsProtocol("ftp")
   || url2.IsProtocol("ftps") )
  {
    // we was using url options for urls, keep the old code work and warning
    if (!url2.GetOptions().empty())
    {
      CLog::Log(LOGWARNING,
                "CCurlFile::{} - <{}> FTP url option is deprecated, please switch to use protocol "
                "option (change "
                "'?' to '|')",
                __FUNCTION__, url2.GetRedacted());
      url2.SetProtocolOptions(url2.GetOptions().substr(1));
      /* ftp has no options */
      url2.SetOptions("");
    }

    /* this is ugly, depending on where we get  */
    /* the link from, it may or may not be      */
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
    for (std::vector<std::string>::iterator it = array.begin(); it != array.end(); ++it)
    {
      if(it != array.begin())
        filename += "/";

      filename += CURL::Encode(*it);
    }

    /* make sure we keep slashes */
    if(StringUtils::EndsWith(url2.GetFileName(), "/"))
      filename += "/";

    url2.SetFileName(filename);

    m_ftpauth.clear();
    if (url2.HasProtocolOption("auth"))
    {
      m_ftpauth = url2.GetProtocolOption("auth");
      StringUtils::ToLower(m_ftpauth);
      if(m_ftpauth.empty())
        m_ftpauth = "any";
    }
    m_ftpport = "";
    if (url2.HasProtocolOption("active"))
    {
      m_ftpport = url2.GetProtocolOption("active");
      if(m_ftpport.empty())
        m_ftpport = "-";
    }
    if (url2.HasProtocolOption("verifypeer"))
    {
      if (url2.GetProtocolOption("verifypeer") == "false")
        m_verifyPeer = false;
    }
    m_ftppasvip = url2.HasProtocolOption("pasvip") && url2.GetProtocolOption("pasvip") != "0";
  }
  else if(url2.IsProtocol("http") ||
    url2.IsProtocol("https"))
  {
    std::shared_ptr<CSettings> s = CServiceBroker::GetSettingsComponent()->GetSettings();
    if (!s)
      return;

    if (!url2.IsLocalHost() &&
      m_proxyhost.empty() &&
      s->GetBool(CSettings::SETTING_NETWORK_USEHTTPPROXY) &&
      !s->GetString(CSettings::SETTING_NETWORK_HTTPPROXYSERVER).empty() &&
      s->GetInt(CSettings::SETTING_NETWORK_HTTPPROXYPORT) > 0)
    {
      m_proxytype = (ProxyType)s->GetInt(CSettings::SETTING_NETWORK_HTTPPROXYTYPE);
      m_proxyhost = s->GetString(CSettings::SETTING_NETWORK_HTTPPROXYSERVER);
      m_proxyport = s->GetInt(CSettings::SETTING_NETWORK_HTTPPROXYPORT);
      m_proxyuser = s->GetString(CSettings::SETTING_NETWORK_HTTPPROXYUSERNAME);
      m_proxypassword = s->GetString(CSettings::SETTING_NETWORK_HTTPPROXYPASSWORD);
      CLog::LogFC(LOGDEBUG, LOGCURL, "<{}> Using proxy {}, type {}", url2.GetRedacted(),
                  m_proxyhost, proxyType2CUrlProxyType[m_proxytype]);
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
          if(m_httpauth.empty())
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
        else if (name == "active-remote")// needed for DACP!
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
            CLog::LogFC(LOGDEBUG, LOGCURL, "<{}> Adding custom header option '{}: ***********'",
                        url2.GetRedacted(), it.first.substr(1));
          }
          else
          {
            SetRequestHeader(it.first, value);
            if (name == "authorization")
              CLog::LogFC(LOGDEBUG, LOGCURL, "<{}> Adding custom header option '{}: ***********'",
                          url2.GetRedacted(), it.first);
            else
              CLog::LogFC(LOGDEBUG, LOGCURL, "<{}> Adding custom header option '{}: {}'",
                          url2.GetRedacted(), it.first, value);
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

bool CCurlFile::Post(const std::string& strURL, const std::string& strPostData, std::string& strHTML)
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
  int size_read = 0;
  strHTML = "";
  char buffer[16384];
  while( (size_read = Read(buffer, sizeof(buffer)-1) ) > 0 )
  {
    buffer[size_read] = 0;
    strHTML.append(buffer, size_read);
  }
  if (m_state->m_cancelled)
    return false;
  return true;
}

bool CCurlFile::Download(const std::string& strURL, const std::string& strFileName, unsigned int* pdwSize)
{
  CLog::Log(LOGINFO, "CCurlFile::{} - {}->{}", __FUNCTION__, CURL::GetRedacted(strURL),
            strFileName);

  std::string strData;
  if (!Get(strURL, strData))
    return false;

  XFILE::CFile file;
  if (!file.OpenForWrite(strFileName, true))
  {
    CLog::Log(LOGERROR, "CCurlFile::{} - <{}> Unable to open file for write: {} ({})", __FUNCTION__,
              CURL::GetRedacted(strURL), strFileName, GetLastError());
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
  CURL url("http://www.msftconnecttest.com/connecttest.txt");
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
    KODI::TIME::Sleep(1ms);
}

void CCurlFile::Reset()
{
  m_state->m_cancelled = false;
}

void CCurlFile::SetProxy(const std::string &type, const std::string &host,
  uint16_t port, const std::string &user, const std::string &password)
{
  m_proxytype = CCurlFile::PROXY_HTTP;
  if (type == "http")
    m_proxytype = CCurlFile::PROXY_HTTP;
  else if (type == "https")
    m_proxytype = CCurlFile::PROXY_HTTPS;
  else if (type == "socks4")
    m_proxytype = CCurlFile::PROXY_SOCKS4;
  else if (type == "socks4a")
    m_proxytype = CCurlFile::PROXY_SOCKS4A;
  else if (type == "socks5")
    m_proxytype = CCurlFile::PROXY_SOCKS5;
  else if (type == "socks5-remote")
    m_proxytype = CCurlFile::PROXY_SOCKS5_REMOTE;
  else
    CLog::Log(LOGERROR, "CCurFile::{} - <{}> Invalid proxy type \"{}\"", __FUNCTION__,
              CURL::GetRedacted(m_url), type);
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
  CLog::Log(LOGDEBUG, "CurlFile::{} - <{}>", __FUNCTION__, redactPath);

  assert(!(!m_state->m_easyHandle ^ !m_state->m_multiHandle));
  if( m_state->m_easyHandle == NULL )
    g_curlInterface.easy_acquire(url2.GetProtocol().c_str(),
                                url2.GetHostName().c_str(),
                                &m_state->m_easyHandle,
                                &m_state->m_multiHandle);

  // setup common curl options
  SetCommonOptions(m_state,
                   m_failOnError && !CServiceBroker::GetLogging().CanLogComponent(LOGCURL));
  SetRequestHeaders(m_state);
  m_state->m_sendRange = m_seekable;
  m_state->m_bRetry = m_allowRetry;

  m_httpresponse = m_state->Connect(m_bufferSize);

  if (m_httpresponse <= 0 || (m_failOnError && m_httpresponse >= 400))
  {
    std::string error;
    if (m_httpresponse >= 400 && CServiceBroker::GetLogging().CanLogComponent(LOGCURL))
    {
      error.resize(4096);
      ReadString(&error[0], 4095);
    }

    CLog::Log(LOGERROR, "CCurlFile::{} - <{}> Failed with code {}:\n{}", __FUNCTION__,
              redactPath, m_httpresponse, error);

    return false;
  }

  SetCorrectHeaders(m_state);

  // since we can't know the stream size up front if we're gzipped/deflated
  // flag the stream with an unknown file size rather than the compressed
  // file size.
  if (!m_state->m_httpheader.GetValue("Content-Encoding").empty() && !StringUtils::EqualsNoCase(m_state->m_httpheader.GetValue("Content-Encoding"), "identity"))
    m_state->m_fileSize = 0;

  // check if this stream is a shoutcast stream. sometimes checking the protocol line is not enough so examine other headers as well.
  // shoutcast streams should be handled by FileShoutcast.
  if ((m_state->m_httpheader.GetProtoLine().substr(0, 3) == "ICY" || !m_state->m_httpheader.GetValue("icy-notice1").empty()
     || !m_state->m_httpheader.GetValue("icy-name").empty()
     || !m_state->m_httpheader.GetValue("icy-br").empty()) && !m_skipshout)
  {
    CLog::Log(LOGDEBUG, "CCurlFile::{} - <{}> File is a shoutcast stream. Re-opening", __FUNCTION__,
              redactPath);
    throw new CRedirectException(new CShoutcastFile);
  }

  m_multisession = false;
  if(url2.IsProtocol("http") || url2.IsProtocol("https"))
  {
    m_multisession = true;
    if(m_state->m_httpheader.GetValue("Server").find("Portable SDK for UPnP devices") != std::string::npos)
    {
      CLog::Log(LOGWARNING,
                "CCurlFile::{} - <{}> Disabling multi session due to broken libupnp server",
                __FUNCTION__, redactPath);
      m_multisession = false;
    }
  }

  if(StringUtils::EqualsNoCase(m_state->m_httpheader.GetValue("Transfer-Encoding"), "chunked"))
    m_state->m_fileSize = 0;

  if(m_state->m_fileSize <= 0)
    m_seekable = false;
  if (m_seekable)
  {
    if(url2.IsProtocol("http")
    || url2.IsProtocol("https"))
    {
      // if server says explicitly it can't seek, respect that
      if(StringUtils::EqualsNoCase(m_state->m_httpheader.GetValue("Accept-Ranges"),"none"))
        m_seekable = false;
    }
  }

  std::string efurl = GetInfoString(CURLINFO_EFFECTIVE_URL);
  if (!efurl.empty())
  {
    if (m_url != efurl)
    {
      std::string redactEfpath = CURL::GetRedacted(efurl);
      CLog::Log(LOGDEBUG, "CCurlFile::{} - <{}> Effective URL is {}", __FUNCTION__, redactPath,
                redactEfpath);
    }
    m_url = efurl;
  }

  return true;
}

bool CCurlFile::OpenForWrite(const CURL& url, bool bOverWrite)
{
  if(m_opened)
    return false;

  if (Exists(url) && !bOverWrite)
    return false;

  CURL url2(url);
  ParseAndCorrectUrl(url2);

  CLog::Log(LOGDEBUG, "CCurlFile::{} - Opening {}", __FUNCTION__, CURL::GetRedacted(m_url));

  assert(m_state->m_easyHandle == NULL);
  g_curlInterface.easy_acquire(url2.GetProtocol().c_str(),
                              url2.GetHostName().c_str(),
                              &m_state->m_easyHandle,
                              &m_state->m_multiHandle);

  // setup common curl options
  SetCommonOptions(m_state);
  SetRequestHeaders(m_state);

  std::string efurl = GetInfoString(CURLINFO_EFFECTIVE_URL);
  if (!efurl.empty())
    m_url = efurl;

  m_opened = true;
  m_forWrite = true;
  m_inError = false;
  m_writeOffset = 0;

  assert(m_state->m_multiHandle);

  g_curlInterface.easy_setopt(m_state->m_easyHandle, CURLOPT_UPLOAD, 1);

  g_curlInterface.multi_add_handle(m_state->m_multiHandle, m_state->m_easyHandle);

  m_state->SetReadBuffer(NULL, 0);

  return true;
}

ssize_t CCurlFile::Write(const void* lpBuf, size_t uiBufSize)
{
  if (!(m_opened && m_forWrite) || m_inError)
    return -1;

  assert(m_state->m_multiHandle);

  m_state->SetReadBuffer(lpBuf, uiBufSize);
  m_state->m_isPaused = false;
  g_curlInterface.easy_pause(m_state->m_easyHandle, CURLPAUSE_CONT);

  CURLMcode result = CURLM_OK;

  m_stillRunning = 1;
  while (m_stillRunning && !m_state->m_isPaused)
  {
    while ((result = g_curlInterface.multi_perform(m_state->m_multiHandle, &m_stillRunning)) == CURLM_CALL_MULTI_PERFORM);

    if (!m_stillRunning)
      break;

    if (result != CURLM_OK)
    {
      long code;
      if(g_curlInterface.easy_getinfo(m_state->m_easyHandle, CURLINFO_RESPONSE_CODE, &code) == CURLE_OK )
        CLog::Log(LOGERROR, "CCurlFile::{} - <{}> Unable to write curl resource with code {}",
                  __FUNCTION__, CURL::GetRedacted(m_url), code);
      m_inError = true;
      return -1;
    }
  }

  m_writeOffset += m_state->m_filePos;
  return m_state->m_filePos;
}

bool CCurlFile::CReadState::ReadString(char *szLine, int iLineLength)
{
  unsigned int want = (unsigned int)iLineLength;

  if((m_fileSize == 0 || m_filePos < m_fileSize) && FillBuffer(want) != FILLBUFFER_OK)
    return false;

  // ensure only available data is considered
  want = std::min(m_buffer.getMaxReadSize(), want);

  /* check if we finished prematurely */
  if (!m_stillRunning && (m_fileSize == 0 || m_filePos != m_fileSize) && !want)
  {
    if (m_fileSize != 0)
      CLog::Log(
          LOGWARNING,
          "CCurlFile::{} - ({}) Transfer ended before entire file was retrieved pos {}, size {}",
          __FUNCTION__, fmt::ptr(this), m_filePos, m_fileSize);

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
  if( m_opened )
  {
    CLog::Log(LOGWARNING, "CCurlFile::{} - <{}> Exist called on open file", __FUNCTION__,
              url.GetRedacted());
    return true;
  }

  CURL url2(url);
  ParseAndCorrectUrl(url2);

  assert(m_state->m_easyHandle == NULL);
  g_curlInterface.easy_acquire(url2.GetProtocol().c_str(),
                              url2.GetHostName().c_str(),
                              &m_state->m_easyHandle, NULL);

  SetCommonOptions(m_state);
  SetRequestHeaders(m_state);
  g_curlInterface.easy_setopt(m_state->m_easyHandle, CURLOPT_TIMEOUT, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_curlconnecttimeout);
  g_curlInterface.easy_setopt(m_state->m_easyHandle, CURLOPT_NOBODY, 1);
  g_curlInterface.easy_setopt(m_state->m_easyHandle, CURLOPT_WRITEDATA, NULL); /* will cause write failure*/

  if(url2.IsProtocol("ftp") || url2.IsProtocol("ftps"))
  {
    g_curlInterface.easy_setopt(m_state->m_easyHandle, CURLOPT_FILETIME, 1);
    // nocwd is less standard, will return empty list for non-existed remote dir on some ftp server, avoid it.
    if (StringUtils::EndsWith(url2.GetFileName(), "/"))
      g_curlInterface.easy_setopt(m_state->m_easyHandle, CURLOPT_FTP_FILEMETHOD, CURLFTPMETHOD_SINGLECWD);
    else
      g_curlInterface.easy_setopt(m_state->m_easyHandle, CURLOPT_FTP_FILEMETHOD, CURLFTPMETHOD_NOCWD);
  }

  CURLcode result = g_curlInterface.easy_perform(m_state->m_easyHandle);

  if (result == CURLE_WRITE_ERROR || result == CURLE_OK)
  {
    g_curlInterface.easy_release(&m_state->m_easyHandle, NULL);
    return true;
  }

  if (result == CURLE_HTTP_RETURNED_ERROR)
  {
    long code;
    if(g_curlInterface.easy_getinfo(m_state->m_easyHandle, CURLINFO_RESPONSE_CODE, &code) == CURLE_OK && code != 404 )
    {
      if (code == 405)
      {
        // If we get a Method Not Allowed response, retry with a GET Request
        g_curlInterface.easy_setopt(m_state->m_easyHandle, CURLOPT_NOBODY, 0);

        g_curlInterface.easy_setopt(m_state->m_easyHandle, CURLOPT_FILETIME, 1);
        g_curlInterface.easy_setopt(m_state->m_easyHandle, CURLOPT_XFERINFOFUNCTION, transfer_abort_callback);
        g_curlInterface.easy_setopt(m_state->m_easyHandle, CURLOPT_NOPROGRESS, 0);

        curl_slist *list = NULL;
        list = g_curlInterface.slist_append(list, "Range: bytes=0-1"); /* try to only request 1 byte */
        g_curlInterface.easy_setopt(m_state->m_easyHandle, CURLOPT_HTTPHEADER, list);

        CURLcode result = g_curlInterface.easy_perform(m_state->m_easyHandle);
        g_curlInterface.slist_free_all(list);

        if (result == CURLE_WRITE_ERROR || result == CURLE_OK)
        {
          g_curlInterface.easy_release(&m_state->m_easyHandle, NULL);
          return true;
        }

        if (result == CURLE_HTTP_RETURNED_ERROR)
        {
          if (g_curlInterface.easy_getinfo(m_state->m_easyHandle, CURLINFO_RESPONSE_CODE, &code) == CURLE_OK && code != 404 )
            CLog::Log(LOGERROR, "CCurlFile::{} - <{}> Failed: HTTP returned error {}", __FUNCTION__,
                      url.GetRedacted(), code);
        }
      }
      else
        CLog::Log(LOGERROR, "CCurlFile::{} - <{}> Failed: HTTP returned error {}", __FUNCTION__,
                  url.GetRedacted(), code);
    }
  }
  else if (result != CURLE_REMOTE_FILE_NOT_FOUND && result != CURLE_FTP_COULDNT_RETR_FILE)
  {
    CLog::Log(LOGERROR, "CCurlFile::{} - <{}> Failed: {}({})", __FUNCTION__, url.GetRedacted(),
              g_curlInterface.easy_strerror(result), result);
  }

  errno = ENOENT;
  g_curlInterface.easy_release(&m_state->m_easyHandle, NULL);
  return false;
}

int64_t CCurlFile::Seek(int64_t iFilePosition, int iWhence)
{
  int64_t nextPos = m_state->m_filePos;

  if(!m_seekable)
    return -1;

  switch(iWhence)
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
  if (m_state->m_fileSize && nextPos > m_state->m_fileSize) return -1;

  if(m_state->Seek(nextPos))
    return nextPos;

  if (m_multisession)
  {
    if (!m_oldState)
    {
      CURL url(m_url);
      m_oldState          = m_state;
      m_state             = new CReadState();
      m_state->m_fileSize = m_oldState->m_fileSize;
      g_curlInterface.easy_acquire(url.GetProtocol().c_str(),
                                  url.GetHostName().c_str(),
                                  &m_state->m_easyHandle,
                                  &m_state->m_multiHandle );
    }
    else
    {
      CReadState *tmp;
      tmp         = m_state;
      m_state     = m_oldState;
      m_oldState  = tmp;

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
  if(response < 0 && (m_state->m_fileSize == 0 || m_state->m_fileSize != m_state->m_filePos))
  {
    if(m_multisession)
    {
      if (m_oldState)
      {
        delete m_state;
        m_state     = m_oldState;
        m_oldState  = NULL;
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
  if (!m_opened) return 0;
  return m_state->m_fileSize;
}

int64_t CCurlFile::GetPosition()
{
  if (!m_opened) return 0;
  return m_state->m_filePos;
}

int CCurlFile::Stat(const CURL& url, struct __stat64* buffer)
{
  // if file is already running, get info from it
  if( m_opened )
  {
    CLog::Log(LOGWARNING, "CCurlFile::{} - <{}> Stat called on open file", __FUNCTION__,
              url.GetRedacted());
    if (buffer)
    {
      *buffer = {};
      buffer->st_size = GetLength();
      buffer->st_mode = _S_IFREG;
    }
    return 0;
  }

  CURL url2(url);
  ParseAndCorrectUrl(url2);

  assert(m_state->m_easyHandle == NULL);
  g_curlInterface.easy_acquire(url2.GetProtocol().c_str(),
                              url2.GetHostName().c_str(),
                              &m_state->m_easyHandle, NULL);

  SetCommonOptions(m_state);
  SetRequestHeaders(m_state);
  g_curlInterface.easy_setopt(m_state->m_easyHandle, CURLOPT_TIMEOUT, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_curlconnecttimeout);
  g_curlInterface.easy_setopt(m_state->m_easyHandle, CURLOPT_NOBODY, 1);
  g_curlInterface.easy_setopt(m_state->m_easyHandle, CURLOPT_FILETIME , 1);

  if(url2.IsProtocol("ftp"))
  {
    // nocwd is less standard, will return empty list for non-existed remote dir on some ftp server, avoid it.
    if (StringUtils::EndsWith(url2.GetFileName(), "/"))
      g_curlInterface.easy_setopt(m_state->m_easyHandle, CURLOPT_FTP_FILEMETHOD, CURLFTPMETHOD_SINGLECWD);
    else
      g_curlInterface.easy_setopt(m_state->m_easyHandle, CURLOPT_FTP_FILEMETHOD, CURLFTPMETHOD_NOCWD);
  }

  CURLcode result = g_curlInterface.easy_perform(m_state->m_easyHandle);

  if(result == CURLE_HTTP_RETURNED_ERROR)
  {
    long code;
    if(g_curlInterface.easy_getinfo(m_state->m_easyHandle, CURLINFO_RESPONSE_CODE, &code) == CURLE_OK && code == 404 )
    {
      g_curlInterface.easy_release(&m_state->m_easyHandle, NULL);
      errno = ENOENT;
      return -1;
    }
  }

  if(result == CURLE_GOT_NOTHING
  || result == CURLE_HTTP_RETURNED_ERROR
  || result == CURLE_RECV_ERROR /* some silly shoutcast servers */ )
  {
    /* some http servers and shoutcast servers don't give us any data on a head request */
    /* request normal and just bail out via progress meter callback after we received data */
    /* somehow curl doesn't reset CURLOPT_NOBODY properly so reset everything */
    SetCommonOptions(m_state);
    SetRequestHeaders(m_state);
    g_curlInterface.easy_setopt(m_state->m_easyHandle, CURLOPT_TIMEOUT, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_curlconnecttimeout);
    g_curlInterface.easy_setopt(m_state->m_easyHandle, CURLOPT_FILETIME, 1);
#if LIBCURL_VERSION_NUM >= 0x072000 // 0.7.32
    g_curlInterface.easy_setopt(m_state->m_easyHandle, CURLOPT_XFERINFOFUNCTION, transfer_abort_callback);
#else
    g_curlInterface.easy_setopt(m_state->m_easyHandle, CURLOPT_PROGRESSFUNCTION, transfer_abort_callback);
#endif
    g_curlInterface.easy_setopt(m_state->m_easyHandle, CURLOPT_NOPROGRESS, 0);

    result = g_curlInterface.easy_perform(m_state->m_easyHandle);

  }

  if( result != CURLE_ABORTED_BY_CALLBACK && result != CURLE_OK )
  {
    g_curlInterface.easy_release(&m_state->m_easyHandle, NULL);
    errno = ENOENT;
    CLog::Log(LOGERROR, "CCurlFile::{} - <{}> Failed: {}({})", __FUNCTION__, url.GetRedacted(),
              g_curlInterface.easy_strerror(result), result);
    return -1;
  }

#if LIBCURL_VERSION_NUM >= 0x073700 // CURL_AT_LEAST_VERSION(0, 7, 55)
  curl_off_t length;
  result = g_curlInterface.easy_getinfo(m_state->m_easyHandle, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T,
                                        &length);
#else
  double length;
  result = g_curlInterface.easy_getinfo(m_state->m_easyHandle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &length);
#endif
  if (result != CURLE_OK || length < 0.0)
  {
    if (url.IsProtocol("ftp"))
    {
      g_curlInterface.easy_release(&m_state->m_easyHandle, NULL);
      CLog::Log(LOGINFO, "CCurlFile::{} - <{}> Content length failed: {}({})", __FUNCTION__,
                url.GetRedacted(), g_curlInterface.easy_strerror(result), result);
      errno = ENOENT;
      return -1;
    }
    else
      length = 0.0;
  }

  SetCorrectHeaders(m_state);

  if (buffer)
  {
    *buffer = {};
    buffer->st_size = static_cast<int64_t>(length);

    // Note: CURLINFO_CONTENT_TYPE returns the last received content-type response header value.
    // In case there is authentication required there might be multiple requests involved and if
    // the last request which actually returns the data does not return a content-type header, but
    // one of the preceding requests, CURLINFO_CONTENT_TYPE returns not the content type of the
    // actual resource requested! m_state contains only the values of the last request, which is
    // what we want here.
    const std::string mimeType = m_state->m_httpheader.GetMimeType();
    if (mimeType.find("text/html") != std::string::npos) // consider html files directories
      buffer->st_mode = _S_IFDIR;
    else
      buffer->st_mode = _S_IFREG;

    long filetime;
    result = g_curlInterface.easy_getinfo(m_state->m_easyHandle, CURLINFO_FILETIME, &filetime);
    if (result != CURLE_OK)
    {
      CLog::Log(LOGINFO, "CCurlFile::{} - <{}> Filetime failed: {}({})", __FUNCTION__,
                url.GetRedacted(), g_curlInterface.easy_strerror(result), result);
    }
    else
    {
      if (filetime != -1)
        buffer->st_mtime = filetime;
    }
  }
  g_curlInterface.easy_release(&m_state->m_easyHandle, NULL);
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
  if (m_buffer.ReadData((char *)lpBuf, want))
  {
    m_filePos += want;
    return want;
  }

  /* check if we finished prematurely */
  if (!m_stillRunning && (m_fileSize == 0 || m_filePos != m_fileSize))
  {
    CLog::Log(LOGWARNING,
              "CCurlFile::CReadState::{} - ({}) Transfer ended before entire file was retrieved "
              "pos {}, size {}",
              __FUNCTION__, fmt::ptr(this), m_filePos, m_fileSize);
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
  // doesn't exceed required size already
  while (m_buffer.getMaxReadSize() < want && m_buffer.getMaxWriteSize() > 0 )
  {
    if (m_cancelled)
      return FILLBUFFER_NO_DATA;

    /* if there is data in overflow buffer, try to use that first */
    if (m_overflowSize)
    {
      unsigned amount = std::min(m_buffer.getMaxWriteSize(), m_overflowSize);
      m_buffer.WriteData(m_overflowBuffer, amount);

      if (amount < m_overflowSize)
        memmove(m_overflowBuffer, m_overflowBuffer + amount, m_overflowSize - amount);

      m_overflowSize -= amount;
      // Shrink memory:
      m_overflowBuffer = (char*)realloc_simple(m_overflowBuffer, m_overflowSize);
      continue;
    }

    CURLMcode result = g_curlInterface.multi_perform(m_multiHandle, &m_stillRunning);
    if (!m_stillRunning)
    {
      if (result == CURLM_OK)
      {
        /* if we still have stuff in buffer, we are fine */
        if (m_buffer.getMaxReadSize())
          return FILLBUFFER_OK;

        // check for errors
        int msgs;
        CURLMsg* msg;
        bool bRetryNow = true;
        bool bError = false;
        while ((msg = g_curlInterface.multi_info_read(m_multiHandle, &msgs)))
        {
          if (msg->msg == CURLMSG_DONE)
          {
            if (msg->data.result == CURLE_OK)
              return FILLBUFFER_OK;

            long httpCode = 0;
            if (msg->data.result == CURLE_HTTP_RETURNED_ERROR)
            {
              g_curlInterface.easy_getinfo(msg->easy_handle, CURLINFO_RESPONSE_CODE, &httpCode);

              // Don't log 404 not-found errors to prevent log-spam
              if (httpCode != 404)
                CLog::Log(LOGERROR,
                          "CCurlFile::CReadState::{} - ({}) Failed: HTTP returned code {}",
                          __FUNCTION__, fmt::ptr(this), httpCode);
            }
            else
            {
              CLog::Log(LOGERROR, "CCurlFile::CReadState::{} - ({}) Failed: {}({})", __FUNCTION__,
                        fmt::ptr(this), g_curlInterface.easy_strerror(msg->data.result),
                        msg->data.result);
            }

            if ( (msg->data.result == CURLE_OPERATION_TIMEDOUT ||
                  msg->data.result == CURLE_PARTIAL_FILE       ||
                  msg->data.result == CURLE_COULDNT_CONNECT    ||
                  msg->data.result == CURLE_RECV_ERROR)        &&
                  !m_bFirstLoop)
            {
              bRetryNow = false; // Leave it to caller whether the operation is retried
              bError = true;
            }
            else if ( (msg->data.result == CURLE_HTTP_RANGE_ERROR              ||
                       httpCode == 416 /* = Requested Range Not Satisfiable */ ||
                       httpCode == 406 /* = Not Acceptable (fixes issues with non compliant HDHomerun servers */) &&
                       m_bFirstLoop                                   &&
                       m_filePos == 0                                 &&
                       m_sendRange)
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
        }

        // Check for an actual error, if not, just return no-data
        if (!bError && !m_bLastError)
          return FILLBUFFER_NO_DATA;

        // Close handle
        if (m_multiHandle && m_easyHandle)
          g_curlInterface.multi_remove_handle(m_multiHandle, m_easyHandle);

        // Reset all the stuff like we would in Disconnect()
        m_buffer.Clear();
        free(m_overflowBuffer);
        m_overflowBuffer = NULL;
        m_overflowSize = 0;
        m_bLastError = true; // Flag error for the next run

        // Retry immediately or leave it up to the caller?
        if ((m_bRetry && retry < CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_curlretries) || (bRetryNow && retry == 0))
        {
          retry++;

          // Connect + seek to current position (again)
          SetResume();
          g_curlInterface.multi_add_handle(m_multiHandle, m_easyHandle);

          CLog::Log(LOGWARNING, "CCurlFile::CReadState::{} - ({}) Reconnect, (re)try {}",
                    __FUNCTION__, fmt::ptr(this), retry);

          // Return to the beginning of the loop:
          continue;
        }

        return FILLBUFFER_NO_DATA; // We failed but flag no data to caller, so it can retry the operation
      }
      return FILLBUFFER_FAIL;
    }

    // We've finished out first loop
    if(m_bFirstLoop && m_buffer.getMaxReadSize() > 0)
      m_bFirstLoop = false;

    // No error this run
    m_bLastError = false;

    switch (result)
    {
      case CURLM_OK:
      {
        int maxfd = -1;
        FD_ZERO(&fdread);
        FD_ZERO(&fdwrite);
        FD_ZERO(&fdexcep);

        // get file descriptors from the transfers
        g_curlInterface.multi_fdset(m_multiHandle, &fdread, &fdwrite, &fdexcep, &maxfd);

        long timeout = 0;
        if (CURLM_OK != g_curlInterface.multi_timeout(m_multiHandle, &timeout) || timeout == -1 || timeout < 200)
          timeout = 200;

        XbmcThreads::EndTime<> endTime{std::chrono::milliseconds(timeout)};
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
            KODI::TIME::Sleep(100ms);
            rc = 0;
#else
            /* Portable sleep for platforms other than Windows. */
            struct timeval wait = { 0, 100 * 1000 }; /* 100ms */
            rc = select(0, NULL, NULL, NULL, &wait);
#endif
          }
          else
          {
            unsigned int time_left = endTime.GetTimeLeft().count();
            struct timeval wait = { (int)time_left / 1000, ((int)time_left % 1000) * 1000 };
            rc = select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &wait);
          }
#ifdef TARGET_WINDOWS
        } while(rc == SOCKET_ERROR && WSAGetLastError() == WSAEINTR);
#else
        } while(rc == SOCKET_ERROR && errno == EINTR);
#endif

        if(rc == SOCKET_ERROR)
        {
#ifdef TARGET_WINDOWS
          char buf[256];
          strerror_s(buf, 256, WSAGetLastError());
          CLog::Log(LOGERROR, "CCurlFile::CReadState::{} - ({}) Failed with socket error:{}",
                    __FUNCTION__, fmt::ptr(this), buf);
#else
          char const * str = strerror(errno);
          CLog::Log(LOGERROR, "CCurlFile::CReadState::{} - ({}) Failed with socket error:{}",
                    __FUNCTION__, fmt::ptr(this), str);
#endif

          return FILLBUFFER_FAIL;
        }
      }
      break;
      case CURLM_CALL_MULTI_PERFORM:
      {
        // we don't keep calling here as that can easily overwrite our buffer which we want to avoid
        // docs says we should call it soon after, but as long as we are reading data somewhere
        // this aught to be soon enough. should stay in socket otherwise
        continue;
      }
      break;
      default:
      {
        CLog::Log(LOGERROR,
                  "CCurlFile::CReadState::{} - ({}) Multi perform failed with code {}, aborting",
                  __FUNCTION__, fmt::ptr(this), result);
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
  m_requestheaders[header] = std::to_string(value);
}

std::string CCurlFile::GetURL(void)
{
  return m_url;
}

std::string CCurlFile::GetRedirectURL()
{
  return GetInfoString(CURLINFO_REDIRECT_URL);
}

std::string CCurlFile::GetInfoString(int infoType)
{
  char* info{};
  CURLcode result = g_curlInterface.easy_getinfo(m_state->m_easyHandle, static_cast<CURLINFO> (infoType), &info);
  if (result != CURLE_OK)
  {
    CLog::Log(LOGERROR,
              "CCurlFile::{} - <{}> Info string request for type {} failed with result code {}",
              __FUNCTION__, CURL::GetRedacted(m_url), infoType, result);
    return "";
  }
  return (info ? info : "");
}

/* STATIC FUNCTIONS */
bool CCurlFile::GetHttpHeader(const CURL &url, CHttpHeader &headers)
{
  try
  {
    CCurlFile file;
    if(file.Stat(url, NULL) == 0)
    {
      headers = file.GetHttpHeader();
      return true;
    }
    return false;
  }
  catch(...)
  {
    CLog::Log(LOGERROR, "CCurlFile::{} - <{}> Exception thrown while trying to retrieve header",
              __FUNCTION__, url.GetRedacted());
    return false;
  }
}

bool CCurlFile::GetMimeType(const CURL &url, std::string &content, const std::string &useragent)
{
  CCurlFile file;
  if (!useragent.empty())
    file.SetUserAgent(useragent);

  struct __stat64 buffer;
  std::string redactUrl = url.GetRedacted();
  if( file.Stat(url, &buffer) == 0 )
  {
    if (buffer.st_mode == _S_IFDIR)
      content = "x-directory/normal";
    else
      content = file.GetProperty(XFILE::FILE_PROPERTY_MIME_TYPE);
    CLog::Log(LOGDEBUG, "CCurlFile::{} - <{}> -> {}", __FUNCTION__, redactUrl, content);
    return true;
  }
  CLog::Log(LOGDEBUG, "CCurlFile::{} - <{}> -> failed", __FUNCTION__, redactUrl);
  content.clear();
  return false;
}

bool CCurlFile::GetContentType(const CURL &url, std::string &content, const std::string &useragent)
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
    CLog::Log(LOGDEBUG, "CCurlFile::{} - <{}> -> {}", __FUNCTION__, redactUrl, content);
    return true;
  }
  CLog::Log(LOGDEBUG, "CCurlFile::{} - <{}> -> failed", __FUNCTION__, redactUrl);
  content.clear();
  return false;
}

bool CCurlFile::GetCookies(const CURL &url, std::string &cookies)
{
  std::string cookiesStr;
  curl_slist* curlCookies;
  CURL_HANDLE* easyHandle;
  CURLM* multiHandle;

  // get the cookies list
  g_curlInterface.easy_acquire(url.GetProtocol().c_str(),
                              url.GetHostName().c_str(),
                              &easyHandle, &multiHandle);
  if (CURLE_OK == g_curlInterface.easy_getinfo(easyHandle, CURLINFO_COOKIELIST, &curlCookies))
  {
    // iterate over each cookie and format it into an RFC 2109 formatted Set-Cookie string
    curl_slist* curlCookieIter = curlCookies;
    while(curlCookieIter)
    {
      // tokenize the CURL cookie string
      std::vector<std::string> valuesVec;
      StringUtils::Tokenize(curlCookieIter->data, valuesVec, "\t");

      // ensure the length is valid
      if (valuesVec.size() < 7)
      {
        CLog::Log(LOGERROR, "CCurlFile::{} - <{}> Invalid cookie: '{}'", __FUNCTION__,
                  url.GetRedacted(), curlCookieIter->data);
        curlCookieIter = curlCookieIter->next;
        continue;
      }

      // create a http-header formatted cookie string
      std::string cookieStr = valuesVec[5] + "=" + valuesVec[6] +
                              "; path=" + valuesVec[2] +
                              "; domain=" + valuesVec[0];

      // append this cookie to the string containing all cookies
      if (!cookiesStr.empty())
        cookiesStr += "\n";
      cookiesStr += cookieStr;

      // move on to the next cookie
      curlCookieIter = curlCookieIter->next;
    }

    // free the curl cookies
    g_curlInterface.slist_free_all(curlCookies);

    // release our handles
    g_curlInterface.easy_release(&easyHandle, &multiHandle);

    // if we have a non-empty cookie string, return it
    if (!cookiesStr.empty())
    {
      cookies = cookiesStr;
      return true;
    }
  }

  // no cookies to return
  return false;
}

int CCurlFile::IoControl(EIoControl request, void* param)
{
  if (request == IOCTRL_SEEK_POSSIBLE)
    return m_seekable ? 1 : 0;

  if (request == IOCTRL_SET_RETRY)
  {
    m_allowRetry = *(bool*) param;
    return 0;
  }

  return -1;
}

const std::string CCurlFile::GetProperty(XFILE::FileProperty type, const std::string &name) const
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
    char *url = nullptr;
    g_curlInterface.easy_getinfo(m_state->m_easyHandle, CURLINFO_EFFECTIVE_URL, &url);
    return url ? url : "";
  }
  default:
    return "";
  }
}

const std::vector<std::string> CCurlFile::GetPropertyValues(XFILE::FileProperty type, const std::string &name) const
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
#if LIBCURL_VERSION_NUM >= 0x073a00 // 0.7.58.0
  curl_off_t speed = 0;
  if (g_curlInterface.easy_getinfo(m_state->m_easyHandle, CURLINFO_SPEED_DOWNLOAD_T, &speed) ==
      CURLE_OK)
    return speed;
#else
  double time = 0.0, size = 0.0;
  if (g_curlInterface.easy_getinfo(m_state->m_easyHandle, CURLINFO_TOTAL_TIME, &time) == CURLE_OK
    && g_curlInterface.easy_getinfo(m_state->m_easyHandle, CURLINFO_SIZE_DOWNLOAD, &size) == CURLE_OK
    && time > 0.0)
  {
    return size / time;
  }
#endif
  return 0.0;
}
