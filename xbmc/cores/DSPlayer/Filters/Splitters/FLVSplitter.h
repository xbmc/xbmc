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

#pragma once


#include "../BaseFilters/BaseSplitter.h"
using namespace boost;
using namespace std;
[uuid("47E792CF-0BBE-4F7A-859C-194B0768650A")]
class CFLVSplitterFilter : public CBaseSplitterFilter
{
	UINT32 m_DataOffset;
	bool m_IgnorePrevSizes;

	bool Sync(__int64& pos);

	struct VideoTweak
	{
		BYTE x;
		BYTE y;
	};

	bool ReadTag(VideoTweak& t);
	
	struct Tag
	{
		UINT32 PreviousTagSize;
		BYTE TagType;
		UINT32 DataSize;
		UINT32 TimeStamp;
		UINT32 StreamID;
	};

	bool ReadTag(Tag& t);

	struct AudioTag
	{
		BYTE SoundFormat;
		BYTE SoundRate;
		BYTE SoundSize;
		BYTE SoundType;
	};

	bool ReadTag(AudioTag& at);

	struct VideoTag
	{
		BYTE FrameType;
		BYTE CodecID;
	};

	bool ReadTag(VideoTag& vt);

	void NormalSeek(REFERENCE_TIME rt);
	void AlternateSeek(REFERENCE_TIME rt);

protected:
	shared_ptr<CBaseSplitterFileEx> m_pFile;
	HRESULT CreateOutputs(IAsyncReader* pAsyncReader);

	bool DemuxInit();
	void DemuxSeek(REFERENCE_TIME rt);
	bool DemuxLoop();

public:
	CFLVSplitterFilter(LPUNKNOWN pUnk, HRESULT* phr);
};

[uuid("C9ECE7B3-1D8E-41F5-9F24-B255DF16C087")]
class CFLVSourceFilter : public CFLVSplitterFilter
{
public:
	CFLVSourceFilter(LPUNKNOWN pUnk, HRESULT* phr);
};

#include "../BaseFilters/BaseVideoFilter.h"
