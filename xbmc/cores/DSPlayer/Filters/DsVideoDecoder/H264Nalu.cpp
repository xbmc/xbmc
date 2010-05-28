/* 
 *  Copyright (C) 2006-2007 Gabest
 *  http://www.gabest.org
 *
 *  Copyright (C) 2007-2010 Team XBMC
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

#ifdef HAS_DS_PLAYER

#include "H264Nalu.h"

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

void CH264Nalu::SetBuffer(BYTE* pBuffer, int nSize, int nNALSize)
{
	m_pBuffer		= pBuffer;
	m_nSize			= nSize;
	m_nNALSize		= nNALSize;
	m_nCurPos		= 0;
	m_nNextRTP		= 0;

	m_nNALStartPos	= 0;
	m_nNALDataPos	= 0;
}

bool CH264Nalu::MoveToNextStartcode()
{
	int		nBuffEnd = (m_nNextRTP > 0) ? min (m_nNextRTP, m_nSize-4) : m_nSize-4;

	for (int i=m_nCurPos; i<nBuffEnd; i++)
	{
		if ((*((DWORD*)(m_pBuffer+i)) & 0x00FFFFFF) == 0x00010000)
		{
			// Find next AnnexB Nal
			m_nCurPos = i;
			return true;
		}
	}

	if ((m_nNALSize != 0) && (m_nNextRTP < m_nSize))
	{
		m_nCurPos = m_nNextRTP;
		return true;
	}

	m_nCurPos = m_nSize;
	return false;
}

bool CH264Nalu::ReadNext()
{
	int		nTemp;

	if (m_nCurPos >= m_nSize) return false;

	if ((m_nNALSize != 0) && (m_nCurPos == m_nNextRTP))
	{
		// RTP Nalu type : (XX XX) XX XX NAL..., with XX XX XX XX or XX XX equal to NAL size
		m_nNALStartPos	= m_nCurPos;
		m_nNALDataPos	= m_nCurPos + m_nNALSize;
		nTemp			= 0;
		for (int i=0; i<m_nNALSize; i++)
		{
			nTemp = (nTemp << 8) + m_pBuffer[m_nCurPos++];
		}
		m_nNextRTP += nTemp + m_nNALSize;
		MoveToNextStartcode();
	}
	else
	{
		// Remove trailing bits
		while (m_pBuffer[m_nCurPos]==0x00 && ((*((DWORD*)(m_pBuffer+m_nCurPos)) & 0x00FFFFFF) != 0x00010000))
			m_nCurPos++;

		// AnnexB Nalu : 00 00 01 NAL...
		m_nNALStartPos	= m_nCurPos;
		m_nCurPos	   += 3;
		m_nNALDataPos	= m_nCurPos;
		MoveToNextStartcode();
	}

	forbidden_bit		= (m_pBuffer[m_nNALDataPos]>>7) & 1;
	nal_reference_idc	= (m_pBuffer[m_nNALDataPos]>>5) & 3;
	nal_unit_type		= (NALU_TYPE) (m_pBuffer[m_nNALDataPos] & 0x1f);

	return true;
}

#endif