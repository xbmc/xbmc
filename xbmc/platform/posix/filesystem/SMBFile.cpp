/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

// SMBFile.cpp: implementation of the CSMBFile class.
//
//////////////////////////////////////////////////////////////////////

#include "SMBFile.h"

#include "PasswordManager.h"
#include "SMBDirectory.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "commons/Exception.h"
#include "filesystem/SpecialProtocol.h"
#include "network/DNSNameCache.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/TimeUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <cstring>
#include <inttypes.h>
#include <mutex>
#include <regex>

#include <libsmbclient.h>

using namespace XFILE;

void xb_smbc_log(void* private_ptr, int level, const char* msg)
{
  const int logLevel = [level]()
  {
    switch (level)
    {
      case 0:
        return LOGWARNING;
      case 1:
        return LOGINFO;
      default:
        return LOGDEBUG;
    }
  }();

  if (std::strchr(msg, '@'))
  {
    // redact User/pass in URLs
    static const std::regex redact("(\\w+://)\\S+:\\S+@");
    CLog::Log(logLevel, "smb: {}", std::regex_replace(msg, redact, "$1USERNAME:PASSWORD@"));
  }
  else
    CLog::Log(logLevel, "smb: {}", msg);
}

void xb_smbc_auth(const char *srv, const char *shr, char *wg, int wglen,
                  char *un, int unlen, char *pw, int pwlen)
{
}

// WTF is this ?, we get the original server cache only
// to set the server cache to this function which call the
// original one anyway. Seems quite silly.
smbc_get_cached_srv_fn orig_cache;
SMBCSRV* xb_smbc_cache(SMBCCTX* c, const char* server, const char* share, const char* workgroup, const char* username)
{
  return orig_cache(c, server, share, workgroup, username);
}

bool CSMB::IsFirstInit = true;

CSMB::CSMB()
{
  m_context = NULL;
  m_OpenConnections = 0;
  m_IdleTimeout = 0;
}

CSMB::~CSMB()
{
  Deinit();
}

void CSMB::Deinit()
{
  std::unique_lock<CCriticalSection> lock(*this);

  /* samba goes loco if deinited while it has some files opened */
  if (m_context)
  {
    smbc_set_context(NULL);
    smbc_free_context(m_context, 1);
    m_context = NULL;
  }
}

void CSMB::Init()
{
  std::unique_lock<CCriticalSection> lock(*this);

  if (!m_context)
  {
    const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();

    // force libsmbclient to use our own smb.conf by overriding HOME
    std::string truehome(getenv("HOME"));
    setenv("HOME", CSpecialProtocol::TranslatePath("special://home").c_str(), 1);

    // Create ~/.kodi/.smb/smb.conf. This file is used by libsmbclient.
    // http://us1.samba.org/samba/docs/man/manpages-3/libsmbclient.7.html
    // http://us1.samba.org/samba/docs/man/manpages-3/smb.conf.5.html
    std::string smb_conf;
    std::string home(getenv("HOME"));
    URIUtils::RemoveSlashAtEnd(home);
    smb_conf = home + "/.smb";
    int result = mkdir(smb_conf.c_str(), 0755);
    if (result == 0 || (errno == EEXIST && IsFirstInit))
    {
      smb_conf += "/smb.conf";
      FILE* f = fopen(smb_conf.c_str(), "w");
      if (f != NULL)
      {
        fprintf(f, "[global]\n");

        fprintf(f, "\tlock directory = %s/.smb/\n", home.c_str());

        // set minimum smbclient protocol version
        if (settings->GetInt(CSettings::SETTING_SMB_MINPROTOCOL) > 0)
        {
          if (settings->GetInt(CSettings::SETTING_SMB_MINPROTOCOL) == 1)
            fprintf(f, "\tclient min protocol = NT1\n");
          else
            fprintf(f, "\tclient min protocol = SMB%d\n", settings->GetInt(CSettings::SETTING_SMB_MINPROTOCOL));
        }

        // set maximum smbclient protocol version
        if (settings->GetInt(CSettings::SETTING_SMB_MAXPROTOCOL) > 0)
        {
          if (settings->GetInt(CSettings::SETTING_SMB_MAXPROTOCOL) == 1)
            fprintf(f, "\tclient max protocol = NT1\n");
          else
            fprintf(f, "\tclient max protocol = SMB%d\n", settings->GetInt(CSettings::SETTING_SMB_MAXPROTOCOL));
        }

        // set legacy security options
        if (settings->GetBool(CSettings::SETTING_SMB_LEGACYSECURITY) && (settings->GetInt(CSettings::SETTING_SMB_MAXPROTOCOL) == 1))
        {
          fprintf(f, "\tclient NTLMv2 auth = no\n");
          fprintf(f, "\tclient use spnego = no\n");
        }

        // set wins server if there's one. name resolve order defaults to 'lmhosts host wins bcast'.
        // if no WINS server has been specified the wins method will be ignored.
        if (settings->GetString(CSettings::SETTING_SMB_WINSSERVER).length() > 0 && !StringUtils::EqualsNoCase(settings->GetString(CSettings::SETTING_SMB_WINSSERVER), "0.0.0.0") )
        {
          fprintf(f, "\twins server = %s\n", settings->GetString(CSettings::SETTING_SMB_WINSSERVER).c_str());
          fprintf(f, "\tname resolve order = bcast wins host\n");
        }
        else
          fprintf(f, "\tname resolve order = bcast host\n");

        // use user-configured charset. if no charset is specified,
        // samba tries to use charset 850 but falls back to ASCII in case it is not available
        if (CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_sambadoscodepage.length() > 0)
          fprintf(f, "\tdos charset = %s\n", CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_sambadoscodepage.c_str());

        // include users configuration if available
        fprintf(f, "\tinclude = %s/.smb/user.conf\n", home.c_str());

        fclose(f);
      }
    }

    // reads smb.conf so this MUST be after we create smb.conf
    // multiple smbc_init calls are ignored by libsmbclient.
    // note: this is important as it initializes the smb old
    // interface compatibility. Samba 3.4.0 or higher has the new interface.
    // note: we leak the following here once, not sure why yet.
    // 48 bytes -> smb_xmalloc_array
    // 32 bytes -> set_param_opt
    // 16 bytes -> set_param_opt
    smbc_init(xb_smbc_auth, 0);

    // setup our context
    m_context = smbc_new_context();

    // restore HOME
    setenv("HOME", truehome.c_str(), 1);

#ifdef DEPRECATED_SMBC_INTERFACE
    smbc_setDebug(m_context, CServiceBroker::GetLogging().CanLogComponent(LOGSAMBA) ? 10 : 0);
    smbc_setLogCallback(m_context, this, xb_smbc_log);
    smbc_setFunctionAuthData(m_context, xb_smbc_auth);
    orig_cache = smbc_getFunctionGetCachedServer(m_context);
    smbc_setFunctionGetCachedServer(m_context, xb_smbc_cache);
    smbc_setOptionOneSharePerServer(m_context, false);
    smbc_setOptionBrowseMaxLmbCount(m_context, 0);
    smbc_setTimeout(m_context, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_sambaclienttimeout * 1000);
    // we do not need to strdup these, smbc_setXXX below will make their own copies
    if (settings->GetString(CSettings::SETTING_SMB_WORKGROUP).length() > 0)
      //! @bug libsmbclient < 4.9 isn't const correct
      smbc_setWorkgroup(m_context, const_cast<char*>(settings->GetString(CSettings::SETTING_SMB_WORKGROUP).c_str()));
    std::string guest = "guest";
    //! @bug libsmbclient < 4.8 isn't const correct
    smbc_setUser(m_context, const_cast<char*>(guest.c_str()));
#else
    m_context->debug = (CServiceBroker::GetLogging().CanLogComponent(LOGSAMBA) ? 10 : 0);
    m_context->callbacks.auth_fn = xb_smbc_auth;
    orig_cache = m_context->callbacks.get_cached_srv_fn;
    m_context->callbacks.get_cached_srv_fn = xb_smbc_cache;
    m_context->options.one_share_per_server = false;
    m_context->options.browse_max_lmb_count = 0;
    m_context->timeout = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_sambaclienttimeout * 1000;
    // we need to strdup these, they will get free'd on smbc_free_context
    if (settings->GetString(CSettings::SETTING_SMB_WORKGROUP).length() > 0)
      m_context->workgroup = strdup(settings->GetString(CSettings::SETTING_SMB_WORKGROUP).c_str());
    m_context->user = strdup("guest");
#endif

    // initialize samba and do some hacking into the settings
    if (smbc_init_context(m_context))
    {
      // setup context using the smb old interface compatibility
      SMBCCTX *old_context = smbc_set_context(m_context);
      // free previous context or we leak it, this comes from smbc_init above.
      // there is a bug in smbclient (old interface), if we init/set a context
      // then set(null)/free it in DeInit above, the next smbc_set_context
      // return the already freed previous context, free again and bang, crash.
      // so we setup a stic bool to track the first init so we can free the
      // context associated with the initial smbc_init.
      if (old_context && IsFirstInit)
      {
        smbc_free_context(old_context, 1);
        IsFirstInit = false;
      }
    }
    else
    {
      smbc_free_context(m_context, 1);
      m_context = NULL;
    }
  }
  m_IdleTimeout = 180;
}

std::string CSMB::URLEncode(const CURL &url)
{
  /* due to smb wanting encoded urls we have to build it manually */

  std::string flat = "smb://";

  /* samba messes up of password is set but no username is set. don't know why yet */
  /* probably the url parser that goes crazy */
  if(url.GetUserName().length() > 0 /* || url.GetPassWord().length() > 0 */)
  {
    if(!url.GetDomain().empty())
    {
      flat += URLEncode(url.GetDomain());
      flat += ";";
    }
    flat += URLEncode(url.GetUserName());
    if(url.GetPassWord().length() > 0)
    {
      flat += ":";
      flat += URLEncode(url.GetPassWord());
    }
    flat += "@";
  }
  flat += URLEncode(url.GetHostName());

  if (url.HasPort())
  {
    flat += StringUtils::Format(":{}", url.GetPort());
  }

  /* okey sadly since a slash is an invalid name we have to tokenize */
  std::vector<std::string> parts;
  StringUtils::Tokenize(url.GetFileName(), parts, "/");
  for (const std::string& it : parts)
  {
    flat += "/";
    flat += URLEncode((it));
  }

  /* okey options should go here, thou current samba doesn't support any */

  return flat;
}

std::string CSMB::URLEncode(const std::string &value)
{
  return CURL::Encode(value);
}

/* This is called from CApplication::ProcessSlow() and is used to tell if smbclient have been idle for too long */
void CSMB::CheckIfIdle()
{
/* We check if there are open connections. This is done without a lock to not halt the mainthread. It should be thread safe as
   worst case scenario is that m_OpenConnections could read 0 and then changed to 1 if this happens it will enter the if which will lead to another check, which is locked.  */
  if (m_OpenConnections == 0)
  { /* I've set the the maximum IDLE time to be 1 min and 30 sec. */
    std::unique_lock<CCriticalSection> lock(*this);
    if (m_OpenConnections == 0 /* check again - when locked */ && m_context != NULL)
    {
      if (m_IdleTimeout > 0)
	  {
        m_IdleTimeout--;
      }
	  else
	  {
            CLog::Log(LOGINFO, "Samba is idle. Closing the remaining connections");
            smb.Deinit();
      }
    }
  }
}

void CSMB::SetActivityTime()
{
  /* Since we get called every 500ms from ProcessSlow we limit the tick count to 180 */
  /* That means we have 2 ticks per second which equals 180/2 == 90 seconds */
  m_IdleTimeout = 180;
}

/* The following two function is used to keep track on how many Opened files/directories there are.
   This makes the idle timer not count if a movie is paused for example */
void CSMB::AddActiveConnection()
{
  std::unique_lock<CCriticalSection> lock(*this);
  m_OpenConnections++;
}
void CSMB::AddIdleConnection()
{
  std::unique_lock<CCriticalSection> lock(*this);
  m_OpenConnections--;
  /* If we close a file we reset the idle timer so that we don't have any weird behaviours if a user
     leaves the movie paused for a long while and then press stop */
  m_IdleTimeout = 180;
}

CURL CSMB::GetResolvedUrl(const CURL& url)
{
  CURL tmpUrl(url);
  std::string resolvedHostName;

  if (CDNSNameCache::Lookup(tmpUrl.GetHostName(), resolvedHostName))
    tmpUrl.SetHostName(resolvedHostName);

  return tmpUrl;
}

CSMB smb;

CSMBFile::CSMBFile()
{
  smb.Init();
  m_fd = -1;
  smb.AddActiveConnection();
  m_allowRetry = true;
}

CSMBFile::~CSMBFile()
{
  Close();
  smb.AddIdleConnection();
}

int64_t CSMBFile::GetPosition()
{
  if (m_fd == -1)
    return -1;
  std::unique_lock<CCriticalSection> lock(smb);
  if (!smb.IsSmbValid())
    return -1;
  return smbc_lseek(m_fd, 0, SEEK_CUR);
}

int64_t CSMBFile::GetLength()
{
  if (m_fd == -1)
    return -1;
  return m_fileSize;
}

bool CSMBFile::Open(const CURL& url)
{
  Close();

  // we can't open files like smb://file.f or smb://server/file.f
  // if a file matches the if below return false, it can't exist on a samba share.
  if (!IsValidFile(url.GetFileName()))
  {
    CLog::Log(LOGINFO, "SMBFile->Open: Bad URL : '{}'", url.GetRedacted());
    return false;
  }
  m_url = url;

  // opening a file to another computer share will create a new session
  // when opening smb://server xbms will try to find folder.jpg in all shares
  // listed, which will create lot's of open sessions.

  std::string strFileName;
  m_fd = OpenFile(url, strFileName);

  CLog::Log(LOGDEBUG, "CSMBFile::Open - opened {}, fd={}", url.GetRedacted(), m_fd);
  if (m_fd == -1)
  {
    // write error to logfile
    CLog::Log(LOGERROR, "SMBFile->Open: Unable to open file : '{}'\nunix_err:'{:x}' error : '{}'",
              CURL::GetRedacted(strFileName), errno, strerror(errno));
    return false;
  }

  std::unique_lock<CCriticalSection> lock(smb);
  if (!smb.IsSmbValid())
    return false;
  struct stat tmpBuffer;
  if (smbc_stat(strFileName.c_str(), &tmpBuffer) < 0)
  {
    smbc_close(m_fd);
    m_fd = -1;
    return false;
  }

  m_fileSize = tmpBuffer.st_size;

  int64_t ret = smbc_lseek(m_fd, 0, SEEK_SET);
  if ( ret < 0 )
  {
    smbc_close(m_fd);
    m_fd = -1;
    return false;
  }
  // We've successfully opened the file!
  return true;
}


/// \brief Checks authentication against SAMBA share. Reads password cache created in CSMBDirectory::OpenDir().
/// \param strAuth The SMB style path
/// \return SMB file descriptor
/*
int CSMBFile::OpenFile(std::string& strAuth)
{
  int fd = -1;

  std::string strPath = g_passwordManager.GetSMBAuthFilename(strAuth);

  fd = smbc_open(strPath.c_str(), O_RDONLY, 0);
  //! @todo Run a loop here that prompts for our username/password as appropriate?
  //! We have the ability to run a file (eg from a button action) without browsing to
  //! the directory first.  In the case of a password protected share that we do
  //! not have the authentication information for, the above smbc_open() will have
  //! returned negative, and the file will not be opened.  While this is not a particular
  //! likely scenario, we might want to implement prompting for the password in this case.
  //! The code from SMBDirectory can be used for this.
  if(fd >= 0)
    strAuth = strPath;

  return fd;
}
*/

int CSMBFile::OpenFile(const CURL &url, std::string& strAuth)
{
  int fd = -1;
  smb.Init();

  strAuth = GetAuthenticatedPath(CSMB::GetResolvedUrl(url));
  std::string strPath = strAuth;

  {
    std::unique_lock<CCriticalSection> lock(smb);
    if (smb.IsSmbValid())
      fd = smbc_open(strPath.c_str(), O_RDONLY, 0);
  }

  if (fd >= 0)
    strAuth = strPath;

  return fd;
}

bool CSMBFile::Exists(const CURL& url)
{
  // we can't open files like smb://file.f or smb://server/file.f
  // if a file matches the if below return false, it can't exist on a samba share.
  if (!IsValidFile(url.GetFileName())) return false;

  smb.Init();
  std::string strFileName = GetAuthenticatedPath(CSMB::GetResolvedUrl(url));

  struct stat info;

  std::unique_lock<CCriticalSection> lock(smb);
  if (!smb.IsSmbValid())
    return false;
  int iResult = smbc_stat(strFileName.c_str(), &info);

  if (iResult < 0) return false;
  return true;
}

int CSMBFile::Stat(struct __stat64* buffer)
{
  if (m_fd == -1)
    return -1;

  struct stat tmpBuffer = {};

  std::unique_lock<CCriticalSection> lock(smb);
  if (!smb.IsSmbValid())
    return -1;
  int iResult = smbc_fstat(m_fd, &tmpBuffer);
  CUtil::StatToStat64(buffer, &tmpBuffer);
  return iResult;
}

int CSMBFile::Stat(const CURL& url, struct __stat64* buffer)
{
  smb.Init();
  std::string strFileName = GetAuthenticatedPath(CSMB::GetResolvedUrl(url));
  std::unique_lock<CCriticalSection> lock(smb);

  if (!smb.IsSmbValid())
    return -1;
  struct stat tmpBuffer = {};
  int iResult = smbc_stat(strFileName.c_str(), &tmpBuffer);
  CUtil::StatToStat64(buffer, &tmpBuffer);
  return iResult;
}

int CSMBFile::Truncate(int64_t size)
{
  if (m_fd == -1) return 0;
  /*
 * This would force us to be dependant on SMBv3.2 which is GPLv3
 * This is only used by the TagLib writers, which are not currently in use
 * So log and warn until we implement TagLib writing & can re-implement this better.
  std::unique_lock<CCriticalSection> lock(smb); // Init not called since it has to be "inited" by now

#if defined(TARGET_ANDROID)
  int iResult = 0;
#else
  int iResult = smbc_ftruncate(m_fd, size);
#endif
*/
  CLog::Log(LOGWARNING, "{} - Warning(smbc_ftruncate called and not implemented)", __FUNCTION__);
  return 0;
}

ssize_t CSMBFile::Read(void *lpBuf, size_t uiBufSize)
{
  if (uiBufSize > SSIZE_MAX)
    uiBufSize = SSIZE_MAX;

  if (m_fd == -1)
    return -1;

  // Some external libs (libass) use test read with zero size and
  // null buffer pointer to check whether file is readable, but
  // libsmbclient always return "-1" if called with null buffer
  // regardless of buffer size.
  // To overcome this, force return "0" in that case.
  if (uiBufSize == 0 && lpBuf == NULL)
    return 0;

  std::unique_lock<CCriticalSection> lock(
      smb); // Init not called since it has to be "inited" by now
  if (!smb.IsSmbValid())
    return -1;
  smb.SetActivityTime();

  ssize_t bytesRead = smbc_read(m_fd, lpBuf, (int)uiBufSize);

  if (m_allowRetry && bytesRead < 0 && errno == EINVAL )
  {
    CLog::Log(LOGERROR, "{} - Error( {}, {}, {} ) - Retrying", __FUNCTION__, bytesRead, errno,
              strerror(errno));
    bytesRead = smbc_read(m_fd, lpBuf, (int)uiBufSize);
  }

  if ( bytesRead < 0 )
    CLog::Log(LOGERROR, "{} - Error( {}, {}, {} )", __FUNCTION__, bytesRead, errno,
              strerror(errno));

  return bytesRead;
}

int64_t CSMBFile::Seek(int64_t iFilePosition, int iWhence)
{
  if (m_fd == -1) return -1;

  std::unique_lock<CCriticalSection> lock(
      smb); // Init not called since it has to be "inited" by now
  if (!smb.IsSmbValid())
    return -1;
  smb.SetActivityTime();
  int64_t pos = smbc_lseek(m_fd, iFilePosition, iWhence);

  if ( pos < 0 )
  {
    CLog::Log(LOGERROR, "{} - Error( {}, {}, {} )", __FUNCTION__, pos, errno, strerror(errno));
    return -1;
  }

  return pos;
}

void CSMBFile::Close()
{
  if (m_fd != -1)
  {
    CLog::Log(LOGDEBUG, "CSMBFile::Close closing fd {}", m_fd);
    std::unique_lock<CCriticalSection> lock(smb);
    if (!smb.IsSmbValid())
      return;
    smbc_close(m_fd);
  }
  m_fd = -1;
}

ssize_t CSMBFile::Write(const void* lpBuf, size_t uiBufSize)
{
  if (m_fd == -1) return -1;

  // lpBuf can be safely casted to void* since xbmc_write will only read from it.
  std::unique_lock<CCriticalSection> lock(smb);
  if (!smb.IsSmbValid())
    return -1;

  return  smbc_write(m_fd, lpBuf, uiBufSize);
}

bool CSMBFile::Delete(const CURL& url)
{
  smb.Init();
  std::string strFile = GetAuthenticatedPath(CSMB::GetResolvedUrl(url));

  std::unique_lock<CCriticalSection> lock(smb);
  if (!smb.IsSmbValid())
    return false;

  int result = smbc_unlink(strFile.c_str());

  if(result != 0)
    CLog::Log(LOGERROR, "{} - Error( {} )", __FUNCTION__, strerror(errno));

  return (result == 0);
}

bool CSMBFile::Rename(const CURL& url, const CURL& urlnew)
{
  smb.Init();
  std::string strFile = GetAuthenticatedPath(CSMB::GetResolvedUrl(url));
  std::string strFileNew = GetAuthenticatedPath(CSMB::GetResolvedUrl(urlnew));
  std::unique_lock<CCriticalSection> lock(smb);
  if (!smb.IsSmbValid())
    return false;

  int result = smbc_rename(strFile.c_str(), strFileNew.c_str());

  if(result != 0)
    CLog::Log(LOGERROR, "{} - Error( {} )", __FUNCTION__, strerror(errno));

  return (result == 0);
}

bool CSMBFile::OpenForWrite(const CURL& url, bool bOverWrite)
{
  m_fileSize = 0;

  Close();

  // we can't open files like smb://file.f or smb://server/file.f
  // if a file matches the if below return false, it can't exist on a samba share.
  if (!IsValidFile(url.GetFileName())) return false;

  std::string strFileName = GetAuthenticatedPath(CSMB::GetResolvedUrl(url));
  std::unique_lock<CCriticalSection> lock(smb);
  if (!smb.IsSmbValid())
    return false;

  if (bOverWrite)
  {
    CLog::Log(LOGWARNING, "SMBFile::OpenForWrite() called with overwriting enabled! - {}",
              CURL::GetRedacted(strFileName));
    m_fd = smbc_creat(strFileName.c_str(), 0);
  }
  else
  {
    m_fd = smbc_open(strFileName.c_str(), O_RDWR, 0);
  }

  if (m_fd == -1)
  {
    // write error to logfile
    CLog::Log(LOGERROR, "SMBFile->Open: Unable to open file : '{}'\nunix_err:'{:x}' error : '{}'",
              CURL::GetRedacted(strFileName), errno, strerror(errno));
    return false;
  }

  // We've successfully opened the file!
  return true;
}

bool CSMBFile::IsValidFile(const std::string& strFileName)
{
  if (strFileName.find('/') == std::string::npos || /* doesn't have sharename */
      StringUtils::EndsWith(strFileName, "/.") || /* not current folder */
      StringUtils::EndsWith(strFileName, "/.."))  /* not parent folder */
      return false;
  return true;
}

std::string CSMBFile::GetAuthenticatedPath(const CURL &url)
{
  CURL authURL(CSMB::GetResolvedUrl(url));
  CPasswordManager::GetInstance().AuthenticateURL(authURL);
  return smb.URLEncode(authURL);
}

int CSMBFile::IoControl(EIoControl request, void* param)
{
  if (request == IOCTRL_SEEK_POSSIBLE)
    return 1;

  if (request == IOCTRL_SET_RETRY)
  {
    m_allowRetry = *(bool*) param;
    return 0;
  }

  return -1;
}

int CSMBFile::GetChunkSize()
{
  const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();

  return settings ? (settings->GetInt(CSettings::SETTING_SMB_CHUNKSIZE) * 1024) : (128 * 1024);
}
