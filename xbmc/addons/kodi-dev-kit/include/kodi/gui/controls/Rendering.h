/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../../c-api/gui/controls/rendering.h"
#include "../Window.h"
#include "../renderHelper.h"

#ifdef __cplusplus

namespace kodi
{
namespace gui
{
namespace controls
{

//============================================================================
/// @defgroup cpp_kodi_gui_windows_controls_CRendering Control Rendering
/// @ingroup cpp_kodi_gui_windows_controls
/// @brief @cpp_class{ kodi::gui::controls::CRendering }
/// **Window control for rendering own parts**\n
/// This rendering control is used when own parts are needed.
///
/// You have the control over them to render direct OpenGL or DirectX content
/// to the screen set by the size of them.
///
/// Alternative can be the virtual functions from t his been ignored if the
/// callbacks are defined by the @ref CRendering_SetIndependentCallbacks
/// function and class is used as single and not as a parent class.
///
/// It has the header @ref Rendering.h "#include <kodi/gui/controls/Rendering.h>"
/// be included to enjoy it.
///
/// Here you find the needed skin part for a @ref Addon_Rendering_control "rendering control".
///
/// @note The call of the control is only possible from the corresponding
/// window as its class and identification number is required.
///
class ATTR_DLL_LOCAL CRendering : public CAddonGUIControlBase
{
public:
  //==========================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CRendering
  /// @brief Construct a new control.
  ///
  /// @param[in] window Related window control class
  /// @param[in] controlId Used skin xml control id
  ///
  CRendering(CWindow* window, int controlId) : CAddonGUIControlBase(window)
  {
    m_controlHandle = m_interface->kodi_gui->window->get_control_render_addon(
        m_interface->kodiBase, m_Window->GetControlHandle(), controlId);
    if (m_controlHandle)
      m_interface->kodi_gui->control_rendering->set_callbacks(m_interface->kodiBase,
                                                              m_controlHandle, this, OnCreateCB,
                                                              OnRenderCB, OnStopCB, OnDirtyCB);
    else
      kodi::Log(ADDON_LOG_FATAL, "kodi::gui::controls::%s can't create control class from Kodi !!!",
                __FUNCTION__);
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CRendering
  /// @brief Destructor.
  ///
  ~CRendering() override
  {
    m_interface->kodi_gui->control_rendering->destroy(m_interface->kodiBase, m_controlHandle);
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CRendering
  /// @brief To create rendering control on Add-on.
  ///
  /// Function creates the needed rendering control for Kodi which becomes
  /// handled and processed from Add-on
  ///
  /// @note This is callback function from Kodi to Add-on and not to use
  /// for calls from add-on to this function.
  ///
  /// @param[in] x Horizontal position
  /// @param[in] y Vertical position
  /// @param[in] w Width of control
  /// @param[in] h Height of control
  /// @param[in] device The device to use. For OpenGL is empty on Direct X is
  ///                   the needed device send.
  /// @return True on success, false otherwise.
  ///
  /// @note The @ref kodi::HardwareContext is basically a simple pointer which
  /// has to be changed to the desired format at the corresponding places using
  /// <b>`static_cast<...>(...)`</b>.
  ///
  virtual bool Create(int x, int y, int w, int h, kodi::HardwareContext device) { return false; }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CRendering
  /// @brief Render process call from Kodi.
  ///
  /// @note This is callback function from  Kodi to Add-on and not to use for
  /// calls from add-on to this function.
  ///
  virtual void Render() {}
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CRendering
  /// @brief Call from Kodi to stop rendering process.
  ///
  /// @note This is callback function from Kodi to Add-on and not to use
  /// for calls from add-on to this function.
  ///
  virtual void Stop() {}
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CRendering
  /// @brief Call from Kodi where add-on becomes asked about dirty rendering
  /// region.
  ///
  /// @note This is callback function from Kodi to Add-on and not to use
  /// for calls from add-on to this function.
  ///
  /// @return True if a render region is dirty and need rendering.
  ///
  virtual bool Dirty() { return false; }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @ingroup cpp_kodi_gui_windows_controls_CRendering
  /// @anchor CRendering_SetIndependentCallbacks
  /// @brief If the class is used independent (with "new CRendering")
  /// and not as parent (with "cCLASS_own : CRendering") from own must
  /// be the callback from Kodi to add-on overdriven with own functions!
  ///
  /// @param[in] cbhdl Addon related class point where becomes given as value on
  ///                  related functions.
  /// @param[in] CBCreate External creation function pointer, see also @ref Create
  ///                     about related values
  /// @param[in] CBRender External render function pointer, see also @ref Render
  ///                     about related values
  /// @param[in] CBStop External stop function pointer, see also @ref Stop
  ///                   about related values
  /// @param[in] CBDirty External dirty function pointer, see also @ref Dirty
  ///                    about related values
  ///
  void SetIndependentCallbacks(kodi::gui::ClientHandle cbhdl,
                               bool (*CBCreate)(kodi::gui::ClientHandle cbhdl,
                                                int x,
                                                int y,
                                                int w,
                                                int h,
                                                kodi::HardwareContext device),
                               void (*CBRender)(kodi::gui::ClientHandle cbhdl),
                               void (*CBStop)(kodi::gui::ClientHandle cbhdl),
                               bool (*CBDirty)(kodi::gui::ClientHandle cbhdl))
  {
    if (!cbhdl || !CBCreate || !CBRender || !CBStop || !CBDirty)
    {
      kodi::Log(ADDON_LOG_ERROR, "kodi::gui::controls::%s called with nullptr !!!", __FUNCTION__);
      return;
    }

    m_interface->kodi_gui->control_rendering->set_callbacks(
        m_interface->kodiBase, m_controlHandle, cbhdl, CBCreate, CBRender, CBStop, CBDirty);
  }
  //--------------------------------------------------------------------------

private:
  /*
   * Defined callback functions from Kodi to add-on, for use in parent / child system
   * (is private)!
   */
  static bool OnCreateCB(
      KODI_GUI_CLIENT_HANDLE cbhdl, int x, int y, int w, int h, ADDON_HARDWARE_CONTEXT device)
  {
    static_cast<CRendering*>(cbhdl)->m_renderHelper = kodi::gui::GetRenderHelper();
    return static_cast<CRendering*>(cbhdl)->Create(x, y, w, h, device);
  }

  static void OnRenderCB(KODI_GUI_CLIENT_HANDLE cbhdl)
  {
    if (!static_cast<CRendering*>(cbhdl)->m_renderHelper)
      return;
    static_cast<CRendering*>(cbhdl)->m_renderHelper->Begin();
    static_cast<CRendering*>(cbhdl)->Render();
    static_cast<CRendering*>(cbhdl)->m_renderHelper->End();
  }

  static void OnStopCB(KODI_GUI_CLIENT_HANDLE cbhdl)
  {
    static_cast<CRendering*>(cbhdl)->Stop();
    static_cast<CRendering*>(cbhdl)->m_renderHelper = nullptr;
  }

  static bool OnDirtyCB(KODI_GUI_CLIENT_HANDLE cbhdl)
  {
    return static_cast<CRendering*>(cbhdl)->Dirty();
  }

  std::shared_ptr<kodi::gui::IRenderHelper> m_renderHelper;
};

} /* namespace controls */
} /* namespace gui */
} /* namespace kodi */

#endif /* __cplusplus */
