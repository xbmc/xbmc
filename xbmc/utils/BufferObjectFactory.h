/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "BufferObject.h"

#include <functional>
#include <memory>
#include <vector>

class CBufferObjectFactory
{
public:
  static std::unique_ptr<CBufferObject> CreateBufferObject();

  static void RegisterBufferObject(std::function<std::unique_ptr<CBufferObject>()>);
  static void ClearBufferObjects();

protected:
  static std::vector<std::function<std::unique_ptr<CBufferObject>()>> m_bufferObjects;
};
