/* 
 *	Copyright (C) 2003-2006 Gabest
 *	http://www.gabest.org
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


#include "FLVSplitter.h"
//#include "../../../DSUtil/DSUtil.h"

#include <initguid.h>
#include <moreuuids.h>

//
// CFLVSplitterFilter
//

CFLVSplitterFilter::CFLVSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CBaseSplitterFilter(NAME("CFLVSplitterFilter"), pUnk, phr, __uuidof(this))
{
}

bool CFLVSplitterFilter::ReadTag(Tag& t)
{
	if(m_pFile->GetRemaining() < 15) 
		return false;

	t.PreviousTagSize = (UINT32)m_pFile->BitRead(32);
	t.TagType = (BYTE)m_pFile->BitRead(8);
	t.DataSize = (UINT32)m_pFile->BitRead(24);
	t.TimeStamp = (UINT32)m_pFile->BitRead(24);
	t.TimeStamp |= (UINT32)m_pFile->BitRead(8) << 24;
	t.StreamID = (UINT32)m_pFile->BitRead(24);

	return m_pFile->GetRemaining() >= t.DataSize;
}

bool CFLVSplitterFilter::ReadTag(AudioTag& at)
{
	if(!m_pFile->GetRemaining()) 
		return false;

	at.SoundFormat = (BYTE)m_pFile->BitRead(4);
	at.SoundRate = (BYTE)m_pFile->BitRead(2);
	at.SoundSize = (BYTE)m_pFile->BitRead(1);
	at.SoundType = (BYTE)m_pFile->BitRead(1);

	return true;
}

bool CFLVSplitterFilter::ReadTag(VideoTag& vt)
{
	if(!m_pFile->GetRemaining()) 
		return false;

	vt.FrameType = (BYTE)m_pFile->BitRead(4);
	vt.CodecID = (BYTE)m_pFile->BitRead(4);

	return true;
}

#ifndef NOVIDEOTWEAK
bool CFLVSplitterFilter::ReadTag(VideoTweak& vt)
{
	if(!m_pFile->GetRemaining()) 
		return false;

	vt.x = (BYTE)m_pFile->BitRead(4);
	vt.y = (BYTE)m_pFile->BitRead(4);

	return true;
}
#endif

bool CFLVSplitterFilter::Sync(__int64& pos)
{
	m_pFile->Seek(pos);

	while(m_pFile->GetRemaining() >= 15)
	{
		__int64 limit = m_pFile->GetRemaining();
		while (true) {
			BYTE b = m_pFile->BitRead(8);
			if (b == 8 || b == 9) break;
			if (--limit < 15) return false;
		}

		pos = m_pFile->GetPos() - 5;
		m_pFile->Seek(pos);

		Tag ct;
		if (ReadTag(ct)) {
			__int64 next = m_pFile->GetPos() + ct.DataSize;
			if(next == m_pFile->GetLength() - 4) {
				m_pFile->Seek(pos);
				return true;
			}
			else if (next <= m_pFile->GetLength() - 19) {
				m_pFile->Seek(next);
				Tag nt;
				if (ReadTag(nt) && (nt.TagType == 8 || nt.TagType == 9 || nt.TagType == 18)) {
					if ((nt.PreviousTagSize == ct.DataSize + 11) ||
						(m_IgnorePrevSizes &&
						 nt.TimeStamp >= ct.TimeStamp &&
						 nt.TimeStamp - ct.TimeStamp <= 1000))
					{
						m_pFile->Seek(pos);
						return true;
					}
				}
			}
		}

		m_pFile->Seek(pos + 5);
	}

	return false;
}

HRESULT CFLVSplitterFilter::CreateOutputs(IAsyncReader* pAsyncReader)
{
	CheckPointer(pAsyncReader, E_POINTER);

	HRESULT hr = E_FAIL;

	m_pFile.reset();
	m_pFile.reset(DNew CBaseSplitterFileEx(pAsyncReader, hr, DEFAULT_CACHE_LENGTH, false));
	if(!m_pFile) 
    return E_OUTOFMEMORY;
	if(FAILED(hr)) 
  {
    m_pFile.reset(); 
    return hr;
  }
  
	m_rtNewStart = m_rtCurrent = 0;
	m_rtNewStop = m_rtStop = m_rtDuration = 0;

	if(m_pFile->BitRead(24) != 'FLV' || m_pFile->BitRead(8) != 1)
		return E_FAIL;

	EXECUTE_ASSERT(m_pFile->BitRead(5) == 0); // TypeFlagsReserved
	bool fTypeFlagsAudio = !!m_pFile->BitRead(1);
	EXECUTE_ASSERT(m_pFile->BitRead(1) == 0); // TypeFlagsReserved
	bool fTypeFlagsVideo = !!m_pFile->BitRead(1);
	m_DataOffset = (UINT32)m_pFile->BitRead(32);

	// doh, these flags aren't always telling the truth
	fTypeFlagsAudio = fTypeFlagsVideo = true;

	Tag t;
	AudioTag at;
	VideoTag vt;

	UINT32 prevTagSize = 0;
	m_IgnorePrevSizes = false;

	m_pFile->Seek(m_DataOffset);

	for(int i = 0; ReadTag(t) && (fTypeFlagsVideo || fTypeFlagsAudio) && i < 100; i++)
	{
		UINT64 next = m_pFile->GetPos() + t.DataSize;

		CStdStringW name;

		CMediaType mt;
		mt.SetSampleSize(1);
		mt.subtype = GUID_NULL;

		if (i != 0 && t.PreviousTagSize != prevTagSize) {
			m_IgnorePrevSizes = true;
		}
		prevTagSize = t.DataSize + 11;

		if(t.TagType == 8 && t.DataSize != 0 && fTypeFlagsAudio)
		{
			UNREFERENCED_PARAMETER(at);
			AudioTag at;
			name = L"Audio";

			if(ReadTag(at))
			{
				int dataSize = t.DataSize - 1;

				fTypeFlagsAudio = false;

				mt.majortype = MEDIATYPE_Audio;
				mt.formattype = FORMAT_WaveFormatEx;
				WAVEFORMATEX* wfe = (WAVEFORMATEX*)mt.AllocFormatBuffer(sizeof(WAVEFORMATEX));
				memset(wfe, 0, sizeof(WAVEFORMATEX));
				wfe->nSamplesPerSec = 44100*(1<<at.SoundRate)/8;
				wfe->wBitsPerSample = 8*(at.SoundSize+1);
				wfe->nChannels = 1*(at.SoundType+1);
				
				switch(at.SoundFormat)
				{
				case 0: // FLV_CODECID_PCM_BE
					mt.subtype = FOURCCMap(wfe->wFormatTag = WAVE_FORMAT_PCM);
					break;
				case 1: // FLV_CODECID_ADPCM
					mt.subtype = FOURCCMap(MAKEFOURCC('A','S','W','F'));
					break;
				case 2:	// FLV_CODECID_MP3
					mt.subtype = FOURCCMap(wfe->wFormatTag = WAVE_FORMAT_MP3);

					{
						CBaseSplitterFileEx::mpahdr h;
						CMediaType mt2;
						if(m_pFile->Read(h, 4, false, &mt2))
							mt = mt2;
					}
					break;
				case 3 :	// FLV_CODECID_PCM_LE
					// ToDo
					break;
				case 4 :	// unknown
					break;
				case 5 :	// FLV_CODECID_NELLYMOSER_8HZ_MONO
					mt.subtype = FOURCCMap(MAKEFOURCC('N','E','L','L'));
					wfe->nSamplesPerSec = 8000;
					break;
				case 6 :	// FLV_CODECID_NELLYMOSER
					mt.subtype = FOURCCMap(MAKEFOURCC('N','E','L','L'));
					break;
				case 10: { // FLV_CODECID_AAC
					if (dataSize < 1 || m_pFile->BitRead(8) != 0) { // packet type 0 == aac header
						fTypeFlagsAudio = true;
						break;
					}

					const int sampleRates[] = {
						96000, 88200, 64000, 48000, 44100, 32000, 24000,
						22050, 16000, 12000, 11025, 8000, 7350
					};
					const int channels[] = {
						0, 1, 2, 3, 4, 5, 6, 8
					};

					__int64 configOffset = m_pFile->GetPos();
					UINT32 configSize = dataSize - 1;
					if (configSize < 2) break;

					// Might break depending on the AAC profile, see ff_mpeg4audio_get_config in ffmpeg's mpeg4audio.c
					m_pFile->BitRead(5);
					int iSampleRate = m_pFile->BitRead(4);
					int iChannels = m_pFile->BitRead(4);
					if (iSampleRate > 12 || iChannels > 7) break;

					wfe = (WAVEFORMATEX*)mt.AllocFormatBuffer(sizeof(WAVEFORMATEX) + configSize);
					memset(wfe, 0, mt.FormatLength());
					wfe->nSamplesPerSec = sampleRates[iSampleRate];
					wfe->wBitsPerSample = 16;
					wfe->nChannels = channels[iChannels];
					wfe->cbSize = configSize;

					m_pFile->Seek(configOffset);
					m_pFile->ByteRead((BYTE*)(wfe+1), configSize);

					mt.subtype = FOURCCMap(wfe->wFormatTag = WAVE_FORMAT_AAC);
				}
				
				}
			}
		}
		else if(t.TagType == 9 && t.DataSize != 0 && fTypeFlagsVideo)
		{
			UNREFERENCED_PARAMETER(vt);
			VideoTag vt;
			if(ReadTag(vt) && vt.FrameType == 1)
			{
				int dataSize = t.DataSize - 1;

				fTypeFlagsVideo = false;
				name = L"Video";

				mt.majortype = MEDIATYPE_Video;
				mt.formattype = FORMAT_VideoInfo;
				VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER));
				memset(vih, 0, sizeof(VIDEOINFOHEADER));

				BITMAPINFOHEADER* bih = &vih->bmiHeader;

				int w, h, arx, ary;

				switch(vt.CodecID)
				{
				case 2:   // H.263
					if(m_pFile->BitRead(17) != 1) break;

					m_pFile->BitRead(13); // Version (5), TemporalReference (8)

					switch(BYTE PictureSize = (BYTE)m_pFile->BitRead(3)) // w00t
					{
					case 0: case 1:
						vih->bmiHeader.biWidth = (WORD)m_pFile->BitRead(8*(PictureSize+1));
						vih->bmiHeader.biHeight = (WORD)m_pFile->BitRead(8*(PictureSize+1));
						break;
					case 2: case 3: case 4: 
						vih->bmiHeader.biWidth = 704 / PictureSize;
						vih->bmiHeader.biHeight = 576 / PictureSize;
						break;
					case 5: case 6: 
						PictureSize -= 3;
						vih->bmiHeader.biWidth = 640 / PictureSize;
						vih->bmiHeader.biHeight = 480 / PictureSize;
						break;
					}

					if(!vih->bmiHeader.biWidth || !vih->bmiHeader.biHeight) break;

					mt.subtype = FOURCCMap(vih->bmiHeader.biCompression = '1VLF');

					break;

				case 5:  // VP6 with alpha
					m_pFile->BitRead(24);
				case 4: { // VP6
					#ifdef NOVIDEOTWEAK
					m_pFile->BitRead(8);
					#else					
					VideoTweak fudge;
					ReadTag(fudge);
					#endif
				
					if (m_pFile->BitRead(1)) {
						// Delta (inter) frame
						fTypeFlagsVideo = true;
						break;
					}
					m_pFile->BitRead(6);
					bool fSeparatedCoeff = !!m_pFile->BitRead(1);
					m_pFile->BitRead(5);
					int filterHeader = m_pFile->BitRead(2);
					m_pFile->BitRead(1);
					if (fSeparatedCoeff || !filterHeader) {
						m_pFile->BitRead(16);
					}

					h = m_pFile->BitRead(8) * 16;
					w = m_pFile->BitRead(8) * 16;

					ary = m_pFile->BitRead(8) * 16;
					arx = m_pFile->BitRead(8) * 16;

					if(arx && arx != w || ary && ary != h) {
						VIDEOINFOHEADER2* vih2 = (VIDEOINFOHEADER2*)mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER2));
						memset(vih2, 0, sizeof(VIDEOINFOHEADER2));
						vih2->dwPictAspectRatioX = arx;
						vih2->dwPictAspectRatioY = ary;
						bih = &vih2->bmiHeader;						
						mt.formattype = FORMAT_VideoInfo2;
						vih = (VIDEOINFOHEADER *)vih2;
					}

					bih->biWidth = w;
					bih->biHeight = h;
					#ifndef NOVIDEOTWEAK
					SetRect(&vih->rcSource, 0, 0, w - fudge.x, h - fudge.y);
					SetRect(&vih->rcTarget, 0, 0, w - fudge.x, h - fudge.y);
					#endif

					mt.subtype = FOURCCMap(bih->biCompression = '4VLF');

					break;
				}
				case 7: { // H.264
					if (dataSize < 4 || m_pFile->BitRead(8) != 0) { // packet type 0 == avc header
						fTypeFlagsVideo = true;
						break;
					}
					m_pFile->BitRead(24); // composition time

					__int64 headerOffset = m_pFile->GetPos();
					UINT32 headerSize = dataSize - 4;
					BYTE *headerData = DNew BYTE[headerSize];

					m_pFile->ByteRead(headerData, headerSize);

					m_pFile->Seek(headerOffset + 9);

					mt.formattype = FORMAT_MPEG2Video;
					MPEG2VIDEOINFO* vih = (MPEG2VIDEOINFO*)mt.AllocFormatBuffer(FIELD_OFFSET(MPEG2VIDEOINFO, dwSequenceHeader) + headerSize);
					memset(vih, 0, mt.FormatLength());
					vih->hdr.bmiHeader.biSize = sizeof(vih->hdr.bmiHeader);
					vih->hdr.bmiHeader.biPlanes = 1;
					vih->hdr.bmiHeader.biBitCount = 24;
					vih->dwFlags = (headerData[4] & 0x03) + 1; // nal length size

					vih->dwProfile = (BYTE)m_pFile->BitRead(8);
					m_pFile->BitRead(8);
					vih->dwLevel = (BYTE)m_pFile->BitRead(8);
					m_pFile->UExpGolombRead(); // seq_parameter_set_id
					if(vih->dwProfile >= 100) { // high profile
						if(m_pFile->UExpGolombRead() == 3) // chroma_format_idc
							m_pFile->BitRead(1); // residue_transform_flag
						m_pFile->UExpGolombRead(); // bit_depth_luma_minus8
						m_pFile->UExpGolombRead(); // bit_depth_chroma_minus8
						m_pFile->BitRead(1); // qpprime_y_zero_transform_bypass_flag
						if(m_pFile->BitRead(1)) // seq_scaling_matrix_present_flag
							for(int i = 0; i < 8; i++)
								if(m_pFile->BitRead(1)) // seq_scaling_list_present_flag
									for(int j = 0, size = i < 6 ? 16 : 64, next = 8; j < size && next != 0; ++j)
										next = (next + m_pFile->SExpGolombRead() + 256) & 255;
					}
					m_pFile->UExpGolombRead(); // log2_max_frame_num_minus4
					UINT64 pic_order_cnt_type = m_pFile->UExpGolombRead();
					if(pic_order_cnt_type == 0) {
						m_pFile->UExpGolombRead(); // log2_max_pic_order_cnt_lsb_minus4
					}
					else if(pic_order_cnt_type == 1) {
						m_pFile->BitRead(1); // delta_pic_order_always_zero_flag
						m_pFile->SExpGolombRead(); // offset_for_non_ref_pic
						m_pFile->SExpGolombRead(); // offset_for_top_to_bottom_field
						UINT64 num_ref_frames_in_pic_order_cnt_cycle = m_pFile->UExpGolombRead();
						for(int i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++)
							m_pFile->SExpGolombRead(); // offset_for_ref_frame[i]
					}
					m_pFile->UExpGolombRead(); // num_ref_frames
					m_pFile->BitRead(1); // gaps_in_frame_num_value_allowed_flag
					UINT64 pic_width_in_mbs_minus1 = m_pFile->UExpGolombRead();
					UINT64 pic_height_in_map_units_minus1 = m_pFile->UExpGolombRead();
					BYTE frame_mbs_only_flag = (BYTE)m_pFile->BitRead(1);
					vih->hdr.bmiHeader.biWidth = vih->hdr.dwPictAspectRatioX = (LONG)((pic_width_in_mbs_minus1 + 1) * 16);
					vih->hdr.bmiHeader.biHeight = vih->hdr.dwPictAspectRatioY = (LONG)((2 - frame_mbs_only_flag) * (pic_height_in_map_units_minus1 + 1) * 16);

					BYTE* src = (BYTE*)headerData + 5;
					BYTE* dst = (BYTE*)vih->dwSequenceHeader;
					BYTE* src_end = (BYTE*)headerData + headerSize;
					BYTE* dst_end = (BYTE*)vih->dwSequenceHeader + headerSize;
					int spsCount = *(src++) & 0x1F;
					int ppsCount = -1;

					vih->cbSequenceHeader = 0;

					while (src < src_end - 1) {
						if (spsCount == 0 && ppsCount == -1) {
							ppsCount = *(src++);
							continue;
						}

						if (spsCount > 0) spsCount--;
						else if (ppsCount > 0) ppsCount--;
						else break;

						int len = ((src[0] << 8) | src[1]) + 2;
						if(src + len > src_end || dst + len > dst_end) {ASSERT(0); break;}
						memcpy(dst, src, len);
						src += len; 
						dst += len;
						vih->cbSequenceHeader += len;
					}

					delete[] headerData;

					mt.subtype = FOURCCMap(vih->hdr.bmiHeader.biCompression = '1CVA');

					break;
				}
				default:
					fTypeFlagsVideo = true;
				}
			}
		}
			
		if(mt.subtype != GUID_NULL)
		{
			vector<CMediaType> mts;
			mts.push_back(mt);
			Com::Auto_Ptr<CBaseSplitterOutputPin> pPinOut(DNew CBaseSplitterOutputPin(mts, name, this, this, &hr));
			EXECUTE_ASSERT(SUCCEEDED(AddOutputPin(t.TagType, pPinOut)));
		}

		m_pFile->Seek(next);
	}

	if(m_pFile->IsRandomAccess())
	{
		__int64 pos = dsmax(m_DataOffset, m_pFile->GetLength() - 256 * 1024);

		if(Sync(pos))
		{
			Tag t;
			AudioTag at;
			VideoTag vt;

			while(ReadTag(t))
			{
				UINT64 next = m_pFile->GetPos() + t.DataSize;

				if(t.TagType == 8 && ReadTag(at) || t.TagType == 9 && ReadTag(vt))
				{
					m_rtDuration = dsmax(m_rtDuration, 10000i64 * t.TimeStamp); 
				}

				m_pFile->Seek(next);
			}
		}
	}

	m_rtNewStop = m_rtStop = m_rtDuration;

	return m_pOutputs.size() > 0 ? S_OK : E_FAIL;
}

bool CFLVSplitterFilter::DemuxInit()
{
	return true;
}

void CFLVSplitterFilter::DemuxSeek(REFERENCE_TIME rt)
{
	if(!m_rtDuration || rt <= 0) {
		m_pFile->Seek(m_DataOffset);
	}
	else if (!m_IgnorePrevSizes) {
		NormalSeek(rt);
	}
	else {
		AlternateSeek(rt);
	}
}

void CFLVSplitterFilter::NormalSeek(REFERENCE_TIME rt)
{
	bool fAudio = !!GetOutputPin(8);
	bool fVideo = !!GetOutputPin(9);

	__int64 pos = m_DataOffset + 1.0 * rt / m_rtDuration * (m_pFile->GetLength() - m_DataOffset);

	if(!Sync(pos))
	{
		ASSERT(0);
		m_pFile->Seek(m_DataOffset);
		return;
	}

	Tag t;
	AudioTag at;
	VideoTag vt;

	while(ReadTag(t))
	{
		if(10000i64 * t.TimeStamp >= rt)
		{
			m_pFile->Seek(m_pFile->GetPos() - 15);
			break;
		}

		m_pFile->Seek(m_pFile->GetPos() + t.DataSize);
	}

	while(m_pFile->GetPos() >= m_DataOffset && (fAudio || fVideo) && ReadTag(t))
	{
		UINT64 prev = m_pFile->GetPos() - 15 - t.PreviousTagSize - 4;

		if(10000i64 * t.TimeStamp <= rt)
		{
			if(t.TagType == 8 && ReadTag(at))
			{
				fAudio = false;
			}
			else if(t.TagType == 9 && ReadTag(vt) && vt.FrameType == 1)
			{
				fVideo = false;
			}
		}

		m_pFile->Seek(prev);
	}

	if(fAudio || fVideo)
	{
		ASSERT(0);
		m_pFile->Seek(m_DataOffset);
	}
}

void CFLVSplitterFilter::AlternateSeek(REFERENCE_TIME rt)
{
	bool hasAudio = !!GetOutputPin(8);
	bool hasVideo = !!GetOutputPin(9);

	__int64 estimPos = m_DataOffset + 1.0 * rt / m_rtDuration * (m_pFile->GetLength() - m_DataOffset);
	__int64 seekBack = 256 * 1024;

	while (true) {
		bool foundAudio = false;
		bool foundVideo = false;
		__int64 bestPos;

		estimPos = dsmax(estimPos - seekBack, m_DataOffset);
		seekBack *= 2;

		if (Sync(estimPos)) {
			Tag t;
			AudioTag at;
			VideoTag vt;

			while (ReadTag(t) && t.TimeStamp * 10000i64 < rt) {
				__int64 cur = m_pFile->GetPos() - 15;
				__int64 next = cur + 15 + t.DataSize;

				if (hasAudio && t.TagType == 8 && ReadTag(at)) {
					foundAudio = true;
					if (!hasVideo) bestPos = cur;
				}
				else if (hasVideo && t.TagType == 9 && ReadTag(vt) && vt.FrameType == 1) {
					foundVideo = true;
					bestPos = cur;
				}

				m_pFile->Seek(next);
			}
		}

		if ((hasAudio && !foundAudio) || (hasVideo && !foundVideo)) {
			if (estimPos == m_DataOffset) {
				m_pFile->Seek(m_DataOffset);
				return;
			}
		}
		else {
			m_pFile->Seek(bestPos);
			return;
		}
	}
}

bool CFLVSplitterFilter::DemuxLoop()
{
	HRESULT hr = S_OK;

	Com::Auto_Ptr<Packet> p;

	Tag t;
	AudioTag at;
	VideoTag vt;

	while(SUCCEEDED(hr) && !CheckRequest(NULL) && m_pFile->GetRemaining())
	{
		if(!ReadTag(t)) break;

		__int64 next = m_pFile->GetPos() + t.DataSize;

		if((t.DataSize > 0) && (t.TagType == 8 && ReadTag(at) || t.TagType == 9 && ReadTag(vt)))
		{
			UINT32 tsOffset = 0;
			if(t.TagType == 9 && vt.FrameType == 5) goto NextTag; // video info/command frame
			if(t.TagType == 9 && vt.CodecID == 4) m_pFile->BitRead(8);
			if(t.TagType == 9 && vt.CodecID == 5) m_pFile->BitRead(32);
			if(t.TagType == 9 && vt.CodecID == 7) {
				if (m_pFile->BitRead(8) != 1) goto NextTag;
				// Tag timestamps specify decode time, this is the display time offset
				tsOffset = m_pFile->BitRead(24);
				tsOffset = (tsOffset + 0xff800000) ^ 0xff800000; // sign extension
			}
			if(t.TagType == 8 && at.SoundFormat == 10) {
				if (m_pFile->BitRead(8) != 1) goto NextTag;
			}
			__int64 dataSize = next - m_pFile->GetPos();
			if (dataSize <= 0) goto NextTag;
			p.Attach(DNew Packet());
			p->TrackNumber = t.TagType;
			p->rtStart = 10000i64 * (t.TimeStamp + tsOffset); 
			p->rtStop = p->rtStart + 1;
			p->bSyncPoint = t.TagType == 9 ? vt.FrameType == 1 : true;
			p->resize(dataSize);
			m_pFile->ByteRead(&p->at(0), p->size());
			hr = DeliverPacket(p);
		}

NextTag:
		m_pFile->Seek(next);
	}

	return true;
}

//
// CFLVSourceFilter
//

CFLVSourceFilter::CFLVSourceFilter(LPUNKNOWN pUnk, HRESULT* phr)
	: CFLVSplitterFilter(pUnk, phr)
{
	m_clsid = __uuidof(this);
	m_pInput.Release();
}


