/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "OSScreenSaver.h"

#include "utils/log.h"

using namespace KODI::WINDOWING;

COSScreenSaverManager::COSScreenSaverManager(std::unique_ptr<IOSScreenSaver> impl)
: m_impl{std::move(impl)}
{
}

COSScreenSaverInhibitor COSScreenSaverManager::CreateInhibitor()
{
  COSScreenSaverInhibitor inhibitor{this};
  if (m_inhibitionCount++ == 0)
  {
    // Inhibit if this was first inhibitor
    CLog::Log(LOGDEBUG, "Inhibiting OS screen saver");
    m_impl->Inhibit();
  }
  return inhibitor;
}

bool COSScreenSaverManager::IsInhibited()
{
  return (m_inhibitionCount > 0);
}

void COSScreenSaverManager::RemoveInhibitor()
{
  if (--m_inhibitionCount == 0)
  {
    CLog::Log(LOGDEBUG, "Uninhibiting OS screen saver");
    // Uninhibit if this was last inhibitor
    m_impl->Uninhibit();
  }
}

COSScreenSaverInhibitor::COSScreenSaverInhibitor() noexcept
: m_active{false}, m_manager{}
{
}

COSScreenSaverInhibitor::COSScreenSaverInhibitor(COSScreenSaverManager* manager)
: m_active{true}, m_manager{manager}
{
}

COSScreenSaverInhibitor::COSScreenSaverInhibitor(COSScreenSaverInhibitor&& other) noexcept
: m_active{false}, m_manager{}
{
  *this = std::move(other);
}

COSScreenSaverInhibitor& COSScreenSaverInhibitor::operator=(COSScreenSaverInhibitor&& other) noexcept
{
  Release();
  m_active = other.m_active;
  m_manager = other.m_manager;
  other.m_active = false;
  other.m_manager = nullptr;
  return *this;
}

bool COSScreenSaverInhibitor::IsActive() const
{
  return m_active;
}

COSScreenSaverInhibitor::operator bool() const
{
  return IsActive();
}

void COSScreenSaverInhibitor::Release()
{
  if (m_active)
  {
    m_manager->RemoveInhibitor();
    m_active = false;
  }
}

COSScreenSaverInhibitor::~COSScreenSaverInhibitor() noexcept
{
  Release();
}



