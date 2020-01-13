/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "LibInputHandler.h"

#include "LibInputKeyboard.h"
#include "LibInputPointer.h"
#include "LibInputSettings.h"
#include "LibInputTouch.h"
#include "utils/log.h"

#include <algorithm>
#include <string.h>

#include <fcntl.h>
#include <linux/input.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <unistd.h>

static int open_restricted(const char *path, int flags, void __attribute__((unused)) *user_data)
{
  int fd = open(path, flags);

  if (fd < 0)
  {
    CLog::Log(LOGERROR, "%s - failed to open %s (%s)", __FUNCTION__, path, strerror(errno));
    return -errno;
  }

  auto ret = ioctl(fd, EVIOCGRAB, (void*)1);
  if (ret < 0)
  {
    CLog::Log(LOGDEBUG, "%s - grab requested, but failed for %s (%s)", __FUNCTION__, path, strerror(errno));
  }

  return fd;
}

static void close_restricted(int fd, void  __attribute__((unused)) *user_data)
{
  close(fd);
}

static const struct libinput_interface m_interface =
{
  open_restricted,
  close_restricted,
};

static void LogHandler(libinput  __attribute__((unused)) *libinput, libinput_log_priority priority, const char *format, va_list args)
{
  if (priority == LIBINPUT_LOG_PRIORITY_DEBUG)
  {
    char buf[512];
    int n = vsnprintf(buf, sizeof(buf), format, args);
    if (n > 0)
      CLog::Log(LOGDEBUG, "libinput: %s", buf);
  }
}

CLibInputHandler::CLibInputHandler() : CThread("libinput")
{
  m_udev = udev_new();
  if (!m_udev)
  {
    CLog::Log(LOGERROR, "CLibInputHandler::%s - failed to get udev context for libinput", __FUNCTION__);
    return;
  }

  m_li = libinput_udev_create_context(&m_interface, nullptr, m_udev);
  if (!m_li)
  {
    CLog::Log(LOGERROR, "CLibInputHandler::%s - failed to get libinput context", __FUNCTION__);
    return;
  }

  libinput_log_set_handler(m_li, LogHandler);
  libinput_log_set_priority(m_li, LIBINPUT_LOG_PRIORITY_DEBUG);

  auto ret = libinput_udev_assign_seat(m_li, "seat0");
  if (ret < 0)
    CLog::Log(LOGERROR, "CLibInputHandler::%s - failed to assign seat", __FUNCTION__);

  m_liFd = libinput_get_fd(m_li);

  m_keyboard.reset(new CLibInputKeyboard());
  m_pointer.reset(new CLibInputPointer());
  m_touch.reset(new CLibInputTouch());
  m_settings.reset(new CLibInputSettings(this));
}

CLibInputHandler::~CLibInputHandler()
{
  StopThread();

  libinput_unref(m_li);
  udev_unref(m_udev);
}

bool CLibInputHandler::SetKeymap(const std::string& layout)
{
  return m_keyboard->SetKeymap(layout);
}

void CLibInputHandler::Start()
{
  Create();
  SetPriority(GetMinPriority());
}

void CLibInputHandler::Process()
{
  int epollFd = epoll_create1(EPOLL_CLOEXEC);
  if (epollFd < 0)
  {
    CLog::Log(LOGERROR, "CLibInputHandler::%s - failed to create epoll file descriptor: %s", __FUNCTION__, strerror(-errno));
    return;
  }

  epoll_event event;
  event.events = EPOLLIN;
  event.data.fd = m_liFd;

  auto ret = epoll_ctl(epollFd, EPOLL_CTL_ADD, m_liFd, &event);
  if (ret < 0)
  {
    CLog::Log(LOGERROR, "CLibInputHandler::%s - failed to add file descriptor to epoll: %s", __FUNCTION__, strerror(-errno));
    close(epollFd);
    return;
  }

  while (!m_bStop)
  {
    epoll_wait(epollFd, &event, 1, 200);

    ret = libinput_dispatch(m_li);
    if (ret < 0)
    {
      CLog::Log(LOGERROR, "CLibInputHandler::%s - libinput_dispatch failed: %s", __FUNCTION__, strerror(-errno));
      close(epollFd);
      return;
    }

    libinput_event *ev;
    while ((ev = libinput_get_event(m_li)) != nullptr)
    {
      ProcessEvent(ev);
      libinput_event_destroy(ev);
    }
  }

  ret = close(epollFd);
  if (ret < 0)
  {
    CLog::Log(LOGERROR, "CLibInputHandler::%s - failed to close epoll file descriptor: %s", __FUNCTION__, strerror(-errno));
    return;
  }
}

void CLibInputHandler::ProcessEvent(libinput_event *ev)
{
  libinput_event_type type = libinput_event_get_type(ev);
  libinput_device *dev = libinput_event_get_device(ev);

  switch (type)
  {
    case LIBINPUT_EVENT_DEVICE_ADDED:
      DeviceAdded(dev);
      break;
    case LIBINPUT_EVENT_DEVICE_REMOVED:
      DeviceRemoved(dev);
      break;
    case LIBINPUT_EVENT_POINTER_BUTTON:
      m_pointer->ProcessButton(libinput_event_get_pointer_event(ev));
      break;
    case LIBINPUT_EVENT_POINTER_MOTION:
      m_pointer->ProcessMotion(libinput_event_get_pointer_event(ev));
      break;
    case LIBINPUT_EVENT_POINTER_MOTION_ABSOLUTE:
      m_pointer->ProcessMotionAbsolute(libinput_event_get_pointer_event(ev));
      break;
    case LIBINPUT_EVENT_POINTER_AXIS:
      m_pointer->ProcessAxis(libinput_event_get_pointer_event(ev));
      break;
    case LIBINPUT_EVENT_KEYBOARD_KEY:
      m_keyboard->ProcessKey(libinput_event_get_keyboard_event(ev));
      m_keyboard->UpdateLeds(dev);
      break;
    case LIBINPUT_EVENT_TOUCH_DOWN:
      m_touch->ProcessTouchDown(libinput_event_get_touch_event(ev));
      break;
    case LIBINPUT_EVENT_TOUCH_MOTION:
      m_touch->ProcessTouchMotion(libinput_event_get_touch_event(ev));
      break;
    case LIBINPUT_EVENT_TOUCH_UP:
      m_touch->ProcessTouchUp(libinput_event_get_touch_event(ev));
      break;
    case LIBINPUT_EVENT_TOUCH_CANCEL:
      m_touch->ProcessTouchCancel(libinput_event_get_touch_event(ev));
      break;
    case LIBINPUT_EVENT_TOUCH_FRAME:
      m_touch->ProcessTouchFrame(libinput_event_get_touch_event(ev));
      break;

    default:
      break;
  }
}

void CLibInputHandler::DeviceAdded(libinput_device *dev)
{
  const char *sysname = libinput_device_get_sysname(dev);
  const char *name = libinput_device_get_name(dev);

  if (libinput_device_has_capability(dev, LIBINPUT_DEVICE_CAP_TOUCH))
  {
    CLog::Log(LOGDEBUG, "CLibInputHandler::%s - touch type device added: %s (%s)", __FUNCTION__, name, sysname);
    m_devices.push_back(libinput_device_ref(dev));
  }

  if (libinput_device_has_capability(dev, LIBINPUT_DEVICE_CAP_POINTER))
  {
    CLog::Log(LOGDEBUG, "CLibInputHandler::%s - pointer type device added: %s (%s)", __FUNCTION__, name, sysname);
    m_devices.push_back(libinput_device_ref(dev));
  }

  if (libinput_device_has_capability(dev, LIBINPUT_DEVICE_CAP_KEYBOARD))
  {
    CLog::Log(LOGDEBUG, "CLibInputHandler::%s - keyboard type device added: %s (%s)", __FUNCTION__, name, sysname);
    m_devices.push_back(libinput_device_ref(dev));
    m_keyboard->GetRepeat(dev);
  }
}

void CLibInputHandler::DeviceRemoved(libinput_device *dev)
{
  const char *sysname = libinput_device_get_sysname(dev);
  const char *name = libinput_device_get_name(dev);

  if (libinput_device_has_capability(dev, LIBINPUT_DEVICE_CAP_TOUCH))
  {
    CLog::Log(LOGDEBUG, "CLibInputHandler::%s - touch type device removed: %s (%s)", __FUNCTION__, name, sysname);
    auto device = std::find(m_devices.begin(), m_devices.end(), libinput_device_unref(dev));
    m_devices.erase(device);
  }

  if (libinput_device_has_capability(dev, LIBINPUT_DEVICE_CAP_POINTER))
  {
    CLog::Log(LOGDEBUG, "CLibInputHandler::%s - pointer type device removed: %s (%s)", __FUNCTION__, name, sysname);
    auto device = std::find(m_devices.begin(), m_devices.end(), libinput_device_unref(dev));
    m_devices.erase(device);
  }

  if (libinput_device_has_capability(dev, LIBINPUT_DEVICE_CAP_KEYBOARD))
  {
    CLog::Log(LOGDEBUG, "CLibInputHandler::%s - keyboard type device removed: %s (%s)", __FUNCTION__, name, sysname);
    auto device = std::find(m_devices.begin(), m_devices.end(), libinput_device_unref(dev));
    m_devices.erase(device);
  }
}
