/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../AddonBase.h"
#include "../c-api/addon-instance/video_codec.h"
#include "inputstream/DemuxPacket.h"
#include "inputstream/StreamCodec.h"
#include "inputstream/StreamCrypto.h"

#ifdef __cplusplus

namespace kodi
{
namespace addon
{

class CInstanceVideoCodec;

//==============================================================================
/// @defgroup cpp_kodi_addon_videocodec_Defs_VideoCodecInitdata class VideoCodecInitdata
/// @ingroup cpp_kodi_addon_videocodec_Defs
/// @brief Initialization data to open a video codec stream.
///
/// ----------------------------------------------------------------------------
///
/// @copydetails cpp_kodi_addon_videocodec_Defs_VideoCodecInitdata_Help
///
///@{
class ATTR_DLL_LOCAL VideoCodecInitdata : public CStructHdl<VideoCodecInitdata, VIDEOCODEC_INITDATA>
{
  /*! \cond PRIVATE */
  friend class CInstanceVideoCodec;
  /*! \endcond */

public:
  /// @defgroup cpp_kodi_addon_videocodec_Defs_VideoCodecInitdata_Help Value Help
  /// @ingroup cpp_kodi_addon_videocodec_Defs_VideoCodecInitdata
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_addon_videocodec_Defs_VideoCodecInitdata :</b>
  /// | Name | Type | Get call
  /// |------|------|----------
  /// | **Codec type** | `VIDEOCODEC_TYPE` | @ref VideoCodecInitdata::GetCodecType "GetCodecType"
  /// | **Codec profile** | `STREAMCODEC_PROFILE` | @ref VideoCodecInitdata::GetCodecProfile "GetCodecProfile"
  /// | **Video formats** | `std::vector<VIDEOCODEC_FORMAT>` | @ref VideoCodecInitdata::GetVideoFormats "GetVideoFormats"
  /// | **Width** | `uint32_t` | @ref VideoCodecInitdata::GetWidth "GetWidth"
  /// | **Height** | `uint32_t` | @ref VideoCodecInitdata::GetHeight "GetHeight"
  /// | **Extra data** | `const uint8_t*` | @ref VideoCodecInitdata::GetExtraData "GetExtraData"
  /// | **Extra data size** | `unsigned int` | @ref VideoCodecInitdata::GetExtraDataSize "GetExtraDataSize"
  /// | **Crypto session** | `kodi::addon::StreamCryptoSession` | @ref VideoCodecInitdata::GetCryptoSession "GetCryptoSession"
  ///

  /// @addtogroup cpp_kodi_addon_videocodec_Defs_VideoCodecInitdata
  ///@{

  /// @brief The codec type required by Kodi to process the stream.
  ///
  /// See @ref VIDEOCODEC_TYPE for possible values.
  VIDEOCODEC_TYPE GetCodecType() const { return m_cStructure->codec; }

  /// @brief Used profiles for non-scalable 2D video
  STREAMCODEC_PROFILE GetCodecProfile() const { return m_cStructure->codecProfile; }

  /// @brief The video stream representations requested by Kodi
  ///
  /// This contains a list of the required video formats. One of them has to
  /// select the addon to return the created image.
  ///
  std::vector<VIDEOCODEC_FORMAT> GetVideoFormats() const
  {
    std::vector<VIDEOCODEC_FORMAT> formats;
    unsigned int i = 0;
    while (i < VIDEOCODEC_FORMAT_MAXFORMATS &&
           m_cStructure->videoFormats[i] != VIDEOCODEC_FORMAT_UNKNOWN)
      formats.emplace_back(m_cStructure->videoFormats[i++]);
    if (formats.empty())
      formats.emplace_back(VIDEOCODEC_FORMAT_UNKNOWN);
    return formats;
  }

  /// @brief Picture width.
  uint32_t GetWidth() const { return m_cStructure->width; }

  /// @brief Picture height.
  uint32_t GetHeight() const { return m_cStructure->height; }

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
  VideoCodecInitdata() = delete;
  VideoCodecInitdata(const VideoCodecInitdata& session) : CStructHdl(session) {}
  VideoCodecInitdata(const VIDEOCODEC_INITDATA* session) : CStructHdl(session) {}
  VideoCodecInitdata(VIDEOCODEC_INITDATA* session) : CStructHdl(session) {}
};
///@}
//------------------------------------------------------------------------------

//##############################################################################
/// @defgroup cpp_kodi_addon_videocodec_Defs Definitions, structures and enumerators
/// @ingroup cpp_kodi_addon_videocodec
/// @brief **Video codec add-on general variables**
///
/// Used to exchange the available options between Kodi and addon.
///
///

//============================================================================
///
/// @addtogroup cpp_kodi_addon_videocodec
/// @brief \cpp_class{ kodi::addon::CInstanceVideoCodec }
/// **Video codec add-on instance**
///
/// This is an addon instance class to add an additional video decoder to Kodi
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
/// Include the header @ref VideoCodec.h "#include <kodi/addon-instance/VideoCodec.h>"
/// to use this class.
///
/// --------------------------------------------------------------------------
///
/// **Example:**
/// This as an example when used together with @ref cpp_kodi_addon_inputstream "kodi::addon::CInstanceInputStream".
///
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/addon-instance/Inputstream.h>
/// #include <kodi/addon-instance/VideoCodec.h>
///
/// class CMyVideoCodec : public kodi::addon::CInstanceVideoCodec
/// {
/// public:
///   CMyVideoCodec(const kodi::addon::IInstanceInfo& instance,
///                 CMyInputstream* inputstream);
///   ...
///
/// private:
///   CMyInputstream* m_inputstream;
/// };
///
/// CMyVideoCodec::CMyVideoCodec(const kodi::addon::IInstanceInfo& instance,
///                              CMyInputstream* inputstream)
///   : kodi::addon::CInstanceVideoCodec(instance),
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
///   if (instance.IsType(ADDON_INSTANCE_VIDEOCODEC))
///   {
///     addonInstance = new CMyVideoCodec(instance, this);
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
class ATTR_DLL_LOCAL CInstanceVideoCodec : public IAddonInstance
{
public:
  //============================================================================
  /// @ingroup cpp_kodi_addon_videocodec
  /// @brief Video codec class constructor used to support multiple instance
  /// types
  ///
  /// @param[in] instance The instance value given to <b>`kodi::addon::CAddonBase::CreateInstance(...)`</b>,
  ///                     or by a inputstream instance if them declared as parent.
  /// @param[in] kodiVersion [opt] Version used in Kodi for this instance, to
  ///                        allow compatibility to older Kodi versions.
  ///
  explicit CInstanceVideoCodec(const IInstanceInfo& instance) : IAddonInstance(instance)
  {
    if (CPrivateBase::m_interface->globalSingleInstance != nullptr)
      throw std::logic_error("kodi::addon::CInstanceVideoCodec: Creation of multiple together with "
                             "single instance way is not allowed!");

    SetAddonStruct(instance);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_videocodec
  /// @brief Destructor
  ///
  ~CInstanceVideoCodec() override = default;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_videocodec
  /// @brief Open the decoder, returns true on success
  ///
  /// Decoders not capable of running multiple instances should return false in case
  /// there is already a instance open.
  ///
  /// @param[in] initData Video codec init data
  /// @return true if successfully done
  ///
  ///
  /// ----------------------------------------------------------------------------
  ///
  /// @copydetails cpp_kodi_addon_videocodec_Defs_VideoCodecInitdata_Help
  ///
  virtual bool Open(const kodi::addon::VideoCodecInitdata& initData) { return false; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_videocodec
  /// @brief Reconfigure the decoder, returns true on success
  ///
  /// Decoders not capable of running multiple instances may be capable of reconfiguring
  /// the running instance. If Reconfigure returns false, player will close / open
  /// the decoder
  ///
  /// @param[in] initData Video codec reconfigure data
  /// @return true if successfully done
  ///
  virtual bool Reconfigure(const kodi::addon::VideoCodecInitdata& initData) { return false; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_videocodec
  /// @brief add data, decoder has to consume the entire packet
  ///
  /// @param[in] packet Data to process for decode
  /// @return true if the packet was consumed or if resubmitting it is useless
  ///
  virtual bool AddData(const DEMUX_PACKET& packet) { return false; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_videocodec
  /// @brief GetPicture controls decoding.
  ///
  /// Player calls it on every cycle it can signal a picture, request a buffer,
  /// or return none, if nothing applies the data is valid until the next
  /// GetPicture return @ref VC_PICTURE
  ///
  /// @param[in,out] Structure which contains the necessary data
  /// @return The with @ref VIDEOCODEC_RETVAL return values
  ///
  virtual VIDEOCODEC_RETVAL GetPicture(VIDEOCODEC_PICTURE& picture) { return VC_ERROR; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_videocodec
  /// @brief should return codecs name
  ///
  /// @return Codec name
  ///
  virtual const char* GetName() { return nullptr; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_videocodec
  /// @brief Reset the decoder
  ///
  virtual void Reset() {}
  //----------------------------------------------------------------------------

  /*!
   * @brief AddonToKodi interface
   */

  //============================================================================
  /// @ingroup cpp_kodi_addon_videocodec
  /// @brief All picture members can be expected to be set correctly except
  /// decodedData and pts.
  ///
  /// GetFrameBuffer has to set decodedData to a valid memory address and return true.
  ///
  /// @param[out] picture The buffer, or unmodified if false is returned
  /// @return In case buffer allocation fails, it return false.
  ///
  /// @note If this returns true, buffer must be freed using @ref ReleaseFrameBuffer().
  ///
  /// @remarks Only called from addon itself
  ///
  bool GetFrameBuffer(VIDEOCODEC_PICTURE& picture)
  {
    return m_instanceData->toKodi->get_frame_buffer(m_instanceData->toKodi->kodiInstance, &picture);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  /// @ingroup cpp_kodi_addon_videocodec
  /// @brief Release the with @ref GetFrameBuffer() given framebuffer.
  ///
  /// @param[in] handle the on @ref VIDEOCODEC_PICTURE.videoBufferHandle defined buffer handle
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
    instance->videocodec->toAddon->open = ADDON_Open;
    instance->videocodec->toAddon->reconfigure = ADDON_Reconfigure;
    instance->videocodec->toAddon->add_data = ADDON_AddData;
    instance->videocodec->toAddon->get_picture = ADDON_GetPicture;
    instance->videocodec->toAddon->get_name = ADDON_GetName;
    instance->videocodec->toAddon->reset = ADDON_Reset;

    m_instanceData = instance->videocodec;
    m_instanceData->toAddon->addonInstance = this;
  }

  inline static bool ADDON_Open(const AddonInstance_VideoCodec* instance,
                                VIDEOCODEC_INITDATA* initData)
  {
    return static_cast<CInstanceVideoCodec*>(instance->toAddon->addonInstance)->Open(initData);
  }

  inline static bool ADDON_Reconfigure(const AddonInstance_VideoCodec* instance,
                                       VIDEOCODEC_INITDATA* initData)
  {
    return static_cast<CInstanceVideoCodec*>(instance->toAddon->addonInstance)
        ->Reconfigure(initData);
  }

  inline static bool ADDON_AddData(const AddonInstance_VideoCodec* instance,
                                   const DEMUX_PACKET* packet)
  {
    return static_cast<CInstanceVideoCodec*>(instance->toAddon->addonInstance)->AddData(*packet);
  }

  inline static VIDEOCODEC_RETVAL ADDON_GetPicture(const AddonInstance_VideoCodec* instance,
                                                   VIDEOCODEC_PICTURE* picture)
  {
    return static_cast<CInstanceVideoCodec*>(instance->toAddon->addonInstance)
        ->GetPicture(*picture);
  }

  inline static const char* ADDON_GetName(const AddonInstance_VideoCodec* instance)
  {
    return static_cast<CInstanceVideoCodec*>(instance->toAddon->addonInstance)->GetName();
  }

  inline static void ADDON_Reset(const AddonInstance_VideoCodec* instance)
  {
    return static_cast<CInstanceVideoCodec*>(instance->toAddon->addonInstance)->Reset();
  }

  AddonInstance_VideoCodec* m_instanceData;
};

} // namespace addon
} // namespace kodi

#endif /* __cplusplus */
