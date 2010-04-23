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


typedef enum
{
	NALU_TYPE_SLICE    = 1,
	NALU_TYPE_DPA      = 2,
	NALU_TYPE_DPB      = 3,
	NALU_TYPE_DPC      = 4,
	NALU_TYPE_IDR      = 5,
	NALU_TYPE_SEI      = 6,
	NALU_TYPE_SPS      = 7,
	NALU_TYPE_PPS      = 8,
	NALU_TYPE_AUD      = 9,
	NALU_TYPE_EOSEQ    = 10,
	NALU_TYPE_EOSTREAM = 11,
	NALU_TYPE_FILL     = 12
} NALU_TYPE;


class CH264Nalu
{
private :
	int			forbidden_bit;      //! should be always FALSE
	int			nal_reference_idc;  //! NALU_PRIORITY_xxxx
	NALU_TYPE	nal_unit_type;      //! NALU_TYPE_xxxx    

	int			m_nNALStartPos;		//! NALU start (including startcode / size)
	int			m_nNALDataPos;		//! Useful part
	unsigned	m_nDataLen;			//! Length of the NAL unit (Excluding the start code, which does not belong to the NALU)

	BYTE*		m_pBuffer;
	int			m_nCurPos;
	int			m_nNextRTP;
	int			m_nSize;
	int			m_nNALSize;

	bool		MoveToNextStartcode();

public :
	NALU_TYPE	GetType()		{ return nal_unit_type; };
	bool		IsRefFrame()	{ return (nal_reference_idc != 0); };

	int			GetDataLength()	{ return m_nCurPos - m_nNALDataPos; };
	BYTE*		GetDataBuffer() { return m_pBuffer + m_nNALDataPos; };
	int			GetRoundedDataLength()
	{
		int		nSize = m_nCurPos - m_nNALDataPos;
		return nSize + 128 - (nSize %128);
	}

	int			GetLength()		{ return m_nCurPos - m_nNALStartPos; };
	BYTE*		GetNALBuffer()	{ return m_pBuffer + m_nNALStartPos; };
	bool		IsEOF()			{ return m_nCurPos >= m_nSize; };

	void		SetBuffer (BYTE* pBuffer, int nSize, int nNALSize);
	bool		ReadNext();
};
