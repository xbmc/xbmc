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

#if defined(HAVE_LIBCRYSTALHD)

#include <deque>

#include "DVDVideoCodec.h"
#include "cores/dvdplayer/DVDStreamInfo.h"
#include "threads/SingleLock.h"

////////////////////////////////////////////////////////////////////////////////////////////
template <class T>
class CSyncPtrQueue
{
public:
  CSyncPtrQueue() { }
  virtual ~CSyncPtrQueue() { }
  void Push(T* p)
  {
    CSingleLock lock(m_Lock);
    m_Queue.push_back(p);
  }

  T* Pop()
  {
    T* p = NULL;
    CSingleLock lock(m_Lock);
    if (m_Queue.size())
    {
      p = m_Queue.front();
      m_Queue.pop_front();
    }
    return p;
  }
  unsigned int Count(){return m_Queue.size();}
protected:
  std::deque<T*> m_Queue;
  CCriticalSection m_Lock;
};

////////////////////////////////////////////////////////////////////////////////////////////
class CPictureBuffer
{
public:
  CPictureBuffer(ERenderFormat format, int width, int height);
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
  ERenderFormat m_format;
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

#if (HAVE_LIBCRYSTALHD == 2)

  typedef struct _BC_INFO_CRYSTAL_ {
	  uint8_t device;
	  union {
		  struct {
			  uint32_t dilRelease:8;
			  uint32_t dilMajor:8;
			  uint32_t dilMinor:16;
		  };
		  uint32_t version;
	  } dilVersion;

	  union {
		  struct {
			  uint32_t drvRelease:4;
			  uint32_t drvMajor:8;
			  uint32_t drvMinor:12;
			  uint32_t drvBuild:8;
		  };
		  uint32_t version;
	  } drvVersion;

	  union {
		  struct {
			  uint32_t fwRelease:4;
			  uint32_t fwMajor:8;
			  uint32_t fwMinor:12;
			  uint32_t fwBuild:8;
		  };
		  uint32_t version;
	  } fwVersion;

	  uint32_t Reserved1; // For future expansion
	  uint32_t Reserved2; // For future expansion
  } BC_INFO_CRYSTAL, *PBC_INFO_CRYSTAL;

#endif

////////////////////////////////////////////////////////////////////////////////////////////

#define CRYSTALHD_FIELD_FULL        0x00
#define CRYSTALHD_FIELD_BOT         0x01
#define CRYSTALHD_FIELD_TOP         0x02

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
#if (HAVE_LIBCRYSTALHD == 2)
  BC_INFO_CRYSTAL m_bc_info_crystal;
#endif

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
