#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "system.h"

#include <vector>
#include <string>
#include "cores/VideoRenderers/RenderFormats.h"



extern "C" {
#include "libavcodec/avcodec.h"
}

class CSetting;

struct DVDCodecAvailableType 
{
  AVCodecID codec;
  const char* setting;
};

// when modifying these structures, make sure you update all codecs accordingly
#define FRAME_TYPE_UNDEF 0
#define FRAME_TYPE_I 1
#define FRAME_TYPE_P 2
#define FRAME_TYPE_B 3
#define FRAME_TYPE_D 4

namespace DXVA { class CRenderPicture; }
namespace VAAPI { class CVaapiRenderPicture; }
namespace VDPAU { class CVdpauRenderPicture; }
class COpenMax;
class COpenMaxVideo;
struct OpenMaxVideoBufferHolder;
class CDVDVideoCodecStageFright;
class CDVDMediaCodecInfo;
class CDVDVideoCodecIMXBuffer;
class CMMALVideoBuffer;
typedef void* EGLImageKHR;


// should be entirely filled by all codecs
struct DVDVideoPicture
{
  double pts; // timestamp in seconds, used in the CDVDPlayer class to keep track of pts
  double dts;

  union
  {
    struct {
      uint8_t* data[4];      // [4] = alpha channel, currently not used
      int iLineSize[4];   // [4] = alpha channel, currently not used
    };
    struct {
      DXVA::CRenderPicture* dxva;
    };
    struct {
      VDPAU::CVdpauRenderPicture* vdpau;
    };
    struct {
      VAAPI::CVaapiRenderPicture* vaapi;
    };

    struct {
      COpenMax *openMax;
      OpenMaxVideoBufferHolder *openMaxBufferHolder;
    };

    struct {
      struct __CVBuffer *cvBufferRef;
    };

    struct {
      CDVDVideoCodecStageFright* stf;
      EGLImageKHR eglimg;
    };

    struct {
      CDVDMediaCodecInfo *mediacodec;
    };

    struct {
      CDVDVideoCodecIMXBuffer *IMXBuffer;
    };

    struct {
      CMMALVideoBuffer *MMALBuffer;
    };

  };

  unsigned int iFlags;

  double       iRepeatPicture;
  double       iDuration;
  unsigned int iFrameType         : 4; // see defines above // 1->I, 2->P, 3->B, 0->Undef
  unsigned int color_matrix       : 4;
  unsigned int color_range        : 1; // 1 indicate if we have a full range of color
  unsigned int chroma_position;
  unsigned int color_primaries;
  unsigned int color_transfer;
  unsigned int extended_format;
  char         stereo_mode[32];

  int8_t* qp_table; // Quantization parameters, primarily used by filters
  int qstride;
  int qscale_type;

  unsigned int iWidth;
  unsigned int iHeight;
  unsigned int iDisplayWidth;  // width of the picture without black bars
  unsigned int iDisplayHeight; // height of the picture without black bars

  ERenderFormat format;
};

struct DVDVideoUserData
{
  uint8_t* data;
  int size;
};

#define DVP_FLAG_TOP_FIELD_FIRST    0x00000001
#define DVP_FLAG_REPEAT_TOP_FIELD   0x00000002 //Set to indicate that the top field should be repeated
#define DVP_FLAG_ALLOCATED          0x00000004 //Set to indicate that this has allocated data
#define DVP_FLAG_INTERLACED         0x00000008 //Set to indicate that this frame is interlaced

#define DVP_FLAG_NOSKIP             0x00000010 // indicate this picture should never be dropped
#define DVP_FLAG_DROPPED            0x00000020 // indicate that this picture has been dropped in decoder stage, will have no data

#define DVD_CODEC_CTRL_SKIPDEINT    0x01000000 // indicate that this picture was requested to have been dropped in deint stage
#define DVD_CODEC_CTRL_NO_POSTPROC  0x02000000 // see GetCodecStats
#define DVD_CODEC_CTRL_DRAIN        0x04000000 // see GetCodecStats

// DVP_FLAG 0x00000100 - 0x00000f00 is in use by libmpeg2!

#define DVP_QSCALE_UNKNOWN          0
#define DVP_QSCALE_MPEG1            1
#define DVP_QSCALE_MPEG2            2
#define DVP_QSCALE_H264             3

class CDVDStreamInfo;
class CDVDCodecOption;
class CDVDCodecOptions;

// VC_ messages, messages can be combined
#define VC_ERROR    0x00000001  // an error occured, no other messages will be returned
#define VC_BUFFER   0x00000002  // the decoder needs more data
#define VC_PICTURE  0x00000004  // the decoder got a picture, call Decode(NULL, 0) again to parse the rest of the data
#define VC_USERDATA 0x00000008  // the decoder found some userdata,  call Decode(NULL, 0) again to parse the rest of the data
#define VC_FLUSHED  0x00000010  // the decoder lost it's state, we need to restart decoding again
#define VC_DROPPED  0x00000020  // needed to identify if a picture was dropped
#define VC_NOBUFFER 0x00000040  // last FFmpeg GetBuffer failed
#define VC_REOPEN   0x00000080  // decoder request to re-open

class CDVDVideoCodec
{
public:

  CDVDVideoCodec() {}
  virtual ~CDVDVideoCodec() {}

  /*
   * Open the decoder, returns true on success
   */
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options) = 0;

  /*
   * Dispose, Free all resources
   */
  virtual void Dispose() = 0;

  /*
   * returns one or a combination of VC_ messages
   * pData and iSize can be NULL, this means we should flush the rest of the data.
   */
  virtual int Decode(uint8_t* pData, int iSize, double dts, double pts) = 0;

 /*
   * Reset the decoder.
   * Should be the same as calling Dispose and Open after each other
   */
  virtual void Reset() = 0;

  /*
   * returns true if successfull
   * the data is valid until the next Decode call
   */
  virtual bool GetPicture(DVDVideoPicture* pDvdVideoPicture) = 0;


  /*
   * returns true if successfull
   * the data is cleared to zero
   */ 
  virtual bool ClearPicture(DVDVideoPicture* pDvdVideoPicture)
  {
    memset(pDvdVideoPicture, 0, sizeof(DVDVideoPicture));
    return true;
  }

  /*
   * returns true if successfull
   * the data is valid until the next Decode call
   * userdata can be anything, for now we use it for closed captioning
   */
  virtual bool GetUserData(DVDVideoUserData* pDvdVideoUserData)
  {
    pDvdVideoUserData->data = NULL;
    pDvdVideoUserData->size = 0;
    return false;
  }

  /*
   * will be called by video player indicating if a frame will eventually be dropped
   * codec can then skip actually decoding the data, just consume the data set picture headers
   */
  virtual void SetDropState(bool bDrop) = 0;

  /*
   * will be called by video player indicating the playback speed. see DVD_PLAYSPEED_NORMAL,
   * DVD_PLAYSPEED_PAUSE and friends.
   */
  virtual void SetSpeed(int iSpeed) {};

  /*
   * returns the number of demuxer bytes in any internal buffers
   */
  virtual int GetDataSize(void)
  {
    return 0;
  }

  /*
   * returns the time in seconds for demuxer bytes in any internal buffers
   */
  virtual double GetTimeSize(void)
  {
    return 0;
  }

  enum EFilterFlags {
    FILTER_NONE                =  0x0,
    FILTER_DEINTERLACE_YADIF   =  0x1,  /* use first deinterlace mode */
    FILTER_DEINTERLACE_ANY     =  0xf,  /* use any deinterlace mode */
    FILTER_DEINTERLACE_FLAGGED = 0x10,  /* only deinterlace flagged frames */
    FILTER_DEINTERLACE_HALFED  = 0x20,  /* do half rate deinterlacing */
    FILTER_ROTATE              = 0x40,  /* rotate image according to the codec hints */
  };

  /*
   * set the type of filters that should be applied at decoding stage if possible
   */
  virtual unsigned int SetFilters(unsigned int filters) { return 0u; }

  /*
   *
   * should return codecs name
   */
  virtual const char* GetName() = 0;

  /*
   *
   * How many packets should player remember, so codec
   * can recover should something cause it to flush
   * outside of players control
   */
  virtual unsigned GetConvergeCount()
  {
    return 0;
  }

  /**
   * Number of references to old pictures that are allowed to
   * be retained when calling decode on the next demux packet
   */
  virtual unsigned GetAllowedReferences() { return 0; }

  /**
   * Hide or Show Settings depending on the currently running hardware 
   *
   */
   static bool IsSettingVisible(const std::string &condition, const std::string &value, const CSetting *setting, void *data);

  /**
  * Interact with user settings so that user disabled codecs are disabled
  */
  static bool IsCodecDisabled(DVDCodecAvailableType* map, unsigned int size, AVCodecID id);

   /* For calculation of dropping requirements player asks for some information.
   *
   * - pts : right after decoder, used to detect gaps (dropped frames in decoder)
   * - droppedPics : indicates if decoder has dropped a picture
   *                 -1 means that decoder has no info on this.
   *
   * If codec does not implement this method, pts of decoded frame at input
   * video player is used. In case decoder does post-proc and de-interlacing there
   * may be quite some frames queued up between exit decoder and entry player.
   */
  virtual bool GetCodecStats(double &pts, int &droppedPics)
  {
    droppedPics= -1;
    return false;
  }

  /**
   * Codec can be informed by player with the following flags:
   *
   * DVD_CODEC_CTRL_NO_POSTPROC :
   *                  if speed is not normal the codec can switch off
   *                  postprocessing and de-interlacing
   *
   * DVD_CODEC_CTRL_DRAIN :
   *                  codecs may do postprocessing and de-interlacing.
   *                  If video buffers in RenderManager are about to run dry,
   *                  this is signaled to codec. Codec can wait for post-proc
   *                  to be finished instead of returning empty and getting another
   *                  packet.
   *
   */
  virtual void SetCodecControl(int flags) {}

  /*
    * Re-open the decoder.
    * Decoder request to re-open
    */
   virtual void Reopen() {};
};
