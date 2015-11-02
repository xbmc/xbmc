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

 
#include "DVDVideoCodec.h"
#include "DVDStreamInfo.h"
#include "threads/Thread.h"
#include "TinyVideoStreamInfo.h"
#include "guilib/Geometry.h"
#include "cores/dvdplayer/DVDClock.h"
#include "rendering/RenderSystem.h"





class DllLibRKCodec;

#define PTS_FREQ        90000
#define UNIT_FREQ       96000

#define RK_DECODE_STATE_BUFFER          0
#define RK_DECODE_STATE_PICTURE         1
#define RK_DECODE_STATE_BUFFER_PICTURE  2
#define RK_DECODE_STATE_ERROR          -1

#define RK_3DMODE_NONE              -1
#define RK_3DMODE_FRAME_PACKING      0
#define RK_3DMODE_TOP_BOTTOM         6
#define RK_3DMODE_SIDE_BY_SIDE       8

extern int64_t RKAdjustLatency;

class CRKCodec : public CThread
{
public:
  
  CRKCodec();
  
  virtual       ~CRKCodec();
  
  bool          OpenDecoder(CDVDStreamInfo &hints);
  
  void          CloseDecoder();
  
  void          Reset();

  void          Flush();
  
  int           Decode(uint8_t *pData, size_t size, double dts, double pts);
  
  bool          GetPicture(DVDVideoPicture* pDvdVideoPicture);
  
  void          SetSpeed(int speed);

  void          SetDropState(bool b);
  
  int           GetDataSize();
  
  double        GetTimeSize();

  void          SetVideo3dMode(const int mode);

  void          Set23976Match(bool b);

  void          UpdateRenderRect(const CRect &SrcRect, const CRect &DestRect);

  static void   RenderUpdateCallBack(const void *ctx, const CRect &SrcRect, const CRect &DestRect);
  
protected:

  virtual void  Process();

  double        GetPlayerPtsSeconds();

  void          CheckVideoRate(CDVDStreamInfo &hints);

  int64_t       GetVideoPts();

  int64_t       SetVideoPts(int64_t  pts);
  
  DllLibRKCodec       *m_dll;

  bool                 m_opened;
  CDVDStreamInfo       m_hints;
  TinyVideoStreamInfo  m_info;
  int                  m_speed;
  double               m_timesize;
  unsigned int         video_rate;

  int64_t              m_1st_pts;
  int64_t              m_cur_pts;
  int64_t              m_last_pts;
  int64_t              m_old_pts;
  int64_t              m_cur_num;
  int64_t              m_old_num;
  int64_t              m_iAdjuestLatency;
  
  CRect                m_display_rect;
  bool                 m_bDrop;
  int                  m_i3DMode;
  RENDER_STEREO_MODE   m_user_stereo_mode;
  RENDER_STEREO_MODE   m_kodi_stereo_mode;
  bool                 m_bMVC3D;
  int64_t              m_1st_adjust;
  int64_t              m_last_adjust;
  int64_t              m_cur_adjust;
  
};



