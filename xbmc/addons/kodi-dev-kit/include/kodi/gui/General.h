/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../AddonBase.h"
#include "../c-api/gui/general.h"

#ifdef __cplusplus

namespace kodi
{
namespace gui
{
//==============================================================================
/// @ingroup cpp_kodi_gui_general
/// @brief **Resolution info class**\n
/// Used to provide the necessary data for the resolution info.
///
class ResolutionInfo : private KODI_GUI_RESOLUTION_INFO
{
public:
  ResolutionInfo(int width, int height, float refreshRate)
  {
    this->width = width;
    this->height = height;
    this->refresh_rate = refreshRate;
  }

  ResolutionInfo(const KODI_GUI_RESOLUTION_INFO& data)
  {
    width = data.width;
    height = data.height;
    refresh_rate = data.refresh_rate;
  }

  ResolutionInfo()
  {
    width = -1;
    height = -1;
    refresh_rate = 0.0f;
  }

  void SetWidth(int width) { this->width = width; }
  int GetWidth() const { return width; }

  void SetHeight(int height) { this->height = height; }
  int GetHeight() const { return height; }

  void SetRefreshRate(float refreshRate) { this->refresh_rate = refreshRate; }
  float GetRefreshRate() const { return refresh_rate; }
};
//------------------------------------------------------------------------------

//==============================================================================
/// @addtogroup cpp_kodi_gui_general
/// Permits the use of the required functions of the add-on to Kodi.
///
/// These are pure functions them no other initialization need.
///
/// It has the header @ref kodi/gui/General.h "#include <kodi/gui/General.h>" be included
/// to enjoy it.
///

//==============================================================================
/// @ingroup cpp_kodi_gui_general
/// @brief Performs a graphical lock of rendering engine.
///
inline void ATTR_DLL_LOCAL Lock()
{
  using namespace ::kodi::addon;
  CPrivateBase::m_interface->toKodi->kodi_gui->general->lock();
}
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_gui_general
/// @brief Performs a graphical unlock of previous locked rendering engine.
///
inline void ATTR_DLL_LOCAL Unlock()
{
  using namespace ::kodi::addon;
  CPrivateBase::m_interface->toKodi->kodi_gui->general->unlock();
}
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_gui_general
/// @brief Return the the current screen height with pixel.
///
/// @return Screen height with pixel
///
inline int ATTR_DLL_LOCAL GetScreenHeight()
{
  using namespace ::kodi::addon;
  return CPrivateBase::m_interface->toKodi->kodi_gui->general->get_screen_height(
      CPrivateBase::m_interface->toKodi->kodiBase);
}
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_gui_general
/// @brief Return the the current screen width with pixel.
///
/// @return Screen width with pixel
///
inline int ATTR_DLL_LOCAL GetScreenWidth()
{
  using namespace ::kodi::addon;
  return CPrivateBase::m_interface->toKodi->kodi_gui->general->get_screen_width(
      CPrivateBase::m_interface->toKodi->kodiBase);
}
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_gui_general
/// @brief Return the the current screen rendering resolution.
///
/// @return Current screen rendering resolution
///
inline int ATTR_DLL_LOCAL GetVideoResolution()
{
  using namespace ::kodi::addon;
  return CPrivateBase::m_interface->toKodi->kodi_gui->general->get_video_resolution(
      CPrivateBase::m_interface->toKodi->kodiBase);
}
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_gui_general
/// @brief Returns the id for the current 'active' dialog as an integer.
///
/// @return The currently active dialog Id
///
///
///-------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// ..
/// int wid = kodi::gui::GetCurrentWindowDialogId();
/// ..
/// ~~~~~~~~~~~~~
///
inline int ATTR_DLL_LOCAL GetCurrentWindowDialogId()
{
  using namespace ::kodi::addon;
  return CPrivateBase::m_interface->toKodi->kodi_gui->general->get_current_window_dialog_id(
      CPrivateBase::m_interface->toKodi->kodiBase);
}
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_gui_general
/// @brief Returns the id for the current 'active' window as an integer.
///
/// @return The currently active window Id
///
///
///-------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// ..
/// int wid = kodi::gui::GetCurrentWindowId();
/// ..
/// ~~~~~~~~~~~~~
///
inline int ATTR_DLL_LOCAL GetCurrentWindowId()
{
  using namespace ::kodi::addon;
  return CPrivateBase::m_interface->toKodi->kodi_gui->general->get_current_window_id(
      CPrivateBase::m_interface->toKodi->kodiBase);
}
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_gui_general
/// @brief To get hardware specific device context interface.
///
/// @return A pointer to the used device with @ref cpp_kodi_Defs_HardwareContext "kodi::HardwareContext"
///
/// @warning This function is only be supported under Windows, on all other
/// OS it return `nullptr`!
///
/// @note Returned Windows class pointer is `ID3D11DeviceContext1`.
///
///
///-------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <d3d11_1.h>
/// ..
/// ID3D11DeviceContext1* context = static_cast<ID3D11DeviceContext1*>(kodi::gui::GetHWContext());
/// ..
/// ~~~~~~~~~~~~~
///
inline kodi::HardwareContext GetHWContext()
{
  using namespace ::kodi::addon;
  return CPrivateBase::m_interface->toKodi->kodi_gui->general->get_hw_context(
      CPrivateBase::m_interface->toKodi->kodiBase);
}
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_gui_general
/// @brief Get the whitelist resolutions.
///
/// @return The whitelist resolutions
///
///
///-------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/gui/General.h>
/// ..
/// std::vector<KODI_GUI_RESOLUTION_INFO> resList = kodi::gui::GetWhitelistResolutions();
/// ..
/// ~~~~~~~~~~~~~
///
inline std::vector<ResolutionInfo> GetWhitelistResolutions()
{
  using namespace ::kodi::addon;
  std::vector<ResolutionInfo> resInfoList;
  KODI_GUI_RESOLUTION_INFO* list{nullptr};
  size_t size{0};

  CPrivateBase::m_interface->toKodi->kodi_gui->general->get_whitelist_resolutions(
      CPrivateBase::m_interface->toKodi->kodiBase, &list, &size);

  if (list)
  {
    for (unsigned int i{0}; i < size; i++)
    {
      ResolutionInfo resInfo{list[i]};
      resInfoList.emplace_back(resInfo);
    }

    free(list);
  }

  return resInfoList;
}
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_gui_general
/// @brief Get the allowed resolutions.
///
/// @return The allowed resolutions
///
///
///-------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/gui/General.h>
/// ..
/// std::vector<KODI_GUI_RESOLUTION_INFO> resList = kodi::gui::GetAllowedResolutions();
/// ..
/// ~~~~~~~~~~~~~
///
inline std::vector<ResolutionInfo> GetAllowedResolutions()
{
  using namespace ::kodi::addon;
  std::vector<ResolutionInfo> resInfoList;
  KODI_GUI_RESOLUTION_INFO* list{nullptr};
  size_t size{0};

  CPrivateBase::m_interface->toKodi->kodi_gui->general->get_allowed_resolutions(
      CPrivateBase::m_interface->toKodi->kodiBase, &list, &size);

  if (list)
  {
    for (unsigned int i{0}; i < size; i++)
    {
      ResolutionInfo resInfo{list[i]};
      resInfoList.emplace_back(resInfo);
    }

    free(list);
  }

  return resInfoList;
}
//------------------------------------------------------------------------------

} /* namespace gui */
} /* namespace kodi */

#endif /* __cplusplus */
