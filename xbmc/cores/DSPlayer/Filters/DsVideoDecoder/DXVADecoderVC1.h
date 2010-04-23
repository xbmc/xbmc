/* 
 *  Copyright (C) 2003-2006 Gabest
 *  http://www.gabest.org
 *
 *  Copyright (C) 2005-2010 Team XBMC
 *  http://www.xbmc.org
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


#pragma once

#include <dxva.h>
#include "DXVADecoder.h"

class CDXVADecoderVC1 :	public CDXVADecoder
{
public:
	CDXVADecoderVC1 (CXBMCVideoDecFilter* pFilter, IAMVideoAccelerator*  pAMVideoAccelerator, DXVAMode nMode, int nPicEntryNumber);
	CDXVADecoderVC1 (CXBMCVideoDecFilter* pFilter, IDirectXVideoDecoder* pDirectXVideoDec, DXVAMode nMode, int nPicEntryNumber, DXVA2_ConfigPictureDecode* pDXVA2Config);
	virtual ~CDXVADecoderVC1(void);

	// === Public functions
	virtual HRESULT DecodeFrame   (BYTE* pDataIn, UINT nSize, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop);
	virtual void	SetExtraData  (BYTE* pDataIn, UINT nSize);
	virtual void	CopyBitstream (BYTE* pDXVABuffer, BYTE* pBuffer, UINT& nSize);
	virtual void	Flush();

	typedef enum
	{
		VC1_PS_TOP_FIELD				= 1,
		VC1_PS_BOTTOM_FIELD				= 2,
		VC1_PS_PROGRESSIVE				= 3
	};

	typedef enum
	{
		VC1_CHROMA_420					= 1,
		VC1_CHROMA_422					= 2,
		VC1_CHROMA_444					= 3
	} VC1_CHROMA_FORMAT;

	typedef enum
	{
		VC1_CR_BICUBIC_QUARTER_CHROMA	= 4,
		VC1_CR_BICUBIC_HALF_CHROMA		= 5,
		VC1_CR_BILINEAR_QUARTER_CHROMA	= 12,
		VC1_CR_BILINEAR_HALF_CHROMA		= 13,
	};

	typedef enum
	{
		VC1_SCAN_ZIGZAG					= 0,
		VC1_SCAN_ALTERNATE_VERTICAL		= 1,
		VC1_SCAN_ALTERNATE_HORIZONTAL	= 2,
		VC1_SCAN_ARBITRARY				= 3		// Use when bConfigHostInverseScan = 1
	} VC1_PIC_SCAN_METHOD;

	typedef enum						// Values for bPicDeblockConfined when bConfigBitstreamRaw = 1
	{
		VC1_EXTENDED_DMV				= 0x0001,
		VC1_PSF							= 0x0002,
		VC1_REFPICFLAG					= 0x0004,
		VC1_FINTERPFLAG					= 0x0008,
		VC1_TFCNTRFLAG					= 0x0010,
		VC1_INTERLACE					= 0x0020,
		VC1_PULLDOWN					= 0x0040,
		VC1_POSTPROCFLAG				= 0x0080
	} VC1_DEBLOCK_CONFINED;

	typedef enum						// Values for bPicSpatialResid8
	{
		VC1_VSTRANSFORM					= 0x0001,
		VC1_DQUANT						= 0x0002,
		VC1_EXTENDED_MV					= 0x0004,
		VC1_FASTUVMC					= 0x0008,
		VC1_LOOPFILTER					= 0x0010,
		VC1_REDIST_FLAG					= 0x0020,
		VC1_PANSCAN_FLAG				= 0x0040,
	} VC1_PIC_SPATIAL_RESID8;


protected :

private:
	DXVA_PictureParameters		m_PictureParams;
	DXVA_SliceInfo				m_SliceInfo;
	WORD						m_wRefPictureIndex[2];

	int							m_nDelayedSurfaceIndex;
	REFERENCE_TIME				m_rtStartDelayed;
	REFERENCE_TIME				m_rtStopDelayed;

	// Private functions
	void					Init();
	HRESULT					DisplayStatus();
	BYTE*					FindNextStartCode(BYTE* pBuffer, UINT nSize, UINT& nPacketSize);

};
