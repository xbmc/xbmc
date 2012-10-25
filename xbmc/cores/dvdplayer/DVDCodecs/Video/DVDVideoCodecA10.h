#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "guilib/Geometry.h"
#include "DVDVideoCodec.h"
#include "DVDStreamInfo.h"

extern "C" {
#include "libcedarv.h"
};

#define DISPQS 10

class CDVDVideoCodecA10;

struct A10VideoBuffer {
  CDVDVideoCodecA10 *codec;
  int                decnr;
  cedarv_picture_t   picture;
};

class CDVDVideoCodecA10 : public CDVDVideoCodec
{
public:

  CDVDVideoCodecA10();
  virtual ~CDVDVideoCodecA10();

  /*
   * Open the decoder, returns true on success
   */
  bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);

  /*
   * Dispose, Free all resources
   */
  void Dispose();

  /*
   * returns one or a combination of VC_ messages
   * pData and iSize can be NULL, this means we should flush the rest of the data.
   */
  int Decode(BYTE* pData, int iSize, double dts, double pts);

 /*
   * Reset the decoder.
   * Should be the same as calling Dispose and Open after each other
   */
  void Reset();

  /*
   * returns true if successfull
   * the data is valid until the next Decode call
   */
  bool GetPicture(DVDVideoPicture* pDvdVideoPicture);


  /*
   * returns true if successfull
   * the data is cleared to zero
   */
  /*-->super
  bool ClearPicture(DVDVideoPicture* pDvdVideoPicture);
  */

  /*
   * returns true if successfull
   * the data is valid until the next Decode call
   * userdata can be anything, for now we use it for closed captioning
   */
  /*-->super
  bool GetUserData(DVDVideoUserData* pDvdVideoUserData);
  */

  /*
   * will be called by video player indicating if a frame will eventually be dropped
   * codec can then skip actually decoding the data, just consume the data set picture headers
   */
  void SetDropState(bool bDrop);

  /*
   * returns the number of demuxer bytes in any internal buffers
   */
  /*-->super
  int GetDataSize(void);
  */

  /*
   * returns the time in seconds for demuxer bytes in any internal buffers
   */
  /*-->super
  virtual double GetTimeSize(void);
  */

  /*
   * set the type of filters that should be applied at decoding stage if possible
   */
  /*-->super
  unsigned int SetFilters(unsigned int filters);
  */

  /*
   *
   * should return codecs name
   */
  const char* GetName();

  /*
   *
   * How many packets should player remember, so codec
   * can recover should something cause it to flush
   * outside of players control
   */
  /*-->super
  virtual unsigned GetConvergeCount();
  */

  void RenderBuffer(A10VideoBuffer *buffer, CRect &srcRect, CRect &dstRect);

private:

  bool DoOpen();

  typedef struct ScalerParameter {
    int width_in;
    int height_in;
    int width_out;
    int height_out;
    u32 addr_y_in;
    u32 addr_c_in;
    u32 addr_y_out;
    u32 addr_u_out;
    u32 addr_v_out;
  } ScalerParameter;

  bool HardwarePictureScaler(ScalerParameter *cdx_scaler_para);
  bool disp_open();
  void disp_close();
  bool scaler_open();
  void scaler_close();

  //disp
  int                   m_hdisp;
  int                   m_scrid;

  //decoding
  cedarv_stream_info_t  m_info;
  float                 m_aspect;
  CDVDStreamInfo        m_hints;
  cedarv_decoder_t     *m_hcedarv;
  int                   m_hscaler;
  u8                   *m_yuvdata;
  DVDVideoPicture       m_picture;

  //rendering
  bool                  m_hwrender;
  int                   m_hlayer;
  int                   m_prevnr; //last dvdplayer render
  bool                  m_firstframe;

  int                   m_decnr;
  int                   m_lastnr; //last display nr
  int                   m_wridx;
  int                   m_rdidx;
  A10VideoBuffer        m_dispq[DISPQS];
  pthread_mutex_t       m_dispq_mutex;
};

extern void A10Render(A10VideoBuffer *buffer,CRect &srcRect, CRect &dstRect);

