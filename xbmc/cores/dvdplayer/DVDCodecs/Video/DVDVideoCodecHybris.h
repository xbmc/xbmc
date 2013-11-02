#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

#include "DVDVideoCodec.h"
#include "DVDResource.h"
#include "DVDStreamInfo.h"
#include "utils/BitstreamConverter.h"
#include <string>
#include <queue>
#include <list>
#include "guilib/GraphicContext.h"

#include <hybris/media/media_compatibility_layer.h>
#include <hybris/media/media_codec_layer.h>

#define STREAM_BUFFER_SIZE            786432 //compressed frame size. 1080p mpeg4 10Mb/s can be un to 786k in size, so this is to make sure frame fits into buffer

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

typedef struct amc_demux {
  uint8_t *pData;
  int iSize;
  double dts;
  double pts;
} amc_demux;

class CDVDVideoCodecHybris : public CDVDVideoCodec
{
public:
  CDVDVideoCodecHybris();
  virtual ~CDVDVideoCodecHybris();
  virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);
  virtual void Dispose();
  virtual int Decode(BYTE* pData, int iSize, double dts, double pts);
  virtual void Reset();
  bool GetPictureCommon(DVDVideoPicture* pDvdVideoPicture);
  virtual bool GetPicture(DVDVideoPicture* pDvdVideoPicture);
  virtual bool ClearPicture(DVDVideoPicture* pDvdVideoPicture);
  virtual void SetDropState(bool bDrop);
  virtual const char* GetName() { return m_name.c_str(); }; // m_name is never changed after open

protected:
  int GetOutputPicture(void);
  void OutputFormatChanged(void);

  std::string m_name;
  unsigned int m_iDecodedWidth;
  unsigned int m_iDecodedHeight;
  unsigned int m_iConvertedWidth;
  unsigned int m_iConvertedHeight;

  CBitstreamConverter *m_bitstream;

  std::priority_queue<double> m_pts;
  std::priority_queue<double> m_dts;

  bool m_bDropPictures;
  bool m_bVideoConvert;

  std::string m_mimeType;
  MediaCodecDelegate m_codec;
  MediaFormat m_format;
  CDVDStreamInfo m_hints;


  std::queue<amc_demux> m_demux;

  bool m_opened;
  bool m_drop;

  DVDVideoPicture m_videoBuffer;
  int m_src_offset[4];
  int m_src_stride[4];

  bool OpenDevices();
};

