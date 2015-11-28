/*
 *      Copyright (C) 2013 Team XBMC
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

#include "MediaCodecInfo.h"

#include "jutils/jutils-details.hpp"

using namespace jni;

int CJNIMediaCodecInfoCodecProfileLevel::AVCProfileBaseline(0);
int CJNIMediaCodecInfoCodecProfileLevel::AVCProfileMain(0);
int CJNIMediaCodecInfoCodecProfileLevel::AVCProfileExtended(0);
int CJNIMediaCodecInfoCodecProfileLevel::AVCProfileHigh(0);
int CJNIMediaCodecInfoCodecProfileLevel::AVCProfileHigh10(0);
int CJNIMediaCodecInfoCodecProfileLevel::AVCProfileHigh422(0);
int CJNIMediaCodecInfoCodecProfileLevel::AVCProfileHigh444(0);
int CJNIMediaCodecInfoCodecProfileLevel::AVCLevel1(0);
int CJNIMediaCodecInfoCodecProfileLevel::AVCLevel1b(0);
int CJNIMediaCodecInfoCodecProfileLevel::AVCLevel11(0);
int CJNIMediaCodecInfoCodecProfileLevel::AVCLevel12(0);
int CJNIMediaCodecInfoCodecProfileLevel::AVCLevel13(0);
int CJNIMediaCodecInfoCodecProfileLevel::AVCLevel2(0);
int CJNIMediaCodecInfoCodecProfileLevel::AVCLevel21(0);
int CJNIMediaCodecInfoCodecProfileLevel::AVCLevel22(0);
int CJNIMediaCodecInfoCodecProfileLevel::AVCLevel3(0);
int CJNIMediaCodecInfoCodecProfileLevel::AVCLevel31(0);
int CJNIMediaCodecInfoCodecProfileLevel::AVCLevel32(0);
int CJNIMediaCodecInfoCodecProfileLevel::AVCLevel4(0);
int CJNIMediaCodecInfoCodecProfileLevel::AVCLevel41(0);
int CJNIMediaCodecInfoCodecProfileLevel::AVCLevel42(0);
int CJNIMediaCodecInfoCodecProfileLevel::AVCLevel5(0);
int CJNIMediaCodecInfoCodecProfileLevel::AVCLevel51(0);
int CJNIMediaCodecInfoCodecProfileLevel::H263ProfileBaseline(0);
int CJNIMediaCodecInfoCodecProfileLevel::H263ProfileH320Coding(0);
int CJNIMediaCodecInfoCodecProfileLevel::H263ProfileBackwardCompatible(0);
int CJNIMediaCodecInfoCodecProfileLevel::H263ProfileISWV2(0);
int CJNIMediaCodecInfoCodecProfileLevel::H263ProfileISWV3(0);
int CJNIMediaCodecInfoCodecProfileLevel::H263ProfileHighCompression(0);
int CJNIMediaCodecInfoCodecProfileLevel::H263ProfileInternet(0);
int CJNIMediaCodecInfoCodecProfileLevel::H263ProfileInterlace(0);
int CJNIMediaCodecInfoCodecProfileLevel::H263ProfileHighLatency(0);
int CJNIMediaCodecInfoCodecProfileLevel::H263Level10(0);
int CJNIMediaCodecInfoCodecProfileLevel::H263Level20(0);
int CJNIMediaCodecInfoCodecProfileLevel::H263Level30(0);
int CJNIMediaCodecInfoCodecProfileLevel::H263Level40(0);
int CJNIMediaCodecInfoCodecProfileLevel::H263Level45(0);
int CJNIMediaCodecInfoCodecProfileLevel::H263Level50(0);
int CJNIMediaCodecInfoCodecProfileLevel::H263Level60(0);
int CJNIMediaCodecInfoCodecProfileLevel::H263Level70(0);
int CJNIMediaCodecInfoCodecProfileLevel::MPEG4ProfileSimple(0);
int CJNIMediaCodecInfoCodecProfileLevel::MPEG4ProfileSimpleScalable(0);
int CJNIMediaCodecInfoCodecProfileLevel::MPEG4ProfileCore(0);
int CJNIMediaCodecInfoCodecProfileLevel::MPEG4ProfileMain(0);
int CJNIMediaCodecInfoCodecProfileLevel::MPEG4ProfileNbit(0);
int CJNIMediaCodecInfoCodecProfileLevel::MPEG4ProfileScalableTexture(0);
int CJNIMediaCodecInfoCodecProfileLevel::MPEG4ProfileSimpleFace(0);
int CJNIMediaCodecInfoCodecProfileLevel::MPEG4ProfileSimpleFBA(0);
int CJNIMediaCodecInfoCodecProfileLevel::MPEG4ProfileBasicAnimated(0);
int CJNIMediaCodecInfoCodecProfileLevel::MPEG4ProfileHybrid(0);
int CJNIMediaCodecInfoCodecProfileLevel::MPEG4ProfileAdvancedRealTime(0);
int CJNIMediaCodecInfoCodecProfileLevel::MPEG4ProfileCoreScalable(0);
int CJNIMediaCodecInfoCodecProfileLevel::MPEG4ProfileAdvancedCoding(0);
int CJNIMediaCodecInfoCodecProfileLevel::MPEG4ProfileAdvancedCore(0);
int CJNIMediaCodecInfoCodecProfileLevel::MPEG4ProfileAdvancedScalable(0);
int CJNIMediaCodecInfoCodecProfileLevel::MPEG4ProfileAdvancedSimple(0);
int CJNIMediaCodecInfoCodecProfileLevel::MPEG4Level0(0);
int CJNIMediaCodecInfoCodecProfileLevel::MPEG4Level0b(0);
int CJNIMediaCodecInfoCodecProfileLevel::MPEG4Level1(0);
int CJNIMediaCodecInfoCodecProfileLevel::MPEG4Level2(0);
int CJNIMediaCodecInfoCodecProfileLevel::MPEG4Level3(0);
int CJNIMediaCodecInfoCodecProfileLevel::MPEG4Level4(0);
int CJNIMediaCodecInfoCodecProfileLevel::MPEG4Level4a(0);
int CJNIMediaCodecInfoCodecProfileLevel::MPEG4Level5(0);
int CJNIMediaCodecInfoCodecProfileLevel::AACObjectMain(0);
int CJNIMediaCodecInfoCodecProfileLevel::AACObjectLC(0);
int CJNIMediaCodecInfoCodecProfileLevel::AACObjectSSR(0);
int CJNIMediaCodecInfoCodecProfileLevel::AACObjectLTP(0);
int CJNIMediaCodecInfoCodecProfileLevel::AACObjectHE(0);
int CJNIMediaCodecInfoCodecProfileLevel::AACObjectScalable(0);
int CJNIMediaCodecInfoCodecProfileLevel::AACObjectERLC(0);
int CJNIMediaCodecInfoCodecProfileLevel::AACObjectLD(0);
int CJNIMediaCodecInfoCodecProfileLevel::AACObjectHE_PS(0);
int CJNIMediaCodecInfoCodecProfileLevel::AACObjectELD(0);
const char *CJNIMediaCodecInfoCodecProfileLevel::m_classname = "platform/android/media/MediaCodecInfo$CodecProfileLevel";

void CJNIMediaCodecInfoCodecProfileLevel::PopulateStaticFields()
{
  if(GetSDKVersion() >= 16)
  {
    jhclass clazz = find_class(m_classname);
    AVCProfileBaseline          = (get_static_field<int>(clazz, "AVCProfileBaseline"));
    AVCProfileMain              = (get_static_field<int>(clazz, "AVCProfileMain"));
    AVCProfileExtended          = (get_static_field<int>(clazz, "AVCProfileExtended"));
    AVCProfileHigh              = (get_static_field<int>(clazz, "AVCProfileHigh"));
    AVCProfileHigh10            = (get_static_field<int>(clazz, "AVCProfileHigh10"));
    AVCProfileHigh422           = (get_static_field<int>(clazz, "AVCProfileHigh422"));
    AVCProfileHigh444           = (get_static_field<int>(clazz, "AVCProfileHigh444"));
    AVCLevel1                   = (get_static_field<int>(clazz, "AVCLevel1"));
    AVCLevel1b                  = (get_static_field<int>(clazz, "AVCLevel1b"));
    AVCLevel11                  = (get_static_field<int>(clazz, "AVCLevel11"));
    AVCLevel12                  = (get_static_field<int>(clazz, "AVCLevel12"));
    AVCLevel13                  = (get_static_field<int>(clazz, "AVCLevel13"));
    AVCLevel2                   = (get_static_field<int>(clazz, "AVCLevel2"));
    AVCLevel21                  = (get_static_field<int>(clazz, "AVCLevel21"));
    AVCLevel22                  = (get_static_field<int>(clazz, "AVCLevel22"));
    AVCLevel3                   = (get_static_field<int>(clazz, "AVCLevel3"));
    AVCLevel31                  = (get_static_field<int>(clazz, "AVCLevel31"));
    AVCLevel32                  = (get_static_field<int>(clazz, "AVCLevel32"));
    AVCLevel4                   = (get_static_field<int>(clazz, "AVCLevel4"));
    AVCLevel41                  = (get_static_field<int>(clazz, "AVCLevel41"));
    AVCLevel42                  = (get_static_field<int>(clazz, "AVCLevel42"));
    AVCLevel5                   = (get_static_field<int>(clazz, "AVCLevel5"));
    AVCLevel51                  = (get_static_field<int>(clazz, "AVCLevel51"));
    H263ProfileBaseline         = (get_static_field<int>(clazz, "H263ProfileBaseline"));
    H263ProfileH320Coding       = (get_static_field<int>(clazz, "H263ProfileH320Coding"));
    H263ProfileBackwardCompatible = (get_static_field<int>(clazz, "H263ProfileBackwardCompatible"));
    H263ProfileISWV2            = (get_static_field<int>(clazz, "H263ProfileISWV2"));
    H263ProfileISWV3            = (get_static_field<int>(clazz, "H263ProfileISWV3"));
    H263ProfileHighCompression  = (get_static_field<int>(clazz, "H263ProfileHighCompression"));
    H263ProfileInternet         = (get_static_field<int>(clazz, "H263ProfileInternet"));
    H263ProfileInterlace        = (get_static_field<int>(clazz, "H263ProfileInterlace"));
    H263ProfileHighLatency      = (get_static_field<int>(clazz, "H263ProfileHighLatency"));
    H263Level10                 = (get_static_field<int>(clazz, "H263Level10"));
    H263Level20                 = (get_static_field<int>(clazz, "H263Level20"));
    H263Level30                 = (get_static_field<int>(clazz, "H263Level30"));
    H263Level40                 = (get_static_field<int>(clazz, "H263Level40"));
    H263Level45                 = (get_static_field<int>(clazz, "H263Level45"));
    H263Level50                 = (get_static_field<int>(clazz, "H263Level50"));
    H263Level60                 = (get_static_field<int>(clazz, "H263Level60"));
    H263Level70                 = (get_static_field<int>(clazz, "H263Level70"));
    MPEG4ProfileSimple          = (get_static_field<int>(clazz, "MPEG4ProfileSimple"));
    MPEG4ProfileSimpleScalable  = (get_static_field<int>(clazz, "MPEG4ProfileSimpleScalable"));
    MPEG4ProfileCore            = (get_static_field<int>(clazz, "MPEG4ProfileCore"));
    MPEG4ProfileMain            = (get_static_field<int>(clazz, "MPEG4ProfileMain"));
    MPEG4ProfileNbit            = (get_static_field<int>(clazz, "MPEG4ProfileNbit"));
    MPEG4ProfileScalableTexture = (get_static_field<int>(clazz, "MPEG4ProfileScalableTexture"));
    MPEG4ProfileSimpleFace      = (get_static_field<int>(clazz, "MPEG4ProfileSimpleFace"));
    MPEG4ProfileSimpleFBA       = (get_static_field<int>(clazz, "MPEG4ProfileSimpleFBA"));
    MPEG4ProfileBasicAnimated   = (get_static_field<int>(clazz, "MPEG4ProfileBasicAnimated"));
    MPEG4ProfileHybrid          = (get_static_field<int>(clazz, "MPEG4ProfileHybrid"));
    MPEG4ProfileAdvancedRealTime = (get_static_field<int>(clazz, "MPEG4ProfileAdvancedRealTime"));
    MPEG4ProfileCoreScalable    = (get_static_field<int>(clazz, "MPEG4ProfileCoreScalable"));
    MPEG4ProfileAdvancedCoding  = (get_static_field<int>(clazz, "MPEG4ProfileAdvancedCoding"));
    MPEG4ProfileAdvancedCore    = (get_static_field<int>(clazz, "MPEG4ProfileAdvancedCore"));
    MPEG4ProfileAdvancedScalable= (get_static_field<int>(clazz, "MPEG4ProfileAdvancedScalable"));
    MPEG4ProfileAdvancedSimple  = (get_static_field<int>(clazz, "MPEG4ProfileAdvancedSimple"));
    MPEG4Level0                 = (get_static_field<int>(clazz, "MPEG4Level0"));
    MPEG4Level0b                = (get_static_field<int>(clazz, "MPEG4Level0b"));
    MPEG4Level1                 = (get_static_field<int>(clazz, "MPEG4Level1"));
    MPEG4Level2                 = (get_static_field<int>(clazz, "MPEG4Level2"));
    MPEG4Level3                 = (get_static_field<int>(clazz, "MPEG4Level3"));
    MPEG4Level4                 = (get_static_field<int>(clazz, "MPEG4Level4"));
    MPEG4Level4a                = (get_static_field<int>(clazz, "MPEG4Level4a"));
    MPEG4Level5                 = (get_static_field<int>(clazz, "MPEG4Level5"));
    AACObjectMain               = (get_static_field<int>(clazz, "AACObjectMain"));
    AACObjectLC                 = (get_static_field<int>(clazz, "AACObjectLC"));
    AACObjectSSR                = (get_static_field<int>(clazz, "AACObjectSSR"));
    AACObjectLTP                = (get_static_field<int>(clazz, "AACObjectLTP"));
    AACObjectHE                 = (get_static_field<int>(clazz, "AACObjectHE"));
    AACObjectScalable           = (get_static_field<int>(clazz, "AACObjectScalable"));
    AACObjectERLC               = (get_static_field<int>(clazz, "AACObjectERLC"));
    AACObjectLD                 = (get_static_field<int>(clazz, "AACObjectLD"));
    AACObjectHE_PS              = (get_static_field<int>(clazz, "AACObjectHE_PS"));
    AACObjectELD                = (get_static_field<int>(clazz, "AACObjectELD"));
  }
}

int CJNIMediaCodecInfoCodecProfileLevel::profile() const
{
  return get_field<int>(m_object, "profile");
}

int CJNIMediaCodecInfoCodecProfileLevel::level() const
{
  return get_field<int>(m_object, "level");
}

/**********************************************************************************/
/**********************************************************************************/
int CJNIMediaCodecInfoCodecCapabilities::COLOR_FormatMonochrome(0);
int CJNIMediaCodecInfoCodecCapabilities::COLOR_Format8bitRGB332(0);
int CJNIMediaCodecInfoCodecCapabilities::COLOR_Format12bitRGB444(0);
int CJNIMediaCodecInfoCodecCapabilities::COLOR_Format16bitARGB4444(0);
int CJNIMediaCodecInfoCodecCapabilities::COLOR_Format16bitARGB1555(0);
int CJNIMediaCodecInfoCodecCapabilities::COLOR_Format16bitRGB565(0);
int CJNIMediaCodecInfoCodecCapabilities::COLOR_Format16bitBGR565(0);
int CJNIMediaCodecInfoCodecCapabilities::COLOR_Format18bitRGB666(0);
int CJNIMediaCodecInfoCodecCapabilities::COLOR_Format18bitARGB1665(0);
int CJNIMediaCodecInfoCodecCapabilities::COLOR_Format19bitARGB1666(0);
int CJNIMediaCodecInfoCodecCapabilities::COLOR_Format24bitRGB888(0);
int CJNIMediaCodecInfoCodecCapabilities::COLOR_Format24bitBGR888(0);
int CJNIMediaCodecInfoCodecCapabilities::COLOR_Format24bitARGB1887(0);
int CJNIMediaCodecInfoCodecCapabilities::COLOR_Format25bitARGB1888(0);
int CJNIMediaCodecInfoCodecCapabilities::COLOR_Format32bitBGRA8888(0);
int CJNIMediaCodecInfoCodecCapabilities::COLOR_Format32bitARGB8888(0);
int CJNIMediaCodecInfoCodecCapabilities::COLOR_FormatYUV411Planar(0);
int CJNIMediaCodecInfoCodecCapabilities::COLOR_FormatYUV411PackedPlanar(0);
int CJNIMediaCodecInfoCodecCapabilities::COLOR_FormatYUV420Planar(0);
int CJNIMediaCodecInfoCodecCapabilities::COLOR_FormatYUV420PackedPlanar(0);
int CJNIMediaCodecInfoCodecCapabilities::COLOR_FormatYUV420SemiPlanar(0);
int CJNIMediaCodecInfoCodecCapabilities::COLOR_FormatYUV422Planar(0);
int CJNIMediaCodecInfoCodecCapabilities::COLOR_FormatYUV422PackedPlanar(0);
int CJNIMediaCodecInfoCodecCapabilities::COLOR_FormatYUV422SemiPlanar(0);
int CJNIMediaCodecInfoCodecCapabilities::COLOR_FormatYCbYCr(0);
int CJNIMediaCodecInfoCodecCapabilities::COLOR_FormatYCrYCb(0);
int CJNIMediaCodecInfoCodecCapabilities::COLOR_FormatCbYCrY(0);
int CJNIMediaCodecInfoCodecCapabilities::COLOR_FormatCrYCbY(0);
int CJNIMediaCodecInfoCodecCapabilities::COLOR_FormatYUV444Interleaved(0);
int CJNIMediaCodecInfoCodecCapabilities::COLOR_FormatRawBayer8bit(0);
int CJNIMediaCodecInfoCodecCapabilities::COLOR_FormatRawBayer10bit(0);
int CJNIMediaCodecInfoCodecCapabilities::COLOR_FormatRawBayer8bitcompressed(0);
int CJNIMediaCodecInfoCodecCapabilities::COLOR_FormatL2(0);
int CJNIMediaCodecInfoCodecCapabilities::COLOR_FormatL4(0);
int CJNIMediaCodecInfoCodecCapabilities::COLOR_FormatL8(0);
int CJNIMediaCodecInfoCodecCapabilities::COLOR_FormatL16(0);
int CJNIMediaCodecInfoCodecCapabilities::COLOR_FormatL24(0);
int CJNIMediaCodecInfoCodecCapabilities::COLOR_FormatL32(0);
int CJNIMediaCodecInfoCodecCapabilities::COLOR_FormatYUV420PackedSemiPlanar(0);
int CJNIMediaCodecInfoCodecCapabilities::COLOR_FormatYUV422PackedSemiPlanar(0);
int CJNIMediaCodecInfoCodecCapabilities::COLOR_Format18BitBGR666(0);
int CJNIMediaCodecInfoCodecCapabilities::COLOR_Format24BitARGB6666(0);
int CJNIMediaCodecInfoCodecCapabilities::COLOR_Format24BitABGR6666(0);
int CJNIMediaCodecInfoCodecCapabilities::COLOR_TI_FormatYUV420PackedSemiPlanar(0);
int CJNIMediaCodecInfoCodecCapabilities::COLOR_QCOM_FormatYUV420SemiPlanar(0);
/* This one isn't exposed in 4.4 */
int CJNIMediaCodecInfoCodecCapabilities::OMX_QCOM_COLOR_FormatYVU420SemiPlanarInterlace(0x7FA30C04);
const char *CJNIMediaCodecInfoCodecCapabilities::m_classname = "platform/android/media/MediaCodecInfo$CodecCapabilities";

void CJNIMediaCodecInfoCodecCapabilities::PopulateStaticFields()
{
  if(GetSDKVersion() >= 16)
  {
    jhclass clazz = find_class(m_classname);
    COLOR_FormatMonochrome            = (get_static_field<int>(clazz, "COLOR_FormatMonochrome"));
    COLOR_Format8bitRGB332            = (get_static_field<int>(clazz, "COLOR_Format8bitRGB332"));
    COLOR_Format12bitRGB444           = (get_static_field<int>(clazz, "COLOR_Format12bitRGB444"));
    COLOR_Format16bitARGB4444         = (get_static_field<int>(clazz, "COLOR_Format16bitARGB4444"));
    COLOR_Format16bitARGB1555         = (get_static_field<int>(clazz, "COLOR_Format16bitARGB1555"));
    COLOR_Format16bitRGB565           = (get_static_field<int>(clazz, "COLOR_Format16bitRGB565"));
    COLOR_Format16bitBGR565           = (get_static_field<int>(clazz, "COLOR_Format16bitBGR565"));
    COLOR_Format18bitRGB666           = (get_static_field<int>(clazz, "COLOR_Format18bitRGB666"));
    COLOR_Format18bitARGB1665         = (get_static_field<int>(clazz, "COLOR_Format18bitARGB1665"));
    COLOR_Format19bitARGB1666         = (get_static_field<int>(clazz, "COLOR_Format19bitARGB1666"));
    COLOR_Format24bitRGB888           = (get_static_field<int>(clazz, "COLOR_Format24bitRGB888"));
    COLOR_Format24bitBGR888           = (get_static_field<int>(clazz, "COLOR_Format24bitBGR888"));
    COLOR_Format24bitARGB1887         = (get_static_field<int>(clazz, "COLOR_Format24bitARGB1887"));
    COLOR_Format25bitARGB1888         = (get_static_field<int>(clazz, "COLOR_Format25bitARGB1888"));
    COLOR_Format32bitBGRA8888         = (get_static_field<int>(clazz, "COLOR_Format32bitBGRA8888"));
    COLOR_Format32bitARGB8888         = (get_static_field<int>(clazz, "COLOR_Format32bitARGB8888"));
    COLOR_FormatYUV411Planar          = (get_static_field<int>(clazz, "COLOR_FormatYUV411Planar"));
    COLOR_FormatYUV411PackedPlanar    = (get_static_field<int>(clazz, "COLOR_FormatYUV411PackedPlanar"));
    COLOR_FormatYUV420Planar          = (get_static_field<int>(clazz, "COLOR_FormatYUV420Planar"));
    COLOR_FormatYUV420PackedPlanar    = (get_static_field<int>(clazz, "COLOR_FormatYUV420PackedPlanar"));
    COLOR_FormatYUV420SemiPlanar      = (get_static_field<int>(clazz, "COLOR_FormatYUV420SemiPlanar"));
    COLOR_FormatYUV422Planar          = (get_static_field<int>(clazz, "COLOR_FormatYUV422Planar"));
    COLOR_FormatYUV422PackedPlanar    = (get_static_field<int>(clazz, "COLOR_FormatYUV422PackedPlanar"));
    COLOR_FormatYUV422SemiPlanar      = (get_static_field<int>(clazz, "COLOR_FormatYUV422SemiPlanar"));
    COLOR_FormatYCbYCr                = (get_static_field<int>(clazz, "COLOR_FormatYCbYCr"));
    COLOR_FormatYCrYCb                = (get_static_field<int>(clazz, "COLOR_FormatYCrYCb"));
    COLOR_FormatCbYCrY                = (get_static_field<int>(clazz, "COLOR_FormatCbYCrY"));
    COLOR_FormatCrYCbY                = (get_static_field<int>(clazz, "COLOR_FormatCrYCbY"));
    COLOR_FormatYUV444Interleaved     = (get_static_field<int>(clazz, "COLOR_FormatYUV444Interleaved"));
    COLOR_FormatRawBayer8bit          = (get_static_field<int>(clazz, "COLOR_FormatRawBayer8bit"));
    COLOR_FormatRawBayer10bit         = (get_static_field<int>(clazz, "COLOR_FormatRawBayer10bit"));
    COLOR_FormatRawBayer8bitcompressed= (get_static_field<int>(clazz, "COLOR_FormatRawBayer8bitcompressed"));
    COLOR_FormatL2                    = (get_static_field<int>(clazz, "COLOR_FormatL2"));
    COLOR_FormatL4                    = (get_static_field<int>(clazz, "COLOR_FormatL4"));
    COLOR_FormatL8                    = (get_static_field<int>(clazz, "COLOR_FormatL8"));
    COLOR_FormatL16                   = (get_static_field<int>(clazz, "COLOR_FormatL16"));
    COLOR_FormatL24                   = (get_static_field<int>(clazz, "COLOR_FormatL24"));
    COLOR_FormatL32                   = (get_static_field<int>(clazz, "COLOR_FormatL32"));
    COLOR_FormatYUV420PackedSemiPlanar= (get_static_field<int>(clazz, "COLOR_FormatYUV420PackedSemiPlanar"));
    COLOR_FormatYUV422PackedSemiPlanar= (get_static_field<int>(clazz, "COLOR_FormatYUV422PackedSemiPlanar"));
    COLOR_Format18BitBGR666           = (get_static_field<int>(clazz, "COLOR_Format18BitBGR666"));
    COLOR_Format24BitARGB6666         = (get_static_field<int>(clazz, "COLOR_Format24BitARGB6666"));
    COLOR_Format24BitABGR6666         = (get_static_field<int>(clazz, "COLOR_Format24BitABGR6666"));
    COLOR_TI_FormatYUV420PackedSemiPlanar = (get_static_field<int>(clazz, "COLOR_TI_FormatYUV420PackedSemiPlanar"));
    COLOR_QCOM_FormatYUV420SemiPlanar = (get_static_field<int>(clazz, "COLOR_QCOM_FormatYUV420SemiPlanar"));
  }
}

std::vector<int> CJNIMediaCodecInfoCodecCapabilities::colorFormats() const
{
  JNIEnv *env = xbmc_jnienv();

  jhintArray colorFormats = get_field<jhintArray>(m_object, "colorFormats");
  jsize size = env->GetArrayLength(colorFormats.get());
  std::vector<int> intarray;
  intarray.resize(size);
  env->GetIntArrayRegion(colorFormats.get(), 0, size, (jint*)intarray.data());

  return intarray;
}

std::vector<CJNIMediaCodecInfoCodecProfileLevel> CJNIMediaCodecInfoCodecCapabilities::profileLevels() const
{
  JNIEnv *env = xbmc_jnienv();

  jhobjectArray oprofileLevels = get_field<jhobjectArray>(m_object, "profileLevels");
  jsize size = env->GetArrayLength(oprofileLevels.get());
  std::vector<CJNIMediaCodecInfoCodecProfileLevel> profileLevels;
  profileLevels.reserve(size);
  for (int i = 0; i < size; i++)
    profileLevels.push_back(CJNIMediaCodecInfoCodecProfileLevel(jhobject(env->GetObjectArrayElement(oprofileLevels.get(), i))));

  return profileLevels;
}

/**********************************************************************************/
/**********************************************************************************/
std::string CJNIMediaCodecInfo::getName() const
{
  return jcast<std::string>(call_method<jhstring>(m_object,
    "getName", "()Ljava/lang/String;"));
}

bool CJNIMediaCodecInfo::isEncoder() const
{
  return call_method<jboolean>(m_object,
    "isEncoder", "()Z");
}

std::vector<std::string> CJNIMediaCodecInfo::getSupportedTypes() const
{
  return jcast<std::vector<std::string>>(call_method<jhobjectArray>(m_object,
    "getSupportedTypes", "()[Ljava/lang/String;"));
}

const CJNIMediaCodecInfoCodecCapabilities CJNIMediaCodecInfo::getCapabilitiesForType(const std::string &type) const
{
  return call_method<jhobject>(m_object,
    "getCapabilitiesForType", "(Ljava/lang/String;)Landroid/media/MediaCodecInfo$CodecCapabilities;",
    jcast<jhstring>(type));
}
