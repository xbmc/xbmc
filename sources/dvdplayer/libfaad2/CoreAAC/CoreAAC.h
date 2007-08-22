/* 
 * CoreAAC - AAC DirectShow Decoder Filter
 *
 * Modification to decode AAC without ADTS and multichannel support
 * christophe.paris@free.fr
 *
 * Under section 8 of the GNU General Public License, the copyright
 * holders of CoreAAC explicitly forbid distribution in the following
 * countries:
 * - Japan
 * - United States of America 
 *
 *
 * AAC DirectShow Decoder Filter
 * Copyright (C) 2003 Robert Cioch
 *  
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include "../faad2/include/faad.h"

// ===========================================================================================

class CCoreAACDecoder : public CTransformFilter,
                 public ISpecifyPropertyPages,
				 public ICoreAACDec
{
public :
	DECLARE_IUNKNOWN
	static CUnknown *WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);    

	CCoreAACDecoder(LPUNKNOWN lpunk, HRESULT *phr);
	virtual ~CCoreAACDecoder();

	// ----- ISpecifyPropertyPages -----
	STDMETHODIMP GetPages(CAUUID *pPages);
 
	// ----- ITransformFilter -----
	HRESULT Transform(IMediaSample *pIn, IMediaSample *pOut);
	HRESULT CheckInputType(const CMediaType *mtIn);
	HRESULT CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut);
	HRESULT DecideBufferSize(IMemAllocator *pAllocator, ALLOCATOR_PROPERTIES *pprop);
	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
	HRESULT CompleteConnect(PIN_DIRECTION direction, IPin *pReceivePin);
	HRESULT StartStreaming(void);
	bool Decode(BYTE *pSrc, DWORD SrcLength, BYTE *pDst, DWORD DstLength, DWORD *ActualDstLength);

	// ----- ICoreAACDec -----
	STDMETHODIMP get_ProfileName(char** name);
    STDMETHODIMP get_SampleRate(int* sample_rate);
    STDMETHODIMP get_Channels(int *channels);
	STDMETHODIMP get_BitsPerSample(int *bits_per_sample);
	STDMETHODIMP get_Bitrate(int *bitrate);
	STDMETHODIMP get_FramesDecoded(unsigned int *frames_decoded);
	STDMETHODIMP get_DownMatrix(bool *down_matrix);
	STDMETHODIMP set_DownMatrix(bool down_matrix);
	
private:
	unsigned char* m_decoderSpecific;
	int m_decoderSpecificLen;
	faacDecHandle m_decHandle;
	int m_Channels;
	int m_SamplesPerSec;
	int m_BitsPerSample;
	bool m_DownMatrix;
	char m_ProfileName[64];

	unsigned int m_Bitrate;
	unsigned int m_brCalcFrames;
	unsigned int m_brBytesConsumed;
	unsigned int m_DecodedFrames;

	unsigned int m_OutputBuffLen;
};

// ===========================================================================================