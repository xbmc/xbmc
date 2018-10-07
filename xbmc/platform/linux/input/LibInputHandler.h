/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/Thread.h"

#include <libinput.h>
#include <libudev.h>
#include <memory>
#include <vector>

class CLibInputKeyboard;
class CLibInputPointer;
class CLibInputSettings;
class CLibInputTouch;

class CLibInputHandler : CThread
{
public:
  CLibInputHandler();
  ~CLibInputHandler();

  void Start();

  bool SetKeymap(const std::string& layout);

private:
  void Process() override;
  void ProcessEvent(libinput_event *ev);
  void DeviceAdded(libinput_device *dev);
  void DeviceRemoved(libinput_device *dev);

  udev *m_udev;
  libinput *m_li;
  int m_liFd;

  std::unique_ptr<CLibInputKeyboard> m_keyboard;
  std::unique_ptr<CLibInputPointer> m_pointer;
  std::unique_ptr<CLibInputSettings> m_settings;
  std::unique_ptr<CLibInputTouch> m_touch;
  std::vector<libinput_device*> m_devices;
};

