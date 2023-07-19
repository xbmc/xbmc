/*
 *  Copyright (C) 2013 Arne Morten Kvarving
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VFSEntry.h"

#include "ServiceBroker.h"
#include "URL.h"
#include "addons/AddonEvents.h"
#include "addons/AddonManager.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonType.h"
#include "addons/interfaces/Filesystem.h"
#include "network/ZeroconfBrowser.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <mutex>
#include <utility>

#if defined(TARGET_WINDOWS)
#ifndef S_IFLNK
#define S_IFLNK 0120000
#endif
#ifndef S_IFBLK
#define S_IFBLK 0
#endif
#ifndef S_IFSOCK
#define S_IFSOCK 0
#endif
#ifndef S_IFREG
#define S_IFREG _S_IFREG
#endif
#ifndef S_IFCHR
#define S_IFCHR _S_IFCHR
#endif
#ifndef S_IFDIR
#define S_IFDIR _S_IFDIR
#endif
#ifndef S_IFIFO
#define S_IFIFO _S_IFIFO
#endif
#endif

namespace ADDON
{

CVFSAddonCache::~CVFSAddonCache()
{
  Deinit();
}

void CVFSAddonCache::Init()
{
  CServiceBroker::GetAddonMgr().Events().Subscribe(this, &CVFSAddonCache::OnEvent);

  // Load all available VFS addons during Kodi start
  std::vector<AddonInfoPtr> addonInfos;
  CServiceBroker::GetAddonMgr().GetAddonInfos(addonInfos, true, AddonType::VFS);

  std::unique_lock<CCriticalSection> lock(m_critSection);
  for (const auto& addonInfo : addonInfos)
  {
    VFSEntryPtr vfs = std::make_shared<CVFSEntry>(addonInfo);
    vfs->Addon()->RegisterInformer(this);

    m_addonsInstances.emplace_back(vfs);

    if (!vfs->GetZeroconfType().empty())
      CZeroconfBrowser::GetInstance()->AddServiceType(vfs->GetZeroconfType());
  }
}

void CVFSAddonCache::Deinit()
{
  CServiceBroker::GetAddonMgr().Events().Unsubscribe(this);
}

const std::vector<VFSEntryPtr> CVFSAddonCache::GetAddonInstances()
{
  std::unique_lock<CCriticalSection> lock(m_critSection);
  return m_addonsInstances;
}

VFSEntryPtr CVFSAddonCache::GetAddonInstance(const std::string& strId)
{
  VFSEntryPtr addon;

  std::unique_lock<CCriticalSection> lock(m_critSection);

  const auto& itAddon = std::find_if(m_addonsInstances.begin(), m_addonsInstances.end(),
    [&strId](const VFSEntryPtr& addon)
    {
      return addon->ID() == strId;
    });

  if (itAddon != m_addonsInstances.end())
    addon = *itAddon;

  return addon;
}

void CVFSAddonCache::OnEvent(const AddonEvent& event)
{
  if (typeid(event) == typeid(AddonEvents::Disabled))
  {
    for (const auto& vfs : m_addonsInstances)
    {
      if (vfs->ID() == event.addonId && !vfs->GetZeroconfType().empty())
        CZeroconfBrowser::GetInstance()->RemoveServiceType(vfs->GetZeroconfType());
    }
  }

  if (typeid(event) == typeid(AddonEvents::Enabled) ||
      typeid(event) == typeid(AddonEvents::Disabled) ||
      typeid(event) == typeid(AddonEvents::ReInstalled))
  {
    if (CServiceBroker::GetAddonMgr().HasType(event.addonId, AddonType::VFS))
      Update(event.addonId);
  }
  else if (typeid(event) == typeid(AddonEvents::UnInstalled))
  {
    Update(event.addonId);
  }
}

bool CVFSAddonCache::IsInUse(const std::string& id)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  const auto& itAddon = std::find_if(m_addonsInstances.begin(), m_addonsInstances.end(),
                                     [&id](const VFSEntryPtr& addon) { return addon->ID() == id; });
  if (itAddon != m_addonsInstances.end() && (*itAddon).use_count() > 1)
    return true;
  return false;
}

void CVFSAddonCache::Update(const std::string& id)
{
  std::vector<VFSEntryPtr> addonmap;

  // Stop used instance if present, otherwise the new becomes created on already created addon base one.
  {
    std::unique_lock<CCriticalSection> lock(m_critSection);

    const auto& itAddon =
        std::find_if(m_addonsInstances.begin(), m_addonsInstances.end(),
                     [&id](const VFSEntryPtr& addon) { return addon->ID() == id; });

    if (itAddon != m_addonsInstances.end())
    {
      (*itAddon)->Addon()->RegisterInformer(nullptr);
      m_addonsInstances.erase(itAddon);
    }
  }

  // Create and init the new VFS addon instance
  AddonInfoPtr addonInfo = CServiceBroker::GetAddonMgr().GetAddonInfo(id, AddonType::VFS);
  if (addonInfo && !CServiceBroker::GetAddonMgr().IsAddonDisabled(id))
  {
    VFSEntryPtr vfs = std::make_shared<CVFSEntry>(addonInfo);

    if (!vfs->GetZeroconfType().empty())
      CZeroconfBrowser::GetInstance()->AddServiceType(vfs->GetZeroconfType());

    std::unique_lock<CCriticalSection> lock(m_critSection);
    m_addonsInstances.emplace_back(vfs);
  }
}

class CVFSURLWrapper
{
  public:
    explicit CVFSURLWrapper(const CURL& url2)
    {
      m_strings.push_back(url2.Get());
      m_strings.push_back(url2.GetDomain());
      m_strings.push_back(url2.GetHostName());
      m_strings.push_back(url2.GetFileName());
      m_strings.push_back(url2.GetOptions());
      m_strings.push_back(url2.GetUserName());
      m_strings.push_back(url2.GetPassWord());
      m_strings.push_back(url2.GetRedacted());
      m_strings.push_back(url2.GetShareName());
      m_strings.push_back(url2.GetProtocol());

      url.url = m_strings[0].c_str();
      url.domain = m_strings[1].c_str();
      url.hostname = m_strings[2].c_str();
      url.filename = m_strings[3].c_str();
      url.port = url2.GetPort();
      url.options = m_strings[4].c_str();
      url.username = m_strings[5].c_str();
      url.password = m_strings[6].c_str();
      url.redacted = m_strings[7].c_str();
      url.sharename = m_strings[8].c_str();
      url.protocol = m_strings[9].c_str();
    }

    VFSURL url;
  protected:
    std::vector<std::string> m_strings;
};

CVFSEntry::ProtocolInfo::ProtocolInfo(const AddonInfoPtr& addonInfo)
  : supportPath(addonInfo->Type(AddonType::VFS)->GetValue("@supportPath").asBoolean()),
    supportUsername(addonInfo->Type(AddonType::VFS)->GetValue("@supportUsername").asBoolean()),
    supportPassword(addonInfo->Type(AddonType::VFS)->GetValue("@supportPassword").asBoolean()),
    supportPort(addonInfo->Type(AddonType::VFS)->GetValue("@supportPort").asBoolean()),
    supportBrowsing(addonInfo->Type(AddonType::VFS)->GetValue("@supportBrowsing").asBoolean()),
    supportWrite(addonInfo->Type(AddonType::VFS)->GetValue("@supportWrite").asBoolean()),
    defaultPort(addonInfo->Type(AddonType::VFS)->GetValue("@defaultPort").asInteger()),
    type(addonInfo->Type(AddonType::VFS)->GetValue("@protocols").asString()),
    label(addonInfo->Type(AddonType::VFS)->GetValue("@label").asInteger())
{
}

CVFSEntry::CVFSEntry(const AddonInfoPtr& addonInfo)
  : IAddonInstanceHandler(ADDON_INSTANCE_VFS, addonInfo),
    m_protocols(addonInfo->Type(AddonType::VFS)->GetValue("@protocols").asString()),
    m_extensions(addonInfo->Type(AddonType::VFS)->GetValue("@extensions").asString()),
    m_zeroconf(addonInfo->Type(AddonType::VFS)->GetValue("@zeroconf").asString()),
    m_files(addonInfo->Type(AddonType::VFS)->GetValue("@files").asBoolean()),
    m_directories(addonInfo->Type(AddonType::VFS)->GetValue("@directories").asBoolean()),
    m_filedirectories(addonInfo->Type(AddonType::VFS)->GetValue("@filedirectories").asBoolean()),
    m_protocolInfo(addonInfo)
{
  if (!addonInfo->Type(AddonType::VFS)->GetValue("@supportDialog").asBoolean())
    m_protocolInfo.type.clear();

  // Create "C" interface structures, used as own parts to prevent API problems on update
  m_ifc.vfs = new AddonInstance_VFSEntry;
  m_ifc.vfs->props = new AddonProps_VFSEntry();
  m_ifc.vfs->toAddon = new KodiToAddonFuncTable_VFSEntry();
  m_ifc.vfs->toKodi = new AddonToKodiFuncTable_VFSEntry();

  m_ifc.vfs->toKodi->kodiInstance = this;
  if (CreateInstance() != ADDON_STATUS_OK)
    CLog::Log(LOGFATAL, "CVFSEntry - Couldn't create instance on add-on '{}'", addonInfo->Name());
}

CVFSEntry::~CVFSEntry()
{
  DestroyInstance();

  // Delete "C" interface structures
  delete m_ifc.vfs->toAddon;
  delete m_ifc.vfs->toKodi;
  delete m_ifc.vfs->props;
  delete m_ifc.vfs;
}

void* CVFSEntry::Open(const CURL& url)
{
  if (!m_ifc.vfs->toAddon->open)
    return nullptr;

  CVFSURLWrapper url2(url);
  return m_ifc.vfs->toAddon->open(m_ifc.vfs, &url2.url);
}

void* CVFSEntry::OpenForWrite(const CURL& url, bool bOverWrite)
{
  if (!m_ifc.vfs->toAddon->open_for_write)
    return nullptr;

  CVFSURLWrapper url2(url);
  return m_ifc.vfs->toAddon->open_for_write(m_ifc.vfs, &url2.url, bOverWrite);
}

bool CVFSEntry::Exists(const CURL& url)
{
  if (!m_ifc.vfs->toAddon->exists)
    return false;

  CVFSURLWrapper url2(url);
  return m_ifc.vfs->toAddon->exists(m_ifc.vfs, &url2.url);
}

int CVFSEntry::Stat(const CURL& url, struct __stat64* buffer)
{
  int ret = -1;
  if (!m_ifc.vfs->toAddon->stat)
    return ret;

  CVFSURLWrapper url2(url);
  STAT_STRUCTURE statBuffer = {};
  ret = m_ifc.vfs->toAddon->stat(m_ifc.vfs, &url2.url, &statBuffer);

  *buffer = {};
  buffer->st_dev = statBuffer.deviceId;
  buffer->st_ino = statBuffer.fileSerialNumber;
  buffer->st_size = statBuffer.size;
  buffer->st_atime = statBuffer.accessTime;
  buffer->st_mtime = statBuffer.modificationTime;
  buffer->st_ctime = statBuffer.statusTime;
  buffer->st_mode = 0;
  if (statBuffer.isDirectory)
    buffer->st_mode |= S_IFDIR;
  if (statBuffer.isSymLink)
    buffer->st_mode |= S_IFLNK;
  if (statBuffer.isBlock)
    buffer->st_mode |= S_IFBLK;
  if (statBuffer.isCharacter)
    buffer->st_mode |= S_IFCHR;
  if (statBuffer.isFifo)
    buffer->st_mode |= S_IFIFO;
  if (statBuffer.isRegular)
    buffer->st_mode |= S_IFREG;
  if (statBuffer.isSocket)
    buffer->st_mode |= S_IFSOCK;

  return ret;
}

ssize_t CVFSEntry::Read(void* ctx, void* lpBuf, size_t uiBufSize)
{
  if (!m_ifc.vfs->toAddon->read)
    return 0;

  return m_ifc.vfs->toAddon->read(m_ifc.vfs, ctx, static_cast<uint8_t*>(lpBuf), uiBufSize);
}

ssize_t CVFSEntry::Write(void* ctx, const void* lpBuf, size_t uiBufSize)
{
  if (!m_ifc.vfs->toAddon->write)
    return 0;

  return m_ifc.vfs->toAddon->write(m_ifc.vfs, ctx, static_cast<const uint8_t*>(lpBuf), uiBufSize);
}

int64_t CVFSEntry::Seek(void* ctx, int64_t position, int whence)
{
  if (!m_ifc.vfs->toAddon->seek)
    return 0;

  return m_ifc.vfs->toAddon->seek(m_ifc.vfs, ctx, position, whence);
}

int CVFSEntry::Truncate(void* ctx, int64_t size)
{
  if (!m_ifc.vfs->toAddon->truncate)
    return 0;

  return m_ifc.vfs->toAddon->truncate(m_ifc.vfs, ctx, size);
}

void CVFSEntry::Close(void* ctx)
{
  if (m_ifc.vfs->toAddon->close)
    m_ifc.vfs->toAddon->close(m_ifc.vfs, ctx);
}

int64_t CVFSEntry::GetPosition(void* ctx)
{
  if (!m_ifc.vfs->toAddon->get_position)
    return 0;

  return m_ifc.vfs->toAddon->get_position(m_ifc.vfs, ctx);
}

int CVFSEntry::GetChunkSize(void* ctx)
{
  if (!m_ifc.vfs->toAddon->get_chunk_size)
    return 0;

  return m_ifc.vfs->toAddon->get_chunk_size(m_ifc.vfs, ctx);
}

int64_t CVFSEntry::GetLength(void* ctx)
{
  if (!m_ifc.vfs->toAddon->get_length)
    return 0;

  return m_ifc.vfs->toAddon->get_length(m_ifc.vfs, ctx);
}

int CVFSEntry::IoControl(void* ctx, XFILE::EIoControl request, void* param)
{
  switch (request)
  {
    case XFILE::EIoControl::IOCTRL_SEEK_POSSIBLE:
    {
      if (!m_ifc.vfs->toAddon->io_control_get_seek_possible)
        return -1;
      return m_ifc.vfs->toAddon->io_control_get_seek_possible(m_ifc.vfs, ctx) ? 1 : 0;
    }
    case XFILE::EIoControl::IOCTRL_CACHE_STATUS:
    {
      if (!m_ifc.vfs->toAddon->io_control_get_cache_status)
        return -1;

      XFILE::SCacheStatus* kodiData = static_cast<XFILE::SCacheStatus*>(param);
      if (!kodiData)
        return -1;

      VFS_CACHE_STATUS_DATA status;
      int ret = m_ifc.vfs->toAddon->io_control_get_cache_status(m_ifc.vfs, ctx, &status) ? 0 : -1;
      if (ret >= 0)
      {
        kodiData->forward = status.forward;
        kodiData->maxrate = status.maxrate;
        kodiData->currate = status.currate;
        kodiData->lowrate = status.lowrate;
      }
      return ret;
    }
    case XFILE::EIoControl::IOCTRL_CACHE_SETRATE:
    {
      if (!m_ifc.vfs->toAddon->io_control_set_cache_rate)
        return -1;

      uint32_t& iParam = *static_cast<uint32_t*>(param);
      return m_ifc.vfs->toAddon->io_control_set_cache_rate(m_ifc.vfs, ctx, iParam) ? 1 : 0;
    }
    case XFILE::EIoControl::IOCTRL_SET_RETRY:
    {
      if (!m_ifc.vfs->toAddon->io_control_set_retry)
        return -1;

      bool& bParam = *static_cast<bool*>(param);
      return m_ifc.vfs->toAddon->io_control_set_retry(m_ifc.vfs, ctx, bParam) ? 0 : -1;
    }

    // Not by addon supported io's
    case XFILE::EIoControl::IOCTRL_SET_CACHE:
    case XFILE::EIoControl::IOCTRL_NATIVE:
    default:
      break;
  }

  return -1;
}

bool CVFSEntry::Delete(const CURL& url)
{
  if (!m_ifc.vfs->toAddon->delete_it)
    return false;

  CVFSURLWrapper url2(url);
  return m_ifc.vfs->toAddon->delete_it(m_ifc.vfs, &url2.url);
}

bool CVFSEntry::Rename(const CURL& url, const CURL& url2)
{
  if (!m_ifc.vfs->toAddon->rename)
    return false;

  CVFSURLWrapper url3(url);
  CVFSURLWrapper url4(url2);
  return m_ifc.vfs->toAddon->rename(m_ifc.vfs, &url3.url, &url4.url);
}

void CVFSEntry::ClearOutIdle()
{
  if (m_ifc.vfs->toAddon->clear_out_idle)
    m_ifc.vfs->toAddon->clear_out_idle(m_ifc.vfs);
}

void CVFSEntry::DisconnectAll()
{
  if (m_ifc.vfs->toAddon->disconnect_all)
    m_ifc.vfs->toAddon->disconnect_all(m_ifc.vfs);
}

bool CVFSEntry::DirectoryExists(const CURL& url)
{
  if (!m_ifc.vfs->toAddon->directory_exists)
    return false;

  CVFSURLWrapper url2(url);
  return m_ifc.vfs->toAddon->directory_exists(m_ifc.vfs, &url2.url);
}

bool CVFSEntry::RemoveDirectory(const CURL& url)
{
  if (!m_ifc.vfs->toAddon->remove_directory)
    return false;

  CVFSURLWrapper url2(url);
  return m_ifc.vfs->toAddon->remove_directory(m_ifc.vfs, &url2.url);
}

bool CVFSEntry::CreateDirectory(const CURL& url)
{
  if (!m_ifc.vfs->toAddon->create_directory)
    return false;

  CVFSURLWrapper url2(url);
  return m_ifc.vfs->toAddon->create_directory(m_ifc.vfs, &url2.url);
}

static void VFSDirEntriesToCFileItemList(int num_entries,
                                         VFSDirEntry* entries,
                                         CFileItemList& items)
{
  for (int i=0;i<num_entries;++i)
  {
    CFileItemPtr item(new CFileItem());
    item->SetLabel(entries[i].label);
    item->SetPath(entries[i].path);
    item->m_dwSize = entries[i].size;
    item->m_dateTime = entries[i].date_time;
    item->m_bIsFolder = entries[i].folder;
    if (entries[i].title)
      item->m_strTitle = entries[i].title;
    for (unsigned int j=0;j<entries[i].num_props;++j)
    {
      if (StringUtils::CompareNoCase(entries[i].properties[j].name, "propmisusepreformatted") == 0)
      {
        if (StringUtils::CompareNoCase(entries[i].properties[j].name, "true") == 0)
          item->SetLabelPreformatted(true);
        else
          item->SetLabelPreformatted(false);
      } else
        item->SetProperty(entries[i].properties[j].name,
                          entries[i].properties[j].val);
    }
    items.Add(item);
  }
}

bool CVFSEntry::GetDirectory(const CURL& url, CFileItemList& items,
                             void* ctx)
{
  if (!m_ifc.vfs->toAddon->get_directory || !m_ifc.vfs->toAddon->free_directory)
    return false;

  VFSGetDirectoryCallbacks callbacks;
  callbacks.ctx = ctx;
  callbacks.get_keyboard_input = CVFSEntryIDirectoryWrapper::DoGetKeyboardInput;
  callbacks.set_error_dialog = CVFSEntryIDirectoryWrapper::DoSetErrorDialog;
  callbacks.require_authentication = CVFSEntryIDirectoryWrapper::DoRequireAuthentication;

  VFSDirEntry* entries = nullptr;
  int num_entries = 0;
  CVFSURLWrapper url2(url);
  bool ret =
      m_ifc.vfs->toAddon->get_directory(m_ifc.vfs, &url2.url, &entries, &num_entries, &callbacks);
  if (ret)
  {
    VFSDirEntriesToCFileItemList(num_entries, entries, items);
    m_ifc.vfs->toAddon->free_directory(m_ifc.vfs, entries, num_entries);
  }

  return ret;
}

bool CVFSEntry::ContainsFiles(const CURL& url, CFileItemList& items)
{
  if (!m_ifc.vfs->toAddon->contains_files || !m_ifc.vfs->toAddon->free_directory)
    return false;

  VFSDirEntry* entries = nullptr;
  int num_entries = 0;

  CVFSURLWrapper url2(url);
  char rootpath[ADDON_STANDARD_STRING_LENGTH];
  rootpath[0] = 0;
  bool ret =
      m_ifc.vfs->toAddon->contains_files(m_ifc.vfs, &url2.url, &entries, &num_entries, rootpath);
  if (!ret)
    return false;

  VFSDirEntriesToCFileItemList(num_entries, entries, items);
  m_ifc.vfs->toAddon->free_directory(m_ifc.vfs, entries, num_entries);
  if (strlen(rootpath))
    items.SetPath(rootpath);

  return true;
}

CVFSEntryIFileWrapper::CVFSEntryIFileWrapper(VFSEntryPtr ptr) : m_addon(std::move(ptr))
{
}

CVFSEntryIFileWrapper::~CVFSEntryIFileWrapper()
{
  Close();
}

bool CVFSEntryIFileWrapper::Open(const CURL& url)
{
  m_context = m_addon->Open(url);
  return m_context != nullptr;
}

bool CVFSEntryIFileWrapper::OpenForWrite(const CURL& url, bool bOverWrite)
{
  m_context = m_addon->OpenForWrite(url, bOverWrite);
  return m_context != nullptr;
}

bool CVFSEntryIFileWrapper::Exists(const CURL& url)
{
  return m_addon->Exists(url);
}

int CVFSEntryIFileWrapper::Stat(const CURL& url, struct __stat64* buffer)
{
  return m_addon->Stat(url, buffer);
}

int CVFSEntryIFileWrapper::Truncate(int64_t size)
{
  return m_addon->Truncate(m_context, size);
}

ssize_t CVFSEntryIFileWrapper::Read(void* lpBuf, size_t uiBufSize)
{
  if (!m_context)
    return 0;

  return m_addon->Read(m_context, lpBuf, uiBufSize);
}

ssize_t CVFSEntryIFileWrapper::Write(const void* lpBuf, size_t uiBufSize)
{
  if (!m_context)
    return 0;

  return m_addon->Write(m_context, lpBuf, uiBufSize);
}

int64_t CVFSEntryIFileWrapper::Seek(int64_t iFilePosition, int whence)
{
  if (!m_context)
    return 0;

  return m_addon->Seek(m_context, iFilePosition, whence);
}

void CVFSEntryIFileWrapper::Close()
{
  if (m_context)
  {
    m_addon->Close(m_context);
    m_context = nullptr;
  }
}

int64_t CVFSEntryIFileWrapper::GetPosition()
{
  if (!m_context)
    return 0;

  return m_addon->GetPosition(m_context);
}

int CVFSEntryIFileWrapper::GetChunkSize()
{
  if (!m_context)
    return 0;

  return m_addon->GetChunkSize(m_context);
}

int64_t CVFSEntryIFileWrapper::GetLength()
{
  if (!m_context)
    return 0;

  return m_addon->GetLength(m_context);
}

int CVFSEntryIFileWrapper::IoControl(XFILE::EIoControl request, void* param)
{
  if (!m_context)
    return 0;

  return m_addon->IoControl(m_context, request, param);
}

bool CVFSEntryIFileWrapper::Delete(const CURL& url)
{
  return m_addon->Delete(url);
}

bool CVFSEntryIFileWrapper::Rename(const CURL& url, const CURL& url2)
{
  return m_addon->Rename(url, url2);
}

CVFSEntryIDirectoryWrapper::CVFSEntryIDirectoryWrapper(VFSEntryPtr ptr) : m_addon(std::move(ptr))
{
}

bool CVFSEntryIDirectoryWrapper::Exists(const CURL& url)
{
  return m_addon->DirectoryExists(url);
}

bool CVFSEntryIDirectoryWrapper::Remove(const CURL& url)
{
  return m_addon->RemoveDirectory(url);
}

bool CVFSEntryIDirectoryWrapper::Create(const CURL& url)
{
  return m_addon->CreateDirectory(url);
}

bool CVFSEntryIDirectoryWrapper::GetDirectory(const CURL& url,
                                              CFileItemList& items)
{
  return m_addon->GetDirectory(url, items, this);
}

bool CVFSEntryIDirectoryWrapper::DoGetKeyboardInput(void* ctx,
                                                    const char* heading,
                                                    char** input,
                                                    bool hidden_input)
{
  return static_cast<CVFSEntryIDirectoryWrapper*>(ctx)->GetKeyboardInput2(heading, input, hidden_input);
}

bool CVFSEntryIDirectoryWrapper::GetKeyboardInput2(const char* heading,
                                                   char** input,
                                                   bool hidden_input)
{
  std::string inp;
  bool result;
  if ((result=GetKeyboardInput(CVariant(std::string(heading)), inp, hidden_input)))
    *input = strdup(inp.c_str());

  return result;
}

void CVFSEntryIDirectoryWrapper::DoSetErrorDialog(void* ctx, const char* heading,
                                                  const char* line1,
                                                  const char* line2,
                                                  const char* line3)
{
  static_cast<CVFSEntryIDirectoryWrapper*>(ctx)->SetErrorDialog2(heading, line1,
                                                                 line2, line3);
}

void CVFSEntryIDirectoryWrapper::SetErrorDialog2(const char* heading,
                                                 const char* line1,
                                                 const char* line2,
                                                 const char* line3)
{
  CVariant l2=0, l3=0;
  if (line2)
    l2 = std::string(line2);
  if (line3)
    l3 = std::string(line3);
  if (m_flags & XFILE::DIR_FLAG_ALLOW_PROMPT)
    SetErrorDialog(CVariant(std::string(heading)),
                   CVariant(std::string(line1)), l2, l3);
}

void CVFSEntryIDirectoryWrapper::DoRequireAuthentication(void* ctx,
                                                         const char* url)
{
  static_cast<CVFSEntryIDirectoryWrapper*>(ctx)->RequireAuthentication2(CURL(url));
}

void CVFSEntryIDirectoryWrapper::RequireAuthentication2(const CURL& url)
{
  if (m_flags & XFILE::DIR_FLAG_ALLOW_PROMPT)
    RequireAuthentication(url);
}

} /*namespace ADDON*/

