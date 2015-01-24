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

#include <deque>

#include "DVDVideoCodec.h"
#include "DVDStreamInfo.h"

////////////////////////////////////////////////////////////////////////////////////////////
template <class T>
class CSyncPtrQueue
{
public:
  CSyncPtrQueue()
  {
    InitializeCriticalSection(&m_Lock);
  }
  virtual ~CSyncPtrQueue()
  {
    DeleteCriticalSection(&m_Lock);
  }
  void Push(T* p)
  {
    EnterCriticalSection(&m_Lock);
    m_Queue.push_back(p);
    LeaveCriticalSection(&m_Lock);
  }
  T* Pop()
  {
    T* p = NULL;
    EnterCriticalSection(&m_Lock);
    if (m_Queue.size())
    {
      p = m_Queue.front();
      m_Queue.pop_front();
    }
    LeaveCriticalSection(&m_Lock);
    return p;
  }
  unsigned int Count(){return m_Queue.size();}
protected:
  std::deque<T*> m_Queue;
  CRITICAL_SECTION m_Lock;
};

////////////////////////////////////////////////////////////////////////////////////////////
class CPictureBuffer
{
public:
  CPictureBuffer(DVDVideoPicture::EFormat format, int width, int height);
  virtual ~CPictureBuffer();

  unsigned int  m_width;
  unsigned int  m_height;
  unsigned int  m_field;
  bool          m_interlace;
  double        m_framerate;
  uint64_t      m_timestamp;
  int           m_color_space;
  unsigned int  m_color_range;
  unsigned int  m_color_matrix;
  uint64_t      m_PictureNumber;
  DVDVideoPicture::EFormat m_format;
  unsigned char *m_y_buffer_ptr;
  unsigned char *m_u_buffer_ptr;
  unsigned char *m_v_buffer_ptr;
  unsigned char *m_uv_buffer_ptr;
  int           m_y_buffer_size;
  int           m_u_buffer_size;
  int           m_v_buffer_size;
  int           m_uv_buffer_size;
};


////////////////////////////////////////////////////////////////////////////////////////////
enum _CRYSTALHD_CODEC_TYPES
{
  CRYSTALHD_CODEC_ID_MPEG2 = 0,
  CRYSTALHD_CODEC_ID_H264  = 1,
  CRYSTALHD_CODEC_ID_AVC1  = 2,
  CRYSTALHD_CODEC_ID_VC1   = 3,
  CRYSTALHD_CODEC_ID_WMV3  = 4,
  CRYSTALHD_CODEC_ID_WVC1  = 5,
};

typedef uint32_t CRYSTALHD_CODEC_TYPE;
////////////////////////////////////////////////////////////////////////////////////////////

#define CRYSTALHD_FIELD_FULL        0x00
#define CRYSTALHD_FIELD_EVEN        0x01
#define CRYSTALHD_FIELD_ODD         0x02

typedef struct CHD_CODEC_PARAMS {
  uint8_t   *sps_pps_buf;
  uint32_t  sps_pps_size;
  uint8_t   nal_size_bytes;
} CHD_CODEC_PARAMS;

class DllLibCrystalHD;
class CMPCInputThread;
class CMPCOutputThread;

class CCrystalHD
{
public:
  virtual ~CCrystalHD();

  static void RemoveInstance(void);
  static CCrystalHD* GetInstance(void);

  bool DevicePresent(void);

  void OpenDevice();
  void CloseDevice();

  bool OpenDecoder(CRYSTALHD_CODEC_TYPE codec_type, CDVDStreamInfo &hints);
  void CloseDecoder(void);
  void Reset(void);

  bool AddInput(unsigned char *pData, size_t size, double dts, double pts);

  int  GetReadyCount(void);
  void BusyListFlush(void);

  bool GetPicture(DVDVideoPicture* pDvdVideoPicture);
  void SetDropState(bool bDrop);

protected:

  DllLibCrystalHD *m_dll;
  void          *m_device;
  bool          m_device_preset;
  bool          m_new_lib;
  bool          m_decoder_open;
  bool          m_has_bcm70015;
  int           m_color_space;
  bool          m_drop_state;
  bool          m_skip_state;
  unsigned int  m_timeout;
  unsigned int  m_wait_timeout;
  unsigned int  m_field;
  unsigned int  m_width;
  unsigned int  m_height;
  int           m_reset;
  int           m_last_pict_num;
  double        m_last_demuxer_pts;
  double        m_last_decoder_pts;

  CMPCOutputThread *m_pOutputThread;
  CSyncPtrQueue<CPictureBuffer> m_BusyList;

private:
  CCrystalHD();
  static CCrystalHD *m_pInstance;

  // bitstream to bytestream (Annex B) conversion support.
  bool bitstream_convert_init(void *in_extradata, int in_extrasize);
  bool bitstream_convert(uint8_t* pData, int iSize, uint8_t **poutbuf, int *poutbuf_size);
  void bitstream_alloc_and_copy( uint8_t **poutbuf, int *poutbuf_size,
    const uint8_t *sps_pps, uint32_t sps_pps_size, const uint8_t *in, uint32_t in_size);

  typedef struct chd_bitstream_ctx {
      uint8_t  length_size;
      uint8_t  first_idr;
      uint8_t *sps_pps_data;
      uint32_t size;
  } chd_bitstream_ctx;

  uint32_t          m_sps_pps_size;
  chd_bitstream_ctx m_sps_pps_context;
  bool              m_convert_bitstream;

  bool extract_sps_pps_from_avcc(int extradata_size, void *extradata);
  CHD_CODEC_PARAMS  m_chd_params;
};

#endif
