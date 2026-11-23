/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AnnouncementManager.h"

#include "FileItem.h"
#include "threads/SingleLock.h"
#include "utils/log.h"

#include <memory>
#include <mutex>
#include <variant>
#include <vector>

using namespace ANNOUNCEMENT;

const std::string CAnnouncementManager::ANNOUNCEMENT_SENDER = "xbmc";

namespace
{

void Deliver(IAnnouncer& announcer, const ANNOUNCEMENT::PlayerEvent& event)
{
  announcer.OnPlayerEvent(event);
}

void Deliver(IAnnouncer& announcer, const ANNOUNCEMENT::PlaylistEvent& event)
{
  announcer.OnPlaylistEvent(event);
}

void Deliver(IAnnouncer& announcer, const ANNOUNCEMENT::GUIEvent& event)
{
  announcer.OnGUIEvent(event);
}

void Deliver(IAnnouncer& announcer, const ANNOUNCEMENT::SystemEvent& event)
{
  announcer.OnSystemEvent(event);
}

void Deliver(IAnnouncer& announcer, const ANNOUNCEMENT::VideoLibraryEvent& event)
{
  announcer.OnVideoLibraryEvent(event);
}

void Deliver(IAnnouncer& announcer, const ANNOUNCEMENT::AudioLibraryEvent& event)
{
  announcer.OnAudioLibraryEvent(event);
}

void Deliver(IAnnouncer& announcer, const ANNOUNCEMENT::InputEvent& event)
{
  announcer.OnInputEvent(event);
}

void Deliver(IAnnouncer& announcer, const ANNOUNCEMENT::PVREvent& event)
{
  announcer.OnPVREvent(event);
}

void Deliver(IAnnouncer& announcer, const ANNOUNCEMENT::InfoEvent& event)
{
  announcer.OnInfoEvent(event);
}

void Deliver(IAnnouncer& announcer, const ANNOUNCEMENT::SourcesEvent& event)
{
  announcer.OnSourcesEvent(event);
}

void Deliver(IAnnouncer& announcer, const ANNOUNCEMENT::FilesEvent& event)
{
  announcer.OnFilesEvent(event);
}

void Deliver(IAnnouncer& announcer, const ANNOUNCEMENT::SettingsEvent& event)
{
  announcer.OnSettingsEvent(event);
}

void Deliver(IAnnouncer& announcer, const ANNOUNCEMENT::OtherEvent& event)
{
  announcer.OnOtherEvent(event);
}

} // unnamed namespace

void IAnnouncer::OnAnnouncement(const Announcement& announcement)
{
  std::visit([this](const auto& event) { Deliver(*this, event); },
             static_cast<const Announcement::variant&>(announcement));
}

CAnnouncementManager::CAnnouncementManager() : CThread("Announce")
{
}

CAnnouncementManager::~CAnnouncementManager()
{
  Deinitialize();
}

void CAnnouncementManager::Start()
{
  Create();
}

void CAnnouncementManager::Deinitialize()
{
  m_bStop = true;
  m_queueEvent.Set();
  StopThread();
  std::unique_lock lock(m_announcersCritSection);
  m_announcers.clear();
}

void CAnnouncementManager::AddAnnouncer(IAnnouncer *listener)
{
  return AddAnnouncer(listener, ANNOUNCE_ALL);
}

void CAnnouncementManager::AddAnnouncer(IAnnouncer* listener, int flagMask)
{
  if (!listener)
    return;

  std::unique_lock lock(m_announcersCritSection);
  m_announcers.emplace(listener, flagMask);
}

void CAnnouncementManager::RemoveAnnouncer(IAnnouncer *listener)
{
  if (!listener)
    return;

  std::unique_lock lock(m_announcersCritSection);
  m_announcers.erase(listener);

  // Its owner may destroy it once this returns, so a call in progress elsewhere must finish
  // first. An announcer removing itself from inside that call is on this thread.
  if (!IsCurrentThread())
    m_announced.wait(lock, [this, listener] { return m_announcing != listener; });
}

void CAnnouncementManager::Announce(const Announcement& announcement)
{
  Announcement queued = announcement;
  if (const auto item = ItemOf(announcement); item)
    queued = WithItem(announcement, std::make_shared<CFileItem>(*item));

  {
    std::unique_lock lock(m_queueCritSection);
    m_announcementQueue.push_back(std::move(queued));
  }
  m_queueEvent.Set();
}

void CAnnouncementManager::DoAnnounce(const Announcement& announcement)
{
  CLog::LogFC(LOGWARNING, LOGANNOUNCE, "CAnnouncementManager - Announcement: {} from {}",
              MessageOf(announcement), SenderOf(announcement));

  const AnnouncementFlag flag = FlagOf(announcement);
  std::unique_lock lock(m_announcersCritSection);

  std::vector<IAnnouncer*> announcers;
  announcers.reserve(m_announcers.size());
  for (const auto& [announcer, flagMask] : m_announcers)
    announcers.push_back(announcer);

  for (IAnnouncer* announcer : announcers)
  {
    // Re-read: the list may have changed while the lock was released for the previous call.
    const auto it = m_announcers.find(announcer);
    if (it == m_announcers.end() || !(flag & it->second))
      continue;

    // The lock is released for the call. Announcers wait on other threads - closing a window
    // waits on the GUI thread - and the GUI thread adds announcers of its own.
    m_announcing = announcer;
    try
    {
      CSingleExit unlock(m_announcersCritSection);
      announcer->OnAnnouncement(announcement);
    }
    catch (...)
    {
      // An exception ends this thread; a removal waiting on the call must not wait for ever
      m_announcing = nullptr;
      m_announced.notifyAll();
      throw;
    }
    m_announcing = nullptr;
    m_announced.notifyAll();
  }
}

void CAnnouncementManager::Process()
{
  SetPriority(ThreadPriority::LOWEST);

  while (!m_bStop)
  {
    std::unique_lock lock(m_queueCritSection);
    if (!m_announcementQueue.empty())
    {
      Announcement announcement = std::move(m_announcementQueue.front());
      m_announcementQueue.pop_front();
      {
        CSingleExit ex(m_queueCritSection);
        DoAnnounce(WithLibraryDetails(std::move(announcement)));
      }
    }
    else
    {
      CSingleExit ex(m_queueCritSection);
      m_queueEvent.Wait();
    }
  }
}
