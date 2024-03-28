/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "InfoScanner.h"
#include "addons/Scraper.h"

#include <string>
#include <utility>

class CFileItem;
class CSetInfoTag;
class EmbeddedArt;

namespace VIDEO
{

//! \brief Base class for set tag loaders.
class ISetInfoTagLoader
{
public:
  //! \brief Constructor
  //! \param item The item to load info for
  //! \param info Scraper info
  //! \param llokInFolder True to look in folder holding file
  ISetInfoTagLoader(const CFileItem& item) : m_item(item) {}
  virtual ~ISetInfoTagLoader() = default;

  //! \brief Returns true if we have info to provide.
  virtual bool HasInfo() const = 0;

  //! \brief Load tag from file.
  //! \brief tag Tag to load info into
  //! \brief prioritise True to prioritise data over existing data in tag
  //! \returns True if tag was read, false otherwise
  virtual CInfoScanner::INFO_TYPE Load(CSetInfoTag& tag, bool prioritise) = 0;

protected:
  const CFileItem& m_item; //!< Reference to item to load for
};

} // namespace VIDEO