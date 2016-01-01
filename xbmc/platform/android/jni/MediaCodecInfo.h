#pragma once
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

#include "JNIBase.h"

class CJNIMediaCodecInfoCodecProfileLevel : public CJNIBase
{
public:
  CJNIMediaCodecInfoCodecProfileLevel(const jni::jhobject &object) : CJNIBase(object) {};
  //~CJNIMediaCodecInfoCodecProfileLevel(){};

  int profile() const;
  int level()   const;

  static void PopulateStaticFields();

  static int AVCProfileBaseline;
  static int AVCProfileMain;
  static int AVCProfileExtended;
  static int AVCProfileHigh;
  static int AVCProfileHigh10;
  static int AVCProfileHigh422;
  static int AVCProfileHigh444;
  static int AVCLevel1;
  static int AVCLevel1b;
  static int AVCLevel11;
  static int AVCLevel12;
  static int AVCLevel13;
  static int AVCLevel2;
  static int AVCLevel21;
  static int AVCLevel22;
  static int AVCLevel3;
  static int AVCLevel31;
  static int AVCLevel32;
  static int AVCLevel4;
  static int AVCLevel41;
  static int AVCLevel42;
  static int AVCLevel5;
  static int AVCLevel51;
  static int H263ProfileBaseline;
  static int H263ProfileH320Coding;
  static int H263ProfileBackwardCompatible;
  static int H263ProfileISWV2;
  static int H263ProfileISWV3;
  static int H263ProfileHighCompression;
  static int H263ProfileInternet;
  static int H263ProfileInterlace;
  static int H263ProfileHighLatency;
  static int H263Level10;
  static int H263Level20;
  static int H263Level30;
  static int H263Level40;
  static int H263Level45;
  static int H263Level50;
  static int H263Level60;
  static int H263Level70;
  static int MPEG4ProfileSimple;
  static int MPEG4ProfileSimpleScalable;
  static int MPEG4ProfileCore;
  static int MPEG4ProfileMain;
  static int MPEG4ProfileNbit;
  static int MPEG4ProfileScalableTexture;
  static int MPEG4ProfileSimpleFace;
  static int MPEG4ProfileSimpleFBA;
  static int MPEG4ProfileBasicAnimated;
  static int MPEG4ProfileHybrid;
  static int MPEG4ProfileAdvancedRealTime;
  static int MPEG4ProfileCoreScalable;
  static int MPEG4ProfileAdvancedCoding;
  static int MPEG4ProfileAdvancedCore;
  static int MPEG4ProfileAdvancedScalable;
  static int MPEG4ProfileAdvancedSimple;
  static int MPEG4Level0;
  static int MPEG4Level0b;
  static int MPEG4Level1;
  static int MPEG4Level2;
  static int MPEG4Level3;
  static int MPEG4Level4;
  static int MPEG4Level4a;
  static int MPEG4Level5;
  static int AACObjectMain;
  static int AACObjectLC;
  static int AACObjectSSR;
  static int AACObjectLTP;
  static int AACObjectHE;
  static int AACObjectScalable;
  static int AACObjectERLC;
  static int AACObjectLD;
  static int AACObjectHE_PS;
  static int AACObjectELD;

private:
  CJNIMediaCodecInfoCodecProfileLevel();
  static const char *m_classname;
};
/*
Compiled from "MediaCodecInfo.java"
public final class android.media.MediaCodecInfo$CodecProfileLevel extends java.lang.Object{
public int profile;
  Signature: I
public int level;
  Signature: I
public android.media.MediaCodecInfo$CodecProfileLevel();
  Signature: ()V
}
*/

class CJNIMediaCodecInfoCodecCapabilities : public CJNIBase
{
public:
  CJNIMediaCodecInfoCodecCapabilities(const jni::jhobject &object) : CJNIBase(object) {};
  //~CJNIMediaCodecInfoCodecCapabilities() {};

  std::vector<int> colorFormats() const;
  std::vector<CJNIMediaCodecInfoCodecProfileLevel> profileLevels() const;

  static void PopulateStaticFields();

  static int COLOR_FormatMonochrome;
  static int COLOR_Format8bitRGB332;
  static int COLOR_Format12bitRGB444;
  static int COLOR_Format16bitARGB4444;
  static int COLOR_Format16bitARGB1555;
  static int COLOR_Format16bitRGB565;
  static int COLOR_Format16bitBGR565;
  static int COLOR_Format18bitRGB666;
  static int COLOR_Format18bitARGB1665;
  static int COLOR_Format19bitARGB1666;
  static int COLOR_Format24bitRGB888;
  static int COLOR_Format24bitBGR888;
  static int COLOR_Format24bitARGB1887;
  static int COLOR_Format25bitARGB1888;
  static int COLOR_Format32bitBGRA8888;
  static int COLOR_Format32bitARGB8888;
  static int COLOR_FormatYUV411Planar;
  static int COLOR_FormatYUV411PackedPlanar;
  static int COLOR_FormatYUV420Planar;
  static int COLOR_FormatYUV420PackedPlanar;
  static int COLOR_FormatYUV420SemiPlanar;
  static int COLOR_FormatYUV422Planar;
  static int COLOR_FormatYUV422PackedPlanar;
  static int COLOR_FormatYUV422SemiPlanar;
  static int COLOR_FormatYCbYCr;
  static int COLOR_FormatYCrYCb;
  static int COLOR_FormatCbYCrY;
  static int COLOR_FormatCrYCbY;
  static int COLOR_FormatYUV444Interleaved;
  static int COLOR_FormatRawBayer8bit;
  static int COLOR_FormatRawBayer10bit;
  static int COLOR_FormatRawBayer8bitcompressed;
  static int COLOR_FormatL2;
  static int COLOR_FormatL4;
  static int COLOR_FormatL8;
  static int COLOR_FormatL16;
  static int COLOR_FormatL24;
  static int COLOR_FormatL32;
  static int COLOR_FormatYUV420PackedSemiPlanar;
  static int COLOR_FormatYUV422PackedSemiPlanar;
  static int COLOR_Format18BitBGR666;
  static int COLOR_Format24BitARGB6666;
  static int COLOR_Format24BitABGR6666;
  static int COLOR_TI_FormatYUV420PackedSemiPlanar;
  static int COLOR_QCOM_FormatYUV420SemiPlanar;
  static int OMX_QCOM_COLOR_FormatYVU420SemiPlanarInterlace;

private:
  CJNIMediaCodecInfoCodecCapabilities();

  static const char *m_classname;
};

class CJNIMediaCodecInfo : public CJNIBase
{
public:
  CJNIMediaCodecInfo(const jni::jhobject &object) : CJNIBase(object) {};
  //~CJNIMediaCodecInfo() {};

  std::string getName()   const;
  bool        isEncoder() const;
  std::vector<std::string> getSupportedTypes() const;
  const CJNIMediaCodecInfoCodecCapabilities getCapabilitiesForType(const std::string &type) const;

private:
  CJNIMediaCodecInfo();
};

