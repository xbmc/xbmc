/* 
 * $Id: IMPCVideoDecFilter.h 792 2008-09-12 20:07:43Z clsid2 $
 *
 * (C) 2006-2007 see AUTHORS
 *
 * This file is part of mplayerc.
 *
 * Mplayerc is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mplayerc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


#pragma once

// Internal codec list (use to enable/disable codec in standalone mode)
typedef enum 
{
  MPCVD_H264      = 1,
  MPCVD_VC1       = MPCVD_H264<<1,
  MPCVD_XVID      = MPCVD_VC1<<1,
  MPCVD_DIVX      = MPCVD_XVID<<1,
  MPCVD_WMV       = MPCVD_DIVX<<1,
  MPCVD_MSMPEG4   = MPCVD_WMV<<1,
  MPCVD_H263      = MPCVD_MSMPEG4<<1,
  MPCVD_SVQ3      = MPCVD_H263<<1,
  MPCVD_THEORA    = MPCVD_SVQ3<<1,
  MPCVD_AMVV      = MPCVD_THEORA<<1,
  MPCVD_FLASH     = MPCVD_AMVV<<1,
  MPCVD_H264_DXVA = MPCVD_FLASH<<1,
  MPCVD_VC1_DXVA  = MPCVD_H264_DXVA<<1,
  MPCVD_VP6       = MPCVD_VC1_DXVA<<1
} MPC_VIDEO_CODEC;

[uuid("CDC3B5B3-A8B0-4c70-A805-9FC80CDEF262")]
interface IMPCVideoDecFilter : public IUnknown
{
  STDMETHOD(Apply()) = 0;

  STDMETHOD(SetThreadNumber(int nValue)) = 0;
  STDMETHOD_(int, GetThreadNumber()) = 0;

  STDMETHOD(SetDiscardMode(int nValue)) = 0;
  STDMETHOD_(int, GetDiscardMode()) = 0;

  STDMETHOD(SetErrorRecognition(int nValue)) = 0;
  STDMETHOD_(int, GetErrorRecognition()) = 0;

  STDMETHOD(SetIDCTAlgo(int nValue)) = 0;
  STDMETHOD_(int, GetIDCTAlgo()) = 0;

  STDMETHOD_(GUID*, GetDXVADecoderGuid()) = 0;

  STDMETHOD(SetActiveCodecs(MPC_VIDEO_CODEC nValue)) = 0;
  STDMETHOD_(MPC_VIDEO_CODEC, GetActiveCodecs()) = 0;

  STDMETHOD_(LPCTSTR, GetVideoCardDescription()) = 0;

  STDMETHOD(SetARMode(int nValue)) = 0;
  STDMETHOD_(int, GetARMode()) = 0;
};
