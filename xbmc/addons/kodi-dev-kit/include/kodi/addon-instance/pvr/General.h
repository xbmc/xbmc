/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../../AddonBase.h"
#include "../../c-api/addon-instance/pvr/pvr_general.h"

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// "C++" Definitions group 1 - General PVR
#ifdef __cplusplus

namespace kodi
{
namespace addon
{

//==============================================================================
/// @defgroup cpp_kodi_addon_pvr_Defs_PVRTypeIntValue class PVRTypeIntValue
/// @ingroup cpp_kodi_addon_pvr_Defs_General
/// @brief **PVR add-on int type value**\n
/// Representation of a <b>`<int, std::string>`</b> value.
///
/// ----------------------------------------------------------------------------
///
/// @copydetails cpp_kodi_addon_pvr_Defs_PVRTypeIntValue_Help
///
///@{
class PVRTypeIntValue : public DynamicCStructHdl<PVRTypeIntValue, PVR_ATTRIBUTE_INT_VALUE>
{
  friend class CInstancePVRClient;

public:
  /*! \cond PRIVATE */
  PVRTypeIntValue(const PVRTypeIntValue& data) : DynamicCStructHdl(data) {}
  /*! \endcond */

  /// @defgroup cpp_kodi_addon_pvr_Defs_PVRTypeIntValue_Help Value Help
  /// @ingroup cpp_kodi_addon_pvr_Defs_PVRTypeIntValue
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_addon_pvr_Defs_PVRTypeIntValue :</b>
  /// | Name | Type | Set call | Get call
  /// |------|------|----------|----------
  /// | **Value** | `int` | @ref PVRTypeIntValue::SetValue "SetValue" | @ref PVRTypeIntValue::GetValue "GetValue"
  /// | **Description** | `std::string` | @ref PVRTypeIntValue::SetDescription "SetDescription" | @ref PVRTypeIntValue::GetDescription "GetDescription"
  ///
  /// @remark Further can there be used his class constructor to set values.

  /// @addtogroup cpp_kodi_addon_pvr_Defs_PVRTypeIntValue
  ///@{

  /// @brief Default class constructor.
  PVRTypeIntValue() = default;

  /// @brief Class constructor with integrated value set.
  ///
  /// @param[in] value Type identification value
  /// @param[in] description Type description text
  PVRTypeIntValue(int value, const std::string& description)
  {
    SetValue(value);
    SetDescription(description);
  }

  /// @brief To set with the identification value.
  void SetValue(int value) { m_cStructure->iValue = value; }

  /// @brief To get with the identification value.
  int GetValue() const { return m_cStructure->iValue; }

  /// @brief To set with the description text of the value.
  void SetDescription(const std::string& description)
  {
    ReallocAndCopyString(&m_cStructure->strDescription, description.c_str());
  }

  /// @brief To get with the description text of the value.
  std::string GetDescription() const
  {
    return m_cStructure->strDescription ? m_cStructure->strDescription : "";
  }
  ///@}

  static PVR_ATTRIBUTE_INT_VALUE* AllocAndCopyData(const std::vector<PVRTypeIntValue>& source)
  {
    PVR_ATTRIBUTE_INT_VALUE* values = new PVR_ATTRIBUTE_INT_VALUE[source.size()]{};
    for (unsigned int i = 0; i < source.size(); ++i)
    {
      values[i].iValue = source[i].GetCStructure()->iValue;
      AllocResources(source[i].GetCStructure(), &values[i]); // handles strDescription
    }
    return values;
  }

  static PVR_ATTRIBUTE_INT_VALUE* AllocAndCopyData(const PVR_ATTRIBUTE_INT_VALUE* source,
                                                   unsigned int size)
  {
    PVR_ATTRIBUTE_INT_VALUE* values = new PVR_ATTRIBUTE_INT_VALUE[size]{};
    for (unsigned int i = 0; i < size; ++i)
    {
      values[i].iValue = source[i].iValue;
      AllocResources(&source[i], &values[i]); // handles strDescription
    }
    return values;
  }

  static void AllocResources(const PVR_ATTRIBUTE_INT_VALUE* source, PVR_ATTRIBUTE_INT_VALUE* target)
  {
    target->strDescription = AllocAndCopyString(source->strDescription);
  }

  static void FreeResources(PVR_ATTRIBUTE_INT_VALUE* target)
  {
    FreeString(target->strDescription);
    target->strDescription = nullptr;
  }

  static void FreeResources(PVR_ATTRIBUTE_INT_VALUE* values, unsigned int size)
  {
    for (unsigned int i = 0; i < size; ++i)
    {
      FreeResources(&values[i]);
    }
    delete[] values;
  }

  static void ReallocAndCopyData(PVR_ATTRIBUTE_INT_VALUE** source,
                                 unsigned int* size,
                                 const std::vector<PVRTypeIntValue>& values)
  {
    FreeResources(*source, *size);
    *source = nullptr;
    *size = static_cast<unsigned int>(values.size());
    if (*size)
      *source = AllocAndCopyData(values);
  }

private:
  PVRTypeIntValue(const PVR_ATTRIBUTE_INT_VALUE* data) : DynamicCStructHdl(data) {}
  PVRTypeIntValue(PVR_ATTRIBUTE_INT_VALUE* data) : DynamicCStructHdl(data) {}
};
///@}
//------------------------------------------------------------------------------

//==============================================================================
/// @defgroup cpp_kodi_addon_pvr_Defs_PVRTypeStringValue class PVRTypeStringValue
/// @ingroup cpp_kodi_addon_pvr_Defs_General
/// @brief **PVR add-on string type value**\n
/// Representation of a <b>`<std::string, std::string>`</b> value.
///
/// ----------------------------------------------------------------------------
///
/// @copydetails cpp_kodi_addon_pvr_Defs_PVRTypeStringValue_Help
///
///@{
class PVRTypeStringValue : public DynamicCStructHdl<PVRTypeStringValue, PVR_ATTRIBUTE_STRING_VALUE>
{
  friend class CInstancePVRClient;

public:
  /*! \cond PRIVATE */
  PVRTypeStringValue(const PVRTypeStringValue& data) : DynamicCStructHdl(data) {}
  /*! \endcond */

  /// @defgroup cpp_kodi_addon_pvr_Defs_PVRTypeStringValue_Help Value Help
  /// @ingroup cpp_kodi_addon_pvr_Defs_PVRTypeStringValue
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_addon_pvr_Defs_PVRTypeStringValue :</b>
  /// | Name | Type | Set call | Get call
  /// |------|------|----------|----------
  /// | **Value** | `std::string` | @ref PVRTypeStringValue::SetValue "SetValue" | @ref PVRTypeStringValue::GetValue "GetValue"
  /// | **Description** | `std::string` | @ref PVRTypeStringValue::SetDescription "SetDescription" | @ref PVRTypeStringValue::GetDescription "GetDescription"
  ///
  /// @remark Further can there be used his class constructor to set values.

  /// @addtogroup cpp_kodi_addon_pvr_Defs_PVRTypeStringValue
  ///@{

  /// @brief Default class constructor.
  PVRTypeStringValue() = default;

  /// @brief Class constructor with integrated value set.
  ///
  /// @param[in] value Type identification value
  /// @param[in] description Type description text
  PVRTypeStringValue(const std::string& value, const std::string& description)
  {
    SetValue(value);
    SetDescription(description);
  }

  /// @brief To set with the identification value.
  void SetValue(const std::string& value)
  {
    ReallocAndCopyString(&m_cStructure->strValue, value.c_str());
  }

  /// @brief To get with the identification value.
  std::string GetValue() const { return m_cStructure->strValue ? m_cStructure->strValue : ""; }

  /// @brief To set with the description text of the value.
  void SetDescription(const std::string& description)
  {
    ReallocAndCopyString(&m_cStructure->strDescription, description.c_str());
  }

  /// @brief To get with the description text of the value.
  std::string GetDescription() const
  {
    return m_cStructure->strDescription ? m_cStructure->strDescription : "";
  }
  ///@}

  static PVR_ATTRIBUTE_STRING_VALUE* AllocAndCopyData(const std::vector<PVRTypeStringValue>& source)
  {
    PVR_ATTRIBUTE_STRING_VALUE* values = new PVR_ATTRIBUTE_STRING_VALUE[source.size()]{};
    for (unsigned int i = 0; i < source.size(); ++i)
      AllocResources(source[i].GetCStructure(), &values[i]); // handles strValue, strDescription
    return values;
  }

  static PVR_ATTRIBUTE_STRING_VALUE* AllocAndCopyData(const PVR_ATTRIBUTE_STRING_VALUE* source,
                                                      unsigned int size)
  {
    PVR_ATTRIBUTE_STRING_VALUE* values = new PVR_ATTRIBUTE_STRING_VALUE[size]{};
    for (unsigned int i = 0; i < size; ++i)
      AllocResources(&source[i], &values[i]); // handles strValue, strDescription
    return values;
  }

  static void AllocResources(const PVR_ATTRIBUTE_STRING_VALUE* source,
                             PVR_ATTRIBUTE_STRING_VALUE* target)
  {
    target->strValue = AllocAndCopyString(source->strValue);
    target->strDescription = AllocAndCopyString(source->strDescription);
  }

  static void FreeResources(PVR_ATTRIBUTE_STRING_VALUE* target)
  {
    FreeString(target->strValue);
    target->strValue = nullptr;
    FreeString(target->strDescription);
    target->strDescription = nullptr;
  }

  static void FreeResources(PVR_ATTRIBUTE_STRING_VALUE* values, unsigned int size)
  {
    for (unsigned int i = 0; i < size; ++i)
      FreeResources(&values[i]);
    delete[] values;
  }

  static void ReallocAndCopyData(PVR_ATTRIBUTE_STRING_VALUE** source,
                                 unsigned int* size,
                                 const std::vector<PVRTypeStringValue>& values)
  {
    FreeResources(*source, *size);
    *source = nullptr;
    *size = static_cast<unsigned int>(values.size());
    if (*size)
      *source = AllocAndCopyData(values);
  }

private:
  PVRTypeStringValue(const PVR_ATTRIBUTE_STRING_VALUE* data) : DynamicCStructHdl(data) {}
  PVRTypeStringValue(PVR_ATTRIBUTE_STRING_VALUE* data) : DynamicCStructHdl(data) {}
};
///@}
//------------------------------------------------------------------------------

//==============================================================================
/// @defgroup cpp_kodi_addon_pvr_Defs_PVRIntSettingDefinition class PVRIntSettingDefinition
/// @ingroup cpp_kodi_addon_pvr_Defs_General
/// @brief **PVR add-on integer setting definition**\n
/// Representation of an integer setting definition.
///
/// ----------------------------------------------------------------------------
///
/// @copydetails cpp_kodi_addon_pvr_Defs_PVRIntSettingDefinition_Help
///
///@{
class PVRIntSettingDefinition
  : public DynamicCStructHdl<PVRIntSettingDefinition, PVR_INT_SETTING_DEFINITION>
{
  friend class CInstancePVRClient;

public:
  /*! \cond PRIVATE */
  PVRIntSettingDefinition() { m_cStructure->iStep = 1; }
  PVRIntSettingDefinition(const PVRIntSettingDefinition& def) : DynamicCStructHdl(def) {}
  /*! \endcond */

  /// @defgroup cpp_kodi_addon_pvr_Defs_General_PVRIntSettingDefinition_Help Value Help
  /// @ingroup cpp_kodi_addon_pvr_Defs_General_PVRIntSettingDefinition
  /// ----------------------------------------------------------------------------
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_addon_pvr_Defs_General_PVRIntSettingDefinition :</b>
  /// | Name | Type | Set call | Get call | Usage
  /// |------|------|----------|----------|-----------
  /// | **Values** | @ref cpp_kodi_addon_pvr_Defs_PVRTypeIntValue "PVRTypeIntValue" | @ref PVRIntSettingDefinition::SetValues "SetValues" | @ref PVRIntSettingDefinition:GetValues "GetValues" | *optional*
  /// | **Default value** | `int`| @ref PVRIntSettingDefinition::SetDefaultValue "SetDefaultValue" | @ref PVRIntSettingDefinition::GetDefaultValue "GetDefaultValue" | *optional*
  /// | **Min value** | `int`| @ref PVRIntSettingDefinition::SetMinValue "SetMinValue" | @ref PVRIntSettingDefinition::GetMinValue "GetMinValue" | *optional*
  /// | **Step** | `int`| @ref PVRIntSettingDefinition::SetStep "SetStep" | @ref PVRIntSettingDefinition::GetStep "GetStep" | *optional*
  /// | **Max value** | `int`| @ref PVRIntSettingDefinition::SetMaxValue "SetMaxValue" | @ref PVRIntSettingDefinition::GetMaxValue "GetMaxValue" | *optional*
  ///

  /// @addtogroup cpp_kodi_addon_pvr_Defs_PVRIntSettingDefinition
  ///@{

  /// @brief Class constructor with integrated values.
  ///
  /// @param[in] settingValues possible setting values
  /// @param[in] defaultValue default setting value
  /// @param[in] minValue minimum setting value
  /// @param[in] step amount to change values from min to max
  /// @param[in] maxValue maximum setting value
  PVRIntSettingDefinition(const std::vector<PVRTypeIntValue>& settingValues,
                          int defaultValue,
                          int minValue,
                          int step,
                          int maxValue)
  {
    SetValues(settingValues);
    SetDefaultValue(defaultValue);
    SetMinValue(minValue);
    SetStep(step);
    SetMaxValue(maxValue);
  }

  /// @brief **optional**\n
  /// value definitions.
  ///
  /// Array containing the possible settings values. If left blank, any int value is accepted.
  ///
  /// @param[in] values List of possible values
  /// @param[in] defaultValue [opt] The default value in list, can also be
  ///                               set by @ref SetDefaultValue()
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_pvr_Defs_PVRTypeIntValue_Help
  void SetValues(const std::vector<PVRTypeIntValue>& values, int defaultValue = -1)
  {
    PVRTypeIntValue::ReallocAndCopyData(&m_cStructure->values, &m_cStructure->iValuesSize, values);
    if (defaultValue != -1)
      m_cStructure->iDefaultValue = defaultValue;
  }

  /// @brief To get with @ref SetValues changed values.
  std::vector<PVRTypeIntValue> GetValues() const
  {
    std::vector<PVRTypeIntValue> ret;
    for (unsigned int i = 0; i < m_cStructure->iValuesSize; ++i)
      ret.emplace_back(m_cStructure->values[i].iValue, m_cStructure->values[i].strDescription);
    return ret;
  }

  /// @brief **optional**\n
  /// The default value for this setting.
  void SetDefaultValue(int defaultValue) { m_cStructure->iDefaultValue = defaultValue; }

  /// @brief To get with @ref SetDefaultValue changed values.
  int GetDefaultValue() const { return m_cStructure->iDefaultValue; }

  /// @brief **optional**\n
  /// The minimum value for this setting.
  void SetMinValue(int minValue) { m_cStructure->iMinValue = minValue; }

  /// @brief To get with @ref SetMinValue changed values.
  int GetMinValue() const { return m_cStructure->iMinValue; }

  /// @brief **optional**\n
  /// The amount for increasing the values for this setting from min to max.
  void SetStep(int step) { m_cStructure->iStep = step; }

  /// @brief To get with @ref SetStep changed values.
  int GetStep() const { return m_cStructure->iStep; }

  /// @brief **optional**\n
  /// The maximum value for this setting.
  void SetMaxValue(int maxValue) { m_cStructure->iMaxValue = maxValue; }

  /// @brief To get with @ref SetMaxValue changed values.
  int GetMaxValue() const { return m_cStructure->iMaxValue; }
  ///@}

  static PVR_INT_SETTING_DEFINITION* AllocAndCopyData(const PVRIntSettingDefinition& source)
  {
    PVR_INT_SETTING_DEFINITION* def = new PVR_INT_SETTING_DEFINITION{};
    AllocResources(source.GetCStructure(), def); // handles values, iValuesSize
    def->iDefaultValue = source.GetCStructure()->iDefaultValue;
    def->iMinValue = source.GetCStructure()->iMinValue;
    def->iStep = source.GetCStructure()->iStep;
    def->iMaxValue = source.GetCStructure()->iMaxValue;
    return def;
  }

  static PVR_INT_SETTING_DEFINITION* AllocAndCopyData(PVR_INT_SETTING_DEFINITION* source)
  {
    PVR_INT_SETTING_DEFINITION* def = new PVR_INT_SETTING_DEFINITION{};
    AllocResources(source, def); // handles values, iValuesSize
    def->iDefaultValue = source->iDefaultValue;
    def->iMinValue = source->iMinValue;
    def->iStep = source->iStep;
    def->iMaxValue = source->iMaxValue;
    return def;
  }

  static void AllocResources(const PVR_INT_SETTING_DEFINITION* source,
                             PVR_INT_SETTING_DEFINITION* target)
  {
    target->values = PVRTypeIntValue::AllocAndCopyData(source->values, source->iValuesSize);
    target->iValuesSize = source->iValuesSize;
  }

  static void FreeResources(PVR_INT_SETTING_DEFINITION* target)
  {
    PVRTypeIntValue::FreeResources(target->values, target->iValuesSize);
    target->values = nullptr;
    target->iValuesSize = 0;
  }

  static void ReallocAndCopyData(PVR_INT_SETTING_DEFINITION** source,
                                 const PVRIntSettingDefinition& def)
  {
    if (*source)
      FreeResources(*source);
    *source = AllocAndCopyData(def);
  }

private:
  PVRIntSettingDefinition(const PVR_INT_SETTING_DEFINITION* def) : DynamicCStructHdl(def) {}
  PVRIntSettingDefinition(PVR_INT_SETTING_DEFINITION* def) : DynamicCStructHdl(def) {}
};
///@}
//------------------------------------------------------------------------------

//==============================================================================
/// @defgroup cpp_kodi_addon_pvr_Defs_PVRStringSettingDefinition class PVRStringSettingDefinition
/// @ingroup cpp_kodi_addon_pvr_Defs_General
/// @brief **PVR add-on string setting definition**\n
/// Representation of a string setting definition.
///
/// ----------------------------------------------------------------------------
///
/// @copydetails cpp_kodi_addon_pvr_Defs_PVRStringSettingDefinition_Help
///
///@{
class PVRStringSettingDefinition
  : public DynamicCStructHdl<PVRStringSettingDefinition, PVR_STRING_SETTING_DEFINITION>
{
  friend class CInstancePVRClient;

public:
  /*! \cond PRIVATE */
  PVRStringSettingDefinition() { m_cStructure->bAllowEmptyValue = true; }
  PVRStringSettingDefinition(const PVRStringSettingDefinition& def) : DynamicCStructHdl(def) {}
  /*! \endcond */

  /// @defgroup cpp_kodi_addon_pvr_Defs_General_PVRStringSettingDefinition_Help Value Help
  /// @ingroup cpp_kodi_addon_pvr_Defs_General_PVRStringSettingDefinition
  /// ----------------------------------------------------------------------------
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_addon_pvr_Defs_General_PVRStringSettingDefinition :</b>
  /// | Name | Type | Set call | Get call | Usage
  /// |------|------|----------|----------|-----------
  /// | **Values** | @ref cpp_kodi_addon_pvr_Defs_PVRTypeStringValue "PVRTypeStringValue" | @ref PVRStringSettingDefinition::SetValues "SetValues" | @ref PVRStringSettingDefinition:GetValues "GetValues" | *optional*
  /// | **Default value** | `std::string`| @ref PVRStringSettingDefinition::SetDefaultValue "SetDefaultValue" | @ref PVRStringSettingDefinition::GetDefaultValue "GetDefaultValue" | *optional*
  /// | **Allow empty value** | `bool`| @ref PVRStringSettingDefinition::SetAllowEmptyValue "SetAllowEmptyValue" | @ref PVRStringSettingDefinition::GetetAllowEmptyValue "GetetAllowEmptyValue" | *optional*
  ///

  /// @addtogroup cpp_kodi_addon_pvr_Defs_PVRStringSettingDefinition
  ///@{

  /// @brief Class constructor with integrated values.
  ///
  /// @param[in] settingValues possible setting values
  /// @param[in] defaultValue default setting value
  /// @param[in] allowEmptyValues allow empty values flag
  PVRStringSettingDefinition(const std::vector<PVRTypeStringValue>& settingValues,
                             const std::string& defaultValue,
                             bool allowEmptyValue)
  {
    SetValues(settingValues);
    SetDefaultValue(defaultValue);
    SetAllowEmptyValue(allowEmptyValue);
  }

  /// @brief **optional**\n
  /// value definitions.
  ///
  /// Array containing the possible settings values. If left blank, any string value is accepted.
  ///
  /// @param[in] values List of possible values
  /// @param[in] defaultValue [opt] The default value in list, can also be
  ///                               set by @ref SetDefaultValue()
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_pvr_Defs_PVRTypeStringValue_Help
  void SetValues(const std::vector<PVRTypeStringValue>& values,
                 const std::string& defaultValue = "")
  {
    PVRTypeStringValue::ReallocAndCopyData(&m_cStructure->values, &m_cStructure->iValuesSize,
                                           values);
    ReallocAndCopyString(&m_cStructure->strDefaultValue, defaultValue.c_str());
  }

  /// @brief To get with @ref SetValues changed values.
  std::vector<PVRTypeStringValue> GetValues() const
  {
    std::vector<PVRTypeStringValue> ret;
    for (unsigned int i = 0; i < m_cStructure->iValuesSize; ++i)
      ret.emplace_back(m_cStructure->values[i].strValue, m_cStructure->values[i].strDescription);
    return ret;
  }

  /// @brief **optional**\n
  /// The default value for this setting.
  void SetDefaultValue(const std::string& defaultValue)
  {
    ReallocAndCopyString(&m_cStructure->strDefaultValue, defaultValue.c_str());
  }

  /// @brief To get with @ref SetDefaultValue changed values.
  std::string GetDefaultValue() const
  {
    return m_cStructure->strDefaultValue ? m_cStructure->strDefaultValue : "";
  }

  /// @brief **optional**\n
  /// The allow empty values flag for this setting.
  void SetAllowEmptyValue(bool allowEmptyValue)
  {
    m_cStructure->bAllowEmptyValue = allowEmptyValue;
  }

  /// @brief To get with @ref SetAllowEmptyValue changed values.
  bool GetAllowEmptyValue() const { return m_cStructure->bAllowEmptyValue; }
  ///@}

  static PVR_STRING_SETTING_DEFINITION* AllocAndCopyData(const PVRStringSettingDefinition& source)
  {
    PVR_STRING_SETTING_DEFINITION* def = new PVR_STRING_SETTING_DEFINITION{};
    AllocResources(source.GetCStructure(), def); // handles strDefaultValue, values, iValuesSize
    def->bAllowEmptyValue = source.GetCStructure()->bAllowEmptyValue;
    return def;
  }

  static PVR_STRING_SETTING_DEFINITION* AllocAndCopyData(PVR_STRING_SETTING_DEFINITION* source)
  {
    PVR_STRING_SETTING_DEFINITION* def = new PVR_STRING_SETTING_DEFINITION{};
    AllocResources(source, def); // handles strDefaultValue, values, iValuesSize
    def->bAllowEmptyValue = source->bAllowEmptyValue;
    return def;
  }

  static void AllocResources(const PVR_STRING_SETTING_DEFINITION* source,
                             PVR_STRING_SETTING_DEFINITION* target)
  {
    target->strDefaultValue = AllocAndCopyString(source->strDefaultValue);
    target->values = PVRTypeStringValue::AllocAndCopyData(source->values, source->iValuesSize);
    target->iValuesSize = source->iValuesSize;
  }

  static void FreeResources(PVR_STRING_SETTING_DEFINITION* target)
  {
    PVRTypeStringValue::FreeResources(target->values, target->iValuesSize);
    target->values = nullptr;
    target->iValuesSize = 0;

    FreeString(target->strDefaultValue);
    target->strDefaultValue = nullptr;
  }

  static void ReallocAndCopyData(PVR_STRING_SETTING_DEFINITION** source,
                                 const PVRStringSettingDefinition& def)
  {
    if (*source)
      FreeResources(*source);
    *source = AllocAndCopyData(def);
  }

private:
  PVRStringSettingDefinition(const PVR_STRING_SETTING_DEFINITION* def) : DynamicCStructHdl(def) {}
  PVRStringSettingDefinition(PVR_STRING_SETTING_DEFINITION* def) : DynamicCStructHdl(def) {}
};
///@}
//------------------------------------------------------------------------------

//==============================================================================
/// @defgroup cpp_kodi_addon_pvr_Defs_PVRSettingDefinition class PVRSettingDefinition
/// @ingroup cpp_kodi_addon_pvr_Defs_General
/// @brief **PVR add-on setting definition**\n
/// Representation of a setting definition.
///
/// ----------------------------------------------------------------------------
///
/// @copydetails cpp_kodi_addon_pvr_Defs_PVRSettingDefinition_Help
///
///@{
class PVRSettingDefinition : public DynamicCStructHdl<PVRSettingDefinition, PVR_SETTING_DEFINITION>
{
  friend class CInstancePVRClient;

public:
  /*! \cond PRIVATE */
  PVRSettingDefinition() = default;
  PVRSettingDefinition(const PVRSettingDefinition& type) : DynamicCStructHdl(type) {}
  /*! \endcond */

  /// @defgroup cpp_kodi_addon_pvr_Defs_General_PVRSettingDefinition_Help Value Help
  /// @ingroup cpp_kodi_addon_pvr_Defs_General_PVRSettingDefinition
  /// ----------------------------------------------------------------------------
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_addon_pvr_Defs_General_PVRSettingDefinition :</b>
  /// | Name | Type | Set call | Get call | Usage
  /// |------|------|----------|----------|-----------
  /// | **Identifier** | `unsigned int` | @ref PVRSettingDefinition::SetId "SetId" | @ref PVRSettingDefinition::GetId "GetId" | *required to set*
  /// | **Name** | `std::string` | @ref PVRSettingDefinition::SetName "SetName" | @ref PVRSettingDefinition::GetName "GetName" | *required to set*
  /// | **Type** | @ref PVR_SETTING_TYPE | @ref PVRSettingDefinition::SetType "SetType" | @ref PVRSettingDefinition:GetType "GetType" | *required to set*
  /// | | | | | |
  /// | **Read-only conditions** | `uint64_t`| @ref PVRSettingDefinition::SetReadonlyConditions "SetReadonlyConditions" | @ref PVRSettingDefinition::GetReadonlyConditions "GetReadonlyConditions" | *optional*
  /// | | | | | |
  /// | **Int Definition** | @ref cpp_kodi_addon_pvr_Defs_PVRIntSettingDefinition "PVRIntSettingDefinition" | @ref PVRSettingDefinition::SetIntDefinition "SetIntDefinition" | @ref PVRSettingDefinition:GetIntDefinition "GetIntDefinition" | *optional*
  /// | **String Definition** | @ref cpp_kodi_addon_pvr_Defs_PVRStringSettingDefinition "PVRStringSettingDefinition" | @ref PVRSettingDefinition::SetStringValues "SetStringDefinition" | @ref PVRSettingDefinition:GetStringDefinition "GetStringDefinition" | *optional*
  ///

  /// @addtogroup cpp_kodi_addon_pvr_Defs_PVRSettingDefinition
  ///@{

  /// @brief Class constructor with integrated values.
  ///
  /// @param[in] settingDefId Setting definition identification value
  /// @param[in] settingDefName Setting definition name
  /// @param[in] readonlyConditions readonly conditions value
  /// @param[in] settingDef int setting definition
  PVRSettingDefinition(unsigned int settingDefId,
                       const std::string& settingDefName,
                       uint64_t readonlyConditions,
                       const PVRIntSettingDefinition& settingDef)
  {
    SetId(settingDefId);
    SetName(settingDefName);
    SetType(PVR_SETTING_TYPE::INTEGER);
    SetReadonlyConditions(readonlyConditions);
    SetIntDefinition(settingDef);
  }

  /// @brief Class constructor with integrated values.
  ///
  /// @param[in] settingDefId Setting definition identification value
  /// @param[in] settingDefName Setting definition name
  /// @param[in] readonlyConditions readonly conditions value
  /// @param[in] settingDef string setting definition
  PVRSettingDefinition(unsigned int settingDefId,
                       const std::string& settingDefName,
                       uint64_t readonlyConditions,
                       const PVRStringSettingDefinition& settingDef)
  {
    SetId(settingDefId);
    SetName(settingDefName);
    SetType(PVR_SETTING_TYPE::STRING);
    SetReadonlyConditions(readonlyConditions);
    SetStringDefinition(settingDef);
  }

  /// @brief Class constructor with integrated values.
  ///
  /// @param[in] settingDefId Setting definition identification value
  /// @param[in] settingDefName Setting definition name
  /// @param[in] eType Setting type
  /// @param[in] readonlyConditions readonly conditions value
  /// @param[in] intSettingDef int setting definition
  /// @param[in] stringSettingDef string setting definition
  PVRSettingDefinition(unsigned int settingDefId,
                       const std::string& settingDefName,
                       PVR_SETTING_TYPE eType,
                       uint64_t readonlyConditions,
                       const PVRIntSettingDefinition& intSettingDef,
                       const PVRStringSettingDefinition& stringSettingDef)
  {
    SetId(settingDefId);
    SetName(settingDefName);
    SetType(eType);
    SetReadonlyConditions(readonlyConditions);
    SetIntDefinition(intSettingDef);
    SetStringDefinition(stringSettingDef);
  }

  /// @brief **required**\n
  /// This setting definition's identifier.
  void SetId(unsigned int defId) { m_cStructure->iId = defId; }

  /// @brief To get with @ref SetId changed values.
  unsigned int GetId() const { return m_cStructure->iId; }

  /// @brief **required**\n
  /// A short localized string with the name of the setting.
  void SetName(const std::string& name)
  {
    ReallocAndCopyString(&m_cStructure->strName, name.c_str());
  }

  /// @brief To get with @ref SetName changed values.
  std::string GetName() const { return m_cStructure->strName ? m_cStructure->strName : ""; }

  /// @brief **required**\n
  /// This setting definition's identifier.
  void SetType(PVR_SETTING_TYPE eType) { m_cStructure->eType = eType; }

  /// @brief To get with @ref SetType changed values.
  PVR_SETTING_TYPE GetType() const { return m_cStructure->eType; }

  /// @brief **optional**\n
  /// The read-only conditions value for this setting.
  /// @ref cpp_kodi_addon_pvr_Defs_General_PVR_SETTING_READONLY_CONDITION "PVR_SETTING_READONLY_CONDITION_*" enum values
  void SetReadonlyConditions(uint64_t conditions)
  {
    m_cStructure->iReadonlyConditions = conditions;
  }

  /// @brief To get with @ref SetReadonlyConditions changed values.
  uint64_t GetReadonlyConditions() const { return m_cStructure->iReadonlyConditions; }

  //----------------------------------------------------------------------------

  /// @brief **optional**\n
  /// @param[in] def integer setting definition
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_pvr_Defs_PVRIntSettingDefinition_Help
  void SetIntDefinition(const PVRIntSettingDefinition& def)
  {
    PVRIntSettingDefinition::ReallocAndCopyData(&m_cStructure->intSettingDefinition, def);
  }

  /// @brief To get with @ref SetIntDefinition changed values.
  PVRIntSettingDefinition GetIntDefinition() const
  {
    PVRIntSettingDefinition ret;
    if (m_cStructure->intSettingDefinition)
    {
      std::vector<PVRTypeIntValue> settingValues;
      settingValues.reserve(m_cStructure->intSettingDefinition->iValuesSize);
      for (unsigned int i = 0; i < m_cStructure->intSettingDefinition->iValuesSize; ++i)
      {
        settingValues.emplace_back(m_cStructure->intSettingDefinition->values[i].iValue,
                                   m_cStructure->intSettingDefinition->values[i].strDescription);
      }
      ret.SetValues(std::move(settingValues));
      ret.SetDefaultValue(m_cStructure->intSettingDefinition->iDefaultValue);
      ret.SetMinValue(m_cStructure->intSettingDefinition->iMinValue);
      ret.SetStep(m_cStructure->intSettingDefinition->iStep);
      ret.SetMaxValue(m_cStructure->intSettingDefinition->iMaxValue);
    }
    return ret;
  }

  //----------------------------------------------------------------------------

  /// @brief **optional**\n
  /// @param[in] def string setting definition
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_pvr_Defs_PVRStringSettingDefinition_Help
  void SetStringDefinition(const PVRStringSettingDefinition& def)
  {
    PVRStringSettingDefinition::ReallocAndCopyData(&m_cStructure->stringSettingDefinition, def);
  }

  /// @brief To get with @ref SetStringDefinition changed values.
  PVRStringSettingDefinition GetStringDefinition() const
  {
    PVRStringSettingDefinition ret;
    if (m_cStructure->stringSettingDefinition)
    {
      std::vector<PVRTypeStringValue> settingValues;
      settingValues.reserve(m_cStructure->stringSettingDefinition->iValuesSize);
      for (unsigned int i = 0; i < m_cStructure->stringSettingDefinition->iValuesSize; ++i)
      {
        settingValues.emplace_back(m_cStructure->stringSettingDefinition->values[i].strValue,
                                   m_cStructure->stringSettingDefinition->values[i].strDescription);
      }
      ret.SetValues(std::move(settingValues));
      ret.SetDefaultValue(m_cStructure->stringSettingDefinition->strDefaultValue);
    }
    return ret;
  }
  ///@}

  static PVR_SETTING_DEFINITION** AllocAndCopyData(const std::vector<PVRSettingDefinition>& source)
  {
    PVR_SETTING_DEFINITION** defs = new PVR_SETTING_DEFINITION* [source.size()] {};
    for (unsigned int i = 0; i < source.size(); ++i)
    {
      defs[i] = new PVR_SETTING_DEFINITION{};
      defs[i]->iId = source[i].GetCStructure()->iId;
      defs[i]->eType = source[i].GetCStructure()->eType;
      defs[i]->iReadonlyConditions = source[i].GetCStructure()->iReadonlyConditions;
      AllocResources(source[i].GetCStructure(),
                     defs[i]); // handles strName, intSettingDefinition, stringSettingDefinition
    }
    return defs;
  }

  static PVR_SETTING_DEFINITION** AllocAndCopyData(PVR_SETTING_DEFINITION** source,
                                                   unsigned int size)
  {
    PVR_SETTING_DEFINITION** defs = new PVR_SETTING_DEFINITION* [size] {};
    for (unsigned int i = 0; i < size; ++i)
    {
      defs[i] = new PVR_SETTING_DEFINITION{};
      defs[i]->iId = source[i]->iId;
      defs[i]->eType = source[i]->eType;
      defs[i]->iReadonlyConditions = source[i]->iReadonlyConditions;
      AllocResources(source[i],
                     defs[i]); // handles strName, intSettingDefinition, stringSettingDefinition
    }
    return defs;
  }

  static void AllocResources(const PVR_SETTING_DEFINITION* source, PVR_SETTING_DEFINITION* target)
  {
    target->strName = AllocAndCopyString(source->strName);
    if (source->intSettingDefinition)
      target->intSettingDefinition =
          PVRIntSettingDefinition::AllocAndCopyData(source->intSettingDefinition);
    if (source->stringSettingDefinition)
      target->stringSettingDefinition =
          PVRStringSettingDefinition::AllocAndCopyData(source->stringSettingDefinition);
  }

  static void FreeResources(PVR_SETTING_DEFINITION* target)
  {
    FreeString(target->strName);
    target->strName = nullptr;

    if (target->intSettingDefinition)
    {
      PVRIntSettingDefinition::FreeResources(target->intSettingDefinition);
      target->intSettingDefinition = nullptr;
    }

    if (target->stringSettingDefinition)
    {
      PVRStringSettingDefinition::FreeResources(target->stringSettingDefinition);
      target->stringSettingDefinition = nullptr;
    }
  }

  static void FreeResources(PVR_SETTING_DEFINITION** defs, unsigned int size)
  {
    for (unsigned int i = 0; i < size; ++i)
    {
      FreeResources(defs[i]);
      delete defs[i];
    }
    delete[] defs;
  }

  static void ReallocAndCopyData(PVR_SETTING_DEFINITION*** source,
                                 unsigned int* size,
                                 const std::vector<PVRSettingDefinition>& defs)
  {
    FreeResources(*source, *size);
    *source = nullptr;
    *size = static_cast<unsigned int>(defs.size());
    if (*size)
      *source = AllocAndCopyData(defs);
  }

private:
  PVRSettingDefinition(const PVR_SETTING_DEFINITION* def) : DynamicCStructHdl(def) {}
  PVRSettingDefinition(PVR_SETTING_DEFINITION* def) : DynamicCStructHdl(def) {}
};
///@}
//------------------------------------------------------------------------------

//==============================================================================
/// @defgroup cpp_kodi_addon_pvr_Defs_General_PVRSettingKeyValuePair class PVRSettingKeyValuePair
/// @ingroup cpp_kodi_addon_pvr_Defs_General
/// @brief **Key-value pair of two ints**\n
/// To hold a pair of two ints.
///
/// ----------------------------------------------------------------------------
///
/// @copydetails cpp_kodi_addon_pvr_Defs_General_PVRSettingKeyValuePair_Help
///
///@{
class PVRSettingKeyValuePair
  : public DynamicCStructHdl<PVRSettingKeyValuePair, PVR_SETTING_KEY_VALUE_PAIR>
{
  friend class CInstancePVRClient;

public:
  /*! \cond PRIVATE */
  PVRSettingKeyValuePair(const PVRSettingKeyValuePair& pair) : DynamicCStructHdl(pair) {}
  /*! \endcond */

  /// @defgroup cpp_kodi_addon_pvr_Defs_General_PVRSettingKeyValuePair_Help Value Help
  /// @ingroup cpp_kodi_addon_pvr_Defs_General_PVRSettingKeyValuePair
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_addon_pvr_Defs_General_PVRSettingKeyValuePair :</b>
  /// | Name | Type | Set call | Get call
  /// |------|------|----------|----------
  /// | **Key** | `unsigned int` | @ref PVRSettingKeyValuePair::SetKey "SetKey" | @ref PVRSettingKeyValuePair::GetKey "GetKey"
  /// | **Type** | @ref PVR_SETTING_TYPE | @ref PVRSettingKeyValuePair::SetType "SetType" | @ref PVRSettingKeyValuePair:GetType "GetType" | *required to set*
  /// | **Int Value** | `int` | @ref PVRSettingKeyValuePair::SetIntValue "SetIntValue" | @ref PVRSettingKeyValuePair::GetIntValue "GetIntValue"
  /// | **String Value** | `std::string` | @ref PVRSettingKeyValuePair::SetStringValue "SetStringValue" | @ref PVRSettingKeyValuePair::GetStringValue "GetStringValue"

  /// @addtogroup cpp_kodi_addon_pvr_Defs_General_PVRSettingKeyValuePair
  ///@{

  /// @brief Default class constructor.
  PVRSettingKeyValuePair() = default;

  /// @brief Class constructor with integrated value set.
  ///
  /// @param[in] key The key
  /// @param[in] value The value
  PVRSettingKeyValuePair(unsigned int key, int value)
  {
    SetKey(key);
    SetType(PVR_SETTING_TYPE::INTEGER);
    SetIntValue(value);
  }

  /// @brief Class constructor with integrated value set.
  ///
  /// @param[in] key The key
  /// @param[in] value The value
  PVRSettingKeyValuePair(unsigned int key, const std::string& value)
  {
    SetKey(key);
    SetType(PVR_SETTING_TYPE::STRING);
    SetStringValue(value);
  }

  /// @brief Class constructor with integrated value set.
  ///
  /// @param[in] key The key
  /// @param[in] eType The type
  /// @param[in] intValue The int value
  /// @param[in] stringValue The string value
  PVRSettingKeyValuePair(unsigned int key,
                         PVR_SETTING_TYPE eType,
                         int intValue,
                         const std::string& stringValue)
  {
    SetKey(key);
    SetType(eType);
    SetIntValue(intValue);
    SetStringValue(stringValue);
  }

  /// @brief To set with the key.
  void SetKey(unsigned int key) { m_cStructure->iKey = key; }

  /// @brief To get with the key.
  unsigned GetKey() const { return m_cStructure->iKey; }

  /// @brief **required**\n
  /// This key value pair's type.
  void SetType(PVR_SETTING_TYPE eType) { m_cStructure->eType = eType; }

  /// @brief To get with @ref SetType changed values.
  PVR_SETTING_TYPE GetType() const { return m_cStructure->eType; }

  /// @brief To set with the value.
  void SetIntValue(int value) { m_cStructure->iValue = value; }

  /// @brief To get with the value.
  int GetIntValue() const { return m_cStructure->iValue; }

  /// @brief To set with the value.
  void SetStringValue(const std::string& value)
  {
    ReallocAndCopyString(&m_cStructure->strValue, value.c_str());
  }

  /// @brief To get with the value.
  std::string GetStringValue() const
  {
    return m_cStructure->strValue ? m_cStructure->strValue : "";
  }
  ///@}

  static PVR_SETTING_KEY_VALUE_PAIR* AllocAndCopyData(
      const std::vector<PVRSettingKeyValuePair>& values)
  {
    PVR_SETTING_KEY_VALUE_PAIR* pairs = new PVR_SETTING_KEY_VALUE_PAIR[values.size()]{};
    for (unsigned int i = 0; i < values.size(); ++i)
    {
      pairs[i].iKey = values[i].GetCStructure()->iKey;
      pairs[i].eType = values[i].GetCStructure()->eType;
      pairs[i].iValue = values[i].GetCStructure()->iValue;
      AllocResources(values[i].GetCStructure(), &pairs[i]); // handles strValue
    }
    return pairs;
  }

  static PVR_SETTING_KEY_VALUE_PAIR* AllocAndCopyData(const PVR_SETTING_KEY_VALUE_PAIR* source,
                                                      unsigned int size)
  {
    PVR_SETTING_KEY_VALUE_PAIR* pairs = new PVR_SETTING_KEY_VALUE_PAIR[size]{};
    for (unsigned int i = 0; i < size; ++i)
    {
      pairs[i].iKey = source[i].iKey;
      pairs[i].eType = source[i].eType;
      pairs[i].iValue = source[i].iValue;
      AllocResources(&source[i], &pairs[i]); // handles strValue
    }
    return pairs;
  }

  static void AllocResources(const PVR_SETTING_KEY_VALUE_PAIR* source,
                             PVR_SETTING_KEY_VALUE_PAIR* target)
  {
    target->strValue = AllocAndCopyString(source->strValue);
  }

  static void FreeResources(PVR_SETTING_KEY_VALUE_PAIR* target) { FreeString(target->strValue); }

  static void FreeResources(PVR_SETTING_KEY_VALUE_PAIR* pairs, unsigned int size)
  {
    for (unsigned int i = 0; i < size; ++i)
      FreeResources(&pairs[i]);
    delete[] pairs;
  }

  static void ReallocAndCopyData(PVR_SETTING_KEY_VALUE_PAIR** source,
                                 unsigned int* size,
                                 const std::vector<PVRSettingKeyValuePair>& values)
  {
    FreeResources(*source, *size);
    *source = nullptr;
    *size = static_cast<unsigned int>(values.size());
    if (*size)
      *source = AllocAndCopyData(values);
  }

private:
  PVRSettingKeyValuePair(const PVR_SETTING_KEY_VALUE_PAIR* pair) : DynamicCStructHdl(pair) {}
  PVRSettingKeyValuePair(PVR_SETTING_KEY_VALUE_PAIR* pair) : DynamicCStructHdl(pair) {}
};
///@}
//------------------------------------------------------------------------------

//==============================================================================
/// @defgroup cpp_kodi_addon_pvr_Defs_PVRCapabilities class PVRCapabilities
/// @ingroup cpp_kodi_addon_pvr_Defs_General
/// @brief **PVR add-on capabilities**\n
/// This class is needed to tell Kodi which options are supported on the addon.
///
/// If a capability is set to **true**, then the corresponding methods from
/// @ref cpp_kodi_addon_pvr "kodi::addon::CInstancePVRClient" need to be
/// implemented.
///
/// As default them all set to **false**.
///
/// Used on @ref kodi::addon::CInstancePVRClient::GetCapabilities().
///
/// ----------------------------------------------------------------------------
///
/// @copydetails cpp_kodi_addon_pvr_Defs_PVRCapabilities_Help
///
///@{
class PVRCapabilities : public DynamicCStructHdl<PVRCapabilities, PVR_ADDON_CAPABILITIES>
{
  friend class CInstancePVRClient;

public:
  /*! \cond PRIVATE */
  PVRCapabilities() = default;
  PVRCapabilities(const PVRCapabilities& capabilities) : DynamicCStructHdl(capabilities) {}
  /*! \endcond */

  /// @defgroup cpp_kodi_addon_pvr_Defs_PVRCapabilities_Help Value Help
  /// @ingroup cpp_kodi_addon_pvr_Defs_PVRCapabilities
  /// ----------------------------------------------------------------------------
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_addon_pvr_Defs_PVRCapabilities :</b>
  /// | Name | Type | Set call | Get call
  /// |------|------|----------|----------
  /// | **Supports EPG** | `boolean` | @ref PVRCapabilities::SetSupportsEPG "SetSupportsEPG" | @ref PVRCapabilities::GetSupportsEPG "GetSupportsEPG"
  /// | **Supports EPG EDL** | `boolean` | @ref PVRCapabilities::SetSupportsEPGEdl "SetSupportsEPGEdl" | @ref PVRCapabilities::GetSupportsEPGEdl "GetSupportsEPGEdl"
  /// | **Supports TV** | `boolean` | @ref PVRCapabilities::SetSupportsTV "SetSupportsTV" | @ref PVRCapabilities::GetSupportsTV "GetSupportsTV"
  /// | **Supports radio** | `boolean` | @ref PVRCapabilities::SetSupportsRadio "SetSupportsRadio" | @ref PVRCapabilities::GetSupportsRadio "GetSupportsRadio"
  /// | **Supports recordings** | `boolean` | @ref PVRCapabilities::SetSupportsRecordings "SetSupportsRecordings" | @ref PVRCapabilities::GetSupportsRecordings "GetSupportsRecordings"
  /// | **Supports recordings undelete** | `boolean` | @ref PVRCapabilities::SetSupportsRecordingsUndelete "SetSupportsRecordingsUndelete" | @ref PVRCapabilities::GetSupportsRecordingsUndelete "SetSupportsRecordingsUndelete"
  /// | **Supports timers** | `boolean` | @ref PVRCapabilities::SetSupportsTimers "SetSupportsTimers" | @ref PVRCapabilities::GetSupportsTimers "GetSupportsTimers"
  /// | **Supports providers** | `boolean` | @ref PVRCapabilities::SetSupportsProviders "SetSupportsProviders" | @ref PVRCapabilities::GetSupportsProviders "GetSupportsProviders"
  /// | **Supports channel groups** | `boolean` | @ref PVRCapabilities::SetSupportsChannelGroups "SetSupportsChannelGroups" | @ref PVRCapabilities::GetSupportsChannelGroups "GetSupportsChannelGroups"
  /// | **Supports channel scan** | `boolean` | @ref PVRCapabilities::SetSupportsChannelScan "SetSupportsChannelScan" | @ref PVRCapabilities::GetSupportsChannelScan "GetSupportsChannelScan"
  /// | **Supports channel settings** | `boolean` | @ref PVRCapabilities::SetSupportsChannelSettings "SetSupportsChannelSettings" | @ref PVRCapabilities::GetSupportsChannelSettings "GetSupportsChannelSettings"
  /// | **Handles input stream** | `boolean` | @ref PVRCapabilities::SetHandlesInputStream "SetHandlesInputStream" | @ref PVRCapabilities::GetHandlesInputStream "GetHandlesInputStream"
  /// | **Handles demuxing** | `boolean` | @ref PVRCapabilities::SetHandlesDemuxing "SetHandlesDemuxing" | @ref PVRCapabilities::GetHandlesDemuxing "GetHandlesDemuxing"
  /// | **Supports recording play count** | `boolean` | @ref PVRCapabilities::SetSupportsRecordingPlayCount "SetSupportsRecordingPlayCount" | @ref PVRCapabilities::GetSupportsRecordingPlayCount "GetSupportsRecordingPlayCount"
  /// | **Supports last played position** | `boolean` | @ref PVRCapabilities::SetSupportsLastPlayedPosition "SetSupportsLastPlayedPosition" | @ref PVRCapabilities::GetSupportsLastPlayedPosition "GetSupportsLastPlayedPosition"
  /// | **Supports recording EDL** | `boolean` | @ref PVRCapabilities::SetSupportsRecordingEdl "SetSupportsRecordingEdl" | @ref PVRCapabilities::GetSupportsRecordingEdl "GetSupportsRecordingEdl"
  /// | **Supports recordings rename** | `boolean` | @ref PVRCapabilities::SetSupportsRecordingsRename "SetSupportsRecordingsRename" | @ref PVRCapabilities::GetSupportsRecordingsRename "GetSupportsRecordingsRename"
  /// | **Supports recordings lifetime change** | `boolean` | @ref PVRCapabilities::SetSupportsRecordingsLifetimeChange "SetSupportsRecordingsLifetimeChange" | @ref PVRCapabilities::GetSupportsRecordingsLifetimeChange "GetSupportsRecordingsLifetimeChange"
  /// | **Supports descramble info** | `boolean` | @ref PVRCapabilities::SetSupportsDescrambleInfo "SetSupportsDescrambleInfo" | @ref PVRCapabilities::GetSupportsDescrambleInfo "GetSupportsDescrambleInfo"
  /// | **Supports async EPG transfer** | `boolean` | @ref PVRCapabilities::SetSupportsAsyncEPGTransfer "SetSupportsAsyncEPGTransfer" | @ref PVRCapabilities::GetSupportsAsyncEPGTransfer "GetSupportsAsyncEPGTransfer"
  /// | **Supports recording size** | `boolean` | @ref PVRCapabilities::SetSupportsRecordingSize "SetSupportsRecordingSize" | @ref PVRCapabilities::GetSupportsRecordingSize "GetSupportsRecordingSize"
  /// | **Supports recordings delete** | `boolean` | @ref PVRCapabilities::SetSupportsRecordingsDelete "SetSupportsRecordingsDelete" | @ref PVRCapabilities::GetSupportsRecordingsDelete "SetSupportsRecordingsDelete"
  /// | **Supports multiple recorded streams** | `boolean` | @ref PVRCapabilities::SetSupportsMultipleRecordedStreams "SetSupportsMultipleRecordedStreams" | @ref PVRCapabilities::GetSupportsMultipleRecordedStreams "GetSupportsMultipleRecordedStreams"
  /// | **Recordings lifetime values** | @ref cpp_kodi_addon_pvr_Defs_PVRTypeIntValue "PVRTypeIntValue" | @ref PVRCapabilities::SetRecordingsLifetimeValues "SetRecordingsLifetimeValues" | @ref PVRCapabilities::GetRecordingsLifetimeValues "GetRecordingsLifetimeValues"
  ///
  /// @warning This class can not be used outside of @ref kodi::addon::CInstancePVRClient::GetCapabilities()
  ///

  /// @addtogroup cpp_kodi_addon_pvr_Defs_PVRCapabilities
  ///@{

  /// @brief Set **true** if the add-on provides EPG information.
  void SetSupportsEPG(bool supportsEPG) { m_cStructure->bSupportsEPG = supportsEPG; }

  /// @brief To get with @ref SetSupportsEPG changed values.
  bool GetSupportsEPG() const { return m_cStructure->bSupportsEPG; }

  /// @brief Set **true** if the backend supports retrieving an edit decision
  /// list for an EPG tag.
  void SetSupportsEPGEdl(bool supportsEPGEdl) { m_cStructure->bSupportsEPGEdl = supportsEPGEdl; }

  /// @brief To get with @ref SetSupportsEPGEdl changed values.
  bool GetSupportsEPGEdl() const { return m_cStructure->bSupportsEPGEdl; }

  /// @brief Set **true** if this add-on provides TV channels.
  void SetSupportsTV(bool supportsTV) { m_cStructure->bSupportsTV = supportsTV; }

  /// @brief To get with @ref SetSupportsTV changed values.
  bool GetSupportsTV() const { return m_cStructure->bSupportsTV; }

  /// @brief Set **true** if this add-on provides TV channels.
  void SetSupportsRadio(bool supportsRadio) { m_cStructure->bSupportsRadio = supportsRadio; }

  /// @brief To get with @ref SetSupportsRadio changed values.
  bool GetSupportsRadio() const { return m_cStructure->bSupportsRadio; }

  /// @brief **true** if this add-on supports playback of recordings stored on
  /// the backend.
  void SetSupportsRecordings(bool supportsRecordings)
  {
    m_cStructure->bSupportsRecordings = supportsRecordings;
  }

  /// @brief To get with @ref SetSupportsRecordings changed values.
  bool GetSupportsRecordings() const { return m_cStructure->bSupportsRecordings; }

  /// @brief Set **true** if this add-on supports undelete of recordings stored
  /// on the backend.
  void SetSupportsRecordingsUndelete(bool supportsRecordingsUndelete)
  {
    m_cStructure->bSupportsRecordingsUndelete = supportsRecordingsUndelete;
  }

  /// @brief To get with @ref SetSupportsRecordings changed values.
  bool GetSupportsRecordingsUndelete() const { return m_cStructure->bSupportsRecordingsUndelete; }

  /// @brief Set **true** if this add-on supports the creation and editing of
  /// timers.
  void SetSupportsTimers(bool supportsTimers) { m_cStructure->bSupportsTimers = supportsTimers; }

  /// @brief To get with @ref SetSupportsTimers changed values.
  bool GetSupportsTimers() const { return m_cStructure->bSupportsTimers; }

  /// @brief Set **true** if this add-on supports providers.
  ///
  /// It uses the following functions:
  /// - @ref kodi::addon::CInstancePVRClient::GetProvidersAmount()
  /// - @ref kodi::addon::CInstancePVRClient::GetProviders()
  void SetSupportsProviders(bool supportsProviders)
  {
    m_cStructure->bSupportsProviders = supportsProviders;
  }

  /// @brief To get with @ref SetSupportsProviders changed values.
  bool GetSupportsProviders() const { return m_cStructure->bSupportsProviders; }

  /// @brief Set **true** if this add-on supports channel groups.
  ///
  /// It use the following functions:
  /// - @ref kodi::addon::CInstancePVRClient::GetChannelGroupsAmount()
  /// - @ref kodi::addon::CInstancePVRClient::GetChannelGroups()
  /// - @ref kodi::addon::CInstancePVRClient::GetChannelGroupMembers()
  void SetSupportsChannelGroups(bool supportsChannelGroups)
  {
    m_cStructure->bSupportsChannelGroups = supportsChannelGroups;
  }

  /// @brief To get with @ref SetSupportsChannelGroups changed values.
  bool GetSupportsChannelGroups() const { return m_cStructure->bSupportsChannelGroups; }

  /// @brief Set **true** if this add-on support scanning for new channels on
  /// the backend.
  ///
  /// It use the following function:
  /// - @ref kodi::addon::CInstancePVRClient::OpenDialogChannelScan()
  void SetSupportsChannelScan(bool supportsChannelScan)
  {
    m_cStructure->bSupportsChannelScan = supportsChannelScan;
  }

  /// @brief To get with @ref SetSupportsChannelScan changed values.
  bool GetSupportsChannelScan() const { return m_cStructure->bSupportsChannelScan; }

  /// @brief Set **true** if this add-on supports channel edit.
  ///
  /// It use the following functions:
  /// - @ref kodi::addon::CInstancePVRClient::DeleteChannel()
  /// - @ref kodi::addon::CInstancePVRClient::RenameChannel()
  /// - @ref kodi::addon::CInstancePVRClient::OpenDialogChannelSettings()
  /// - @ref kodi::addon::CInstancePVRClient::OpenDialogChannelAdd()
  void SetSupportsChannelSettings(bool supportsChannelSettings)
  {
    m_cStructure->bSupportsChannelSettings = supportsChannelSettings;
  }

  /// @brief To get with @ref SetSupportsChannelSettings changed values.
  bool GetSupportsChannelSettings() const { return m_cStructure->bSupportsChannelSettings; }

  /// @brief Set **true** if this add-on provides an input stream. false if Kodi
  /// handles the stream.
  void SetHandlesInputStream(bool handlesInputStream)
  {
    m_cStructure->bHandlesInputStream = handlesInputStream;
  }

  /// @brief To get with @ref SetHandlesInputStream changed values.
  bool GetHandlesInputStream() const { return m_cStructure->bHandlesInputStream; }

  /// @brief Set **true** if this add-on demultiplexes packets.
  void SetHandlesDemuxing(bool handlesDemuxing)
  {
    m_cStructure->bHandlesDemuxing = handlesDemuxing;
  }

  /// @brief To get with @ref SetHandlesDemuxing changed values.
  bool GetHandlesDemuxing() const { return m_cStructure->bHandlesDemuxing; }

  /// @brief Set **true** if the backend supports play count for recordings.
  void SetSupportsRecordingPlayCount(bool supportsRecordingPlayCount)
  {
    m_cStructure->bSupportsRecordingPlayCount = supportsRecordingPlayCount;
  }

  /// @brief To get with @ref SetSupportsRecordingPlayCount changed values.
  bool GetSupportsRecordingPlayCount() const { return m_cStructure->bSupportsRecordingPlayCount; }

  /// @brief Set **true** if the backend supports store/retrieve of last played
  /// position for recordings.
  void SetSupportsLastPlayedPosition(bool supportsLastPlayedPosition)
  {
    m_cStructure->bSupportsLastPlayedPosition = supportsLastPlayedPosition;
  }

  /// @brief To get with @ref SetSupportsLastPlayedPosition changed values.
  bool GetSupportsLastPlayedPosition() const { return m_cStructure->bSupportsLastPlayedPosition; }

  /// @brief Set **true** if the backend supports retrieving an edit decision
  /// list for recordings.
  void SetSupportsRecordingEdl(bool supportsRecordingEdl)
  {
    m_cStructure->bSupportsRecordingEdl = supportsRecordingEdl;
  }

  /// @brief To get with @ref SetSupportsRecordingEdl changed values.
  bool GetSupportsRecordingEdl() const { return m_cStructure->bSupportsRecordingEdl; }

  /// @brief Set **true** if the backend supports renaming recordings.
  void SetSupportsRecordingsRename(bool supportsRecordingsRename)
  {
    m_cStructure->bSupportsRecordingsRename = supportsRecordingsRename;
  }

  /// @brief To get with @ref SetSupportsRecordingsRename changed values.
  bool GetSupportsRecordingsRename() const { return m_cStructure->bSupportsRecordingsRename; }

  /// @brief Set **true** if the backend supports changing lifetime for
  /// recordings.
  void SetSupportsRecordingsLifetimeChange(bool supportsRecordingsLifetimeChange)
  {
    m_cStructure->bSupportsRecordingsLifetimeChange = supportsRecordingsLifetimeChange;
  }

  /// @brief To get with @ref SetSupportsRecordingsLifetimeChange changed
  /// values.
  bool GetSupportsRecordingsLifetimeChange() const
  {
    return m_cStructure->bSupportsRecordingsLifetimeChange;
  }

  /// @brief Set **true** if the backend supports descramble information for
  /// playing channels.
  void SetSupportsDescrambleInfo(bool supportsDescrambleInfo)
  {
    m_cStructure->bSupportsDescrambleInfo = supportsDescrambleInfo;
  }

  /// @brief To get with @ref SetSupportsDescrambleInfo changed values.
  bool GetSupportsDescrambleInfo() const { return m_cStructure->bSupportsDescrambleInfo; }

  /// @brief Set **true** if this addon-on supports asynchronous transfer of epg
  /// events to Kodi using the callback function
  /// @ref kodi::addon::CInstancePVRClient::EpgEventStateChange().
  void SetSupportsAsyncEPGTransfer(bool supportsAsyncEPGTransfer)
  {
    m_cStructure->bSupportsAsyncEPGTransfer = supportsAsyncEPGTransfer;
  }

  /// @brief To get with @ref SetSupportsAsyncEPGTransfer changed values.
  bool GetSupportsAsyncEPGTransfer() const { return m_cStructure->bSupportsAsyncEPGTransfer; }

  /// @brief Set **true** if this addon-on supports retrieving size of recordings.
  void SetSupportsRecordingSize(bool supportsRecordingSize)
  {
    m_cStructure->bSupportsRecordingSize = supportsRecordingSize;
  }

  /// @brief To get with @ref SetSupportsRecordingSize changed values.
  bool GetSupportsRecordingSize() const { return m_cStructure->bSupportsRecordingSize; }

  /// @brief Set **true** if this add-on supports delete of recordings stored
  /// on the backend.
  void SetSupportsRecordingsDelete(bool supportsRecordingsDelete)
  {
    m_cStructure->bSupportsRecordingsDelete = supportsRecordingsDelete;
  }

  /// @brief To get with @ref SetSupportsRecordingsDelete changed values.
  bool GetSupportsRecordingsDelete() const { return m_cStructure->bSupportsRecordingsDelete; }

  /// @brief Set **true** if this add-on supports multiple streams for recordings at a time.
  void SetSupportsMultipleRecordedStreams(bool supportsMultipleRecordedStreams)
  {
    m_cStructure->bSupportsMultipleRecordedStreams = supportsMultipleRecordedStreams;
  }

  /// @brief To get with @ref SetSupportsMultiRecordedStreams changed values.
  bool GetSupportsMultipleRecordedStreams() const
  {
    return m_cStructure->bSupportsMultipleRecordedStreams;
  }

  /// @brief **optional**\n
  /// Set array containing the possible values for @ref PVRRecording::SetLifetime().
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_pvr_Defs_PVRTypeIntValue_Help
  void SetRecordingsLifetimeValues(const std::vector<PVRTypeIntValue>& recordingsLifetimeValues)
  {
    PVRTypeIntValue::ReallocAndCopyData(&m_cStructure->recordingsLifetimeValues,
                                        &m_cStructure->iRecordingsLifetimesSize,
                                        recordingsLifetimeValues);
  }

  /// @brief To get with @ref SetRecordingsLifetimeValues changed values.
  std::vector<PVRTypeIntValue> GetRecordingsLifetimeValues() const
  {
    std::vector<PVRTypeIntValue> recordingsLifetimeValues;
    for (unsigned int i = 0; i < m_cStructure->iRecordingsLifetimesSize; ++i)
      recordingsLifetimeValues.emplace_back(
          m_cStructure->recordingsLifetimeValues[i].iValue,
          m_cStructure->recordingsLifetimeValues[i].strDescription);
    return recordingsLifetimeValues;
  }
  ///@}

  static void AllocResources(const PVR_ADDON_CAPABILITIES* source, PVR_ADDON_CAPABILITIES* target)
  {
    if (target->iRecordingsLifetimesSize)
    {
      target->recordingsLifetimeValues = PVRTypeIntValue::AllocAndCopyData(
          source->recordingsLifetimeValues, source->iRecordingsLifetimesSize);
    }
  }

  static void FreeResources(PVR_ADDON_CAPABILITIES* target)
  {
    PVRTypeIntValue::FreeResources(target->recordingsLifetimeValues,
                                   target->iRecordingsLifetimesSize);
    target->recordingsLifetimeValues = nullptr;
    target->iRecordingsLifetimesSize = 0;
  }

private:
  PVRCapabilities(const PVR_ADDON_CAPABILITIES* capabilities) : DynamicCStructHdl(capabilities) {}
  PVRCapabilities(PVR_ADDON_CAPABILITIES* capabilities) : DynamicCStructHdl(capabilities) {}
};
///@}
//------------------------------------------------------------------------------

//==============================================================================
/// @defgroup cpp_kodi_addon_pvr_Defs_General_Inputstream_PVRStreamProperty class PVRStreamProperty
/// @ingroup cpp_kodi_addon_pvr_Defs_General_Inputstream
/// @brief **PVR stream property value handler**\n
/// To set for Kodi wanted stream properties.
///
/// ----------------------------------------------------------------------------
///
/// @copydetails cpp_kodi_addon_pvr_Defs_General_Inputstream_PVRStreamProperty_Help
///
///---------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// ...
///
/// PVR_ERROR CMyPVRInstance::GetChannelStreamProperties(const kodi::addon::PVRChannel& channel,
///                                                      PVR_SOURCE source,
///                                                      std::vector<kodi::addon::PVRStreamProperty>& properties)
/// {
///   ...
///   properties.emplace_back(PVR_STREAM_PROPERTY_INPUTSTREAM, "inputstream.adaptive");
///   return PVR_ERROR_NO_ERROR;
/// }
///
/// ...
/// ~~~~~~~~~~~~~
///
///
/// **Example 2:**
/// ~~~~~~~~~~~~~{.cpp}
/// ...
///
/// PVR_ERROR CMyPVRInstance::GetChannelStreamProperties(const kodi::addon::PVRChannel& channel,
///                                                      PVR_SOURCE source,
///                                                      std::vector<kodi::addon::PVRStreamProperty>& properties)
/// {
///   ...
///   kodi::addon::PVRStreamProperty property;
///   property.SetName(PVR_STREAM_PROPERTY_INPUTSTREAM);
///   property.SetValue("inputstream.adaptive");
///   properties.emplace_back(property);
///   return PVR_ERROR_NO_ERROR;
/// }
///
/// ...
/// ~~~~~~~~~~~~~
///
///@{
class PVRStreamProperty : public DynamicCStructHdl<PVRStreamProperty, PVR_NAMED_VALUE>
{
  friend class CInstancePVRClient;

public:
  /*! \cond PRIVATE */
  PVRStreamProperty(const PVRStreamProperty& property) : DynamicCStructHdl(property) {}
  /*! \endcond */

  /// @defgroup cpp_kodi_addon_pvr_Defs_General_Inputstream_PVRStreamProperty_Help Value Help
  /// @ingroup cpp_kodi_addon_pvr_Defs_General_Inputstream_PVRStreamProperty
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_addon_pvr_Defs_General_Inputstream_PVRStreamProperty :</b>
  /// | Name | Type | Set call | Get call
  /// |------|------|----------|----------
  /// | **Name** | `int` | @ref PVRStreamProperty::SetValue "SetName" | @ref PVRStreamProperty::GetName "GetName"
  /// | **Value** | `std::string` | @ref PVRStreamProperty::SetValue "SetValue" | @ref PVRStreamProperty::GetValue "GetValue"
  ///
  /// @remark Further can there be used his class constructor to set values.

  /// @addtogroup cpp_kodi_addon_pvr_Defs_General_Inputstream_PVRStreamProperty
  ///@{

  /// @brief Default class constructor.
  PVRStreamProperty() = default;

  /// @brief Class constructor with integrated value set.
  ///
  /// @param[in] name Type identification
  /// @param[in] value Type used property value
  PVRStreamProperty(const std::string& name, const std::string& value)
  {
    SetName(name);
    SetValue(value);
  }

  /// @brief To set with the identification name.
  void SetName(const std::string& name)
  {
    ReallocAndCopyString(&m_cStructure->strName, name.c_str());
  }

  /// @brief To get with the identification name.
  std::string GetName() const { return m_cStructure->strName ? m_cStructure->strName : ""; }

  /// @brief To set with the used property value.
  void SetValue(const std::string& value)
  {
    ReallocAndCopyString(&m_cStructure->strValue, value.c_str());
  }

  /// @brief To get with the used property value.
  std::string GetValue() const { return m_cStructure->strValue ? m_cStructure->strValue : ""; }
  ///@}

  static void AllocResources(const PVR_NAMED_VALUE* source, PVR_NAMED_VALUE* target)
  {
    target->strName = AllocAndCopyString(source->strName);
    target->strValue = AllocAndCopyString(source->strValue);
  }

  static void FreeResources(PVR_NAMED_VALUE* target)
  {
    FreeString(target->strName);
    FreeString(target->strValue);
  }

private:
  PVRStreamProperty(const PVR_NAMED_VALUE* property) : DynamicCStructHdl(property) {}
  PVRStreamProperty(PVR_NAMED_VALUE* property) : DynamicCStructHdl(property) {}
};
///@}
//------------------------------------------------------------------------------

} /* namespace addon */
} /* namespace kodi */

#endif /* __cplusplus */
