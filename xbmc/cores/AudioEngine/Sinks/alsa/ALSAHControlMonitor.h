/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "platform/Platform.h"

#include <map>
#include <string>
#include <vector>

#include <alsa/asoundlib.h>

class CALSAHControlMonitor : public IPlatformService
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
    int useCount = 0;

    explicit CTLHandle(snd_hctl_t *handle_) : handle(handle_) {}
    CTLHandle() : handle(NULL) {}
  };

  std::map<std::string, CTLHandle> m_ctlHandles;

  std::vector<int> m_fdMonitorIds;
};
