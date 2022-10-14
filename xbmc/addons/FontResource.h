/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/Resource.h"

#include <memory>

namespace ADDON
{
class CFontResource : public CResource
{
public:
  explicit CFontResource(const AddonInfoPtr& addonInfo);

  //! \brief Check whether file is allowed or not (no filters here).
  bool IsAllowed(const std::string& file) const override { return true; }

  //! \brief Get the font path if given font file is served by the add-on.
  //! \param[in] file File name of font.
  //! \param[out] path Full path to font if found.
  //! \return True if font was found, false otherwise.
  bool GetFont(const std::string& file, std::string& path) const;

  //! \brief Callback executed after installation
  void OnPostInstall(bool update, bool modal) override;
};

}
