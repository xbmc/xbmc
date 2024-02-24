/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DVDResource.h"
#include "cores/VideoPlayer/Buffers/VideoBuffer.h"
#include "cores/VideoPlayer/Interface/DemuxPacket.h"
#include "cores/VideoPlayer/Process/ProcessInfo.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/mastering_display_metadata.h>
}

#include <vector>
#include <string>
#include <map>

class CSetting;

// when modifying these structures, make sure you update all codecs accordingly
#define FRAME_TYPE_UNDEF 0
#define FRAME_TYPE_I     1
#define FRAME_TYPE_P     2
#define FRAME_TYPE_B     3
#define FRAME_TYPE_D     4


// should be entirely filled by all codecs
struct VideoPicture
{
public:
  VideoPicture();
  ~VideoPicture();
  VideoPicture& CopyRef(const VideoPicture &pic);
  VideoPicture& SetParams(const VideoPicture &pic);
  void Reset(); // reinitialize members, videoBuffer will be released if set!

  CVideoBuffer *videoBuffer = nullptr;

  double pts; // timestamp in seconds, used in the CVideoPlayer class to keep track of pts
  double dts;
  unsigned int iFlags;
  double iRepeatPicture;
  double iDuration;
  unsigned int iFrameType         : 4;  //< see defines above // 1->I, 2->P, 3->B, 0->Undef
  AVColorSpace color_space;
  unsigned int color_range        : 1;  //< 1 indicate if we have a full range of color
  AVChromaLocation chroma_position;
  AVColorPrimaries color_primaries; // heuristics applied when original is AVCOL_PRI_UNSPECIFIED
  AVColorPrimaries m_originalColorPrimaries;
  AVColorTransferCharacteristic color_transfer;
  unsigned int colorBits = 8;
  std::string stereoMode;

  int8_t* qp_table;                //< Quantization parameters, primarily used by filters
  int qstride;
  int qscale_type;
  int pict_type;

  StreamHdrType hdrType;

  bool hasDisplayMetadata = false;
  AVMasteringDisplayMetadata displayMetadata;
  bool hasLightMetadata = false;
  AVContentLightMetadata lightMetadata;

  AVPixelFormat pixelFormat; //< source pixel format

  unsigned int iWidth;
  unsigned int iHeight;
  unsigned int iDisplayWidth;           //< width of the picture without black bars
  unsigned int iDisplayHeight;          //< height of the picture without black bars

private:
  VideoPicture(VideoPicture const&);
  VideoPicture& operator=(VideoPicture const&);
};

#define DVP_FLAG_TOP_FIELD_FIRST    0x00000001
#define DVP_FLAG_REPEAT_TOP_FIELD   0x00000002  //< Set to indicate that the top field should be repeated
#define DVP_FLAG_INTERLACED         0x00000008  //< Set to indicate that this frame is interlaced
#define DVP_FLAG_DROPPED            0x00000010  //< indicate that this picture has been dropped in decoder stage, will have no data

#define DVD_CODEC_CTRL_SKIPDEINT    0x01000000  //< request to skip a deinterlacing cycle, if possible
#define DVD_CODEC_CTRL_NO_POSTPROC  0x02000000  //< see GetCodecStats
#define DVD_CODEC_CTRL_HURRY        0x04000000  //< see GetCodecStats
#define DVD_CODEC_CTRL_DROP         0x08000000  //< drop in decoder or set DVP_FLAG_DROPPED, no render of frame
#define DVD_CODEC_CTRL_DROP_ANY     0x10000000  //< drop some non-reference frame
#define DVD_CODEC_CTRL_DRAIN        0x20000000  //< squeeze out pictured without feeding new packets
#define DVD_CODEC_CTRL_ROTATE       0x40000000  //< rotate if renderer does not support it

// DVP_FLAG 0x00000100 - 0x00000f00 is in use by libmpeg2!

#define DVP_QSCALE_UNKNOWN          0
#define DVP_QSCALE_MPEG1            1
#define DVP_QSCALE_MPEG2            2
#define DVP_QSCALE_H264             3

class CDVDStreamInfo;
class CDVDCodecOption;
class CDVDCodecOptions;

class CDVDVideoCodec
{
public:

  enum VCReturn
  {
    VC_NONE = 0,
    VC_ERROR,           //< an error occurred, no other messages will be returned
    VC_FATAL,           //< non recoverable error
    VC_BUFFER,          //< the decoder needs more data
    VC_PICTURE,         //< the decoder got a picture, call Decode(NULL, 0) again to parse the rest of the data
    VC_FLUSHED,         //< the decoder lost it's state, we need to restart decoding again
    VC_NOBUFFER,        //< last FFmpeg GetBuffer failed
    VC_REOPEN,          //< decoder request to re-open
    VC_EOF              //< EOF
  };

  explicit CDVDVideoCodec(CProcessInfo &processInfo) : m_processInfo(processInfo) {}
  virtual ~CDVDVideoCodec() = default;

  /**
   * Open the decoder, returns true on success
   * Decoders not capable of running multiple instances should return false in case
   * there is already a instance open
   */
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options) = 0;

  /**
   * Reconfigure the decoder, returns true on success
   * Decoders not capable of running multiple instances may be capable of reconfiguring
   * the running instance. If Reconfigure returns false, player will close / open
   * the decoder
   */
  virtual bool Reconfigure(CDVDStreamInfo &hints)
  {
    return false;
  }

  /**
   * add data, decoder has to consume the entire packet
   * returns true if the packet was consumed or if resubmitting it is useless
   */
  virtual bool AddData(const DemuxPacket &packet) = 0;

  /**
   * Reset the decoder.
   * Should be the same as calling Dispose and Open after each other
   */
  virtual void Reset() = 0;

  /**
   * GetPicture controls decoding. Player calls it on every cycle
   * it can signal a picture, request a buffer, or return none, if nothing applies
   * the data is valid until the next GetPicture return VC_PICTURE
   */
  virtual VCReturn GetPicture(VideoPicture* pVideoPicture) = 0;

  /**
   * will be called by video player indicating the playback speed. see DVD_PLAYSPEED_NORMAL,
   * DVD_PLAYSPEED_PAUSE and friends.
   */
  virtual void SetSpeed(int iSpeed) {}

  /**
   * should return codecs name
   */
  virtual const char* GetName() = 0;

  /**
   * How many packets should player remember, so codec can recover should
   * something cause it to flush outside of players control
   */
  virtual unsigned GetConvergeCount()
  {
    return 0;
  }

  /**
   * Number of references to old pictures that are allowed to be retained when
   * calling decode on the next demux packet
   */
  virtual unsigned GetAllowedReferences() { return 0; }

  /**
   * For calculation of dropping requirements player asks for some information.
   * - pts : right after decoder, used to detect gaps (dropped frames in decoder)
   * - droppedFrames : indicates if decoder has dropped a frame
   *                 -1 means that decoder has no info on this.
   * - skippedPics : indicates if postproc has skipped a already decoded picture
   *                 -1 means that decoder has no info on this.
   *
   * If codec does not implement this method, pts of decoded frame at input
   * video player is used. In case decoder does post-proc and de-interlacing there
   * may be quite some frames queued up between exit decoder and entry player.
   */
  virtual bool GetCodecStats(double &pts, int &droppedFrames, int &skippedPics)
  {
    droppedFrames = -1;
    skippedPics = -1;
    return false;
  }

  /**
   * Codec can be informed by player with the following flags:
   *
   * DVD_CODEC_CTRL_NO_POSTPROC :
   *                  if speed is not normal the codec can switch off
   *                  postprocessing and de-interlacing
   *
   * DVD_CODEC_CTRL_HURRY :
   *                  codecs may do postprocessing and de-interlacing.
   *                  If video buffers in RenderManager are about to run dry,
   *                  this is signaled to codec. Codec can wait for post-proc
   *                  to be finished instead of returning empty and getting another
   *                  packet.
   *
   * DVD_CODEC_CTRL_DRAIN :
   *                  instruct decoder to deliver last pictures without requesting
   *                  new packets
   *
   * DVD_CODEC_CTRL_DROP :
   *                  this packet is going to be dropped. decoder is free to use it
   *                  for decoding
   *
   */
  virtual void SetCodecControl(int flags) {}

  /**
   * Re-open the decoder.
   * Decoder request to re-open
   */
  virtual void Reopen() {}

protected:
  CProcessInfo &m_processInfo;
};

// callback interface for ffmpeg hw accelerators
class IHardwareDecoder : public IDVDResourceCounted<IHardwareDecoder>
{
public:
  IHardwareDecoder() = default;
  ~IHardwareDecoder() override = default;
  virtual bool Open(AVCodecContext* avctx, AVCodecContext* mainctx, const enum AVPixelFormat) = 0;
  virtual CDVDVideoCodec::VCReturn Decode(AVCodecContext* avctx, AVFrame* frame) = 0;
  virtual bool GetPicture(AVCodecContext* avctx, VideoPicture* picture) = 0;
  virtual CDVDVideoCodec::VCReturn Check(AVCodecContext* avctx) = 0;
  virtual void Reset() {}
  virtual unsigned GetAllowedReferences() { return 0; }
  virtual bool CanSkipDeint() {return false; }
  virtual const std::string Name() = 0;
  virtual void SetCodecControl(int flags) {}
};

class ICallbackHWAccel
{
public:
  virtual ~ICallbackHWAccel() = default;
  virtual IHardwareDecoder* GetHWAccel() = 0;
  virtual bool GetPictureCommon(VideoPicture* pVideoPicture) = 0;
};
