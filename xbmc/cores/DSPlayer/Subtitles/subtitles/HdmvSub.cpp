/* 
 * $Id: HdmvSub.cpp 1048 2009-04-18 17:39:19Z casimir666 $
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

#include "StdAfx.h"
#include "HdmvSub.h"
#include "..\DSUtil\GolombBuffer.h"

#if (0)		// Set to 1 to activate HDMV subtitles traces
	#define TRACE_HDMVSUB		TRACE
#else
	#define TRACE_HDMVSUB
#endif


CHdmvSub::CHdmvSub(void)
{
	m_nColorNumber				= 0;

	m_nCurSegment				= NO_SEGMENT;
	m_pSegBuffer				= NULL;
	m_nTotalSegBuffer			= 0;
	m_nSegBufferPos				= 0;
	m_nSegSize					= 0;
	m_pCurrentObject			= NULL;
	m_pDefaultPalette			= NULL;
	m_nDefaultPaletteNbEntry	= 0;

	memset (&m_VideoDescriptor, 0, sizeof(VIDEO_DESCRIPTOR));
}

CHdmvSub::~CHdmvSub()
{
	while (! m_pObjects.empty())
	{
    delete m_pObjects.back();
    m_pObjects.pop_back();
	}

	delete[] m_pSegBuffer;
	delete[] m_pDefaultPalette;
	delete m_pCurrentObject;
}


void CHdmvSub::AllocSegment(int nSize)
{
	if (nSize > m_nTotalSegBuffer)
	{
		delete[] m_pSegBuffer;
		m_pSegBuffer		= DNew BYTE[nSize];
		m_nTotalSegBuffer	= nSize;
	}
	m_nSegBufferPos	 = 0;
	m_nSegSize       = nSize;
}

__w64 int CHdmvSub::GetStartPosition(REFERENCE_TIME rt, double fps)
{
	CompositionObject*	pObject;

	// Cleanup old PG
  int i = 0;
	while (m_pObjects.size() > 0)
	{
		pObject = m_pObjects.front();
		if (pObject->m_rtStop < rt)
		{
			TRACE_HDMVSUB ("CHdmvSub:HDMV remove object %d  %S => %S (rt=%S)\n", pObject->GetRLEDataSize(), 
						   ReftimeToString (pObject->m_rtStart), ReftimeToString(pObject->m_rtStop), ReftimeToString(rt));
			m_pObjects.pop_front();
      i++;
			delete pObject;
		}
		else
			break;
	}

	return i; 
}

HRESULT CHdmvSub::ParseSample(IMediaSample* pSample)
{
	CheckPointer (pSample, E_POINTER);
	HRESULT				hr;
	REFERENCE_TIME		rtStart = INVALID_TIME, rtStop = INVALID_TIME;
	BYTE*				pData = NULL;
	int					lSampleLen;

    hr = pSample->GetPointer(&pData);
    if(FAILED(hr) || pData == NULL) return hr;
	lSampleLen = pSample->GetActualDataLength();

	pSample->GetTime(&rtStart, &rtStop);
	if (pData)
	{
		CGolombBuffer		SampleBuffer (pData, lSampleLen);
		if (m_nCurSegment == NO_SEGMENT)
		{
			HDMV_SEGMENT_TYPE	nSegType	= (HDMV_SEGMENT_TYPE)SampleBuffer.ReadByte();
			USHORT				nUnitSize	= SampleBuffer.ReadShort();
			lSampleLen -=3;

			switch (nSegType)
			{
			case PALETTE :
			case OBJECT :
			case PRESENTATION_SEG :
			case END_OF_DISPLAY :
				m_nCurSegment = nSegType;
				AllocSegment (nUnitSize);
				break;
			default :
				return VFW_E_SAMPLE_REJECTED;
			}
		}

		if (m_nCurSegment != NO_SEGMENT)
		{
			if (m_nSegBufferPos < m_nSegSize)
			{
				int		nSize = min (m_nSegSize-m_nSegBufferPos, lSampleLen);
				SampleBuffer.ReadBuffer (m_pSegBuffer+m_nSegBufferPos, nSize);
				m_nSegBufferPos += nSize;
			}
			
			if (m_nSegBufferPos >= m_nSegSize)
			{
				CGolombBuffer	SegmentBuffer (m_pSegBuffer, m_nSegSize);

				switch (m_nCurSegment)
				{
				case PALETTE :
					TRACE_HDMVSUB ("CHdmvSub:PALETTE            rtStart=%10I64d\n", rtStart);
					ParsePalette(&SegmentBuffer, m_nSegSize);
					break;
				case OBJECT :
					//TRACE_HDMVSUB ("CHdmvSub:OBJECT             %S\n", ReftimeToString(rtStart));
					ParseObject(&SegmentBuffer, m_nSegSize);
					break;
				case PRESENTATION_SEG :
					TRACE_HDMVSUB ("CHdmvSub:PRESENTATION_SEG   %S (size=%d)\n", ReftimeToString(rtStart), m_nSegSize);
					
					if (m_pCurrentObject)
					{
						m_pCurrentObject->m_rtStop = rtStart;
						m_pObjects.push_back (m_pCurrentObject);
						TRACE_HDMVSUB ("CHdmvSub:HDMV : %S => %S\n", ReftimeToString (m_pCurrentObject->m_rtStart), ReftimeToString(rtStart));
						m_pCurrentObject = NULL;
					}

					if (ParsePresentationSegment(&SegmentBuffer) > 0)
					{
						m_pCurrentObject->m_rtStart	= rtStart;
						m_pCurrentObject->m_rtStop	= _I64_MAX;
					}
					break;
				case WINDOW_DEF :
					// TRACE_HDMVSUB ("CHdmvSub:WINDOW_DEF         %S\n", ReftimeToString(rtStart));
					break;
				case END_OF_DISPLAY :
					// TRACE_HDMVSUB ("CHdmvSub:END_OF_DISPLAY     %S\n", ReftimeToString(rtStart));
					break;
				default :
					TRACE_HDMVSUB ("CHdmvSub:UNKNOWN Seg %d     rtStart=0x%10dd\n", m_nCurSegment, rtStart);
				}

				m_nCurSegment = NO_SEGMENT;
				return hr;
			}
		}
	}

	return hr;
}

int CHdmvSub::ParsePresentationSegment(CGolombBuffer* pGBuffer)
{
	COMPOSITION_DESCRIPTOR	CompositionDescriptor;
	BYTE					nObjectNumber;
	bool					palette_update_flag;
	BYTE					palette_id_ref;

	ParseVideoDescriptor(pGBuffer, &m_VideoDescriptor);
	ParseCompositionDescriptor(pGBuffer, &CompositionDescriptor);
	palette_update_flag	= !!(pGBuffer->ReadByte() & 0x80);
	palette_id_ref		= pGBuffer->ReadByte();
	nObjectNumber		= pGBuffer->ReadByte();

	if (nObjectNumber > 0)
	{
		delete m_pCurrentObject;
		m_pCurrentObject = DNew CompositionObject();
		ParseCompositionObject (pGBuffer, m_pCurrentObject);
	}

	return nObjectNumber;
}

void CHdmvSub::ParsePalette(CGolombBuffer* pGBuffer, USHORT nSize)		// #497
{
	int		nNbEntry;
	BYTE	palette_id				= pGBuffer->ReadByte();
	BYTE	palette_version_number	= pGBuffer->ReadByte();

	ASSERT ((nSize-2) % sizeof(HDMV_PALETTE) == 0);
	nNbEntry = (nSize-2) / sizeof(HDMV_PALETTE);
	HDMV_PALETTE*	pPalette = (HDMV_PALETTE*)pGBuffer->GetBufferPos();

	if (m_pDefaultPalette == NULL || m_nDefaultPaletteNbEntry != nNbEntry)
	{
		delete[] m_pDefaultPalette;
		m_pDefaultPalette		 = new HDMV_PALETTE[nNbEntry];
		m_nDefaultPaletteNbEntry = nNbEntry;
	}
	memcpy (m_pDefaultPalette, pPalette, nNbEntry*sizeof(HDMV_PALETTE));

	if (m_pCurrentObject)
		m_pCurrentObject->SetPalette (nNbEntry, pPalette, m_VideoDescriptor.nVideoWidth>720);
}

void CHdmvSub::ParseObject(CGolombBuffer* pGBuffer, USHORT nUnitSize)	// #498
{
	SHORT	object_id	= pGBuffer->ReadShort();
	BYTE	m_sequence_desc;

	ASSERT (m_pCurrentObject != NULL);
	if (m_pCurrentObject)// && m_pCurrentObject->m_object_id_ref == object_id)
	{
		m_pCurrentObject->m_version_number	= pGBuffer->ReadByte();
		m_sequence_desc						= pGBuffer->ReadByte();

		if (m_sequence_desc & 0x80)
		{
			DWORD	object_data_length  = (DWORD)pGBuffer->BitRead(24);
			
			m_pCurrentObject->m_width			= pGBuffer->ReadShort();
			m_pCurrentObject->m_height 			= pGBuffer->ReadShort();

			m_pCurrentObject->SetRLEData (pGBuffer->GetBufferPos(), nUnitSize-11, object_data_length-4);

			TRACE_HDMVSUB ("CHdmvSub:NewObject	size=%ld, total obj=%d, %dx%d\n", object_data_length, m_pObjects.size(),
						   m_pCurrentObject->m_width, m_pCurrentObject->m_height);
		}
		else
			m_pCurrentObject->AppendRLEData (pGBuffer->GetBufferPos(), nUnitSize-4);
	}
}

void CHdmvSub::ParseCompositionObject(CGolombBuffer* pGBuffer, CompositionObject* pCompositionObject)
{
	BYTE	bTemp;
	pCompositionObject->m_object_id_ref	= pGBuffer->ReadShort();
	pCompositionObject->m_window_id_ref	= pGBuffer->ReadByte();
	bTemp = pGBuffer->ReadByte();
	pCompositionObject->m_object_cropped_flag	= !!(bTemp & 0x80);
	pCompositionObject->m_forced_on_flag		= !!(bTemp & 0x40);
	pCompositionObject->m_horizontal_position	= pGBuffer->ReadShort();
	pCompositionObject->m_vertical_position		= pGBuffer->ReadShort();

	if (pCompositionObject->m_object_cropped_flag)
	{
		pCompositionObject->m_cropping_horizontal_position	= pGBuffer->ReadShort();
		pCompositionObject->m_cropping_vertical_position	= pGBuffer->ReadShort();
		pCompositionObject->m_cropping_width				= pGBuffer->ReadShort();
		pCompositionObject->m_cropping_height				= pGBuffer->ReadShort();
	}
}

void CHdmvSub::ParseVideoDescriptor(CGolombBuffer* pGBuffer, VIDEO_DESCRIPTOR* pVideoDescriptor)
{
	pVideoDescriptor->nVideoWidth   = pGBuffer->ReadShort();
	pVideoDescriptor->nVideoHeight  = pGBuffer->ReadShort();
	pVideoDescriptor->bFrameRate	= pGBuffer->ReadByte();
}

void CHdmvSub::ParseCompositionDescriptor(CGolombBuffer* pGBuffer, COMPOSITION_DESCRIPTOR* pCompositionDescriptor)
{
	pCompositionDescriptor->nNumber	= pGBuffer->ReadShort();
	pCompositionDescriptor->bState	= pGBuffer->ReadByte();
}

void CHdmvSub::Render(SubPicDesc& spd, REFERENCE_TIME rt, RECT& bbox)
{
	CompositionObject*	pObject = FindObject (rt);

	ASSERT (pObject!=NULL && spd.w >= pObject->m_width && spd.h >= pObject->m_height);

	if (pObject && spd.w >= pObject->m_width && spd.h >= pObject->m_height)
	{
		if (!pObject->HavePalette())
			pObject->SetPalette (m_nDefaultPaletteNbEntry, m_pDefaultPalette, m_VideoDescriptor.nVideoWidth>720);

		TRACE_HDMVSUB ("CHdmvSub:Render	    size=%ld,  ObjRes=%dx%d,  SPDRes=%dx%d\n", pObject->GetRLEDataSize(), 
					   pObject->m_width, pObject->m_height, spd.w, spd.h);
		pObject->Render(spd);

		bbox.left	= 0;
		bbox.top	= 0;
		bbox.right	= bbox.left + pObject->m_width;
		bbox.bottom	= bbox.top  + pObject->m_height;
	}
}

HRESULT CHdmvSub::GetTextureSize (int pos, SIZE& MaxTextureSize, SIZE& VideoSize, POINT& VideoTopLeft)
{
  std::list<CompositionObject *>::iterator it = m_pObjects.begin();
  std::advance(it, pos);
	CompositionObject*	pObject = *it;
	if (pObject)
	{
		// Texture size should be video size width. Height is limited (to prevent performances issues with
		// more than 1024x768 pixels)
		MaxTextureSize.cx	= min (m_VideoDescriptor.nVideoWidth, 1920);
		MaxTextureSize.cy	= min (m_VideoDescriptor.nVideoHeight, 1024*768/MaxTextureSize.cx);

		VideoSize.cx	= m_VideoDescriptor.nVideoWidth;
		VideoSize.cy	= m_VideoDescriptor.nVideoHeight;

		VideoTopLeft.x	= pObject->m_horizontal_position;
		VideoTopLeft.y	= pObject->m_vertical_position;

		return S_OK;
	}

	ASSERT (FALSE);
	return E_INVALIDARG;
}


void CHdmvSub::Reset()
{
	CompositionObject*	pObject;
	while (! m_pObjects.empty())
	{
    delete m_pObjects.back();
    m_pObjects.pop_back();
	}
}

CHdmvSub::CompositionObject*	CHdmvSub::FindObject(REFERENCE_TIME rt)
{
  std::list<CompositionObject *>::const_iterator it = m_pObjects.begin();
	for(; it != m_pObjects.end(); ++it)
	{
		CompositionObject*	pObject = *it;

		if (rt >= pObject->m_rtStart && rt < pObject->m_rtStop)
			return pObject;

	}

	return NULL;
}


// ===== CHdmvSub::CompositionObject

CHdmvSub::CompositionObject::CompositionObject()
{
	m_rtStart		= 0;
	m_rtStop		= 0;
	m_pRLEData		= NULL;
	m_nRLEDataSize	= 0;
	m_nRLEPos		= 0;
	m_nColorNumber	= 0;
	memsetd (m_Colors, 0xFF000000, sizeof(m_Colors));
}

CHdmvSub::CompositionObject::~CompositionObject()
{
	delete[] m_pRLEData;
}

void CHdmvSub::CompositionObject::SetPalette (int nNbEntry, HDMV_PALETTE* pPalette, bool bIsHD)
{
	m_nColorNumber	= nNbEntry;

	for (int i=0; i<m_nColorNumber; i++)
	{
		if (pPalette[i].T != 0)	// Prevent ugly background when Alpha=0 (but RGB different from 0)
		{
			if (bIsHD)
				m_Colors[pPalette[i].entry_id] = YCrCbToRGB_Rec709 (255-pPalette[i].T, pPalette[i].Y, pPalette[i].Cr, pPalette[i].Cb);
			else
				m_Colors[pPalette[i].entry_id] = YCrCbToRGB_Rec601 (255-pPalette[i].T, pPalette[i].Y, pPalette[i].Cr, pPalette[i].Cb);
		}
//		TRACE_HDMVSUB ("%03d : %08x\n", pPalette[i].entry_id, m_Colors[pPalette[i].entry_id]);
	}
}


void CHdmvSub::CompositionObject::SetRLEData(BYTE* pBuffer, int nSize, int nTotalSize)
{
	delete[] m_pRLEData;
	m_pRLEData		= DNew BYTE[nTotalSize];
	m_nRLEDataSize	= nTotalSize;
	m_nRLEPos		= nSize;

	memcpy (m_pRLEData, pBuffer, nSize);
}

void CHdmvSub::CompositionObject::AppendRLEData(BYTE* pBuffer, int nSize)
{
	ASSERT (m_nRLEPos+nSize <= m_nRLEDataSize);
	if (m_nRLEPos+nSize <= m_nRLEDataSize)
	{
		memcpy (m_pRLEData+m_nRLEPos, pBuffer, nSize);
		m_nRLEPos += nSize;
	}
}

void CHdmvSub::CompositionObject::Render(SubPicDesc& spd)
{
	if (m_pRLEData)
	{
		CGolombBuffer	GBuffer (m_pRLEData, m_nRLEDataSize);
		BYTE			bTemp;
		BYTE			bSwitch;
		bool			bEndOfLine = false;

		BYTE			nPaletteIndex;
		SHORT			nCount;
		SHORT			nX	= 0;
		SHORT			nY	= 0;

		while ((nY < m_height) && !GBuffer.IsEOF())
		{
			bTemp = GBuffer.ReadByte();
			if (bTemp != 0)
			{
				nPaletteIndex = bTemp;
				nCount		  = 1;
			}
			else
			{
				bSwitch = GBuffer.ReadByte();
				if (!(bSwitch & 0x80))
				{
					if (!(bSwitch & 0x40))
					{
						nCount		= bSwitch & 0x3F;
						if (nCount > 0)
							nPaletteIndex	= 0;
					}
					else
					{
						nCount			= (bSwitch&0x3F) <<8 | (SHORT)GBuffer.ReadByte();
						nPaletteIndex	= 0;
					}
				}
				else
				{
					if (!(bSwitch & 0x40))
					{
						nCount			= bSwitch & 0x3F;
						nPaletteIndex	= GBuffer.ReadByte();
					}
					else
					{
						nCount			= (bSwitch&0x3F) <<8 | (SHORT)GBuffer.ReadByte();
						nPaletteIndex	= GBuffer.ReadByte();
					}
				}
			}

			if (nCount>0)
			{
				FillSolidRect (spd, nX, nY, nCount, 1, m_Colors[nPaletteIndex]);
				nX += nCount;
			}
			else
			{
				nY++;
				nX = 0;
			}
		}
	}
}
