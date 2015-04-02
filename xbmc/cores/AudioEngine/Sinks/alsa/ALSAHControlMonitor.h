#pragma once
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

#include <string>
#include <map>
#include <vector>

#include <alsa/asoundlib.h>

class CALSAHControlMonitor
{
public:
  CALSAHControlMonitor();
  ~CALSAHControlMonitor();

  bool Add(const std::string& ctlHandleName,
           snd_ctl_elem_iface_t interface,
           unsigned int device,
           const std::string& name);

  void Clear();

  void Start();
  void Stop();

private:
  static int HCTLCallback(snd_hctl_elem_t *elem, unsigned int mask);
  static void FDEventCallback(int id, int fd, short revents, void *data);

  snd_hctl_t* GetHandle(const std::string& ctlHandleName);
  void PutHandle(const std::string& ctlHandleName);

  struct CTLHandle
  {
    snd_hctl_t *handle;
    int useCount;

    CTLHandle(snd_hctl_t *handle_) : handle(handle_), useCount(0) {}
    CTLHandle() : handle(NULL), useCount(0) {}
  };

  std::map<std::string, CTLHandle> m_ctlHandles;

  std::vector<int> m_fdMonitorIds;
};

#endif

