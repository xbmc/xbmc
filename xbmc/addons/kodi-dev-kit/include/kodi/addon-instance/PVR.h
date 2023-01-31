/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../c-api/addon-instance/pvr.h"
#include "pvr/ChannelGroups.h"
#include "pvr/Channels.h"
#include "pvr/EDL.h"
#include "pvr/EPG.h"
#include "pvr/General.h"
#include "pvr/MenuHook.h"
#include "pvr/Providers.h"
#include "pvr/Recordings.h"
#include "pvr/Stream.h"
#include "pvr/Timers.h"

#ifdef __cplusplus

/*!
 * @internal
 * @brief PVR "C++" API interface
 *
 * In this field are the pure addon-side C++ data.
 *
 * @note Changes can be made without problems and have no influence on other
 * PVR addons that have already been created.\n
 * \n
 * Therefore, @ref ADDON_INSTANCE_VERSION_PVR_MIN can be ignored for these
 * fields and only the @ref ADDON_INSTANCE_VERSION_PVR needs to be increased.\n
 * \n
 * Only must be min version increased if a new compile of addon breaks after
 * changes here.
 *
 * Have by add of new parts a look about **Doxygen** `\@ingroup`, so that
 * added parts included in documentation.
 *
 * If you add addon side related documentation, where his dev need know, use `///`.
 * For parts only for Kodi make it like here.
 *
 * @endinternal
 */

namespace kodi
{
namespace addon
{

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// "C++" Doxygen group set for the definitions
//{{{

//==============================================================================
/// @defgroup cpp_kodi_addon_pvr_Defs Definitions, structures and enumerators
/// @ingroup cpp_kodi_addon_pvr
/// @brief **PVR client add-on instance definition values**\n
/// All PVR functions associated data structures.
///
/// Used to exchange the available options between Kodi and addon.\n
/// The groups described here correspond to the groups of functions on PVR
/// instance class.
///

//##############################################################################
/// @defgroup cpp_kodi_addon_pvr_Defs_General 1. General
/// @ingroup cpp_kodi_addon_pvr_Defs
/// @brief **PVR add-on general variables**\n
/// Used to exchange the available options between Kodi and addon.
///
/// This group also includes @ref cpp_kodi_addon_pvr_Defs_PVRCapabilities with
/// which Kodi an @ref kodi::addon::CInstancePVRClient::GetCapabilities()
/// queries the supported **modules** of the addon.
///
/// The standard values are also below, once for error messages and once to
/// @ref kodi::addon::CInstancePVRClient::ConnectionStateChange() to give Kodi
/// any information.
///
///@{
//##############################################################################
/// @defgroup cpp_kodi_addon_pvr_Defs_General_Inputstream class PVRStreamProperty & definition PVR_STREAM_PROPERTY
/// @ingroup cpp_kodi_addon_pvr_Defs_General
/// @brief **Inputstream variables**\n
/// This includes values related to the outside of PVR available inputstream
/// system.
///
/// This can be by separate instance on same addon, by handling in Kodi itself
/// or to reference of another addon where support needed inputstream.
///
/// @note This is complete independent from own system included here
/// @ref cpp_kodi_addon_pvr_Streams "inputstream".
///
//------------------------------------------------------------------------------
///@}

//##############################################################################
/// @defgroup cpp_kodi_addon_pvr_Defs_Channel 2. Channel
/// @ingroup cpp_kodi_addon_pvr_Defs
/// @brief **PVR add-on channel**\n
/// Used to exchange the available channel options between Kodi and addon.
///
/// Modules here are mainly intended for @ref cpp_kodi_addon_pvr_Channels "channels",
/// but are also used on other modules to identify the respective TV/radio
/// channel.
///
/// Because of @ref cpp_kodi_addon_pvr_Defs_Channel_PVRSignalStatus and
/// @ref cpp_kodi_addon_pvr_Defs_Channel_PVRDescrambleInfo is a special case at
/// this point. This is currently only used on running streams, but it may be
/// possible that this must always be usable in connection with PiP in the
/// future.
///
//------------------------------------------------------------------------------

//##############################################################################
/// @defgroup cpp_kodi_addon_pvr_Defs_ChannelGroup 3. Channel Group
/// @ingroup cpp_kodi_addon_pvr_Defs
/// @brief **PVR add-on channel group**\n
/// This group contains data classes and values which are used in PVR on
/// @ref cpp_kodi_addon_pvr_supportsChannelGroups "channel groups".
///
//------------------------------------------------------------------------------

//##############################################################################
/// @defgroup cpp_kodi_addon_pvr_Defs_epg 4. EPG Tag
/// @ingroup cpp_kodi_addon_pvr_Defs
/// @brief **PVR add-on EPG data**\n
/// Used on @ref cpp_kodi_addon_pvr_EPGTag "EPG methods in PVR instance class".
///
/// See related modules about, also below in this view are few macros where
/// default values of associated places.
///
//------------------------------------------------------------------------------

//##############################################################################
/// @defgroup cpp_kodi_addon_pvr_Defs_Recording 5. Recording
/// @ingroup cpp_kodi_addon_pvr_Defs
/// @brief **Representation of a recording**\n
/// Used to exchange the available recording data between Kodi and addon on
/// @ref cpp_kodi_addon_pvr_Recordings "Recordings methods in PVR instance class".
///
//------------------------------------------------------------------------------

//##############################################################################
/// @defgroup cpp_kodi_addon_pvr_Defs_Timer 6. Timer
/// @ingroup cpp_kodi_addon_pvr_Defs
/// @brief **PVR add-on timer data**\n
/// Used to exchange the available timer data between Kodi and addon on
/// @ref cpp_kodi_addon_pvr_Timers "Timers methods in PVR instance class".
///
//------------------------------------------------------------------------------

//##############################################################################
/// @defgroup cpp_kodi_addon_pvr_Defs_Provider 7. Provider
/// @ingroup cpp_kodi_addon_pvr_Defs
/// @brief **Representation of a provider**\n
/// For list of all providers from the backend.
///
//------------------------------------------------------------------------------

//##############################################################################
/// @defgroup cpp_kodi_addon_pvr_Defs_Menuhook 8. Menuhook
/// @ingroup cpp_kodi_addon_pvr_Defs
/// @brief **PVR Context menu data**\n
/// Define data for the context menus available to the user
///
//------------------------------------------------------------------------------

//##############################################################################
/// @defgroup cpp_kodi_addon_pvr_Defs_EDLEntry 9. Edit decision list (EDL)
/// @ingroup cpp_kodi_addon_pvr_Defs
/// @brief **An edit decision list or EDL is used in the post-production process
/// of film editing and video editing**\n
/// Used on @ref kodi::addon::CInstancePVRClient::GetEPGTagEdl and
/// @ref kodi::addon::CInstancePVRClient::GetRecordingEdl
///
//------------------------------------------------------------------------------

//##############################################################################
/// @defgroup cpp_kodi_addon_pvr_Defs_Stream 10. Inputstream
/// @ingroup cpp_kodi_addon_pvr_Defs
/// @brief **Inputstream**\n
/// This includes classes and values that are used in the PVR inputstream.
///
/// Used on @ref cpp_kodi_addon_pvr_Streams "Inputstream methods in PVR instance class".
///
/// @note The parts here will be removed in the future and replaced by the
/// separate @ref cpp_kodi_addon_inputstream "inputstream addon instance".
/// If there is already a possibility, new addons should do it via the
/// inputstream instance.
///
//------------------------------------------------------------------------------

//}}}
//______________________________________________________________________________

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// "C++" PVR addon instance class
//{{{

//==============================================================================
/// @addtogroup cpp_kodi_addon_pvr
/// @brief \cpp_class{ kodi::addon::CInstancePVRClient }
/// **PVR client add-on instance**
///
/// Kodi features powerful [Live TV](https://kodi.wiki/view/Live_TV) and
/// [video recording (DVR/PVR)](http://en.wikipedia.org/wiki/Digital_video_recorder)
/// abilities using a very flexible distributed application structure. That is, by
/// leveraging other existing third-party
/// [PVR backend applications](https://kodi.wiki/view/PVR_backend) or
/// [DVR devices](https://kodi.wiki/view/PVR_backend)
/// that specialize in receiving television signals and also support the same type
/// of [client–server model](http://en.wikipedia.org/wiki/client%E2%80%93server_model)
/// which Kodi uses, (following a [frontend-backend](http://en.wikipedia.org/wiki/Front_and_back_ends)
/// design principle for [separation of concerns](http://en.wikipedia.org/wiki/Separation_of_concerns)),
/// these PVR features in Kodi allow you to watch Live TV, listen to radio, view an EPG TV-Guide
/// and schedule recordings, and also enables many other TV related features, all using
/// Kodi as your primary interface once the initial pairing connection and
/// configuration have been done.
///
/// @note It is very important to understand that with "Live TV" in the reference
/// to PVR in Kodi, we do not mean [streaming video](http://en.wikipedia.org/wiki/Streaming_media)
/// from the internet via websites providing [free content](https://kodi.wiki/view/Free_content)
/// or online services such as Netflix, Hulu, Vudu and similar, no matter if that
/// content is actually streamed live or not. If that is what you are looking for
/// then you might want to look into [Video Addons](https://kodi.wiki/view/Add-ons)
/// for Kodi instead, (which again is not the same as the "PVR" or "Live TV" we
/// discuss in this article), but remember that [Kodi does not provide any video
/// content or video streaming services](https://kodi.wiki/view/Free_content).
///
/// The use of the PVR is based on the @ref CInstancePVRClient.
///
/// Include the header @ref PVR.h "#include <kodi/addon-instance/PVR.h>"
/// to use this class.
///
///
/// ----------------------------------------------------------------------------
///
/// Here is an example of what the <b>`addon.xml.in`</b> would look like for an PVR addon:
///
/// ~~~~~~~~~~~~~{.xml}
/// <?xml version="1.0" encoding="UTF-8"?>
/// <addon
///   id="pvr.myspecialnamefor"
///   version="1.0.0"
///   name="My special PVR addon"
///   provider-name="Your Name">
///   <requires>@ADDON_DEPENDS@</requires>
///   <extension
///     point="kodi.pvrclient"
///     library_@PLATFORM@="@LIBRARY_FILENAME@"/>
///   <extension point="xbmc.addon.metadata">
///     <summary lang="en_GB">My PVR addon addon</summary>
///     <description lang="en_GB">My PVR addon description</description>
///     <platform>@PLATFORM@</platform>
///   </extension>
/// </addon>
/// ~~~~~~~~~~~~~
///
///
/// At <b>`<extension point="kodi.pvrclient" ...>`</b> the basic instance definition is declared, this is intended to identify the addon as an PVR and to see its supported types:
/// | Name | Description
/// |------|----------------------
/// | <b>`point`</b> | The identification of the addon instance to inputstream is mandatory <b>`kodi.pvrclient`</b>. In addition, the instance declared in the first <b>`<extension ... />`</b> is also the main type of addon.
/// | <b>`library_@PLATFORM@`</b> | The runtime library used for the addon. This is usually declared by cmake and correctly displayed in the translated `addon.xml`.
///
///
/// @remark For more detailed description of the <b>`addon.xml`</b>, see also https://kodi.wiki/view/Addon.xml.
///
///
/// --------------------------------------------------------------------------
///
/// **Example:**
///
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/addon-instance/PVR.h>
///
/// class CMyPVRClient : public ::kodi::addon::CInstancePVRClient
/// {
/// public:
///   CMyPVRClient(const kodi::addon::IInstanceInfo& instance);
///
///   PVR_ERROR GetCapabilities(kodi::addon::PVRCapabilities& capabilities) override;
///   PVR_ERROR GetBackendName(std::string& name) override;
///   PVR_ERROR GetBackendVersion(std::string& version) override;
///
///   PVR_ERROR GetProvidersAmount(int& amount) override;
///   PVR_ERROR GetProviders(std::vector<kodi::addon::PVRProvider>& providers) override;
///   PVR_ERROR GetChannelsAmount(int& amount) override;
///   PVR_ERROR GetChannels(bool radio, std::vector<kodi::addon::PVRChannel>& channels) override;
///   PVR_ERROR GetChannelStreamProperties(const kodi::addon::PVRChannel&	channel,
///                                        std::vector<kodi::addon::PVRStreamProperty>& properties) override;
///
/// private:
///   std::vector<kodi::addon::PVRChannel> m_myChannels;
/// };
///
/// CMyPVRClient::CMyPVRClient(const kodi::addon::IInstanceInfo& instance)
///   : CInstancePVRClient(instance)
/// {
///   kodi::addon::PVRChannel channel;
///   channel.SetUniqueId(123);
///   channel.SetChannelNumber(1);
///   channel.SetChannelName("My test channel");
///   m_myChannels.push_back(channel);
/// }
///
/// PVR_ERROR CMyPVRClient::GetCapabilities(kodi::addon::PVRCapabilities& capabilities)
/// {
///   capabilities.SetSupportsTV(true);
///   return PVR_ERROR_NO_ERROR;
/// }
///
/// PVR_ERROR CMyPVRClient::GetBackendName(std::string& name)
/// {
///   name = "My special PVR client";
///   return PVR_ERROR_NO_ERROR;
/// }
///
/// PVR_ERROR CMyPVRClient::GetBackendVersion(std::string& version)
/// {
///   version = "1.0.0";
///   return PVR_ERROR_NO_ERROR;
/// }
///
/// PVR_ERROR CMyInstance::GetProvidersAmount(int& amount)
/// {
///   amount = m_myProviders.size();
///   return PVR_ERROR_NO_ERROR;
/// }
///
/// PVR_ERROR CMyPVRClient::GetProviders(std::vector<kodi::addon::PVRProvider>& providers)
/// {
///   providers = m_myProviders;
///   return PVR_ERROR_NO_ERROR;
/// }
///
/// PVR_ERROR CMyInstance::GetChannelsAmount(int& amount)
/// {
///   amount = m_myChannels.size();
///   return PVR_ERROR_NO_ERROR;
/// }
///
/// PVR_ERROR CMyPVRClient::GetChannels(bool radio, std::vector<kodi::addon::PVRChannel>& channels)
/// {
///   channels = m_myChannels;
///   return PVR_ERROR_NO_ERROR;
/// }
///
/// PVR_ERROR CMyPVRClient::GetChannelStreamProperties(const kodi::addon::PVRChannel& channel,
///                                                    std::vector<kodi::addon::PVRStreamProperty>& properties)
/// {
///   if (channel.GetUniqueId() == 123)
///   {
///     properties.push_back(PVR_STREAM_PROPERTY_STREAMURL, "http://distribution.bbb3d.renderfarming.net/video/mp4/bbb_sunflower_1080p_30fps_normal.mp4");
///     properties.push_back(PVR_STREAM_PROPERTY_ISREALTIMESTREAM, "true");
///     return PVR_ERROR_NO_ERROR;
///   }
///   return PVR_ERROR_UNKNOWN;
/// }
///
/// ...
///
/// //----------------------------------------------------------------------
///
/// class CMyAddon : public ::kodi::addon::CAddonBase
/// {
/// public:
///   CMyAddon() = default;
///   ADDON_STATUS CreateInstance(const kodi::addon::IInstanceInfo& instance,
///                               KODI_ADDON_INSTANCE_HDL& hdl) override;
/// };
///
/// // If you use only one instance in your add-on, can be instanceType and
/// // instanceID ignored
/// ADDON_STATUS CMyAddon::CreateInstance(const kodi::addon::IInstanceInfo& instance,
///                                       KODI_ADDON_INSTANCE_HDL& hdl)
/// {
///   if (instance.IsType(ADDON_INSTANCE_PVR))
///   {
///     kodi::Log(ADDON_LOG_INFO, "Creating my PVR client instance");
///     hdl = new CMyPVRClient(instance, version);
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
/// The destruction of the example class `CMyPVRClient` is called from
/// Kodi's header. Manually deleting the add-on instance is not required.
///
class ATTR_DLL_LOCAL CInstancePVRClient : public IAddonInstance
{
public:
  //============================================================================
  /// @defgroup cpp_kodi_addon_pvr_Base 1. Basic functions
  /// @ingroup cpp_kodi_addon_pvr
  /// @brief **Functions to manage the addon and get basic information about it**\n
  /// These are e.g. @ref GetCapabilities to know supported groups at
  /// this addon or the others to get information about the source of the PVR
  /// stream.
  ///
  /// The with "Valid implementation required." declared functions are mandatory,
  /// all others are an option.
  ///
  ///
  ///---------------------------------------------------------------------------
  ///
  /// **Basic parts in interface:**\n
  /// Copy this to your project and extend with your parts or leave functions
  /// complete away where not used or supported.
  ///
  /// @copydetails cpp_kodi_addon_pvr_Base_header_addon_auto_check
  /// @copydetails cpp_kodi_addon_pvr_Base_source_addon_auto_check
  ///
  ///@{

  //============================================================================
  /// @brief PVR client class constructor.
  ///
  /// Used by an add-on that only supports only PVR and only in one instance.
  ///
  ///
  /// --------------------------------------------------------------------------
  ///
  /// **Here's example about the use of this:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// #include <kodi/addon-instance/PVR.h>
  /// ...
  ///
  /// class ATTR_DLL_LOCAL CPVRExample
  ///   : public kodi::addon::CAddonBase,
  ///     public kodi::addon::CInstancePVRClient
  /// {
  /// public:
  ///   CPVRExample()
  ///   {
  ///   }
  ///
  ///   ~CPVRExample() override;
  ///   {
  ///   }
  ///
  ///   ...
  /// };
  ///
  /// ADDONCREATOR(CPVRExample)
  /// ~~~~~~~~~~~~~
  ///
  CInstancePVRClient() : IAddonInstance(IInstanceInfo(CPrivateBase::m_interface->firstKodiInstance))
  {
    if (CPrivateBase::m_interface->globalSingleInstance != nullptr)
      throw std::logic_error("kodi::addon::CInstancePVRClient: Creation of more as one in single "
                             "instance way is not allowed!");

    SetAddonStruct(CPrivateBase::m_interface->firstKodiInstance);
    CPrivateBase::m_interface->globalSingleInstance = this;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief PVR client class constructor used to support multiple instance
  /// types.
  ///
  /// @param[in] instance The instance value given to
  ///                     <b>`kodi::addon::CAddonBase::CreateInstance(...)`</b>.
  ///
  ///
  /// --------------------------------------------------------------------------
  ///
  /// **Here's example about the use of this:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// class CMyPVRClient : public ::kodi::addon::CInstancePVRClient
  /// {
  /// public:
  ///   CMyPVRClient(const kodi::addon::IInstanceInfo& instance)
  ///     : CInstancePVRClient(instance)
  ///   {
  ///      ...
  ///   }
  ///
  ///   ...
  /// };
  ///
  /// ADDON_STATUS CMyAddon::CreateInstance(const kodi::addon::IInstanceInfo& instance,
  ///                                       KODI_ADDON_INSTANCE_HDL& hdl)
  /// {
  ///   kodi::Log(ADDON_LOG_INFO, "Creating my PVR client instance");
  ///   hdl = new CMyPVRClient(instance);
  ///   return ADDON_STATUS_OK;
  /// }
  /// ~~~~~~~~~~~~~
  ///
  explicit CInstancePVRClient(const IInstanceInfo& instance) : IAddonInstance(instance)
  {
    if (CPrivateBase::m_interface->globalSingleInstance != nullptr)
      throw std::logic_error("kodi::addon::CInstancePVRClient: Creation of multiple together with "
                             "single instance way is not allowed!");

    SetAddonStruct(instance);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Destructor
  ///
  ~CInstancePVRClient() override = default;
  //----------------------------------------------------------------------------

  //--==----==----==----==----==----==----==----==----==----==----==----==----==

  //============================================================================
  /// @brief Get the list of features that this add-on provides.
  ///
  /// Called by Kodi to query the add-on's capabilities.
  /// Used to check which options should be presented in the UI, which methods to call, etc.
  /// All capabilities that the add-on supports should be set to true.
  ///
  /// @param capabilities The with @ref cpp_kodi_addon_pvr_Defs_PVRCapabilities defined add-on's capabilities.
  /// @return @ref PVR_ERROR_NO_ERROR if the properties were fetched successfully.
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_pvr_Defs_PVRCapabilities_Help
  ///
  ///
  /// --------------------------------------------------------------------------
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// PVR_ERROR CMyPVRClient::GetCapabilities(kodi::addon::PVRCapabilities& capabilities)
  /// {
  ///   capabilities.SetSupportsTV(true);
  ///   capabilities.SetSupportsEPG(true);
  ///   return PVR_ERROR_NO_ERROR;
  /// }
  /// ~~~~~~~~~~~~~
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @note Valid implementation required.
  ///
  virtual PVR_ERROR GetCapabilities(kodi::addon::PVRCapabilities& capabilities) = 0;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Get the name reported by the backend that will be displayed in the UI.
  ///
  /// @param[out] name The name reported by the backend that will be displayed in the UI.
  /// @return @ref PVR_ERROR_NO_ERROR if successfully done
  ///
  ///
  /// --------------------------------------------------------------------------
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// PVR_ERROR CMyPVRClient::GetBackendName(std::string& name)
  /// {
  ///   name = "My special PVR client";
  ///   return PVR_ERROR_NO_ERROR;
  /// }
  /// ~~~~~~~~~~~~~
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @note Valid implementation required.
  ///
  virtual PVR_ERROR GetBackendName(std::string& name) = 0;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Get the version string reported by the backend that will be
  /// displayed in the UI.
  ///
  /// @param[out] version The version string reported by the backend that will be
  ///                     displayed in the UI.
  /// @return @ref PVR_ERROR_NO_ERROR if successfully done
  ///
  ///
  /// --------------------------------------------------------------------------
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// PVR_ERROR CMyPVRClient::GetBackendVersion(std::string& version)
  /// {
  ///   version = "1.0.0";
  ///   return PVR_ERROR_NO_ERROR;
  /// }
  /// ~~~~~~~~~~~~~
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @note Valid implementation required.
  ///
  virtual PVR_ERROR GetBackendVersion(std::string& version) = 0;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Get the hostname of the pvr backend server
  ///
  /// @param[out] hostname Hostname as ip address or alias. If backend does not
  ///                      utilize a server, return empty string.
  /// @return @ref PVR_ERROR_NO_ERROR if successfully done
  ///
  virtual PVR_ERROR GetBackendHostname(std::string& hostname) { return PVR_ERROR_NOT_IMPLEMENTED; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief To get the connection string reported by the backend that will be
  /// displayed in the UI.
  ///
  /// @param[out] connection The connection string reported by the backend that
  ///                        will be displayed in the UI.
  /// @return @ref PVR_ERROR_NO_ERROR if successfully done
  ///
  virtual PVR_ERROR GetConnectionString(std::string& connection)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Get the disk space reported by the backend (if supported).
  ///
  /// @param[in] total The total disk space in KiB.
  /// @param[in] used The used disk space in KiB.
  /// @return @ref PVR_ERROR_NO_ERROR if the drive space has been fetched
  ///         successfully.
  ///
  ///
  /// --------------------------------------------------------------------------
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// PVR_ERROR CMyPVRClient::GetDriveSpace(uint64_t& total, uint64_t& used)
  /// {
  ///   total = 100 * 1024 * 1024; // To set complete size of drive in KiB (100GB)
  ///   used =  12232424; // To set the used amount
  ///   return PVR_ERROR_NO_ERROR;
  /// }
  /// ~~~~~~~~~~~~~
  ///
  virtual PVR_ERROR GetDriveSpace(uint64_t& total, uint64_t& used)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Call one of the settings related menu hooks (if supported).
  ///
  /// Supported @ref cpp_kodi_addon_pvr_Defs_Menuhook_PVRMenuhook "menu hook "
  /// instances have to be added in `constructor()`, by calling @ref AddMenuHook()
  /// on the callback.
  ///
  /// @param[in] menuhook The hook to call.
  /// @return @ref PVR_ERROR_NO_ERROR if the hook was called successfully.
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_pvr_Defs_Menuhook_PVRMenuhook_Help
  ///
  ///
  /// --------------------------------------------------------------------------
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// PVR_ERROR CMyPVRClient::CallSettingsMenuHook(const kodi::addon::PVRMenuhook& menuhook)
  /// {
  ///   if (menuhook.GetHookId() == 2)
  ///     kodi::QueueNotification(QUEUE_INFO, "", kodi::GetLocalizedString(menuhook.GetLocalizedStringId()));
  ///   return PVR_ERROR_NO_ERROR;
  /// }
  /// ~~~~~~~~~~~~~
  ///
  virtual PVR_ERROR CallSettingsMenuHook(const kodi::addon::PVRMenuhook& menuhook)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //==========================================================================
  /// @brief **Callback to Kodi Function**\nAdd or replace a menu hook for the context menu for this add-on
  ///
  /// This is a callback function, called from addon to give Kodi his context menu's.
  ///
  /// @param[in] menuhook The with @ref cpp_kodi_addon_pvr_Defs_Menuhook_PVRMenuhook defined hook to add
  ///
  /// @remarks Only called from addon itself
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_pvr_Defs_Menuhook_PVRMenuhook_Help
  ///
  ///
  /// --------------------------------------------------------------------------
  ///
  /// **Here's an example of the use of it:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// #include <kodi/addon-instance/PVR.h>
  /// ...
  ///
  /// {
  ///   kodi::addon::PVRMenuhook hook;
  ///   hook.SetHookId(1);
  ///   hook.SetCategory(PVR_MENUHOOK_CHANNEL);
  ///   hook.SetLocalizedStringId(30000);
  ///   AddMenuHook(hook);
  /// }
  ///
  /// {
  ///   kodi::addon::PVRMenuhook hook;
  ///   hook.SetHookId(2);
  ///   hook.SetCategory(PVR_MENUHOOK_SETTING);
  ///   hook.SetLocalizedStringId(30001);
  ///   AddMenuHook(hook);
  /// }
  /// ...
  /// ~~~~~~~~~~~~~
  ///
  /// **Here another way:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// #include <kodi/addon-instance/PVR.h>
  /// ...
  ///
  /// AddMenuHook(kodi::addon::PVRMenuhook(1, 30000, PVR_MENUHOOK_CHANNEL));
  /// AddMenuHook(kodi::addon::PVRMenuhook(2, 30001, PVR_MENUHOOK_SETTING));
  /// ...
  /// ~~~~~~~~~~~~~
  ///
  inline void AddMenuHook(const kodi::addon::PVRMenuhook& hook)
  {
    m_instanceData->toKodi->AddMenuHook(m_instanceData->toKodi->kodiInstance, hook);
  }
  //----------------------------------------------------------------------------

  //==========================================================================
  /// @brief **Callback to Kodi Function**\n
  /// Notify a state change for a PVR backend connection.
  ///
  /// @param[in] connectionString The connection string reported by the backend
  ///                             that can be displayed in the UI.
  /// @param[in] newState The by @ref PVR_CONNECTION_STATE defined new state.
  /// @param[in] message A localized addon-defined string representing the new
  ///                    state, that can be displayed in the UI or **empty** if
  ///                    the Kodi-defined default string for the new state
  ///                    shall be displayed.
  ///
  /// @remarks Only called from addon itself
  ///
  ///
  /// --------------------------------------------------------------------------
  ///
  ///
  /// **Here's an example of the use of it:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// #include <kodi/addon-instance/PVR.h>
  /// #include <kodi/General.h> /* for kodi::GetLocalizedString(...) */
  /// ...
  ///
  ///   ConnectionStateChange("PVR demo connection lost", PVR_CONNECTION_STATE_DISCONNECTED, kodi::GetLocalizedString(30005, "Lost connection to Server"););
  /// ...
  /// ~~~~~~~~~~~~~
  ///
  inline void ConnectionStateChange(const std::string& connectionString,
                                    PVR_CONNECTION_STATE newState,
                                    const std::string& message)
  {
    m_instanceData->toKodi->ConnectionStateChange(
        m_instanceData->toKodi->kodiInstance, connectionString.c_str(), newState, message.c_str());
  }
  //----------------------------------------------------------------------------

  //==========================================================================
  /// @brief **Callback to Kodi Function**\n
  /// Get user data path of the PVR addon.
  ///
  /// @return Path of current Kodi user
  ///
  /// @remarks Only called from addon itself
  ///
  /// @note Alternatively, @ref kodi::GetAddonPath() can be used for this.
  ///
  inline std::string UserPath() const { return m_instanceData->props->strUserPath; }
  //----------------------------------------------------------------------------

  //==========================================================================
  /// @brief **Callback to Kodi Function**\n
  /// Get main client path of the PVR addon.
  ///
  /// @return Path of addon client
  ///
  /// @remarks Only called from addon itself.
  ///
  /// @note Alternatively, @ref kodi::GetBaseUserPath() can be used for this.
  ///
  inline std::string ClientPath() const { return m_instanceData->props->strClientPath; }
  //----------------------------------------------------------------------------

  ///@}
  //--==----==----==----==----==----==----==----==----==----==----==----==----==

  //============================================================================
  /// @defgroup cpp_kodi_addon_pvr_Channels 2. Channels (required)
  /// @ingroup cpp_kodi_addon_pvr
  /// @brief **Functions to get available TV or Radio channels**\n
  /// These are mandatory functions for using this addon to get the available
  /// channels.
  ///
  /// @remarks Either @ref PVRCapabilities::SetSupportsTV "SetSupportsTV()" or
  /// @ref PVRCapabilities::SetSupportsRadio "SetSupportsRadio()" is required to
  /// be set to <b>`true`</b>.\n
  /// If a channel changes after the initial import, or if a new one was added,
  /// then the add-on should call @ref TriggerChannelUpdate().
  ///
  ///
  ///---------------------------------------------------------------------------
  ///
  /// **Channel parts in interface:**\n
  /// Copy this to your project and extend with your parts or leave functions
  /// complete away where not used or supported.
  ///
  /// @copydetails cpp_kodi_addon_pvr_Channels_header_addon_auto_check
  /// @copydetails cpp_kodi_addon_pvr_Channels_source_addon_auto_check
  ///
  ///@{

  //============================================================================
  /// @brief The total amount of providers on the backend
  ///
  /// @param[out] amount The total amount of providers on the backend
  /// @return @ref PVR_ERROR_NO_ERROR if the amount has been fetched successfully.
  ///
  /// @remarks Optional, and only used if @ref PVRCapabilities::SetSupportsProviders
  ///          "supportsProviders" is set to true.
  ///
  virtual PVR_ERROR GetProvidersAmount(int& amount) { return PVR_ERROR_NOT_IMPLEMENTED; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Request the list of all providers from the backend.
  ///
  /// @param[out] results The channels defined with
  ///                     @ref cpp_kodi_addon_pvr_Defs_PVRProvider and
  ///                     available at the addon, then transferred with
  ///                     @ref cpp_kodi_addon_pvr_Defs_PVRProvidersResultSet.
  /// @return @ref PVR_ERROR_NO_ERROR if the list has been fetched successfully.
  ///
  /// @remarks Optional, and only used if @ref PVRCapabilities::SetSupportsProviders
  ///          "supportsProviders" is set to true.
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_pvr_Defs_PVRProvider_Help
  ///
  ///---------------------------------------------------------------------------
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// ...
  /// PVR_ERROR CMyPVRInstance::GetProviders(kodi::addon::PVRProvidersResultSet& results)
  /// {
  ///   // Minimal demo example, in reality bigger and loop to transfer all
  ///   kodi::addon::PVRProvider provider;
  ///   provider.SetUniqueId(123);
  ///   provider.SetProviderName("My provider name");
  ///   provider.SetProviderType(PVR_PROVIDER_TYPE_SATELLITE);
  ///   ...
  ///
  ///   // Give it now to Kodi
  ///   results.Add(provider);
  ///   return PVR_ERROR_NO_ERROR;
  /// }
  /// ...
  /// ~~~~~~~~~~~~~
  ///
  virtual PVR_ERROR GetProviders(kodi::addon::PVRProvidersResultSet& results)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief **Callback to Kodi Function**\n
  /// Request Kodi to update it's list of providers.
  ///
  /// @remarks Only called from addon itself.
  ///
  inline void TriggerProvidersUpdate()
  {
    m_instanceData->toKodi->TriggerProvidersUpdate(m_instanceData->toKodi->kodiInstance);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief The total amount of channels on the backend
  ///
  /// @param[out] amount The total amount of channels on the backend
  /// @return @ref PVR_ERROR_NO_ERROR if the amount has been fetched successfully.
  ///
  /// @remarks Valid implementation required.
  ///
  virtual PVR_ERROR GetChannelsAmount(int& amount) { return PVR_ERROR_NOT_IMPLEMENTED; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Request the list of all channels from the backend.
  ///
  /// @param[in] radio True to get the radio channels, false to get the TV channels.
  /// @param[out] results The channels defined with @ref cpp_kodi_addon_pvr_Defs_Channel_PVRChannel
  ///                     and available at the addon, them transferred with
  ///                     @ref cpp_kodi_addon_pvr_Defs_Channel_PVRChannelsResultSet.
  /// @return @ref PVR_ERROR_NO_ERROR if the list has been fetched successfully.
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_pvr_Defs_Channel_PVRChannel_Help
  ///
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @remarks
  /// If @ref PVRCapabilities::SetSupportsTV() is set to
  /// <b>`true`</b>, a valid result set needs to be provided for <b>`radio = false`</b>.\n
  /// If @ref PVRCapabilities::SetSupportsRadio() is set to
  /// <b>`true`</b>, a valid result set needs to be provided for <b>`radio = true`</b>.
  /// At least one of these two must provide a valid result set.
  ///
  ///
  ///---------------------------------------------------------------------------
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// ...
  /// PVR_ERROR CMyPVRInstance::GetChannels(bool radio, kodi::addon::PVRChannelsResultSet& results)
  /// {
  ///   // Minimal demo example, in reality bigger and loop to transfer all
  ///   kodi::addon::PVRChannel channel;
  ///   channel.SetUniqueId(123);
  ///   channel.SetIsRadio(false);
  ///   channel.SetChannelNumber(1);
  ///   channel.SetChannelName("My channel name");
  ///   ...
  ///
  ///   // Give it now to Kodi
  ///   results.Add(channel);
  ///   return PVR_ERROR_NO_ERROR;
  /// }
  /// ...
  /// ~~~~~~~~~~~~~
  ///
  virtual PVR_ERROR GetChannels(bool radio, kodi::addon::PVRChannelsResultSet& results)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Get the stream properties for a channel from the backend.
  ///
  /// @param[in] channel The channel to get the stream properties for.
  /// @param[out] properties the properties required to play the stream.
  /// @return @ref PVR_ERROR_NO_ERROR if the stream is available.
  ///
  /// @remarks If @ref PVRCapabilities::SetSupportsTV "SetSupportsTV" or
  /// @ref PVRCapabilities::SetSupportsRadio "SetSupportsRadio" are set to true
  /// and @ref PVRCapabilities::SetHandlesInputStream "SetHandlesInputStream" is
  /// set to false.\n\n
  /// In this case the implementation must fill the property @ref PVR_STREAM_PROPERTY_STREAMURL
  /// with the URL Kodi should resolve to playback the channel.
  ///
  /// @note The value directly related to inputstream must always begin with the
  /// name of the associated add-on, e.g. <b>`"inputstream.adaptive.manifest_update_parameter"`</b>.
  ///
  ///
  ///---------------------------------------------------------------------------
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// ...
  /// PVR_ERROR CMyPVRInstance::GetChannelStreamProperties(const kodi::addon::PVRChannel& channel,
  ///                                                      std::vector<kodi::addon::PVRStreamProperty>& properties)
  /// {
  ///   ...
  ///   properties.emplace_back(PVR_STREAM_PROPERTY_INPUTSTREAM, "inputstream.adaptive");
  ///   properties.emplace_back("inputstream.adaptive.manifest_type", "mpd");
  ///   properties.emplace_back("inputstream.adaptive.manifest_update_parameter", "full");
  ///   properties.emplace_back(PVR_STREAM_PROPERTY_MIMETYPE, "application/xml+dash");
  ///   return PVR_ERROR_NO_ERROR;
  /// }
  /// ...
  /// ~~~~~~~~~~~~~
  ///
  virtual PVR_ERROR GetChannelStreamProperties(
      const kodi::addon::PVRChannel& channel,
      std::vector<kodi::addon::PVRStreamProperty>& properties)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Get the signal status of the stream that's currently open.
  ///
  /// @param[out] signalStatus The signal status.
  /// @return @ref PVR_ERROR_NO_ERROR if the signal status has been read successfully, false otherwise.
  ///
  /// @remarks Optional, and only used if @ref PVRCapabilities::SetHandlesInputStream "SetHandlesInputStream"
  /// is set to true.
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_pvr_Defs_Channel_PVRSignalStatus_Help
  ///
  ///
  /// --------------------------------------------------------------------------
  ///
  ///
  /// **Here's example about the use of this:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// #include <kodi/addon-instance/PVR.h>
  /// ...
  ///
  /// class ATTR_DLL_LOCAL CPVRExample
  ///   : public kodi::addon::CAddonBase,
  ///     public kodi::addon::CInstancePVRClient
  /// {
  /// public:
  ///   ...
  ///   PVR_ERROR SignalStatus(PVRSignalStatus &signalStatus) override
  ///   {
  ///     signalStatus.SetAapterName("Example adapter 1");
  ///     signalStatus.SetAdapterStatus("OK");
  ///     signalStatus.SetSignal(0xFFFF); // 100%
  ///
  ///     return PVR_ERROR_NO_ERROR;
  ///   }
  /// };
  ///
  /// ADDONCREATOR(CPVRExample)
  /// ~~~~~~~~~~~~~
  ///
  virtual PVR_ERROR GetSignalStatus(int channelUid, kodi::addon::PVRSignalStatus& signalStatus)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Get the descramble information of the stream that's currently open.
  ///
  /// @param[out] descrambleInfo The descramble information.
  /// @return @ref PVR_ERROR_NO_ERROR if the descramble information has been
  ///         read successfully, false otherwise.
  ///
  /// @remarks Optional, and only used if @ref PVRCapabilities::SetSupportsDescrambleInfo "supportsDescrambleInfo"
  /// is set to true.
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_pvr_Defs_Channel_PVRDescrambleInfo_Help
  ///
  virtual PVR_ERROR GetDescrambleInfo(int channelUid,
                                      kodi::addon::PVRDescrambleInfo& descrambleInfo)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief **Callback to Kodi Function**\n
  /// Request Kodi to update it's list of channels.
  ///
  /// @remarks Only called from addon itself.
  ///
  inline void TriggerChannelUpdate()
  {
    m_instanceData->toKodi->TriggerChannelUpdate(m_instanceData->toKodi->kodiInstance);
  }
  //----------------------------------------------------------------------------

  ///@}
  //--==----==----==----==----==----==----==----==----==----==----==----==----==

  //============================================================================
  /// @defgroup cpp_kodi_addon_pvr_supportsChannelGroups 3. Channel Groups (optional)
  /// @ingroup cpp_kodi_addon_pvr
  /// @brief <b>Bring in this functions if you have set @ref PVRCapabilities::SetSupportsChannelGroups "supportsChannelGroups"
  /// to true</b>\n
  /// This is used to divide available addon channels into groups, which can
  /// then be selected by the user.
  ///
  ///
  ///---------------------------------------------------------------------------
  ///
  /// **Channel group parts in interface:**\n
  /// Copy this to your project and extend with your parts or leave functions
  /// complete away where not used or supported.
  ///
  /// @copydetails cpp_kodi_addon_pvr_supportsChannelGroups_header_addon_auto_check
  /// @copydetails cpp_kodi_addon_pvr_supportsChannelGroups_source_addon_auto_check
  ///
  ///@{

  //============================================================================
  /// @brief Get the total amount of channel groups on the backend if it supports channel groups.
  ///
  /// @param[out] amount The total amount of channel groups on the backend
  /// @return @ref PVR_ERROR_NO_ERROR if the amount has been fetched successfully.
  ///
  /// @remarks Required if @ref PVRCapabilities::SetSupportsChannelGroups "supportsChannelGroups"  is set to true.
  ///
  virtual PVR_ERROR GetChannelGroupsAmount(int& amount) { return PVR_ERROR_NOT_IMPLEMENTED; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Get a list of available channel groups on addon
  ///
  /// Request the list of all channel groups from the backend if it supports
  /// channel groups.
  ///
  /// @param[in] radio True to get the radio channel groups, false to get the
  ///                  TV channel groups.
  /// @param[out] results List of available groups on addon defined with
  ///                     @ref cpp_kodi_addon_pvr_Defs_ChannelGroup_PVRChannelGroup,
  ///                     them transferred with
  ///                     @ref cpp_kodi_addon_pvr_Defs_ChannelGroup_PVRChannelGroupsResultSet.
  /// @return @ref PVR_ERROR_NO_ERROR if the list has been fetched successfully.
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_pvr_Defs_ChannelGroup_PVRChannelGroup_Help
  ///
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @remarks Required if @ref PVRCapabilities::SetSupportsChannelGroups "supportsChannelGroups"
  /// is set to true.
  ///
  ///
  ///---------------------------------------------------------------------------
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// ...
  /// PVR_ERROR CMyPVRInstance::GetChannelGroups(bool radio, kodi::addon::PVRChannelGroupsResultSet& groups)
  /// {
  ///   kodi::addon::PVRChannelGroup group;
  ///   group.SetIsRadio(false);
  ///   group.SetGroupName("My group name");
  ///   group.SetPosition(1);
  ///   ...
  ///
  ///   // Give it now to Kodi
  ///   results.Add(group);
  ///   return PVR_ERROR_NO_ERROR;
  /// }
  /// ...
  /// ~~~~~~~~~~~~~
  ///
  virtual PVR_ERROR GetChannelGroups(bool radio, kodi::addon::PVRChannelGroupsResultSet& results)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Get a list of members on a group
  ///
  /// Request the list of all group members of a group from the backend if it
  /// supports channel groups.
  ///
  /// @param[in] group The group to get the members for.
  /// @param[out] results List of available group member channels defined with
  ///                     @ref cpp_kodi_addon_pvr_Defs_ChannelGroup_PVRChannelGroupMember,
  ///                     them transferred with
  ///                     @ref PVRChannelGroupMembersResultSet.
  /// @return @ref PVR_ERROR_NO_ERROR if the list has been fetched successfully.
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_pvr_Defs_ChannelGroup_PVRChannelGroupMember_Help
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @remarks Required if @ref PVRCapabilities::SetSupportsChannelGroups "supportsChannelGroups"
  /// is set to true.
  ///
  ///
  ///---------------------------------------------------------------------------
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// ...
  /// PVR_ERROR CMyPVRInstance::GetChannelGroupMembers(const kodi::addon::PVRChannelGroup& group,
  ///                                                  kodi::addon::PVRChannelGroupMembersResultSet& results)
  /// {
  ///   for (const auto& myGroup : m_myGroups)
  ///   {
  ///     if (myGroup.strGroupName == group.GetGroupName())
  ///     {
  ///       for (unsigned int iChannelPtr = 0; iChannelPtr < myGroup.members.size(); iChannelPtr++)
  ///       {
  ///         int iId = myGroup.members.at(iChannelPtr) - 1;
  ///         if (iId < 0 || iId > (int)m_channels.size() - 1)
  ///           continue;
  ///
  ///         PVRDemoChannel &channel = m_channels.at(iId);
  ///         kodi::addon::PVRChannelGroupMember kodiGroupMember;
  ///         kodiGroupMember.SetGroupName(group.GetGroupName());
  ///         kodiGroupMember.SetChannelUniqueId(channel.iUniqueId);
  ///         kodiGroupMember.SetChannelNumber(channel.iChannelNumber);
  ///         kodiGroupMember.SetSubChannelNumber(channel.iSubChannelNumber);
  ///
  ///         results.Add(kodiGroupMember);
  ///       }
  ///     }
  ///   }
  ///   return PVR_ERROR_NO_ERROR;
  /// }
  /// ...
  /// ~~~~~~~~~~~~~
  ///
  virtual PVR_ERROR GetChannelGroupMembers(const kodi::addon::PVRChannelGroup& group,
                                           kodi::addon::PVRChannelGroupMembersResultSet& results)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief **Callback to Kodi Function**\n
  /// Request Kodi to update it's list of channel groups.
  ///
  /// @remarks Only called from addon itself
  ///
  inline void TriggerChannelGroupsUpdate()
  {
    m_instanceData->toKodi->TriggerChannelGroupsUpdate(m_instanceData->toKodi->kodiInstance);
  }
  //----------------------------------------------------------------------------

  ///@}
  //--==----==----==----==----==----==----==----==----==----==----==----==----==

  //============================================================================
  /// @defgroup cpp_kodi_addon_pvr_supportsChannelEdit 4. Channel edit (optional)
  /// @ingroup cpp_kodi_addon_pvr
  /// @brief <b>Bring in this functions if you have set @ref PVRCapabilities::SetSupportsChannelSettings "supportsChannelSettings"
  /// to true or for @ref OpenDialogChannelScan() set @ref PVRCapabilities::SetSupportsChannelScan "supportsChannelScan"
  /// to true</b>\n
  /// The support of this is a pure option and not mandatory.
  ///
  ///
  ///---------------------------------------------------------------------------
  ///
  /// **Channel edit parts in interface:**\n
  /// Copy this to your project and extend with your parts or leave functions
  /// complete away where not used or supported.
  ///
  /// @copydetails cpp_kodi_addon_pvr_supportsChannelEdit_header_addon_auto_check
  /// @copydetails cpp_kodi_addon_pvr_supportsChannelEdit_source_addon_auto_check
  ///
  ///@{

  //============================================================================
  /// @brief Delete a channel from the backend.
  ///
  /// @param[in] channel The channel to delete.
  /// @return @ref PVR_ERROR_NO_ERROR if the channel has been deleted successfully.
  /// @remarks Required if @ref PVRCapabilities::SetSupportsChannelSettings "supportsChannelSettings"
  /// is set to true.
  ///
  virtual PVR_ERROR DeleteChannel(const kodi::addon::PVRChannel& channel)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //==========================================================================
  /// @brief Rename a channel on the backend.
  ///
  /// @param[in] channel The channel to rename, containing the new channel name.
  /// @return @ref PVR_ERROR_NO_ERROR if the channel has been renamed successfully.
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_pvr_Defs_Channel_PVRChannel_Help
  ///
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @remarks Optional, and only used if @ref PVRCapabilities::SetSupportsChannelSettings "supportsChannelSettings"
  /// is set to true.
  ///
  virtual PVR_ERROR RenameChannel(const kodi::addon::PVRChannel& channel)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //==========================================================================
  /// @brief Show the channel settings dialog, if supported by the backend.
  ///
  /// @param[in] channel The channel to show the dialog for.
  /// @return @ref PVR_ERROR_NO_ERROR if the dialog has been displayed successfully.
  ///
  /// @remarks Required if @ref PVRCapabilities::SetSupportsChannelSettings "supportsChannelSettings" is set to true.
  /// @note Use @ref cpp_kodi_gui_CWindow "kodi::gui::CWindow" to create dialog for them.
  ///
  virtual PVR_ERROR OpenDialogChannelSettings(const kodi::addon::PVRChannel& channel)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //==========================================================================
  /// @brief Show the dialog to add a channel on the backend, if supported by the backend.
  ///
  /// @param[in] channel The channel to add.
  /// @return @ref PVR_ERROR_NO_ERROR if the channel has been added successfully.
  ///
  /// @remarks Required if @ref PVRCapabilities::SetSupportsChannelSettings "supportsChannelSettings" is set to true.
  /// @note Use @ref cpp_kodi_gui_CWindow "kodi::gui::CWindow" to create dialog for them.
  ///
  virtual PVR_ERROR OpenDialogChannelAdd(const kodi::addon::PVRChannel& channel)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //==========================================================================
  /// @brief Show the channel scan dialog if this backend supports it.
  ///
  /// @return @ref PVR_ERROR_NO_ERROR if the dialog was displayed successfully.
  ///
  /// @remarks Required if @ref PVRCapabilities::SetSupportsChannelScan "supportsChannelScan" is set to true.
  /// @note Use @ref cpp_kodi_gui_CWindow "kodi::gui::CWindow" to create dialog for them.
  ///
  virtual PVR_ERROR OpenDialogChannelScan() { return PVR_ERROR_NOT_IMPLEMENTED; }
  //----------------------------------------------------------------------------

  //==========================================================================
  /// @brief Call one of the channel related menu hooks (if supported).
  ///
  /// Supported @ref cpp_kodi_addon_pvr_Defs_Menuhook_PVRMenuhook instances have to be added in
  /// `constructor()`, by calling @ref AddMenuHook() on the callback.
  ///
  /// @param[in] menuhook The hook to call.
  /// @param[in] item The selected channel item for which the hook was called.
  /// @return @ref PVR_ERROR_NO_ERROR if the hook was called successfully.
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_pvr_Defs_Menuhook_PVRMenuhook_Help
  ///
  virtual PVR_ERROR CallChannelMenuHook(const kodi::addon::PVRMenuhook& menuhook,
                                        const kodi::addon::PVRChannel& item)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  ///@}
  //--==----==----==----==----==----==----==----==----==----==----==----==----==

  //============================================================================
  /// @defgroup cpp_kodi_addon_pvr_EPGTag 4. EPG methods (optional)
  /// @ingroup cpp_kodi_addon_pvr
  /// @brief **PVR EPG methods**\n
  /// These C ++ class functions of are intended for processing EPG information
  /// and for giving it to Kodi.
  ///
  /// The necessary data is transferred with @ref cpp_kodi_addon_pvr_Defs_epg_PVREPGTag.
  ///
  /// @remarks Only used by Kodi if @ref PVRCapabilities::SetSupportsEPG "supportsEPG"
  /// is set to true.\n\n
  ///
  ///
  ///---------------------------------------------------------------------------
  ///
  /// **EPG parts in interface:**\n
  /// Copy this to your project and extend with your parts or leave functions
  /// complete away where not used or supported.
  ///
  /// @copydetails cpp_kodi_addon_pvr_EPGTag_header_addon_auto_check
  /// @copydetails cpp_kodi_addon_pvr_EPGTag_source_addon_auto_check
  ///
  ///@{

  //============================================================================
  /// @brief Request the EPG for a channel from the backend.
  ///
  /// @param[in] channelUid The UID of the channel to get the EPG table for.
  /// @param[in] start Get events after this time (UTC).
  /// @param[in] end Get events before this time (UTC).
  /// @param[out] results List where available EPG information becomes
  ///                     transferred with @ref cpp_kodi_addon_pvr_Defs_epg_PVREPGTag
  ///                     and given to Kodi
  /// @return @ref PVR_ERROR_NO_ERROR if the table has been fetched successfully.
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_pvr_Defs_epg_PVREPGTag_Help
  ///
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @remarks Required if @ref PVRCapabilities::SetSupportsEPG "supportsEPG" is set to true.
  ///
  ///
  ///---------------------------------------------------------------------------
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// ...
  /// PVR_ERROR CMyPVRInstance::GetEPGForChannel(int channelUid,
  ///                                            time_t start,
  ///                                            time_t end,
  ///                                            kodi::addon::PVREPGTagsResultSet& results)
  /// {
  ///   // Minimal demo example, in reality bigger, loop to transfer all and to
  ///   // match wanted times.
  ///   kodi::addon::PVREPGTag tag;
  ///   tag.SetUniqueBroadcastId(123);
  ///   tag.SetUniqueChannelId(123);
  ///   tag.SetTitle("My epg entry name");
  ///   tag.SetGenreType(EPG_EVENT_CONTENTMASK_MOVIEDRAMA);
  ///   tag.SetStartTime(1589148283); // Seconds elapsed since 00:00 hours, Jan 1, 1970 UTC
  ///   tag.SetEndTime(1589151913);
  ///   ...
  ///
  ///   // Give it now to Kodi
  ///   results.Add(tag);
  ///   return PVR_ERROR_NO_ERROR;
  /// }
  /// ...
  /// ~~~~~~~~~~~~~
  ///
  virtual PVR_ERROR GetEPGForChannel(int channelUid,
                                     time_t start,
                                     time_t end,
                                     kodi::addon::PVREPGTagsResultSet& results)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Check if the given EPG tag can be recorded.
  ///
  /// @param[in] tag the @ref cpp_kodi_addon_pvr_Defs_epg_PVREPGTag "epg tag" to check.
  /// @param[out] isRecordable Set to true if the tag can be recorded.
  /// @return @ref PVR_ERROR_NO_ERROR if bIsRecordable has been set successfully.
  ///
  /// @remarks Optional, it return @ref PVR_ERROR_NOT_IMPLEMENTED by parent to let Kodi decide.
  ///
  virtual PVR_ERROR IsEPGTagRecordable(const kodi::addon::PVREPGTag& tag, bool& isRecordable)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Check if the given EPG tag can be played.
  ///
  /// @param[in] tag the @ref cpp_kodi_addon_pvr_Defs_epg_PVREPGTag "epg tag" to check.
  /// @param[out] isPlayable Set to true if the tag can be played.
  /// @return @ref PVR_ERROR_NO_ERROR if bIsPlayable has been set successfully.
  ///
  /// @remarks Required if add-on supports playing epg tags.
  ///
  virtual PVR_ERROR IsEPGTagPlayable(const kodi::addon::PVREPGTag& tag, bool& isPlayable)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Retrieve the edit decision list (EDL) of an EPG tag on the backend.
  ///
  /// @param[in] tag The @ref cpp_kodi_addon_pvr_Defs_epg_PVREPGTag "epg tag".
  /// @param[out] edl The function has to write the EDL into this array.
  /// @return @ref PVR_ERROR_NO_ERROR if the EDL was successfully read or no EDL exists.
  ///
  /// @remarks Required if @ref PVRCapabilities::SetSupportsEPGEdl "supportsEPGEdl" is set to true.
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_pvr_Defs_EDLEntry_PVREDLEntry_Help
  ///
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @remarks Required if @ref PVRCapabilities::SetSupportsEPGEdl "supportsEPGEdl" is set to true.
  ///
  virtual PVR_ERROR GetEPGTagEdl(const kodi::addon::PVREPGTag& tag,
                                 std::vector<kodi::addon::PVREDLEntry>& edl)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Get the stream properties for an epg tag from the backend.
  ///
  /// @param[in] tag The @ref cpp_kodi_addon_pvr_Defs_epg_PVREPGTag "epg tag" to get the stream properties for.
  /// @param[out] properties the properties required to play the stream.
  /// @return @ref PVR_ERROR_NO_ERROR if the stream is available.
  ///
  /// @remarks Required if add-on supports playing epg tags.
  /// In this case your implementation must fill the property @ref PVR_STREAM_PROPERTY_STREAMURL
  /// with the URL Kodi should resolve to playback the epg tag.
  /// It return @ref PVR_ERROR_NOT_IMPLEMENTED from parent if this add-on won't provide this function.
  ///
  /// @note The value directly related to inputstream must always begin with the
  /// name of the associated add-on, e.g. <b>`"inputstream.adaptive.manifest_update_parameter"`</b>.
  ///
  ///
  ///---------------------------------------------------------------------------
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// ...
  /// PVR_ERROR CMyPVRInstance::GetEPGTagStreamProperties(const kodi::addon::PVREPGTag& tag,
  ///                                                     std::vector<kodi::addon::PVRStreamProperty>& properties)
  /// {
  ///   ...
  ///   properties.emplace_back(PVR_STREAM_PROPERTY_INPUTSTREAM, "inputstream.adaptive");
  ///   properties.emplace_back("inputstream.adaptive.manifest_type", "mpd");
  ///   properties.emplace_back("inputstream.adaptive.manifest_update_parameter", "full");
  ///   properties.emplace_back(PVR_STREAM_PROPERTY_MIMETYPE, "application/xml+dash");
  ///   return PVR_ERROR_NO_ERROR;
  /// }
  /// ...
  /// ~~~~~~~~~~~~~
  ///
  virtual PVR_ERROR GetEPGTagStreamProperties(
      const kodi::addon::PVREPGTag& tag, std::vector<kodi::addon::PVRStreamProperty>& properties)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Tell the client the past time frame to use when notifying epg events back to Kodi
  ///
  /// The client might push epg events asynchronously to Kodi using the callback function
  /// @ref EpgEventStateChange. To be able to only push events that are actually of
  /// interest for Kodi, client needs to know about the epg time frame Kodi uses. Kodi supplies
  /// the current epg max past time frame value @ref EpgMaxPastDays() when creating the addon
  /// and calls @ref SetEPGMaxPastDays later whenever Kodi's epg time frame value changes.
  ///
  /// @param[in] pastDays number of days before "now". @ref EPG_TIMEFRAME_UNLIMITED means that Kodi
  ///                     is interested in all epg events, regardless of event times.
  /// @return @ref PVR_ERROR_NO_ERROR if new value was successfully set.
  ///
  /// @remarks Required if @ref PVRCapabilities::SetSupportsEPG "supportsEPG" is set to true.
  ///
  virtual PVR_ERROR SetEPGMaxPastDays(int pastDays) { return PVR_ERROR_NOT_IMPLEMENTED; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Tell the client the future time frame to use when notifying epg events back to Kodi
  ///
  /// The client might push epg events asynchronously to Kodi using the callback function
  /// @ref EpgEventStateChange. To be able to only push events that are actually of
  /// interest for Kodi, client needs to know about the epg time frame Kodi uses. Kodi supplies
  /// the current epg max future time frame value @ref EpgMaxFutureDays() when creating the addon
  /// and calls @ref SetEPGMaxFutureDays later whenever Kodi's epg time frame value changes.
  ///
  /// @param[in] futureDays number of days from "now". @ref EPG_TIMEFRAME_UNLIMITED means that Kodi
  ///                       is interested in all epg events, regardless of event times.
  /// @return @ref PVR_ERROR_NO_ERROR if new value was successfully set.
  ///
  /// @remarks Required if @ref PVRCapabilities::SetSupportsEPG "supportsEPG" is set to true.
  ///
  virtual PVR_ERROR SetEPGMaxFutureDays(int futureDays) { return PVR_ERROR_NOT_IMPLEMENTED; }
  //----------------------------------------------------------------------------

  //==========================================================================
  /// @brief Call one of the EPG related menu hooks (if supported).
  ///
  /// Supported @ref cpp_kodi_addon_pvr_Defs_Menuhook_PVRMenuhook instances have to be added in
  /// `constructor()`, by calling @ref AddMenuHook() on the callback.
  ///
  /// @param[in] menuhook The hook to call.
  /// @param[in] tag The selected EPG item for which the hook was called.
  /// @return @ref PVR_ERROR_NO_ERROR if the hook was called successfully.
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_pvr_Defs_Menuhook_PVRMenuhook_Help
  ///
  virtual PVR_ERROR CallEPGMenuHook(const kodi::addon::PVRMenuhook& menuhook,
                                    const kodi::addon::PVREPGTag& tag)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //==========================================================================
  /// @brief **Callback to Kodi Function**\n
  /// Get the Max past days handled by Kodi.
  ///
  /// If > @ref EPG_TIMEFRAME_UNLIMITED, in async epg mode, deliver only events in the
  /// range from 'end time > now - EpgMaxPastDays()' to 'start time < now + EpgMaxFutureDays().
  /// @ref EPG_TIMEFRAME_UNLIMITED, notify all events.
  ///
  /// @return The Max past days handled by Kodi
  ///
  inline int EpgMaxPastDays() const { return m_instanceData->props->iEpgMaxPastDays; }
  //----------------------------------------------------------------------------

  //==========================================================================
  /// @brief **Callback to Kodi Function**\n
  /// Get the Max future days handled by Kodi.
  ///
  /// If > @ref EPG_TIMEFRAME_UNLIMITED, in async epg mode, deliver only events in the
  /// range from 'end time > now - EpgMaxPastDays()' to 'start time < now + EpgMaxFutureDays().
  /// @ref EPG_TIMEFRAME_UNLIMITED, notify all events.
  ///
  /// @return The Max future days handled by Kodi
  ///
  inline int EpgMaxFutureDays() const { return m_instanceData->props->iEpgMaxFutureDays; }
  //----------------------------------------------------------------------------

  //==========================================================================
  /// @brief **Callback to Kodi Function**\n
  /// Schedule an EPG update for the given channel channel.
  ///
  /// @param[in] channelUid The unique id of the channel for this add-on
  ///
  /// @remarks Only called from addon itself
  ///
  inline void TriggerEpgUpdate(unsigned int channelUid)
  {
    m_instanceData->toKodi->TriggerEpgUpdate(m_instanceData->toKodi->kodiInstance, channelUid);
  }
  //----------------------------------------------------------------------------

  //==========================================================================
  /// @brief **Callback to Kodi Function**\n
  /// Notify a state change for an EPG event.
  ///
  /// @param[in] tag The @ref cpp_kodi_addon_pvr_Defs_epg_PVREPGTag "EPG tag" where have event.
  /// @param[in] newState The new state.
  /// - For @ref EPG_EVENT_CREATED and @ref EPG_EVENT_UPDATED, tag must be filled with all available event data, not just a delta.
  /// - For @ref EPG_EVENT_DELETED, it is sufficient to fill @ref kodi::addon::PVREPGTag::SetUniqueBroadcastId
  ///
  /// @remarks Only called from addon itself,
  ///
  ///
  ///---------------------------------------------------------------------------
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// ...
  ///
  /// void CMyPVRInstance::MyProcessFunction()
  /// {
  ///   ...
  ///   kodi::addon::PVREPGTag tag; // Here as mini add, in real it should be a complete tag
  ///   tag.SetUniqueId(123);
  ///
  ///   // added namespace here not needed to have, only to have more clear for where is
  ///   kodi::addon::CInstancePVRClient::EpgEventStateChange(tag, EPG_EVENT_UPDATED);
  ///   ...
  /// }
  ///
  /// ...
  /// ~~~~~~~~~~~~~
  ///
  inline void EpgEventStateChange(kodi::addon::PVREPGTag& tag, EPG_EVENT_STATE newState)
  {
    m_instanceData->toKodi->EpgEventStateChange(m_instanceData->toKodi->kodiInstance, tag.GetTag(),
                                                newState);
  }
  //----------------------------------------------------------------------------

  ///@}
  //--==----==----==----==----==----==----==----==----==----==----==----==----==

  //============================================================================
  /// @defgroup cpp_kodi_addon_pvr_Recordings 5. Recordings (optional)
  /// @ingroup cpp_kodi_addon_pvr
  /// @brief **PVR recording methods**\n
  /// To transfer available recordings of the PVR backend and to allow possible
  /// playback.
  ///
  /// @remarks Only used by Kodi if @ref PVRCapabilities::SetSupportsRecordings "supportsRecordings"
  /// is set to true.\n\n
  /// If a recordings changes after the initial import, or if a new one was added,
  /// then the add-on should call @ref TriggerRecordingUpdate().
  ///
  ///
  ///---------------------------------------------------------------------------
  ///
  /// **Recordings parts in interface:**\n
  /// Copy this to your project and extend with your parts or leave functions
  /// complete away where not used or supported.
  ///
  /// @copydetails cpp_kodi_addon_pvr_Recordings_header_addon_auto_check
  /// @copydetails cpp_kodi_addon_pvr_Recordings_source_addon_auto_check
  ///
  ///@{

  //============================================================================
  /// @brief To get amount of recording present on backend
  ///
  /// @param[in] deleted if set return deleted recording (called if
  ///                    @ref PVRCapabilities::SetSupportsRecordingsUndelete "supportsRecordingsUndelete"
  ///                    set to true)
  /// @param[out] amount The total amount of recordings on the backend
  /// @return @ref PVR_ERROR_NO_ERROR if the amount has been fetched successfully.
  ///
  /// @remarks Optional, and only used if @ref PVRCapabilities::SetSupportsRecordings "supportsRecordings" is set to true.
  ///
  virtual PVR_ERROR GetRecordingsAmount(bool deleted, int& amount)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Request the list of all recordings from the backend, if supported.
  ///
  /// Recording entries are added to Kodi by calling TransferRecordingEntry() on the callback.
  ///
  /// @param[in] deleted if set return deleted recording (called if
  ///                    @ref PVRCapabilities::SetSupportsRecordingsUndelete "supportsRecordingsUndelete"
  ///                    set to true)
  /// @param[out] results List of available recordings with @ref cpp_kodi_addon_pvr_Defs_Recording_PVRRecording
  ///                     becomes transferred with @ref cpp_kodi_addon_pvr_Defs_Recording_PVRRecordingsResultSet
  ///                     and given to Kodi
  /// @return @ref PVR_ERROR_NO_ERROR if the recordings have been fetched successfully.
  ///
  /// @remarks Optional, and only used if @ref PVRCapabilities::SetSupportsRecordings "supportsRecordings"
  /// is set to true.
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_pvr_Defs_Recording_PVRRecording_Help
  ///
  ///
  ///---------------------------------------------------------------------------
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// ...
  /// PVR_ERROR CMyPVRInstance::GetRecordings(bool deleted, kodi::addon::PVRRecordingsResultSet& results)
  /// {
  ///   // Minimal demo example, in reality bigger and loop to transfer all
  ///   kodi::addon::PVRRecording recording;
  ///   recording.SetRecordingId(123);
  ///   recording.SetTitle("My recording name");
  ///   ...
  ///
  ///   // Give it now to Kodi
  ///   results.Add(recording);
  ///   return PVR_ERROR_NO_ERROR;
  /// }
  /// ...
  /// ~~~~~~~~~~~~~
  ///
  virtual PVR_ERROR GetRecordings(bool deleted, kodi::addon::PVRRecordingsResultSet& results)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Delete a recording on the backend.
  ///
  /// @param[in] recording The @ref cpp_kodi_addon_pvr_Defs_Recording_PVRRecording to delete.
  /// @return @ref PVR_ERROR_NO_ERROR if the recording has been deleted successfully.
  ///
  /// @remarks Optional, and only used if @ref PVRCapabilities::SetSupportsRecordings "supportsRecordings"
  /// is set to true.
  ///
  virtual PVR_ERROR DeleteRecording(const kodi::addon::PVRRecording& recording)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Undelete a recording on the backend.
  ///
  /// @param[in] recording The @ref cpp_kodi_addon_pvr_Defs_Recording_PVRRecording to undelete.
  /// @return @ref PVR_ERROR_NO_ERROR if the recording has been undeleted successfully.
  ///
  /// @remarks Optional, and only used if @ref PVRCapabilities::SetSupportsRecordingsUndelete "supportsRecordingsUndelete"
  /// is set to true.
  ///
  virtual PVR_ERROR UndeleteRecording(const kodi::addon::PVRRecording& recording)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief  Delete all recordings permanent which in the deleted folder on the backend.
  ///
  /// @return @ref PVR_ERROR_NO_ERROR if the recordings has been deleted successfully.
  ///
  virtual PVR_ERROR DeleteAllRecordingsFromTrash() { return PVR_ERROR_NOT_IMPLEMENTED; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Rename a recording on the backend.
  ///
  /// @param[in] recording The @ref cpp_kodi_addon_pvr_Defs_Recording_PVRRecording
  ///                      to rename, containing the new name.
  /// @return @ref PVR_ERROR_NO_ERROR if the recording has been renamed successfully.
  ///
  /// @remarks Optional, and only used if @ref PVRCapabilities::SetSupportsRecordings "supportsRecordings"
  /// is set to true.
  ///
  virtual PVR_ERROR RenameRecording(const kodi::addon::PVRRecording& recording)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Set the lifetime of a recording on the backend.
  ///
  /// @param[in] recording The @ref cpp_kodi_addon_pvr_Defs_Recording_PVRRecording
  ///                      to change the lifetime for. recording.iLifetime
  ///                      contains the new lieftime value.
  /// @return @ref PVR_ERROR_NO_ERROR if the recording's lifetime has been set
  ///         successfully.
  ///
  /// @remarks Required if @ref PVRCapabilities::SetSupportsRecordingsLifetimeChange "supportsRecordingsLifetimeChange"
  /// is set to true.
  ///
  virtual PVR_ERROR SetRecordingLifetime(const kodi::addon::PVRRecording& recording)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Set the play count of a recording on the backend.
  ///
  /// @param[in] recording The @ref cpp_kodi_addon_pvr_Defs_Recording_PVRRecording
  ///                      to change the play count.
  /// @param[in] count Play count.
  /// @return @ref PVR_ERROR_NO_ERROR if the recording's play count has been set
  /// successfully.
  ///
  /// @remarks Required if @ref PVRCapabilities::SetSupportsRecordingPlayCount "supportsRecordingPlayCount"
  /// is set to true.
  ///
  virtual PVR_ERROR SetRecordingPlayCount(const kodi::addon::PVRRecording& recording, int count)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Set the last watched position of a recording on the backend.
  ///
  /// @param[in] recording The @ref cpp_kodi_addon_pvr_Defs_Recording_PVRRecording.
  /// @param[in] lastplayedposition The last watched position in seconds
  /// @return @ref PVR_ERROR_NO_ERROR if the position has been stored successfully.
  ///
  /// @remarks Required if @ref PVRCapabilities::SetSupportsLastPlayedPosition "supportsLastPlayedPosition"
  /// is set to true.
  ///
  virtual PVR_ERROR SetRecordingLastPlayedPosition(const kodi::addon::PVRRecording& recording,
                                                   int lastplayedposition)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Retrieve the last watched position of a recording on the backend.
  ///
  /// @param[in] recording The @ref cpp_kodi_addon_pvr_Defs_Recording_PVRRecording.
  /// @param[out] position The last watched position in seconds
  /// @return @ref PVR_ERROR_NO_ERROR if the amount has been fetched successfully.
  ///
  /// @remarks Required if @ref PVRCapabilities::SetSupportsRecordingPlayCount "supportsRecordingPlayCount"
  /// is set to true.
  ///
  virtual PVR_ERROR GetRecordingLastPlayedPosition(const kodi::addon::PVRRecording& recording,
                                                   int& position)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Retrieve the edit decision list (EDL) of a recording on the backend.
  ///
  /// @param[in] recording The @ref cpp_kodi_addon_pvr_Defs_Recording_PVRRecording.
  /// @param[out] edl The function has to write the EDL into this array.
  /// @return @ref PVR_ERROR_NO_ERROR if the EDL was successfully read or no EDL exists.
  ///
  /// @remarks Required if @ref PVRCapabilities::SetSupportsRecordingEdl "supportsRecordingEdl"
  /// is set to true.
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_pvr_Defs_EDLEntry_PVREDLEntry_Help
  ///
  virtual PVR_ERROR GetRecordingEdl(const kodi::addon::PVRRecording& recording,
                                    std::vector<kodi::addon::PVREDLEntry>& edl)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Retrieve the size of a recording on the backend.
  ///
  /// @param[in] recording The recording to get the size in bytes for.
  /// @param[out] size The size in bytes of the recording
  /// @return @ref PVR_ERROR_NO_ERROR if the recording's size has been set successfully.
  ///
  /// @remarks Required if @ref PVRCapabilities::SetSupportsRecordingSize "supportsRecordingSize"
  /// is set to true.
  ///
  virtual PVR_ERROR GetRecordingSize(const kodi::addon::PVRRecording& recording, int64_t& size)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Get the stream properties for a recording from the backend.
  ///
  /// @param[in] recording The @ref cpp_kodi_addon_pvr_Defs_Recording_PVRRecording
  ///                      to get the stream properties for.
  /// @param[out] properties The properties required to play the stream.
  /// @return @ref PVR_ERROR_NO_ERROR if the stream is available.
  ///
  /// @remarks Required if @ref PVRCapabilities::SetSupportsRecordings "supportsRecordings"
  /// is set to true and the add-on does not implement recording stream functions
  /// (@ref OpenRecordedStream, ...).\n
  /// In this case your implementation must fill the property @ref PVR_STREAM_PROPERTY_STREAMURL
  /// with the URL Kodi should resolve to playback the recording.
  ///
  /// @note The value directly related to inputstream must always begin with the
  /// name of the associated add-on, e.g. <b>`"inputstream.adaptive.manifest_update_parameter"`</b>.
  ///
  ///
  ///---------------------------------------------------------------------------
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// ...
  /// PVR_ERROR CMyPVRInstance::GetRecordingStreamProperties(const kodi::addon::PVRRecording& recording,
  ///                                                        std::vector<kodi::addon::PVRStreamProperty>& properties)
  /// {
  ///   ...
  ///   properties.emplace_back(PVR_STREAM_PROPERTY_INPUTSTREAM, "inputstream.adaptive");
  ///   properties.emplace_back("inputstream.adaptive.manifest_type", "mpd");
  ///   properties.emplace_back("inputstream.adaptive.manifest_update_parameter", "full");
  ///   properties.emplace_back(PVR_STREAM_PROPERTY_MIMETYPE, "application/xml+dash");
  ///   return PVR_ERROR_NO_ERROR;
  /// }
  /// ...
  /// ~~~~~~~~~~~~~
  ///
  virtual PVR_ERROR GetRecordingStreamProperties(
      const kodi::addon::PVRRecording& recording,
      std::vector<kodi::addon::PVRStreamProperty>& properties)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //==========================================================================
  /// @brief Call one of the recording related menu hooks (if supported).
  ///
  /// Supported @ref cpp_kodi_addon_pvr_Defs_Menuhook_PVRMenuhook instances have to be added in
  /// `constructor()`, by calling @ref AddMenuHook() on the callback.
  ///
  /// @param[in] menuhook The hook to call.
  /// @param[in] item The selected recording item for which the hook was called.
  /// @return @ref PVR_ERROR_NO_ERROR if the hook was called successfully.
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_pvr_Defs_Menuhook_PVRMenuhook_Help
  ///
  virtual PVR_ERROR CallRecordingMenuHook(const kodi::addon::PVRMenuhook& menuhook,
                                          const kodi::addon::PVRRecording& item)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief **Callback to Kodi Function**\n
  /// Display a notification in Kodi that a recording started or stopped on the
  /// server.
  ///
  /// @param[in] recordingName The name of the recording to display
  /// @param[in] fileName The filename of the recording
  /// @param[in] on True when recording started, false when it stopped
  ///
  /// @remarks Only called from addon itself
  ///
  inline void RecordingNotification(const std::string& recordingName,
                                    const std::string& fileName,
                                    bool on)
  {
    m_instanceData->toKodi->RecordingNotification(m_instanceData->toKodi->kodiInstance,
                                                  recordingName.c_str(), fileName.c_str(), on);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief **Callback to Kodi Function**\n
  /// Request Kodi to update it's list of recordings.
  ///
  /// @remarks Only called from addon itself
  ///
  inline void TriggerRecordingUpdate()
  {
    m_instanceData->toKodi->TriggerRecordingUpdate(m_instanceData->toKodi->kodiInstance);
  }
  //----------------------------------------------------------------------------

  ///@}
  //--==----==----==----==----==----==----==----==----==----==----==----==----==

  //============================================================================
  /// @defgroup cpp_kodi_addon_pvr_Timers 6. Timers (optional)
  /// @ingroup cpp_kodi_addon_pvr
  /// @brief **PVR timer methods**\n
  /// For editing and displaying timed work, such as video recording.
  ///
  /// @remarks Only used by Kodi if @ref PVRCapabilities::SetSupportsTimers "supportsTimers"
  /// is set to true.\n\n
  /// If a timer changes after the initial import, or if a new one was added,
  /// then the add-on should call @ref TriggerTimerUpdate().
  ///
  ///
  ///---------------------------------------------------------------------------
  ///
  /// **Timer parts in interface:**\n
  /// Copy this to your project and extend with your parts or leave functions
  /// complete away where not used or supported.
  ///
  /// @copydetails cpp_kodi_addon_pvr_Timers_header_addon_auto_check
  /// @copydetails cpp_kodi_addon_pvr_Timers_source_addon_auto_check
  ///
  ///@{

  //============================================================================
  /// @brief Retrieve the timer types supported by the backend.
  ///
  /// @param[out] types The function has to write the definition of the
  ///                   @ref cpp_kodi_addon_pvr_Defs_Timer_PVRTimerType types
  ///                   into this array.
  /// @return @ref PVR_ERROR_NO_ERROR if the types were successfully written to
  ///         the array.
  ///
  /// @note Maximal 32 entries are allowed inside.
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_pvr_Defs_Timer_PVRTimerType_Help
  ///
  virtual PVR_ERROR GetTimerTypes(std::vector<kodi::addon::PVRTimerType>& types)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief To get total amount of timers on the backend or -1 on error.
  ///
  /// @param[out] amount The total amount of timers on the backend
  /// @return @ref PVR_ERROR_NO_ERROR if the amount has been fetched successfully.
  ///
  /// @note Required to use if @ref PVRCapabilities::SetSupportsTimers "supportsTimers"
  /// is set to true.
  ///
  virtual PVR_ERROR GetTimersAmount(int& amount) { return PVR_ERROR_NOT_IMPLEMENTED; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Request the list of all timers from the backend if supported.
  ///
  /// @param[out] results List of available timers with @ref cpp_kodi_addon_pvr_Defs_Timer_PVRTimer
  ///                     becomes transferred with @ref cpp_kodi_addon_pvr_Defs_Timer_PVRTimersResultSet
  ///                     and given to Kodi
  /// @return @ref PVR_ERROR_NO_ERROR if the list has been fetched successfully.
  ///
  /// @note Required to use if @ref PVRCapabilities::SetSupportsTimers "supportsTimers"
  /// is set to true.
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_pvr_Defs_Timer_PVRTimer_Help
  ///
  ///
  ///---------------------------------------------------------------------------
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// ...
  /// PVR_ERROR CMyPVRInstance::GetTimers(kodi::addon::PVRTimersResultSet& results)
  /// {
  ///   // Minimal demo example, in reality bigger and loop to transfer all
  ///   kodi::addon::PVRTimer timer;
  ///   timer.SetClientIndex(123);
  ///   timer.SetState(PVR_TIMER_STATE_SCHEDULED);
  ///   timer.SetTitle("My timer name");
  ///   ...
  ///
  ///   // Give it now to Kodi
  ///   results.Add(timer);
  ///   return PVR_ERROR_NO_ERROR;
  /// }
  /// ...
  /// ~~~~~~~~~~~~~
  ///
  virtual PVR_ERROR GetTimers(kodi::addon::PVRTimersResultSet& results)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Add a timer on the backend.
  ///
  /// @param[in] timer The timer to add.
  /// @return @ref PVR_ERROR_NO_ERROR if the timer has been added successfully.
  ///
  /// @note Required to use if @ref PVRCapabilities::SetSupportsTimers "supportsTimers"
  /// is set to true.
  ///
  virtual PVR_ERROR AddTimer(const kodi::addon::PVRTimer& timer)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Delete a timer on the backend.
  ///
  /// @param[in] timer The timer to delete.
  /// @param[in] forceDelete Set to true to delete a timer that is currently
  ///                        recording a program.
  /// @return @ref PVR_ERROR_NO_ERROR if the timer has been deleted successfully.
  ///
  /// @note Required to use if @ref PVRCapabilities::SetSupportsTimers "supportsTimers"
  /// is set to true.
  ///
  virtual PVR_ERROR DeleteTimer(const kodi::addon::PVRTimer& timer, bool forceDelete)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Update the timer information on the backend.
  ///
  /// @param[in] timer The timer to update.
  /// @return @ref PVR_ERROR_NO_ERROR if the timer has been updated successfully.
  ///
  /// @note Required to use if @ref PVRCapabilities::SetSupportsTimers "supportsTimers"
  /// is set to true.
  ///
  virtual PVR_ERROR UpdateTimer(const kodi::addon::PVRTimer& timer)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Call one of the timer related menu hooks (if supported).
  ///
  /// Supported @ref cpp_kodi_addon_pvr_Defs_Menuhook_PVRMenuhook instances have
  /// to be added in `constructor()`, by calling @ref AddMenuHook() on the
  /// callback.
  ///
  /// @param[in] menuhook The hook to call.
  /// @param[in] item The selected timer item for which the hook was called.
  /// @return @ref PVR_ERROR_NO_ERROR if the hook was called successfully.
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_pvr_Defs_Menuhook_PVRMenuhook_Help
  ///
  virtual PVR_ERROR CallTimerMenuHook(const kodi::addon::PVRMenuhook& menuhook,
                                      const kodi::addon::PVRTimer& item)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief **Callback to Kodi Function**\n
  /// Request Kodi to update it's list of timers.
  ///
  /// @remarks Only called from addon itself
  ///
  inline void TriggerTimerUpdate()
  {
    m_instanceData->toKodi->TriggerTimerUpdate(m_instanceData->toKodi->kodiInstance);
  }
  //----------------------------------------------------------------------------

  ///@}
  //--==----==----==----==----==----==----==----==----==----==----==----==----==

  //============================================================================
  /// @defgroup cpp_kodi_addon_pvr_PowerManagement 7. Power management events (optional)
  /// @ingroup cpp_kodi_addon_pvr
  /// @brief **Used to notify the pvr addon for power management events**\n
  /// Used to allow any energy savings.
  ///
  ///
  ///---------------------------------------------------------------------------
  ///
  /// **Power management events in interface:**\n
  /// Copy this to your project and extend with your parts or leave functions
  /// complete away where not used or supported.
  ///
  /// @copydetails cpp_kodi_addon_pvr_PowerManagement_header_addon_auto_check
  /// @copydetails cpp_kodi_addon_pvr_PowerManagement_source_addon_auto_check
  ///
  ///@{

  //============================================================================
  /// @brief To notify addon about system sleep
  ///
  /// @return @ref PVR_ERROR_NO_ERROR If successfully done.
  ///
  virtual PVR_ERROR OnSystemSleep() { return PVR_ERROR_NOT_IMPLEMENTED; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief To notify addon about system wake up
  ///
  /// @return @ref PVR_ERROR_NO_ERROR If successfully done.
  ///
  virtual PVR_ERROR OnSystemWake() { return PVR_ERROR_NOT_IMPLEMENTED; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief To notify addon power saving on system is activated
  ///
  /// @return @ref PVR_ERROR_NO_ERROR If successfully done.
  ///
  virtual PVR_ERROR OnPowerSavingActivated() { return PVR_ERROR_NOT_IMPLEMENTED; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief To notify addon power saving on system is deactivated
  ///
  /// @return @ref PVR_ERROR_NO_ERROR If successfully done.
  ///
  virtual PVR_ERROR OnPowerSavingDeactivated() { return PVR_ERROR_NOT_IMPLEMENTED; }
  //----------------------------------------------------------------------------

  ///@}
  //--==----==----==----==----==----==----==----==----==----==----==----==----==

  //============================================================================
  /// @defgroup cpp_kodi_addon_pvr_Streams 8. Inputstream
  /// @ingroup cpp_kodi_addon_pvr
  /// @brief **PVR Inputstream**\n
  /// This includes functions that are used in the PVR inputstream.
  ///
  /// @warning The parts here will be removed in the future and replaced by the
  /// separate @ref cpp_kodi_addon_inputstream "inputstream addon instance".
  /// If there is already a possibility, new addons should do it via the
  /// inputstream instance.
  ///
  ///@{

  //============================================================================
  /// @defgroup cpp_kodi_addon_pvr_Streams_TV 8.1. TV stream
  /// @ingroup cpp_kodi_addon_pvr_Streams
  /// @brief **PVR TV stream**\n
  /// Stream processing regarding live TV.
  ///
  ///
  ///---------------------------------------------------------------------------
  ///
  /// **TV stream parts in interface:**\n
  /// Copy this to your project and extend with your parts or leave functions
  /// complete away where not used or supported.
  ///
  /// @copydetails cpp_kodi_addon_pvr_Streams_TV_header_addon_auto_check
  /// @copydetails cpp_kodi_addon_pvr_Streams_TV_source_addon_auto_check
  ///
  ///@{

  //============================================================================
  /// @brief Open a live stream on the backend.
  ///
  /// @param[in] channel The channel to stream.
  /// @return True if the stream has been opened successfully, false otherwise.
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_pvr_Defs_Channel_PVRChannel_Help
  ///
  ///
  /// --------------------------------------------------------------------------
  ///
  /// @remarks Required if @ref PVRCapabilities::SetHandlesInputStream() or
  /// @ref PVRCapabilities::SetHandlesDemuxing() is set to true.
  /// @ref CloseLiveStream() will always be called by Kodi prior to calling this
  /// function.
  ///
  virtual bool OpenLiveStream(const kodi::addon::PVRChannel& channel) { return false; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Close an open live stream.
  ///
  /// @remarks Required if @ref PVRCapabilities::SetHandlesInputStream() or
  /// @ref PVRCapabilities::SetHandlesDemuxing() is set to true.
  ///
  virtual void CloseLiveStream() {}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Read from an open live stream.
  ///
  /// @param[in] pBuffer The buffer to store the data in.
  /// @param[in] iBufferSize The amount of bytes to read.
  /// @return The amount of bytes that were actually read from the stream.
  ///
  /// @remarks Required if @ref PVRCapabilities::SetHandlesInputStream() is set
  /// to true.
  ///
  virtual int ReadLiveStream(unsigned char* buffer, unsigned int size) { return 0; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Seek in a live stream on a backend that supports timeshifting.
  ///
  /// @param[in] position The position to seek to.
  /// @param[in] whence [optional] offset relative to
  ///                   You can set the value of whence to one of three things:
  /// |   Value | int | Description |
  /// |:--------:|:---:|:----------------------------------------------------|
  /// | SEEK_SET |  0 | position is relative to the beginning of the file. This is probably what you had in mind anyway, and is the most commonly used value for whence.
  /// | SEEK_CUR |  1 | position is relative to the current file pointer position. So, in effect, you can say, "Move to my current position plus 30 bytes," or, "move to my current position minus 20 bytes."
  /// | SEEK_END |  2 | position is relative to the end of the file. Just like SEEK_SET except from the other end of the file. Be sure to use negative values for offset if you want to back up from the end of the file, instead of going past the end into oblivion.
  ///
  /// @return The new position.
  ///
  /// @remarks Optional, and only used if @ref PVRCapabilities::SetHandlesInputStream()
  /// is set to true.
  ///
  virtual int64_t SeekLiveStream(int64_t position, int whence) { return 0; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Obtain the length of a live stream.
  ///
  /// @return The total length of the stream that's currently being read.
  ///
  /// @remarks Optional, and only used if @ref PVRCapabilities::SetHandlesInputStream()
  /// is set to true.
  ///
  virtual int64_t LengthLiveStream() { return 0; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @defgroup cpp_kodi_addon_pvr_Streams_TV_Demux 8.1.1. Stream demuxing
  /// @ingroup cpp_kodi_addon_pvr_Streams_TV
  /// @brief **PVR stream demuxing**\n
  /// Read TV streams with own demux within addon.
  ///
  /// This is only on Live TV streams and only if @ref PVRCapabilities::SetHandlesDemuxing()
  /// has been set to "true".
  ///
  ///
  ///---------------------------------------------------------------------------
  ///
  /// **Stream demuxing parts in interface:**\n
  /// Copy this to your project and extend with your parts or leave functions
  /// complete away where not used or supported.
  ///
  /// @copydetails cpp_kodi_addon_pvr_Streams_TV_Demux_header_addon_auto_check
  /// @copydetails cpp_kodi_addon_pvr_Streams_TV_Demux_source_addon_auto_check
  ///
  ///@{

  //============================================================================
  /// @brief Get the stream properties of the stream that's currently being read.
  ///
  /// @param[in] properties The properties of the currently playing stream.
  /// @return @ref PVR_ERROR_NO_ERROR if the properties have been fetched successfully.
  ///
  /// @remarks Required, and only used if addon has its own demuxer.
  ///
  virtual PVR_ERROR GetStreamProperties(std::vector<kodi::addon::PVRStreamProperties>& properties)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Read the next packet from the demultiplexer, if there is one.
  ///
  /// @return The next packet.
  /// If there is no next packet, then the add-on should return the packet
  /// created by calling @ref AllocateDemuxPacket(0) on the callback.
  /// If the stream changed and Kodi's player needs to be reinitialised, then,
  /// the add-on should call @ref AllocateDemuxPacket(0) on the callback, and set
  /// the streamid to @ref DMX_SPECIALID_STREAMCHANGE and return the value.
  /// The add-on should return `nullptr` if an error occurred.
  ///
  /// @remarks Required, and only used if addon has its own demuxer.
  /// Return `nullptr` if this add-on won't provide this function.
  ///
  virtual DEMUX_PACKET* DemuxRead() { return nullptr; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Reset the demultiplexer in the add-on.
  ///
  /// @remarks Required, and only used if addon has its own demuxer.
  ///
  virtual void DemuxReset() {}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Abort the demultiplexer thread in the add-on.
  ///
  /// @remarks Required, and only used if addon has its own demuxer.
  ///
  virtual void DemuxAbort() {}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Flush all data that's currently in the demultiplexer buffer in the
  /// add-on.
  ///
  /// @remarks Required, and only used if addon has its own demuxer.
  ///
  virtual void DemuxFlush() {}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Notify the pvr addon/demuxer that Kodi wishes to change playback
  /// speed.
  ///
  /// @param[in] speed The requested playback speed
  ///
  /// @remarks Optional, and only used if addon has its own demuxer.
  ///
  virtual void SetSpeed(int speed) {}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Notify the pvr addon/demuxer that Kodi wishes to fill demux queue.
  ///
  /// @param[in] mode The requested filling mode
  ///
  /// @remarks Optional, and only used if addon has its own demuxer.
  ///
  virtual void FillBuffer(bool mode) {}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Notify the pvr addon/demuxer that Kodi wishes to seek the stream by
  /// time.
  ///
  /// @param[in] time The absolute time since stream start
  /// @param[in] backwards True to seek to keyframe BEFORE time, else AFTER
  /// @param[in] startpts can be updated to point to where display should start
  /// @return True if the seek operation was possible
  ///
  /// @remarks Optional, and only used if addon has its own demuxer.
  ///          Return False if this add-on won't provide this function.
  ///
  virtual bool SeekTime(double time, bool backwards, double& startpts) { return false; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief **Callback to Kodi Function**\n
  /// Get the codec id used by Kodi.
  ///
  /// @param[in] codecName The name of the codec
  /// @return The codec_id, or a codec_id with 0 values when not supported
  ///
  /// @remarks Only called from addon itself
  ///
  inline PVRCodec GetCodecByName(const std::string& codecName) const
  {
    return PVRCodec(m_instanceData->toKodi->GetCodecByName(m_instanceData->toKodi->kodiInstance,
                                                           codecName.c_str()));
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief **Callback to Kodi Function**\n
  /// Allocate a demux packet. Free with @ref FreeDemuxPacket().
  ///
  /// @param[in] iDataSize The size of the data that will go into the packet
  /// @return The allocated packet
  ///
  /// @remarks Only called from addon itself
  ///
  inline DEMUX_PACKET* AllocateDemuxPacket(int iDataSize)
  {
    return m_instanceData->toKodi->AllocateDemuxPacket(m_instanceData->toKodi->kodiInstance,
                                                       iDataSize);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief **Callback to Kodi Function**\n
  /// Free a packet that was allocated with @ref AllocateDemuxPacket().
  ///
  /// @param[in] pPacket The packet to free
  ///
  /// @remarks Only called from addon itself.
  ///
  inline void FreeDemuxPacket(DEMUX_PACKET* pPacket)
  {
    m_instanceData->toKodi->FreeDemuxPacket(m_instanceData->toKodi->kodiInstance, pPacket);
  }
  //----------------------------------------------------------------------------
  ///@}

  ///@}
  //--==----==----==----==----==----==----==----==----==----==----==----==----==

  //============================================================================
  /// @defgroup cpp_kodi_addon_pvr_Streams_Recording 8.2. Recording stream
  /// @ingroup cpp_kodi_addon_pvr_Streams
  /// @brief **PVR Recording stream**\n
  /// Stream processing regarding recordings.
  ///
  /// @note Demuxing is not possible with the recordings.
  ///
  ///
  ///---------------------------------------------------------------------------
  ///
  /// **Recording stream parts in interface:**\n
  /// Copy this to your project and extend with your parts or leave functions
  /// complete away where not used or supported.
  ///
  /// @copydetails cpp_kodi_addon_pvr_Streams_Recording_header_addon_auto_check
  /// @copydetails cpp_kodi_addon_pvr_Streams_Recording_source_addon_auto_check
  ///
  ///@{

  //============================================================================
  /// @brief Open a stream to a recording on the backend.
  ///
  /// @param[in] recording The recording to open.
  /// @return True if the stream has been opened successfully, false otherwise.
  ///
  /// @remarks Optional, and only used if @ref PVRCapabilities::SetSupportsRecordings()
  /// is set to true. @ref CloseRecordedStream() will always be called by Kodi
  /// prior to calling this function.
  ///
  virtual bool OpenRecordedStream(const kodi::addon::PVRRecording& recording) { return false; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Close an open stream from a recording.
  ///
  /// @remarks Optional, and only used if @ref PVRCapabilities::SetSupportsRecordings()
  /// is set to true.
  ///
  virtual void CloseRecordedStream() {}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Read from a recording.
  ///
  /// @param[in] buffer The buffer to store the data in.
  /// @param[in] size The amount of bytes to read.
  /// @return The amount of bytes that were actually read from the stream.
  ///
  /// @remarks Optional, and only used if @ref PVRCapabilities::SetSupportsRecordings()
  /// is set to true.
  ///
  virtual int ReadRecordedStream(unsigned char* buffer, unsigned int size) { return 0; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Seek in a recorded stream.
  ///
  /// @param[in] position The position to seek to.
  /// @param[in] whence [optional] offset relative to
  ///                   You can set the value of whence to one of three things:
  /// |   Value | int | Description |
  /// |:--------:|:---:|:----------------------------------------------------|
  /// | SEEK_SET |  0 | position is relative to the beginning of the file. This is probably what you had in mind anyway, and is the most commonly used value for whence.
  /// | SEEK_CUR |  1 | position is relative to the current file pointer position. So, in effect, you can say, "Move to my current position plus 30 bytes," or, "move to my current position minus 20 bytes."
  /// | SEEK_END |  2 | position is relative to the end of the file. Just like SEEK_SET except from the other end of the file. Be sure to use negative values for offset if you want to back up from the end of the file, instead of going past the end into oblivion.
  ///
  /// @return The new position.
  ///
  /// @remarks Optional, and only used if @ref PVRCapabilities::SetSupportsRecordings()
  /// is set to true.
  ///
  virtual int64_t SeekRecordedStream(int64_t position, int whence) { return 0; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Obtain the length of a recorded stream.
  ///
  /// @return The total length of the stream that's currently being read.
  ///
  /// @remarks Optional, and only used if @ref PVRCapabilities::SetSupportsRecordings()
  /// is true (=> @ref ReadRecordedStream).
  ///
  virtual int64_t LengthRecordedStream() { return 0; }
  //----------------------------------------------------------------------------

  ///@}
  //--==----==----==----==----==----==----==----==----==----==----==----==----==

  //============================================================================
  /// @defgroup cpp_kodi_addon_pvr_Streams_Various 8.3. Various functions
  /// @ingroup cpp_kodi_addon_pvr_Streams
  /// @brief **Various other PVR stream related functions**\n
  /// These apply to all other groups in inputstream and are therefore declared
  /// as several.
  ///
  ///
  ///---------------------------------------------------------------------------
  ///
  /// **Various stream parts in interface:**\n
  /// Copy this to your project and extend with your parts or leave functions
  /// complete away where not used or supported.
  ///
  /// @copydetails cpp_kodi_addon_pvr_Streams_Various_header_addon_auto_check
  /// @copydetails cpp_kodi_addon_pvr_Streams_Various_source_addon_auto_check
  ///
  ///@{

  //============================================================================
  ///
  /// @brief Check if the backend support pausing the currently playing stream.
  ///
  /// This will enable/disable the pause button in Kodi based on the return
  /// value.
  ///
  /// @return false if the PVR addon/backend does not support pausing, true if
  ///         possible
  ///
  virtual bool CanPauseStream() { return false; }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  /// @brief Check if the backend supports seeking for the currently playing
  /// stream.
  ///
  /// This will enable/disable the rewind/forward buttons in Kodi based on the
  /// return value.
  ///
  /// @return false if the PVR addon/backend does not support seeking, true if
  ///         possible
  ///
  virtual bool CanSeekStream() { return false; }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  /// @brief Notify the pvr addon that Kodi (un)paused the currently playing
  /// stream.
  ///
  /// @param[in] paused To inform by `true` is paused and with `false` playing
  ///
  virtual void PauseStream(bool paused) {}
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  /// @brief Check for real-time streaming.
  ///
  /// @return true if current stream is real-time
  ///
  virtual bool IsRealTimeStream() { return false; }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  /// @brief Get stream times.
  ///
  /// @param[out] times A pointer to the data to be filled by the implementation.
  /// @return @ref PVR_ERROR_NO_ERROR on success.
  ///
  virtual PVR_ERROR GetStreamTimes(kodi::addon::PVRStreamTimes& times)
  {
    return PVR_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  /// @brief Obtain the chunk size to use when reading streams.
  ///
  /// @param[out] chunksize must be filled with the chunk size in bytes.
  /// @return @ref PVR_ERROR_NO_ERROR if the chunk size has been fetched successfully.
  ///
  /// @remarks Optional, and only used if not reading from demuxer (=> @ref DemuxRead) and
  /// @ref PVRCapabilities::SetSupportsRecordings() is true (=> @ref ReadRecordedStream) or
  /// @ref PVRCapabilities::SetHandlesInputStream() is true (=> @ref ReadLiveStream).
  ///
  virtual PVR_ERROR GetStreamReadChunkSize(int& chunksize) { return PVR_ERROR_NOT_IMPLEMENTED; }
  //----------------------------------------------------------------------------

  ///@}
  //--==----==----==----==----==----==----==----==----==----==----==----==----==

private:
  void SetAddonStruct(KODI_ADDON_INSTANCE_STRUCT* instance)
  {
    instance->hdl = this;

    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    instance->pvr->toAddon->GetCapabilities = ADDON_GetCapabilities;
    instance->pvr->toAddon->GetConnectionString = ADDON_GetConnectionString;
    instance->pvr->toAddon->GetBackendName = ADDON_GetBackendName;
    instance->pvr->toAddon->GetBackendVersion = ADDON_GetBackendVersion;
    instance->pvr->toAddon->GetBackendHostname = ADDON_GetBackendHostname;
    instance->pvr->toAddon->GetDriveSpace = ADDON_GetDriveSpace;
    instance->pvr->toAddon->CallSettingsMenuHook = ADDON_CallSettingsMenuHook;
    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    instance->pvr->toAddon->GetChannelsAmount = ADDON_GetChannelsAmount;
    instance->pvr->toAddon->GetChannels = ADDON_GetChannels;
    instance->pvr->toAddon->GetChannelStreamProperties = ADDON_GetChannelStreamProperties;
    instance->pvr->toAddon->GetSignalStatus = ADDON_GetSignalStatus;
    instance->pvr->toAddon->GetDescrambleInfo = ADDON_GetDescrambleInfo;
    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    instance->pvr->toAddon->GetProvidersAmount = ADDON_GetProvidersAmount;
    instance->pvr->toAddon->GetProviders = ADDON_GetProviders;
    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    instance->pvr->toAddon->GetChannelGroupsAmount = ADDON_GetChannelGroupsAmount;
    instance->pvr->toAddon->GetChannelGroups = ADDON_GetChannelGroups;
    instance->pvr->toAddon->GetChannelGroupMembers = ADDON_GetChannelGroupMembers;
    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    instance->pvr->toAddon->DeleteChannel = ADDON_DeleteChannel;
    instance->pvr->toAddon->RenameChannel = ADDON_RenameChannel;
    instance->pvr->toAddon->OpenDialogChannelSettings = ADDON_OpenDialogChannelSettings;
    instance->pvr->toAddon->OpenDialogChannelAdd = ADDON_OpenDialogChannelAdd;
    instance->pvr->toAddon->OpenDialogChannelScan = ADDON_OpenDialogChannelScan;
    instance->pvr->toAddon->CallChannelMenuHook = ADDON_CallChannelMenuHook;
    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    instance->pvr->toAddon->GetEPGForChannel = ADDON_GetEPGForChannel;
    instance->pvr->toAddon->IsEPGTagRecordable = ADDON_IsEPGTagRecordable;
    instance->pvr->toAddon->IsEPGTagPlayable = ADDON_IsEPGTagPlayable;
    instance->pvr->toAddon->GetEPGTagEdl = ADDON_GetEPGTagEdl;
    instance->pvr->toAddon->GetEPGTagStreamProperties = ADDON_GetEPGTagStreamProperties;
    instance->pvr->toAddon->SetEPGMaxPastDays = ADDON_SetEPGMaxPastDays;
    instance->pvr->toAddon->SetEPGMaxFutureDays = ADDON_SetEPGMaxFutureDays;
    instance->pvr->toAddon->CallEPGMenuHook = ADDON_CallEPGMenuHook;
    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    instance->pvr->toAddon->GetRecordingsAmount = ADDON_GetRecordingsAmount;
    instance->pvr->toAddon->GetRecordings = ADDON_GetRecordings;
    instance->pvr->toAddon->DeleteRecording = ADDON_DeleteRecording;
    instance->pvr->toAddon->UndeleteRecording = ADDON_UndeleteRecording;
    instance->pvr->toAddon->DeleteAllRecordingsFromTrash = ADDON_DeleteAllRecordingsFromTrash;
    instance->pvr->toAddon->RenameRecording = ADDON_RenameRecording;
    instance->pvr->toAddon->SetRecordingLifetime = ADDON_SetRecordingLifetime;
    instance->pvr->toAddon->SetRecordingPlayCount = ADDON_SetRecordingPlayCount;
    instance->pvr->toAddon->SetRecordingLastPlayedPosition = ADDON_SetRecordingLastPlayedPosition;
    instance->pvr->toAddon->GetRecordingLastPlayedPosition = ADDON_GetRecordingLastPlayedPosition;
    instance->pvr->toAddon->GetRecordingEdl = ADDON_GetRecordingEdl;
    instance->pvr->toAddon->GetRecordingSize = ADDON_GetRecordingSize;
    instance->pvr->toAddon->GetRecordingStreamProperties = ADDON_GetRecordingStreamProperties;
    instance->pvr->toAddon->CallRecordingMenuHook = ADDON_CallRecordingMenuHook;
    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    instance->pvr->toAddon->GetTimerTypes = ADDON_GetTimerTypes;
    instance->pvr->toAddon->GetTimersAmount = ADDON_GetTimersAmount;
    instance->pvr->toAddon->GetTimers = ADDON_GetTimers;
    instance->pvr->toAddon->AddTimer = ADDON_AddTimer;
    instance->pvr->toAddon->DeleteTimer = ADDON_DeleteTimer;
    instance->pvr->toAddon->UpdateTimer = ADDON_UpdateTimer;
    instance->pvr->toAddon->CallTimerMenuHook = ADDON_CallTimerMenuHook;
    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    instance->pvr->toAddon->OnSystemSleep = ADDON_OnSystemSleep;
    instance->pvr->toAddon->OnSystemWake = ADDON_OnSystemWake;
    instance->pvr->toAddon->OnPowerSavingActivated = ADDON_OnPowerSavingActivated;
    instance->pvr->toAddon->OnPowerSavingDeactivated = ADDON_OnPowerSavingDeactivated;
    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    instance->pvr->toAddon->OpenLiveStream = ADDON_OpenLiveStream;
    instance->pvr->toAddon->CloseLiveStream = ADDON_CloseLiveStream;
    instance->pvr->toAddon->ReadLiveStream = ADDON_ReadLiveStream;
    instance->pvr->toAddon->SeekLiveStream = ADDON_SeekLiveStream;
    instance->pvr->toAddon->LengthLiveStream = ADDON_LengthLiveStream;
    instance->pvr->toAddon->GetStreamProperties = ADDON_GetStreamProperties;
    instance->pvr->toAddon->GetStreamReadChunkSize = ADDON_GetStreamReadChunkSize;
    instance->pvr->toAddon->IsRealTimeStream = ADDON_IsRealTimeStream;
    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    instance->pvr->toAddon->OpenRecordedStream = ADDON_OpenRecordedStream;
    instance->pvr->toAddon->CloseRecordedStream = ADDON_CloseRecordedStream;
    instance->pvr->toAddon->ReadRecordedStream = ADDON_ReadRecordedStream;
    instance->pvr->toAddon->SeekRecordedStream = ADDON_SeekRecordedStream;
    instance->pvr->toAddon->LengthRecordedStream = ADDON_LengthRecordedStream;
    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    instance->pvr->toAddon->DemuxReset = ADDON_DemuxReset;
    instance->pvr->toAddon->DemuxAbort = ADDON_DemuxAbort;
    instance->pvr->toAddon->DemuxFlush = ADDON_DemuxFlush;
    instance->pvr->toAddon->DemuxRead = ADDON_DemuxRead;
    //--==----==----==----==----==----==----==----==----==----==----==----==----==
    instance->pvr->toAddon->CanPauseStream = ADDON_CanPauseStream;
    instance->pvr->toAddon->PauseStream = ADDON_PauseStream;
    instance->pvr->toAddon->CanSeekStream = ADDON_CanSeekStream;
    instance->pvr->toAddon->SeekTime = ADDON_SeekTime;
    instance->pvr->toAddon->SetSpeed = ADDON_SetSpeed;
    instance->pvr->toAddon->FillBuffer = ADDON_FillBuffer;
    instance->pvr->toAddon->GetStreamTimes = ADDON_GetStreamTimes;

    m_instanceData = instance->pvr;
    m_instanceData->toAddon->addonInstance = this;
  }

  inline static PVR_ERROR ADDON_GetCapabilities(const AddonInstance_PVR* instance,
                                                PVR_ADDON_CAPABILITIES* capabilities)
  {
    PVRCapabilities cppCapabilities(capabilities);
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->GetCapabilities(cppCapabilities);
  }

  inline static PVR_ERROR ADDON_GetBackendName(const AddonInstance_PVR* instance,
                                               char* str,
                                               int memSize)
  {
    std::string backendName;
    PVR_ERROR err = static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
                        ->GetBackendName(backendName);
    if (err == PVR_ERROR_NO_ERROR)
      strncpy(str, backendName.c_str(), memSize);
    return err;
  }

  inline static PVR_ERROR ADDON_GetBackendVersion(const AddonInstance_PVR* instance,
                                                  char* str,
                                                  int memSize)
  {
    std::string backendVersion;
    PVR_ERROR err = static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
                        ->GetBackendVersion(backendVersion);
    if (err == PVR_ERROR_NO_ERROR)
      strncpy(str, backendVersion.c_str(), memSize);
    return err;
  }

  inline static PVR_ERROR ADDON_GetBackendHostname(const AddonInstance_PVR* instance,
                                                   char* str,
                                                   int memSize)
  {
    std::string backendHostname;
    PVR_ERROR err = static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
                        ->GetBackendHostname(backendHostname);
    if (err == PVR_ERROR_NO_ERROR)
      strncpy(str, backendHostname.c_str(), memSize);
    return err;
  }

  inline static PVR_ERROR ADDON_GetConnectionString(const AddonInstance_PVR* instance,
                                                    char* str,
                                                    int memSize)
  {
    std::string connectionString;
    PVR_ERROR err = static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
                        ->GetConnectionString(connectionString);
    if (err == PVR_ERROR_NO_ERROR)
      strncpy(str, connectionString.c_str(), memSize);
    return err;
  }

  inline static PVR_ERROR ADDON_GetDriveSpace(const AddonInstance_PVR* instance,
                                              uint64_t* total,
                                              uint64_t* used)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->GetDriveSpace(*total, *used);
  }

  inline static PVR_ERROR ADDON_CallSettingsMenuHook(const AddonInstance_PVR* instance,
                                                     const PVR_MENUHOOK* menuhook)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->CallSettingsMenuHook(menuhook);
  }

  //--==----==----==----==----==----==----==----==----==----==----==----==----==

  inline static PVR_ERROR ADDON_GetChannelsAmount(const AddonInstance_PVR* instance, int* amount)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->GetChannelsAmount(*amount);
  }

  inline static PVR_ERROR ADDON_GetChannels(const AddonInstance_PVR* instance,
                                            PVR_HANDLE handle,
                                            bool radio)
  {
    PVRChannelsResultSet result(instance, handle);
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->GetChannels(radio, result);
  }

  inline static PVR_ERROR ADDON_GetChannelStreamProperties(const AddonInstance_PVR* instance,
                                                           const PVR_CHANNEL* channel,
                                                           PVR_NAMED_VALUE* properties,
                                                           unsigned int* propertiesCount)
  {
    *propertiesCount = 0;
    std::vector<PVRStreamProperty> propertiesList;
    PVR_ERROR error = static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
                          ->GetChannelStreamProperties(channel, propertiesList);
    if (error == PVR_ERROR_NO_ERROR)
    {
      for (const auto& property : propertiesList)
      {
        strncpy(properties[*propertiesCount].strName, property.GetCStructure()->strName,
                sizeof(properties[*propertiesCount].strName) - 1);
        strncpy(properties[*propertiesCount].strValue, property.GetCStructure()->strValue,
                sizeof(properties[*propertiesCount].strValue) - 1);
        ++*propertiesCount;
        if (*propertiesCount > STREAM_MAX_PROPERTY_COUNT)
          break;
      }
    }
    return error;
  }

  inline static PVR_ERROR ADDON_GetSignalStatus(const AddonInstance_PVR* instance,
                                                int channelUid,
                                                PVR_SIGNAL_STATUS* signalStatus)
  {
    PVRSignalStatus cppSignalStatus(signalStatus);
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->GetSignalStatus(channelUid, cppSignalStatus);
  }

  inline static PVR_ERROR ADDON_GetDescrambleInfo(const AddonInstance_PVR* instance,
                                                  int channelUid,
                                                  PVR_DESCRAMBLE_INFO* descrambleInfo)
  {
    PVRDescrambleInfo cppDescrambleInfo(descrambleInfo);
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->GetDescrambleInfo(channelUid, cppDescrambleInfo);
  }

  //--==----==----==----==----==----==----==----==----==----==----==----==----==

  inline static PVR_ERROR ADDON_GetProvidersAmount(const AddonInstance_PVR* instance, int* amount)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->GetProvidersAmount(*amount);
  }

  inline static PVR_ERROR ADDON_GetProviders(const AddonInstance_PVR* instance, PVR_HANDLE handle)
  {
    PVRProvidersResultSet result(instance, handle);
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)->GetProviders(result);
  }

  //--==----==----==----==----==----==----==----==----==----==----==----==----==

  inline static PVR_ERROR ADDON_GetChannelGroupsAmount(const AddonInstance_PVR* instance,
                                                       int* amount)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->GetChannelGroupsAmount(*amount);
  }

  inline static PVR_ERROR ADDON_GetChannelGroups(const AddonInstance_PVR* instance,
                                                 PVR_HANDLE handle,
                                                 bool radio)
  {
    PVRChannelGroupsResultSet result(instance, handle);
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->GetChannelGroups(radio, result);
  }

  inline static PVR_ERROR ADDON_GetChannelGroupMembers(const AddonInstance_PVR* instance,
                                                       PVR_HANDLE handle,
                                                       const PVR_CHANNEL_GROUP* group)
  {
    PVRChannelGroupMembersResultSet result(instance, handle);
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->GetChannelGroupMembers(group, result);
  }

  //--==----==----==----==----==----==----==----==----==----==----==----==----==

  inline static PVR_ERROR ADDON_DeleteChannel(const AddonInstance_PVR* instance,
                                              const PVR_CHANNEL* channel)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->DeleteChannel(channel);
  }

  inline static PVR_ERROR ADDON_RenameChannel(const AddonInstance_PVR* instance,
                                              const PVR_CHANNEL* channel)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->RenameChannel(channel);
  }

  inline static PVR_ERROR ADDON_OpenDialogChannelSettings(const AddonInstance_PVR* instance,
                                                          const PVR_CHANNEL* channel)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->OpenDialogChannelSettings(channel);
  }

  inline static PVR_ERROR ADDON_OpenDialogChannelAdd(const AddonInstance_PVR* instance,
                                                     const PVR_CHANNEL* channel)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->OpenDialogChannelAdd(channel);
  }

  inline static PVR_ERROR ADDON_OpenDialogChannelScan(const AddonInstance_PVR* instance)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->OpenDialogChannelScan();
  }

  inline static PVR_ERROR ADDON_CallChannelMenuHook(const AddonInstance_PVR* instance,
                                                    const PVR_MENUHOOK* menuhook,
                                                    const PVR_CHANNEL* channel)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->CallChannelMenuHook(menuhook, channel);
  }

  //--==----==----==----==----==----==----==----==----==----==----==----==----==

  inline static PVR_ERROR ADDON_GetEPGForChannel(const AddonInstance_PVR* instance,
                                                 PVR_HANDLE handle,
                                                 int channelUid,
                                                 time_t start,
                                                 time_t end)
  {
    PVREPGTagsResultSet result(instance, handle);
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->GetEPGForChannel(channelUid, start, end, result);
  }

  inline static PVR_ERROR ADDON_IsEPGTagRecordable(const AddonInstance_PVR* instance,
                                                   const EPG_TAG* tag,
                                                   bool* isRecordable)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->IsEPGTagRecordable(tag, *isRecordable);
  }

  inline static PVR_ERROR ADDON_IsEPGTagPlayable(const AddonInstance_PVR* instance,
                                                 const EPG_TAG* tag,
                                                 bool* isPlayable)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->IsEPGTagPlayable(tag, *isPlayable);
  }

  inline static PVR_ERROR ADDON_GetEPGTagEdl(const AddonInstance_PVR* instance,
                                             const EPG_TAG* tag,
                                             PVR_EDL_ENTRY* edl,
                                             int* size)
  {
    std::vector<PVREDLEntry> edlList;
    PVR_ERROR error = static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
                          ->GetEPGTagEdl(tag, edlList);
    if (static_cast<int>(edlList.size()) > *size)
    {
      kodi::Log(
          ADDON_LOG_WARNING,
          "CInstancePVRClient::%s: Truncating %d EDL entries from client to permitted size %d",
          __func__, static_cast<int>(edlList.size()), *size);
      edlList.resize(*size);
    }
    *size = 0;
    if (error == PVR_ERROR_NO_ERROR)
    {
      for (const auto& edlEntry : edlList)
      {
        edl[*size] = *edlEntry;
        ++*size;
      }
    }
    return error;
  }

  inline static PVR_ERROR ADDON_GetEPGTagStreamProperties(const AddonInstance_PVR* instance,
                                                          const EPG_TAG* tag,
                                                          PVR_NAMED_VALUE* properties,
                                                          unsigned int* propertiesCount)
  {
    *propertiesCount = 0;
    std::vector<PVRStreamProperty> propertiesList;
    PVR_ERROR error = static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
                          ->GetEPGTagStreamProperties(tag, propertiesList);
    if (error == PVR_ERROR_NO_ERROR)
    {
      for (const auto& property : propertiesList)
      {
        strncpy(properties[*propertiesCount].strName, property.GetCStructure()->strName,
                sizeof(properties[*propertiesCount].strName) - 1);
        strncpy(properties[*propertiesCount].strValue, property.GetCStructure()->strValue,
                sizeof(properties[*propertiesCount].strValue) - 1);
        ++*propertiesCount;
        if (*propertiesCount > STREAM_MAX_PROPERTY_COUNT)
          break;
      }
    }
    return error;
  }

  inline static PVR_ERROR ADDON_SetEPGMaxPastDays(const AddonInstance_PVR* instance, int pastDays)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->SetEPGMaxPastDays(pastDays);
  }

  inline static PVR_ERROR ADDON_SetEPGMaxFutureDays(const AddonInstance_PVR* instance,
                                                    int futureDays)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->SetEPGMaxFutureDays(futureDays);
  }

  inline static PVR_ERROR ADDON_CallEPGMenuHook(const AddonInstance_PVR* instance,
                                                const PVR_MENUHOOK* menuhook,
                                                const EPG_TAG* tag)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->CallEPGMenuHook(menuhook, tag);
  }

  //--==----==----==----==----==----==----==----==----==----==----==----==----==

  inline static PVR_ERROR ADDON_GetRecordingsAmount(const AddonInstance_PVR* instance,
                                                    bool deleted,
                                                    int* amount)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->GetRecordingsAmount(deleted, *amount);
  }

  inline static PVR_ERROR ADDON_GetRecordings(const AddonInstance_PVR* instance,
                                              PVR_HANDLE handle,
                                              bool deleted)
  {
    PVRRecordingsResultSet result(instance, handle);
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->GetRecordings(deleted, result);
  }

  inline static PVR_ERROR ADDON_DeleteRecording(const AddonInstance_PVR* instance,
                                                const PVR_RECORDING* recording)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->DeleteRecording(recording);
  }

  inline static PVR_ERROR ADDON_UndeleteRecording(const AddonInstance_PVR* instance,
                                                  const PVR_RECORDING* recording)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->UndeleteRecording(recording);
  }

  inline static PVR_ERROR ADDON_DeleteAllRecordingsFromTrash(const AddonInstance_PVR* instance)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->DeleteAllRecordingsFromTrash();
  }

  inline static PVR_ERROR ADDON_RenameRecording(const AddonInstance_PVR* instance,
                                                const PVR_RECORDING* recording)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->RenameRecording(recording);
  }

  inline static PVR_ERROR ADDON_SetRecordingLifetime(const AddonInstance_PVR* instance,
                                                     const PVR_RECORDING* recording)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->SetRecordingLifetime(recording);
  }

  inline static PVR_ERROR ADDON_SetRecordingPlayCount(const AddonInstance_PVR* instance,
                                                      const PVR_RECORDING* recording,
                                                      int count)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->SetRecordingPlayCount(recording, count);
  }

  inline static PVR_ERROR ADDON_SetRecordingLastPlayedPosition(const AddonInstance_PVR* instance,
                                                               const PVR_RECORDING* recording,
                                                               int lastplayedposition)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->SetRecordingLastPlayedPosition(recording, lastplayedposition);
  }

  inline static PVR_ERROR ADDON_GetRecordingLastPlayedPosition(const AddonInstance_PVR* instance,
                                                               const PVR_RECORDING* recording,
                                                               int* position)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->GetRecordingLastPlayedPosition(recording, *position);
  }

  inline static PVR_ERROR ADDON_GetRecordingEdl(const AddonInstance_PVR* instance,
                                                const PVR_RECORDING* recording,
                                                PVR_EDL_ENTRY* edl,
                                                int* size)
  {
    std::vector<PVREDLEntry> edlList;
    PVR_ERROR error = static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
                          ->GetRecordingEdl(recording, edlList);
    if (static_cast<int>(edlList.size()) > *size)
    {
      kodi::Log(
          ADDON_LOG_WARNING,
          "CInstancePVRClient::%s: Truncating %d EDL entries from client to permitted size %d",
          __func__, static_cast<int>(edlList.size()), *size);
      edlList.resize(*size);
    }
    *size = 0;
    if (error == PVR_ERROR_NO_ERROR)
    {
      for (const auto& edlEntry : edlList)
      {
        edl[*size] = *edlEntry;
        ++*size;
      }
    }
    return error;
  }

  inline static PVR_ERROR ADDON_GetRecordingSize(const AddonInstance_PVR* instance,
                                                 const PVR_RECORDING* recording,
                                                 int64_t* size)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->GetRecordingSize(recording, *size);
  }

  inline static PVR_ERROR ADDON_GetRecordingStreamProperties(const AddonInstance_PVR* instance,
                                                             const PVR_RECORDING* recording,
                                                             PVR_NAMED_VALUE* properties,
                                                             unsigned int* propertiesCount)
  {
    *propertiesCount = 0;
    std::vector<PVRStreamProperty> propertiesList;
    PVR_ERROR error = static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
                          ->GetRecordingStreamProperties(recording, propertiesList);
    if (error == PVR_ERROR_NO_ERROR)
    {
      for (const auto& property : propertiesList)
      {
        strncpy(properties[*propertiesCount].strName, property.GetCStructure()->strName,
                sizeof(properties[*propertiesCount].strName) - 1);
        strncpy(properties[*propertiesCount].strValue, property.GetCStructure()->strValue,
                sizeof(properties[*propertiesCount].strValue) - 1);
        ++*propertiesCount;
        if (*propertiesCount > STREAM_MAX_PROPERTY_COUNT)
          break;
      }
    }
    return error;
  }

  inline static PVR_ERROR ADDON_CallRecordingMenuHook(const AddonInstance_PVR* instance,
                                                      const PVR_MENUHOOK* menuhook,
                                                      const PVR_RECORDING* recording)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->CallRecordingMenuHook(menuhook, recording);
  }

  //--==----==----==----==----==----==----==----==----==----==----==----==----==

  inline static PVR_ERROR ADDON_GetTimerTypes(const AddonInstance_PVR* instance,
                                              PVR_TIMER_TYPE* types,
                                              int* typesCount)
  {
    *typesCount = 0;
    std::vector<PVRTimerType> timerTypes;
    PVR_ERROR error = static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
                          ->GetTimerTypes(timerTypes);
    if (error == PVR_ERROR_NO_ERROR)
    {
      for (const auto& timerType : timerTypes)
      {
        types[*typesCount] = *timerType;
        ++*typesCount;
        if (*typesCount >= PVR_ADDON_TIMERTYPE_ARRAY_SIZE)
          break;
      }
    }
    return error;
  }

  inline static PVR_ERROR ADDON_GetTimersAmount(const AddonInstance_PVR* instance, int* amount)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->GetTimersAmount(*amount);
  }

  inline static PVR_ERROR ADDON_GetTimers(const AddonInstance_PVR* instance, PVR_HANDLE handle)
  {
    PVRTimersResultSet result(instance, handle);
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)->GetTimers(result);
  }

  inline static PVR_ERROR ADDON_AddTimer(const AddonInstance_PVR* instance, const PVR_TIMER* timer)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)->AddTimer(timer);
  }

  inline static PVR_ERROR ADDON_DeleteTimer(const AddonInstance_PVR* instance,
                                            const PVR_TIMER* timer,
                                            bool forceDelete)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->DeleteTimer(timer, forceDelete);
  }

  inline static PVR_ERROR ADDON_UpdateTimer(const AddonInstance_PVR* instance,
                                            const PVR_TIMER* timer)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)->UpdateTimer(timer);
  }

  inline static PVR_ERROR ADDON_CallTimerMenuHook(const AddonInstance_PVR* instance,
                                                  const PVR_MENUHOOK* menuhook,
                                                  const PVR_TIMER* timer)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->CallTimerMenuHook(menuhook, timer);
  }

  //--==----==----==----==----==----==----==----==----==----==----==----==----==

  inline static PVR_ERROR ADDON_OnSystemSleep(const AddonInstance_PVR* instance)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)->OnSystemSleep();
  }

  inline static PVR_ERROR ADDON_OnSystemWake(const AddonInstance_PVR* instance)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)->OnSystemWake();
  }

  inline static PVR_ERROR ADDON_OnPowerSavingActivated(const AddonInstance_PVR* instance)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->OnPowerSavingActivated();
  }

  inline static PVR_ERROR ADDON_OnPowerSavingDeactivated(const AddonInstance_PVR* instance)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->OnPowerSavingDeactivated();
  }

  // obsolete parts below
  ///@{

  inline static bool ADDON_OpenLiveStream(const AddonInstance_PVR* instance,
                                          const PVR_CHANNEL* channel)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->OpenLiveStream(channel);
  }

  inline static void ADDON_CloseLiveStream(const AddonInstance_PVR* instance)
  {
    static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)->CloseLiveStream();
  }

  inline static int ADDON_ReadLiveStream(const AddonInstance_PVR* instance,
                                         unsigned char* buffer,
                                         unsigned int size)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->ReadLiveStream(buffer, size);
  }

  inline static int64_t ADDON_SeekLiveStream(const AddonInstance_PVR* instance,
                                             int64_t position,
                                             int whence)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->SeekLiveStream(position, whence);
  }

  inline static int64_t ADDON_LengthLiveStream(const AddonInstance_PVR* instance)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)->LengthLiveStream();
  }

  inline static PVR_ERROR ADDON_GetStreamProperties(const AddonInstance_PVR* instance,
                                                    PVR_STREAM_PROPERTIES* properties)
  {
    properties->iStreamCount = 0;
    std::vector<PVRStreamProperties> cppProperties;
    PVR_ERROR err = static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
                        ->GetStreamProperties(cppProperties);
    if (err == PVR_ERROR_NO_ERROR)
    {
      for (unsigned int i = 0; i < cppProperties.size(); ++i)
      {
        memcpy(&properties->stream[i],
               static_cast<PVR_STREAM_PROPERTIES::PVR_STREAM*>(cppProperties[i]),
               sizeof(PVR_STREAM_PROPERTIES::PVR_STREAM));
        ++properties->iStreamCount;

        if (properties->iStreamCount >= PVR_STREAM_MAX_STREAMS)
        {
          kodi::Log(
              ADDON_LOG_ERROR,
              "CInstancePVRClient::%s: Addon given with '%li' more allowed streams where '%i'",
              __func__, cppProperties.size(), PVR_STREAM_MAX_STREAMS);
          break;
        }
      }
    }

    return err;
  }

  inline static PVR_ERROR ADDON_GetStreamReadChunkSize(const AddonInstance_PVR* instance,
                                                       int* chunksize)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->GetStreamReadChunkSize(*chunksize);
  }

  inline static bool ADDON_IsRealTimeStream(const AddonInstance_PVR* instance)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)->IsRealTimeStream();
  }

  inline static bool ADDON_OpenRecordedStream(const AddonInstance_PVR* instance,
                                              const PVR_RECORDING* recording)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->OpenRecordedStream(recording);
  }

  inline static void ADDON_CloseRecordedStream(const AddonInstance_PVR* instance)
  {
    static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)->CloseRecordedStream();
  }

  inline static int ADDON_ReadRecordedStream(const AddonInstance_PVR* instance,
                                             unsigned char* buffer,
                                             unsigned int size)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->ReadRecordedStream(buffer, size);
  }

  inline static int64_t ADDON_SeekRecordedStream(const AddonInstance_PVR* instance,
                                                 int64_t position,
                                                 int whence)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->SeekRecordedStream(position, whence);
  }

  inline static int64_t ADDON_LengthRecordedStream(const AddonInstance_PVR* instance)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->LengthRecordedStream();
  }

  inline static void ADDON_DemuxReset(const AddonInstance_PVR* instance)
  {
    static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)->DemuxReset();
  }

  inline static void ADDON_DemuxAbort(const AddonInstance_PVR* instance)
  {
    static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)->DemuxAbort();
  }

  inline static void ADDON_DemuxFlush(const AddonInstance_PVR* instance)
  {
    static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)->DemuxFlush();
  }

  inline static DEMUX_PACKET* ADDON_DemuxRead(const AddonInstance_PVR* instance)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)->DemuxRead();
  }

  inline static bool ADDON_CanPauseStream(const AddonInstance_PVR* instance)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)->CanPauseStream();
  }

  inline static bool ADDON_CanSeekStream(const AddonInstance_PVR* instance)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)->CanSeekStream();
  }

  inline static void ADDON_PauseStream(const AddonInstance_PVR* instance, bool bPaused)
  {
    static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)->PauseStream(bPaused);
  }

  inline static bool ADDON_SeekTime(const AddonInstance_PVR* instance,
                                    double time,
                                    bool backwards,
                                    double* startpts)
  {
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->SeekTime(time, backwards, *startpts);
  }

  inline static void ADDON_SetSpeed(const AddonInstance_PVR* instance, int speed)
  {
    static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)->SetSpeed(speed);
  }

  inline static void ADDON_FillBuffer(const AddonInstance_PVR* instance, bool mode)
  {
    static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)->FillBuffer(mode);
  }

  inline static PVR_ERROR ADDON_GetStreamTimes(const AddonInstance_PVR* instance,
                                               PVR_STREAM_TIMES* times)
  {
    PVRStreamTimes cppTimes(times);
    return static_cast<CInstancePVRClient*>(instance->toAddon->addonInstance)
        ->GetStreamTimes(cppTimes);
  }
  ///@}

  AddonInstance_PVR* m_instanceData = nullptr;
};
//}}}
//______________________________________________________________________________

} /* namespace addon */
} /* namespace kodi */

#endif /* __cplusplus */
