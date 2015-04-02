/*
 *      Copyright (C) 2014 Team Kodi
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#include "system.h"
#ifdef HAS_ALSA

#include "ALSAHControlMonitor.h"

#include "AEFactory.h"
#include "linux/FDEventMonitor.h"
#include "utils/log.h"

CALSAHControlMonitor::CALSAHControlMonitor()
{
}

CALSAHControlMonitor::~CALSAHControlMonitor()
{
  Clear();
}

bool CALSAHControlMonitor::Add(const std::string& ctlHandleName,
                               snd_ctl_elem_iface_t interface,
                               unsigned int device,
                               const std::string& name)
{
  snd_hctl_t *hctl = GetHandle(ctlHandleName);

  if (!hctl)
  {
    return false;
  }

  snd_ctl_elem_id_t *id;

  snd_ctl_elem_id_alloca(&id);

  snd_ctl_elem_id_set_interface(id, interface);
  snd_ctl_elem_id_set_name     (id, name.c_str());
  snd_ctl_elem_id_set_device   (id, device);

  snd_hctl_elem_t *elem = snd_hctl_find_elem(hctl, id);

  if (!elem)
  {
    PutHandle(ctlHandleName);
    return false;
  }

  snd_hctl_elem_set_callback(elem, HCTLCallback);

  return true;
}

void CALSAHControlMonitor::Clear()
{
  Stop();

  for (std::map<std::string, CTLHandle>::iterator it = m_ctlHandles.begin();
       it != m_ctlHandles.end(); ++it)
  {
    snd_hctl_close(it->second.handle);
  }
  m_ctlHandles.clear();
}

void CALSAHControlMonitor::Start()
{
  assert(m_fdMonitorIds.size() == 0);

  std::vector<struct pollfd> pollfds;
  std::vector<CFDEventMonitor::MonitoredFD> monitoredFDs;

  for (std::map<std::string, CTLHandle>::iterator it = m_ctlHandles.begin();
       it != m_ctlHandles.end(); ++it)
  {
    pollfds.resize(snd_hctl_poll_descriptors_count(it->second.handle));
    int fdcount = snd_hctl_poll_descriptors(it->second.handle, &pollfds[0], pollfds.size());

    for (int j = 0; j < fdcount; ++j)
    {
      monitoredFDs.push_back(CFDEventMonitor::MonitoredFD(pollfds[j].fd,
                                                          pollfds[j].events,
                                                          FDEventCallback,
                                                          it->second.handle));
    }
  }

  g_fdEventMonitor.AddFDs(monitoredFDs, m_fdMonitorIds);
}


void CALSAHControlMonitor::Stop()
{
  g_fdEventMonitor.RemoveFDs(m_fdMonitorIds);
  m_fdMonitorIds.clear();
}

int CALSAHControlMonitor::HCTLCallback(snd_hctl_elem_t *elem, unsigned int mask)
{
  /* _REMOVE is a special value instead of a bit and must be checked first */
  if (mask == SND_CTL_EVENT_MASK_REMOVE)
  {
    /* Either the device was removed (which is handled in ALSADeviceMonitor instead)
     * or snd_hctl_close() got called. */
    return 0;
  }

  if (mask & SND_CTL_EVENT_MASK_VALUE)
  {
    CLog::Log(LOGDEBUG, "CALSAHControlMonitor - Monitored ALSA hctl value changed");

    /*
     * Currently we just re-enumerate on any change.
     * Custom callbacks for handling other control monitoring may be implemented when needed.
     */
    CAEFactory::DeviceChange();
  }

  return 0;
}

void CALSAHControlMonitor::FDEventCallback(int id, int fd, short revents, void *data)
{
  /* Run ALSA event handling when the FD has events */
  snd_hctl_t *hctl = (snd_hctl_t *)data;
  snd_hctl_handle_events(hctl);
}

snd_hctl_t* CALSAHControlMonitor::GetHandle(const std::string& ctlHandleName)
{
  if (!m_ctlHandles.count(ctlHandleName))
  {
    snd_hctl_t *hctl;

    if (snd_hctl_open(&hctl, ctlHandleName.c_str(), 0) != 0)
    {
        CLog::Log(LOGWARNING, "CALSAHControlMonitor::GetHandle - snd_hctl_open() failed for \"%s\"", ctlHandleName.c_str());
        return NULL;
    }
    if (snd_hctl_load(hctl) != 0)
    {
      CLog::Log(LOGERROR, "CALSAHControlMonitor::GetHandle - snd_hctl_load() failed for \"%s\"", ctlHandleName.c_str());
      snd_hctl_close(hctl);
      return NULL;
    }

    snd_hctl_nonblock(hctl, 1);

    m_ctlHandles[ctlHandleName] = CTLHandle(hctl);
  }

  m_ctlHandles[ctlHandleName].useCount++;
  return m_ctlHandles[ctlHandleName].handle;
}

void CALSAHControlMonitor::PutHandle(const std::string& ctlHandleName)
{
  if (--m_ctlHandles[ctlHandleName].useCount == 0)
  {
    snd_hctl_close(m_ctlHandles[ctlHandleName].handle);
    m_ctlHandles.erase(ctlHandleName);
  }
}


#endif
