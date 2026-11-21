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
#include <list>
#include <memory>
#include <string>

/**
 * @brief Factory that provides CBufferObject registration and creation
 *
 */
class CBufferObjectFactory
{
public:
  /**
   * @brief Create a CBufferObject from the registered BufferObject types.
   *        In the future this may include some criteria for selecting a specific
   *        CBufferObject derived type. Currently it returns the CBufferObject
   *        implementation that was registered last.
   *
   * @return std::unique_ptr<CBufferObject>
   */
  static std::unique_ptr<CBufferObject> CreateBufferObject(bool needsCreateBySize);

  /**
   * @brief Create a CBufferObject of the registered type with the given name.
   *
   * @return std::unique_ptr<CBufferObject>, or nullptr if no type of that name is registered
   */
  static std::unique_ptr<CBufferObject> CreateBufferObjectByName(const std::string& name);

  /**
   * @brief Registers a CBufferObject class to class to the factory.
   *
   */
  static void RegisterBufferObject(const std::function<std::unique_ptr<CBufferObject>()>&);

  /**
   * @brief Clears the list of registered CBufferObject types
   *
   */
  static void ClearBufferObjects();

protected:
  static std::list<std::function<std::unique_ptr<CBufferObject>()>> m_bufferObjects;
};
