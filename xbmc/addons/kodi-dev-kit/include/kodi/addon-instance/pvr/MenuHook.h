/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../../AddonBase.h"
#include "../../c-api/addon-instance/pvr/pvr_menu_hook.h"

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// "C++" Definitions group 7 - Menu hook
#ifdef __cplusplus

namespace kodi
{
namespace addon
{

//==============================================================================
/// @defgroup cpp_kodi_addon_pvr_Defs_Menuhook_PVRMenuhook class PVRMenuhook
/// @ingroup cpp_kodi_addon_pvr_Defs_Menuhook
/// @brief **Context menu hook**\n
/// Menu hooks that are available in the context menus while playing a stream via this add-on.
/// And in the Live TV settings dialog.
///
/// Possible menu's given to Kodi.
///
/// This can be becomes used on this, if @ref kodi::addon::CInstancePVRClient::AddMenuHook()
/// was set to related type:
/// - @ref kodi::addon::CInstancePVRClient::CallSettingsMenuHook()
/// - @ref kodi::addon::CInstancePVRClient::CallChannelMenuHook()
/// - @ref kodi::addon::CInstancePVRClient::CallEPGMenuHook()
/// - @ref kodi::addon::CInstancePVRClient::CallRecordingMenuHook()
/// - @ref kodi::addon::CInstancePVRClient::CallTimerMenuHook()
///
/// ----------------------------------------------------------------------------
///
/// @copydetails cpp_kodi_addon_pvr_Defs_Menuhook_PVRMenuhook_Help
///
///@{
class PVRMenuhook : public CStructHdl<PVRMenuhook, PVR_MENUHOOK>
{
  friend class CInstancePVRClient;

public:
  /// @addtogroup cpp_kodi_addon_pvr_Defs_Menuhook_PVRMenuhook
  /// @brief Optional class constructor with value set.
  ///
  /// @param[in] hookId This hook's identifier
  /// @param[in] localizedStringId Localized string identifier
  /// @param[in] category Category of menu hook, defined with @ref PVR_MENUHOOK_CAT
  ///
  ///
  /// --------------------------------------------------------------------------
  ///
  /// Example:
  /// ~~~~~~~~~~~~~{.cpp}
  /// AddMenuHook(kodi::addon::PVRMenuhook(1, 30001, PVR_MENUHOOK_CHANNEL));
  /// ~~~~~~~~~~~~~
  ///
  PVRMenuhook(unsigned int hookId, unsigned int localizedStringId, PVR_MENUHOOK_CAT category)
  {
    m_cStructure->iHookId = hookId;
    m_cStructure->iLocalizedStringId = localizedStringId;
    m_cStructure->category = category;
  }

  /*! \cond PRIVATE */
  PVRMenuhook()
  {
    m_cStructure->iHookId = 0;
    m_cStructure->iLocalizedStringId = 0;
    m_cStructure->category = PVR_MENUHOOK_UNKNOWN;
  }
  PVRMenuhook(const PVRMenuhook& data) : CStructHdl(data) {}
  /*! \endcond */

  /// @defgroup cpp_kodi_addon_pvr_Defs_Menuhook_PVRMenuhook_Help Value Help
  /// @ingroup cpp_kodi_addon_pvr_Defs_Menuhook_PVRMenuhook
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_addon_pvr_Defs_Menuhook_PVRMenuhook :</b>
  /// | Name | Type | Set call | Get call | Usage
  /// |------|------|----------|----------|-----------
  /// | **This hook's identifier** | `unsigned int` | @ref PVRMenuhook::SetHookId "SetHookId" | @ref PVRMenuhook::GetHookId "GetHookId" | *required to set*
  /// | **Localized string Identifier** | `unsigned int` | @ref PVRMenuhook::SetLocalizedStringId "SetLocalizedStringId" | @ref PVRMenuhook::GetLocalizedStringId "GetLocalizedStringId" | *required to set*
  /// | **Category of menu hook** | @ref PVR_MENUHOOK_CAT | @ref PVRMenuhook::SetCategory "SetCategory" | @ref PVRMenuhook::GetCategory "GetCategory" | *required to set*

  /// @addtogroup cpp_kodi_addon_pvr_Defs_Menuhook_PVRMenuhook
  ///@{

  /// @brief **required**\n
  /// This hook's identifier.
  void SetHookId(unsigned int hookId) { m_cStructure->iHookId = hookId; }

  /// @brief To get with @ref SetHookId() changed values.
  unsigned int GetHookId() const { return m_cStructure->iHookId; }

  /// @brief **required**\n
  /// The id of the label for this hook in @ref kodi::GetLocalizedString().
  void SetLocalizedStringId(unsigned int localizedStringId)
  {
    m_cStructure->iLocalizedStringId = localizedStringId;
  }

  /// @brief To get with @ref SetLocalizedStringId() changed values.
  unsigned int GetLocalizedStringId() const { return m_cStructure->iLocalizedStringId; }

  /// @brief **required**\n
  /// Category of menu hook.
  void SetCategory(PVR_MENUHOOK_CAT category) { m_cStructure->category = category; }

  /// @brief To get with @ref SetCategory() changed values.
  PVR_MENUHOOK_CAT GetCategory() const { return m_cStructure->category; }
  ///@}

private:
  PVRMenuhook(const PVR_MENUHOOK* data) : CStructHdl(data) {}
  PVRMenuhook(PVR_MENUHOOK* data) : CStructHdl(data) {}
};
///@}
//------------------------------------------------------------------------------

} /* namespace addon */
} /* namespace kodi */

#endif /* __cplusplus */
