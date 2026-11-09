/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/Addon.h"

#include <string>

class TiXmlElement;

namespace ADDON
{

//! \brief A web app, whose settings include the ones Kodi reads for every web app
class CWebApp : public CAddon
{
public:
  explicit CWebApp(const AddonInfoPtr& addonInfo);

  //! \brief Whether the app imports a kodi.webapp whose settings Kodi defines for the app
  bool LeavesKodiSettingsToKodi() const;

protected:
  bool LoadSettingsDefinition(const std::string& path,
                              CXBMCTinyXML& doc,
                              AddonInstanceId id) override;

private:
  void DropUnusedKodiCategories(TiXmlElement& section) const;
};

} // namespace ADDON
