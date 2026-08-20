/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Setting.h"
#include "settings/SettingsValueXmlSerializer.h"
#include "settings/lib/SettingsManager.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

#include <concepts>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace KODI::SETTINGS
{
/*!
 * \brief Outcome of a setting migration operation.
 */
enum class SettingConversionResult
{
  FAILURE, ///< Failed conversion, irrecoverable inconsistent state.
  NOT_PRESENT, ///< The old setting ID could not be located.
  INVALID, ///< The old setting has a value incompatible with its data type.
  CONVERTED, ///< The conversion was successful.
};

namespace impl
{
// Target CTraitedSetting, but it's a template and cannot be checked directly with derived_from.
// It's derived from CSetting and we need the Value alias - which also identifies CTraitedSetting
// and its descendants.
template<typename T>
concept TraitedSetting = requires { typename T::Value; } && std::derived_from<T, CSetting>;

// Constrains Fn to be callable with a FromT::Value argument and to return
// std::pair<ToT::Value, ToT::Value> (converted value + default value).
template<typename Fn, typename FromT, typename ToT>
concept SettingConverter =
    TraitedSetting<FromT> && TraitedSetting<ToT> && std::invocable<Fn, typename FromT::Value> &&
    std::same_as<std::invoke_result_t<Fn, typename FromT::Value>,
                 std::pair<typename ToT::Value, typename ToT::Value>>;

/*!
 * \brief Migrates a single setting from one type and ID to another, applying the supplied
 *        value conversion.
 *
 * Looks up \p oldSettingId in the XML tree \p root, parses its value as setting of type \p FromT
 * (for example CSettingBool), passes that value to \p converter, and writes the result as a
 * setting of type \p ToT identified by \p newSettingId. The old node is removed on success.
 *
 * \tparam FromT  A CTraitedSetting-derived type representing the old setting.
 *                Must satisfy the \c TraitedSetting concept.
 * \tparam ToT    A CTraitedSetting-derived type representing the new setting.
 *                Must satisfy the \c TraitedSetting concept.
 * \tparam ConvertFn  A callable type satisfying the \c SettingConverter concept for \p FromT and
 *                    \p ToT.
 *
 * \param[in,out] root          Root element of the settings XML document. Must not be \c nullptr.
 * \param[in] oldSettingId  Dot-separated ID of the setting to migrate from.
 * \param[in] newSettingId  Dot-separated ID of the setting to migrate to.
 * \param[in] converter     Callable that converts the setting value from a \p FromT::Value to a
 *                      `std::pair<ToT::Value, ToT::Value>` (resp. converted value, default value).
 * \return Success status, see \ref SettingConversionResult
 *
 * \note Callers should prefer type-wrapped functions such as ConvertSettingBoolToInt().
 *
 */
template<TraitedSetting FromT, TraitedSetting ToT, SettingConverter<FromT, ToT> ConvertFn>
SettingConversionResult ConvertSingleSetting(TiXmlElement* root,
                                             std::string_view oldSettingId,
                                             std::string_view newSettingId,
                                             ConvertFn converter)
{
  if (TiXmlElement* elem = CSettingsManager::LocateSetting(root, oldSettingId); elem != nullptr)
  {
    //! maybe future @todo: strategy - read/write settings without dependency on CSetting* classes?
    try
    {
      auto oldSetting = std::make_shared<FromT>(oldSettingId, nullptr);

      const std::string oldValue = elem->FirstChild() ? elem->FirstChild()->ValueStr() : "";

      if (!oldSetting->FromString(oldValue))
      {
        CLog::Log(LOGWARNING,
                  "Settings conversion: unable to load the value of the old "
                  "setting \"{}\": \"{}\". "
                  "The new setting \"{}\" will have its default value.",
                  oldSettingId, oldValue, newSettingId);
        return SettingConversionResult::INVALID;
      }

      auto [newValue, defaultValue] = std::invoke(converter, oldSetting->GetValue());

      // Prepare a new setting
      auto newSetting = std::make_shared<ToT>(newSettingId, nullptr);

      newSetting->SetDefault(defaultValue);
      newSetting->SetValue(newValue);

      // Add the new setting and remove the old one
      // The new setting doesn't have to be in the same place in the file as the old one.
      if (!CSettingsValueXmlSerializer::SerializeSetting(root, newSetting) ||
          !XMLUtils::RemoveNode(elem))
        return SettingConversionResult::FAILURE;

      CLog::LogF(LOGDEBUG,
                 "Successful conversion of old setting \"{}\" / \"{}\" to new "
                 "setting \"{}\" / \"{}\".",
                 oldSettingId, oldValue, newSettingId, newValue);
      return SettingConversionResult::CONVERTED;
    }
    catch (...)
    {
      return SettingConversionResult::FAILURE;
    }
  }

  CLog::LogF(LOGDEBUG,
             "Old setting \"{}\" not found. The new setting \"{}\" will have "
             "its default value.",
             oldSettingId, newSettingId);

  return SettingConversionResult::NOT_PRESENT;
}
} // namespace impl

struct SettingBoolToIntMapping
{
  int m_default;
  int m_false;
  int m_true;
};

SettingConversionResult ConvertSettingBoolToInt(TiXmlElement* root,
                                                std::string_view oldSettingId,
                                                std::string_view newSettingId,
                                                const SettingBoolToIntMapping& mapping);

} // namespace KODI::SETTINGS
