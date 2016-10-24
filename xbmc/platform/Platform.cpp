/*
 *      Copyright (C) 2016 Team Kodi
 *      http://kodi.tv
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
#include "Platform.h"
#include "Application.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

const std::string CPlatform::NoValidUUID = "NOUUID";

// Override for platform ports
#if !defined(PLATFORM_OVERRIDE_CLASSPLATFORM)

CPlatform* CPlatform::CreateInstance()
{
  return new CPlatform();
}

#endif

// base class definitions

CPlatform::CPlatform()
{
  m_uuid = NoValidUUID;
}

void CPlatform::Init()
{
  InitInstanceIdentifier();
}

void CPlatform::InitInstanceIdentifier()
{
  using namespace XFILE;

  const auto path = CSpecialProtocol::TranslatePath("special://home/instance_id");
  if (CFile::Exists(path))
  {
    CFile file;
    if (file.Open(path))
    {
      char temp[36];
      if (file.Read(temp, 36) == 36)
      {
        m_uuid = temp;
        return;
      }
    }
  }

  CLog::Log(LOGDEBUG, "instance id not found. creating..");
  auto uuid = StringUtils::CreateUUID();
  CFile file;
  if (file.OpenForWrite(path, true) && file.Write(uuid.c_str(), 36) == 36)
    m_uuid = std::move(uuid);
  else
    CLog::Log(LOGERROR, "failed to write instance id");
}
