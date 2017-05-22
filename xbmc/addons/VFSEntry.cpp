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
#include "AddonManager.h"
#include "utils/StringUtils.h"
#include "URL.h"

namespace ADDON
{

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

std::unique_ptr<CVFSEntry> CVFSEntry::FromExtension(AddonProps props,
                                                    const cp_extension_t* ext)
{
  std::string protocols = CAddonMgr::GetInstance().GetExtValue(ext->configuration, "@protocols");
  std::string extensions = CAddonMgr::GetInstance().GetExtValue(ext->configuration, "@extensions");
  bool files = CAddonMgr::GetInstance().GetExtValue(ext->configuration, "@files") == "true";
  bool directories = CAddonMgr::GetInstance().GetExtValue(ext->configuration, "@directories") == "true";
  bool filedirectories = CAddonMgr::GetInstance().GetExtValue(ext->configuration, "@filedirectories") == "true";

  return std::unique_ptr<CVFSEntry>(new CVFSEntry(std::move(props),
                                                  protocols,
                                                  extensions,
                                                  files, directories,
                                                  filedirectories));
}


CVFSEntry::CVFSEntry(AddonProps props,
                     const std::string& protocols,
                     const std::string& extensions,
                     bool files, bool directories, bool filedirectories)
 : CAddonDll(std::move(props)),
   m_protocols(protocols),
   m_extensions(extensions),
   m_files(files),
   m_directories(directories),
   m_filedirectories(filedirectories)
{
  memset(&m_struct, 0, sizeof(m_struct));
  m_struct.toKodi.kodiInstance = this;
}

bool CVFSEntry::Create()
{
  return CAddonDll::Create(ADDON_INSTANCE_VFS, &m_struct, &m_struct.props) == ADDON_STATUS_OK;
}

void* CVFSEntry::Open(const CURL& url)
{
  if (!Initialized())
    return NULL;

  CVFSURLWrapper url2(url);
  return m_struct.toAddon.Open(&url2.url);
}

void* CVFSEntry::OpenForWrite(const CURL& url, bool bOverWrite)
{
  if (!Initialized())
    return NULL;

  CVFSURLWrapper url2(url);
  return m_struct.toAddon.OpenForWrite(&url2.url, bOverWrite);
}

bool CVFSEntry::Exists(const CURL& url)
{
  if (!Initialized())
    return false;

  CVFSURLWrapper url2(url);
  return m_struct.toAddon.Exists(&url2.url);
}

int CVFSEntry::Stat(const CURL& url, struct __stat64* buffer)
{
  if (!Initialized())
    return -1;

  CVFSURLWrapper url2(url);
  return m_struct.toAddon.Stat(&url2.url, buffer);
}

ssize_t CVFSEntry::Read(void* ctx, void* lpBuf, size_t uiBufSize)
{
  if (!Initialized())
    return 0;

  return m_struct.toAddon.Read(ctx, lpBuf, uiBufSize);
}

ssize_t CVFSEntry::Write(void* ctx, void* lpBuf, size_t uiBufSize)
{
  if (!Initialized())
    return 0;

  return m_struct.toAddon.Write(ctx, lpBuf, uiBufSize);
}

int64_t CVFSEntry::Seek(void* ctx, int64_t position, int whence)
{
  if (!Initialized())
    return 0;

  return m_struct.toAddon.Seek(ctx, position, whence);
}

int CVFSEntry::Truncate(void* ctx, int64_t size)
{
  if (!Initialized())
    return 0;

  return m_struct.toAddon.Truncate(ctx, size);
}

void CVFSEntry::Close(void* ctx)
{
  if (!Initialized())
    return;

  m_struct.toAddon.Close(ctx);
}

int64_t CVFSEntry::GetPosition(void* ctx)
{
  if (!Initialized())
    return 0;

  return m_struct.toAddon.GetPosition(ctx);
}

int CVFSEntry::GetChunkSize(void* ctx)
{
  if (!Initialized())
    return 0;

  return m_struct.toAddon.GetChunkSize(ctx);
}

int64_t CVFSEntry::GetLength(void* ctx)
{
  if (!Initialized())
    return 0;

  return m_struct.toAddon.GetLength(ctx);
}

int CVFSEntry::IoControl(void* ctx, XFILE::EIoControl request, void* param)
{
  if (!Initialized())
    return -1;

  return m_struct.toAddon.IoControl(ctx, request, param);
}

bool CVFSEntry::Delete(const CURL& url)
{
  if (!Initialized())
    return false;

  CVFSURLWrapper url2(url);
  return m_struct.toAddon.Delete(&url2.url);
}

bool CVFSEntry::Rename(const CURL& url, const CURL& url2)
{
  if (!Initialized())
    return false;

  CVFSURLWrapper url3(url);
  CVFSURLWrapper url4(url2);
  return m_struct.toAddon.Rename(&url3.url, &url4.url);
}

void CVFSEntry::ClearOutIdle()
{
  if (!Initialized())
    return;

  m_struct.toAddon.ClearOutIdle();
}

void CVFSEntry::DisconnectAll()
{
  if (!Initialized())
    return;

  m_struct.toAddon.DisconnectAll();
}

bool CVFSEntry::DirectoryExists(const CURL& url)
{
  if (!Initialized())
    return false;

  CVFSURLWrapper url2(url);
  return m_struct.toAddon.DirectoryExists(&url2.url);
}

bool CVFSEntry::RemoveDirectory(const CURL& url)
{
  if (!Initialized())
    return false;

  CVFSURLWrapper url2(url);
  return m_struct.toAddon.RemoveDirectory(&url2.url);
}

bool CVFSEntry::CreateDirectory(const CURL& url)
{
  if (!Initialized())
    return false;

  CVFSURLWrapper url2(url);
  return m_struct.toAddon.CreateDirectory(&url2.url);
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
    for (int j=0;j<entries[i].num_props;++j)
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
  if (!Initialized() && !Create())
    return false;

  VFSCallbacks callbacks;
  callbacks.ctx = ctx;
  callbacks.GetKeyboardInput = CVFSEntryIDirectoryWrapper::DoGetKeyboardInput;
  callbacks.SetErrorDialog = CVFSEntryIDirectoryWrapper::DoSetErrorDialog;
  callbacks.RequireAuthentication = CVFSEntryIDirectoryWrapper::DoRequireAuthentication;

  VFSDirEntry* entries;
  int num_entries;
  CVFSURLWrapper url2(url);
  void* ctx2 = m_struct.toAddon.GetDirectory(&url2.url, &entries, &num_entries, &callbacks);
  if (ctx2)
  {
    VFSDirEntriesToCFileItemList(num_entries, entries, items);
    m_struct.toAddon.FreeDirectory(ctx2);

    return true;
  }

  return false;
}

bool CVFSEntry::ContainsFiles(const CURL& url, CFileItemList& items)
{
  if (!Initialized() && !Create())
    return false;

  VFSDirEntry* entries;
  int num_entries;

  CVFSURLWrapper url2(url);
  char rootpath[1024];
  rootpath[0] = 0;
  void* ctx = m_struct.toAddon.ContainsFiles(&url2.url, &entries, &num_entries, rootpath);
  if (!ctx)
    return false;

  VFSDirEntriesToCFileItemList(num_entries, entries, items);
  m_struct.toAddon.FreeDirectory(ctx);
  if (strlen(rootpath))
    items.SetPath(rootpath);

  return true;
}

CVFSEntryIFileWrapper::CVFSEntryIFileWrapper(VFSEntryPtr ptr) :
  m_context(NULL), m_addon(ptr)
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

ssize_t CVFSEntryIFileWrapper::Write(void* lpBuf, size_t uiBufSize)
{
  if (!m_context)
    return 0;

  return m_addon->Write(m_context, lpBuf, uiBufSize);
}

int64_t CVFSEntryIFileWrapper::Seek(int64_t iFilePosition, int iWhence)
{
  if (!m_context)
    return 0;

  return m_addon->Seek(m_context, iFilePosition, iWhence);
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
                                                    char** input)
{
  return ((CVFSEntryIDirectoryWrapper*)ctx)->GetKeyboardInput2(heading, input);
}

bool CVFSEntryIDirectoryWrapper::GetKeyboardInput2(const char* heading,
                                                   char** input)
{
  std::string inp;
  bool result;
  if ((result=GetKeyboardInput(CVariant(std::string(heading)), inp)))
    *input = strdup(inp.c_str());

  return result;
}

void CVFSEntryIDirectoryWrapper::DoSetErrorDialog(void* ctx, const char* heading,
                                                  const char* line1,
                                                  const char* line2,
                                                  const char* line3)
{
  ((CVFSEntryIDirectoryWrapper*)ctx)->SetErrorDialog2(heading, line1,
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
  ((CVFSEntryIDirectoryWrapper*)ctx)->RequireAuthentication2(CURL(url));
}

void CVFSEntryIDirectoryWrapper::RequireAuthentication2(const CURL& url)
{
  if (m_flags & XFILE::DIR_FLAG_ALLOW_PROMPT)
    RequireAuthentication(url);
}

} /*namespace ADDON*/

