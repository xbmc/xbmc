/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/ISubSettings.h"
#include "settings/lib/ISettingCallback.h"
#include "settings/lib/ISettingsHandler.h"

class TiXmlNode;

/*!
 * \brief Class handling application support for settings.
 */

class CApplicationSettingsHandling : public ISettingCallback,
                                     public ISettingsHandler,
                                     public ISubSettings
{
public:
  //! \brief Push the raster's shape and the interface's fill policy into the graphics context.
  //! Public because the skin reload calls it too - the raster and the skin selected against it
  //! must move in one step on the render thread.
  static void ApplyRasterSettings();

  /*!
   * \brief Carry a moved raster to the layout, whatever moved it - a raster stated over the API
   *        arrives by this same path.
   *
   * The raster is the target a skin's resolution is selected against, so it posts a skin reload
   * and lets that apply the raster in the same step rather than applying it here first.
   */
  static void ApplyRasterChange();

protected:
  void RegisterSettings();
  void UnregisterSettings();

  bool Load(const TiXmlNode* settings) override;
  bool Save(TiXmlNode* settings) const override;
  void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;
  void OnSettingAction(const std::shared_ptr<const CSetting>& setting) override;
  bool OnSettingUpdate(const std::shared_ptr<CSetting>& setting,
                       const char* oldSettingId,
                       const TiXmlNode* oldSettingNode) override;
};
