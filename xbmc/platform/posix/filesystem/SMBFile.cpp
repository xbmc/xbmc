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

#include <array>
#include <chrono>
#include <cstring>
#include <inttypes.h>
#include <limits>
#include <list>
#include <mutex>
#include <regex>
#include <thread>
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
// This implementation overrides all four libsmbclient cache hooks
// (get / add / remove / purge) so that a single LRU-based cache
// is the sole, authoritative owner of every server connection.
// MAX_CACHED_SERVERS (15) caps the number of tracked sessions.
//
// Eviction is synchronous - when the cache is full, the least recently
// used entry is torn down immediately via remove_unused_server().
// The call is made without holding g_cacheMutex, because on success
// the library calls back into our own remove_cached_srv_fn which needs
// to acquire that mutex (avoiding a self-deadlock). If the server
// is still busy (open files), it is rotated to the front of the LRU
// so that the next eviction picks a different victim.
//
// libsmbclient adds a small overhead of 1-3 sessions (transport,
// IPC$, etc.) that are not visible to our cache.
// WithMAX_CACHED_SERVERS = 15 the total remains well below
// the Windows 20-session limit.
// ---------------------------------------------------------------
namespace
{
// 15 is a safe value - the total stays under 20 even with the library's
// additional overhead.
constexpr size_t MAX_CACHED_SERVERS = 15;
constexpr std::array<std::chrono::milliseconds, 3> SMB_RECONNECT_BACKOFF{
    std::chrono::milliseconds{100}, std::chrono::milliseconds{250}, std::chrono::milliseconds{500}};

class CLibsmbFileOperations final : public SMBFileRecovery::ISMBFileOperations
{
public:
  void Init() override { smb.Init(); }
  void AddActiveConnection() override { smb.AddActiveConnection(); }
  void AddIdleConnection() override { smb.AddIdleConnection(); }
  void SetActivityTime() override { smb.SetActivityTime(); }
  bool IsValid() const override { return smb.IsSmbValid(); }
  CCriticalSection& GetCriticalSection() override { return smb; }
  CURL Resolve(const CURL& url) override { return CSMB::GetResolvedUrl(url); }
  std::string URLEncode(const CURL& url) override { return smb.URLEncode(url); }

  int Open(const std::string& path, int flags) override
  {
    return smbc_open(path.c_str(), flags, 0);
  }
  int Create(const std::string& path) override { return smbc_creat(path.c_str(), 0); }
  int Close(int fd) override { return smbc_close(fd); }
  ssize_t Read(int fd, void* buffer, size_t size) override { return smbc_read(fd, buffer, size); }
  ssize_t Write(int fd, const void* buffer, size_t size) override
  {
    return smbc_write(fd, buffer, size);
  }
  int64_t Seek(int fd, int64_t offset, int whence) override
  {
    return smbc_lseek(fd, offset, whence);
  }
  int Stat(const std::string& path, struct stat* buffer) override
  {
    return smbc_stat(path.c_str(), buffer);
  }
  int FStat(int fd, struct stat* buffer) override { return smbc_fstat(fd, buffer); }
  int Unlink(const std::string& path) override { return smbc_unlink(path.c_str()); }
  int Rename(const std::string& from, const std::string& to) override
  {
    return smbc_rename(from.c_str(), to.c_str());
  }
};

SMBFileRecovery::ISMBFileOperations& GetDefaultFileOperations()
{
  static CLibsmbFileOperations operations;
  return operations;
}

bool AddOffset(int64_t base, int64_t offset, int64_t& result)
{
  if ((offset > 0 && base > std::numeric_limits<int64_t>::max() - offset) ||
      (offset < 0 && base < std::numeric_limits<int64_t>::min() - offset))
    return false;

  result = base + offset;
  return true;
}

enum class SeekStatus
{
  SUCCESS,
  ERROR,
  POSITION_INVALID,
};

SeekStatus SeekFileExactly(SMBFileRecovery::ISMBFileOperations& fileOperations,
                           int fd,
                           int64_t offset,
                           int whence,
                           int64_t& resolvedPosition)
{
  errno = 0;
  resolvedPosition = fileOperations.Seek(fd, offset, whence);
  if (resolvedPosition < 0)
  {
    if (errno == 0)
      errno = EIO;
    return SeekStatus::ERROR;
  }

  if (whence == SEEK_SET)
  {
    if (resolvedPosition != offset)
    {
      errno = EIO;
      return SeekStatus::POSITION_INVALID;
    }

    errno = 0;
    return SeekStatus::SUCCESS;
  }

  if (whence != SEEK_END)
  {
    errno = EINVAL;
    return SeekStatus::ERROR;
  }

  errno = 0;
  const int64_t validatedPosition = fileOperations.Seek(fd, resolvedPosition, SEEK_SET);
  if (validatedPosition != resolvedPosition)
  {
    if (validatedPosition >= 0)
      errno = EIO;
    else if (errno == 0)
      errno = EIO;
    return SeekStatus::POSITION_INVALID;
  }

  errno = 0;
  return SeekStatus::SUCCESS;
}

// Cached remove_unused_server function pointer, obtained once during Init.
smbc_remove_unused_server_fn g_removeUnusedFn = nullptr;

// Build a cache key from the four components that libsmbclient uses
// internally. Backslashes are used as separators because they cannot
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
std::list<CacheEntry> g_lru;
// Map from key to LRU iterator for O(1) lookup.
std::unordered_map<std::string, std::list<CacheEntry>::iterator> g_map;
// Reverse index: remove_cached_srv_fn only gives us an SMBCSRV*,
// so we need to map back to a key to clean up g_lru/g_map.
std::unordered_map<SMBCSRV*, std::string> g_srvToKey;
// g_cacheMutex protects g_lru, g_map, and g_srvToKey. In practice
// every caller already holds the global CSMB mutex, so this mutex
// is not doing independent serialisation work - it is kept as cheap
// future-proofing in case a code path ever calls the cache hooks
// without the outer lock.
std::mutex g_cacheMutex;

// -----------------------------------------------------------------
// get_cached_srv_fn - pure lookup against our own tracking.
// Because we own all four hooks, an entry that is not in g_map
// genuinely does not exist yet; libsmbclient's SMBC_server()
// will create a new connection and report it back via
// xb_smbc_cache_add(). No network call happens here, ever.
// -----------------------------------------------------------------
SMBCSRV* xb_smbc_cache_get(SMBCCTX* /*c*/,
                           const char* server,
                           const char* share,
                           const char* workgroup,
                           const char* username)
{
  std::lock_guard<std::mutex> lock(g_cacheMutex);

  const std::string key = MakeKey(server ? server : "", share ? share : "",
                                  username ? username : "", workgroup ? workgroup : "");

  auto it = g_map.find(key);
  if (it == g_map.end())
    return nullptr;

  // Move to front (most recently used)
  g_lru.splice(g_lru.begin(), g_lru, it->second);
  return it->second->srv;
}

// -----------------------------------------------------------------
// remove_cached_srv_fn - called whenever a server is actually torn
// down: either by our own synchronous eviction, or by libsmbclient's
// built-in staleness check. This keeps g_map/g_lru accurate.
// -----------------------------------------------------------------
int xb_smbc_cache_remove(SMBCCTX* /*c*/, SMBCSRV* srv)
{
  std::lock_guard<std::mutex> lock(g_cacheMutex);

  auto keyIt = g_srvToKey.find(srv);
  if (keyIt == g_srvToKey.end())
    return 0; // not tracked - already gone, or never entered our cache

  auto mapIt = g_map.find(keyIt->second);
  if (mapIt != g_map.end())
  {
    g_lru.erase(mapIt->second);
    g_map.erase(mapIt);
  }
  g_srvToKey.erase(keyIt);
  return 0;
}

// -----------------------------------------------------------------
// purge_cached_fn - tells smbc_free_context()'s shutdown path
// whether we still hold anything. If we report "not fully purged",
// it falls back to force-closing every remaining live server
// itself - a safety net we must not silently disable.
// -----------------------------------------------------------------
int xb_smbc_cache_purge(SMBCCTX* /*c*/)
{
  std::lock_guard<std::mutex> lock(g_cacheMutex);
  return g_lru.empty() ? 0 : 1;
}

// -----------------------------------------------------------------
// Evict the LRU tail immediately.
// remove_unused_server() is a synchronous network call (tree
// disconnect + socket close), so it must run WITHOUT g_cacheMutex
// held: on success it calls back into xb_smbc_cache_remove(),
// which needs to take g_cacheMutex itself - holding it here too
// would self-deadlock (std::mutex isn't recursive).
// Must be called with the CSMB lock held, same as any other
// libsmbclient call.
// -----------------------------------------------------------------
void EvictIfOverCapacity(SMBCCTX* c)
{
  // Try a few candidates - busy checks are cheap (no network I/O),
  // so we can skip over busy entries to keep the tracked count tight.
  constexpr int kMaxCandidates = 5;
  for (int attempt = 0; attempt < kMaxCandidates; ++attempt)
  {
    // May perform more than one network round-trip if we've drifted over capacity.
    SMBCSRV* victim = nullptr;
    std::string victimKey;
    {
      std::lock_guard<std::mutex> lock(g_cacheMutex);
      if (g_lru.size() <= MAX_CACHED_SERVERS)
        return;
      victim = g_lru.back().srv;
      victimKey = g_lru.back().key;
    }

    if (!g_removeUnusedFn || !victim)
      return;

    auto start = std::chrono::steady_clock::now(); // measures elapsed time
    int rc = g_removeUnusedFn(c, victim);
    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::steady_clock::now() - start)
                         .count();

    if (rc == 0)
    {
      if (elapsedMs > 50)
        CLog::LogF(LOGWARNING, "[SMB_SESSION_LIMIT] '{}' eviction took {} ms", victimKey,
                   elapsedMs);
      else
        CLog::LogF(LOGDEBUG, "[SMB_SESSION_LIMIT] '{}' evicted (cache full)", victimKey);
      // xb_smbc_cache_remove() already erased the entry for us.
      continue; // re-check size; loop exits once back at/under capacity
    }

    // Still busy - rotate to front and try the next candidate.
    {
      std::lock_guard<std::mutex> lock(g_cacheMutex);
      auto it = g_map.find(victimKey);
      if (it != g_map.end() && it->second->srv == victim)
        g_lru.splice(g_lru.begin(), g_lru, it->second);
    }
    CLog::LogF(LOGDEBUG, "[SMB_SESSION_LIMIT] '{}' still busy, trying next candidate", victimKey);
  }
}

// -----------------------------------------------------------------
// add_cached_srv_fn - called by libsmbclient exactly once,
// right after it creates a genuinely new connection.
// This is the single correct point for a new entry to enter our cache.
// Eviction is performed immediately if we are over capacity.
// -----------------------------------------------------------------
int xb_smbc_cache_add(SMBCCTX* c,
                      SMBCSRV* srv,
                      const char* server,
                      const char* share,
                      const char* workgroup,
                      const char* username)
{
  {
    std::lock_guard<std::mutex> lock(g_cacheMutex);
    const std::string key = MakeKey(server ? server : "", share ? share : "",
                                    username ? username : "", workgroup ? workgroup : "");
    g_lru.push_front({key, srv});
    g_map[key] = g_lru.begin();
    g_srvToKey[srv] = key;
    CLog::LogF(LOGDEBUG, "[SMB_SESSION_LIMIT] Server cache ADD '{}' (total cached: {})", key,
               g_lru.size());
  }

  EvictIfOverCapacity(c);
  return 0;
}

// -----------------------------------------------------------------
// Called from CSMB::Deinit() - synchronously close everything
// we still track before the context is freed.
// Snapshot the servers first, then call remove_unused_server()
// WITHOUT g_cacheMutex held to avoid the deadlock described
// in EvictIfOverCapacity. Entries that still fail (busy)
// are left tracked; purge will then report "not fully purged" and
// smbc_free_context()'s own force-close fallback will finish them.
// -----------------------------------------------------------------
void ClearServerCache(SMBCCTX* c)
{
  std::list<SMBCSRV*> toClose;
  {
    std::lock_guard<std::mutex> lock(g_cacheMutex);
    for (const auto& entry : g_lru)
      toClose.push_back(entry.srv);
  }

  if (g_removeUnusedFn)
  {
    for (SMBCSRV* srv : toClose)
    {
      if (!srv)
        continue;
      int rc = g_removeUnusedFn(c, srv);
      if (rc != 0)
        CLog::LogF(LOGWARNING,
                   "[SMB_SESSION_LIMIT] ClearServerCache: remove_unused_server failed (rc={}) - "
                   "left tracked for smbc_free_context's own cleanup",
                   rc);
    }
  }
  else
  {
    CLog::LogF(LOGWARNING,
               "[SMB_SESSION_LIMIT] ClearServerCache: remove_unused_server not available");
  }
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

// Track whether initialization has completed so an existing SMB configuration
// directory is accepted only during the first initialization.
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

  // Close everything we're still tracking before the context goes
  // away. Our hooks are safe to leave installed: once cleared,
  // they are inert no-ops.
  if (m_context)
    ClearServerCache(m_context);

  /* samba goes loco if deinited while it has some files opened */
  if (m_context)
  {
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
    // Install our cache hooks: get, add, remove, purge - we are
    // the sole authoritative owner of server caching.
    smbc_setFunctionGetCachedServer(m_context, xb_smbc_cache_get);
    smbc_setFunctionAddCachedServer(m_context, xb_smbc_cache_add);
    smbc_setFunctionRemoveCachedServer(m_context, xb_smbc_cache_remove);
    smbc_setFunctionPurgeCachedServers(m_context, xb_smbc_cache_purge);
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
      smbc_set_context(m_context);
      IsFirstInit = false;

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

CSMBFile::CSMBFile() : CSMBFile(GetDefaultFileOperations())
{
}

CSMBFile::CSMBFile(SMBFileRecovery::ISMBFileOperations& fileOperations)
  : m_fileOperations(fileOperations)
{
  m_fileOperations.Init();
  m_fd = -1;
  m_fileOperations.AddActiveConnection();
  m_allowRetry = true;
}

CSMBFile::~CSMBFile()
{
  Close();
  m_fileOperations.AddIdleConnection();
}

int64_t CSMBFile::GetPosition()
{
  std::unique_lock lock(m_fileOperations.GetCriticalSection());
  if (m_fd == -1 || !m_fileOperations.IsValid())
    return -1;
  return m_fileOperations.Seek(m_fd, 0, SEEK_CUR);
}

int64_t CSMBFile::GetLength()
{
  std::unique_lock lock(m_fileOperations.GetCriticalSection());
  if (m_fd == -1 && !m_reopenOnNextRead)
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
  // Keep the URL used for reconnects password-free. Explicit credentials are
  // copied to the password manager after the initial open succeeds.
  m_url = CURL(url.GetWithoutUserDetails());
  m_url.SetDomain(url.GetDomain());
  m_url.SetUserName(url.GetUserName());

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

  if (!url.GetPassWord().empty())
  {
    CPasswordManager& passwordManager = CPasswordManager::GetInstance();
    passwordManager.SaveAuthenticatedURL(url, false);

    const CURL resolvedUrl = m_fileOperations.Resolve(url);
    if (resolvedUrl.GetHostName() != url.GetHostName())
      passwordManager.SaveAuthenticatedURL(resolvedUrl, false);
  }

  std::unique_lock lock(m_fileOperations.GetCriticalSection());
  if (!m_fileOperations.IsValid())
  {
    // Deinit() closes open files when destroying the SMB context, so only
    // the now-invalid descriptor needs to be reset here.
    m_fd = -1;
    return false;
  }
  struct stat tmpBuffer;
  if (m_fileOperations.Stat(strFileName, &tmpBuffer) < 0)
  {
    m_fileOperations.Close(m_fd);
    m_fd = -1;
    return false;
  }

  m_fileSize = tmpBuffer.st_size;

  int64_t ret = m_fileOperations.Seek(m_fd, 0, SEEK_SET);
  if (ret < 0)
  {
    m_fileOperations.Close(m_fd);
    m_fd = -1;
    return false;
  }

  m_reopenEnabled = true;
  m_reopenOnNextRead = false;
  m_readPosition = ret;
  ++m_positionGeneration;
  m_reopenInProgressGeneration = 0;
  ResetRecoveryStateLocked();

  // We've successfully opened the file!
  return true;
}

int CSMBFile::OpenFile(const CURL& url, std::string& strAuth)
{
  int fd = -1;
  m_fileOperations.Init();

  strAuth = GetAuthenticatedPath(m_fileOperations.Resolve(url));
  std::string strPath = strAuth;

  {
    std::unique_lock lock(m_fileOperations.GetCriticalSection());
    if (!m_fileOperations.IsValid())
      return -1;
    fd = m_fileOperations.Open(strPath, O_RDONLY);
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

  m_fileOperations.Init();
  std::string strFileName = GetAuthenticatedPath(m_fileOperations.Resolve(url));

  struct stat info;

  std::unique_lock lock(m_fileOperations.GetCriticalSection());
  if (!m_fileOperations.IsValid())
    return false;
  int iResult = m_fileOperations.Stat(strFileName, &info);

  if (iResult < 0)
    return false;
  return true;
}

int CSMBFile::Stat(struct __stat64* buffer)
{
  struct stat tmpBuffer = {};

  std::unique_lock lock(m_fileOperations.GetCriticalSection());
  if (m_fd == -1 || !m_fileOperations.IsValid())
    return -1;
  int iResult = m_fileOperations.FStat(m_fd, &tmpBuffer);
  if (iResult == 0)
    m_fileSize = tmpBuffer.st_size;
  CUtil::StatToStat64(buffer, &tmpBuffer);
  return iResult;
}

int CSMBFile::Stat(const CURL& url, struct __stat64* buffer)
{
  m_fileOperations.Init();
  std::string strFileName = GetAuthenticatedPath(m_fileOperations.Resolve(url));
  std::unique_lock lock(m_fileOperations.GetCriticalSection());

  if (!m_fileOperations.IsValid())
    return -1;
  struct stat tmpBuffer = {};
  int iResult = m_fileOperations.Stat(strFileName, &tmpBuffer);
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

bool CSMBFile::CanAttemptRecoveryLocked()
{
  if (!m_reopenEnabled || m_recoveryAttempts >= SMB_RECONNECT_BACKOFF.size())
  {
    m_reopenEnabled = false;
    m_reopenOnNextRead = false;
    errno = m_lastRecoveryError != 0 ? m_lastRecoveryError : EIO;
    return false;
  }

  return true;
}

bool CSMBFile::BeginRecoveryAttemptLocked()
{
  if (!CanAttemptRecoveryLocked())
    return false;
  ++m_recoveryAttempts;
  return true;
}

void CSMBFile::ResetRecoveryStateLocked()
{
  m_recoveryAttempts = 0;
  m_lastRecoveryError = 0;
}

bool CSMBFile::ReopenAtPositionLocked(int64_t offset,
                                      int whence,
                                      const std::string& reopenPath,
                                      int64_t& resolvedPosition)
{
  if (!m_fileOperations.IsValid())
  {
    errno = ENOTCONN;
    return false;
  }

  if (!m_reopenEnabled)
  {
    errno = EINVAL;
    return false;
  }

  errno = 0;
  const int newFd = m_fileOperations.Open(reopenPath, O_RDONLY);
  if (newFd < 0)
  {
    if (errno == 0)
      errno = EIO;
    return false;
  }

  struct stat tmpBuffer = {};
  errno = 0;
  const bool fileSizeAvailable = m_fileOperations.FStat(newFd, &tmpBuffer) == 0;
  if (fileSizeAvailable)
    m_fileSize = tmpBuffer.st_size;

  if (SeekFileExactly(m_fileOperations, newFd, offset, whence, resolvedPosition) !=
      SeekStatus::SUCCESS)
  {
    const int seekErrno = errno;
    m_fileOperations.Close(newFd);
    errno = seekErrno;
    return false;
  }

  m_fd = newFd;
  return true;
}

ssize_t CSMBFile::Read(void* lpBuf, size_t uiBufSize)
{
  if (uiBufSize > SSIZE_MAX)
    uiBufSize = SSIZE_MAX;

  // Some external libs (libass) use test read with zero size and
  // null buffer pointer to check whether file is readable, but
  // libsmbclient always return "-1" if called with null buffer
  // regardless of buffer size.
  // To overcome this, force return "0" in that case.
  std::unique_lock lock(m_fileOperations.GetCriticalSection());
  if (uiBufSize == 0 && lpBuf == NULL)
    return m_fd == -1 ? -1 : 0;
  if (!m_fileOperations.IsValid())
    return -1;

  if (m_fd == -1)
  {
    if (!m_reopenOnNextRead)
      return -1;
    if (m_reopenInProgressGeneration != 0)
    {
      errno = EAGAIN;
      return -1;
    }

    const CURL reopenUrl = m_url;
    const int64_t reopenOffset = m_readPosition;
    const uint64_t reopenGeneration = m_positionGeneration;
    m_reopenInProgressGeneration = reopenGeneration;
    lock.unlock();
    const std::string reopenPath = GetAuthenticatedPath(reopenUrl);
    lock.lock();

    if (!m_reopenOnNextRead || m_positionGeneration != reopenGeneration ||
        m_reopenInProgressGeneration != reopenGeneration)
    {
      errno = ECANCELED;
      return -1;
    }

    int64_t resolvedPosition = -1;
    if (!BeginRecoveryAttemptLocked() ||
        !ReopenAtPositionLocked(reopenOffset, SEEK_SET, reopenPath, resolvedPosition))
    {
      const int reopenErrno = errno != 0 ? errno : EIO;
      m_lastRecoveryError = reopenErrno;
      if (!SMBFileRecovery::IsReconnectableReadError(reopenErrno))
      {
        m_reopenEnabled = false;
        m_reopenOnNextRead = false;
      }
      if (m_reopenInProgressGeneration == reopenGeneration)
        m_reopenInProgressGeneration = 0;
      errno = reopenErrno;
      return -1;
    }
    m_reopenOnNextRead = false;
    m_reopenInProgressGeneration = 0;
  }

  m_fileOperations.SetActivityTime();

  const int64_t failedReadOffset = m_readPosition;
  auto readLocked = [&]()
  {
    errno = 0;
    ssize_t result = m_fileOperations.Read(m_fd, lpBuf, uiBufSize);
    int readErrno = result < 0 ? errno : 0;

    if (m_allowRetry && result < 0 && readErrno == EINVAL)
    {
      CLog::Log(LOGERROR, "{} - Error( {}, {}, {} ) - Retrying", __FUNCTION__, result, readErrno,
                strerror(readErrno));
      errno = 0;
      result = m_fileOperations.Read(m_fd, lpBuf, uiBufSize);
      readErrno = result < 0 ? errno : 0;
    }

    if (result < 0 && readErrno == 0)
      readErrno = EIO;
    errno = readErrno;
    return result;
  };

  ssize_t bytesRead = readLocked();
  if (bytesRead > 0)
  {
    m_readPosition += bytesRead;
    ResetRecoveryStateLocked();
    return bytesRead;
  }

  int readErrno = errno;
  const bool unexpectedEof =
      bytesRead == 0 && !SMBFileRecovery::IsValidEof(failedReadOffset, m_fileSize);
  if (!m_reopenEnabled || (!unexpectedEof && !SMBFileRecovery::IsReconnectableReadError(readErrno)))
  {
    if (bytesRead == 0 && !unexpectedEof)
      ResetRecoveryStateLocked();
    if (bytesRead < 0)
      CLog::Log(LOGERROR, "{} - Error( {}, {}, {} )", __FUNCTION__, bytesRead, readErrno,
                strerror(readErrno));
    errno = readErrno;
    return bytesRead;
  }

  const int originalReadErrno = unexpectedEof ? EIO : readErrno;
  m_lastRecoveryError = originalReadErrno;
  const uint64_t recoveryGeneration = m_positionGeneration;

  if (!m_allowRetry)
  {
    // Let the caller schedule retries, but ensure its next Read() reopens the connection instead of
    // using the failed descriptor again.
    const int failedFd = m_fd;
    m_fd = -1;
    m_reopenOnNextRead = true;
    m_reopenInProgressGeneration = 0;
    m_fileOperations.Close(failedFd);
    errno = originalReadErrno;
    return bytesRead;
  }

  int finalErrno = originalReadErrno;
  const CURL reopenUrl = m_url;
  const int failedFd = m_fd;
  m_fd = -1;
  m_reopenOnNextRead = true;
  m_reopenInProgressGeneration = recoveryGeneration;
  m_fileOperations.Close(failedFd);
  errno = originalReadErrno;
  lock.unlock();

  unsigned int attempts = 0;
  while (true)
  {
    lock.lock();
    if (!m_reopenOnNextRead || m_positionGeneration != recoveryGeneration || m_fd != -1 ||
        m_reopenInProgressGeneration != recoveryGeneration)
    {
      lock.unlock();
      errno = ECANCELED;
      return -1;
    }

    if (!CanAttemptRecoveryLocked())
    {
      finalErrno = errno;
      attempts = m_recoveryAttempts;
      lock.unlock();
      break;
    }

    const auto backoff = SMB_RECONNECT_BACKOFF[m_recoveryAttempts];
    lock.unlock();
    std::this_thread::sleep_for(backoff);
    const std::string reopenPath = GetAuthenticatedPath(reopenUrl);
    lock.lock();

    if (!m_reopenOnNextRead || m_positionGeneration != recoveryGeneration || m_fd != -1 ||
        m_reopenInProgressGeneration != recoveryGeneration)
    {
      lock.unlock();
      errno = ECANCELED;
      return -1;
    }

    if (!BeginRecoveryAttemptLocked())
    {
      finalErrno = errno;
      attempts = m_recoveryAttempts;
      lock.unlock();
      break;
    }
    attempts = m_recoveryAttempts;

    int64_t resolvedPosition = -1;
    if (!ReopenAtPositionLocked(failedReadOffset, SEEK_SET, reopenPath, resolvedPosition))
    {
      finalErrno = errno != 0 ? errno : EIO;
      m_lastRecoveryError = finalErrno;
      lock.unlock();
      if (!SMBFileRecovery::IsReconnectableReadError(finalErrno))
        break;
      continue;
    }

    bytesRead = readLocked();
    readErrno = errno;
    if (bytesRead > 0)
    {
      m_reopenOnNextRead = false;
      m_reopenInProgressGeneration = 0;
      m_readPosition += bytesRead;
      ResetRecoveryStateLocked();
      CLog::Log(LOGWARNING, "SMB read recovered at offset {} on attempt {}", failedReadOffset,
                attempts);
      errno = 0;
      return bytesRead;
    }

    if (bytesRead == 0 && SMBFileRecovery::IsValidEof(failedReadOffset, m_fileSize))
    {
      m_reopenOnNextRead = false;
      m_reopenInProgressGeneration = 0;
      ResetRecoveryStateLocked();
      CLog::Log(LOGWARNING, "SMB read recovered at offset {} on attempt {}", failedReadOffset,
                attempts);
      errno = 0;
      return 0;
    }

    const bool retryUnexpectedEof = bytesRead == 0;
    finalErrno = retryUnexpectedEof ? originalReadErrno : readErrno;
    m_lastRecoveryError = finalErrno;
    const int replacementFd = m_fd;
    m_fd = -1;
    m_fileOperations.Close(replacementFd);
    errno = finalErrno;
    lock.unlock();

    if (!retryUnexpectedEof && !SMBFileRecovery::IsReconnectableReadError(finalErrno))
      break;
  }

  lock.lock();
  if (m_positionGeneration != recoveryGeneration)
  {
    lock.unlock();
    errno = ECANCELED;
    return -1;
  }
  m_fd = -1;
  m_reopenEnabled = false;
  m_reopenOnNextRead = false;
  if (m_reopenInProgressGeneration == recoveryGeneration)
    m_reopenInProgressGeneration = 0;
  m_lastRecoveryError = finalErrno;
  lock.unlock();

  CLog::Log(LOGERROR, "SMB read recovery failed at offset {} after {} attempts: errno {}",
            failedReadOffset, attempts, finalErrno);
  errno = finalErrno;

  return -1;
}

int64_t CSMBFile::Seek(int64_t iFilePosition, int iWhence)
{
  std::unique_lock lock(m_fileOperations.GetCriticalSection());
  if (!m_fileOperations.IsValid())
    return -1;
  m_fileOperations.SetActivityTime();

  if (m_fd != -1 && !m_reopenEnabled)
    return m_fileOperations.Seek(m_fd, iFilePosition, iWhence);

  const bool seekFromEnd = iWhence == SEEK_END;
  int64_t target = iFilePosition;
  if (iWhence == SEEK_CUR)
  {
    if (!AddOffset(m_readPosition, iFilePosition, target))
    {
      errno = EOVERFLOW;
      return -1;
    }
  }
  else if (iWhence != SEEK_SET && !seekFromEnd)
  {
    errno = EINVAL;
    return -1;
  }

  if (!seekFromEnd && target < 0)
  {
    errno = EINVAL;
    return -1;
  }

  if (m_fd != -1)
  {
    int64_t resolvedPosition = -1;
    const SeekStatus seekStatus =
        SeekFileExactly(m_fileOperations, m_fd, seekFromEnd ? iFilePosition : target,
                        seekFromEnd ? SEEK_END : SEEK_SET, resolvedPosition);
    if (seekStatus == SeekStatus::SUCCESS)
    {
      target = resolvedPosition;
      if (seekFromEnd)
      {
        struct stat tmpBuffer = {};
        if (m_fileOperations.FStat(m_fd, &tmpBuffer) == 0)
          m_fileSize = tmpBuffer.st_size;
      }
      m_readPosition = target;
      ++m_positionGeneration;
      errno = 0;
      return target;
    }

    const int seekErrno = errno != 0 ? errno : EIO;
    if (seekStatus == SeekStatus::POSITION_INVALID &&
        !SMBFileRecovery::IsReconnectableReadError(seekErrno))
    {
      const int failedFd = m_fd;
      m_fd = -1;
      m_reopenEnabled = false;
      m_reopenOnNextRead = false;
      m_reopenInProgressGeneration = 0;
      m_lastRecoveryError = seekErrno;
      ++m_positionGeneration;
      m_fileOperations.Close(failedFd);
      errno = seekErrno;
      return -1;
    }

    if (!SMBFileRecovery::IsReconnectableReadError(seekErrno))
    {
      CLog::Log(LOGERROR, "{} - Error( {}, {}, {} )", __FUNCTION__, resolvedPosition, seekErrno,
                strerror(seekErrno));
      errno = seekErrno;
      return -1;
    }

    const int failedFd = m_fd;
    m_lastRecoveryError = seekErrno;
    m_fd = -1;
    m_reopenOnNextRead = true;
    m_fileOperations.Close(failedFd);
  }
  else if (!m_reopenEnabled || !m_reopenOnNextRead)
  {
    return -1;
  }

  const uint64_t seekGeneration = ++m_positionGeneration;
  m_reopenInProgressGeneration = seekGeneration;
  const CURL reopenUrl = m_url;
  int finalErrno = m_lastRecoveryError != 0 ? m_lastRecoveryError : EIO;
  unsigned int attempts = 0;

  while (true)
  {
    if (!CanAttemptRecoveryLocked())
    {
      finalErrno = errno;
      attempts = m_recoveryAttempts;
      break;
    }

    const auto backoff = SMB_RECONNECT_BACKOFF[m_recoveryAttempts];
    lock.unlock();
    std::this_thread::sleep_for(backoff);
    const std::string reopenPath = GetAuthenticatedPath(reopenUrl);
    lock.lock();

    if (!m_reopenOnNextRead || m_positionGeneration != seekGeneration || m_fd != -1 ||
        m_reopenInProgressGeneration != seekGeneration)
    {
      errno = ECANCELED;
      return -1;
    }

    if (!BeginRecoveryAttemptLocked())
    {
      finalErrno = errno;
      attempts = m_recoveryAttempts;
      break;
    }
    attempts = m_recoveryAttempts;

    int64_t resolvedPosition = -1;
    if (ReopenAtPositionLocked(seekFromEnd ? iFilePosition : target,
                               seekFromEnd ? SEEK_END : SEEK_SET, reopenPath, resolvedPosition))
    {
      target = resolvedPosition;
      m_readPosition = target;
      m_reopenOnNextRead = false;
      m_reopenInProgressGeneration = 0;
      CLog::Log(LOGWARNING, "SMB seek recovered at offset {} on attempt {}", target, attempts);
      errno = 0;
      return target;
    }

    finalErrno = errno != 0 ? errno : EIO;
    m_lastRecoveryError = finalErrno;
    if (!SMBFileRecovery::IsReconnectableReadError(finalErrno))
    {
      m_reopenEnabled = false;
      m_reopenOnNextRead = false;
      break;
    }
  }

  m_fd = -1;
  m_reopenEnabled = false;
  m_reopenOnNextRead = false;
  if (m_reopenInProgressGeneration == seekGeneration)
    m_reopenInProgressGeneration = 0;
  m_lastRecoveryError = finalErrno;
  CLog::Log(LOGERROR, "SMB seek recovery failed at offset {} after {} attempts: errno {}", target,
            attempts, finalErrno);
  errno = finalErrno;
  return -1;
}

void CSMBFile::Close()
{
  std::unique_lock lock(m_fileOperations.GetCriticalSection());

  const int fd = m_fd;
  m_fd = -1;
  m_reopenEnabled = false;
  m_reopenOnNextRead = false;
  m_reopenInProgressGeneration = 0;
  m_readPosition = 0;
  ++m_positionGeneration;
  ResetRecoveryStateLocked();

  if (fd != -1 && m_fileOperations.IsValid())
  {
    CLog::Log(LOGDEBUG, "CSMBFile::Close closing fd {}", fd);
    m_fileOperations.Close(fd);
  }
}

ssize_t CSMBFile::Write(const void* lpBuf, size_t uiBufSize)
{
  std::unique_lock lock(m_fileOperations.GetCriticalSection());
  if (m_fd == -1 || !m_fileOperations.IsValid())
    return -1;

  return m_fileOperations.Write(m_fd, lpBuf, uiBufSize);
}

bool CSMBFile::Delete(const CURL& url)
{
  m_fileOperations.Init();
  std::string strFile = GetAuthenticatedPath(m_fileOperations.Resolve(url));

  std::unique_lock lock(m_fileOperations.GetCriticalSection());
  if (!m_fileOperations.IsValid())
    return false;

  int result = m_fileOperations.Unlink(strFile);

  if (result != 0)
    CLog::Log(LOGERROR, "{} - Error( {} )", __FUNCTION__, strerror(errno));

  return (result == 0);
}

bool CSMBFile::Rename(const CURL& url, const CURL& urlnew)
{
  m_fileOperations.Init();
  std::string strFile = GetAuthenticatedPath(m_fileOperations.Resolve(url));
  std::string strFileNew = GetAuthenticatedPath(m_fileOperations.Resolve(urlnew));
  std::unique_lock lock(m_fileOperations.GetCriticalSection());
  if (!m_fileOperations.IsValid())
    return false;

  int result = m_fileOperations.Rename(strFile, strFileNew);

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

  std::string strFileName = GetAuthenticatedPath(m_fileOperations.Resolve(url));
  std::unique_lock lock(m_fileOperations.GetCriticalSection());
  if (!m_fileOperations.IsValid())
    return false;

  if (bOverWrite)
  {
    CLog::Log(LOGWARNING, "SMBFile::OpenForWrite() called with overwriting enabled! - {}",
              CURL::GetRedacted(strFileName));
    m_fd = m_fileOperations.Create(strFileName);
  }
  else
  {
    m_fd = m_fileOperations.Open(strFileName, O_RDWR);
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
  CURL authURL(m_fileOperations.Resolve(url));
  CPasswordManager::GetInstance().AuthenticateURL(authURL);
  return m_fileOperations.URLEncode(authURL);
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
