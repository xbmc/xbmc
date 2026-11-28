/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DeviceFolder.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "URL.h"
#include "XBDateTime.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cstdlib>
#include <memory>

#include <emscripten/emscripten.h>
#include <emscripten/proxying.h>
#include <emscripten/threading.h>
#include <sys/stat.h>

using namespace XFILE;

namespace
{
// Keep in sync with device_folder.js.
enum Op
{
  OP_AVAILABLE = 0,
  OP_LIST = 1,
  OP_STAT = 2,
  OP_OPEN = 3,
  OP_PICK = 4,
  OP_CLOSE = 5,
  OP_WATCH = 6,
};

struct Call
{
  int op{OP_AVAILABLE};
  std::string path;
  int status{-1};
  std::string text;
  int64_t size{0};
  int64_t mtime{0};
  bool isDir{false};
};

constexpr size_t MAX_READ_BYTES = 4 * 1024 * 1024;

std::atomic<bool> g_storagesChanged{false};
} // namespace

extern "C"
{
  void kodi_device_folder_start(em_proxying_ctx* ctx, void* call, int op, const char* path);
  int kodi_device_folder_read(const char* url, double offset, void* buffer, int length);

  EMSCRIPTEN_KEEPALIVE void kodi_device_folder_finish(em_proxying_ctx* ctx,
                                                      void* callPtr,
                                                      int status,
                                                      char* text,
                                                      double size,
                                                      double mtime,
                                                      int isDir)
  {
    auto* call = static_cast<Call*>(callPtr);
    call->status = status;
    if (text)
    {
      call->text = text;
      free(text);
    }
    call->size = static_cast<int64_t>(size);
    call->mtime = static_cast<int64_t>(mtime);
    call->isDir = isDir != 0;
    emscripten_proxy_finish(ctx);
  }

  EMSCRIPTEN_KEEPALIVE void kodi_device_folder_changed()
  {
    g_storagesChanged = true;
  }
}

namespace
{
void StartOnBrowserThread(em_proxying_ctx* ctx, void* arg)
{
  auto* call = static_cast<Call*>(arg);
  kodi_device_folder_start(ctx, call, call->op, call->path.c_str());
}

// Blocks the calling thread until the browser thread has finished the operation.
bool Run(Call& call)
{
  if (!emscripten_proxy_sync_with_ctx(emscripten_proxy_get_system_queue(),
                                      emscripten_main_runtime_thread_id(), StartOnBrowserThread,
                                      &call))
    return false;
  return call.status == 0;
}

std::string BridgePath(const CURL& url)
{
  return url.GetHostName() + "/" + url.GetFileName();
}

bool Stat(const CURL& url, Call& call)
{
  if (url.GetHostName().empty())
  {
    call.isDir = true;
    return true;
  }
  call.op = OP_STAT;
  call.path = BridgePath(url);
  return Run(call);
}

void ParseFolders(const std::string& text, std::vector<DeviceFolder>& folders)
{
  for (const std::string& line : StringUtils::Split(text, "\n"))
  {
    const std::vector<std::string> fields = StringUtils::Split(line, "\t");
    if (fields.size() == 5)
      folders.push_back({"device://" + fields[3] + "/", fields[4], fields[0] == "r"});
  }
}
} // namespace

bool CDeviceFolderDirectory::IsPickerAvailable()
{
  Call call;
  call.op = OP_AVAILABLE;
  return Run(call);
}

std::vector<DeviceFolder> CDeviceFolderDirectory::GetFolders()
{
  std::vector<DeviceFolder> folders;
  Call call;
  call.op = OP_LIST;
  if (Run(call))
    ParseFolders(call.text, folders);
  return folders;
}

bool CDeviceFolderDirectory::PickFolder(DeviceFolder& folder)
{
  Call call;
  call.op = OP_PICK;
  if (!Run(call))
    return false;

  std::vector<DeviceFolder> folders;
  ParseFolders(call.text, folders);
  if (folders.empty())
    return false;

  folder = folders.front();
  return true;
}

void CDeviceFolderDirectory::WatchStorages()
{
  Call call;
  call.op = OP_WATCH;
  Run(call);
}

bool CDeviceFolderDirectory::HaveStoragesChanged()
{
  return g_storagesChanged.exchange(false);
}

bool CDeviceFolderDirectory::GetDirectory(const CURL& url, CFileItemList& items)
{
  const bool isRoot = url.GetHostName().empty();

  Call call;
  call.op = OP_LIST;
  if (!isRoot)
    call.path = BridgePath(url);
  if (!Run(call))
  {
    CLog::LogF(LOGERROR, "{} failed ({})", url.GetRedacted(), call.status);
    return false;
  }

  std::string base = isRoot ? "device://" : url.Get();
  if (!isRoot)
    URIUtils::AddSlashAtEnd(base);

  for (const std::string& line : StringUtils::Split(call.text, "\n"))
  {
    const std::vector<std::string> fields = StringUtils::Split(line, "\t");
    if (fields.size() != 5)
      continue;

    const bool isDir = fields[0] != "f";
    auto item = std::make_shared<CFileItem>(fields[4]);
    item->SetPath(base + fields[3] + (isDir ? "/" : ""));
    item->SetFolder(isDir);
    if (fields[3].starts_with('.'))
      item->SetProperty("file:hidden", true);
    if (!isDir)
    {
      item->SetSize(std::strtoll(fields[1].c_str(), nullptr, 10));
      item->SetDateTime(
          CDateTime(static_cast<time_t>(std::strtoll(fields[2].c_str(), nullptr, 10))));
    }
    items.Add(item);
  }
  return true;
}

bool CDeviceFolderDirectory::Exists(const CURL& url)
{
  Call call;
  return Stat(url, call) && call.isDir;
}

CacheType CDeviceFolderDirectory::GetCacheType(const CURL& url) const
{
  return url.GetHostName().empty() ? CacheType::NEVER : CacheType::ONCE;
}

CDeviceFolderFile::~CDeviceFolderFile()
{
  Close();
}

bool CDeviceFolderFile::Open(const CURL& url)
{
  Close();

  Call call;
  call.op = OP_OPEN;
  call.path = BridgePath(url);
  if (!Run(call) || call.text.empty())
  {
    CLog::LogF(LOGERROR, "{} failed ({})", url.GetRedacted(), call.status);
    return false;
  }

  m_blobUrl = call.text;
  m_length = call.size;
  m_position = 0;
  return true;
}

bool CDeviceFolderFile::Exists(const CURL& url)
{
  Call call;
  return ::Stat(url, call) && !call.isDir;
}

int CDeviceFolderFile::Stat(const CURL& url, struct __stat64* buffer)
{
  Call call;
  if (!::Stat(url, call))
  {
    errno = ENOENT;
    return -1;
  }

  if (buffer)
  {
    *buffer = {};
    buffer->st_mode = call.isDir ? S_IFDIR : S_IFREG;
    buffer->st_size = call.size;
    buffer->st_mtime = call.mtime;
  }
  return 0;
}

ssize_t CDeviceFolderFile::Read(void* bufPtr, size_t bufSize)
{
  if (m_blobUrl.empty())
    return -1;
  if (m_position >= m_length)
    return 0;

  const size_t wanted =
      std::min({bufSize, MAX_READ_BYTES, static_cast<size_t>(m_length - m_position)});
  const int got = kodi_device_folder_read(m_blobUrl.c_str(), static_cast<double>(m_position),
                                          bufPtr, static_cast<int>(wanted));
  if (got < 0)
    return -1;

  m_position += got;
  return got;
}

int64_t CDeviceFolderFile::Seek(int64_t iFilePosition, int iWhence)
{
  int64_t target = iFilePosition;
  if (iWhence == SEEK_CUR)
    target += m_position;
  else if (iWhence == SEEK_END)
    target += m_length;
  else if (iWhence != SEEK_SET)
    return -1;

  if (target < 0 || target > m_length)
    return -1;

  m_position = target;
  return m_position;
}

void CDeviceFolderFile::Close()
{
  if (m_blobUrl.empty())
    return;

  Call call;
  call.op = OP_CLOSE;
  call.path = m_blobUrl;
  Run(call);
  m_blobUrl.clear();
  m_length = 0;
  m_position = 0;
}

int CDeviceFolderFile::IoControl(IOControl request, void* param)
{
  if (request == IOControl::SEEK_POSSIBLE)
    return 1;
  return -1;
}
