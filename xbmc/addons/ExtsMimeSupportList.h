/*
 *  Copyright (C) 2005-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/AddonEvents.h"
#include "addons/IAddon.h"
#include "threads/CriticalSection.h"

namespace ADDON
{
class CAddonMgr;
}

namespace KODI
{
namespace ADDONS
{

/*!
 * @brief Class to manage all available and activated add-ons and
 * to release their types to the outside for selection.
 *
 * @note It may also make sense in the future to expand this class and design it
 * globally in Kodi so that other addons can also be managed with it (all which
 * have mimetypes and file extensions, e.g. vfs, audioencoder).
 */
class CExtsMimeSupportList
{
public:
  CExtsMimeSupportList(ADDON::CAddonMgr& addonMgr);
  ~CExtsMimeSupportList();

  /*!
   * @brief Filter selection values.
   */
  enum class FilterSelect
  {
    /*! To select all available */
    all,

    /*! To select only them where support tags */
    hasTags,

    /*! Get only where support tracks within */
    hasTracks
  };

  /*!
   * @brief Structure to store available data for related addon.
   */
  struct SupportValues
  {
    // Type of stored addon to check on scan
    ADDON::TYPE m_addonType{ADDON::ADDON_UNKNOWN};

    // Related addon info class
    std::shared_ptr<ADDON::CAddonInfo> m_addonInfo;

    // Addon his own codec identification name
    std::string m_codecName;

    // If as true support addons own song information tags
    bool m_hasTags{false};

    // To know addon includes several tracks inside one file, if set to true
    bool m_hasTracks{false};

    // List of supported file extensions by addon
    // First: Extension name
    // Second: Language id to identify in strings.po defined description text about
    std::map<std::string, int> m_supportedExtensions;

    // List of supported mimetypes by addon
    // First: Mimetype name
    // Second: Language id to identify in strings.po defined description text about
    std::map<std::string, int> m_supportedMimetypes;
  };

  /*!
   * @brief Get list of all by audiodecoder supported parts.
   *
   * Thought to use on planned new window about management about supported
   * extensions and mimetypes in Kodi and to allow edit by user to enable or
   * disable corresponding parts.
   *
   * This function is also used to notify Kodi supported formats and to allow
   * playback.
   *
   * @param[in] select To filter the listed information by type
   * @return List of the available types listed for the respective add-on
   */
  std::vector<SupportValues> GetSupportedAddonInfos(FilterSelect select);

  /*!
   * @brief To query the desired file extension is supported in it.
   *
   * @param[in] ext Extension name to check
   * @return True if within supported, false if not
   */
  bool IsExtensionSupported(const std::string& ext);

  /*!
   * @brief To get a list of all compatible audio decoder add-ons for the file extension.
   *
   * @param[in] ext Extension name to check
   * @param[in] select To filter the listed information by type
   * @return List of @ref ADDON::CAddonInfo where support related extension
   */
  std::vector<std::pair<ADDON::TYPE, std::shared_ptr<ADDON::CAddonInfo>>>
  GetExtensionSupportedAddonInfos(const std::string& ext, FilterSelect select);

  /*!
   * @brief To query the desired file mimetype is supported in it.
   *
   * @param[in] mimetype Mimetype name to check
   * @return True if within supported, false if not
   */
  bool IsMimetypeSupported(const std::string& mimetype);

  /*!
   * @brief To get a list of all compatible audio decoder add-ons for the mimetype.
   *
   * @param[in] mimetype Mimetype name to check
   * @param[in] select To filter the listed information by type
   * @return List of @ref ADDON::CAddonInfo where support related mimetype
   */
  std::vector<std::pair<ADDON::TYPE, std::shared_ptr<ADDON::CAddonInfo>>>
  GetMimetypeSupportedAddonInfos(const std::string& mimetype, FilterSelect select);

protected:
  void Update(const std::string& id);
  void OnEvent(const ADDON::AddonEvent& event);

  static SupportValues ScanAddonProperties(ADDON::TYPE type,
                                           const std::shared_ptr<ADDON::CAddonInfo>& addonInfo);

  CCriticalSection m_critSection;

  std::vector<SupportValues> m_supportedList;
  ADDON::CAddonMgr& m_addonMgr;
};

} /* namespace ADDONS */
} /* namespace KODI */
