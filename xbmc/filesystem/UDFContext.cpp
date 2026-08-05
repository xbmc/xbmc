/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "UDFContext.h"

#include "URL.h"
#include "threads/CriticalSection.h"
#include "utils/log.h"

#include <map>
#include <mutex>

#include <udfread/udfread.h>

using namespace XFILE;

namespace
{
CCriticalSection g_mountLock;

//! Volumes currently mounted, so that concurrent readers of one image share a single mount
std::map<std::string, std::weak_ptr<CUDFContext>, std::less<>> g_mounts;
} // namespace

CUDFContext::~CUDFContext()
{
  if (m_udf)
    udfread_close(m_udf); // Closes the block input too
}

bool CUDFContext::Mount(const std::string& image)
{
  m_udf = udfread_init();
  if (!m_udf)
    return false;

  udfread_block_input* blockInput{m_blockInput.GetBlockInput(image)};
  if (!blockInput)
  {
    udfread_close(m_udf);
    m_udf = nullptr;
    return false;
  }

  if (udfread_open_input(m_udf, blockInput) < 0)
  {
    blockInput->close(blockInput);
    udfread_close(m_udf);
    m_udf = nullptr;
    return false;
  }

  return true;
}

std::shared_ptr<CUDFContext> CUDFContext::Get(const std::string& image)
{
  std::unique_lock lock(g_mountLock);

  if (const auto it{g_mounts.find(image)}; it != g_mounts.end())
  {
    if (std::shared_ptr<CUDFContext> context{it->second.lock()}; context)
      return context;

    g_mounts.erase(it);
  }

  // Cannot use make_shared as the constructor is private
  std::shared_ptr<CUDFContext> context{new CUDFContext()};
  if (!context->Mount(image))
    return nullptr;

  // One line per mount, so a run of reads that is sharing one mount is distinguishable from one
  // that is re-mounting for every file
  CLog::LogF(LOGDEBUG, "Mounted UDF volume of {}", CURL::GetRedacted(image));

  // Forget any volumes that have since been unmounted
  std::erase_if(g_mounts, [](const auto& mount) { return mount.second.expired(); });

  g_mounts[image] = context;

  return context;
}

CUDFMount::CUDFMount(const std::string& path)
{
  if (const CURL url{path}; url.IsProtocol("udf"))
    m_context = CUDFContext::Get(url.GetHostName());
}
