/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ISetInfoTagLoader.h"

#include <string>

//! \brief Set tag loader using nfo files.
class CSetTagLoaderNFO : public KODI::VIDEO::ISetInfoTagLoader
{
public:
  CSetTagLoaderNFO(const std::string& title);

  ~CSetTagLoaderNFO() override = default;

  //! \brief Returns whether or not read has info.
  bool HasInfo() const override;

  //! \brief Load "tag" from nfo file.
  //! \brief tag Tag to load info into
  CInfoScanner::INFO_TYPE Load(CSetInfoTag& tag, bool prioritise = false) override;

protected:
  std::string m_path; //!< Path to nfo file
};
