/*
 *  Copyright (C) 2017-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ShaderTypes.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

class CTexture;

namespace KODI::SHADER
{
/*!
 * \brief A lookup table to apply color transforms in a shader
 */
class IShaderLut
{
public:
  IShaderLut(std::string id, std::string path) : m_id(std::move(id)), m_path(std::move(path)) {}
  virtual ~IShaderLut() = default;

  /*!
   * \brief Create the LUT and allocate resources
   *
   * \param lut The LUT information structure
   *
   * \return Returns true if successful and the LUT can be used, false otherwise
   */
  virtual bool Create(const ShaderLut& lut) = 0;

  /*!
   * \brief Gets ID of LUT
   *
   * \return Returns unique name (ID) of look up texture
   */
  const std::string& GetID() const { return m_id; }

  /*!
   * \brief Gets full path of LUT
   *
   * \return Returns full path of look up texture
   */
  const std::string& GetPath() const { return m_path; }

  /*!
   * \brief Gets texture of LUT
   *
   * \return Returns pointer to the texture where the LUT data is stored in
   */
  virtual CTexture* GetTexture() = 0;

protected:
  std::string m_id;
  std::string m_path;
};
} // namespace KODI::SHADER
