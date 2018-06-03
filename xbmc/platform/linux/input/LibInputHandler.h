/*
 *      Copyright (C) 2005-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "threads/Thread.h"

#include <libinput.h>
#include <libudev.h>
#include <memory>
#include <vector>

class CLibInputKeyboard;
class CLibInputPointer;
class CLibInputTouch;

class CLibInputHandler : CThread
{
public:
  CLibInputHandler();
  ~CLibInputHandler();

  void Start();

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
  std::unique_ptr<CLibInputTouch> m_touch;
  std::vector<libinput_device*> m_devices;
};

