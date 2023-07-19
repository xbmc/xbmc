/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/AudioEngine/Sinks/osx/CoreAudioDevice.h"
#include "cores/AudioEngine/Utils/AEAudioFormat.h"
#include "cores/AudioEngine/Utils/AEDeviceInfo.h"

#include <string>
#include <utility>
#include <vector>

struct CADeviceInstance
{
  AudioDeviceID audioDeviceId;
  unsigned int streamIndex;
  unsigned int sourceId;
};
typedef std::vector< std::pair<struct CADeviceInstance, CAEDeviceInfo> > CADeviceList;

//Hierarchy:
// Device
//       - 1..n streams
//            1..n formats
//       - 0..n sources
//on non planar devices we have numstreams * numsources devices for our list
//on planar devices we have 1 * numsources devices for our list
class AEDeviceEnumerationOSX
{
public:
  /*!
  * @brief C'tor - initialises the Enumerator and calls Enumerate
  * @param  deviceID - the CoreAudio Device ID which will be the base of the enumerated device list
  */
  AEDeviceEnumerationOSX(AudioDeviceID deviceID);
  // d'tor
  ~AEDeviceEnumerationOSX() = default;

  /*!
  * @brief Gets the device list which was enumerated by the last call to Enumerate
  *        (which is also called in c'tor).
  *
  * @return Returns the device list.
  */
  CADeviceList  GetDeviceInfoList() const;

  /*!
  * @brief Fetches all metadata from the CoreAudio device which is needed to generate a proper DeviceList for AE
  *        This method is always called from C'tor but can be called multiple times if the streams of a device
  *        changed. This fills m_caStreamInfos.
  *        After this call - GetDeviceInfoList will reflect the Enumerated metadata.
  * @return false when streamlist couldn't be fetched from device - else true
  */
  bool          Enumerate();

  /*!
  * @brief Returns the number of Planes for a device. This will be 1 for non-planar devices and > 1 for planar devices.
  * @return Number of planes for this device.
  */
  unsigned int  GetNumPlanes() const;

  /*!
  * @brief Checks if the m_deviceID belongs to a planar or non-planar device.
  * @return true if m_deviceID belongs to a planar device - else false.
  */
  bool          IsPlanar() const { return m_isPlanar; }

  /*!
  * @brief Tries to find a suitable CoreAudio format which matches the given AEAudioFormat as close as possible.
  * @param streamIdx [in/out] - if streamIdx != INT_MAX only formats of the given streamIdx are checked
  *                             if streamIdx == INT_MAX - formats of all streams in the device are considered
  *                             On success this parameter returns the selected streamIdx.
  *
  * @param format    [in]     - the requested AE format which should be matched to the stream formats of CA
  * @param outputFormat [out] - the found CA format which matches best to the requested AE format
  * @param outputStream [out] - the coreaudio streamid which contains the coreaudio format returned in outputFormat
  * @return true if a matching corea audio format was found - else false
  */
  bool          FindSuitableFormatForStream(UInt32 &streamIdx, const AEAudioFormat &format, bool virt,
                                            AudioStreamBasicDescription &outputFormat,
                                            AudioStreamID &outputStream) const;

  /*!
  * @brief Returns the device name which belongs to m_deviceID without any stream/source suffixes
  * @return the CA device name
  */
  std::string   GetMasterDeviceName() const { return m_deviceName; }

  /*!
  * @brief Tries to return a proper channelmap from CA in a format AE understands
  * @param channelMap [in/out] - returns the found channelmap in AE format
  *                              if initialised with a map the number of channels is used to determine
  *                              if stereo or multichannel map should be fetched
  * @param channelsPerFrame [int] - the number of channels which should be mapped
  *                                 (also decides if stereo or multichannel map is fetched similar to channelMap param)
  */
  void          GetAEChannelMap(CAEChannelInfo &channelMap, unsigned int channelsPerFrame) const;

  /*!
   * @brief Scores a format based on:
   *   1. Matching passthrough characteristics (i.e. passthrough flag)
   *   2. Matching sample rate.
   *   3. Matching bits per channel (or higher).
   *   4. Matching number of channels (or higher).
   *
   * @param formatDesc   [in] - The CA FormatDescription which should be scored
   * @param format [in] - the AE format which should be matched as good as possible
   * @return - the score of formatDesc - higher scores indicate better matching to "format"
   *          (scores > 10000 indicate passthrough formats)
   */
  // public because its used in unit tests ...
  float             ScoreFormat(const AudioStreamBasicDescription &formatDesc, const AEAudioFormat &format) const;

private:

  /*!
  * @brief Checks if this is a digital device based on CA transportType or name
  * @return - true if this is a digital device - else false.
  */
  bool              isDigitalDevice() const;

  /*!
  * @brief Checks if there are passthrough formats or digital formats
  *       (the latter are passthrough formats with dedicated format config like AC3/DTS)
  * @param formatList [in] - the format list to be evaluated
  * @param hasPassthroughFormats [out] - true if there were passthrough formats in the list
  * @param hasDigitalFormat [out] - true if there were dedicated passthrough formats in the list
  */
  void              hasPassthroughOrDigitalFormats(const StreamFormatList &formatList,
                                                   bool &hasPassthroughFormats,
                                                   bool &hasDigitalFormat) const;

  /*!
  * @brief Gets the AE devicetype for this device based the given criteria
  * @param hasPassthroughFormats [in] - flag indicating that the device has passthrough formats
  * @param isDigital [in] - flag indicating that the device is digital
  * @param numChannels [in] - the number of channels of the device
  * @param transportType [in] - the transportType of the device
  * @return the AE devicetype
  */
  enum AEDeviceType getDeviceType(bool hasPassthroughFormats, bool isDigital,
                                  UInt32 numChannels, UInt32 transportType) const;

  /*!
  * @brief Fetches all ca streams from the ca device and fills m_channelsPerStream and m_streams
  */
  void              fillStreamList();

  /*!
  * @brief Scores a samplerate based on:
  * 1. Prefer exact match
  * 2. Prefer exact multiple of source samplerate and prefer the lowest
  *
  * @param destinationRate [in] - the destination samplerate to score
  * @param sourceRate [in] - the sourceRate of the audio format - this is the samplerate the score is based on
  * @return the score
  */
  float             scoreSampleRate(Float64 destinationRate, unsigned int sourceRate) const;

  bool              hasSampleRate(const AESampleRateList &list, const unsigned int samplerate) const;
  bool              hasDataFormat(const AEDataFormatList &list, const enum AEDataFormat format) const;
  bool              hasDataType(const AEDataTypeList &list, CAEStreamInfo::DataType type) const;

  /*!
  * @brief Converts a CA format description to a list of AEFormat descriptions (as one format can result
  *        in more then 1 AE format - e.x. AC3 ca format results in AC3 and DTS AE format
  *
  * @param formatDesc [in] - The CA format description to be converted
  * @param isDigital  [in] - Flag indicating if the parent stream of formatDesc is digital
  *                        (for allowing bitstreaming without dedicated AC3 format in CA)
  * @return The list of converted AE formats.
  */
  AEDataFormatList  caFormatToAE(const AudioStreamBasicDescription &formatDesc, bool isDigital) const;
  AEDataTypeList caFormatToAEType(const AudioStreamBasicDescription &formatDesc, bool isDigital) const;


  /*!
  * @brief Convert a CA channel label to an AE channel.
  * @param CAChannelLabel - the CA channel label to be converted
  * @return the corresponding AEChannel
  */
  enum AEChannel    caChannelToAEChannel(const AudioChannelLabel &CAChannelLabel) const;

  // for filling out the AEDeviceInfo object based on
  // the data gathered on the last call to Enumerate()
  /*!
  * @brief Returns all AE formats for the CA stream at the given index
  * @param streamIdx [in] - index into m_caStreamInfos
  * @return - the list of AE formats in that stream.
  */
  AEDataFormatList  getFormatListForStream(UInt32 streamIdx) const;

  AEDataTypeList  getTypeListForStream(UInt32 streamIdx) const;

  /*!
  * @brief Returns the AE channelinfo/channel map for the CA stream at the given index
  * @param streamIdx [in] - index into m_caStreamInfos
  * @return - the of AE channel info for that stream.
  */
  CAEChannelInfo    getChannelInfoForStream(UInt32 streamIdx) const;

  /*!
  * @brief Returns the AE samplerates for the CA stream at the given index
  * @param streamIdx [in] - index into m_caStreamInfos
  * @return - the list of AE samplerates for that stream.
  */
  AESampleRateList  getSampleRateListForStream(UInt32 streamIdx) const;

  /*!
  * @brief Returns the AE device name for the CA stream at the given index
  * @param streamIdx [in] - index into m_caStreamInfos
  * @return - The devicename for that stream
  */
  std::string       getDeviceNameForStream(UInt32 streamIdx) const;

  /*!
  * @brief - Returns the extra displayname shown to the User in addition to getDisplayNameForStream
  *          for the CA stream at the given index
  * @param streamIdx [in] - index into m_caStreamInfos
  * @return - The extra displayname for that stream (might be empty)
  */
  std::string       getExtraDisplayNameForStream(UInt32 streamIdx) const;

  AudioDeviceID     m_deviceID;
  bool m_isPlanar = false;
  std::string       m_deviceName;

  typedef struct
  {
    AudioStreamID streamID;
    StreamFormatList formatList;
    StreamFormatList formatListVirt;
    UInt32 numChannels;
    bool isDigital;
    bool hasPassthroughFormats;
    enum AEDeviceType deviceType;
  } caStreamInfo;
  std::vector<caStreamInfo>       m_caStreamInfos;
  CCoreAudioDevice                m_caDevice;
};
