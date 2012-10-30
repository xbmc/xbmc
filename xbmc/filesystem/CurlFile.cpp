/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "CurlFile.h"
#include "utils/URIUtils.h"
#include "Util.h"
#include "URL.h"
#include "settings/AdvancedSettings.h"
#include "settings/GUISettings.h"
#include "settings/Settings.h"
#include "File.h"

#include <vector>
#include <climits>

#ifdef _LINUX
#include <errno.h>
#include <inttypes.h>
#include "../linux/XFileUtils.h"
#include "../linux/XTimeUtils.h"
#include "../linux/ConvUtils.h"
#endif

#include "DllLibCurl.h"
#include "ShoutcastFile.h"
#include "SpecialProtocol.h"
#include "utils/CharsetConverter.h"
#include "utils/log.h"

using namespace XFILE;
using namespace XCURL;

#define XMIN(a,b) ((a)<(b)?(a):(b))
#define FITS_INT(a) (((a) <= INT_MAX) && ((a) >= INT_MIN))

#define dllselect select

// curl calls this routine to debug
extern "C" int debug_callback(CURL_HANDLE *handle, curl_infotype info, char *output, size_t size, void *data)
{
  if (info == CURLINFO_DATA_IN || info == CURLINFO_DATA_OUT)
    return 0;

  // Only shown cURL debug into with loglevel DEBUG_SAMBA or higher
  if( g_advancedSettings.m_logLevel < LOG_LEVEL_DEBUG_SAMBA )
    return 0;

  CStdString strLine;
  strLine.append(output, size);
  std::vector<CStdString> vecLines;
  CUtil::Tokenize(strLine, vecLines, "\r\n");
  std::vector<CStdString>::const_iterator it = vecLines.begin();

  while (it != vecLines.end()) {
    CLog::Log(LOGDEBUG, "Curl::Debug %s", (*it).c_str());
    it++;
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

extern "C" size_t header_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
  CCurlFile::CReadState *state = (CCurlFile::CReadState *)stream;
  return state->HeaderCallback(ptr, size, nmemb);
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

size_t CCurlFile::CReadState::HeaderCallback(void *ptr, size_t size, size_t nmemb)
{
  // clear any previous header
  if(m_headerdone)
  {
    m_httpheader.Clear();
    m_headerdone = false;
  }

  // libcurl doc says that this info is not always \0 terminated
  char* strData = (char*)ptr;
  int iSize = size * nmemb;

  if (strData[iSize] != 0)
  {
    strData = (char*)malloc(iSize + 1);
    strncpy(strData, (char*)ptr, iSize);
    strData[iSize] = 0;
  }
  else strData = strdup((char*)ptr);

  if(strcmp(strData, "\r\n") == 0)
    m_headerdone = true;

  m_httpheader.Parse(strData);

  free(strData);

  return iSize;
}

size_t CCurlFile::CReadState::WriteCallback(char *buffer, size_t size, size_t nitems)
{
  unsigned int amount = size * nitems;
//  CLog::Log(LOGDEBUG, "CCurlFile::WriteCallback (%p) with %i bytes, readsize = %i, writesize = %i", this, amount, m_buffer.getMaxReadSize(), m_buffer.getMaxWriteSize() - m_overflowSize);
  if (m_overflowSize)
  {
    // we have our overflow buffer - first get rid of as much as we can
    unsigned int maxWriteable = XMIN((unsigned int)m_buffer.getMaxWriteSize(), m_overflowSize);
    if (maxWriteable)
    {
      if (!m_buffer.WriteData(m_overflowBuffer, maxWriteable))
        CLog::Log(LOGERROR, "Unable to write to buffer - what's up?");
      if (m_overflowSize > maxWriteable)
      { // still have some more - copy it down
        memmove(m_overflowBuffer, m_overflowBuffer + maxWriteable, m_overflowSize - maxWriteable);
      }
      m_overflowSize -= maxWriteable;
    }
  }
  // ok, now copy the data into our ring buffer
  unsigned int maxWriteable = XMIN((unsigned int)m_buffer.getMaxWriteSize(), amount);
  if (maxWriteable)
  {
    if (!m_buffer.WriteData(buffer, maxWriteable))
    {
      CLog::Log(LOGERROR, "%s - Unable to write to buffer with %i bytes - what's up?", __FUNCTION__, maxWriteable);
    }
    else
    {
      amount -= maxWriteable;
      buffer += maxWriteable;
    }
  }
  if (amount)
  {
//    CLog::Log(LOGDEBUG, "CCurlFile::WriteCallback(%p) not enough free space for %i bytes", (void*)this,  amount);

    m_overflowBuffer = (char*)realloc_simple(m_overflowBuffer, amount + m_overflowSize);
    if(m_overflowBuffer == NULL)
    {
      CLog::Log(LOGWARNING, "%s - Failed to grow overflow buffer from %i bytes to %i bytes", __FUNCTION__, m_overflowSize, amount + m_overflowSize);
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
  m_filePos = 0;
  m_fileSize = 0;
  m_bufferSize = 0;
  m_cancelled = false;
  m_bFirstLoop = true;
  m_headerdone = false;
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
    if(!FillBuffer(m_bufferSize))
    {
      if(!m_buffer.SkipBytes(-len))
        CLog::Log(LOGERROR, "%s - Failed to restore position after failed fill", __FUNCTION__);
      else
        m_filePos -= len;
      return false;
    }

    if(!FITS_INT(pos - m_filePos) || !m_buffer.SkipBytes((int)(pos - m_filePos)))
    {
      CLog::Log(LOGERROR, "%s - Failed to skip to position after having filled buffer", __FUNCTION__);
      if(!m_buffer.SkipBytes(-len))
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

long CCurlFile::CReadState::Connect(unsigned int size)
{
  g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_RESUME_FROM_LARGE, m_filePos);
  g_curlInterface.multi_add_handle(m_multiHandle, m_easyHandle);

  m_bufferSize = size;
  m_buffer.Destroy();
  m_buffer.Create(size * 3);
  m_headerdone = false;

  // read some data in to try and obtain the length
  // maybe there's a better way to get this info??
  m_stillRunning = 1;
  if (!FillBuffer(1))
  {
    CLog::Log(LOGERROR, "CCurlFile::CReadState::Open, didn't get any data from stream.");
    return -1;
  }

  double length;
  if (CURLE_OK == g_curlInterface.easy_getinfo(m_easyHandle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &length))
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
}


CCurlFile::~CCurlFile()
{
  if (m_opened)
    Close();
  delete m_state;
  g_curlInterface.Unload();
}

CCurlFile::CCurlFile()
{
  g_curlInterface.Load(); // loads the curl dll and resolves exports etc.
  m_curlAliasList = NULL;
  m_curlHeaderList = NULL;
  m_opened = false;
  m_multisession  = true;
  m_seekable = true;
  m_useOldHttpVersion = false;
  m_connecttimeout = 0;
  m_lowspeedtime = 0;
  m_ftpauth = "";
  m_ftpport = "";
  m_ftppasvip = false;
  m_bufferSize = 32768;
  m_binary = true;
  m_postdata = "";
  m_postdataset = false;
  m_username = "";
  m_password = "";
  m_httpauth = "";
  m_state = new CReadState();
  m_skipshout = false;
  m_httpresponse = -1;
}

//Has to be called before Open()
void CCurlFile::SetBufferSize(unsigned int size)
{
  m_bufferSize = size;
}

void CCurlFile::Close()
{
  m_state->Disconnect();

  m_url.Empty();
  m_referer.Empty();
  m_cookie.Empty();

  /* cleanup */
  if( m_curlAliasList )
    g_curlInterface.slist_free_all(m_curlAliasList);
  if( m_curlHeaderList )
    g_curlInterface.slist_free_all(m_curlHeaderList);

  m_curlAliasList = NULL;
  m_curlHeaderList = NULL;
  m_opened = false;
}

void CCurlFile::SetCommonOptions(CReadState* state)
{
  CURL_HANDLE* h = state->m_easyHandle;

  g_curlInterface.easy_reset(h);

  g_curlInterface.easy_setopt(h, CURLOPT_DEBUGFUNCTION, debug_callback);

  if( g_advancedSettings.m_logLevel >= LOG_LEVEL_DEBUG )
    g_curlInterface.easy_setopt(h, CURLOPT_VERBOSE, TRUE);
  else
    g_curlInterface.easy_setopt(h, CURLOPT_VERBOSE, FALSE);

  g_curlInterface.easy_setopt(h, CURLOPT_WRITEDATA, state);
  g_curlInterface.easy_setopt(h, CURLOPT_WRITEFUNCTION, write_callback);

  // set username and password for current handle
  if (m_username.length() > 0 && m_password.length() > 0)
  {
    CStdString userpwd = m_username + ":" + m_password;
    g_curlInterface.easy_setopt(h, CURLOPT_USERPWD, userpwd.c_str());
  }

  // make sure headers are seperated from the data stream
  g_curlInterface.easy_setopt(h, CURLOPT_WRITEHEADER, state);
  g_curlInterface.easy_setopt(h, CURLOPT_HEADERFUNCTION, header_callback);
  g_curlInterface.easy_setopt(h, CURLOPT_HEADER, FALSE);

  g_curlInterface.easy_setopt(h, CURLOPT_FTP_USE_EPSV, 0); // turn off epsv

  // Allow us to follow two redirects
  g_curlInterface.easy_setopt(h, CURLOPT_FOLLOWLOCATION, TRUE);
  g_curlInterface.easy_setopt(h, CURLOPT_MAXREDIRS, 5);

  // Enable cookie engine for current handle to re-use them in future requests
  CStdString strCookieFile;
  CStdString strTempPath = CSpecialProtocol::TranslatePath(g_advancedSettings.m_cachePath);
  URIUtils::AddFileToFolder(strTempPath, "cookies.dat", strCookieFile);

  g_curlInterface.easy_setopt(h, CURLOPT_COOKIEFILE, strCookieFile.c_str());
  g_curlInterface.easy_setopt(h, CURLOPT_COOKIEJAR, strCookieFile.c_str());

  // Set custom cookie if requested
  if (!m_cookie.IsEmpty())
    g_curlInterface.easy_setopt(h, CURLOPT_COOKIE, m_cookie.c_str());

  g_curlInterface.easy_setopt(h, CURLOPT_COOKIELIST, "FLUSH");

  // When using multiple threads you should set the CURLOPT_NOSIGNAL option to
  // TRUE for all handles. Everything will work fine except that timeouts are not
  // honored during the DNS lookup - which you can work around by building libcurl
  // with c-ares support. c-ares is a library that provides asynchronous name
  // resolves. Unfortunately, c-ares does not yet support IPv6.
  g_curlInterface.easy_setopt(h, CURLOPT_NOSIGNAL, TRUE);

  // not interested in failed requests
  g_curlInterface.easy_setopt(h, CURLOPT_FAILONERROR, 1);

  // enable support for icecast / shoutcast streams
  m_curlAliasList = g_curlInterface.slist_append(m_curlAliasList, "ICY 200 OK");
  g_curlInterface.easy_setopt(h, CURLOPT_HTTP200ALIASES, m_curlAliasList);

  // never verify peer, we don't have any certificates to do this
  g_curlInterface.easy_setopt(h, CURLOPT_SSL_VERIFYPEER, 0);
  g_curlInterface.easy_setopt(h, CURLOPT_SSL_VERIFYHOST, 0);

  g_curlInterface.easy_setopt(m_state->m_easyHandle, CURLOPT_URL, m_url.c_str());
  g_curlInterface.easy_setopt(m_state->m_easyHandle, CURLOPT_TRANSFERTEXT, FALSE);

  // setup POST data if it is set (and it may be empty)
  if (m_postdataset)
  {
    g_curlInterface.easy_setopt(h, CURLOPT_POST, 1 );
    g_curlInterface.easy_setopt(h, CURLOPT_POSTFIELDSIZE, m_postdata.length());
    g_curlInterface.easy_setopt(h, CURLOPT_POSTFIELDS, m_postdata.c_str());
  }

  // setup Referer header if needed
  if (!m_referer.IsEmpty())
    g_curlInterface.easy_setopt(h, CURLOPT_REFERER, m_referer.c_str());
  else
  {
    g_curlInterface.easy_setopt(h, CURLOPT_REFERER, NULL);
    g_curlInterface.easy_setopt(h, CURLOPT_AUTOREFERER, TRUE);
  }

  // setup any requested authentication
  if( m_ftpauth.length() > 0 )
  {
    g_curlInterface.easy_setopt(h, CURLOPT_FTP_SSL, CURLFTPSSL_TRY);
    if( m_ftpauth.Equals("any") )
      g_curlInterface.easy_setopt(h, CURLOPT_FTPSSLAUTH, CURLFTPAUTH_DEFAULT);
    else if( m_ftpauth.Equals("ssl") )
      g_curlInterface.easy_setopt(h, CURLOPT_FTPSSLAUTH, CURLFTPAUTH_SSL);
    else if( m_ftpauth.Equals("tls") )
      g_curlInterface.easy_setopt(h, CURLOPT_FTPSSLAUTH, CURLFTPAUTH_TLS);
  }

  // setup requested http authentication method
  if(m_httpauth.length() > 0)
  {
    if( m_httpauth.Equals("any") )
      g_curlInterface.easy_setopt(h, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
    else if( m_httpauth.Equals("anysafe") )
      g_curlInterface.easy_setopt(h, CURLOPT_HTTPAUTH, CURLAUTH_ANYSAFE);
    else if( m_httpauth.Equals("digest") )
      g_curlInterface.easy_setopt(h, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
    else if( m_httpauth.Equals("ntlm") )
      g_curlInterface.easy_setopt(h, CURLOPT_HTTPAUTH, CURLAUTH_NTLM);
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

  // setup Content-Encoding if requested
  if( m_contentencoding.length() > 0 )
    g_curlInterface.easy_setopt(h, CURLOPT_ENCODING, m_contentencoding.c_str());

  if (m_userAgent.length() > 0)
    g_curlInterface.easy_setopt(h, CURLOPT_USERAGENT, m_userAgent.c_str());
  else /* set some default agent as shoutcast doesn't return proper stuff otherwise */
    g_curlInterface.easy_setopt(h, CURLOPT_USERAGENT, g_settings.m_userAgent.c_str());

  if (m_useOldHttpVersion)
    g_curlInterface.easy_setopt(h, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
  else
    SetRequestHeader("Connection", "keep-alive");

  if (g_advancedSettings.m_curlDisableIPV6)
    g_curlInterface.easy_setopt(h, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);

  if (m_proxy.length() > 0)
  {
    g_curlInterface.easy_setopt(h, CURLOPT_PROXY, m_proxy.c_str());
    if (m_proxyuserpass.length() > 0)
      g_curlInterface.easy_setopt(h, CURLOPT_PROXYUSERPWD, m_proxyuserpass.c_str());

  }
  if (m_customrequest.length() > 0)
    g_curlInterface.easy_setopt(h, CURLOPT_CUSTOMREQUEST, m_customrequest.c_str());

  if (m_connecttimeout == 0)
    m_connecttimeout = g_advancedSettings.m_curlconnecttimeout;

  // set our timeouts, we abort connection after m_timeout, and reads after no data for m_timeout seconds
  g_curlInterface.easy_setopt(h, CURLOPT_CONNECTTIMEOUT, m_connecttimeout);

  // We abort in case we transfer less than 1byte/second
  g_curlInterface.easy_setopt(h, CURLOPT_LOW_SPEED_LIMIT, 1);

  if (m_lowspeedtime == 0)
    m_lowspeedtime = g_advancedSettings.m_curllowspeedtime;

  // Set the lowspeed time very low as it seems Curl takes much longer to detect a lowspeed condition
  g_curlInterface.easy_setopt(h, CURLOPT_LOW_SPEED_TIME, m_lowspeedtime);
}

void CCurlFile::SetRequestHeaders(CReadState* state)
{
  if(m_curlHeaderList)
  {
    g_curlInterface.slist_free_all(m_curlHeaderList);
    m_curlHeaderList = NULL;
  }

  MAPHTTPHEADERS::iterator it;
  for(it = m_requestheaders.begin(); it != m_requestheaders.end(); it++)
  {
    CStdString buffer = it->first + ": " + it->second;
    m_curlHeaderList = g_curlInterface.slist_append(m_curlHeaderList, buffer.c_str());
  }

  // add user defined headers
  if (m_curlHeaderList && state->m_easyHandle)
    g_curlInterface.easy_setopt(state->m_easyHandle, CURLOPT_HTTPHEADER, m_curlHeaderList);

}

void CCurlFile::SetCorrectHeaders(CReadState* state)
{
  CHttpHeader& h = state->m_httpheader;
  /* workaround for shoutcast server wich doesn't set content type on standard mp3 */
  if( h.GetMimeType().IsEmpty() )
  {
    if( !h.GetValue("icy-notice1").IsEmpty()
    || !h.GetValue("icy-name").IsEmpty()
    || !h.GetValue("icy-br").IsEmpty() )
    h.Parse("Content-Type: audio/mpeg\r\n");
  }

  /* hack for google video */
  if ( h.GetMimeType().Equals("text/html")
  &&  !h.GetValue("Content-Disposition").IsEmpty() )
  {
    CStdString strValue = h.GetValue("Content-Disposition");
    if (strValue.Find("filename=") > -1 && strValue.Find(".flv") > -1)
      h.Parse("Content-Type: video/flv\r\n");
  }
}

void CCurlFile::ParseAndCorrectUrl(CURL &url2)
{
  CStdString strProtocol = url2.GetTranslatedProtocol();
  url2.SetProtocol(strProtocol);

  if( strProtocol.Equals("ftp")
  ||  strProtocol.Equals("ftps") )
  {
    /* this is uggly, depending on from where   */
    /* we get the link it may or may not be     */
    /* url encoded. if handed from ftpdirectory */
    /* it won't be so let's handle that case    */

    CStdString partial, filename(url2.GetFileName());
    CStdStringArray array;

    /* our current client doesn't support utf8 */
    g_charsetConverter.utf8ToStringCharset(filename);

    /* TODO: create a tokenizer that doesn't skip empty's */
    CUtil::Tokenize(filename, array, "/");
    filename.Empty();
    for(CStdStringArray::iterator it = array.begin(); it != array.end(); it++)
    {
      if(it != array.begin())
        filename += "/";

      partial = *it;
      CURL::Encode(partial);
      filename += partial;
    }

    /* make sure we keep slashes */
    if(url2.GetFileName().Right(1) == "/")
      filename += "/";

    url2.SetFileName(filename);

    CStdString options = url2.GetOptions().Mid(1);
    options.TrimRight('/'); // hack for trailing slashes being added from source

    m_ftpauth = "";
    m_ftpport = "";
    m_ftppasvip = false;

    /* parse options given */
    CUtil::Tokenize(options, array, "&");
    for(CStdStringArray::iterator it = array.begin(); it != array.end(); it++)
    {
      CStdString name, value;
      int pos = it->Find('=');
      if(pos >= 0)
      {
        name = it->Left(pos);
        value = it->Mid(pos+1, it->size());
      }
      else
      {
        name = (*it);
        value = "";
      }

      if(name.Equals("auth"))
      {
        m_ftpauth = value;
        if(m_ftpauth.IsEmpty())
          m_ftpauth = "any";
      }
      else if(name.Equals("active"))
      {
        m_ftpport = value;
        if(value.IsEmpty())
          m_ftpport = "-";
      }
      else if(name.Equals("pasvip"))
      {
        if(value == "0")
          m_ftppasvip = false;
        else
          m_ftppasvip = true;
      }
    }

    /* ftp has no options */
    url2.SetOptions("");
  }
  else if( strProtocol.Equals("http")
       ||  strProtocol.Equals("https"))
  {
    if (g_guiSettings.GetBool("network.usehttpproxy")
        && !g_guiSettings.GetString("network.httpproxyserver").empty()
        && !g_guiSettings.GetString("network.httpproxyport").empty()
        && m_proxy.IsEmpty())
    {
      m_proxy = "http://" + g_guiSettings.GetString("network.httpproxyserver");
      m_proxy += ":" + g_guiSettings.GetString("network.httpproxyport");
      if (g_guiSettings.GetString("network.httpproxyusername").length() > 0 && m_proxyuserpass.IsEmpty())
      {
        m_proxyuserpass = g_guiSettings.GetString("network.httpproxyusername");
        m_proxyuserpass += ":" + g_guiSettings.GetString("network.httpproxypassword");
      }
      CLog::Log(LOGDEBUG, "Using proxy %s", m_proxy.c_str());
    }

    // get username and password
    m_username = url2.GetUserName();
    m_password = url2.GetPassWord();

    // handle any protocol options
    CStdString options = url2.GetProtocolOptions();
    options.TrimRight('/'); // hack for trailing slashes being added from source
    if (options.length() > 0)
    {
      // clear protocol options
      url2.SetProtocolOptions("");
      // set xbmc headers
      CStdStringArray array;
      CUtil::Tokenize(options, array, "&");
      for(CStdStringArray::iterator it = array.begin(); it != array.end(); it++)
      {
        // parse name, value
        CStdString name, value;
        int pos = it->Find('=');
        if(pos >= 0)
        {
          name = it->Left(pos);
          value = it->Mid(pos+1, it->size());
        }
        else
        {
          name = (*it);
          value = "";
        }

        // url decode value
        CURL::Decode(value);

        if(name.Equals("auth"))
        {
          m_httpauth = value;
          if(m_httpauth.IsEmpty())
            m_httpauth = "any";
        }
        else if (name.Equals("Referer"))
          SetReferer(value);
        else if (name.Equals("User-Agent"))
          SetUserAgent(value);
        else if (name.Equals("Cookie"))
          SetCookie(value);
        else if (name.Equals("Encoding"))
          SetContentEncoding(value);
        else if (name.Equals("noshout") && value.Equals("true"))
          m_skipshout = true;
        else
          SetRequestHeader(name, value);
      }
    }
  }

  if (m_username.length() > 0 && m_password.length() > 0)
    m_url = url2.GetWithoutUserDetails();
  else
    m_url = url2.Get();
}

bool CCurlFile::Post(const CStdString& strURL, const CStdString& strPostData, CStdString& strHTML)
{
  m_postdata = strPostData;
  m_postdataset = true;
  return Service(strURL, strHTML);
}

bool CCurlFile::Get(const CStdString& strURL, CStdString& strHTML)
{
  m_postdata = "";
  m_postdataset = false;
  return Service(strURL, strHTML);
}

bool CCurlFile::Service(const CStdString& strURL, CStdString& strHTML)
{
  if (Open(strURL))
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

bool CCurlFile::ReadData(CStdString& strHTML)
{
  int size_read = 0;
  int data_size = 0;
  strHTML = "";
  char buffer[16384];
  while( (size_read = Read(buffer, sizeof(buffer)-1) ) > 0 )
  {
    buffer[size_read] = 0;
    strHTML.append(buffer, size_read);
    data_size += size_read;
  }
  if (m_state->m_cancelled)
    return false;
  return true;
}

bool CCurlFile::Download(const CStdString& strURL, const CStdString& strFileName, LPDWORD pdwSize)
{
  CLog::Log(LOGINFO, "Download: %s->%s", strURL.c_str(), strFileName.c_str());

  CStdString strData;
  if (!Get(strURL, strData))
    return false;

  XFILE::CFile file;
  if (!file.OpenForWrite(strFileName, true))
  {
    CLog::Log(LOGERROR, "Unable to open file %s: %u",
    strFileName.c_str(), GetLastError());
    return false;
  }
  if (strData.size())
    file.Write(strData.c_str(), strData.size());
  file.Close();

  if (pdwSize != NULL)
  {
    *pdwSize = strData.size();
  }

  return true;
}

// Detect whether we are "online" or not! Very simple and dirty!
bool CCurlFile::IsInternet(bool checkDNS /* = true */)
{
  CStdString strURL = "http://www.google.com";
  if (!checkDNS)
    strURL = "http://74.125.19.103"; // www.google.com ip

  bool found = Exists(strURL);
  Close();

  return found;
}

void CCurlFile::Cancel()
{
  m_state->m_cancelled = true;
  while (m_opened)
    Sleep(1);
}

void CCurlFile::Reset()
{
  m_state->m_cancelled = false;
}

bool CCurlFile::Open(const CURL& url)
{

  m_opened = true;

  CURL url2(url);
  ParseAndCorrectUrl(url2);

  CLog::Log(LOGDEBUG, "CurlFile::Open(%p) %s", (void*)this, m_url.c_str());

  ASSERT(!(!m_state->m_easyHandle ^ !m_state->m_multiHandle));
  if( m_state->m_easyHandle == NULL )
    g_curlInterface.easy_aquire(url2.GetProtocol(), url2.GetHostName(), &m_state->m_easyHandle, &m_state->m_multiHandle );

  // setup common curl options
  SetCommonOptions(m_state);
  SetRequestHeaders(m_state);

  m_httpresponse = m_state->Connect(m_bufferSize);
  if( m_httpresponse < 0 || m_httpresponse >= 400)
    return false;

  SetCorrectHeaders(m_state);

  // since we can't know the stream size up front if we're gzipped/deflated
  // flag the stream with an unknown file size rather than the compressed
  // file size.
  if (m_contentencoding.size() > 0)
    m_state->m_fileSize = 0;

  // check if this stream is a shoutcast stream. sometimes checking the protocol line is not enough so examine other headers as well.
  // shoutcast streams should be handled by FileShoutcast.
  if ((m_state->m_httpheader.GetProtoLine().Left(3) == "ICY" || !m_state->m_httpheader.GetValue("icy-notice1").IsEmpty()
     || !m_state->m_httpheader.GetValue("icy-name").IsEmpty()
     || !m_state->m_httpheader.GetValue("icy-br").IsEmpty()) && !m_skipshout)
  {
    CLog::Log(LOGDEBUG,"CurlFile - file <%s> is a shoutcast stream. re-opening", m_url.c_str());
    throw new CRedirectException(new CShoutcastFile);
  }

  m_multisession = false;
  if(m_url.Left(5).Equals("http:") || m_url.Left(6).Equals("https:"))
  {
    m_multisession = true;
    if(m_state->m_httpheader.GetValue("Server").Find("Portable SDK for UPnP devices") >= 0)
    {
      CLog::Log(LOGWARNING, "CurlFile - disabling multi session due to broken libupnp server");
      m_multisession = false;
    }
  }

  if(m_state->m_httpheader.GetValue("Transfer-Encoding").Equals("chunked"))
    m_state->m_fileSize = 0;

  m_seekable = false;
  if(m_state->m_fileSize > 0)
  {
    m_seekable = true;

    if(url2.GetProtocol().Equals("http")
    || url2.GetProtocol().Equals("https"))
    {
      // if server says explicitly it can't seek, respect that
      if(m_state->m_httpheader.GetValue("Accept-Ranges").Equals("none"))
        m_seekable = false;
    }
  }

  char* efurl;
  if (CURLE_OK == g_curlInterface.easy_getinfo(m_state->m_easyHandle, CURLINFO_EFFECTIVE_URL,&efurl) && efurl)
    m_url = efurl;

  return true;
}

bool CCurlFile::CReadState::ReadString(char *szLine, int iLineLength)
{
  unsigned int want = (unsigned int)iLineLength;

  if((m_fileSize == 0 || m_filePos < m_fileSize) && !FillBuffer(want))
    return false;

  // ensure only available data is considered
  want = XMIN((unsigned int)m_buffer.getMaxReadSize(), want);

  /* check if we finished prematurely */
  if (!m_stillRunning && (m_fileSize == 0 || m_filePos != m_fileSize) && !want)
  {
    if (m_fileSize != 0)
      CLog::Log(LOGWARNING, "%s - Transfer ended before entire file was retrieved pos %"PRId64", size %"PRId64, __FUNCTION__, m_filePos, m_fileSize);

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
  return (bool)((pLine - szLine) > 0);
}

bool CCurlFile::Exists(const CURL& url)
{
  // if file is already running, get info from it
  if( m_opened )
  {
    CLog::Log(LOGWARNING, "%s - Exist called on open file", __FUNCTION__);
    return true;
  }

  CURL url2(url);
  ParseAndCorrectUrl(url2);

  ASSERT(m_state->m_easyHandle == NULL);
  g_curlInterface.easy_aquire(url2.GetProtocol(), url2.GetHostName(), &m_state->m_easyHandle, NULL);

  SetCommonOptions(m_state);
  SetRequestHeaders(m_state);
  g_curlInterface.easy_setopt(m_state->m_easyHandle, CURLOPT_TIMEOUT, 5);
  g_curlInterface.easy_setopt(m_state->m_easyHandle, CURLOPT_NOBODY, 1);
  g_curlInterface.easy_setopt(m_state->m_easyHandle, CURLOPT_WRITEDATA, NULL); /* will cause write failure*/

  if(url2.GetProtocol() == "ftp")
  {
    g_curlInterface.easy_setopt(m_state->m_easyHandle, CURLOPT_FILETIME, 1);
    g_curlInterface.easy_setopt(m_state->m_easyHandle, CURLOPT_FTP_FILEMETHOD, CURLFTPMETHOD_NOCWD);
  }

  CURLcode result = g_curlInterface.easy_perform(m_state->m_easyHandle);
  g_curlInterface.easy_release(&m_state->m_easyHandle, NULL);

  if (result == CURLE_WRITE_ERROR || result == CURLE_OK)
    return true;

  errno = ENOENT;
  return false;
}

int64_t CCurlFile::Seek(int64_t iFilePosition, int iWhence)
{
  int64_t nextPos = m_state->m_filePos;
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

  if(!m_seekable)
    return -1;

  CReadState* oldstate = NULL;
  if(m_multisession)
  {
    CURL url(m_url);
    oldstate = m_state;
    m_state = new CReadState();

    g_curlInterface.easy_aquire(url.GetProtocol(), url.GetHostName(), &m_state->m_easyHandle, &m_state->m_multiHandle );

    // setup common curl options
    SetCommonOptions(m_state);
  }
  else
    m_state->Disconnect();

  /* caller might have changed some headers (needed for daap)*/
  SetRequestHeaders(m_state);

  m_state->m_filePos = nextPos;
  if (oldstate)
    m_state->m_fileSize = oldstate->m_fileSize;

  long response = m_state->Connect(m_bufferSize);
  if(response < 0 && (m_state->m_fileSize == 0 || m_state->m_fileSize != m_state->m_filePos))
  {
    m_seekable = false;
    if(oldstate)
    {
      delete m_state;
      m_state = oldstate;
    }
    return -1;
  }

  SetCorrectHeaders(m_state);
  delete oldstate;

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
    CLog::Log(LOGWARNING, "%s - Stat called on open file", __FUNCTION__);
    if (buffer)
    {
      memset(buffer, 0, sizeof(struct __stat64));
      buffer->st_size = GetLength();
      buffer->st_mode = _S_IFREG;
    }
    return 0;
  }

  CURL url2(url);
  ParseAndCorrectUrl(url2);

  ASSERT(m_state->m_easyHandle == NULL);
  g_curlInterface.easy_aquire(url2.GetProtocol(), url2.GetHostName(), &m_state->m_easyHandle, NULL);

  SetCommonOptions(m_state);
  SetRequestHeaders(m_state);
  g_curlInterface.easy_setopt(m_state->m_easyHandle, CURLOPT_TIMEOUT, 5);
  g_curlInterface.easy_setopt(m_state->m_easyHandle, CURLOPT_NOBODY, 1);
  g_curlInterface.easy_setopt(m_state->m_easyHandle, CURLOPT_WRITEDATA, NULL); /* will cause write failure*/
  g_curlInterface.easy_setopt(m_state->m_easyHandle, CURLOPT_FILETIME , 1); 

  if(url2.GetProtocol() == "ftp")
  {
    g_curlInterface.easy_setopt(m_state->m_easyHandle, CURLOPT_FILETIME, 1);
    g_curlInterface.easy_setopt(m_state->m_easyHandle, CURLOPT_FTP_FILEMETHOD, CURLFTPMETHOD_NOCWD);
  }

  CURLcode result = g_curlInterface.easy_perform(m_state->m_easyHandle);


  if(result == CURLE_HTTP_RETURNED_ERROR)
  {
    long code;
    if(g_curlInterface.easy_getinfo(m_state->m_easyHandle, CURLINFO_RESPONSE_CODE, &code) == CURLE_OK && code == 404 )
      return -1;
  }

  if(result == CURLE_GOT_NOTHING 
  || result == CURLE_HTTP_RETURNED_ERROR 
  || result == CURLE_RECV_ERROR /* some silly shoutcast servers */ )
  {
    /* some http servers and shoutcast servers don't give us any data on a head request */
    /* request normal and just fail out, it's their loss */
    /* somehow curl doesn't reset CURLOPT_NOBODY properly so reset everything */
    SetCommonOptions(m_state);
    SetRequestHeaders(m_state);
    g_curlInterface.easy_setopt(m_state->m_easyHandle, CURLOPT_TIMEOUT, 5);
    g_curlInterface.easy_setopt(m_state->m_easyHandle, CURLOPT_RANGE, "0-0");
    g_curlInterface.easy_setopt(m_state->m_easyHandle, CURLOPT_WRITEDATA, NULL); /* will cause write failure*/
    g_curlInterface.easy_setopt(m_state->m_easyHandle, CURLOPT_FILETIME , 1); 
    result = g_curlInterface.easy_perform(m_state->m_easyHandle);
  }

  if( result == CURLE_HTTP_RANGE_ERROR )
  {
    /* crap can't use the range option, disable it and try again */
    g_curlInterface.easy_setopt(m_state->m_easyHandle, CURLOPT_RANGE, NULL);
    result = g_curlInterface.easy_perform(m_state->m_easyHandle);
  }

  if( result != CURLE_WRITE_ERROR && result != CURLE_OK )
  {
    g_curlInterface.easy_release(&m_state->m_easyHandle, NULL);
    errno = ENOENT;
    return -1;
  }

  double length;
  if (CURLE_OK != g_curlInterface.easy_getinfo(m_state->m_easyHandle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &length) || length < 0.0)
  {
    if (url.GetProtocol() == "ftp")
    {
      g_curlInterface.easy_release(&m_state->m_easyHandle, NULL);
      errno = ENOENT;
      return -1;
    }
    else
      length = 0.0;
  }

  SetCorrectHeaders(m_state);

  if(buffer)
  {
    char *content;
    if (CURLE_OK != g_curlInterface.easy_getinfo(m_state->m_easyHandle, CURLINFO_CONTENT_TYPE, &content))
    {
      g_curlInterface.easy_release(&m_state->m_easyHandle, NULL);
      errno = ENOENT;
      return -1;
    }
    else
    {
      memset(buffer, 0, sizeof(struct __stat64));
      buffer->st_size = (int64_t)length;
      if(content && strstr(content, "text/html")) //consider html files directories
        buffer->st_mode = _S_IFDIR;
      else
        buffer->st_mode = _S_IFREG;
    }
    long filetime;
    if (CURLE_OK != g_curlInterface.easy_getinfo(m_state->m_easyHandle, CURLINFO_FILETIME, &filetime))
    {
      CLog::Log(LOGWARNING, "%s - Cannot get curl filetime", __FUNCTION__);
    }
    else
    {
      if (filetime != -1)
      {
        CLog::Log(LOGDEBUG, "%s - curl filetime: %ld", __FUNCTION__, filetime);
        buffer->st_mtime = filetime;
      }
    }
  }
  g_curlInterface.easy_release(&m_state->m_easyHandle, NULL);
  return 0;
}

unsigned int CCurlFile::CReadState::Read(void* lpBuf, int64_t uiBufSize)
{
  /* only request 1 byte, for truncated reads (only if not eof) */
  if((m_fileSize == 0 || m_filePos < m_fileSize) && !FillBuffer(1))
    return 0;

  /* ensure only available data is considered */
  unsigned int want = (unsigned int)XMIN(m_buffer.getMaxReadSize(), uiBufSize);

  /* xfer data to caller */
  if (m_buffer.ReadData((char *)lpBuf, want))
  {
    m_filePos += want;
    return want;
  }

  /* check if we finished prematurely */
  if (!m_stillRunning && (m_fileSize == 0 || m_filePos != m_fileSize))
  {
    CLog::Log(LOGWARNING, "%s - Transfer ended before entire file was retrieved pos %"PRId64", size %"PRId64, __FUNCTION__, m_filePos, m_fileSize);
    return 0;
  }

  return 0;
}

/* use to attempt to fill the read buffer up to requested number of bytes */
bool CCurlFile::CReadState::FillBuffer(unsigned int want)
{
  int retry=0;
  fd_set fdread;
  fd_set fdwrite;
  fd_set fdexcep;

  // only attempt to fill buffer if transactions still running and buffer
  // doesnt exceed required size already
  while ((unsigned int)m_buffer.getMaxReadSize() < want && m_buffer.getMaxWriteSize() > 0 )
  {
    if (m_cancelled)
      return false;

    /* if there is data in overflow buffer, try to use that first */
    if (m_overflowSize)
    {
      unsigned amount = XMIN((unsigned int)m_buffer.getMaxWriteSize(), m_overflowSize);
      m_buffer.WriteData(m_overflowBuffer, amount);

      if (amount < m_overflowSize)
        memcpy(m_overflowBuffer, m_overflowBuffer+amount,m_overflowSize-amount);

      m_overflowSize -= amount;
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
          return true;

        /* verify that we are actually okey */
        int msgs;
        CURLcode CURLresult = CURLE_OK;
        CURLMsg* msg;
        while ((msg = g_curlInterface.multi_info_read(m_multiHandle, &msgs)))
        {
          if (msg->msg == CURLMSG_DONE)
          {
            if (msg->data.result == CURLE_OK)
              return true;

            CLog::Log(LOGWARNING, "%s: curl failed with code %i", __FUNCTION__, msg->data.result);

            // We need to check the data.result here as we don't want to retry on every error
            if ( (msg->data.result == CURLE_OPERATION_TIMEDOUT ||
                  msg->data.result == CURLE_PARTIAL_FILE       ||
                  msg->data.result == CURLE_RECV_ERROR)        &&
                  !m_bFirstLoop)
              CURLresult=msg->data.result;
            else
              return false;
          }
        }

        // Don't retry, when we didn't "see" any error
        if (CURLresult == CURLE_OK)
          return false;

        // Close handle
        if (m_multiHandle && m_easyHandle)
          g_curlInterface.multi_remove_handle(m_multiHandle, m_easyHandle);

        // Reset all the stuff like we would in Disconnect()
        m_buffer.Clear();
        free(m_overflowBuffer);
        m_overflowBuffer = NULL;
        m_overflowSize = 0;

        // If we got here something is wrong
        if (++retry > g_advancedSettings.m_curlretries)
        {
          CLog::Log(LOGWARNING, "%s: Reconnect failed!", __FUNCTION__);
          // Reset the rest of the variables like we would in Disconnect()
          m_filePos = 0;
          m_fileSize = 0;
          m_bufferSize = 0;

          return false;
        }

        CLog::Log(LOGDEBUG, "%s: Reconnect, (re)try %i", __FUNCTION__, retry);

        // Connect + seek to current position (again)
        g_curlInterface.easy_setopt(m_easyHandle, CURLOPT_RESUME_FROM_LARGE, m_filePos);
        g_curlInterface.multi_add_handle(m_multiHandle, m_easyHandle);

        // Return to the beginning of the loop:
        continue;
      }
      return false;
    }

    // We've finished out first loop
    if(m_bFirstLoop && m_buffer.getMaxReadSize() > 0)
      m_bFirstLoop = false;

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
        if (CURLM_OK != g_curlInterface.multi_timeout(m_multiHandle, &timeout) || timeout == -1)
          timeout = 200;

        struct timeval t = { timeout / 1000, (timeout % 1000) * 1000 };

        /* Wait until data is available or a timeout occurs.
           We call dllselect(maxfd + 1, ...), specially in case of (maxfd == -1),
           we call dllselect(0, ...), which is basically equal to sleep. */
        if (SOCKET_ERROR == dllselect(maxfd + 1, &fdread, &fdwrite, &fdexcep, &t))
        {
          CLog::Log(LOGERROR, "%s - curl failed with socket error", __FUNCTION__);
          return false;
        }
      }
      break;
      case CURLM_CALL_MULTI_PERFORM:
      {
        // we don't keep calling here as that can easily overwrite our buffer which we want to avoid
        // docs says we should call it soon after, but aslong as we are reading data somewhere
        // this aught to be soon enough. should stay in socket otherwise
        continue;
      }
      break;
      default:
      {
        CLog::Log(LOGERROR, "%s - curl multi perform failed with code %d, aborting", __FUNCTION__, result);
        return false;
      }
      break;
    }
  }
  return true;
}

void CCurlFile::ClearRequestHeaders()
{
  m_requestheaders.clear();
}

void CCurlFile::SetRequestHeader(CStdString header, CStdString value)
{
  m_requestheaders[header] = value;
}

void CCurlFile::SetRequestHeader(CStdString header, long value)
{
  CStdString buffer;
  buffer.Format("%ld", value);
  m_requestheaders[header] = buffer;
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
    CLog::Log(LOGERROR, "%s - Exception thrown while trying to retrieve header url: %s", __FUNCTION__, url.Get().c_str());
    return false;
  }
}

bool CCurlFile::GetMimeType(const CURL &url, CStdString &content, CStdString useragent)
{
  CCurlFile file;
  if (!useragent.IsEmpty())
    file.SetUserAgent(useragent);

  struct __stat64 buffer;
  if( file.Stat(url, &buffer) == 0 )
  {
    if (buffer.st_mode == _S_IFDIR)
      content = "x-directory/normal";
    else
      content = file.GetMimeType();
    CLog::Log(LOGDEBUG, "CCurlFile::GetMimeType - %s -> %s", url.Get().c_str(), content.c_str());
    return true;
  }
  CLog::Log(LOGDEBUG, "CCurlFile::GetMimeType - %s -> failed", url.Get().c_str());
  content = "";
  return false;
}

int CCurlFile::IoControl(EIoControl request, void* param)
{
  if(request == IOCTRL_SEEK_POSSIBLE)
    return m_seekable ? 1 : 0;

  return -1;
}
