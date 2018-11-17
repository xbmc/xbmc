/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "LogindUtils.h"

#include "SessionUtils.h"
#include "utils/log.h"

#include "platform/linux/DBusMessage.h"

#include <sys/sysmacros.h>

namespace
{
constexpr auto logindService{"org.freedesktop.login1"};
constexpr auto logindObject{"/org/freedesktop/login1"};
constexpr auto logindManagerInterface{"org.freedesktop.login1.Manager"};
constexpr auto logindSessionInterface{"org.freedesktop.login1.Session"};
} // namespace

int CLogindUtils::Open(const std::string& path, int flags)
{
  struct stat st;
  int ret = stat(path.c_str(), &st);
  if (ret < 0)
  {
    CLog::Log(LOGERROR, "logind: stat failed for: {}", path);
    return -1;
  }

  if (!S_ISCHR(st.st_mode))
  {
    CLog::Log(LOGERROR, "logind: invalid device passed");
    return -1;
  }

  int fd = TakeDevice(m_sessionPath, major(st.st_rdev), minor(st.st_rdev));
  if (fd < 0)
    return fd;

  ret = fcntl(fd, F_GETFL);
  if (ret < 0)
  {
    CLog::Log(LOGERROR, "logind: F_GETFL failed");
    Close(fd);
    return -1;
  }

  if (flags & O_NONBLOCK)
  {
    ret = fcntl(fd, F_SETFL, O_NONBLOCK);
    if (ret < 0)
    {
      CLog::Log(LOGERROR, "logind: F_SETFL failed");
      Close(fd);
      return -1;
    }
  }

  return fd;
}

void CLogindUtils::Close(int fd)
{
  struct stat st;
  int ret = fstat(fd, &st);
  close(fd);
  if (ret < 0)
  {
    CLog::Log(LOGERROR, "logind: cannot fstat fd: {}", fd);
    return;
  }

  if (!S_ISCHR(st.st_mode))
  {
    CLog::Log(LOGERROR, "logind: invalid device passed");
    return;
  }

  ReleaseDevice(m_sessionPath, major(st.st_rdev), minor(st.st_rdev));
}

std::string CLogindUtils::GetSessionPath()
{
  CDBusMessage message(logindService, logindObject, logindManagerInterface, "GetSessionByPID");
  message.AppendArguments<uint32_t>(getpid());

  CDBusError error;
  auto reply = message.Send(m_connection, error);
  if (!reply)
  {
    CLog::Log(LOGERROR, "logind: GetSessionByPID failed: ({})", error.Message());
    return "";
  }

  // left this c-style for now as our dbus methods don't accept DBUS_TYPE_OBJECT_PATH
  char* path;
  auto b = dbus_message_get_args(reply, nullptr, DBUS_TYPE_OBJECT_PATH, &path, DBUS_TYPE_INVALID);

  if (!b)
  {
    CLog::Log(LOGERROR, "logind: failed to get session path");
    return "";
  }

  return std::string{path};
}

int CLogindUtils::TakeDevice(const std::string& sessionPath, uint32_t major, uint32_t minor)
{
  CDBusMessage message(logindService, sessionPath, logindSessionInterface, "TakeDevice");
  message.AppendArguments<uint32_t, uint32_t>(major, minor);

  CDBusError error;
  auto reply = message.Send(m_connection, error);
  if (!reply)
  {
    CLog::Log(LOGERROR, "logind: TakeDevice failed for session: {} ({})", sessionPath,
              error.Message());
    return -1;
  }

  // left this c-style for now as our dbus methods don't accept DBUS_TYPE_UNIX_FD
  int fd;
  dbus_bool_t active;
  auto b = dbus_message_get_args(reply, nullptr, DBUS_TYPE_UNIX_FD, &fd, DBUS_TYPE_BOOLEAN, &active,
                                 DBUS_TYPE_INVALID);

  if (!b)
  {
    CLog::Log(LOGERROR, "logind: failed to get unix fd");
    return -1;
  }

  return fd;
}

void CLogindUtils::ReleaseDevice(const std::string& sessionPath, uint32_t major, uint32_t minor)
{
  CDBusMessage message(logindService, sessionPath, logindSessionInterface, "ReleaseDevice");
  message.AppendArguments<uint32_t, uint32_t>(major, minor);

  if (!message.SendAsyncSystem())
  {
    CLog::Log(LOGERROR, "logind: ReleaseDevice failed");
  }
}

bool CLogindUtils::TakeControl()
{
  CDBusMessage message(logindService, m_sessionPath, logindSessionInterface, "TakeControl");

  message.AppendArgument<bool>(false);

  CDBusError error;
  auto reply = message.Send(m_connection, error);

  if (!reply)
  {
    CLog::Log(LOGERROR, "logind: cannot take control over session: {} ({})", m_sessionPath,
              error.Message());
    return false;
  }

  return true;
}

void CLogindUtils::ReleaseControl()
{
  CDBusMessage message(logindService, m_sessionPath, logindSessionInterface, "ReleaseControl");
  message.SendAsync(m_connection);
}

bool CLogindUtils::Activate()
{
  CDBusMessage message(logindService, m_sessionPath, logindSessionInterface, "Activate");

  if (!message.SendAsync(m_connection))
  {
    CLog::Log(LOGERROR, "logind: cannot activate session: {}", m_sessionPath);
    return false;
  }

  return true;
}

bool CLogindUtils::Connect()
{
  if (!m_connection.Connect(DBUS_BUS_SYSTEM, false))
  {
    CLog::Log(LOGERROR, "logind: cannot connect to system dbus");
    return false;
  }

  m_sessionPath = GetSessionPath();

  if (m_sessionPath.empty())
  {
    CLog::Log(LOGERROR, "logind: no dbus session available, cannot register session");
    return false;
  }

  if (!TakeControl())
  {
    return false;
  }

  if (!Activate())
  {
    return false;
  }

  CLog::Log(LOGDEBUG, "logind: successfully registered session: {}", m_sessionPath);

  return true;
}

void CLogindUtils::Destroy()
{
  ReleaseControl();
}
