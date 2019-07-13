/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/Resource.h"

#include <string>

class CURL;

namespace ADDON
{

//! \brief A collection of images. The collection can have a type.
class CImageResource : public CResource
{
public:
  explicit CImageResource(const AddonInfoPtr& addonInfo);

  void OnPreUnInstall() override;

  bool IsAllowed(const std::string &file) const override;
  std::string GetFullPath(const std::string &filePath) const override;

  //! \brief Returns type of image collection
  const std::string& GetType() const { return m_type; }

private:
  bool HasXbt(CURL& xbtUrl) const;

  std::string m_type; //!< Type of images
};

} /* namespace ADDON */
