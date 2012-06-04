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

// FileSmb.cpp: implementation of the CFile class.
//
//////////////////////////////////////////////////////////////////////

#include "os-dependent.h"
#include <inttypes.h>
#include "client.h"
#include "FileSMB.h"
#include <libsmbclient.h>
#include "platform/threads/mutex.h"

using namespace ADDON;

void xb_smbc_log(const char* msg)
{
  XBMC->Log(LOG_INFO, "%s%s", "smb: ", msg);
}

void xb_smbc_auth(const char *srv, const char *shr, char *wg, int wglen,
                  char *un, int unlen, char *pw, int pwlen)
{
  return;
}

smbc_get_cached_srv_fn orig_cache;

SMBCSRV* xb_smbc_cache(SMBCCTX* c, const char* server, const char* share, const char* workgroup, const char* username)
{
  return orig_cache(c, server, share, workgroup, username);
}

namespace PLATFORM
{

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
  PLATFORM::CLockObject lock(*this);

  /* samba goes loco if deinited while it has some files opened */
  if (m_context)
  {
    try
    {
      smbc_set_context(NULL);
      smbc_free_context(m_context, 1);
    }
    catch(...)
    {
      XBMC->Log(LOG_ERROR,"exception on CSMB::Deinit. errno: %d", errno);
    }
    m_context = NULL;
  }
}

void CSMB::Init()
{
  PLATFORM::CLockObject lock(*this);

  if (!m_context)
  {
    // Create ~/.smb/smb.conf. This file is used by libsmbclient.
    // http://us1.samba.org/samba/docs/man/manpages-3/libsmbclient.7.html
    // http://us1.samba.org/samba/docs/man/manpages-3/smb.conf.5.html
    char smb_conf[MAX_PATH];

    sprintf(smb_conf, "%s/.smb", getenv("HOME"));
    mkdir(smb_conf, 0755);
    sprintf(smb_conf, "%s/.smb/smb.conf", getenv("HOME"));
    FILE* f = fopen(smb_conf, "w");

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
      fprintf(f, "\tlock directory = %s/.smb/\n", getenv("HOME"));

      fprintf(f, "\tname resolve order = lmhosts bcast host\n");

      fclose(f);
    }

    // reads smb.conf so this MUST be after we create smb.conf
    // multiple smbc_init calls are ignored by libsmbclient.
    smbc_init(xb_smbc_auth, 0);

    // setup our context
    m_context = smbc_new_context();
    m_context->debug = 10; //g_advancedSettings.m_logLevel == LOG_LEVEL_DEBUG_SAMBA ? 10 : 0;
    m_context->callbacks.auth_fn = xb_smbc_auth;
    orig_cache = m_context->callbacks.get_cached_srv_fn;
    m_context->callbacks.get_cached_srv_fn = xb_smbc_cache;
    m_context->options.one_share_per_server = false;
    m_context->options.browse_max_lmb_count = 0;
    m_context->timeout = 10000; //g_advancedSettings.m_sambaclienttimeout * 1000;

    // initialize samba and do some hacking into the settings
    if (smbc_init_context(m_context))
    {
      /* setup old interface to use this context */
      smbc_set_context(m_context);
    }
    else
    {
      smbc_free_context(m_context, 1);
      m_context = NULL;
    }
  }
  m_IdleTimeout = 180;
}

void CSMB::Purge()
{
}

/* This is called from CApplication::ProcessSlow() and is used to tell if smbclient have been idle for too long */
void CSMB::CheckIfIdle()
{
  /* We check if there are open connections. This is done without a lock to not halt the mainthread. It should be thread safe as
   worst case scenario is that m_OpenConnections could read 0 and then changed to 1 if this happens it will enter the if wich will lead to another check, wich is locked.  */
  if (m_OpenConnections == 0)
  { /* I've set the the maxiumum IDLE time to be 1 min and 30 sec. */
    PLATFORM::CLockObject lock(*this);
    if (m_OpenConnections == 0 /* check again - when locked */ && m_context != NULL)
    {
      if (m_IdleTimeout > 0)
      {
        m_IdleTimeout--;
      }
      else
      {
        XBMC->Log(LOG_NOTICE, "Samba is idle. Closing the remaining connections");
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
  PLATFORM::CLockObject lock(*this);
  m_OpenConnections++;
}

void CSMB::AddIdleConnection()
{
  PLATFORM::CLockObject lock(*this);
  m_OpenConnections--;
  /* If we close a file we reset the idle timer so that we don't have any wierd behaviours if a user
     leaves the movie paused for a long while and then press stop */
  m_IdleTimeout = 180;

  /* Temporary hack for this PVR addon because we are running outside XBMC */
  if (m_OpenConnections == 0)
  {
    m_IdleTimeout = 0;
    CSMB::CheckIfIdle();
  }
}

CSMB smb;

CFile::CFile()
{
  smb.Init();
  m_fd = -1;
  smb.AddActiveConnection();
}

CFile::~CFile()
{
  Close();
  smb.AddIdleConnection();
}

int64_t CFile::GetPosition()
{
  if (m_fd == -1)
    return 0;
  smb.Init();
  PLATFORM::CLockObject lock(smb);
  int64_t pos = smbc_lseek(m_fd, 0, SEEK_CUR);
  if ( pos < 0 )
    return 0;
  return pos;
}

int64_t CFile::GetLength()
{
  if (m_fd == -1) return 0;
  return m_fileSize;
}

bool CFile::Open(const CStdString& strFileName, unsigned int flags)
{
  Close();

  m_fileName = strFileName;

  m_fd = OpenFile();

  XBMC->Log(LOG_DEBUG,"CFile::Open - opened %s, fd=%d", m_fileName.c_str(), m_fd);
  if (m_fd == -1)
  {
    // write error to logfile
    XBMC->Log(LOG_INFO, "FileSmb->Open: Unable to open file : '%s'\nunix_err:'%x' error : '%s'", strFileName.c_str(), errno, strerror(errno));
    return false;
  }

  CLockObject lock(smb);
  struct stat tmpBuffer;
  if (smbc_stat(strFileName, &tmpBuffer) < 0)
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

int CFile::OpenFile()
{
  int fd = -1;
  smb.Init();

  CStdString strPath = m_fileName;

  {
    CLockObject lock(smb);
    fd = smbc_open(strPath.c_str(), O_RDONLY, 0);
  }

  // file open failed, try to open the directory to force authentication
  if (fd < 0 && errno == EACCES)
  {
    XBMC->Log(LOG_ERROR, "%s: File open on %s failed\n", __FUNCTION__, strPath.c_str());
  }

  return fd;
}

bool CFile::Exists(const CStdString& strFileName, bool bUseCache)
{
  // we can't open files like smb://file.f or smb://server/file.f
  // if a file matches the if below return false, it can't exist on a samba share.
  //if (!IsValidFile(url.GetFileName())) return false;

  smb.Init();

  struct stat info;

  CLockObject lock(smb);
  int iResult = smbc_stat(strFileName, &info);

  if (iResult < 0)
    return false;
  return true;
}

int CFile::Stat(struct stat64* buffer)
{
  if (m_fd == -1)
    return -1;

  struct stat tmpBuffer = {0};

  CLockObject lock(smb);
  int iResult = smbc_fstat(m_fd, &tmpBuffer);

  memset(buffer, 0, sizeof(struct stat64));
  buffer->st_dev = tmpBuffer.st_dev;
  buffer->st_ino = tmpBuffer.st_ino;
  buffer->st_mode = tmpBuffer.st_mode;
  buffer->st_nlink = tmpBuffer.st_nlink;
  buffer->st_uid = tmpBuffer.st_uid;
  buffer->st_gid = tmpBuffer.st_gid;
  buffer->st_rdev = tmpBuffer.st_rdev;
  buffer->st_size = tmpBuffer.st_size;
  buffer->st_atime = tmpBuffer.st_atime;
  buffer->st_mtime = tmpBuffer.st_mtime;
  buffer->st_ctime = tmpBuffer.st_ctime;

  return iResult;
}

unsigned long CFile::Read(void* lpBuf, int64_t uiBufSize)
{
  if (m_fd == -1)
    return 0;

  CLockObject lock(smb); // Init not called since it has to be "inited" by now
  smb.SetActivityTime();
  /* work around stupid bug in samba */
  /* some samba servers hava a bug in it where the */
  /* 17th bit will be ignored in a request of data */
  /* this can lead to a very small return of data */
  /* also worse, a request of exactly 64k will return */
  /* as if eof, client has a workaround for windows */
  /* thou it seems other servers are affected too */
  if ( uiBufSize >= 64*1024-2 )
    uiBufSize = 64*1024-2;

  int bytesRead = smbc_read(m_fd, lpBuf, (int)uiBufSize);

  if ( bytesRead < 0 && errno == EINVAL )
  {
    XBMC->Log(LOG_ERROR, "%s - Error( %d, %d, %s ) - Retrying", __FUNCTION__, bytesRead, errno, strerror(errno));
    bytesRead = smbc_read(m_fd, lpBuf, (int)uiBufSize);
  }

  if ( bytesRead < 0 )
  {
    XBMC->Log(LOG_ERROR, "%s - Error( %d, %d, %s )", __FUNCTION__, bytesRead, errno, strerror(errno));
    return 0;
  }

  return (unsigned int) bytesRead;
}

int64_t CFile::Seek(int64_t iFilePosition, int iWhence)
{
  if (m_fd == -1)
    return -1;

  CLockObject lock(smb); // Init not called since it has to be "inited" by now
  smb.SetActivityTime();
  int64_t pos = smbc_lseek(m_fd, iFilePosition, iWhence);

  if ( pos < 0 )
  {
    XBMC->Log(LOG_ERROR, "%s - Error( %"PRId64", %d, %s )", __FUNCTION__, pos, errno, strerror(errno));
    return -1;
  }

  return (int64_t)pos;
}

void CFile::Close()
{
  if (m_fd != -1)
  {
    XBMC->Log(LOG_DEBUG,"CFile::Close closing fd %d", m_fd);
    CLockObject lock(smb);
    smbc_close(m_fd);
  }
  m_fd = -1;
}

bool CFile::IsInvalid()
{
  if (m_fd < 0)
    return true;
  else
    return false;
}

}