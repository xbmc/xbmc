/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../AddonBase.h"
#include "../StreamCrypto.h"
#include "../c-api/addon-instance/inputstream.h"

#ifdef __cplusplus

#include <map>

namespace kodi
{
namespace addon
{

class CInstanceInputStream;

class ATTRIBUTE_HIDDEN InputstreamProperty
  : public CStructHdl<InputstreamProperty, INPUTSTREAM_PROPERTY>
{
  friend class CInstanceInputStream;

public:
  std::string GetURL() const { return m_cStructure->m_strURL; }

  std::string GetMimeType() const { return m_cStructure->m_mimeType; }

  unsigned int GetPropertiesAmount() const
  {
    return m_cStructure->m_nCountInfoValues;
  }

  const std::map<std::string, std::string> GetProperties() const
  {
    std::map<std::string, std::string> props;
    for (unsigned int i = 0; i < m_cStructure->m_nCountInfoValues; ++i)
    {
      props.emplace(m_cStructure->m_ListItemProperties[i].m_strKey,
                    m_cStructure->m_ListItemProperties[i].m_strValue);
    }
    return props;
  }

  std::string GetLibFolder() const { return m_cStructure->m_libFolder; }

  std::string GetProfileFolder() const { return m_cStructure->m_profileFolder; }

private:
  InputstreamProperty() = delete;
  InputstreamProperty(const InputstreamProperty& stream) = delete;
  InputstreamProperty(const INPUTSTREAM_PROPERTY* stream) : CStructHdl(stream) {}
  InputstreamProperty(INPUTSTREAM_PROPERTY* stream) : CStructHdl(stream) {}
};

class ATTRIBUTE_HIDDEN InputstreamCapabilities
  : public CStructHdl<InputstreamCapabilities, INPUTSTREAM_CAPABILITIES>
{
  friend class CInstanceInputStream;

public:
  /*! \cond PRIVATE */
  InputstreamCapabilities() = default;
  InputstreamCapabilities(const InputstreamCapabilities& stream) : CStructHdl(stream) {}
  /*! \endcond */

  void SetMask(uint32_t mask) const { m_cStructure->m_mask = mask; }

  uint32_t GetMask() const { return m_cStructure->m_mask; }

private:
  InputstreamCapabilities(const INPUTSTREAM_CAPABILITIES* stream) : CStructHdl(stream) {}
  InputstreamCapabilities(INPUTSTREAM_CAPABILITIES* stream) : CStructHdl(stream) {}
};

class ATTRIBUTE_HIDDEN InputstreamMasteringMetadata
  : public CStructHdl<InputstreamMasteringMetadata, INPUTSTREAM_MASTERING_METADATA>
{
  friend class CInstanceInputStream;
  friend class InputstreamInfo;

public:
  /*! \cond PRIVATE */
  InputstreamMasteringMetadata() = default;
  InputstreamMasteringMetadata(const InputstreamMasteringMetadata& stream) : CStructHdl(stream) {}
  /*! \endcond */

  bool operator==(const kodi::addon::InputstreamMasteringMetadata& right) const
  {
    if (memcmp(m_cStructure, right.m_cStructure, sizeof(INPUTSTREAM_MASTERING_METADATA)) == 0)
      return true;
    return false;
  }

  void SetPrimaryR_ChromaticityX(double value) { m_cStructure->primary_r_chromaticity_x = value; }

  double GetPrimaryR_ChromaticityX() { return m_cStructure->primary_r_chromaticity_x; }

  void SetPrimaryR_ChromaticityY(double value) { m_cStructure->primary_r_chromaticity_y = value; }

  double GetPrimaryR_ChromaticityY() { return m_cStructure->primary_r_chromaticity_y; }

  void SetPrimaryG_ChromaticityX(double value) { m_cStructure->primary_g_chromaticity_x = value; }

  double GetPrimaryG_ChromaticityX() { return m_cStructure->primary_g_chromaticity_x; }

  void SetPrimaryG_ChromaticityY(double value) { m_cStructure->primary_g_chromaticity_y = value; }

  double GetPrimaryG_ChromaticityY() { return m_cStructure->primary_g_chromaticity_y; }

  void SetPrimaryB_ChromaticityX(double value) { m_cStructure->primary_b_chromaticity_x = value; }

  double GetPrimaryB_ChromaticityX() { return m_cStructure->primary_b_chromaticity_x; }

  void SetPrimaryB_ChromaticityY(double value) { m_cStructure->primary_b_chromaticity_y = value; }

  double GetPrimaryB_ChromaticityY() { return m_cStructure->primary_b_chromaticity_y; }

  void SetWhitePoint_ChromaticityX(double value)
  {
    m_cStructure->white_point_chromaticity_x = value;
  }

  double GetWhitePoint_ChromaticityX() { return m_cStructure->white_point_chromaticity_x; }

  void SetWhitePoint_ChromaticityY(double value)
  {
    m_cStructure->white_point_chromaticity_y = value;
  }

  double GetWhitePoint_ChromaticityY() { return m_cStructure->white_point_chromaticity_y; }

  void SetLuminanceMax(double value) { m_cStructure->luminance_max = value; }

  double GetLuminanceMax() { return m_cStructure->luminance_max; }

  void SetLuminanceMin(double value) { m_cStructure->luminance_min = value; }

  double GetLuminanceMin() { return m_cStructure->luminance_min; }

private:
  InputstreamMasteringMetadata(const INPUTSTREAM_MASTERING_METADATA* stream) : CStructHdl(stream) {}
  InputstreamMasteringMetadata(INPUTSTREAM_MASTERING_METADATA* stream) : CStructHdl(stream) {}
};

class ATTRIBUTE_HIDDEN InputstreamContentlightMetadata
  : public CStructHdl<InputstreamContentlightMetadata, INPUTSTREAM_CONTENTLIGHT_METADATA>
{
  friend class CInstanceInputStream;
  friend class InputstreamInfo;

public:
  /*! \cond PRIVATE */
  InputstreamContentlightMetadata() = default;
  InputstreamContentlightMetadata(const InputstreamContentlightMetadata& stream)
    : CStructHdl(stream)
  {
  }
  /*! \endcond */

  bool operator==(const kodi::addon::InputstreamContentlightMetadata& right) const
  {
    if (memcmp(m_cStructure, right.m_cStructure, sizeof(INPUTSTREAM_CONTENTLIGHT_METADATA)) == 0)
      return true;
    return false;
  }

  void SetMaxCll(uint64_t value) { m_cStructure->max_cll = value; }

  uint64_t GetMaxCll() { return m_cStructure->max_cll; }

  void SetMaxFall(uint64_t value) { m_cStructure->max_fall = value; }

  uint64_t GetMaxFall() { return m_cStructure->max_fall; }

private:
  InputstreamContentlightMetadata(const INPUTSTREAM_CONTENTLIGHT_METADATA* stream)
    : CStructHdl(stream)
  {
  }
  InputstreamContentlightMetadata(INPUTSTREAM_CONTENTLIGHT_METADATA* stream) : CStructHdl(stream) {}
};

class ATTRIBUTE_HIDDEN InputstreamInfo : public CStructHdl<InputstreamInfo, INPUTSTREAM_INFO>
{
  friend class CInstanceInputStream;

public:
  /*! \cond PRIVATE */
  InputstreamInfo() = default;
  InputstreamInfo(const InputstreamInfo& stream) : CStructHdl(stream) { CopyExtraData(); }
  /*! \endcond */

  void SetStreamType(INPUTSTREAM_TYPE streamType) { m_cStructure->m_streamType = streamType; }

  INPUTSTREAM_TYPE GetStreamType() const { return m_cStructure->m_streamType; }

  void SetFeatures(uint32_t features) { m_cStructure->m_features = features; }

  uint32_t GetFeatures() const { return m_cStructure->m_features; }

  void SetFlags(uint32_t flags) { m_cStructure->m_flags = flags; }

  uint32_t GetFlags() const { return m_cStructure->m_flags; }

  void SetName(const std::string& name)
  {
    strncpy(m_cStructure->m_name, name.c_str(), INPUTSTREAM_MAX_STRING_NAME_SIZE);
  }

  std::string GetName() const { return m_cStructure->m_name; }

  void SetCodecName(const std::string& codecName)
  {
    strncpy(m_cStructure->m_codecName, codecName.c_str(), INPUTSTREAM_MAX_STRING_CODEC_SIZE);
  }

  std::string GetCodecName() const { return m_cStructure->m_codecName; }

  void SetCodecInternalName(const std::string& codecName)
  {
    strncpy(m_cStructure->m_codecInternalName, codecName.c_str(),
            INPUTSTREAM_MAX_STRING_CODEC_SIZE);
  }

  std::string GetCodecInternalName() const { return m_cStructure->m_codecInternalName; }

  void SetCodecProfile(STREAMCODEC_PROFILE codecProfile)
  {
    m_cStructure->m_codecProfile = codecProfile;
  }

  STREAMCODEC_PROFILE GetCodecProfile() const { return m_cStructure->m_codecProfile; }

  void SetPhysicalIndex(unsigned int id) { m_cStructure->m_pID = id; }

  unsigned int GetPhysicalIndex() const { return m_cStructure->m_pID; }

  void SetExtraData(const std::vector<uint8_t>& extraData)
  {
    m_extraData = extraData;
    m_cStructure->m_ExtraData = m_extraData.data();
    m_cStructure->m_ExtraSize = m_extraData.size();
  }

  void SetExtraData(const uint8_t* extraData, size_t extraSize)
  {
    m_extraData.clear();
    if (extraData && extraSize > 0)
    {
      for (size_t i = 0; i < extraSize; ++i)
        m_extraData.emplace_back(extraData[i]);
    }

    m_cStructure->m_ExtraData = m_extraData.data();
    m_cStructure->m_ExtraSize = m_extraData.size();
  }

  const std::vector<uint8_t>& GetExtraData() { return m_extraData; }

  size_t GetExtraDataSize() { return m_extraData.size(); }

  bool CompareExtraData(const uint8_t* extraData, size_t extraSize) const
  {
    if (m_extraData.size() != extraSize)
      return false;
    for (size_t i = 0; i < extraSize; ++i)
    {
      if (m_extraData[i] != extraData[i])
        return false;
    }
    return true;
  }

  void ClearExtraData()
  {
    m_extraData.clear();
    m_cStructure->m_ExtraData = m_extraData.data();
    m_cStructure->m_ExtraSize = m_extraData.size();
  }

  void SetLanguage(const std::string& language)
  {
    strncpy(m_cStructure->m_language, language.c_str(), INPUTSTREAM_MAX_STRING_LANGUAGE_SIZE);
  }

  std::string GetLanguage() const { return m_cStructure->m_language; }

  void SetFpsScale(unsigned int fpsScale) { m_cStructure->m_FpsScale = fpsScale; }

  unsigned int GetFpsScale() const { return m_cStructure->m_FpsScale; }

  void SetFpsRate(unsigned int fpsRate) { m_cStructure->m_FpsRate = fpsRate; }

  unsigned int GetFpsRate() const { return m_cStructure->m_FpsRate; }

  void SetHeight(unsigned int height) { m_cStructure->m_Height = height; }

  unsigned int GetHeight() const { return m_cStructure->m_Height; }

  void SetWidth(unsigned int width) { m_cStructure->m_Width = width; }

  unsigned int GetWidth() const { return m_cStructure->m_Width; }

  void SetAspect(float aspect) { m_cStructure->m_Aspect = aspect; }

  float GetAspect() const { return m_cStructure->m_Aspect; }

  void SetChannels(unsigned int channels) { m_cStructure->m_Channels = channels; }

  unsigned int GetChannels() const { return m_cStructure->m_Channels; }

  void SetSampleRate(unsigned int sampleRate) { m_cStructure->m_SampleRate = sampleRate; }

  unsigned int GetSampleRate() const { return m_cStructure->m_SampleRate; }

  void SetBitRate(unsigned int bitRate) { m_cStructure->m_BitRate = bitRate; }

  unsigned int GetBitRate() const { return m_cStructure->m_BitRate; }

  void SetBitsPerSample(unsigned int bitsPerSample)
  {
    m_cStructure->m_BitsPerSample = bitsPerSample;
  }

  unsigned int GetBitsPerSample() const { return m_cStructure->m_BitsPerSample; }

  void SetBlockAlign(unsigned int blockAlign) { m_cStructure->m_BlockAlign = blockAlign; }

  unsigned int GetBlockAlign() const { return m_cStructure->m_BlockAlign; }

  void SetCryptoInfo(const CRYPTO_INFO& cryptoInfo) { m_cStructure->m_cryptoInfo = cryptoInfo; }

  const CRYPTO_INFO& GetCryptoInfo() const { return m_cStructure->m_cryptoInfo; }

  void SetCodecFourCC(unsigned int codecFourCC) { m_cStructure->m_codecFourCC = codecFourCC; }

  unsigned int GetCodecFourCC() const { return m_cStructure->m_codecFourCC; }

  void SetColorSpace(INPUTSTREAM_COLORSPACE colorSpace) { m_cStructure->m_colorSpace = colorSpace; }

  INPUTSTREAM_COLORSPACE GetColorSpace() const { return m_cStructure->m_colorSpace; }

  void SetColorRange(INPUTSTREAM_COLORRANGE colorRange) { m_cStructure->m_colorRange = colorRange; }

  INPUTSTREAM_COLORRANGE GetColorRange() const { return m_cStructure->m_colorRange; }

  void SetColorPrimaries(INPUTSTREAM_COLORPRIMARIES colorPrimaries)
  {
    m_cStructure->m_colorPrimaries = colorPrimaries;
  }

  INPUTSTREAM_COLORPRIMARIES GetColorPrimaries() const { return m_cStructure->m_colorPrimaries; }

  void SetColorTransferCharacteristic(INPUTSTREAM_COLORTRC colorTransferCharacteristic)
  {
    m_cStructure->m_colorTransferCharacteristic = colorTransferCharacteristic;
  }

  INPUTSTREAM_COLORTRC GetColorTransferCharacteristic() const
  {
    return m_cStructure->m_colorTransferCharacteristic;
  }

  void SetMasteringMetadata(const kodi::addon::InputstreamMasteringMetadata& masteringMetadata)
  {
    m_masteringMetadata = masteringMetadata;
    m_cStructure->m_masteringMetadata = m_masteringMetadata;
  }

  const kodi::addon::InputstreamMasteringMetadata& GetMasteringMetadata() const
  {
    return m_masteringMetadata;
  }

  void ClearMasteringMetadata() { m_cStructure->m_masteringMetadata = nullptr; }

  void SetContentLightMetadata(
      const kodi::addon::InputstreamContentlightMetadata& contentLightMetadata)
  {
    m_contentLightMetadata = contentLightMetadata;
    m_cStructure->m_contentLightMetadata = m_contentLightMetadata;
  }

  const kodi::addon::InputstreamContentlightMetadata& GetContentLightMetadata() const
  {
    return m_contentLightMetadata;
  }

  void ClearContentLightMetadata() { m_cStructure->m_contentLightMetadata = nullptr; }

private:
  InputstreamInfo(const INPUTSTREAM_INFO* stream) : CStructHdl(stream) { CopyExtraData(); }
  InputstreamInfo(INPUTSTREAM_INFO* stream) : CStructHdl(stream) { CopyExtraData(); }

  void CopyExtraData()
  {
    if (m_cStructure->m_ExtraData && m_cStructure->m_ExtraSize > 0)
    {
      for (unsigned int i = 0; i < m_cStructure->m_ExtraSize; ++i)
        m_extraData.emplace_back(m_cStructure->m_ExtraData[i]);
    }
    if (m_cStructure->m_masteringMetadata)
      m_masteringMetadata = m_cStructure->m_masteringMetadata;
    if (m_cStructure->m_contentLightMetadata)
      m_contentLightMetadata = m_cStructure->m_contentLightMetadata;
  }
  std::vector<uint8_t> m_extraData;
  InputstreamMasteringMetadata m_masteringMetadata;
  InputstreamContentlightMetadata m_contentLightMetadata;
};

class ATTRIBUTE_HIDDEN InputstreamTimes : public CStructHdl<InputstreamTimes, INPUTSTREAM_TIMES>
{
  friend class CInstanceInputStream;

public:
  /*! \cond PRIVATE */
  InputstreamTimes() = default;
  InputstreamTimes(const InputstreamTimes& stream) : CStructHdl(stream) {}
  /*! \endcond */

  void SetStartTime(time_t startTime) const { m_cStructure->startTime = startTime; }

  time_t GetStartTime() const { return m_cStructure->startTime; }

  void SetPtsStart(double ptsStart) const { m_cStructure->ptsStart = ptsStart; }

  double GetPtsStart() const { return m_cStructure->ptsStart; }

  void SetPtsBegin(double ptsBegin) const { m_cStructure->ptsBegin = ptsBegin; }

  double GetPtsBegin() const { return m_cStructure->ptsBegin; }

  void SetPtsEnd(double ptsEnd) const { m_cStructure->ptsEnd = ptsEnd; }

  double GetPtsEnd() const { return m_cStructure->ptsEnd; }

private:
  InputstreamTimes(const INPUTSTREAM_TIMES* stream) : CStructHdl(stream) {}
  InputstreamTimes(INPUTSTREAM_TIMES* stream) : CStructHdl(stream) {}
};

class ATTRIBUTE_HIDDEN CInstanceInputStream : public IAddonInstance
{
public:
  explicit CInstanceInputStream(KODI_HANDLE instance, const std::string& kodiVersion = "")
    : IAddonInstance(ADDON_INSTANCE_INPUTSTREAM,
                     !kodiVersion.empty() ? kodiVersion
                                          : GetKodiTypeVersion(ADDON_INSTANCE_INPUTSTREAM))
  {
    if (CAddonBase::m_interface->globalSingleInstance != nullptr)
      throw std::logic_error("kodi::addon::CInstanceInputStream: Creation of multiple together "
                             "with single instance way is not allowed!");

    SetAddonStruct(instance, m_kodiVersion);
  }

  ~CInstanceInputStream() override = default;

  /*!
     * Open a stream.
     * @param props
     * @return True if the stream has been opened successfully, false otherwise.
     * @remarks
     */
  virtual bool Open(const kodi::addon::InputstreamProperty& props) = 0;

  /*!
     * Close an open stream.
     * @remarks
     */
  virtual void Close() = 0;

  /*!
     * Get Capabilities of this addon.
     * @param capabilities The add-on's capabilities.
     * @remarks
     */
  virtual void GetCapabilities(kodi::addon::InputstreamCapabilities& capabilities) = 0;

  /*!
     * Get IDs of available streams
     * @remarks
     */
  virtual bool GetStreamIds(std::vector<unsigned int>& ids) = 0;

  /*!
     * Get stream properties of a stream.
     * @param streamid unique id of stream
     * @return struc of stream properties
     * @remarks
     */
  virtual bool GetStream(int streamid, kodi::addon::InputstreamInfo& stream) { return false; }

  /*!
     * Enable or disable a stream.
     * A disabled stream does not send demux packets
     * @param streamid unique id of stream
     * @param enable true for enable, false for disable
     * @remarks
     */
  virtual void EnableStream(int streamid, bool enable) = 0;

  /*!
    * Opens a stream for playback.
    * @param streamid unique id of stream
    * @remarks
    */
  virtual bool OpenStream(int streamid) = 0;

  /*!
     * Reset the demultiplexer in the add-on.
     * @remarks Required if bHandlesDemuxing is set to true.
     */
  virtual void DemuxReset() {}

  /*!
     * Abort the demultiplexer thread in the add-on.
     * @remarks Required if bHandlesDemuxing is set to true.
     */
  virtual void DemuxAbort() {}

  /*!
     * Flush all data that's currently in the demultiplexer buffer in the add-on.
     * @remarks Required if bHandlesDemuxing is set to true.
     */
  virtual void DemuxFlush() {}

  /*!
     * Read the next packet from the demultiplexer, if there is one.
     * @return The next packet.
     *         If there is no next packet, then the add-on should return the
     *         packet created by calling AllocateDemuxPacket(0) on the callback.
     *         If the stream changed and Kodi's player needs to be reinitialised,
     *         then, the add-on should call AllocateDemuxPacket(0) on the
     *         callback, and set the streamid to DMX_SPECIALID_STREAMCHANGE and
     *         return the value.
     *         The add-on should return NULL if an error occurred.
     * @remarks Return NULL if this add-on won't provide this function.
     */
  virtual DemuxPacket* DemuxRead() { return nullptr; }

  /*!
     * Notify the InputStream addon/demuxer that Kodi wishes to seek the stream by time
     * Demuxer is required to set stream to an IDR frame
     * @param time The absolute time since stream start
     * @param backwards True to seek to keyframe BEFORE time, else AFTER
     * @param startpts can be updated to point to where display should start
     * @return True if the seek operation was possible
     * @remarks Optional, and only used if addon has its own demuxer.
     */
  virtual bool DemuxSeekTime(double time, bool backwards, double& startpts) { return false; }

  /*!
     * Notify the InputStream addon/demuxer that Kodi wishes to change playback speed
     * @param speed The requested playback speed
     * @remarks Optional, and only used if addon has its own demuxer.
     */
  virtual void DemuxSetSpeed(int speed) {}

  /*!
     * Sets desired width / height
     * @param width / hight
     */
  virtual void SetVideoResolution(int width, int height) {}

  /*!
     * Totel time in ms
     * @remarks
     */
  virtual int GetTotalTime() { return -1; }

  /*!
     * Playing time in ms
     * @remarks
     */
  virtual int GetTime() { return -1; }

  /*!
    * Get current timing values in PTS scale
    * @remarks
    */
  virtual bool GetTimes(kodi::addon::InputstreamTimes& times) { return false; }

  /*!
     * Positions inputstream to playing time given in ms
     * @remarks
     */
  virtual bool PosTime(int ms) { return false; }

  /*!
  * Return currently selected chapter
  * @remarks
  */
  virtual int GetChapter() { return -1; };

  /*!
  * Return number of available chapters
  * @remarks
  */
  virtual int GetChapterCount() { return 0; };

  /*!
  * Return name of chapter # ch
  * @remarks
  */
  virtual const char* GetChapterName(int ch) { return nullptr; };

  /*!
  * Return position if chapter # ch in milliseconds
  * @remarks
  */
  virtual int64_t GetChapterPos(int ch) { return 0; };

  /*!
  * Seek to the beginning of chapter # ch
  * @remarks
  */
  virtual bool SeekChapter(int ch) { return false; };

  /*!
     * Read from an open stream.
     * @param buffer The buffer to store the data in.
     * @param bufferSize The amount of bytes to read.
     * @return The amount of bytes that were actually read from the stream.
     * @remarks Return -1 if this add-on won't provide this function.
     */
  virtual int ReadStream(uint8_t* buffer, unsigned int bufferSize) { return -1; }

  /*!
     * Seek in a stream.
     * @param position The position to seek to.
     * @param whence ?
     * @return The new position.
     * @remarks Return -1 if this add-on won't provide this function.
     */
  virtual int64_t SeekStream(int64_t position, int whence = SEEK_SET) { return -1; }

  /*!
     * @return The position in the stream that's currently being read.
     * @remarks Return -1 if this add-on won't provide this function.
     */
  virtual int64_t PositionStream() { return -1; }

  /*!
     * @return The total length of the stream that's currently being read.
     * @remarks Return -1 if this add-on won't provide this function.
     */
  virtual int64_t LengthStream() { return -1; }

  /*!
     * @return Obtain the chunk size to use when reading streams.
     * @remarks Return 0 if this add-on won't provide this function.
     */
  virtual int GetBlockSize() { return 0; }

  /*!
     *  Check for real-time streaming
     *  @return true if current stream is real-time
     */
  virtual bool IsRealTimeStream() { return true; }

  /*!
     * @brief Allocate a demux packet. Free with FreeDemuxPacket
     * @param dataSize The size of the data that will go into the packet
     * @return The allocated packet
     */
  DemuxPacket* AllocateDemuxPacket(int dataSize)
  {
    return m_instanceData->toKodi->allocate_demux_packet(m_instanceData->toKodi->kodiInstance,
                                                         dataSize);
  }

  /*!
     * @brief Allocate a demux packet. Free with FreeDemuxPacket
     * @param dataSize The size of the data that will go into the packet
     * @return The allocated packet
     */
  DemuxPacket* AllocateEncryptedDemuxPacket(int dataSize, unsigned int encryptedSubsampleCount)
  {
    return m_instanceData->toKodi->allocate_encrypted_demux_packet(
        m_instanceData->toKodi->kodiInstance, dataSize, encryptedSubsampleCount);
  }

  /*!
     * @brief Free a packet that was allocated with AllocateDemuxPacket
     * @param packet The packet to free
     */
  void FreeDemuxPacket(DemuxPacket* packet)
  {
    return m_instanceData->toKodi->free_demux_packet(m_instanceData->toKodi->kodiInstance, packet);
  }

private:
  static int compareVersion(const int v1[3], const int v2[3])
  {
    for (unsigned i(0); i < 3; ++i)
      if (v1[i] != v2[i])
        return v1[i] - v2[i];
    return 0;
  }

  void SetAddonStruct(KODI_HANDLE instance, const std::string& kodiVersion)
  {
    if (instance == nullptr)
      throw std::logic_error("kodi::addon::CInstanceInputStream: Creation with empty addon "
                             "structure not allowed, table must be given from Kodi!");
    int api[3] = { 0, 0, 0 };
    sscanf(kodiVersion.c_str(), "%d.%d.%d", &api[0], &api[1], &api[2]);

    m_instanceData = static_cast<AddonInstance_InputStream*>(instance);
    m_instanceData->toAddon->addonInstance = this;
    m_instanceData->toAddon->open = ADDON_Open;
    m_instanceData->toAddon->close = ADDON_Close;
    m_instanceData->toAddon->get_capabilities = ADDON_GetCapabilities;

    m_instanceData->toAddon->get_stream_ids = ADDON_GetStreamIds;
    m_instanceData->toAddon->get_stream = ADDON_GetStream;
    m_instanceData->toAddon->enable_stream = ADDON_EnableStream;
    m_instanceData->toAddon->open_stream = ADDON_OpenStream;
    m_instanceData->toAddon->demux_reset = ADDON_DemuxReset;
    m_instanceData->toAddon->demux_abort = ADDON_DemuxAbort;
    m_instanceData->toAddon->demux_flush = ADDON_DemuxFlush;
    m_instanceData->toAddon->demux_read = ADDON_DemuxRead;
    m_instanceData->toAddon->demux_seek_time = ADDON_DemuxSeekTime;
    m_instanceData->toAddon->demux_set_speed = ADDON_DemuxSetSpeed;
    m_instanceData->toAddon->set_video_resolution = ADDON_SetVideoResolution;

    m_instanceData->toAddon->get_total_time = ADDON_GetTotalTime;
    m_instanceData->toAddon->get_time = ADDON_GetTime;

    m_instanceData->toAddon->get_times = ADDON_GetTimes;
    m_instanceData->toAddon->pos_time = ADDON_PosTime;

    m_instanceData->toAddon->read_stream = ADDON_ReadStream;
    m_instanceData->toAddon->seek_stream = ADDON_SeekStream;
    m_instanceData->toAddon->position_stream = ADDON_PositionStream;
    m_instanceData->toAddon->length_stream = ADDON_LengthStream;
    m_instanceData->toAddon->is_real_time_stream = ADDON_IsRealTimeStream;

    // Added on 2.0.10
    m_instanceData->toAddon->get_chapter = ADDON_GetChapter;
    m_instanceData->toAddon->get_chapter_count = ADDON_GetChapterCount;
    m_instanceData->toAddon->get_chapter_name = ADDON_GetChapterName;
    m_instanceData->toAddon->get_chapter_pos = ADDON_GetChapterPos;
    m_instanceData->toAddon->seek_chapter = ADDON_SeekChapter;

    // Added on 2.0.12
    m_instanceData->toAddon->block_size_stream = ADDON_GetBlockSize;

    /*
    // Way to include part on new API version
    int minPartVersion[3] = { 3, 0, 0 };
    if (compareVersion(api, minPartVersion) >= 0)
    {

    }
    */
  }

  inline static bool ADDON_Open(const AddonInstance_InputStream* instance,
                                INPUTSTREAM_PROPERTY* props)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->Open(props);
  }

  inline static void ADDON_Close(const AddonInstance_InputStream* instance)
  {
    static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->Close();
  }

  inline static void ADDON_GetCapabilities(const AddonInstance_InputStream* instance,
                                           INPUTSTREAM_CAPABILITIES* capabilities)
  {
    InputstreamCapabilities caps(capabilities);
    static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->GetCapabilities(caps);
  }


  // IDemux
  inline static bool ADDON_GetStreamIds(const AddonInstance_InputStream* instance,
                                        struct INPUTSTREAM_IDS* ids)
  {
    std::vector<unsigned int> idList;
    bool ret =
        static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->GetStreamIds(idList);
    if (ret)
    {
      for (size_t i = 0; i < idList.size() && i < INPUTSTREAM_MAX_STREAM_COUNT; ++i)
      {
        ids->m_streamCount++;
        ids->m_streamIds[i] = idList[i];
      }
    }
    return ret;
  }

  inline static bool ADDON_GetStream(
      const AddonInstance_InputStream* instance,
      int streamid,
      struct INPUTSTREAM_INFO* info,
      KODI_HANDLE* demuxStream,
      KODI_HANDLE (*transfer_stream)(KODI_HANDLE handle,
                                     int streamId,
                                     struct INPUTSTREAM_INFO* stream))
  {
    InputstreamInfo infoData(info);
    bool ret = static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)
                   ->GetStream(streamid, infoData);
    if (ret && transfer_stream)
    {
      // Do this with given callback to prevent memory problems and leaks. This
      // then create on Kodi the needed class where then given back on demuxStream.
      *demuxStream = transfer_stream(instance->toKodi->kodiInstance, streamid, info);
    }
    return ret;
  }

  inline static void ADDON_EnableStream(const AddonInstance_InputStream* instance,
                                        int streamid,
                                        bool enable)
  {
    static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)
        ->EnableStream(streamid, enable);
  }

  inline static bool ADDON_OpenStream(const AddonInstance_InputStream* instance, int streamid)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)
        ->OpenStream(streamid);
  }

  inline static void ADDON_DemuxReset(const AddonInstance_InputStream* instance)
  {
    static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->DemuxReset();
  }

  inline static void ADDON_DemuxAbort(const AddonInstance_InputStream* instance)
  {
    static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->DemuxAbort();
  }

  inline static void ADDON_DemuxFlush(const AddonInstance_InputStream* instance)
  {
    static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->DemuxFlush();
  }

  inline static DemuxPacket* ADDON_DemuxRead(const AddonInstance_InputStream* instance)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->DemuxRead();
  }

  inline static bool ADDON_DemuxSeekTime(const AddonInstance_InputStream* instance,
                                         double time,
                                         bool backwards,
                                         double* startpts)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)
        ->DemuxSeekTime(time, backwards, *startpts);
  }

  inline static void ADDON_DemuxSetSpeed(const AddonInstance_InputStream* instance, int speed)
  {
    static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->DemuxSetSpeed(speed);
  }

  inline static void ADDON_SetVideoResolution(const AddonInstance_InputStream* instance,
                                              int width,
                                              int height)
  {
    static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)
        ->SetVideoResolution(width, height);
  }


  // IDisplayTime
  inline static int ADDON_GetTotalTime(const AddonInstance_InputStream* instance)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->GetTotalTime();
  }

  inline static int ADDON_GetTime(const AddonInstance_InputStream* instance)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->GetTime();
  }

  // ITime
  inline static bool ADDON_GetTimes(const AddonInstance_InputStream* instance,
                                    INPUTSTREAM_TIMES* times)
  {
    InputstreamTimes cppTimes(times);
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->GetTimes(cppTimes);
  }

  // IPosTime
  inline static bool ADDON_PosTime(const AddonInstance_InputStream* instance, int ms)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->PosTime(ms);
  }

  inline static int ADDON_GetChapter(const AddonInstance_InputStream* instance)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->GetChapter();
  }

  inline static int ADDON_GetChapterCount(const AddonInstance_InputStream* instance)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->GetChapterCount();
  }

  inline static const char* ADDON_GetChapterName(const AddonInstance_InputStream* instance, int ch)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->GetChapterName(ch);
  }

  inline static int64_t ADDON_GetChapterPos(const AddonInstance_InputStream* instance, int ch)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->GetChapterPos(ch);
  }

  inline static bool ADDON_SeekChapter(const AddonInstance_InputStream* instance, int ch)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->SeekChapter(ch);
  }

  inline static int ADDON_ReadStream(const AddonInstance_InputStream* instance,
                                     uint8_t* buffer,
                                     unsigned int bufferSize)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)
        ->ReadStream(buffer, bufferSize);
  }

  inline static int64_t ADDON_SeekStream(const AddonInstance_InputStream* instance,
                                         int64_t position,
                                         int whence)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)
        ->SeekStream(position, whence);
  }

  inline static int64_t ADDON_PositionStream(const AddonInstance_InputStream* instance)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->PositionStream();
  }

  inline static int64_t ADDON_LengthStream(const AddonInstance_InputStream* instance)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->LengthStream();
  }

  inline static int ADDON_GetBlockSize(const AddonInstance_InputStream* instance)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->GetBlockSize();
  }

  inline static bool ADDON_IsRealTimeStream(const AddonInstance_InputStream* instance)
  {
    return static_cast<CInstanceInputStream*>(instance->toAddon->addonInstance)->IsRealTimeStream();
  }

  AddonInstance_InputStream* m_instanceData;
};

} /* namespace addon */
} /* namespace kodi */

#endif /* __cplusplus */
