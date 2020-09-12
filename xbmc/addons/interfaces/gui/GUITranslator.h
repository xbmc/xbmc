/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/kodi-dev-kit/include/kodi/c-api/gui/input/action_ids.h"

namespace ADDON
{

/*!
 * \brief Translates data types from GUI API to the corresponding format in Kodi.
 *
 * This class is stateless.
 */
class CAddonGUITranslator
{
  CAddonGUITranslator() = delete;

public:
  /*!
   * \brief Translate Kodi's action id's to addon
   * \param kodiId Kodi's action identifier
   * \return Addon action identifier
   */
  static ADDON_ACTION TranslateActionIdToAddon(int kodiId);

  /*!
   * \brief Translate addon's action id's to Kodi
   * \param addonId Addon's action identifier
   * \return Kodi action identifier
   */
  static int TranslateActionIdToKodi(ADDON_ACTION addonId);
};

} /* namespace ADDON */
