/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WebFilterLists.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "XBDateTime.h"
#include "filesystem/CurlFile.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include "platform/linux/waylandhost/WaylandHost.h"

#include <chrono>
#include <limits>
#include <mutex>
#include <sstream>
#include <vector>

#include <poll.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <unistd.h>

using namespace KODI::WEB;

namespace
{

constexpr const char* LIST_DIR = "special://profile/webdata/filterlists/";
constexpr int REFRESH_MS = 6 * 60 * 60 * 1000;

int HttpStatus(const CHttpHeader& header)
{
  const std::vector<std::string> fields = StringUtils::Split(header.GetProtoLine(), ' ');
  return fields.size() > 1 ? std::atoi(fields[1].c_str()) : 0;
}

// the published generation of a list, as its ETag or else its Last-Modified date
bool SameGeneration(const CHttpHeader& header, const std::string& etag, const std::string& modified)
{
  const std::string publishedEtag = header.GetValue("etag");
  if (!publishedEtag.empty())
    return publishedEtag == etag;
  const std::string publishedModified = header.GetValue("last-modified");
  return !publishedModified.empty() && publishedModified == modified;
}

int64_t ListExpiresSeconds(const std::string& list)
{
  std::istringstream lines(list);
  std::string line;
  std::getline(lines, line);
  if (!StringUtils::StartsWith(line, "["))
    lines.seekg(0);
  while (std::getline(lines, line) && StringUtils::StartsWith(line, "!"))
  {
    std::string field = line.substr(1);
    StringUtils::Trim(field);
    if (!StringUtils::StartsWithNoCase(field, "Expires:"))
      continue;

    const std::vector<std::string> words = StringUtils::Split(field.substr(8), ' ', 3);
    std::vector<std::string> value;
    for (const auto& word : words)
      if (!word.empty())
        value.push_back(word);
    if (value.size() < 2)
      return 0;

    const int64_t count = std::atoll(value[0].c_str());
    if (StringUtils::StartsWithNoCase(value[1], "day"))
      return count * 24 * 60 * 60;
    if (StringUtils::StartsWithNoCase(value[1], "hour"))
      return count * 60 * 60;
    return 0;
  }
  return 0;
}

// both times come from the server, as the local clock may not be set yet
bool Expired(const CHttpHeader& published, const std::string& fetched, const std::string& expires)
{
  const int64_t seconds = std::atoll(expires.c_str());
  const CDateTime now = CDateTime::FromRFC1123DateTime(published.GetValue("date"));
  const CDateTime then = CDateTime::FromRFC1123DateTime(fetched);
  if (seconds <= 0 || !now.IsValid() || !then.IsValid())
    return true;

  return (now - then).GetSecondsTotal() >= seconds;
}

bool WriteFile(const std::string& path, const std::string& data)
{
  const std::string temp = path + ".new";
  XFILE::CFile file;
  if (!file.OpenForWrite(temp, true))
    return false;
  const bool written = file.Write(data.data(), data.size()) == static_cast<ssize_t>(data.size());
  file.Close();
  return written && XFILE::CFile::Rename(temp, path);
}

} // unnamed namespace

CWebFilterLists::CWebFilterLists(WAYLANDHOST::CWaylandHost& host)
  : CThread("WebFilterLists"),
    m_host(host)
{
}

CWebFilterLists::~CWebFilterLists()
{
  m_bStop = true;
  Wake();
  StopThread(true);
  if (m_wakeFd >= 0)
    close(m_wakeFd);
  if (m_clockFd >= 0)
    close(m_clockFd);
}

void CWebFilterLists::Start(const std::set<std::string>& used)
{
  const auto advanced = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();
  for (const auto& [name, url] : advanced->m_webFilterLists)
  {
    if (!WAYLANDHOST::CWaylandHost::IsValidWebProfile(name))
      CLog::Log(LOGWARNING, "CWebFilterLists::{}: \"{}\" cannot name a list", __FUNCTION__, name);
    else if (!url.empty())
      m_lists.emplace(name, url);
  }

  m_dir = CSpecialProtocol::TranslatePath(LIST_DIR);
  XFILE::CDirectory::Create(m_dir);
  RemoveUnconfigured();

  m_wakeFd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
  m_clockFd = timerfd_create(CLOCK_REALTIME, TFD_CLOEXEC | TFD_NONBLOCK);
  if (m_wakeFd < 0 || m_clockFd < 0)
  {
    CLog::Log(LOGERROR, "CWebFilterLists::{}: no event descriptors, lists are not fetched",
              __FUNCTION__);
    return;
  }

  std::unique_lock lock(m_lock);
  for (const auto& name : used)
    Track(name);
  if (m_wanted.empty())
    return;
  m_started = true;
  Create();
}

void CWebFilterLists::RemoveUnconfigured()
{
  CFileItemList items;
  XFILE::CDirectory::GetDirectory(m_dir, items, "", XFILE::DIR_FLAG_NO_FILE_DIRS);
  for (const auto& item : items)
  {
    const std::string name = URIUtils::ReplaceExtension(URIUtils::GetFileName(item->GetPath()), "");
    if (!m_lists.count(name))
      XFILE::CFile::Delete(item->GetPath());
  }
}

bool CWebFilterLists::Track(const std::string& name)
{
  if (!m_lists.count(name) || !m_wanted.insert(name).second)
    return false;
  m_pending.insert(name);
  return true;
}

void CWebFilterLists::Want(const std::string& name)
{
  std::unique_lock lock(m_lock);
  if (m_wakeFd < 0 || !Track(name))
    return;

  if (m_started)
  {
    Wake();
    return;
  }
  m_started = true;
  Create();
}

void CWebFilterLists::Wake()
{
  if (m_wakeFd >= 0)
    eventfd_write(m_wakeFd, 1);
}

void CWebFilterLists::ArmClockChange()
{
  struct itimerspec never = {};
  never.it_value.tv_sec = std::numeric_limits<time_t>::max();
  timerfd_settime(m_clockFd, TFD_TIMER_ABSTIME | TFD_TIMER_CANCEL_ON_SET, &never, nullptr);
}

CWebFilterLists::Wakeup CWebFilterLists::Wait(bool retry)
{
  const auto due = std::chrono::steady_clock::now() + std::chrono::milliseconds(REFRESH_MS);
  struct pollfd fds[2] = {{m_wakeFd, POLLIN, 0}, {m_clockFd, POLLIN, 0}};
  uint64_t count;

  while (!m_bStop)
  {
    const auto left = std::chrono::duration_cast<std::chrono::milliseconds>(
        due - std::chrono::steady_clock::now());
    if (left.count() <= 0)
      return Wakeup::DUE;

    ArmClockChange();
    const int ready = poll(fds, 2, static_cast<int>(left.count()));
    if (ready < 0 && errno != EINTR)
      return Wakeup::STOP;

    eventfd_t wakes = 0;
    eventfd_read(m_wakeFd, &wakes);
    if (wakes)
      return Wakeup::WANTED;
    if (read(m_clockFd, &count, sizeof(count)) < 0 && errno == ECANCELED && retry)
    {
      CLog::Log(LOGINFO, "CWebFilterLists::{}: the clock was set, retrying", __FUNCTION__);
      return Wakeup::CLOCK;
    }
  }
  return Wakeup::STOP;
}

void CWebFilterLists::Process()
{
  Wakeup wakeup = Wakeup::DUE;
  while (wakeup != Wakeup::STOP && !m_bStop)
    wakeup = Wait(!FetchAll(wakeup == Wakeup::DUE));
}

bool CWebFilterLists::FetchAll(bool all)
{
  std::unique_lock lock(m_lock);
  const std::set<std::string> names = all ? m_wanted : m_pending;
  lock.unlock();

  bool fetched = true;
  for (const auto& name : names)
  {
    if (m_bStop)
      break;

    const Result result = Fetch(name, m_lists.at(name));
    if (result == Result::FAILED)
      fetched = false;
    else
    {
      lock.lock();
      m_pending.erase(name);
      lock.unlock();
    }
    if (result == Result::UPDATED || !m_handed.count(name))
      Hand(name);
  }
  return fetched;
}

void CWebFilterLists::Hand(const std::string& name)
{
  const std::string path = m_dir + name + ".txt";
  if (!XFILE::CFile::Exists(path, false))
    return;
  m_host.WebSetContentFilterList(name, path);
  m_handed.insert(name);
}

CWebFilterLists::Result CWebFilterLists::Fetch(const std::string& name, const std::string& url)
{
  const std::string path = m_dir + name + ".txt";
  const std::string metaPath = m_dir + name + ".meta";
  XFILE::CCurlFile http;

  std::vector<std::string> meta;
  std::vector<uint8_t> metaData;
  if (XFILE::CFile().LoadFile(metaPath, metaData) > 0)
    meta = StringUtils::Split(std::string(metaData.begin(), metaData.end()), '\n');
  if (meta.size() >= 3 && meta[0] == url && XFILE::CFile::Exists(path, false))
  {
    CHttpHeader published;
    if (XFILE::CCurlFile::GetHttpHeader(CURL(url), published))
    {
      if (SameGeneration(published, meta[1], meta[2]))
      {
        CLog::Log(LOGINFO, "CWebFilterLists::{}: {} is unchanged", __FUNCTION__, name);
        return Result::UNCHANGED;
      }
      if (meta.size() >= 5 && !Expired(published, meta[3], meta[4]))
      {
        CLog::Log(LOGINFO, "CWebFilterLists::{}: {} has not expired", __FUNCTION__, name);
        return Result::UNCHANGED;
      }
    }

    if (!meta[1].empty())
      http.SetRequestHeader("If-None-Match", meta[1]);
    if (!meta[2].empty())
      http.SetRequestHeader("If-Modified-Since", meta[2]);
  }

  // a range request makes a server answer 206 rather than 200 or 304
  if (!http.Open(CURL(url + "|seekable=0")))
  {
    CLog::Log(LOGWARNING, "CWebFilterLists::{}: {} cannot be fetched from {}", __FUNCTION__, name,
              url);
    return Result::FAILED;
  }

  const int status = HttpStatus(http.GetHttpHeader());
  if (status == 304)
  {
    CLog::Log(LOGINFO, "CWebFilterLists::{}: {} is unchanged", __FUNCTION__, name);
    return Result::UNCHANGED;
  }

  std::string list;
  char buffer[65536];
  ssize_t read;
  while ((read = http.Read(buffer, sizeof(buffer))) > 0)
    list.append(buffer, read);
  const std::string etag = http.GetHttpHeader().GetValue("etag");
  const std::string modified = http.GetHttpHeader().GetValue("last-modified");
  const std::string fetched = http.GetHttpHeader().GetValue("date");
  http.Close();

  if (status != 200 || read < 0 || list.empty())
  {
    CLog::Log(LOGWARNING,
              "CWebFilterLists::{}: {}: HTTP status {}, {} bytes, keeping what is stored",
              __FUNCTION__, name, status, list.size());
    return Result::FAILED;
  }

  const std::string expires = std::to_string(ListExpiresSeconds(list));
  const std::string stored =
      StringUtils::Join(std::vector<std::string>{url, etag, modified, fetched, expires, ""}, "\n");
  if (!WriteFile(path, list) || !WriteFile(metaPath, stored))
  {
    CLog::Log(LOGERROR, "CWebFilterLists::{}: {} cannot be stored in {}", __FUNCTION__, name,
              m_dir);
    return Result::FAILED;
  }

  CLog::Log(LOGINFO, "CWebFilterLists::{}: {} updated, {} bytes", __FUNCTION__, name, list.size());
  return Result::UPDATED;
}
