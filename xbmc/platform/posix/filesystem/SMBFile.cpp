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
#include <list>
#include <mutex>
#include <regex>
#include <unordered_map>

#include <libsmbclient.h>

using namespace XFILE;

// ---------------------------------------------------------------
// [Fix] LRU cache for SMB server sessions.
//
// Desktop Windows limits SMB sessions to 20 per user. Kodi's library
// scanner accesses every available share to look for artwork, which
// quickly exhausts that limit when more than 20 shares exist.
//
// This cache holds at most MAX_CACHED_SERVERS (15) sessions alive,
// evicting the least recently used share when the limit is reached.
// The session count stays well below 20, preventing "Invalid argument"
// errors.
//
// The cache key includes username and workgroup to correctly isolate
// sessions when different credentials are used for the same server/share.
//
// To avoid use-after-free after a network loss, cache hits always
// re-fetch the real server pointer from libsmbclient.  If the pointer
// changed, the old one is simply dropped - we never call
// remove_unused_server on it, avoiding a double-free risk.
//
// The implementation caches the remove_unused_server function pointer
// once at init and stores the SMBCSRV* directly in the LRU list so that
// eviction never requires another library call.
//
// Safety measures:
//   - The existing CSMB mutex serialises all library calls, including
//     Deinit().  Only one thread can be inside xb_smbc_cache at a time,
//     making additional synchronisation unnecessary.
//   - The cache mutex (g_cacheMutex) is held during the entire cache
//     operation and protects the LRU structures against the cleanup
//     that Deinit() performs under the same lock.
//   - On shutdown, the original library callback is restored before the
//     SMB context is freed, and orig_cache_fn is nullified so any future
//     call will return an error immediately.
//   - Tracked servers are explicitly released in Deinit() via
//     remove_unused_server.
//
// libsmbclient adds a small overhead of 1-3 sessions (transport, IPC$,
// etc.) that are not visible to our cache.  With MAX_CACHED_SERVERS = 15
// the total remains well below the Windows 20-session limit.
// ---------------------------------------------------------------
namespace
{
// 15 is a safe value - the total stays under 20 even with the library's
// additional overhead.
constexpr size_t MAX_CACHED_SERVERS = 15;

// Cached remove_unused_server function pointer, obtained once during Init.
smbc_remove_unused_server_fn g_removeUnusedFn = nullptr;

// Build a cache key from the four components that libsmbclient uses
// internally.  Backslashes are used as separators because they cannot
// appear in share names or workgroup / user names.
std::string MakeKey(const std::string& server,
                    const std::string& share,
                    const std::string& username,
                    const std::string& workgroup)
{
  std::string key;
  key.reserve(server.size() + share.size() + username.size() + workgroup.size() + 3);
  key.append(server).push_back('\\');
  key.append(share).push_back('\\');
  key.append(username).push_back('\\');
  key.append(workgroup);
  return key;
}

// Cache entry stores the key and the associated server pointer
// so eviction can call remove_unused_server directly.
struct CacheEntry
{
  std::string key;
  SMBCSRV* srv = nullptr;
};

// LRU list - front is most-recently-used, back is least-recently-used.
// The map provides O(1) lookup from key to list iterator.
std::list<CacheEntry> g_lru;
std::unordered_map<std::string, std::list<CacheEntry>::iterator> g_map;
std::mutex g_cacheMutex;

// Original libsmbclient callback, saved during CSMB::Init().
smbc_get_cached_srv_fn orig_cache_fn = nullptr;

// -----------------------------------------------------------------
// Evict the least-recently-used entry from the cache.
// Must be called with g_cacheMutex held.
// -----------------------------------------------------------------
void EvictOne(SMBCCTX* c)
{
  // Copy key and pointer by value - pop_back() will destroy the node.
  const std::string evictKey = g_lru.back().key;
  SMBCSRV* const evictSrv = g_lru.back().srv;

  CLog::LogF(LOGDEBUG, "[SMB_SESSION_LIMIT] Server cache EVICT '{}'", evictKey);

  if (g_removeUnusedFn && evictSrv)
  {
    int rc = g_removeUnusedFn(c, evictSrv);
    if (rc != 0)
      CLog::LogF(LOGWARNING, "[SMB_SESSION_LIMIT] remove_unused_server failed for '{}' (rc={})",
                 evictKey, rc);
  }
  else if (!g_removeUnusedFn)
  {
    CLog::LogF(LOGWARNING, "[SMB_SESSION_LIMIT] remove_unused_server not available - "
                           "evicting from tracking only");
  }

  g_map.erase(evictKey);
  g_lru.pop_back();
}

// -----------------------------------------------------------------
// Replacement for smbc_get_cached_srv_fn.
//   - cache hit  -> move key to front, re-fetch real server pointer.
//   - cache miss -> create new entry, evict LRU if needed.
// The whole operation runs under g_cacheMutex; the outer smb mutex
// guarantees only one thread is in here at a time.
// -----------------------------------------------------------------
SMBCSRV* xb_smbc_cache(
    SMBCCTX* c, const char* server, const char* share, const char* workgroup, const char* username)
{
  std::lock_guard<std::mutex> lock(g_cacheMutex);

  if (!orig_cache_fn)
  {
    CLog::LogF(LOGERROR, "[SMB_SESSION_LIMIT] orig_cache_fn is null - cannot create session");
    return nullptr;
  }

  const std::string srv(server ? server : "");
  const std::string shr(share ? share : "");
  const std::string wg(workgroup ? workgroup : "");
  const std::string usr(username ? username : "");
  const std::string key = MakeKey(srv, shr, usr, wg);

  // ---------- cache hit ---------------------------------------------------
  auto it = g_map.find(key);
  if (it != g_map.end())
  {
    // Move to front (most recently used)
    g_lru.splice(g_lru.begin(), g_lru, it->second);

    SMBCSRV* oldSrv = it->second->srv; // remember the old pointer

    // Always re-fetch the current server pointer from the library.
    // This prevents use-after-free if the library recreated the
    // server after a network loss.
    SMBCSRV* realSrv = orig_cache_fn(c, server, share, workgroup, username);
    if (realSrv)
    {
      // Server is valid. If the pointer changed, just update our entry
      // without touching the old pointer - the library owns its lifetime.
      if (realSrv != oldSrv)
        it->second->srv = realSrv;

      CLog::LogF(LOGDEBUG, "[SMB_SESSION_LIMIT] Server cache HIT for '{}'", key);
      return realSrv;
    }

    // The library no longer has this server - remove the stale entry
    // and fall through to the cache-miss path to create a fresh one.
    CLog::LogF(LOGDEBUG, "[SMB_SESSION_LIMIT] Cached server no longer valid for '{}'", key);
    g_lru.erase(it->second);
    g_map.erase(it);
    // intentional fall-through
  }

  // ---------- cache miss - create new entry --------------------------------
  SMBCSRV* srvPtr = orig_cache_fn(c, server, share, workgroup, username);
  if (!srvPtr)
  {
    CLog::LogF(LOGERROR, "[SMB_SESSION_LIMIT] orig_cache_fn failed for '{}'", key);
    return nullptr;
  }

  // ---------- evict oldest if limit reached --------------------------------
  if (g_lru.size() >= MAX_CACHED_SERVERS)
    EvictOne(c);

  // ---------- insert new entry at front ------------------------------------
  g_lru.push_front({key, srvPtr});
  g_map[key] = g_lru.begin();

  CLog::LogF(LOGDEBUG, "[SMB_SESSION_LIMIT] Server cache CREATED for '{}' (total cached: {})", key,
             g_lru.size());
  return srvPtr;
}

// -----------------------------------------------------------------
// Called from CSMB::Deinit() - remove every entry we still track.
// Must be called before the SMB context is freed.
// -----------------------------------------------------------------
void ClearServerCache(SMBCCTX* c)
{
  std::lock_guard<std::mutex> lock(g_cacheMutex);

  // First, nullify the original callback so that any future calls
  // (or waiting threads) will immediately see an error state.
  orig_cache_fn = nullptr;

  if (g_removeUnusedFn)
  {
    for (const auto& entry : g_lru)
    {
      if (entry.srv)
      {
        int rc = g_removeUnusedFn(c, entry.srv);
        if (rc != 0)
          CLog::LogF(LOGWARNING,
                     "[SMB_SESSION_LIMIT] ClearServerCache: "
                     "remove_unused_server failed for '{}' (rc={})",
                     entry.key, rc);
      }
    }
  }
  else
  {
    CLog::LogF(LOGWARNING, "[SMB_SESSION_LIMIT] ClearServerCache: "
                           "remove_unused_server not available");
  }

  g_lru.clear();
  g_map.clear();
}
} // unnamed namespace

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

void xb_smbc_auth(
    const char* srv, const char* shr, char* wg, int wglen, char* un, int unlen, char* pw, int pwlen)
{
}

// This flag protects against an old Samba bug where smbc_set_context()
// returns an already freed pointer on subsequent inits. It is still
// needed on some current Samba 4.0 installations.
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
  std::unique_lock lock(*this);

  // Save the original callback *before* ClearServerCache nullifies it.
  smbc_get_cached_srv_fn oldCallback = orig_cache_fn;

  // Clear the LRU cache, which also nullifies orig_cache_fn.
  if (m_context)
    ClearServerCache(m_context);

  /* samba goes loco if deinited while it has some files opened */
  if (m_context)
  {
    // Restore the original callback before we destroy the context
    // so the library won't call our wrapper with a dead context.
    smbc_setFunctionGetCachedServer(m_context, oldCallback);

    smbc_set_context(NULL);
    smbc_free_context(m_context, 1);
    m_context = NULL;
  }

  // The cached remove function pointer is no longer valid.
  g_removeUnusedFn = nullptr;
}

void CSMB::Init()
{
  std::unique_lock lock(*this);

  if (!m_context)
  {
    const std::shared_ptr<CSettings> settings =
        CServiceBroker::GetSettingsComponent()->GetSettings();

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
        switch (settings->GetInt(CSettings::SETTING_SMB_MINPROTOCOL))
        {
          case 0:
          default:
            break;
          case 1:
            fprintf(f, "\tclient min protocol = NT1\n");
            break;
          case 2:
            fprintf(f, "\tclient min protocol = SMB2_02\n");
            break;
          case 21:
            fprintf(f, "\tclient min protocol = SMB2_10\n");
            break;
          case 3:
            fprintf(f, "\tclient min protocol = SMB3\n");
            break;
        }

        // set maximum smbclient protocol version
        switch (settings->GetInt(CSettings::SETTING_SMB_MAXPROTOCOL))
        {
          case 0:
          default:
            break;
          case 1:
            fprintf(f, "\tclient max protocol = NT1\n");
            break;
          case 2:
            fprintf(f, "\tclient max protocol = SMB2_02\n");
            break;
          case 21:
            fprintf(f, "\tclient max protocol = SMB2_10\n");
            break;
          case 3:
            fprintf(f, "\tclient max protocol = SMB3\n");
            break;
        }

        // set legacy security options
        if (settings->GetBool(CSettings::SETTING_SMB_LEGACYSECURITY) &&
            (settings->GetInt(CSettings::SETTING_SMB_MAXPROTOCOL) == 1))
        {
          fprintf(f, "\tclient NTLMv2 auth = no\n");
          fprintf(f, "\tclient use spnego = no\n");
        }

        // set wins server if there's one. name resolve order defaults to 'lmhosts host wins bcast'.
        // if no WINS server has been specified the wins method will be ignored.
        if (!settings->GetString(CSettings::SETTING_SMB_WINSSERVER).empty() &&
            !StringUtils::EqualsNoCase(settings->GetString(CSettings::SETTING_SMB_WINSSERVER),
                                       "0.0.0.0"))
        {
          fprintf(f, "\twins server = %s\n",
                  settings->GetString(CSettings::SETTING_SMB_WINSSERVER).c_str());
          fprintf(f, "\tname resolve order = bcast wins host\n");
        }
        else
          fprintf(f, "\tname resolve order = bcast host\n");

        // use user-configured charset. if no charset is specified,
        // samba tries to use charset 850 but falls back to ASCII in case it is not available
        if (!CServiceBroker::GetSettingsComponent()
                 ->GetAdvancedSettings()
                 ->m_sambadoscodepage.empty())
          fprintf(f, "\tdos charset = %s\n",
                  CServiceBroker::GetSettingsComponent()
                      ->GetAdvancedSettings()
                      ->m_sambadoscodepage.c_str());

        // include users configuration if available
        fprintf(f, "\tinclude = %s/.smb/user.conf\n", home.c_str());

        fclose(f);
      }
    }

    // reads smb.conf so this MUST be after we create smb.conf
    // multiple smbc_init calls are ignored by libsmbclient.
    // note: this is important as it initializes the smb old
    // interface compatibility. Samba 3.4.0 or higher has the new interface.
    // smbc_init is deprecated but still required for the old-interface
    // compatibility used by smbc_set_context / smbc_free_context.
    smbc_init(xb_smbc_auth, 0);

    // setup our context
    m_context = smbc_new_context();
    if (!m_context)
    {
      CLog::LogF(LOGERROR, "smbc_new_context() failed");
      setenv("HOME", truehome.c_str(), 1);
      return;
    }

    // restore HOME
    setenv("HOME", truehome.c_str(), 1);

    // Use the modern Samba 4 API (non-deprecated setter functions).
    smbc_setDebug(m_context, CServiceBroker::GetLogging().CanLogComponent(LOGSAMBA) ? 10 : 0);
    smbc_setLogCallback(m_context, this, xb_smbc_log);
    smbc_setFunctionAuthData(m_context, xb_smbc_auth);
    orig_cache_fn = smbc_getFunctionGetCachedServer(m_context);
    smbc_setFunctionGetCachedServer(m_context, xb_smbc_cache); // [Fix] Install LRU cache
    smbc_setOptionOneSharePerServer(m_context, false);
    smbc_setOptionBrowseMaxLmbCount(m_context, 0);
    smbc_setTimeout(
        m_context,
        CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_sambaclienttimeout * 1000);

    // Some older Samba 4.x headers (pre-4.9 / 4.8) still declare these functions
    // with non-const char* parameters, so a const_cast is needed for compatibility.
    if (!settings->GetString(CSettings::SETTING_SMB_WORKGROUP).empty())
      smbc_setWorkgroup(
          m_context,
          const_cast<char*>(settings->GetString(CSettings::SETTING_SMB_WORKGROUP).c_str()));
    smbc_setUser(m_context, const_cast<char*>("guest"));

    // initialize samba and do some hacking into the settings
    if (smbc_init_context(m_context))
    {
      // setup context using the smb old interface compatibility
      SMBCCTX* old_context = smbc_set_context(m_context);

      // There is a bug in old Samba versions where smbc_set_context may
      // return an already freed pointer on subsequent inits.  IsFirstInit
      // guarantees we only free the very first compatibility context once.
      if (old_context && IsFirstInit)
      {
        smbc_free_context(old_context, 1);
        IsFirstInit = false;
      }

      // Cache the remove_unused_server function pointer.
      g_removeUnusedFn = smbc_getFunctionRemoveUnusedServer(m_context);
      if (!g_removeUnusedFn)
      {
        CLog::LogF(LOGWARNING, "[SMB_SESSION_LIMIT] remove_unused_server function not available; "
                               "sessions may not be closed on eviction");
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

std::string CSMB::URLEncode(const CURL& url)
{
  /* due to smb wanting encoded urls we have to build it manually */

  std::string flat = "smb://";

  /* Samba's URL parser is modern and handles missing username gracefully,
     but we keep the explicit check for safety. */
  if (!url.GetUserName().empty() /* || url.GetPassWord().length() > 0 */)
  {
    if (!url.GetDomain().empty())
    {
      flat += URLEncode(url.GetDomain());
      flat += ";";
    }
    flat += URLEncode(url.GetUserName());
    if (!url.GetPassWord().empty())
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

std::string CSMB::URLEncode(const std::string& value)
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
    std::unique_lock lock(*this);
    if (m_OpenConnections == 0 /* check again - when locked */ && m_context != NULL)
    {
      if (m_IdleTimeout > 0)
      {
        m_IdleTimeout--;
      }
      else
      {
        CLog::Log(LOGINFO, "Samba is idle. Closing the remaining connections");
        Deinit();
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
  std::unique_lock lock(*this);
  m_OpenConnections++;
}
void CSMB::AddIdleConnection()
{
  std::unique_lock lock(*this);
  m_OpenConnections--;
  /* If we close a file we reset the idle timer so that we don't have any weird behaviours if a user
     leaves the movie paused for a long while and then press stop */
  m_IdleTimeout = 180;
}

CURL CSMB::GetResolvedUrl(const CURL& url)
{
  CURL tmpUrl(url);
  std::string resolvedHostName;

  if (CServiceBroker::GetDNSNameCache()->Lookup(tmpUrl.GetHostName(), resolvedHostName))
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
  std::unique_lock lock(smb);
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

  std::unique_lock lock(smb);
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
  if (ret < 0)
  {
    smbc_close(m_fd);
    m_fd = -1;
    return false;
  }
  // We've successfully opened the file!
  return true;
}

int CSMBFile::OpenFile(const CURL& url, std::string& strAuth)
{
  int fd = -1;
  smb.Init();

  strAuth = GetAuthenticatedPath(CSMB::GetResolvedUrl(url));
  std::string strPath = strAuth;

  {
    std::unique_lock lock(smb);
    if (!smb.IsSmbValid())
      return -1;
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
  if (!IsValidFile(url.GetFileName()))
    return false;

  smb.Init();
  std::string strFileName = GetAuthenticatedPath(CSMB::GetResolvedUrl(url));

  struct stat info;

  std::unique_lock lock(smb);
  if (!smb.IsSmbValid())
    return false;
  int iResult = smbc_stat(strFileName.c_str(), &info);

  if (iResult < 0)
    return false;
  return true;
}

int CSMBFile::Stat(struct __stat64* buffer)
{
  if (m_fd == -1)
    return -1;

  struct stat tmpBuffer = {};

  std::unique_lock lock(smb);
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
  std::unique_lock lock(smb);

  if (!smb.IsSmbValid())
    return -1;
  struct stat tmpBuffer = {};
  int iResult = smbc_stat(strFileName.c_str(), &tmpBuffer);
  CUtil::StatToStat64(buffer, &tmpBuffer);
  return iResult;
}

int CSMBFile::Truncate(int64_t size)
{
  if (m_fd == -1)
    return 0;
  /*
 * This would force us to be dependant on SMBv3.2 which is GPLv3
 * This is only used by the TagLib writers, which are not currently in use
 * So log and warn until we implement TagLib writing & can re-implement this better.
  std::unique_lock lock(smb); // Init not called since it has to be "inited" by now

#if defined(TARGET_ANDROID)
  int iResult = 0;
#else
  int iResult = smbc_ftruncate(m_fd, size);
#endif
*/
  CLog::Log(LOGWARNING, "{} - Warning(smbc_ftruncate called and not implemented)", __FUNCTION__);
  return 0;
}

ssize_t CSMBFile::Read(void* lpBuf, size_t uiBufSize)
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

  std::unique_lock lock(smb); // Init not called since it has to be "inited" by now
  if (!smb.IsSmbValid())
    return -1;
  smb.SetActivityTime();

  ssize_t bytesRead = smbc_read(m_fd, lpBuf, (int)uiBufSize);

  if (m_allowRetry && bytesRead < 0 && errno == EINVAL)
  {
    CLog::Log(LOGERROR, "{} - Error( {}, {}, {} ) - Retrying", __FUNCTION__, bytesRead, errno,
              strerror(errno));
    bytesRead = smbc_read(m_fd, lpBuf, (int)uiBufSize);
  }

  if (bytesRead < 0)
    CLog::Log(LOGERROR, "{} - Error( {}, {}, {} )", __FUNCTION__, bytesRead, errno,
              strerror(errno));

  return bytesRead;
}

int64_t CSMBFile::Seek(int64_t iFilePosition, int iWhence)
{
  if (m_fd == -1)
    return -1;

  std::unique_lock lock(smb); // Init not called since it has to be "inited" by now
  if (!smb.IsSmbValid())
    return -1;
  smb.SetActivityTime();
  int64_t pos = smbc_lseek(m_fd, iFilePosition, iWhence);

  if (pos < 0)
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
    std::unique_lock lock(smb);
    if (!smb.IsSmbValid())
      return;
    smbc_close(m_fd);
  }
  m_fd = -1;
}

ssize_t CSMBFile::Write(const void* lpBuf, size_t uiBufSize)
{
  if (m_fd == -1)
    return -1;

  // lpBuf can be safely casted to void* since xbmc_write will only read from it.
  std::unique_lock lock(smb);
  if (!smb.IsSmbValid())
    return -1;

  return smbc_write(m_fd, lpBuf, uiBufSize);
}

bool CSMBFile::Delete(const CURL& url)
{
  smb.Init();
  std::string strFile = GetAuthenticatedPath(CSMB::GetResolvedUrl(url));

  std::unique_lock lock(smb);
  if (!smb.IsSmbValid())
    return false;

  int result = smbc_unlink(strFile.c_str());

  if (result != 0)
    CLog::Log(LOGERROR, "{} - Error( {} )", __FUNCTION__, strerror(errno));

  return (result == 0);
}

bool CSMBFile::Rename(const CURL& url, const CURL& urlnew)
{
  smb.Init();
  std::string strFile = GetAuthenticatedPath(CSMB::GetResolvedUrl(url));
  std::string strFileNew = GetAuthenticatedPath(CSMB::GetResolvedUrl(urlnew));
  std::unique_lock lock(smb);
  if (!smb.IsSmbValid())
    return false;

  int result = smbc_rename(strFile.c_str(), strFileNew.c_str());

  if (result != 0)
    CLog::Log(LOGERROR, "{} - Error( {} )", __FUNCTION__, strerror(errno));

  return (result == 0);
}

bool CSMBFile::OpenForWrite(const CURL& url, bool bOverWrite)
{
  m_fileSize = 0;

  Close();

  // we can't open files like smb://file.f or smb://server/file.f
  // if a file matches the if below return false, it can't exist on a samba share.
  if (!IsValidFile(url.GetFileName()))
    return false;

  std::string strFileName = GetAuthenticatedPath(CSMB::GetResolvedUrl(url));
  std::unique_lock lock(smb);
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
      StringUtils::EndsWith(strFileName, "/..")) /* not parent folder */
    return false;
  return true;
}

std::string CSMBFile::GetAuthenticatedPath(const CURL& url)
{
  CURL authURL(CSMB::GetResolvedUrl(url));
  CPasswordManager::GetInstance().AuthenticateURL(authURL);
  return smb.URLEncode(authURL);
}

int CSMBFile::IoControl(IOControl request, void* param)
{
  if (request == IOControl::SEEK_POSSIBLE)
    return 1;

  if (request == IOControl::SET_RETRY)
  {
    m_allowRetry = *(bool*)param;
    return 0;
  }

  return -1;
}

int CSMBFile::GetChunkSize()
{
  const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();

  if (!settings)
    return (64 * 1024);

  // Only SMBv2.1 and SMBv3 supports large MTU
  if (settings->GetInt(CSettings::SETTING_SMB_MINPROTOCOL) > 2)
  {
    return (settings->GetInt(CSettings::SETTING_SMB_CHUNKSIZE) * 1024);
  }

  return (64 * 1024);
}
