/* 
 * $Id: HdmvClipInfo.cpp 1306 2009-10-24 07:31:11Z casimir666 $
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

#include "stdafx.h"
#include "HdmvClipInfo.h"
#include "DSUtil.h"

extern LCID    ISO6392ToLcid(LPCSTR code);

CHdmvClipInfo::CHdmvClipInfo(void)
{
  m_hFile      = INVALID_HANDLE_VALUE;
  m_bIsHdmv    = false;
}

CHdmvClipInfo::~CHdmvClipInfo()
{
  CloseFile(S_OK);
}

HRESULT CHdmvClipInfo::CloseFile(HRESULT hr)
{
  if (m_hFile != INVALID_HANDLE_VALUE)
  {
    CloseHandle(m_hFile);
    m_hFile = INVALID_HANDLE_VALUE;
  }
  return hr;
}

DWORD CHdmvClipInfo::ReadDword()
{
  return ReadByte()<<24 | ReadByte()<<16 | ReadByte()<<8 | ReadByte();
}

SHORT CHdmvClipInfo::ReadShort()
{
  return ReadByte()<<8 | ReadByte();
}

BYTE CHdmvClipInfo::ReadByte()
{
  BYTE  bVal;
  DWORD  dwRead;
  ReadFile (m_hFile, &bVal, sizeof(bVal), &dwRead, NULL);

  return bVal;
}

void CHdmvClipInfo::ReadBuffer(BYTE* pBuff, int nLen)
{
  DWORD  dwRead;
  ReadFile (m_hFile, pBuff, nLen, &dwRead, NULL);
}

HRESULT CHdmvClipInfo::ReadProgramInfo()
{
  BYTE    number_of_program_sequences;
  BYTE    number_of_streams_in_ps;
  DWORD    dwPos;

  m_Streams.clear();
  SetFilePointer (m_hFile, ProgramInfo_start_address, NULL, FILE_BEGIN);

  ReadDword();  //length
  ReadByte();    //reserved_for_word_align
  number_of_program_sequences    = (BYTE)ReadByte();
  int iStream = 0;
  for (int i=0; i<number_of_program_sequences; i++)
  {     
    ReadDword();  //SPN_program_sequence_start
    ReadShort();  //program_map_PID
    number_of_streams_in_ps = (BYTE)ReadByte();    //number_of_streams_in_ps
    ReadByte();    //reserved_for_future_use
  
    for (int stream_index=0; stream_index<number_of_streams_in_ps; stream_index++)
    { 
      m_Streams.resize(iStream + 1);
      m_Streams[iStream].m_PID      = ReadShort();  // stream_PID
      
      // == StreamCodingInfo
      dwPos  = SetFilePointer(m_hFile, 0, NULL, FILE_CURRENT) + 1;
      dwPos += ReadByte();  // length
      m_Streams[iStream].m_Type  = (PES_STREAM_TYPE)ReadByte();
      
      switch (m_Streams[iStream].m_Type)
      {
      case VIDEO_STREAM_MPEG1:
      case VIDEO_STREAM_MPEG2:
      case VIDEO_STREAM_H264:
      case VIDEO_STREAM_VC1:
        {
          uint8 Temp = ReadByte(); 
          BDVM_VideoFormat VideoFormat = (BDVM_VideoFormat)(Temp >> 4);
          BDVM_FrameRate FrameRate = (BDVM_FrameRate)(Temp & 0xf);
          Temp = ReadByte(); 
          BDVM_AspectRatio AspectRatio = (BDVM_AspectRatio)(Temp >> 4);

          m_Streams[iStream].m_VideoFormat = VideoFormat;
          m_Streams[iStream].m_FrameRate = FrameRate;
          m_Streams[iStream].m_AspectRatio = AspectRatio;
        }
        break;
      case AUDIO_STREAM_MPEG1:
      case AUDIO_STREAM_MPEG2:
      case AUDIO_STREAM_LPCM:
      case AUDIO_STREAM_AC3:
      case AUDIO_STREAM_DTS:
      case AUDIO_STREAM_AC3_TRUE_HD:
      case AUDIO_STREAM_AC3_PLUS:
      case AUDIO_STREAM_DTS_HD:
      case AUDIO_STREAM_DTS_HD_MASTER_AUDIO:
      case SECONDARY_AUDIO_AC3_PLUS:
      case SECONDARY_AUDIO_DTS_HD:
        {
          uint8 Temp = ReadByte(); 
          BDVM_ChannelLayout ChannelLayout = (BDVM_ChannelLayout)(Temp >> 4);
          BDVM_SampleRate SampleRate = (BDVM_SampleRate)(Temp & 0xF);

          ReadBuffer((BYTE*)m_Streams[iStream].m_LanguageCode, 3);
          m_Streams[iStream].m_LCID = ISO6392ToLcid (m_Streams[iStream].m_LanguageCode);
          m_Streams[iStream].m_ChannelLayout = ChannelLayout;
          m_Streams[iStream].m_SampleRate = SampleRate;
        }
        break;
      case PRESENTATION_GRAPHICS_STREAM:
      case INTERACTIVE_GRAPHICS_STREAM:
        {
          ReadBuffer((BYTE*)m_Streams[iStream].m_LanguageCode, 3);
          m_Streams[iStream].m_LCID = ISO6392ToLcid (m_Streams[iStream].m_LanguageCode);
        }
        break;
      case SUBTITLE_STREAM:
        {
          ReadByte(); // Should this really be here?
          ReadBuffer((BYTE*)m_Streams[iStream].m_LanguageCode, 3);
          m_Streams[iStream].m_LCID = ISO6392ToLcid (m_Streams[iStream].m_LanguageCode);
        }
        break;
      default :
        break;
      }

      iStream++;
      SetFilePointer(m_hFile, dwPos, NULL, FILE_BEGIN);
    }   
  }  
  return S_OK;
}


HRESULT CHdmvClipInfo::ReadInfo(const wchar_t *strFile)
{
  BYTE    Buff[100];

  m_bIsHdmv = false;
  m_hFile   = CreateFile(LPCWSTR(strFile), GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, 
                 OPEN_EXISTING, FILE_ATTRIBUTE_READONLY|FILE_FLAG_SEQUENTIAL_SCAN, NULL);

  if(m_hFile != INVALID_HANDLE_VALUE)
  {
    ReadBuffer(Buff, 4);
    if (memcmp (Buff, "HDMV", 4)) return CloseFile(VFW_E_INVALID_FILE_FORMAT);

    ReadBuffer(Buff, 4);
    if ((memcmp (Buff, "0200", 4)!=0) && (memcmp (Buff, "0100", 4)!=0)) return CloseFile (VFW_E_INVALID_FILE_FORMAT);

    SequenceInfo_start_address  = ReadDword();
    ProgramInfo_start_address  = ReadDword();

    ReadProgramInfo();

    m_bIsHdmv = true;

    return CloseFile(S_OK);
  }
  
  return AmHresultFromWin32(GetLastError());
}

CHdmvClipInfo::Stream* CHdmvClipInfo::FindStream(SHORT wPID)
{
  int nStreams = int(m_Streams.size());
  for (int i=0; i<nStreams; i++)
  {
    if (m_Streams[i].m_PID == wPID)
      return &m_Streams[i];
  }

  return NULL;
}

LPCTSTR CHdmvClipInfo::Stream::Format()
{
  switch (m_Type)
  {
  case VIDEO_STREAM_MPEG1:
    return _T("Mpeg1");
  case VIDEO_STREAM_MPEG2:
    return _T("Mpeg2");
  case VIDEO_STREAM_H264:
    return _T("H264");
  case VIDEO_STREAM_VC1:
    return _T("VC1");
  case AUDIO_STREAM_MPEG1:
    return _T("MPEG1");
  case AUDIO_STREAM_MPEG2:
    return _T("MPEG2");
  case AUDIO_STREAM_LPCM:
    return _T("LPCM");
  case AUDIO_STREAM_AC3:
    return _T("AC3");
  case AUDIO_STREAM_DTS:
    return _T("DTS");
  case AUDIO_STREAM_AC3_TRUE_HD:
    return _T("MLP");
  case AUDIO_STREAM_AC3_PLUS:
    return _T("DD+");
  case AUDIO_STREAM_DTS_HD:
    return _T("DTS-HD");
  case AUDIO_STREAM_DTS_HD_MASTER_AUDIO:
    return _T("DTS-HD XLL");
  case SECONDARY_AUDIO_AC3_PLUS:
    return _T("Sec DD+");
  case SECONDARY_AUDIO_DTS_HD:
    return _T("Sec DTS-HD");
  case PRESENTATION_GRAPHICS_STREAM :
    return _T("PG");
  case INTERACTIVE_GRAPHICS_STREAM :
    return _T("IG");
  case SUBTITLE_STREAM :
    return _T("Text");
  default :
    return _T("Unknown");
  }
}


HRESULT CHdmvClipInfo::ReadPlaylist(LPCTSTR strFile, REFERENCE_TIME& rtDuration, std::list<CStdString>& Playlist)
{
  DWORD        dwPos;
  DWORD        dwTemp;
  SHORT        nPlaylistItems;
  REFERENCE_TIME    rtIn, rtOut;
  BYTE        Buff[100];
  CStdString        strTemp;

  //strTemp.Format(_T("%sPLAYLIST\\%s"), strPath, strFile);
  strTemp.Format(_T("%s"),strFile);
  m_hFile   = CreateFile(strTemp, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, 
    OPEN_EXISTING, FILE_ATTRIBUTE_READONLY|FILE_FLAG_SEQUENTIAL_SCAN, NULL);

  if(m_hFile != INVALID_HANDLE_VALUE)
  {
    ReadBuffer(Buff, 4);
    if (memcmp (Buff, "MPLS", 4)) return VFW_E_INVALID_FILE_FORMAT;

    ReadBuffer(Buff, 4);
    if ((memcmp (Buff, "0200", 4)!=0) && (memcmp (Buff, "0100", 4)!=0)) return VFW_E_INVALID_FILE_FORMAT;

    dwPos = ReadDword();
    SetFilePointer(m_hFile, dwPos, NULL, FILE_BEGIN);

    ReadDword();
    ReadShort();
    dwTemp       = ReadShort();
    nPlaylistItems = min((SHORT)dwTemp, 5);  // Main movie should be more than 5 parts...
    ReadShort();

    dwPos    += 10;
    rtDuration = 0;
    for (int i=0; i<nPlaylistItems; i++)
    {    
      SetFilePointer(m_hFile, dwPos, NULL, FILE_BEGIN);
      dwPos = dwPos + ReadShort() + 2;
      ReadBuffer(Buff, 5);
      //TODO FIX THIS
      //strTemp.Format(_T("%sSTREAM\\%c%c%c%c%c.M2TS"), strPath, Buff[0], Buff[1], Buff[2], Buff[3], Buff[4]);
      Playlist.push_back(strTemp);

      ReadBuffer(Buff, 4);
      if (memcmp (Buff, "M2TS", 4)) return VFW_E_INVALID_FILE_FORMAT;
      ReadBuffer(Buff, 3);

      dwTemp  = ReadDword();
      rtIn = 20000i64*dwTemp/90;  // Carefull : 32->33 bits!

      dwTemp  = ReadDword();
      rtOut = 20000i64*dwTemp/90;  // Carefull : 32->33 bits!

      rtDuration += (rtOut - rtIn);

      //      TRACE ("File : %S,  Duration : %S, Total duration  : %S\n", strName, ReftimeToString (rtOut - rtIn), ReftimeToString (rtDuration));
    }

    CloseHandle (m_hFile);
    return S_OK;
  }

  return AmHresultFromWin32(GetLastError());
}

HRESULT CHdmvClipInfo::FindMainMovie(LPCTSTR strFolder, CStdString& strPlaylistFile, std::list<CStdString>& MainPlaylist)
{
  HRESULT        hr    = E_FAIL;
  REFERENCE_TIME    rtMax  = 0;
  REFERENCE_TIME    rtCurrent;
  CStdString        strPath (strFolder);
  CStdString        strFilter;
  CStdString        strCurrentPlaylist;
  std::list<CStdString>  Playlist;
  WIN32_FIND_DATA    fd = {0};

  strPath.Replace(_T("\\PLAYLIST\\"), _T("\\"));
  //strPath.Replace(_T("\\BDMV\\"),    _T("\\"));
  strPath.Replace(_T("\\STREAM\\"),    _T("\\"));
  strPath  += _T("\\BDMV\\");
  strFilter.Format (_T("%sPLAYLIST\\*.mpls"), strPath);

  HANDLE hFind = FindFirstFile(strFilter, &fd);
  if(hFind != INVALID_HANDLE_VALUE)
  {
    do
    {
      strCurrentPlaylist.Format(_T("%sPLAYLIST\\%s"), strPath, fd.cFileName);
      Playlist.clear();
      
      // Main movie shouldn't have duplicate M2TS filename...
      CStdString newpath;
      newpath = strPath;
      newpath.AppendFormat(_T("\\%s"),fd.cFileName);
      if (ReadPlaylist(newpath, rtCurrent, Playlist) == S_OK && rtCurrent > rtMax)
      {
        rtMax      = rtCurrent;

        strPlaylistFile = strCurrentPlaylist;
        MainPlaylist.clear();
        MainPlaylist.insert(MainPlaylist.end(), Playlist.begin(), Playlist.end());

        hr        = S_OK;
      }
    }
    while(FindNextFile(hFind, &fd));
    
    FindClose(hFind);
  }

  return hr;
}
