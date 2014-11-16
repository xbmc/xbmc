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

// SMBFile.cpp: implementation of the CSMBFile class.
//
//////////////////////////////////////////////////////////////////////

#include "system.h"
#include "SMBFile.h"
#include "PasswordManager.h"
#include "SMBDirectory.h"
#include <libsmbclient.h>
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "Util.h"
#include "utils/Environment.h"
#include "utils/StringUtils.h"
#include "utils/TimeUtils.h"
#include "filesystem/SpecialProtocol.h"
#include "commons/Exception.h"

using namespace XFILE;

void xb_smbc_log(const char* msg)
{
  CLog::Log(LOGINFO, "%s%s", "smb: ", msg);
}

void xb_smbc_auth(const char *srv, const char *shr, char *wg, int wglen,
                  char *un, int unlen, char *pw, int pwlen)
{
  CURL url;
  url.SetProtocol("smb");
  url.SetHostName(srv);
  url.SetFileName(shr);
  url.SetUserName(un);
  url.SetPassword(pw);
  CPasswordManager::GetInstance().AuthenticateURL(url);
  std::string un2 = url.GetUserName();
  std::string wg2 = wg;
  size_t slash = un2.find("\\");
  if(slash != std::string::npos)
    wg2 = un2.substr(0, slash);
  strncpy(wg, wg2.c_str(), wglen);
  strncpy(un, un2.c_str(), unlen);
  strncpy(pw, url.GetPassWord().c_str(), pwlen);
  return ;
}

smbc_get_cached_srv_fn orig_cache;

SMBCSRV* xb_smbc_cache(SMBCCTX* c, const char* server, const char* share, const char* workgroup, const char* username)
{
  /* standard cache will return same session to same server on different shares, but since authentication  *
   * can change username between shares, we must lookup any authentication before looking up cached server */
  CURL url;
  url.SetProtocol("smb");
  url.SetHostName(server);
  url.SetFileName(share);
  url.SetUserName(username);
  url.SetPassword("");
  CPasswordManager::GetInstance().AuthenticateURL(url);
  std::string un2 = url.GetUserName();
  std::string wg2 = workgroup;
  size_t slash = un2.find("\\");
  if(slash != std::string::npos)
    wg2 = un2.substr(0, slash);
  return orig_cache(c, server, share, wg2.c_str(), un2.c_str());
}

CSMB::CSMB()
{
  m_IdleTimeout = 0;
  m_context = NULL;
}

CSMB::~CSMB()
{
  Deinit();
}

void CSMB::Deinit()
{
  CSingleLock lock(*this);

  /* samba goes loco if deinited while it has some files opened */
  if (m_context)
  {
    smbc_free_context(m_context, 1);
    m_context = NULL;
  }
}

void CSMB::Init()
{
  CSingleLock lock(*this);
  if (!m_context)
  {
    // Create ~/.smb/smb.conf. This file is used by libsmbclient.
    // http://us1.samba.org/samba/docs/man/manpages-3/libsmbclient.7.html
    // http://us1.samba.org/samba/docs/man/manpages-3/smb.conf.5.html

    CStdString home = CSpecialProtocol::TranslatePath("special://temp/");
    char smb_conf[MAX_PATH];
    snprintf(smb_conf, sizeof(smb_conf), "%s/.smb", home.c_str());
    if (mkdir(smb_conf, 0755) == 0 || errno == EEXIST)
    {
      snprintf(smb_conf, sizeof(smb_conf), "%s/.smb/smb.conf", home.c_str());
      FILE* f = fopen(smb_conf, "w+");
      if (f != NULL)
      {
        fprintf(f, "[global]\n");

        // make sure we're not acting like a server
        fprintf(f, "\tpreferred master = no\n");
        fprintf(f, "\tlocal master = no\n");
        fprintf(f, "\tdomain master = no\n");

        // use the weaker LANMAN password hash in order to be compatible with older servers
        fprintf(f, "\tclient lanman auth = yes\n");
        fprintf(f, "\tlanman auth = yes\n");

        fprintf(f, "\tsocket options = TCP_NODELAY IPTOS_LOWDELAY SO_RCVBUF=65536 SO_SNDBUF=65536\n");      
        fprintf(f, "\tlock directory = %s/.smb/\n", home.c_str());

        // set wins server if there's one. name resolve order defaults to 'lmhosts host wins bcast'.
        // if no WINS server has been specified the wins method will be ignored.
        if (CSettings::Get().GetString("smb.winsserver").length() > 0 && !StringUtils::EqualsNoCase(CSettings::Get().GetString("smb.winsserver"), "0.0.0.0") )
        {
          fprintf(f, "\twins server = %s\n", CSettings::Get().GetString("smb.winsserver").c_str());
          fprintf(f, "\tname resolve order = bcast wins host\n");
        }
        else
          fprintf(f, "\tname resolve order = bcast host\n");

        // use user-configured charset. if no charset is specified,
        // samba tries to use charset 850 but falls back to ASCII in case it is not available
        if (g_advancedSettings.m_sambadoscodepage.length() > 0)
          fprintf(f, "\tdos charset = %s\n", g_advancedSettings.m_sambadoscodepage.c_str());

        fclose(f);
      }
    }

    // reads smb.conf so this MUST be after we create smb.conf
    // setup our context

    std::string old_home = CEnvironment::getenv("HOME");
    CEnvironment::setenv("HOME", home.c_str());
    m_context = smbc_new_context();
    CEnvironment::setenv("HOME", old_home.c_str(), 1);
#ifdef DEPRECATED_SMBC_INTERFACE
    smbc_setDebug(m_context, g_advancedSettings.CanLogComponent(LOGSAMBA) ? 10 : 0);
    smbc_setFunctionAuthData(m_context, xb_smbc_auth);
    orig_cache = smbc_getFunctionGetCachedServer(m_context);
    smbc_setFunctionGetCachedServer(m_context, xb_smbc_cache);
    smbc_setOptionOneSharePerServer(m_context, false);
    smbc_setOptionBrowseMaxLmbCount(m_context, 0);
    smbc_setTimeout(m_context, g_advancedSettings.m_sambaclienttimeout * 1000);
    if (CSettings::Get().GetString("smb.workgroup").length() > 0)
      smbc_setWorkgroup(m_context, strdup(CSettings::Get().GetString("smb.workgroup").c_str()));
    smbc_setUser(m_context, strdup("guest"));

    close_fn    = smbc_getFunctionClose(m_context);
    closedir_fn = smbc_getFunctionClosedir(m_context);
    creat_fn    = smbc_getFunctionCreat(m_context);
    fstat_fn    = smbc_getFunctionFstat(m_context);
    getxattr_fn = smbc_getFunctionGetxattr(m_context);
    lseek_fn    = smbc_getFunctionLseek(m_context);
    lseek_fn    = smbc_getFunctionLseek(m_context);
    mkdir_fn    = smbc_getFunctionMkdir(m_context);
    open_fn     = smbc_getFunctionOpen(m_context);
    opendir_fn  = smbc_getFunctionOpendir(m_context);
    read_fn     = smbc_getFunctionRead(m_context);
    readdir_fn  = smbc_getFunctionReaddir(m_context);
    rename_fn   = smbc_getFunctionRename(m_context);
    rmdir_fn    = smbc_getFunctionRmdir(m_context);
    stat_fn     = smbc_getFunctionStat(m_context);
    unlink_fn   = smbc_getFunctionUnlink(m_context);
    write_fn    = smbc_getFunctionWrite(m_context);
#else
    m_context->debug = (g_advancedSettings.CanLogComponent(LOGSAMBA) ? 10 : 0);
    m_context->callbacks.auth_fn = xb_smbc_auth;
    orig_cache = m_context->callbacks.get_cached_srv_fn;
    m_context->callbacks.get_cached_srv_fn = xb_smbc_cache;
    m_context->options.one_share_per_server = false;
    m_context->options.browse_max_lmb_count = 0;
    m_context->timeout = g_advancedSettings.m_sambaclienttimeout * 1000;
    if (CSettings::Get().GetString("smb.workgroup").length() > 0)
      m_context->workgroup = strdup(CSettings::Get().GetString("smb.workgroup").c_str());
    m_context->user = strdup("guest");

    close_fn    = m_context->close_fn;
    closedir_fn = m_context->closedir_fn;
    creat_fn    = m_context->creat_fn;
    fstat_fn    = m_context->fstat_fn;
    getxattr_fn = m_context->getxattr_fn;
    lseek_fn    = m_context->lseek_fn;
    lseek_fn    = m_context->lseek_fn;
    mkdir_fn    = m_context->mkdir_fn;
    open_fn     = m_context->open_fn;
    opendir_fn  = m_context->opendir_fn;
    read_fn     = m_context->read_fn;
    readdir_fn  = m_context->readdir_fn;
    rename_fn   = m_context->rename_fn;
    rmdir_fn    = m_context->rmdir_fn;
    stat_fn     = m_context->stat_fn;
    unlink_fn   = m_context->unlink_fn;
    write_fn    = m_context->write_fn;
#endif

    // initialize samba and do some hacking into the settings
    if (smbc_init_context(m_context) == NULL)
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

  if(url.GetDomain().length() > 0)
  {
    flat += URLEncode(url.GetDomain());
    flat += ";";
  }

  /* samba messes up of password is set but no username is set. don't know why yet */
  /* probably the url parser that goes crazy */
  if(url.GetUserName().length() > 0 /* || url.GetPassWord().length() > 0 */)
  {
    flat += URLEncode(url.GetUserName());
    if(url.GetPassWord().length() > 0)
    {
      flat += ":";
      flat += URLEncode(url.GetPassWord());
    }
    flat += "@";
  }
  flat += URLEncode(url.GetHostName());

  /* okey sadly since a slash is an invalid name we have to tokenize */
  std::vector<std::string> parts;
  std::vector<std::string>::iterator it;
  StringUtils::Tokenize(url.GetFileName(), parts, "/");
  for( it = parts.begin(); it != parts.end(); it++ )
  {
    flat += "/";
    flat += URLEncode((*it));
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
   worst case scenario is that m_OpenConnections could read 0 and then changed to 1 if this happens it will enter the if wich will lead to another check, wich is locked.  */
  if (m_OpenConnections == 0)
  { /* I've set the the maxiumum IDLE time to be 1 min and 30 sec. */
    CSingleLock lock(*this);
    if (m_OpenConnections == 0 /* check again - when locked */ && m_context != NULL)
    {
      if (m_IdleTimeout > 0)
	  {
        m_IdleTimeout--;
      }
	  else
	  {
        CLog::Log(LOGNOTICE, "Samba is idle. Closing the remaining connections");
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
  CSingleLock lock(*this);
  m_OpenConnections++;
}
void CSMB::AddIdleConnection()
{
  CSingleLock lock(*this);
  m_OpenConnections--;
  /* If we close a file we reset the idle timer so that we don't have any wierd behaviours if a user
     leaves the movie paused for a long while and then press stop */
  m_IdleTimeout = 180;
}

CSMB smb;

CSMBFile::CSMBFile()
{
  smb.Init();
  m_fd = NULL;
  smb.AddActiveConnection();
}

CSMBFile::~CSMBFile()
{
  Close();
  smb.AddIdleConnection();
}

int64_t CSMBFile::GetPosition()
{
  if (m_fd == NULL)
    return -1;
  CSingleLock lock(smb);
  return smb.lseek_fn(smb.GetContext(), m_fd, 0, SEEK_CUR);
}

int64_t CSMBFile::GetLength()
{
  if (m_fd == NULL)
    return -1;

  CSingleLock lock(smb);
  struct stat buf;
  if (smb.fstat_fn(smb.GetContext(), m_fd, &buf) < 0)
    return -1;

  return buf.st_size;
}

bool CSMBFile::Open(const CURL& url)
{
  Close();

  // we can't open files like smb://file.f or smb://server/file.f
  // if a file matches the if below return false, it can't exist on a samba share.
  if (!IsValidFile(url.GetFileName()))
  {
      CLog::Log(LOGNOTICE,"SMBFile->Open: Bad URL : '%s'",url.GetFileName().c_str());
      return false;
  }

  // opening a file to another computer share will create a new session
  // when opening smb://server xbms will try to find folder.jpg in all shares
  // listed, which will create lot's of open sessions.

  std::string strFileName = smb.URLEncode(url);

  if (m_fd == NULL)
  {
    CSingleLock lock(smb);
    m_fd = smb.open_fn(smb.GetContext(), strFileName.c_str(), O_RDONLY, 0);
  }

  CLog::Log(LOGDEBUG,"CSMBFile::Open - opened %s, fd=%p",url.GetFileName().c_str(), m_fd);
  if (m_fd == NULL)
  {
    // write error to logfile
    CLog::Log(LOGINFO, "SMBFile->Open: Unable to open file : '%s'\nunix_err:'%x' error : '%s'", CURL::GetRedacted(strFileName).c_str(), errno, strerror(errno));
    return false;
  }

  int64_t ret = smb.lseek_fn(smb.GetContext(), m_fd, 0, SEEK_SET);
  if ( ret < 0 )
  {
    smb.close_fn(smb.GetContext(), m_fd);
    m_fd = NULL;
    return false;
  }
  // We've successfully opened the file!
  return true;
}

bool CSMBFile::Exists(const CURL& url)
{
  // we can't open files like smb://file.f or smb://server/file.f
  // if a file matches the if below return false, it can't exist on a samba share.
  if (!IsValidFile(url.GetFileName())) return false;

  smb.Init();
  std::string strFileName = smb.URLEncode(url);

  struct stat info;

  CSingleLock lock(smb);
  int iResult = smb.stat_fn(smb.GetContext(), strFileName.c_str(), &info);

  if (iResult < 0) return false;
  return true;
}

int CSMBFile::Stat(struct __stat64* buffer)
{
  if (m_fd == NULL)
    return -1;

  struct stat tmpBuffer = {0};

  CSingleLock lock(smb);
  int iResult = smb.fstat_fn(smb.GetContext(), m_fd, &tmpBuffer);
  CUtil::StatToStat64(buffer, &tmpBuffer);
  return iResult;
}

int CSMBFile::Stat(const CURL& url, struct __stat64* buffer)
{
  smb.Init();
  std::string strFileName = smb.URLEncode(url);
  CSingleLock lock(smb);

  struct stat tmpBuffer = {0};
  int iResult = smb.stat_fn(smb.GetContext(), strFileName.c_str(), &tmpBuffer);
  CUtil::StatToStat64(buffer, &tmpBuffer);
  return iResult;
}

int CSMBFile::Truncate(int64_t size)
{
  if (m_fd == NULL) return 0;
/* 
 * This would force us to be dependant on SMBv3.2 which is GPLv3
 * This is only used by the TagLib writers, which are not currently in use
 * So log and warn until we implement TagLib writing & can re-implement this better.
  CSingleLock lock(smb); // Init not called since it has to be "inited" by now

#if defined(TARGET_ANDROID)
  int iResult = 0;
#else
  int iResult = smbc_ftruncate(m_fd, size);
#endif
*/
  CLog::Log(LOGWARNING, "%s - Warning(smbc_ftruncate called and not implemented)", __FUNCTION__);
  return 0;
}

ssize_t CSMBFile::Read(void *lpBuf, size_t uiBufSize)
{
  if (uiBufSize > SSIZE_MAX)
    uiBufSize = SSIZE_MAX;

  if (m_fd == NULL)
    return -1;

  // Some external libs (libass) use test read with zero size and 
  // null buffer pointer to check whether file is readable, but 
  // libsmbclient always return "-1" if called with null buffer 
  // regardless of buffer size.
  // To overcome this, force return "0" in that case.
  if (uiBufSize == 0 && lpBuf == NULL)
    return 0;

  CSingleLock lock(smb); // Init not called since it has to be "inited" by now
  smb.SetActivityTime();
  /* work around stupid bug in samba */
  /* some samba servers has a bug in it where the */
  /* 17th bit will be ignored in a request of data */
  /* this can lead to a very small return of data */
  /* also worse, a request of exactly 64k will return */
  /* as if eof, client has a workaround for windows */
  /* thou it seems other servers are affected too */
  if( uiBufSize >= 64*1024-2 )
    uiBufSize = 64*1024-2;

  ssize_t bytesRead = smb.read_fn(smb.GetContext(), m_fd, lpBuf, (int)uiBufSize);

  if ( bytesRead < 0 && errno == EINVAL )
  {
    CLog::Log(LOGERROR, "%s - Error( %"PRIdS", %d, %s ) - Retrying", __FUNCTION__, bytesRead, errno, strerror(errno));
    bytesRead = smb.read_fn(smb.GetContext(), m_fd, lpBuf, (int)uiBufSize);
  }

  if ( bytesRead < 0 )
    CLog::Log(LOGERROR, "%s - Error( %"PRIdS", %d, %s )", __FUNCTION__, bytesRead, errno, strerror(errno));

  return bytesRead;
}

int64_t CSMBFile::Seek(int64_t iFilePosition, int iWhence)
{
  if (m_fd == NULL) return -1;

  CSingleLock lock(smb); // Init not called since it has to be "inited" by now
  smb.SetActivityTime();
  int64_t pos = smb.lseek_fn(smb.GetContext(), m_fd, iFilePosition, iWhence);

  if ( pos < 0 )
  {
    CLog::Log(LOGERROR, "%s - Error( %" PRId64", %d, %s )", __FUNCTION__, pos, errno, strerror(errno));
    return -1;
  }

  return (int64_t)pos;
}

void CSMBFile::Close()
{
  if (m_fd)
  {
    CLog::Log(LOGDEBUG,"CSMBFile::Close closing fd %p", m_fd);
    CSingleLock lock(smb);
    smb.close_fn(smb.GetContext(), m_fd);
  }
  m_fd = NULL;
}

ssize_t CSMBFile::Write(const void* lpBuf, size_t uiBufSize)
{
  if (m_fd == NULL) return -1;

  // lpBuf can be safely casted to void* since xbmc_write will only read from it.
  CSingleLock lock(smb);
  return  smb.write_fn(smb.GetContext(), m_fd, (void*)lpBuf, uiBufSize);
}

bool CSMBFile::Delete(const CURL& url)
{
  smb.Init();
  std::string strFile = smb.URLEncode(url);

  CSingleLock lock(smb);

  int result = smb.unlink_fn(smb.GetContext(), strFile.c_str());

  if(result != 0)
    CLog::Log(LOGERROR, "%s - Error( %s )", __FUNCTION__, strerror(errno));

  return (result == 0);
}

bool CSMBFile::Rename(const CURL& url, const CURL& urlnew)
{
  smb.Init();
  std::string strFile = smb.URLEncode(url);
  std::string strFileNew = smb.URLEncode(urlnew);
  CSingleLock lock(smb);
  int result = smb.rename_fn(smb.GetContext(), strFile.c_str()
                           , smb.GetContext(), strFileNew.c_str());

  if(result != 0)
    CLog::Log(LOGERROR, "%s - Error( %s )", __FUNCTION__, strerror(errno));

  return (result == 0);
}

bool CSMBFile::OpenForWrite(const CURL& url, bool bOverWrite)
{
  Close();
  // we can't open files like smb://file.f or smb://server/file.f
  // if a file matches the if below return false, it can't exist on a samba share.
  if (!IsValidFile(url.GetFileName())) return false;

  std::string strFileName = smb.URLEncode(url);
  CSingleLock lock(smb);

  if (bOverWrite)
  {
    CLog::Log(LOGWARNING, "SMBFile::OpenForWrite() called with overwriting enabled! - %s", strFileName.c_str());
    m_fd = smb.creat_fn(smb.GetContext(), strFileName.c_str(), 0);
  }
  else
  {
    m_fd = smb.open_fn(smb.GetContext(), strFileName.c_str(), O_RDWR, 0);
  }

  if (m_fd == NULL)
  {
    // write error to logfile
    CLog::Log(LOGERROR, "SMBFile->Open: Unable to open file : '%s'\nunix_err:'%x' error : '%s'", strFileName.c_str(), errno, strerror(errno));
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

