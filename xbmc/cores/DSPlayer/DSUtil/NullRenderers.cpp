/* 
 *  Copyright (C) 2003-2006 Gabest
 *  http://www.gabest.org
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "StdAfx.h"
#include "NullRenderers.h"
#include <moreuuids.h>

//
// CNullRenderer
//

CNullRenderer::CNullRenderer(REFCLSID clsid, TCHAR* pName, LPUNKNOWN pUnk, HRESULT* phr) 
  : CDSBaseRenderer(clsid, pName, pUnk, phr)
{
}

//
// CNullVideoRenderer
//

CNullVideoRenderer::CNullVideoRenderer(LPUNKNOWN pUnk, HRESULT* phr) 
  : CNullRenderer(__uuidof(this), NAME("Null Video Renderer"), pUnk, phr)
{
}

HRESULT CNullVideoRenderer::CheckMediaType(const CMediaType* pmt)
{
  return pmt->majortype == MEDIATYPE_Video
    || pmt->subtype == MEDIASUBTYPE_MPEG2_VIDEO
    ? S_OK
    : E_FAIL;
}

//
// CNullUVideoRenderer
//

CNullUVideoRenderer::CNullUVideoRenderer(LPUNKNOWN pUnk, HRESULT* phr) 
  : CNullRenderer(__uuidof(this), NAME("Null Video Renderer (Uncompressed)"), pUnk, phr)
{
}

HRESULT CNullUVideoRenderer::CheckMediaType(const CMediaType* pmt)
{
  return pmt->majortype == MEDIATYPE_Video
    && (pmt->subtype == MEDIASUBTYPE_YV12
    || pmt->subtype == MEDIASUBTYPE_I420
    || pmt->subtype == MEDIASUBTYPE_YUYV
    || pmt->subtype == MEDIASUBTYPE_IYUV
    || pmt->subtype == MEDIASUBTYPE_YVU9
    || pmt->subtype == MEDIASUBTYPE_Y411
    || pmt->subtype == MEDIASUBTYPE_Y41P
    || pmt->subtype == MEDIASUBTYPE_YUY2
    || pmt->subtype == MEDIASUBTYPE_YVYU
    || pmt->subtype == MEDIASUBTYPE_UYVY
    || pmt->subtype == MEDIASUBTYPE_Y211
    || pmt->subtype == MEDIASUBTYPE_RGB1
    || pmt->subtype == MEDIASUBTYPE_RGB4
    || pmt->subtype == MEDIASUBTYPE_RGB8
    || pmt->subtype == MEDIASUBTYPE_RGB565
    || pmt->subtype == MEDIASUBTYPE_RGB555
    || pmt->subtype == MEDIASUBTYPE_RGB24
    || pmt->subtype == MEDIASUBTYPE_RGB32
    || pmt->subtype == MEDIASUBTYPE_ARGB1555
    || pmt->subtype == MEDIASUBTYPE_ARGB4444
    || pmt->subtype == MEDIASUBTYPE_ARGB32
    || pmt->subtype == MEDIASUBTYPE_A2R10G10B10
    || pmt->subtype == MEDIASUBTYPE_A2B10G10R10)
    ? S_OK
    : E_FAIL;
}

//
// CNullAudioRenderer
//

CNullAudioRenderer::CNullAudioRenderer(LPUNKNOWN pUnk, HRESULT* phr) 
  : CNullRenderer(__uuidof(this), NAME("Null Audio Renderer"), pUnk, phr)
{
}

HRESULT CNullAudioRenderer::CheckMediaType(const CMediaType* pmt)
{
  return pmt->majortype == MEDIATYPE_Audio
    || pmt->majortype == MEDIATYPE_Midi
    || pmt->subtype == MEDIASUBTYPE_MPEG2_AUDIO
    || pmt->subtype == MEDIASUBTYPE_DOLBY_AC3
    || pmt->subtype == MEDIASUBTYPE_DVD_LPCM_AUDIO
    || pmt->subtype == MEDIASUBTYPE_DTS
    || pmt->subtype == MEDIASUBTYPE_SDDS
    || pmt->subtype == MEDIASUBTYPE_MPEG1AudioPayload
    || pmt->subtype == MEDIASUBTYPE_MPEG1Audio
    || pmt->subtype == MEDIASUBTYPE_MPEG1Audio
    ? S_OK
    : E_FAIL;
}

//
// CNullUAudioRenderer
//

CNullUAudioRenderer::CNullUAudioRenderer(LPUNKNOWN pUnk, HRESULT* phr) 
  : CNullRenderer(__uuidof(this), NAME("Null Audio Renderer (Uncompressed)"), pUnk, phr)
{
}

HRESULT CNullUAudioRenderer::CheckMediaType(const CMediaType* pmt)
{
  return pmt->majortype == MEDIATYPE_Audio
    && (pmt->subtype == MEDIASUBTYPE_PCM
    || pmt->subtype == MEDIASUBTYPE_IEEE_FLOAT
    || pmt->subtype == MEDIASUBTYPE_DRM_Audio
    || pmt->subtype == MEDIASUBTYPE_DOLBY_AC3_SPDIF
    || pmt->subtype == MEDIASUBTYPE_RAW_SPORT
    || pmt->subtype == MEDIASUBTYPE_SPDIF_TAG_241h)
    ? S_OK
    : E_FAIL;
}

//
// CNullTextRenderer
//

HRESULT CNullTextRenderer::CTextInputPin::CheckMediaType(const CMediaType* pmt)
{
  return pmt->majortype == MEDIATYPE_Text
    || pmt->majortype == MEDIATYPE_ScriptCommand
    || pmt->majortype == MEDIATYPE_Subtitle 
    || pmt->subtype == MEDIASUBTYPE_DVD_SUBPICTURE 
    || pmt->subtype == MEDIASUBTYPE_CVD_SUBPICTURE 
    || pmt->subtype == MEDIASUBTYPE_SVCD_SUBPICTURE 
    ? S_OK 
    : E_FAIL;
}

#pragma warning (disable : 4355)
CNullTextRenderer::CNullTextRenderer(LPUNKNOWN pUnk, HRESULT* phr)
  : CBaseFilter(NAME("CNullTextRenderer"), pUnk, this, __uuidof(this), phr) 
{
  m_pInput.reset(new CTextInputPin(this, this, phr));
}
#pragma warning (default : 4355)
