/*
 *  Copyright (C) 2024-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "InfoScanner.h"

#include <string>

class CFileItem;
class CSetInfoTag;
class EmbeddedArt;

namespace KODI::VIDEO
{

//! \brief Base class for set tag loaders.
class ISetInfoTagLoader
{
public:
  //! \brief Constructor
  //! \param title The title of the set to load info for
  explicit ISetInfoTagLoader(const std::string& title) : m_title(title) {}
  virtual ~ISetInfoTagLoader() = default;

  //! \brief Returns true if we have info to provide.
  virtual bool HasInfo() const = 0;

  //! \brief Load tag from file.
  //! \brief tag Tag to load info into
  //! \brief prioritise True to prioritise data over existing data in tag
  //! \returns True if tag was read, false otherwise
  virtual CInfoScanner::InfoType Load(CSetInfoTag& tag, bool prioritise) = 0;

protected:
  const std::string m_title;
};

} // namespace KODI::VIDEO
