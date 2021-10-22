/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <vector>

namespace KODI
{
namespace ADDONS
{

/*!
 * @brief To identify related type about entry
 */
enum class AddonSupportType
{
  /* To set if type relates to a file extension */
  Extension,

  /* Used if relates to a Internet Media Type (MIME-Type) */
  Mimetype
};

/*!
 * @brief Information structure with which a supported format of an addon can
 * be stored.
 */
struct AddonSupportEntry
{
  /* Type what this is about */
  AddonSupportType m_type;

  /* The name used for processing */
  std::string m_name;

  /* User-friendly description of the name type stored here. */
  std::string m_description;

  /* Path to use own icon about. */
  std::string m_icon;
};

/*!
 * @brief Parent class to manage all available mimetypes and file extensions of
 * the respective add-on and its types.
 *
 * This class should be integrated in the respective add-on type manager and in
 * order to get an overview of all that are intended for file processing.
 *
 * @todo Extend this class with database usage and support to activate /
 * deactivate the respective formats and thus to override add-ons if several
 * support the same.
 */
class IAddonSupportList
{
public:
  IAddonSupportList() = default;
  virtual ~IAddonSupportList() = default;

  /*!
   * @brief To give all file extensions and MIME types supported by the addon.
   *
   * @param[in] addonId Identifier about wanted addon
   * @return List of all supported parts on selected addon
   */
  virtual std::vector<AddonSupportEntry> GetSupportedExtsAndMimeTypes(
      const std::string& addonId) = 0;
};

} /* namespace ADDONS */
} /* namespace KODI */
