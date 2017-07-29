/*
 *      Copyright (C) 2013 Arne Morten Kvarving
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

#include "VFSEntry.h"
#include "URL.h"
#include "addons/binary-addons/BinaryAddonBase.h"
#include "addons/binary-addons/BinaryAddonManager.h"
#include "ServiceBroker.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

namespace ADDON
{

CVFSAddonCache::~CVFSAddonCache()
{
  Deinit();
}

void CVFSAddonCache::Init()
{
  CAddonMgr::GetInstance().Events().Subscribe(this, &CVFSAddonCache::OnEvent);
  Update();
}

void CVFSAddonCache::Deinit()
{
  CAddonMgr::GetInstance().Events().Unsubscribe(this);
}

const std::vector<VFSEntryPtr> CVFSAddonCache::GetAddonInstances()
{
  CSingleLock lock(m_critSection);
  return m_addonsInstances;
}

VFSEntryPtr CVFSAddonCache::GetAddonInstance(const std::string& strId, TYPE type)
{
  VFSEntryPtr addon;

  CSingleLock lock(m_critSection);

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
  if (typeid(event) == typeid(AddonEvents::InstalledChanged))
    Update();
}

void CVFSAddonCache::Update()
{
  std::vector<VFSEntryPtr> addonmap;

  BinaryAddonBaseList addonInfos;
  CServiceBroker::GetBinaryAddonManager().GetAddonInfos(addonInfos, true, ADDON_VFS);
  for (const auto& addonInfo : addonInfos)
  {
    VFSEntryPtr vfs = std::make_shared<CVFSEntry>(addonInfo);
    addonmap.push_back(vfs);
  }

  {
    CSingleLock lock(m_critSection);
    m_addonsInstances = std::move(addonmap);
  }
}

class CVFSURLWrapper
{
  public:
    CVFSURLWrapper(const CURL& url2)
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
    }

    VFSURL url;
  protected:
    std::vector<std::string> m_strings;
};

CVFSEntry::CVFSEntry(BinaryAddonBasePtr addonInfo)
  : IAddonInstanceHandler(ADDON_INSTANCE_VFS, addonInfo),
    m_protocols(addonInfo->Type(ADDON_VFS)->GetValue("@protocols").asString()),
    m_extensions(addonInfo->Type(ADDON_VFS)->GetValue("@extensions").asString()),
    m_files(addonInfo->Type(ADDON_VFS)->GetValue("@files").asBoolean()),
    m_directories(addonInfo->Type(ADDON_VFS)->GetValue("@directories").asBoolean()),
    m_filedirectories(addonInfo->Type(ADDON_VFS)->GetValue("@filedirectories").asBoolean())

{
  m_struct = {{ 0 }};
  m_struct.toKodi.kodiInstance = this;
  if (CreateInstance(&m_struct) != ADDON_STATUS_OK)
    CLog::Log(LOGFATAL, "CVFSEntry - Couldn't create instance on add-on '%s'", addonInfo->Name().c_str());
}

CVFSEntry::~CVFSEntry()
{
  DestroyInstance();
}

void* CVFSEntry::Open(const CURL& url)
{
  if (!m_struct.toAddon.open)
    return nullptr;

  CVFSURLWrapper url2(url);
  return m_struct.toAddon.open(&m_struct, &url2.url);
}

void* CVFSEntry::OpenForWrite(const CURL& url, bool bOverWrite)
{
  if (!m_struct.toAddon.open_for_write)
    return nullptr;

  CVFSURLWrapper url2(url);
  return m_struct.toAddon.open_for_write(&m_struct, &url2.url, bOverWrite);
}

bool CVFSEntry::Exists(const CURL& url)
{
  if (!m_struct.toAddon.exists)
    return false;

  CVFSURLWrapper url2(url);
  return m_struct.toAddon.exists(&m_struct, &url2.url);
}

int CVFSEntry::Stat(const CURL& url, struct __stat64* buffer)
{
  if (!m_struct.toAddon.stat)
    return -1;

  CVFSURLWrapper url2(url);
  return m_struct.toAddon.stat(&m_struct, &url2.url, buffer);
}

ssize_t CVFSEntry::Read(void* ctx, void* lpBuf, size_t uiBufSize)
{
  if (!m_struct.toAddon.read)
    return 0;

  return m_struct.toAddon.read(&m_struct, ctx, lpBuf, uiBufSize);
}

ssize_t CVFSEntry::Write(void* ctx, const void* lpBuf, size_t uiBufSize)
{
  if (!m_struct.toAddon.write)
    return 0;

  return m_struct.toAddon.write(&m_struct, ctx, lpBuf, uiBufSize);
}

int64_t CVFSEntry::Seek(void* ctx, int64_t position, int whence)
{
  if (!m_struct.toAddon.seek)
    return 0;

  return m_struct.toAddon.seek(&m_struct, ctx, position, whence);
}

int CVFSEntry::Truncate(void* ctx, int64_t size)
{
  if (!m_struct.toAddon.truncate)
    return 0;

  return m_struct.toAddon.truncate(&m_struct, ctx, size);
}

void CVFSEntry::Close(void* ctx)
{
  if (m_struct.toAddon.close)
    m_struct.toAddon.close(&m_struct, ctx);
}

int64_t CVFSEntry::GetPosition(void* ctx)
{
  if (!m_struct.toAddon.get_position)
    return 0;

  return m_struct.toAddon.get_position(&m_struct, ctx);
}

int CVFSEntry::GetChunkSize(void* ctx)
{
  if (!m_struct.toAddon.get_chunk_size)
    return 0;

  return m_struct.toAddon.get_chunk_size(&m_struct, ctx);
}

int64_t CVFSEntry::GetLength(void* ctx)
{
  if (!m_struct.toAddon.get_length)
    return 0;

  return m_struct.toAddon.get_length(&m_struct, ctx);
}

int CVFSEntry::IoControl(void* ctx, XFILE::EIoControl request, void* param)
{
  if (!m_struct.toAddon.io_control)
    return -1;

  return m_struct.toAddon.io_control(&m_struct, ctx, request, param);
}

bool CVFSEntry::Delete(const CURL& url)
{
  if (!m_struct.toAddon.delete_it)
    return false;

  CVFSURLWrapper url2(url);
  return m_struct.toAddon.delete_it(&m_struct, &url2.url);
}

bool CVFSEntry::Rename(const CURL& url, const CURL& url2)
{
  if (!m_struct.toAddon.rename)
    return false;

  CVFSURLWrapper url3(url);
  CVFSURLWrapper url4(url2);
  return m_struct.toAddon.rename(&m_struct, &url3.url, &url4.url);
}

void CVFSEntry::ClearOutIdle()
{
  if (m_struct.toAddon.clear_out_idle)
    m_struct.toAddon.clear_out_idle(&m_struct);
}

void CVFSEntry::DisconnectAll()
{
  if (m_struct.toAddon.disconnect_all)
    m_struct.toAddon.disconnect_all(&m_struct);
}

bool CVFSEntry::DirectoryExists(const CURL& url)
{
  if (!m_struct.toAddon.directory_exists)
    return false;

  CVFSURLWrapper url2(url);
  return m_struct.toAddon.directory_exists(&m_struct, &url2.url);
}

bool CVFSEntry::RemoveDirectory(const CURL& url)
{
  if (!m_struct.toAddon.remove_directory)
    return false;

  CVFSURLWrapper url2(url);
  return m_struct.toAddon.remove_directory(&m_struct, &url2.url);
}

bool CVFSEntry::CreateDirectory(const CURL& url)
{
  if (!m_struct.toAddon.create_directory)
    return false;

  CVFSURLWrapper url2(url);
  return m_struct.toAddon.create_directory(&m_struct, &url2.url);
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
    //item->m_dateTime = entries[i].mtime;
    item->m_bIsFolder = entries[i].folder;
    if (entries[i].title)
      item->m_strTitle = entries[i].title;
    for (unsigned int j=0;j<entries[i].num_props;++j)
    {
      if (strcasecmp(entries[i].properties[j].name, "propmisusepreformatted") == 0)
      {
        if (strcasecmp(entries[i].properties[j].name, "true") == 0)
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
  if (!m_struct.toAddon.get_directory || !m_struct.toAddon.free_directory)
    return false;

  VFSGetDirectoryCallbacks callbacks;
  callbacks.ctx = ctx;
  callbacks.get_keyboard_input = CVFSEntryIDirectoryWrapper::DoGetKeyboardInput;
  callbacks.set_error_dialog = CVFSEntryIDirectoryWrapper::DoSetErrorDialog;
  callbacks.require_authentication = CVFSEntryIDirectoryWrapper::DoRequireAuthentication;

  VFSDirEntry* entries = nullptr;
  int num_entries = 0;
  CVFSURLWrapper url2(url);
  bool ret = m_struct.toAddon.get_directory(&m_struct, &url2.url, &entries, &num_entries, &callbacks);
  if (ret)
  {
    VFSDirEntriesToCFileItemList(num_entries, entries, items);
    m_struct.toAddon.free_directory(&m_struct, entries, num_entries);
  }

  return ret;
}

bool CVFSEntry::ContainsFiles(const CURL& url, CFileItemList& items)
{
  if (!m_struct.toAddon.contains_files || !m_struct.toAddon.free_directory)
    return false;

  VFSDirEntry* entries = nullptr;
  int num_entries = 0;

  CVFSURLWrapper url2(url);
  char rootpath[ADDON_STANDARD_STRING_LENGTH];
  rootpath[0] = 0;
  bool ret = m_struct.toAddon.contains_files(&m_struct, &url2.url, &entries, &num_entries, rootpath);
  if (!ret)
    return false;

  VFSDirEntriesToCFileItemList(num_entries, entries, items);
  m_struct.toAddon.free_directory(&m_struct, entries, num_entries);
  if (strlen(rootpath))
    items.SetPath(rootpath);

  return true;
}

CVFSEntryIFileWrapper::CVFSEntryIFileWrapper(VFSEntryPtr ptr) :
  m_context(nullptr), m_addon(ptr)
{
}

CVFSEntryIFileWrapper::~CVFSEntryIFileWrapper()
{
  Close();
}

bool CVFSEntryIFileWrapper::Open(const CURL& url)
{
  m_context = m_addon->Open(url);
  return m_context != NULL;
}

bool CVFSEntryIFileWrapper::OpenForWrite(const CURL& url, bool bOverWrite)
{
  m_context = m_addon->OpenForWrite(url, bOverWrite);
  return m_context != NULL;
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
    m_context = NULL;
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

CVFSEntryIDirectoryWrapper::CVFSEntryIDirectoryWrapper(VFSEntryPtr ptr) :
  m_addon(ptr)
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

