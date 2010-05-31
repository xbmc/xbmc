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
#include "DShowUtil/Mpeg2Def.h"

#define NO_SUBTITLE_PID			1		// Fake PID use for the "No subtitle" entry

template <typename t_CType>
t_CType GetFormatHelper(t_CType &_pInfo, const CMediaType *_pFormat)
{
	ASSERT(_pFormat->cbFormat >= sizeof(*_pInfo));
	_pInfo = (t_CType)_pFormat->pbFormat;
	return _pInfo;
}

LONGLONG GetMediaTypeQuality(const CMediaType *_pMediaType, int _PresentationFormat);

class CMpegSplitterFile : public CBaseSplitterFileEx
{
  std::map<WORD, BYTE> m_pid2pes;
	CMpegSplitterFile::avchdr avch;
	bool m_bIsHdmv;


	HRESULT Init();

	void OnComplete();

public:
	CHdmvClipInfo &m_ClipInfo;
	CMpegSplitterFile(IAsyncReader* pAsyncReader, HRESULT& hr, bool bIsHdmv, CHdmvClipInfo &ClipInfo);

	REFERENCE_TIME NextPTS(DWORD TrackNum);

	CCritSec m_csProps;

	enum {us, ps, ts, es, pva} m_type;

	REFERENCE_TIME m_rtMin, m_rtMax;
	__int64 m_posMin, m_posMax;
	int m_rate; // byte/sec

	struct stream
	{
		CMpegSplitterFile *m_pFile;
		CMediaType mt;
		WORD pid;
		BYTE pesid, ps1id;
		bool operator < (const stream &_Other) const;
		struct stream() {pid = pesid = ps1id = 0;}
		operator DWORD() const {return pid ? pid : ((pesid<<8)|ps1id);}
		bool operator == (const struct stream& s) const {return (DWORD)*this == (DWORD)s;}
	};

	enum {video, audio, subpic, unknown};

	class CStreamList : public std::vector<stream>
	{
	public:
		void Insert(stream& s, CMpegSplitterFile *_pFile)
		{
			s.m_pFile = _pFile;
      for (std::vector<stream>::const_iterator it = begin(); it != end(); it++)
      {
        stream _Other = *it;
        bool betterstream = false;
        if (s.mt.majortype == MEDIATYPE_Audio && _Other.mt.majortype == MEDIATYPE_Audio)
	      {
		      int iProgram0;
		      const CHdmvClipInfo::Stream *pClipInfo0;
		      const CMpegSplitterFile::program * pProgram0 = s.m_pFile->FindProgram(s.pid, iProgram0, pClipInfo0);
		      int StreamType0 = pClipInfo0 ? pClipInfo0->m_Type : pProgram0 ? pProgram0->streams[iProgram0].type : 0;
		      int iProgram1;
		      const CHdmvClipInfo::Stream *pClipInfo1;
		      const CMpegSplitterFile::program * pProgram1 = s.m_pFile->FindProgram(_Other.pid, iProgram1, pClipInfo1);
		      int StreamType1 = pClipInfo1 ? pClipInfo1->m_Type : pProgram1 ? pProgram1->streams[iProgram1].type : 0;
      		
		      if (s.mt.formattype == FORMAT_WaveFormatEx && _Other.mt.formattype != FORMAT_WaveFormatEx)
			      betterstream = true;
		      if (s.mt.formattype != FORMAT_WaveFormatEx && _Other.mt.formattype == FORMAT_WaveFormatEx)
			      betterstream = false;

          LONGLONG Quality0 = GetMediaTypeQuality(&s.mt, StreamType0);
		      LONGLONG Quality1 = GetMediaTypeQuality(&_Other.mt, StreamType1);
		      if (Quality0 > Quality1)
			      betterstream = true;
		      if (Quality0 < Quality1)
			      betterstream = false;
	      }
	      DWORD DefaultFirst = s;
	      DWORD DefaultSecond = _Other;
        if (DefaultFirst < DefaultSecond)
          betterstream = true;
        if (betterstream)
        {
          insert(it--,s);
          return;
        }
      }
      push_back(s);
			/*for(POSITION pos = GetHeadPosition(); pos; GetNext(pos))
			{
				stream& s2 = GetAt(pos);
				if(s < s2) {InsertBefore(pos, s); return;}
			}*/

			//AddTail(s);
		}

		static CStdStringW ToString(int type)
		{
			return 
				type == video ? L"Video" : 
				type == audio ? L"Audio" : 
				type == subpic ? L"Subtitle" : 
				L"Unknown";
		}

    bool Find(stream s)
    {
      for (std::vector<stream>::iterator it = begin(); it != end(); it++)
      {
        const stream& ss = *it;
        if (ss == s)
          return true;
      }
      return false;
    }

		const stream* FindStream(int pid)
		{
      for (std::vector<stream>::iterator it = begin(); it != end(); it++)
      {
        const stream& s = *it;
        if (s.pid == pid)
          return &s;
      }
			/*for(POSITION pos = GetHeadPosition(); pos; GetNext(pos))
			{
				const stream& s = GetAt(pos);
				if(s.pid == pid) return &s;
			}*/

			return NULL;
		}

	} m_streams[unknown];

	HRESULT SearchStreams(__int64 start, __int64 stop);
	DWORD AddStream(WORD pid, BYTE pesid, DWORD len);
	void  AddHdmvPGStream(WORD pid, const char* language_code);
	std::vector<stream>* GetMasterStream();
	bool IsHdmv() { return m_bIsHdmv; };

	struct program
	{
		WORD					program_number;
		struct stream
		{
			WORD				pid;
			PES_STREAM_TYPE		type;

		};
		stream streams[64];
		struct program() {memset(this, 0, sizeof(*this));}
	};

  std::map<WORD, program> m_programs;

	void UpdatePrograms(const trhdr& h);
	const program* FindProgram(WORD pid, int &iStream, const CHdmvClipInfo::Stream * &_pClipInfo);

	
};
