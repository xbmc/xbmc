/*
 *      Copyright (C) 2017 Team XBMC
 *      http://xbmc.org
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

#include "OSScreenSaver.h"

#include "utils/log.h"

using namespace KODI::WINDOWING;

COSScreenSaverManager::COSScreenSaverManager(std::unique_ptr<IOSScreenSaver>&& impl)
: m_impl(std::move(impl))
{
}

COSScreenSaverInhibitor COSScreenSaverManager::CreateInhibitor()
{
  COSScreenSaverInhibitor inhibitor(this);
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

COSScreenSaverInhibitor::COSScreenSaverInhibitor()
: m_active(false), m_manager(nullptr)
{
}

COSScreenSaverInhibitor::COSScreenSaverInhibitor(COSScreenSaverManager* manager)
: m_active(true), m_manager(manager)
{
}

COSScreenSaverInhibitor::COSScreenSaverInhibitor(COSScreenSaverInhibitor&& other)
: m_active(false), m_manager(nullptr)
{
  *this = std::move(other);
}

COSScreenSaverInhibitor& COSScreenSaverInhibitor::operator=(COSScreenSaverInhibitor&& other)
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

COSScreenSaverInhibitor::~COSScreenSaverInhibitor()
{
  Release();
}



