/*
 *      Copyright (C) 2014 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "AEDeviceEnumerationOSX.h"
#include "cores/AudioEngine/Sinks/osx/CoreAudioChannelLayout.h"
#include "cores/AudioEngine/Sinks/osx/CoreAudioHelpers.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

#include <sstream>

#define CA_MAX_CHANNELS 72
// default channel map - in case it can't be fetched from the device
static enum AEChannel CAChannelMap[CA_MAX_CHANNELS + 1] = {
  AE_CH_FL , AE_CH_FR , AE_CH_BL , AE_CH_BR , AE_CH_FC , AE_CH_LFE , AE_CH_SL , AE_CH_SR ,
  AE_CH_UNKNOWN1  , AE_CH_UNKNOWN2  , AE_CH_UNKNOWN3  , AE_CH_UNKNOWN4  , 
  AE_CH_UNKNOWN5  , AE_CH_UNKNOWN6  , AE_CH_UNKNOWN7  , AE_CH_UNKNOWN8  ,
  AE_CH_UNKNOWN9  , AE_CH_UNKNOWN10 , AE_CH_UNKNOWN11 , AE_CH_UNKNOWN12 ,
  AE_CH_UNKNOWN13 , AE_CH_UNKNOWN14 , AE_CH_UNKNOWN15 , AE_CH_UNKNOWN16 ,
  AE_CH_UNKNOWN17 , AE_CH_UNKNOWN18 , AE_CH_UNKNOWN19 , AE_CH_UNKNOWN20 ,
  AE_CH_UNKNOWN21 , AE_CH_UNKNOWN22 , AE_CH_UNKNOWN23 , AE_CH_UNKNOWN24 ,
  AE_CH_UNKNOWN25 , AE_CH_UNKNOWN26 , AE_CH_UNKNOWN27 , AE_CH_UNKNOWN28 ,
  AE_CH_UNKNOWN29 , AE_CH_UNKNOWN30 , AE_CH_UNKNOWN31 , AE_CH_UNKNOWN32 ,
  AE_CH_UNKNOWN33 , AE_CH_UNKNOWN34 , AE_CH_UNKNOWN35 , AE_CH_UNKNOWN36 ,
  AE_CH_UNKNOWN37 , AE_CH_UNKNOWN38 , AE_CH_UNKNOWN39 , AE_CH_UNKNOWN40 ,
  AE_CH_UNKNOWN41 , AE_CH_UNKNOWN42 , AE_CH_UNKNOWN43 , AE_CH_UNKNOWN44 ,
  AE_CH_UNKNOWN45 , AE_CH_UNKNOWN46 , AE_CH_UNKNOWN47 , AE_CH_UNKNOWN48 ,
  AE_CH_UNKNOWN49 , AE_CH_UNKNOWN50 , AE_CH_UNKNOWN51 , AE_CH_UNKNOWN52 ,
  AE_CH_UNKNOWN53 , AE_CH_UNKNOWN54 , AE_CH_UNKNOWN55 , AE_CH_UNKNOWN56 ,
  AE_CH_UNKNOWN57 , AE_CH_UNKNOWN58 , AE_CH_UNKNOWN59 , AE_CH_UNKNOWN60 ,
  AE_CH_UNKNOWN61 , AE_CH_UNKNOWN62 , AE_CH_UNKNOWN63 , AE_CH_UNKNOWN64 ,

  AE_CH_NULL
};

AEDeviceEnumerationOSX::AEDeviceEnumerationOSX(AudioDeviceID deviceID)
: m_deviceID(deviceID)
, m_isPlanar(false)
, m_caDevice(deviceID)
{
  Enumerate();
}

bool AEDeviceEnumerationOSX::Enumerate()
{
  AudioStreamIdList streamList;
  bool isDigital = isDigitalDevice();
  bool ret = false;
  UInt32 transportType = m_caDevice.GetTransportType();
  m_caStreamInfos.clear();
  m_isPlanar = true;
  m_deviceName = m_caDevice.GetName();

  if (m_caDevice.GetStreams(&streamList))
  {
    for (UInt32 streamIdx = 0; streamIdx < streamList.size(); streamIdx++)
    {
      caStreamInfo info;
      info.streamID = streamList[streamIdx];
      info.numChannels = m_caDevice.GetNumChannelsOfStream(streamIdx);
      // one stream with num channels other then 1 is enough to make this device non-planar
      if (info.numChannels != 1)
        m_isPlanar = false;

      CCoreAudioStream::GetAvailablePhysicalFormats(streamList[streamIdx], &info.formatList);
      
      hasPassthroughOrDigitalFormats(info.formatList, info.hasPassthroughFormats, info.isDigital);

      info.isDigital |= isDigital;
      info.deviceType = getDeviceType(info.hasPassthroughFormats, info.isDigital, info.numChannels, transportType);
      m_caStreamInfos.push_back(info);
    }
    ret = true;
  }

  return ret;
}

// device/stream is digital if the transportType is digital
// or the devicename suggests that it is digital
bool AEDeviceEnumerationOSX::isDigitalDevice() const
{
  bool isDigital = m_caDevice.IsDigital();

  // if it is no digital stream per definition
  // check if the device name suggests that it is digital
  // (some hackintonshs are not so smart in announcing correct
  // ca devices ...
  if (!isDigital)
  {
    std::string devNameLower = m_caDevice.GetName();
    StringUtils::ToLower(devNameLower);                       
    isDigital = devNameLower.find("digital") != std::string::npos;
  }
  return isDigital;
}

void AEDeviceEnumerationOSX::hasPassthroughOrDigitalFormats(const StreamFormatList &formatList, bool &hasPassthroughFormats, bool &hasDigitalFormat) const
{
  hasDigitalFormat = false;
  hasPassthroughFormats = false;

  for(UInt32 formatIdx = 0; formatIdx < formatList.size(); formatIdx++)
  {
    const AudioStreamBasicDescription &desc = formatList[formatIdx].mFormat;
    if (desc.mFormatID == kAudioFormatAC3 || desc.mFormatID == kAudioFormat60958AC3)
    {
      hasDigitalFormat = true;
      hasPassthroughFormats = true;
      break;
    }
    else
    {
      // PassthroughFormat 2ch 16bit LE 48000Hz / 192000Hz */
      if (desc.mBitsPerChannel == 16 && 
          !(desc.mFormatFlags & kAudioFormatFlagIsBigEndian) &&
          desc.mChannelsPerFrame == 2 && 
          (desc.mSampleRate == 48000 || desc.mSampleRate == 192000))
      {
        hasPassthroughFormats = true;
      }
    }
  }
}

enum AEDeviceType AEDeviceEnumerationOSX::getDeviceType(bool hasPassthroughFormats, 
                                                        bool isDigital, 
                                                        UInt32 numChannels,  
                                                        UInt32 transportType) const
{
  // flag indicating that the device name "sounds" like HDMI
  bool hasHdmiName = m_deviceName.find("HDMI") != std::string::npos;
  // flag indicating that the device name "sounds" like DisplayPort
  bool hasDisplayPortName = m_deviceName.find("DisplayPort") != std::string::npos;
  enum AEDeviceType deviceType = AE_DEVTYPE_PCM;//default    
  
  // decide the type of the device based on the discovered information
  // in the streams
  // device defaults to PCM (see start of the while loop)
  // it can be HDMI, DisplayPort or Optical
  // for all of those types it needs to support
  // passthroughformats and needs to be a digital port
  if (hasPassthroughFormats && isDigital)
  {
    // if the max number of channels was more then 2
    // this can be HDMI or DisplayPort or Thunderbolt
    if (numChannels > 2)
    {
      // either the devicename suggests its HDMI
      // or CA reported the transportType as HDMI
      if (hasHdmiName || transportType == kIOAudioDeviceTransportTypeHdmi)
        deviceType = AE_DEVTYPE_HDMI;
      
      // either the devicename suggests its DisplayPort
      // or CA reported the transportType as DisplayPort or Thunderbolt
      if (hasDisplayPortName || 
          transportType == kIOAudioDeviceTransportTypeDisplayPort || 
          transportType == kIOAudioDeviceTransportTypeThunderbolt)
        deviceType = AE_DEVTYPE_DP;
    }
    else// treat all other digital passthrough devices as optical
      deviceType = AE_DEVTYPE_IEC958;

    //treat all other digital devices as HDMI to let options open to the user
    if (deviceType == AE_DEVTYPE_PCM)
      deviceType = AE_DEVTYPE_HDMI;
  }

  // devicename based overwrites from former code - maybe FIXME at some point when we
  // are sure that the upper detection does its job in all[tm] use cases
  if (hasHdmiName)
    deviceType = AE_DEVTYPE_HDMI;
  if (hasDisplayPortName)
    deviceType = AE_DEVTYPE_DP;

  return deviceType;
}

CADeviceList AEDeviceEnumerationOSX::GetDeviceInfoList() const
{
  CADeviceList list;
  UInt32 numDevices = m_caStreamInfos.size();
  if (m_isPlanar)
    numDevices = 1;
  
  for (UInt32 streamIdx = 0; streamIdx < numDevices; streamIdx++)
  {
    CAEDeviceInfo deviceInfo;
    struct CADeviceInstance devInstance;
    devInstance.audioDeviceId = m_deviceID;
    devInstance.streamIndex = streamIdx;
    devInstance.sourceId = INT_MAX;//don't set audio source by default
    
    deviceInfo.m_deviceName = getDeviceNameForStream(streamIdx);
    deviceInfo.m_displayName = m_deviceName;
    deviceInfo.m_displayNameExtra = getExtraDisplayNameForStream(streamIdx);
    deviceInfo.m_channels = getChannelInfoForStream(streamIdx);
    deviceInfo.m_sampleRates = getSampleRateListForStream(streamIdx);
    deviceInfo.m_dataFormats = getFormatListForStream(streamIdx);
    deviceInfo.m_deviceType = m_caStreamInfos[streamIdx].deviceType;
    
    CoreAudioDataSourceList sourceList;
    // if this enumerator contains multiple devices with more then 1 source we add :source suffixes to the
    // device names and overwrite the extraname with the source name
    if (numDevices == 1 && m_caDevice.GetDataSources(&sourceList) && sourceList.size() > 1)
    {
      for (unsigned sourceIdx = 0; sourceIdx < sourceList.size(); sourceIdx++)
      {
        std::stringstream sourceIdxStr;
        sourceIdxStr << sourceIdx;
        deviceInfo.m_deviceName = getDeviceNameForStream(streamIdx) + ":source" + sourceIdxStr.str();
        deviceInfo.m_displayNameExtra = m_caDevice.GetDataSourceName(sourceList[sourceIdx]);
        devInstance.sourceId = sourceList[sourceIdx];
        list.push_back(std::make_pair(devInstance, deviceInfo));
      }
    }
    else
      list.push_back(std::make_pair(devInstance, deviceInfo));
  }
  return list;
}

unsigned int AEDeviceEnumerationOSX::GetNumPlanes() const
{
  if (m_isPlanar)
    return m_caStreamInfos.size();
  else
    return 1;//interlaved - one plane
}

bool AEDeviceEnumerationOSX::hasSampleRate(const AESampleRateList &list, const unsigned int samplerate) const
{
  for (size_t i = 0; i < list.size(); ++i)
  {
    if (list[i] == samplerate)
      return true;
  }
  return false;
}

bool AEDeviceEnumerationOSX::hasDataFormat(const AEDataFormatList &list, const enum AEDataFormat format) const
{
  for (size_t i = 0; i < list.size(); ++i)
  {
    if (list[i] == format)
      return true;
  }
  return false;
}

AEDataFormatList AEDeviceEnumerationOSX::getFormatListForStream(UInt32 streamIdx) const
{
  AEDataFormatList returnDataFormatList;
  if (streamIdx >= m_caStreamInfos.size())
    return returnDataFormatList;
  
  // check the streams
  const StreamFormatList &formatList = m_caStreamInfos[streamIdx].formatList;
  for(UInt32 formatIdx = 0; formatIdx < formatList.size(); formatIdx++)
  {
    AudioStreamBasicDescription formatDesc = formatList[formatIdx].mFormat;
    AEDataFormatList aeFormatList = caFormatToAE(formatDesc, m_caStreamInfos[streamIdx].isDigital);
    for (UInt32 formatListIdx = 0; formatListIdx < aeFormatList.size(); formatListIdx++)
    {
      if (!hasDataFormat(returnDataFormatList, aeFormatList[formatListIdx]))
        returnDataFormatList.push_back(aeFormatList[formatListIdx]);
    }
  }
  return returnDataFormatList;
}

CAEChannelInfo AEDeviceEnumerationOSX::getChannelInfoForStream(UInt32 streamIdx) const
{
  CAEChannelInfo channelInfo;
  if (streamIdx >= m_caStreamInfos.size())
    return channelInfo;
  
  if (m_isPlanar)
  {
    //get channel map to match the devices channel layout as set in audio-midi-setup
    GetAEChannelMap(channelInfo, GetNumPlanes());
  }
  else
  {
    //get channel map to match the devices channel layout as set in audio-midi-setup
    GetAEChannelMap(channelInfo, m_caDevice.GetNumChannelsOfStream(streamIdx));
  }
  return channelInfo;
}

AEDataFormatList AEDeviceEnumerationOSX::caFormatToAE(const AudioStreamBasicDescription &formatDesc, bool isDigital) const
{
  AEDataFormatList formatList;
  // add stream format info
  switch (formatDesc.mFormatID)
  {
    case kAudioFormatAC3:
    case kAudioFormat60958AC3:
      formatList.push_back(AE_FMT_AC3);
      formatList.push_back(AE_FMT_DTS);
      break;
    default:
      switch(formatDesc.mBitsPerChannel)
    {
      case 16:
        if (formatDesc.mFormatFlags & kAudioFormatFlagIsBigEndian)
          formatList.push_back(AE_FMT_S16BE);
        else
        {
          /* Passthrough is possible with a 2ch digital output */
          if (formatDesc.mChannelsPerFrame == 2 && isDigital)
          {
            if (formatDesc.mSampleRate == 48000)
            {
              formatList.push_back(AE_FMT_AC3);
              formatList.push_back(AE_FMT_DTS);
            }
            else if (formatDesc.mSampleRate == 192000)
            {
              formatList.push_back(AE_FMT_EAC3);
            }
          }
          formatList.push_back(AE_FMT_S16LE);
        }
        break;
      case 24:
        if (formatDesc.mFormatFlags & kAudioFormatFlagIsBigEndian)
          formatList.push_back(AE_FMT_S24BE3);
        else
          formatList.push_back(AE_FMT_S24LE3);
        break;
      case 32:
        if (formatDesc.mFormatFlags & kAudioFormatFlagIsFloat)
          formatList.push_back(AE_FMT_FLOAT);
        else
        {
          if (formatDesc.mFormatFlags & kAudioFormatFlagIsBigEndian)
            formatList.push_back(AE_FMT_S32BE);
          else
            formatList.push_back(AE_FMT_S32LE);
        }
        break;
    }
      break;
  }
  return formatList;
}

AESampleRateList AEDeviceEnumerationOSX::getSampleRateListForStream(UInt32 streamIdx) const
{
  AESampleRateList returnSampleRateList;
  if (streamIdx >= m_caStreamInfos.size())
    return returnSampleRateList;
  
  // check the streams
  const StreamFormatList &formatList = m_caStreamInfos[streamIdx].formatList;
  for(UInt32 formatIdx = 0; formatIdx < formatList.size(); formatIdx++)
  {
    AudioStreamBasicDescription formatDesc = formatList[formatIdx].mFormat;
    // add sample rate info
    // for devices which return kAudioStreamAnyRatee
    // we add 44.1khz and 48khz - user can use
    // the "fixed" audio config to force one of them
    if (formatDesc.mSampleRate == kAudioStreamAnyRate)
    {
      CLog::Log(LOGINFO, "%s reported samplerate is kAudioStreamAnyRate adding 44.1khz and 48khz", __FUNCTION__);
      formatDesc.mSampleRate = 44100;
      if (!hasSampleRate(returnSampleRateList, formatDesc.mSampleRate))
        returnSampleRateList.push_back(formatDesc.mSampleRate);
      formatDesc.mSampleRate = 48000;
    }
    
    if (!hasSampleRate(returnSampleRateList, formatDesc.mSampleRate))
      returnSampleRateList.push_back(formatDesc.mSampleRate);
  }
  
  return returnSampleRateList;
}

std::string AEDeviceEnumerationOSX::getDeviceNameForStream(UInt32 streamIdx) const
{
  std::string deviceName = "";
  if (m_isPlanar)// planar devices are saved as :stream0
    deviceName = m_deviceName + ":stream0";
  else
  {
    std::stringstream streamIdxStr;
    streamIdxStr << streamIdx;
    deviceName = m_deviceName + ":stream" + streamIdxStr.str();
  }
  return deviceName;
}

std::string AEDeviceEnumerationOSX::getExtraDisplayNameForStream(UInt32 streamIdx) const
{ 
  // for distinguishing the streams inside one device we add
  // the corresponding channels to the extraDisplayName
  // planar devices are ignored here as their streams are
  // the channels not different subdevices
  if (m_caStreamInfos.size() > 1 && !m_isPlanar)
  {
    // build a string with the channels for this stream
    UInt32 startChannel = 0;
    CCoreAudioStream::GetStartingChannelInDevice(m_caStreamInfos[streamIdx].streamID, startChannel);
    UInt32 numChannels = m_caDevice.GetNumChannelsOfStream(streamIdx);
    std::stringstream extraName;
    extraName << "Channels ";
    extraName << startChannel;
    extraName << " - ";
    extraName << startChannel + numChannels - 1;
    CLog::Log(LOGNOTICE, "%s adding stream %d as pseudo device with start channel %d and %d channels total", __FUNCTION__, (unsigned int)streamIdx, (unsigned int)startChannel, (unsigned int)numChannels);
    return extraName.str();
  }

  //for all other devices use the datasource as extraname
  return m_caDevice.GetCurrentDataSourceName();
}

float AEDeviceEnumerationOSX::scoreSampleRate(Float64 destinationRate, unsigned int sourceRate) const
{
  float score = 0;
  double intPortion;
  double fracPortion = modf(destinationRate / sourceRate, &intPortion);

  score += (1 - fracPortion) * 1000;      // prefer sample rates that are multiples of the source sample rate
  score += (intPortion == 1.0) ? 500 : 0;   // prefer exact matches over other multiples
  score += (intPortion > 1 && intPortion < 100) ? (100 - intPortion) / 100 * 100 : 0; // prefer smaller multiples otherwise

  return score;
}

float AEDeviceEnumerationOSX::ScoreFormat(const AudioStreamBasicDescription &formatDesc, const AEAudioFormat &format) const
{
  float score = 0;
  if (format.m_dataFormat == AE_FMT_AC3 ||
      format.m_dataFormat == AE_FMT_DTS)
  {
    if (formatDesc.mFormatID == kAudioFormat60958AC3 ||
        formatDesc.mFormatID == 'IAC3' ||
        formatDesc.mFormatID == kAudioFormatAC3)
    {
      if (formatDesc.mSampleRate == format.m_sampleRate &&
          formatDesc.mBitsPerChannel == CAEUtil::DataFormatToBits(format.m_dataFormat) &&
          formatDesc.mChannelsPerFrame == format.m_channelLayout.Count())
      {
        // perfect match
        score = FLT_MAX;
      }
    }
  }
  if (format.m_dataFormat == AE_FMT_AC3 ||
      format.m_dataFormat == AE_FMT_DTS ||
      format.m_dataFormat == AE_FMT_EAC3)
  { // we should be able to bistreaming in PCM if the samplerate, bitdepth and channels match
    if (formatDesc.mSampleRate       == format.m_sampleRate                            &&
        formatDesc.mBitsPerChannel   == CAEUtil::DataFormatToBits(format.m_dataFormat) &&
        formatDesc.mChannelsPerFrame == format.m_channelLayout.Count()                 &&
        formatDesc.mFormatID         == kAudioFormatLinearPCM)
    {
      score = FLT_MAX / 2;
    }
  }
  else
  { // non-passthrough, whatever works is fine
    if (formatDesc.mFormatID == kAudioFormatLinearPCM)
    {
      score += scoreSampleRate(formatDesc.mSampleRate, format.m_sampleRate);

      if (formatDesc.mChannelsPerFrame == format.m_channelLayout.Count())
        score += 5;
      else if (formatDesc.mChannelsPerFrame > format.m_channelLayout.Count())
        score += 1;
      if (format.m_dataFormat == AE_FMT_FLOAT || format.m_dataFormat == AE_FMT_FLOATP)
      { // for float, prefer the highest bitdepth we have
        if (formatDesc.mBitsPerChannel >= 16)
          score += (formatDesc.mBitsPerChannel / 8);
      }
      else
      {
        if (formatDesc.mBitsPerChannel == CAEUtil::DataFormatToBits(format.m_dataFormat))
          score += 5;
        else if (formatDesc.mBitsPerChannel == CAEUtil::DataFormatToBits(format.m_dataFormat))
          score += 1;
      }
    }
  }
  return score;
}

bool AEDeviceEnumerationOSX::FindSuitableFormatForStream(UInt32 &streamIdx, const AEAudioFormat &format, AudioStreamBasicDescription &outputFormat, EPassthroughMode &passthrough, AudioStreamID &outputStream) const
{
  CLog::Log(LOGDEBUG, "%s: Finding stream for format %s", __FUNCTION__, CAEUtil::DataFormatToStr(format.m_dataFormat));
  
  bool                        formatFound  = false;
  float                       outputScore  = 0;
  UInt32                      streamIdxStart = streamIdx;
  UInt32                      streamIdxEnd   = streamIdx + 1;
  UInt32                      streamIdxCurrent = streamIdx;
  passthrough                                  = PassthroughModeNone;
  
  if (streamIdx == INT_MAX)
  {
    streamIdxStart = 0;
    streamIdxEnd = m_caStreamInfos.size();
    streamIdxCurrent = 0;
  }
  
  if (streamIdxCurrent >= m_caStreamInfos.size())
    return false;
  
  // loop over all streams or over given streams (depends on initial value of param streamIdx
  for(streamIdxCurrent = streamIdxStart; streamIdxCurrent < streamIdxEnd; streamIdxCurrent++)
  {
    
    // Probe physical formats
    const StreamFormatList &formats = m_caStreamInfos[streamIdxCurrent].formatList;
    for (StreamFormatList::const_iterator j = formats.begin(); j != formats.end(); ++j)
    {
      AudioStreamBasicDescription formatDesc = j->mFormat;

      // for devices with kAudioStreamAnyRate
      // assume that the user uses a fixed config
      // and knows what he is doing - so we use
      // the requested samplerate here
      if (formatDesc.mSampleRate == kAudioStreamAnyRate)
        formatDesc.mSampleRate = format.m_sampleRate;

      float score = ScoreFormat(formatDesc, format);

      std::string formatString;
      CLog::Log(LOGDEBUG, "%s: Physical Format: %s rated %f", __FUNCTION__, StreamDescriptionToString(formatDesc, formatString), score);

      if (score > outputScore)
      {
        if (score > 10000)
        {
            if (score > FLT_MAX/2)
              passthrough  = PassthroughModeNative;
            else
              passthrough  = PassthroughModeBitstream;
        }
        outputScore  = score;
        outputFormat = formatDesc;
        outputStream = m_caStreamInfos[streamIdxCurrent].streamID;
        streamIdx = streamIdxCurrent;// return the streamIdx for the caller
        formatFound = true;
      }
    }
  }
  
  if (m_isPlanar)
    outputFormat.mChannelsPerFrame = std::min((size_t)format.m_channelLayout.Count(), m_caStreamInfos.size());
  
  return formatFound;
}

// map coraudio channel labels to activeae channel labels
enum AEChannel AEDeviceEnumerationOSX::caChannelToAEChannel(const AudioChannelLabel &CAChannelLabel) const
{
  enum AEChannel ret = AE_CH_NULL;
  static unsigned int unknownChannel = AE_CH_UNKNOWN1;
  switch(CAChannelLabel)
  {
    case kAudioChannelLabel_Left:
      ret = AE_CH_FL;
      break;
    case kAudioChannelLabel_Right:
      ret = AE_CH_FR;
      break;
    case kAudioChannelLabel_Center:
      ret = AE_CH_FC;
      break;
    case kAudioChannelLabel_LFEScreen:
      ret = AE_CH_LFE;
      break;
    case kAudioChannelLabel_LeftSurroundDirect:
      ret = AE_CH_SL;
      break;
    case kAudioChannelLabel_RightSurroundDirect:
      ret = AE_CH_SR;
      break;
    case kAudioChannelLabel_LeftCenter:
      ret = AE_CH_FLOC;
      break;
    case kAudioChannelLabel_RightCenter:
      ret = AE_CH_FROC;
      break;
    case kAudioChannelLabel_CenterSurround:
      ret = AE_CH_TC;
      break;
    case kAudioChannelLabel_LeftSurround:
      ret = AE_CH_SL;
      break;
    case kAudioChannelLabel_RightSurround:
      ret = AE_CH_SR;
      break;
    case kAudioChannelLabel_VerticalHeightLeft:
      ret = AE_CH_TFL;
      break;
    case kAudioChannelLabel_VerticalHeightRight:
      ret = AE_CH_TFR;
      break;
    case kAudioChannelLabel_VerticalHeightCenter:
      ret = AE_CH_TFC;
      break;
    case kAudioChannelLabel_TopCenterSurround:
      ret = AE_CH_TC;
      break;
    case kAudioChannelLabel_TopBackLeft:
      ret = AE_CH_TBL;
      break;
    case kAudioChannelLabel_TopBackRight:
      ret = AE_CH_TBR;
      break;
    case kAudioChannelLabel_TopBackCenter:
      ret = AE_CH_TBC;
      break;
    case kAudioChannelLabel_RearSurroundLeft:
      ret = AE_CH_BL;
      break;
    case kAudioChannelLabel_RearSurroundRight:
      ret = AE_CH_BR;
      break;
    case kAudioChannelLabel_LeftWide:
      ret = AE_CH_BLOC;
      break;
    case kAudioChannelLabel_RightWide:
      ret = AE_CH_BROC;
      break;
    case kAudioChannelLabel_LFE2:
      ret = AE_CH_LFE;
      break;
    case kAudioChannelLabel_LeftTotal:
      ret = AE_CH_FL;
      break;
    case kAudioChannelLabel_RightTotal:
      ret = AE_CH_FR;
      break;
    case kAudioChannelLabel_HearingImpaired:
      ret = AE_CH_FC;
      break;
    case kAudioChannelLabel_Narration:
      ret = AE_CH_FC;
      break;
    case kAudioChannelLabel_Mono:
      ret = AE_CH_FC;
      break;
    case kAudioChannelLabel_DialogCentricMix:
      ret = AE_CH_FC;
      break;
    case kAudioChannelLabel_CenterSurroundDirect:
      ret = AE_CH_TC;
      break;
    case kAudioChannelLabel_Haptic:
      ret = AE_CH_FC;
      break;
    default:
      ret = (enum AEChannel)unknownChannel++;
  }
  if (unknownChannel == AE_CH_MAX)
    unknownChannel = AE_CH_UNKNOWN1;
  
  return ret;
}

//Note: in multichannel mode CA will either pull 2 channels of data (stereo) or 6/8 channels of data
//(every speaker setup with more then 2 speakers). The difference between the number of real speakers
//and 6/8 channels needs to be padded with unknown channels so that the sample size fits 6/8 channels
//
//device [in] - the device whose channel layout should be used
//channelMap [in/out] - if filled it will it indicates that we are called from initialize and we log the requested map, out returns the channelMap for device
//channelsPerFrame [in] - the number of channels this device is configured to (e.x. 2 or 6/8)
void AEDeviceEnumerationOSX::GetAEChannelMap(CAEChannelInfo &channelMap, unsigned int channelsPerFrame) const
{
  CCoreAudioChannelLayout calayout;
  bool logMapping = channelMap.Count() > 0; // only log if the engine requests a layout during init
  bool mapAvailable = false;
  unsigned int numberChannelsInDeviceLayout = CA_MAX_CHANNELS; // default number of channels from CAChannelMap
  AudioChannelLayout *layout = NULL;
  
  // try to fetch either the multichannel or the stereo channel layout from the device
  if (channelsPerFrame == 2 || channelMap.Count() == 2)
    mapAvailable = m_caDevice.GetPreferredChannelLayoutForStereo(calayout);
  else
    mapAvailable = m_caDevice.GetPreferredChannelLayout(calayout);
  
  // if a map was fetched - check if it is usable
  if (mapAvailable)
  {
    layout = calayout;
    if (layout == NULL || layout->mChannelLayoutTag != kAudioChannelLayoutTag_UseChannelDescriptions)
      mapAvailable = false;// wrong map format
    else
      numberChannelsInDeviceLayout = layout->mNumberChannelDescriptions;
  }
  
  // start the mapping action
  // the number of channels to be added to the outgoing channelmap
  // this is CA_MAX_CHANNELS at max and might be lower for some output devices (channelsPerFrame)
  unsigned int numChannelsToMap = std::min((unsigned int)CA_MAX_CHANNELS, (unsigned int)channelsPerFrame);
  
  // if there was a map fetched we force the number of
  // channels to map to channelsPerFrame (this allows mapping
  // of more then CA_MAX_CHANNELS if needed)
  if (mapAvailable)
    numChannelsToMap = channelsPerFrame;
  
  std::string layoutStr;
  
  if (logMapping)
  {
    CLog::Log(LOGDEBUG, "%s Engine requests layout %s", __FUNCTION__, ((std::string)channelMap).c_str());
    
    if (mapAvailable)
      CLog::Log(LOGDEBUG, "%s trying to map to %s layout: %s", __FUNCTION__, channelsPerFrame == 2 ? "stereo" : "multichannel", calayout.ChannelLayoutToString(*layout, layoutStr));
    else
      CLog::Log(LOGDEBUG, "%s no map available - using static multichannel map layout", __FUNCTION__);
  }
  
  channelMap.Reset();// start with an empty map
  
  for (unsigned int channel = 0; channel < numChannelsToMap; channel++)
  {
    // we only try to map channels which are defined in the device layout
    enum AEChannel currentChannel;
    if (channel < numberChannelsInDeviceLayout)
    {
      // get the channel from the fetched map
      if (mapAvailable)
        currentChannel = caChannelToAEChannel(layout->mChannelDescriptions[channel].mChannelLabel);
      else// get the channel from the default map
        currentChannel = CAChannelMap[channel];
      
    }
    else// fill with unknown channels
      currentChannel = caChannelToAEChannel(kAudioChannelLabel_Unknown);
    
    if(!channelMap.HasChannel(currentChannel))// only add if not already added
      channelMap += currentChannel;
  }
  
  if (logMapping)
    CLog::Log(LOGDEBUG, "%s mapped channels to layout %s", __FUNCTION__, ((std::string)channelMap).c_str());
}
