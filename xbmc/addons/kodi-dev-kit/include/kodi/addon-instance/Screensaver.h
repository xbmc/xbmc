/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../AddonBase.h"
#include "../c-api/addon-instance/screensaver.h"
#include "../gui/renderHelper.h"

#ifdef __cplusplus
namespace kodi
{
namespace addon
{

//==============================================================================
/// @addtogroup cpp_kodi_addon_screensaver
/// @brief \cpp_class{ kodi::addon::CInstanceScreensaver }
/// **Screensaver add-on instance**
///
/// A screensaver is a Kodi addon that fills the screen with moving images or
/// patterns when the computer is not in use. Initially designed to prevent
/// phosphor burn-in on CRT and plasma computer monitors (hence the name),
/// screensavers are now used primarily for entertainment, security or to
/// display system status information.
///
/// Include the header @ref Screensaver.h "#include <kodi/addon-instance/ScreenSaver.h>"
/// to use this class.
///
/// This interface allows the creating of screensavers for Kodi, based upon
/// **DirectX** or/and **OpenGL** rendering with `C++` code.
///
/// The interface is small and easy usable. It has three functions:
///
/// * <b><c>Start()</c></b> - Called on creation
/// * <b><c>Render()</c></b> - Called at render time
/// * <b><c>Stop()</c></b> - Called when the screensaver has no work
///
/// Additionally, there are several \ref cpp_kodi_addon_screensaver_CB "other functions"
/// available in which the child class can ask about the current hardware,
/// including the device, display and several other parts.
///
/// ----------------------------------------------------------------------------
///
/// Here is an example of what the <b>`addon.xml.in`</b> would look like for an
/// screensaver addon:
///
/// ~~~~~~~~~~~~~{.xml}
/// <?xml version="1.0" encoding="UTF-8"?>
/// <addon
///   id="screensaver.myspecialnamefor"
///   version="1.0.0"
///   name="My special screensaver addon"
///   provider-name="Your Name">
///   <requires>@ADDON_DEPENDS@</requires>
///   <extension
///     point="xbmc.ui.screensaver"
///     library_@PLATFORM@="@LIBRARY_FILENAME@"/>
///   <extension point="xbmc.addon.metadata">
///     <summary lang="en_GB">My screensaver addon</summary>
///     <description lang="en_GB">My screensaver addon description</description>
///     <platform>@PLATFORM@</platform>
///   </extension>
/// </addon>
/// ~~~~~~~~~~~~~
///
/// Description to screensaver related addon.xml values:
/// | Name                          | Description
/// |:------------------------------|----------------------------------------
/// | <b>`point`</b>                | Addon type specification<br>At all addon types and for this kind always <b>"xbmc.ui.screensaver"</b>.
/// | <b>`library_@PLATFORM@`</b>   | Sets the used library name, which is automatically set by cmake at addon build.
///
/// @remark For more detailed description of the <b>`addon.xml`</b>, see also https://kodi.wiki/view/Addon.xml.
///
///
/// --------------------------------------------------------------------------
///
/// **Here is an example of the minimum required code to start a screensaver:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/addon-instance/Screensaver.h>
///
/// class CMyScreenSaver : public kodi::addon::CAddonBase,
///                        public kodi::addon::CInstanceScreensaver
/// {
/// public:
///   CMyScreenSaver();
///
///   bool Start() override;
///   void Render() override;
/// };
///
/// CMyScreenSaver::CMyScreenSaver()
/// {
///   ...
/// }
///
/// bool CMyScreenSaver::Start()
/// {
///   ...
///   return true;
/// }
///
/// void CMyScreenSaver::Render()
/// {
///   ...
/// }
///
/// ADDONCREATOR(CMyScreenSaver)
/// ~~~~~~~~~~~~~
///
///
/// --------------------------------------------------------------------------
///
///
/// **Here is another example where the screensaver is used together with
/// other instance types:**
///
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/addon-instance/Screensaver.h>
///
/// class CMyScreenSaver : public kodi::addon::CInstanceScreensaver
/// {
/// public:
///   CMyScreenSaver(KODI_HANDLE instance, const std::string& version);
///
///   bool Start() override;
///   void Render() override;
/// };
///
/// CMyScreenSaver::CMyScreenSaver(KODI_HANDLE instance, const std::string& version)
///   : CInstanceScreensaver(instance, version)
/// {
///   ...
/// }
///
/// bool CMyScreenSaver::Start()
/// {
///   ...
///   return true;
/// }
///
/// void CMyScreenSaver::Render()
/// {
///   ...
/// }
///
///
/// //----------------------------------------------------------------------
///
/// class CMyAddon : public kodi::addon::CAddonBase
/// {
/// public:
///   CMyAddon() = default;
///   ADDON_STATUS CreateInstance(int instanceType,
///                               const std::string& instanceID,
///                               KODI_HANDLE instance,
///                               const std::string& version,
///                               KODI_HANDLE& addonInstance) override;
/// };
///
/// // If you use only one instance in your add-on, can be instanceType and
/// // instanceID ignored
/// ADDON_STATUS CMyAddon::CreateInstance(int instanceType,
///                                       const std::string& instanceID,
///                                       KODI_HANDLE instance,
///                                       const std::string& version,
///                                       KODI_HANDLE& addonInstance)
/// {
///   if (instanceType == ADDON_INSTANCE_SCREENSAVER)
///   {
///     kodi::Log(ADDON_LOG_INFO, "Creating my Screensaver");
///     addonInstance = new CMyScreenSaver(instance, version);
///     return ADDON_STATUS_OK;
///   }
///   else if (...)
///   {
///     ...
///   }
///   return ADDON_STATUS_UNKNOWN;
/// }
///
/// ADDONCREATOR(CMyAddon)
/// ~~~~~~~~~~~~~
///
/// The destruction of the example class `CMyScreenSaver` is called from
/// Kodi's header. Manually deleting the add-on instance is not required.
///
class ATTRIBUTE_HIDDEN CInstanceScreensaver : public IAddonInstance
{
public:
  //============================================================================
  /// @ingroup cpp_kodi_addon_screensaver
  /// @brief Screensaver class constructor.
  ///
  /// Used by an add-on that only supports screensavers.
  ///
  CInstanceScreensaver()
    : IAddonInstance(ADDON_INSTANCE_SCREENSAVER, GetKodiTypeVersion(ADDON_INSTANCE_SCREENSAVER))
  {
    if (CAddonBase::m_interface->globalSingleInstance != nullptr)
      throw std::logic_error("kodi::addon::CInstanceScreensaver: Creation of more as one in single "
                             "instance way is not allowed!");

    SetAddonStruct(CAddonBase::m_interface->firstKodiInstance);
    CAddonBase::m_interface->globalSingleInstance = this;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_screensaver
  /// @brief Screensaver class constructor used to support multiple instance
  /// types.
  ///
  /// @param[in] instance The instance value given to
  ///                     <b>`kodi::addon::CAddonBase::CreateInstance(...)`</b>.
  /// @param[in] kodiVersion [opt] Version used in Kodi for this instance, to
  ///                        allow compatibility to older Kodi versions.
  ///
  /// @note Recommended to set <b>`kodiVersion`</b>.
  ///
  ///
  /// --------------------------------------------------------------------------
  ///
  /// **Here's example about the use of this:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// class CMyScreenSaver : public kodi::addon::CInstanceScreensaver
  /// {
  /// public:
  ///   CMyScreenSaver(KODI_HANDLE instance, const std::string& kodiVersion)
  ///     : kodi::addon::CInstanceScreensaver(instance, kodiVersion)
  ///   {
  ///      ...
  ///   }
  ///
  ///   ...
  /// };
  ///
  /// ADDON_STATUS CMyAddon::CreateInstance(int instanceType,
  ///                                       const std::string& instanceID,
  ///                                       KODI_HANDLE instance,
  ///                                       const std::string& version,
  ///                                       KODI_HANDLE& addonInstance)
  /// {
  ///   kodi::Log(ADDON_LOG_INFO, "Creating my screensaver");
  ///   addonInstance = new CMyScreenSaver(instance, version);
  ///   return ADDON_STATUS_OK;
  /// }
  /// ~~~~~~~~~~~~~
  ///
  explicit CInstanceScreensaver(KODI_HANDLE instance, const std::string& kodiVersion = "")
    : IAddonInstance(ADDON_INSTANCE_SCREENSAVER,
                     !kodiVersion.empty() ? kodiVersion
                                          : GetKodiTypeVersion(ADDON_INSTANCE_SCREENSAVER))
  {
    if (CAddonBase::m_interface->globalSingleInstance != nullptr)
      throw std::logic_error("kodi::addon::CInstanceScreensaver: Creation of multiple together "
                             "with single instance way is not allowed!");

    SetAddonStruct(instance);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_screensaver
  /// @brief Destructor.
  ///
  ~CInstanceScreensaver() override = default;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_screensaver
  /// @brief Used to notify the screensaver that it has been started.
  ///
  /// @return true if the screensaver was started successfully, false otherwise
  ///
  virtual bool Start() { return true; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_screensaver
  /// @brief Used to inform the screensaver that the rendering control was
  /// stopped.
  ///
  virtual void Stop() {}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_screensaver
  /// @brief Used to indicate when the add-on should render
  ///
  virtual void Render() {}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @defgroup cpp_kodi_addon_screensaver_CB Information functions
  /// @ingroup cpp_kodi_addon_screensaver
  /// @brief **To get info about the device, display and several other parts**
  ///
  ///@{

  //============================================================================
  /// @ingroup cpp_kodi_addon_screensaver_CB
  /// @brief Device that represents the display adapter.
  ///
  /// @return A pointer to the device
  ///
  /// @note This is only available on **DirectX**, It us unused (`nullptr`) on
  /// **OpenGL**
  ///
  /// This value can also be becomed by @ref kodi::gui::GetHWContext() and is
  /// recommended to use.
  ///
  ///-------------------------------------------------------------------------
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// #include <d3d11_1.h>
  /// ..
  /// // Note: Device() there is used inside addon child class about
  /// // kodi::addon::CInstanceVisualization
  /// ID3D11DeviceContext1* context = static_cast<ID3D11DeviceContext1*>(kodi::addon::CInstanceVisualization::Device());
  /// ..
  /// ~~~~~~~~~~~~~
  ///
  inline kodi::HardwareContext Device() { return m_instanceData->props->device; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_screensaver_CB
  /// @brief Returns the X position of the rendering window.
  ///
  /// @return The X position, in pixels
  ///
  inline int X() { return m_instanceData->props->x; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_screensaver_CB
  /// @brief Returns the Y position of the rendering window.
  ///
  /// @return The Y position, in pixels
  ///
  inline int Y() { return m_instanceData->props->y; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_screensaver_CB
  /// @brief Returns the width of the rendering window.
  ///
  /// @return The width, in pixels
  ///
  inline int Width() { return m_instanceData->props->width; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_screensaver_CB
  /// @brief Returns the height of the rendering window.
  ///
  /// @return The height, in pixels
  ///
  inline int Height() { return m_instanceData->props->height; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_screensaver_CB
  /// @brief Pixel aspect ratio (often abbreviated PAR) is a ratio that
  /// describes how the width of a pixel compares to the height of that pixel.
  ///
  /// @return The pixel aspect ratio used by the display
  ///
  inline float PixelRatio() { return m_instanceData->props->pixelRatio; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_screensaver_CB
  /// @brief Used to get the name of the add-on defined in `addon.xml`.
  ///
  /// @return The add-on name
  ///
  inline std::string Name() { return m_instanceData->props->name; }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  /// @ingroup cpp_kodi_addon_screensaver_CB
  /// @brief Used to get the full path where the add-on is installed.
  ///
  /// @return The add-on installation path
  ///
  inline std::string Presets() { return m_instanceData->props->presets; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_screensaver_CB
  /// @brief Used to get the full path to the add-on's user profile.
  ///
  /// @note The trailing folder (consisting of the add-on's ID) is not created
  /// by default. If it is needed, you must call kodi::vfs::CreateDirectory()
  /// to create the folder.
  ///
  /// @return Path to the user profile
  ///
  inline std::string Profile() { return m_instanceData->props->profile; }
  //----------------------------------------------------------------------------

  ///@}

private:
  void SetAddonStruct(KODI_HANDLE instance)
  {
    if (instance == nullptr)
      throw std::logic_error("kodi::addon::CInstanceScreensaver: Creation with empty addon "
                             "structure not allowed, table must be given from Kodi!");

    m_instanceData = static_cast<AddonInstance_Screensaver*>(instance);
    m_instanceData->toAddon->addonInstance = this;
    m_instanceData->toAddon->Start = ADDON_Start;
    m_instanceData->toAddon->Stop = ADDON_Stop;
    m_instanceData->toAddon->Render = ADDON_Render;
  }

  inline static bool ADDON_Start(AddonInstance_Screensaver* instance)
  {
    CInstanceScreensaver* thisClass =
        static_cast<CInstanceScreensaver*>(instance->toAddon->addonInstance);
    thisClass->m_renderHelper = kodi::gui::GetRenderHelper();
    return thisClass->Start();
  }

  inline static void ADDON_Stop(AddonInstance_Screensaver* instance)
  {
    CInstanceScreensaver* thisClass =
        static_cast<CInstanceScreensaver*>(instance->toAddon->addonInstance);
    thisClass->Stop();
    thisClass->m_renderHelper = nullptr;
  }

  inline static void ADDON_Render(AddonInstance_Screensaver* instance)
  {
    CInstanceScreensaver* thisClass =
        static_cast<CInstanceScreensaver*>(instance->toAddon->addonInstance);

    if (!thisClass->m_renderHelper)
      return;
    thisClass->m_renderHelper->Begin();
    thisClass->Render();
    thisClass->m_renderHelper->End();
  }

  /*
   * Background render helper holds here and in addon base.
   * In addon base also to have for the others, and stored here for the worst
   * case where this class is independent from base and base becomes closed
   * before.
   *
   * This is on Kodi with GL unused and the calls to there are empty (no work)
   * On Kodi with Direct X where angle is present becomes this used.
   */
  std::shared_ptr<kodi::gui::IRenderHelper> m_renderHelper;
  AddonInstance_Screensaver* m_instanceData;
};

} /* namespace addon */
} /* namespace kodi */
#endif /* __cplusplus */
