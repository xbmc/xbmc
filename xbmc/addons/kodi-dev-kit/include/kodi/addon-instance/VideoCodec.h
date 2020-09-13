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

class ATTRIBUTE_HIDDEN VideoCodecInitdata
  : public CStructHdl<VideoCodecInitdata, VIDEOCODEC_INITDATA>
{
  friend class CInstanceVideoCodec;

public:
  VIDEOCODEC_TYPE GetCodecType() const { return m_cStructure->codec; }

  STREAMCODEC_PROFILE GetCodecProfile() const { return m_cStructure->codecProfile; }

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

  uint32_t GetWidth() const { return m_cStructure->width; }

  uint32_t GetHeight() const { return m_cStructure->height; }

  const uint8_t* GetExtraData() const { return m_cStructure->extraData; }

  unsigned int GetExtraDataSize() const { return m_cStructure->extraDataSize; }

  kodi::addon::StreamCryptoSession GetCryptoSession() const { return &m_cStructure->cryptoSession; }

private:
  VideoCodecInitdata() = delete;
  VideoCodecInitdata(const VideoCodecInitdata& session) : CStructHdl(session) {}
  VideoCodecInitdata(const VIDEOCODEC_INITDATA* session) : CStructHdl(session) {}
  VideoCodecInitdata(VIDEOCODEC_INITDATA* session) : CStructHdl(session) {}
};

class ATTRIBUTE_HIDDEN CInstanceVideoCodec : public IAddonInstance
{
public:
  explicit CInstanceVideoCodec(KODI_HANDLE instance, const std::string& kodiVersion = "")
    : IAddonInstance(ADDON_INSTANCE_VIDEOCODEC,
                     !kodiVersion.empty() ? kodiVersion
                                          : GetKodiTypeVersion(ADDON_INSTANCE_VIDEOCODEC))
  {
    if (CAddonBase::m_interface->globalSingleInstance != nullptr)
      throw std::logic_error("kodi::addon::CInstanceVideoCodec: Creation of multiple together with "
                             "single instance way is not allowed!");

    SetAddonStruct(instance);
  }

  ~CInstanceVideoCodec() override = default;

  //! \copydoc CInstanceVideoCodec::Open
  virtual bool Open(const kodi::addon::VideoCodecInitdata& initData) { return false; };

  //! \copydoc CInstanceVideoCodec::Reconfigure
  virtual bool Reconfigure(const kodi::addon::VideoCodecInitdata& initData) { return false; };

  //! \copydoc CInstanceVideoCodec::AddData
  virtual bool AddData(const DEMUX_PACKET& packet) { return false; };

  //! \copydoc CInstanceVideoCodec::GetPicture
  virtual VIDEOCODEC_RETVAL GetPicture(VIDEOCODEC_PICTURE& picture) { return VC_ERROR; };

  //! \copydoc CInstanceVideoCodec::GetName
  virtual const char* GetName() { return nullptr; };

  //! \copydoc CInstanceVideoCodec::Reset
  virtual void Reset(){};

  /*!
   * @brief AddonToKodi interface
   */

  //! \copydoc CInstanceVideoCodec::GetFrameBuffer
  bool GetFrameBuffer(VIDEOCODEC_PICTURE& picture)
  {
    return m_instanceData->toKodi->get_frame_buffer(m_instanceData->toKodi->kodiInstance, &picture);
  }

  //! \copydoc CInstanceVideoCodec::ReleaseFrameBuffer
  void ReleaseFrameBuffer(void* buffer)
  {
    return m_instanceData->toKodi->release_frame_buffer(m_instanceData->toKodi->kodiInstance,
                                                        buffer);
  }

private:
  void SetAddonStruct(KODI_HANDLE instance)
  {
    if (instance == nullptr)
      throw std::logic_error("kodi::addon::CInstanceVideoCodec: Creation with empty addon "
                             "structure not allowed, table must be given from Kodi!");

    m_instanceData = static_cast<AddonInstance_VideoCodec*>(instance);

    m_instanceData->toAddon->addonInstance = this;
    m_instanceData->toAddon->open = ADDON_Open;
    m_instanceData->toAddon->reconfigure = ADDON_Reconfigure;
    m_instanceData->toAddon->add_data = ADDON_AddData;
    m_instanceData->toAddon->get_picture = ADDON_GetPicture;
    m_instanceData->toAddon->get_name = ADDON_GetName;
    m_instanceData->toAddon->reset = ADDON_Reset;
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
