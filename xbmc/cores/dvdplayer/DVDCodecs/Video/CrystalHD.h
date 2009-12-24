#pragma once
/*
 *      Copyright (C) 2005-2009 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#if defined(HAVE_LIBCRYSTALHD)

#include "DVDVideoCodec.h"

/* We really don't want to include ffmpeg headers, so define these */
enum _BCM_CODEC_TYPES
{
  BC_CODEC_ID_MPEG2 = 2,
  BC_CODEC_ID_H264  = 28,
  BC_CODEC_ID_VC1   = 73,
};
enum _BCM_STREAM_TYPE
{
	BC_STREAM_TYPE_ES         = 0,
	BC_STREAM_TYPE_PES        = 1,
	BC_STREAM_TYPE_TS         = 2,
	BC_STREAM_TYPE_ES_TSTAMP	= 6,
};

typedef uint32_t BCM_CODEC_TYPE;
typedef uint32_t BCM_STREAM_TYPE;
////////////////////////////////////////////////////////////////////////////////////////////

#define CRYSTALHD_FIELD_FULL        0x00
#define CRYSTALHD_FIELD_EVEN        0x01
#define CRYSTALHD_FIELD_ODD         0x02

class CCrystalHD
{
public:
  virtual ~CCrystalHD();

  static void RemoveInstance(void);
  static CCrystalHD* GetInstance(void);

  bool Open(BCM_STREAM_TYPE stream_type, BCM_CODEC_TYPE codec_type);
  void Close(void);
  bool IsOpenforDecode(void);
  void Flush(void);
  unsigned int GetInputCount(void);
  bool AddInput(unsigned char *pData, size_t size, double pts);
  bool GotPicture(int* field);
  bool GetPicture(DVDVideoPicture* pDvdVideoPicture);
  void SetDropState(bool bDrop);

protected:
  void SetFrameRate(uint32_t resolution);
  void SetAspectRatio(uint32_t aspect_ratio, uint32_t custom_aspect_ratio_width_height);
  bool GetDecoderOutput(void);
  
  void          *m_dl_handle;
  void          *m_Device;
  
  bool          m_IsConfigured;
  bool          m_drop_state;
  const char    *m_pFormatName;
  unsigned int  m_OutputTimeout;
  unsigned int  m_field;
  unsigned int  m_width;
  unsigned int  m_height;
  bool          m_interlace;
  double        m_framerate;
  uint64_t      m_timestamp;
  unsigned int  m_PictureNumber;
  unsigned int  m_aspectratio_x;
  unsigned int  m_aspectratio_y;
  unsigned char *m_y_buffer_ptr;
  unsigned char *m_uv_buffer_ptr;
  int           m_y_buffer_size;
  int           m_uv_buffer_size;

private:
  CCrystalHD();
  static CCrystalHD *m_pInstance;
};

#endif
