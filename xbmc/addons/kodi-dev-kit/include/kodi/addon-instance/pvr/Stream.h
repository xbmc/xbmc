/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../../AddonBase.h"
#include "../../c-api/addon-instance/pvr/pvr_stream.h"

//¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
// "C++" Definitions group 9 - PVR stream definitions (NOTE: Becomes replaced
// in future by inputstream addon instance way)

#ifdef __cplusplus

namespace kodi
{
namespace addon
{

//==============================================================================
/// @defgroup cpp_kodi_addon_pvr_Defs_Stream_PVRCodec class PVRCodec
/// @ingroup cpp_kodi_addon_pvr_Defs_Stream
/// @brief **PVR codec identifier**\n
/// Used to exchange the desired codec type between Kodi and addon.
///
/// @ref kodi::addon::CInstancePVRClient::GetCodecByName is used to get this data.
///
/// ----------------------------------------------------------------------------
///
/// @copydetails cpp_kodi_addon_pvr_Defs_Stream_PVRCodec_Help
///
///@{
class PVRCodec : public CStructHdl<PVRCodec, PVR_CODEC>
{
  friend class CInstancePVRClient;

public:
  /*! \cond PRIVATE */
  PVRCodec()
  {
    m_cStructure->codec_type = PVR_CODEC_TYPE_UNKNOWN;
    m_cStructure->codec_id = PVR_INVALID_CODEC_ID;
  }
  PVRCodec(const PVRCodec& type) : CStructHdl(type) {}
  /*! \endcond */

  /// @defgroup cpp_kodi_addon_pvr_Defs_Stream_PVRCodec_Help Value Help
  /// @ingroup cpp_kodi_addon_pvr_Defs_Stream_PVRCodec
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_addon_pvr_Defs_Stream_PVRCodec :</b>
  /// | Name | Type | Set call | Get call
  /// |------|------|----------|----------
  /// | **Codec type** | @ref PVR_CODEC_TYPE | @ref PVRCodec::SetCodecType "SetCodecType" | @ref PVRCodec::GetCodecType "GetCodecType"
  /// | **Codec identifier** | `unsigned int` | @ref PVRCodec::SetCodecId "SetCodecId" | @ref PVRCodec::GetCodecId "GetCodecId"
  ///

  /// @addtogroup cpp_kodi_addon_pvr_Defs_Stream_PVRCodec
  ///@{

  /// @brief Codec type.
  void SetCodecType(PVR_CODEC_TYPE codecType) { m_cStructure->codec_type = codecType; }

  /// @brief To get with @ref SetCodecType() changed values.
  PVR_CODEC_TYPE GetCodecType() const { return m_cStructure->codec_type; }

  /// @brief Codec id.
  ///
  /// Related codec identifier, normally match the ffmpeg id's.
  void SetCodecId(unsigned int codecId) { m_cStructure->codec_id = codecId; }

  /// @brief To get with @ref SetCodecId() changed values.
  unsigned int GetCodecId() const { return m_cStructure->codec_id; }
  ///@}

private:
  PVRCodec(const PVR_CODEC& type) : CStructHdl(&type) {}
  PVRCodec(const PVR_CODEC* type) : CStructHdl(type) {}
  PVRCodec(PVR_CODEC* type) : CStructHdl(type) {}
};
///@}
//------------------------------------------------------------------------------

//==============================================================================
/// @defgroup cpp_kodi_addon_pvr_Defs_Stream_PVRStreamProperties class PVRStreamProperties
/// @ingroup cpp_kodi_addon_pvr_Defs_Stream
/// @brief **PVR stream properties**\n
/// All information about a respective stream is stored in this, so that Kodi
/// can process the data given by the addon after demux.
///
/// ----------------------------------------------------------------------------
///
/// @copydetails cpp_kodi_addon_pvr_Defs_Stream_PVRStreamProperties_Help
///
///@{
class PVRStreamProperties
  : public CStructHdl<PVRStreamProperties, PVR_STREAM_PROPERTIES::PVR_STREAM>
{
  friend class CInstancePVRClient;

public:
  /*! \cond PRIVATE */
  PVRStreamProperties() { memset(m_cStructure, 0, sizeof(PVR_STREAM_PROPERTIES::PVR_STREAM)); }
  PVRStreamProperties(const PVRStreamProperties& type) : CStructHdl(type) {}
  /*! \endcond */

  /// @defgroup cpp_kodi_addon_pvr_Defs_Stream_PVRStreamProperties_Help Value Help
  /// @ingroup cpp_kodi_addon_pvr_Defs_Stream_PVRStreamProperties
  /// ----------------------------------------------------------------------------
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_addon_pvr_Defs_Stream_PVRStreamProperties :</b>
  /// | Name | Type | Set call | Get call
  /// |------|------|----------|----------
  /// | **PID** | `unsigned int` | @ref PVRStreamProperties::SetPID "SetPID" | @ref PVRStreamProperties::GetPID "GetPID"
  /// | **Codec type** | @ref PVR_CODEC_TYPE | @ref PVRStreamProperties::SetCodecType "SetCodecType" | @ref PVRStreamProperties::GetCodecType "GetCodecType"
  /// | **Codec identifier** | `unsigned int` | @ref PVRStreamProperties::SetCodecId "SetCodecId" | @ref PVRStreamProperties::GetCodecId "GetCodecId"
  /// | **Language** | `std::string` | @ref PVRStreamProperties::SetLanguage "SetLanguage" | @ref PVRStreamProperties::GetLanguage "GetLanguage"
  /// | **Subtitle info** | `int` | @ref PVRStreamProperties::SetSubtitleInfo "SetSubtitleInfo" | @ref PVRStreamProperties::GetSubtitleInfo "GetSubtitleInfo"
  /// | **FPS scale** | `int` | @ref PVRStreamProperties::SetFPSScale "SetFPSScale" | @ref PVRStreamProperties::GetFPSScale "GetFPSScale"
  /// | **FPS rate** | `int` | @ref PVRStreamProperties::SetFPSRate "SetFPSRate" | @ref PVRStreamProperties::GetFPSRate "GetFPSRate"
  /// | **Height** | `int` | @ref PVRStreamProperties::SetHeight "SetHeight" | @ref PVRStreamProperties::GetHeight "GetHeight"
  /// | **Width** | `int` | @ref PVRStreamProperties::SetWidth "SetWidth" | @ref PVRStreamProperties::GetWidth "GetWidth"
  /// | **Aspect ratio** | `float` | @ref PVRStreamProperties::SetAspect "SetAspect" | @ref PVRStreamProperties::GetAspect "GetAspect"
  /// | **Channels** | `int` | @ref PVRStreamProperties::SetChannels "SetChannels" | @ref PVRStreamProperties::GetChannels "GetChannels"
  /// | **Samplerate** | `int` | @ref PVRStreamProperties::SetSampleRate "SetSampleRate" | @ref PVRStreamProperties::GetSampleRate "GetSampleRate"
  /// | **Block align** | `int` | @ref PVRStreamProperties::SetBlockAlign "SetBlockAlign" | @ref PVRStreamProperties::GetBlockAlign "GetBlockAlign"
  /// | **Bit rate** | `int` | @ref PVRStreamProperties::SetBitRate "SetBitRate" | @ref PVRStreamProperties::GetBitRate "GetBitRate"
  /// | **Bits per sample** | `int` | @ref PVRStreamProperties::SetBitsPerSample "SetBitsPerSample" | @ref PVRStreamProperties::GetBitsPerSample "GetBitsPerSample"
  ///

  /// @addtogroup cpp_kodi_addon_pvr_Defs_Stream_PVRStreamProperties
  ///@{

  /// @brief PID.
  void SetPID(unsigned int pid) { m_cStructure->iPID = pid; }

  /// @brief To get with @ref SetPID() changed values.
  unsigned int GetPID() const { return m_cStructure->iPID; }

  /// @brief Codec type this stream.
  void SetCodecType(PVR_CODEC_TYPE codecType) { m_cStructure->iCodecType = codecType; }

  /// @brief To get with @ref SetCodecType() changed values.
  PVR_CODEC_TYPE GetCodecType() const { return m_cStructure->iCodecType; }

  /// @brief Codec id of this stream.
  void SetCodecId(unsigned int codecId) { m_cStructure->iCodecId = codecId; }

  /// @brief To get with @ref SetCodecId() changed values.
  unsigned int GetCodecId() const { return m_cStructure->iCodecId; }

  /// @brief 3 letter language id.
  void SetLanguage(const std::string& language)
  {
    if (language.size() > 3)
    {
      kodi::Log(ADDON_LOG_ERROR,
                "PVRStreamProperties::%s: Language string size '%li' higher as needed 3", __func__,
                language.size());
      return;
    }
    m_cStructure->strLanguage[0] = language[0];
    m_cStructure->strLanguage[1] = language[1];
    m_cStructure->strLanguage[2] = language[2];
    m_cStructure->strLanguage[3] = 0;
  }

  /// @brief To get with @ref SetLanguage() changed values.
  std::string GetLanguage() const { return m_cStructure->strLanguage; }

  /// @brief Subtitle Info
  void SetSubtitleInfo(int subtitleInfo) { m_cStructure->iSubtitleInfo = subtitleInfo; }

  /// @brief To get with @ref SetSubtitleInfo() changed values.
  int GetSubtitleInfo() const { return m_cStructure->iSubtitleInfo; }

  /// @brief To set scale of 1000 and a rate of 29970 will result in 29.97 fps.
  void SetFPSScale(int fpsScale) { m_cStructure->iFPSScale = fpsScale; }

  /// @brief To get with @ref SetFPSScale() changed values.
  int GetFPSScale() const { return m_cStructure->iFPSScale; }

  /// @brief FPS rate
  void SetFPSRate(int fpsRate) { m_cStructure->iFPSRate = fpsRate; }

  /// @brief To get with @ref SetFPSRate() changed values.
  int GetFPSRate() const { return m_cStructure->iFPSRate; }

  /// @brief Height of the stream reported by the demuxer
  void SetHeight(int height) { m_cStructure->iHeight = height; }

  /// @brief To get with @ref SetHeight() changed values.
  int GetHeight() const { return m_cStructure->iHeight; }

  /// @brief Width of the stream reported by the demuxer.
  void SetWidth(int width) { m_cStructure->iWidth = width; }

  /// @brief To get with @ref SetWidth() changed values.
  int GetWidth() const { return m_cStructure->iWidth; }

  /// @brief Display aspect ratio of the stream.
  void SetAspect(float aspect) { m_cStructure->fAspect = aspect; }

  /// @brief To get with @ref SetAspect() changed values.
  float GetAspect() const { return m_cStructure->fAspect; }

  /// @brief Amount of channels.
  void SetChannels(int channels) { m_cStructure->iChannels = channels; }

  /// @brief To get with @ref SetChannels() changed values.
  int GetChannels() const { return m_cStructure->iChannels; }

  /// @brief Sample rate.
  void SetSampleRate(int sampleRate) { m_cStructure->iSampleRate = sampleRate; }

  /// @brief To get with @ref SetSampleRate() changed values.
  int GetSampleRate() const { return m_cStructure->iSampleRate; }

  /// @brief Block alignment
  void SetBlockAlign(int blockAlign) { m_cStructure->iBlockAlign = blockAlign; }

  /// @brief To get with @ref SetBlockAlign() changed values.
  int GetBlockAlign() const { return m_cStructure->iBlockAlign; }

  /// @brief Bit rate.
  void SetBitRate(int bitRate) { m_cStructure->iBitRate = bitRate; }

  /// @brief To get with @ref SetBitRate() changed values.
  int GetBitRate() const { return m_cStructure->iBitRate; }

  /// @brief Bits per sample.
  void SetBitsPerSample(int bitsPerSample) { m_cStructure->iBitsPerSample = bitsPerSample; }

  /// @brief To get with @ref SetBitsPerSample() changed values.
  int GetBitsPerSample() const { return m_cStructure->iBitsPerSample; }
  ///@}

private:
  PVRStreamProperties(const PVR_STREAM_PROPERTIES::PVR_STREAM* type) : CStructHdl(type) {}
  PVRStreamProperties(PVR_STREAM_PROPERTIES::PVR_STREAM* type) : CStructHdl(type) {}
};
///@}
//------------------------------------------------------------------------------

//==============================================================================
/// @defgroup cpp_kodi_addon_pvr_Defs_Stream_PVRStreamTimes class PVRStreamTimes
/// @ingroup cpp_kodi_addon_pvr_Defs_Stream
/// @brief **Times of playing stream (Live TV and recordings)**\n
/// This class is used to transfer the necessary data when
/// @ref kodi::addon::PVRStreamProperties::GetStreamTimes is called.
///
/// ----------------------------------------------------------------------------
///
/// @copydetails cpp_kodi_addon_pvr_Defs_Stream_PVRStreamTimes_Help
///
///@{
class PVRStreamTimes : public CStructHdl<PVRStreamTimes, PVR_STREAM_TIMES>
{
  friend class CInstancePVRClient;

public:
  /*! \cond PRIVATE */
  PVRStreamTimes() { memset(m_cStructure, 0, sizeof(PVR_STREAM_TIMES)); }
  PVRStreamTimes(const PVRStreamTimes& type) : CStructHdl(type) {}
  /*! \endcond */

  /// @defgroup cpp_kodi_addon_pvr_Defs_Stream_PVRStreamTimes_Help Value Help
  /// @ingroup cpp_kodi_addon_pvr_Defs_Stream_PVRStreamTimes
  /// ----------------------------------------------------------------------------
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_addon_pvr_Defs_Stream_PVRStreamTimes :</b>
  /// | Name | Type | Set call | Get call
  /// |------|------|----------|----------
  /// | **Start time** | `time_t` | @ref PVRStreamTimes::SetStartTime "SetStartTime" | @ref PVRStreamTimes::GetStartTime "GetStartTime"
  /// | **PTS start** | `int64_t` | @ref PVRStreamTimes::SetPTSStart "SetPTSStart" | @ref PVRStreamTimes::GetPTSStart "GetPTSStart"
  /// | **PTS begin** | `int64_t` | @ref PVRStreamTimes::SetPTSBegin "SetPTSBegin" | @ref PVRStreamTimes::GetPTSBegin "GetPTSBegin"
  /// | **PTS end** | `int64_t` | @ref PVRStreamTimes::SetPTSEnd "SetPTSEnd" | @ref PVRStreamTimes::GetPTSEnd "GetPTSEnd"
  ///

  /// @addtogroup cpp_kodi_addon_pvr_Defs_Stream_PVRStreamTimes
  ///@{

  /// @brief For recordings, this must be zero. For Live TV, this is a reference
  /// time in units of time_t (UTC) from which time elapsed starts. Ideally start
  /// of tv show, but can be any other value.
  void SetStartTime(time_t startTime) { m_cStructure->startTime = startTime; }

  /// @brief To get with @ref SetStartTime() changed values.
  time_t GetStartTime() const { return m_cStructure->startTime; }

  /// @brief The pts of startTime.
  void SetPTSStart(int64_t ptsStart) { m_cStructure->ptsStart = ptsStart; }

  /// @brief To get with @ref SetPTSStart() changed values.
  int64_t GetPTSStart() const { return m_cStructure->ptsStart; }

  /// @brief Earliest pts player can seek back. Value is in micro seconds,
  /// relative to PTS start. For recordings, this must be zero. For Live TV, this
  /// must be zero if not timeshifting and must point to begin of the timeshift
  /// buffer, otherwise.
  void SetPTSBegin(int64_t ptsBegin) { m_cStructure->ptsBegin = ptsBegin; }

  /// @brief To get with @ref SetPTSBegin() changed values.
  int64_t GetPTSBegin() const { return m_cStructure->ptsBegin; }

  /// @brief Latest pts player can seek forward. Value is in micro seconds,
  /// relative to PTS start. For recordings, this must be the total length. For
  /// Live TV, this must be zero if not timeshifting and must point to end of
  /// the timeshift buffer, otherwise.
  void SetPTSEnd(int64_t ptsEnd) { m_cStructure->ptsEnd = ptsEnd; }

  /// @brief To get with @ref SetPTSEnd() changed values.
  int64_t GetPTSEnd() const { return m_cStructure->ptsEnd; }
  ///@}

private:
  PVRStreamTimes(const PVR_STREAM_TIMES* type) : CStructHdl(type) {}
  PVRStreamTimes(PVR_STREAM_TIMES* type) : CStructHdl(type) {}
};
///@}
//------------------------------------------------------------------------------

} /* namespace addon */
} /* namespace kodi */

#endif /* __cplusplus */
