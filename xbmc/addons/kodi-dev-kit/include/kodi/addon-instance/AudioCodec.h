/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../AddonBase.h"
#include "../c-api/addon-instance/audio_codec.h"
#include "inputstream/DemuxPacket.h"
#include "inputstream/StreamCodec.h"
#include "inputstream/StreamCrypto.h"

#ifdef __cplusplus

namespace kodi
{
namespace addon
{

class CInstanceAudioCodec;

//==============================================================================
/// @defgroup cpp_kodi_addon_audiocodec_Defs_AudioCodecInitdata class AudioCodecInitdata
/// @ingroup cpp_kodi_addon_audiocodec_Defs
/// @brief Initialization data to open an audio codec stream.
///
/// ----------------------------------------------------------------------------
///
/// @copydetails cpp_kodi_addon_audiocodec_Defs_AudioCodecInitdata_Help
///
///@{
class ATTR_DLL_LOCAL AudioCodecInitdata : public CStructHdl<AudioCodecInitdata, AUDIOCODEC_INITDATA>
{
  /*! \cond PRIVATE */
  friend class CInstanceAudioCodec;
  /*! \endcond */

public:
  /// @defgroup cpp_kodi_addon_audiocodec_Defs_AudioCodecInitdata_Help Value Help
  /// @ingroup cpp_kodi_addon_audiocodec_Defs_AudioCodecInitdata
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_addon_audiocodec_Defs_AudioCodecInitdata :</b>
  /// | Name | Type | Get call
  /// |------|------|----------
  /// | **Codec type** | `AUDIOCODEC_TYPE` | @ref AudioCodecInitdata::GetCodecType "GetCodecType"
  /// | **Codec profile** | `STREAMCODEC_PROFILE` | @ref AudioCodecInitdata::GetCodecProfile "GetCodecProfile"
  /// | **Audio formats** | `std::vector<AUDIOCODEC_FORMAT>` | @ref AudioCodecInitdata::GetAudioFormats "GetAudioFormats"
  /// | **Extra data** | `const uint8_t*` | @ref AudioCodecInitdata::GetExtraData "GetExtraData"
  /// | **Extra data size** | `unsigned int` | @ref AudioCodecInitdata::GetExtraDataSize "GetExtraDataSize"
  /// | **Crypto session** | `kodi::addon::StreamCryptoSession` | @ref AudioCodecInitdata::GetCryptoSession "GetCryptoSession"
  ///

  /// @addtogroup cpp_kodi_addon_audiocodec_Defs_AudioCodecInitdata
  ///@{

  /// @brief The codec type required by Kodi to process the stream.
  ///
  /// See @ref AUDIOCODEC_TYPE for possible values.
  AUDIOCODEC_TYPE GetCodecType() const { return m_cStructure->codec; }

  /// @brief Used profile
  STREAMCODEC_PROFILE GetCodecProfile() const { return m_cStructure->codecProfile; }

  /// @brief Depending on the required decoding, additional data given by the stream.
  const uint8_t* GetExtraData() const { return m_cStructure->extraData; }

  /// @brief Size of the data given with @ref extraData.
  unsigned int GetExtraDataSize() const { return m_cStructure->extraDataSize; }

  /// @brief **Data to manage stream cryptography**\n
  /// To get class structure manages any encryption values required in order to have
  /// them available in their stream processing.
  ///
  /// ----------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_inputstream_Defs_Info_StreamCryptoSession_Help
  ///
  kodi::addon::StreamCryptoSession GetCryptoSession() const { return &m_cStructure->cryptoSession; }

  ///@}

private:
  AudioCodecInitdata() = delete;
  AudioCodecInitdata(const AudioCodecInitdata& session) : CStructHdl(session) {}
  AudioCodecInitdata(const AUDIOCODEC_INITDATA* session) : CStructHdl(session) {}
  AudioCodecInitdata(AUDIOCODEC_INITDATA* session) : CStructHdl(session) {}
};
///@}
//------------------------------------------------------------------------------

//##############################################################################
/// @defgroup cpp_kodi_addon_audiocodec_Defs Definitions, structures and enumerators
/// @ingroup cpp_kodi_addon_audiocodec
/// @brief **Audio codec add-on general variables**
///
/// Used to exchange the available options between Kodi and addon.
///
///

//============================================================================
///
/// @addtogroup cpp_kodi_addon_audiocodec
/// @brief \cpp_class{ kodi::addon::CInstanceAudioCodec }
/// **Audio codec add-on instance**
///
/// This is an addon instance class to add an additional audio decoder to Kodi
/// using addon.
///
/// This means that either a new type of decoding can be introduced to an input
/// stream add-on that requires special types of decoding.
///
/// When using the inputstream addon, @ref cpp_kodi_addon_inputstream_Defs_Interface_InputstreamInfo
/// to @ref cpp_kodi_addon_inputstream_Defs_Info is used to declare that the
/// decoder stored in the addon is used.
///
/// @note At the moment this can only be used together with input stream addons,
/// independent use as a codec addon is not yet possible.
///
/// Include the header @ref AudioCodec.h "#include <kodi/addon-instance/AudioCodec.h>"
/// to use this class.
///
/// --------------------------------------------------------------------------
///
/// **Example:**
/// This as an example when used together with @ref cpp_kodi_addon_inputstream "kodi::addon::CInstanceInputStream".
///
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/addon-instance/Inputstream.h>
/// #include <kodi/addon-instance/AudioCodec.h>
///
/// class CMyAudioCodec : public kodi::addon::CInstanceAudioCodec
/// {
/// public:
///   CMyAudioCodec(const kodi::addon::IInstanceInfo& instance,
///                 CMyInputstream* inputstream);
///   ...
///
/// private:
///   CMyInputstream* m_inputstream;
/// };
///
/// CMyAudioCodec::CMyAudioCodec(const kodi::addon::IInstanceInfo& instance,
///                              CMyInputstream* inputstream)
///   : kodi::addon::CInstanceAudioCodec(instance),
///     m_inputstream(inputstream)
/// {
///   ...
/// }
/// ...
///
/// //----------------------------------------------------------------------
///
/// class CMyInputstream : public kodi::addon::CInstanceInputStream
/// {
/// public:
///   CMyInputstream(KODI_HANDLE instance, const std::string& kodiVersion);
///
///   ADDON_STATUS CreateInstance(const kodi::addon::IInstanceInfo& instance,
///                               KODI_ADDON_INSTANCE_HDL& hdl) override;
///   ...
/// };
///
/// CMyInputstream::CMyInputstream(KODI_HANDLE instance, const std::string& kodiVersion)
///   : kodi::addon::CInstanceInputStream(instance, kodiVersion)
/// {
///   ...
/// }
///
/// ADDON_STATUS CMyInputstream::CreateInstance(const kodi::addon::IInstanceInfo& instance,
///                                             KODI_ADDON_INSTANCE_HDL& hdl)
/// {
/// {
///   if (instance.IsType(ADDON_INSTANCE_AUDIOCODEC))
///   {
///     addonInstance = new CMyAudioCodec(instance, this);
///     return ADDON_STATUS_OK;
///   }
///   return ADDON_STATUS_NOT_IMPLEMENTED;
/// }
///
/// ...
///
/// //----------------------------------------------------------------------
///
/// class CMyAddon : public kodi::addon::CAddonBase
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
///   if (instance.IsType(ADDON_INSTANCE_INPUTSTREAM))
///   {
///     kodi::Log(ADDON_LOG_NOTICE, "Creating my Inputstream");
///     hdl = new CMyInputstream(instance);
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
/// The destruction of the example class `CMyInputstream` is called from
/// Kodi's header. Manually deleting the add-on instance is not required.
///
///
///
class ATTR_DLL_LOCAL CInstanceAudioCodec : public IAddonInstance
{
public:
  //============================================================================
  /// @ingroup cpp_kodi_addon_audiocodec
  /// @brief Audio codec class constructor used to support multiple instance
  /// types
  ///
  /// @param[in] instance The instance value given to <b>`kodi::addon::CAddonBase::CreateInstance(...)`</b>,
  ///                     or by a inputstream instance if them declared as parent.
  /// @param[in] kodiVersion [opt] Version used in Kodi for this instance, to
  ///                        allow compatibility to older Kodi versions.
  ///
  explicit CInstanceAudioCodec(const IInstanceInfo& instance) : IAddonInstance(instance)
  {
    if (CPrivateBase::m_interface->globalSingleInstance != nullptr)
      throw std::logic_error("kodi::addon::CInstanceAudioCodec: Creation of multiple together with "
                             "single instance way is not allowed!");

    SetAddonStruct(instance);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_audiocodec
  /// @brief Destructor
  ///
  ~CInstanceAudioCodec() override = default;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_audiocodec
  /// @brief Open the decoder, returns true on success
  ///
  /// Decoders not capable of running multiple instances should return false in case
  /// there is already a instance open.
  ///
  /// @param[in] initData Audio codec init data
  /// @return true if successfully done
  ///
  ///
  /// ----------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_audiocodec_Defs_AudioCodecInitdata_Help
  ///
  virtual bool Open(const kodi::addon::AudioCodecInitdata& initData) { return false; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_audiocodec
  /// @brief add data, decoder has to consume the entire packet
  ///
  /// @param[in] packet Data to process for decode
  /// @return true if the packet was consumed or if resubmitting it is useless
  ///
  virtual bool AddData(const DEMUX_PACKET& packet) { return false; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_audiocodec
  /// @brief GetFrame controls decoding.
  ///
  /// Player calls it on every cycle it can signal a frame, request a buffer,
  /// or return none, if nothing applies the data is valid until the next
  /// GetFramee return @ref VC_FRAME
  ///
  /// @param[in,out] Structure which contains the necessary data
  /// @return The with @ref AUDIOCODEC_RETVAL return values
  ///
  virtual AUDIOCODEC_RETVAL GetFrame(AUDIOCODEC_FRAME& frame) { return AC_ERROR; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_audiocodec
  /// @brief should return codecs name
  ///
  /// @return Codec name
  ///
  virtual const char* GetName() { return nullptr; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_audiocodec
  /// @brief Reset the decoder
  ///
  virtual void Reset() {}
  //----------------------------------------------------------------------------

  /*!
   * @brief AddonToKodi interface
   */

  //============================================================================
  /// @ingroup cpp_kodi_addon_audiocodec
  /// @brief All frame members can be expected to be set correctly except
  /// decodedData and pts.
  ///
  /// GetFrameBuffer has to set decodedData to a valid memory address and return true.
  ///
  /// @param[out] frame The buffer, or unmodified if false is returned
  /// @return In case buffer allocation fails, it return false.
  ///
  /// @note If this returns true, buffer must be freed using @ref ReleaseFrameBuffer().
  ///
  /// @remarks Only called from addon itself
  ///
  bool GetFrameBuffer(AUDIOCODEC_FRAME& frame)
  {
    return m_instanceData->toKodi->get_frame_buffer(m_instanceData->toKodi->kodiInstance, &frame);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  /// @ingroup cpp_kodi_addon_audiocodec
  /// @brief Release the with @ref GetFrameBuffer() given framebuffer.
  ///
  /// @param[in] handle the on @ref AUDIOCODEC_FRAME.audioBufferHandle defined buffer handle
  ///
  /// @remarks Only called from addon itself
  ///
  void ReleaseFrameBuffer(void* buffer)
  {
    return m_instanceData->toKodi->release_frame_buffer(m_instanceData->toKodi->kodiInstance,
                                                        buffer);
  }
  //----------------------------------------------------------------------------

private:
  void SetAddonStruct(KODI_ADDON_INSTANCE_STRUCT* instance)
  {
    instance->hdl = this;
    instance->audiocodec->toAddon->open = ADDON_Open;
    instance->audiocodec->toAddon->add_data = ADDON_AddData;
    instance->audiocodec->toAddon->get_frame = ADDON_GetFrame;
    instance->audiocodec->toAddon->get_name = ADDON_GetName;
    instance->audiocodec->toAddon->reset = ADDON_Reset;

    m_instanceData = instance->audiocodec;
    m_instanceData->toAddon->addonInstance = this;
  }

  inline static bool ADDON_Open(const AddonInstance_AudioCodec* instance,
                                AUDIOCODEC_INITDATA* initData)
  {
    return static_cast<CInstanceAudioCodec*>(instance->toAddon->addonInstance)->Open(initData);
  }

  inline static bool ADDON_AddData(const AddonInstance_AudioCodec* instance,
                                   const DEMUX_PACKET* packet)
  {
    return static_cast<CInstanceAudioCodec*>(instance->toAddon->addonInstance)->AddData(*packet);
  }

  inline static AUDIOCODEC_RETVAL ADDON_GetFrame(const AddonInstance_AudioCodec* instance,
                                                   AUDIOCODEC_FRAME* frame)
  {
    return static_cast<CInstanceAudioCodec*>(instance->toAddon->addonInstance)
        ->GetFrame(*frame);
  }

  inline static const char* ADDON_GetName(const AddonInstance_AudioCodec* instance)
  {
    return static_cast<CInstanceAudioCodec*>(instance->toAddon->addonInstance)->GetName();
  }

  inline static void ADDON_Reset(const AddonInstance_AudioCodec* instance)
  {
    return static_cast<CInstanceAudioCodec*>(instance->toAddon->addonInstance)->Reset();
  }

  AddonInstance_AudioCodec* m_instanceData;
};

} // namespace addon
} // namespace kodi

#endif /* __cplusplus */
