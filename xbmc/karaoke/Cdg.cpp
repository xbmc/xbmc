/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "Util.h"
#include "AudioContext.h"
#include "utils/GUIInfoManager.h"
#include "MusicInfoTag.h"
#include "GUIWindowManager.h"
#include "AdvancedSettings.h"
#include "Cdg.h"
#include "utils/SingleLock.h"

#include "karaokelyrics.h"

using namespace MUSIC_INFO;
using namespace XFILE;


CCdg::CCdg()
{
  m_hOffset = 0;
  m_vOffset = 0;
  m_BgroundColor = 0xFF;
  m_BorderColor = 0xFF;
  memset( (void *) &m_SubCode, 0, sizeof(SubCode));
  ClearDisplay();
}
CCdg::~CCdg()
{}

void CCdg::ReadSubCode(SubCode* pPacket)
{
  m_SubCode = *pPacket;
  if ((m_SubCode.command & SC_MASK) == SC_CDG_COMMAND) // CD+G?
  {
    switch (m_SubCode.instruction & SC_MASK)
    {
    case CDG_TILEBLOCKNORMAL: TileBlock(FALSE); break;
    case CDG_TILEBLOCKXOR: TileBlock(TRUE); break;
    case CDG_SCROLLPRESET: Scroll(FALSE); break;
    case CDG_SCROLLCOPY: Scroll(TRUE); break;
    case CDG_MEMORYPRESET: MemoryPreset(); break;
    case CDG_BORDERPRESET: BorderPreset(); break;
    case CDG_ALPHA: SetAlpha(); break;
    case CDG_COLORTABLELO: ColorTable(TRUE); break;
    case CDG_COLORTABLEHI: ColorTable(FALSE); break;
    default: break;
    }
  }
}

CDG_COLOR CCdg::GetColor(BYTE ClutOffset)
{
  if (ClutOffset & 0xF0) return 0;
  return m_ColorTable[ClutOffset];
}

BYTE CCdg::GetClutOffset(UINT uiRow, UINT uiCol)
{
  if (uiRow >= HEIGHT || uiCol >= WIDTH)
    return m_BorderColor;
  return m_PixelMap[uiRow][uiCol];
}

UINT CCdg::GetHOffset()
{
  return m_hOffset;
}

UINT CCdg::GetVOffset()
{
  return m_vOffset;
}

void CCdg::ClearDisplay()
{
  memset( (void *) m_PixelMap, 0, HEIGHT*WIDTH*sizeof(BYTE));
  memset((void*) m_ColorTable, 0, 16*sizeof(CDG_COLOR));
}
BYTE CCdg::GetBackgroundColor()
{
  return m_BgroundColor;
}

BYTE CCdg::GetBorderColor()
{
  return m_BorderColor;
}

void CCdg::MemoryPreset()
{
  CDG_MemPreset* preset = (CDG_MemPreset*) & (m_SubCode.data);
  BYTE Repeat = preset->repeat & 0x0F;
  if (Repeat) return ;  //No need for multiple clearings, we have a reliable stream...
  m_BgroundColor = preset->color & 0x0F;
  UINT i, j;
  for (i = BORDERWIDTH ; i < WIDTH - BORDERWIDTH ; i++)
    for (j = BORDERHEIGHT ; j < HEIGHT - BORDERHEIGHT ; j++)
      m_PixelMap[j][i] = m_BgroundColor;
}

void CCdg::BorderPreset()
{
  CDG_BorderPreset* preset = ( CDG_BorderPreset*) & (m_SubCode.data);
  m_BorderColor = preset->color & 0x0F;
  UINT i, j;
  for (i = 0 ;i < BORDERWIDTH;i++)
    for (j = 0;j < HEIGHT;j++)
      m_PixelMap[j][i] = m_BorderColor;
  for (i = WIDTH - BORDERWIDTH ;i < WIDTH;i++)
    for (j = 0;j < HEIGHT;j++)
      m_PixelMap[j][i] = m_BorderColor;
  for (i = 0 ;i < WIDTH;i++)
    for (j = 0;j < BORDERHEIGHT;j++)
      m_PixelMap[j][i] = m_BorderColor;
  for (i = 0 ;i < WIDTH;i++)
    for (j = HEIGHT - BORDERHEIGHT;j < HEIGHT;j++)
      m_PixelMap[j][i] = m_BorderColor;
}

void CCdg::ColorTable(bool IsLo)
{
  UINT offset;
  if (IsLo) offset = 0;
  else offset = 8;
  CDG_LoadCLUT* clut = (CDG_LoadCLUT*) & (m_SubCode.data);
  CDG_COLOR red, green, greentemp, blue;
  for (UINT i = 0; i < 8; i++)
  {
    red = clut->colorSpec[i] & 0x003C;
    red = red << 6;
    greentemp = (clut->colorSpec[i] & 0x3000) >> 8;
    green = (clut->colorSpec[i] & 0x0003) << 6;
    green |= greentemp;
    blue = (clut->colorSpec[i] & 0x0F00) >> 8;
    m_ColorTable[offset + i] = 0xF000 | red | green | blue;  //Defaults to opaque alpha
  }
}

void CCdg::Scroll(bool IsLoop)
{
  CDG_Scroll * scroll = (CDG_Scroll*) & (m_SubCode.data);
  scroll->color &= 0x0F;
  BYTE* pcolor;
  if (IsLoop) pcolor = NULL;  // Is looping mode
  else pcolor = (BYTE*) & scroll->color;
  BYTE hSCmd = (scroll->hScroll & 0x30) >> 4;
  BYTE vSCmd = (scroll->vScroll & 0x30) >> 4;

  switch (hSCmd)
  {
  case 1: ScrollRight(pcolor); break;
  case 2: ScrollLeft(pcolor); break;
  default: break;
  }
  switch (vSCmd)
  {
  case 1: ScrollDown(pcolor); break;
  case 2: ScrollUp(pcolor); break;
  default: break;
  }
  m_hOffset = scroll->hScroll & 0x07;
  m_vOffset = scroll->vScroll & 0x0F;
}

void CCdg::ScrollRight(BYTE* pcolor)
{
  BYTE PixelTemp[HEIGHT][BORDERWIDTH];
  UINT i, j;
  if (!pcolor)   //Loop the scrolling
  {
    for (i = WIDTH - BORDERWIDTH ; i < WIDTH ; i++)
      for (j = 0;j < HEIGHT;j++)
        PixelTemp[j][BORDERWIDTH - WIDTH + i] = m_PixelMap[j][i];
  }
  else         //Fill the background
  {
    for (i = WIDTH - BORDERWIDTH;i < WIDTH;i++)
      for (j = 0;j < HEIGHT;j++)
        PixelTemp[j][BORDERWIDTH - WIDTH + i] = *pcolor;
  }
  for (i = BORDERWIDTH ; i < WIDTH ; i++)   //Fill scrolled area
    for (j = 0;j < HEIGHT;j++)
      m_PixelMap[j][i] = m_PixelMap[j][i - BORDERWIDTH];
  for (i = 0;i < BORDERWIDTH;i++)      //Fill uncovered area
    for (j = 0;j < HEIGHT;j++)
      m_PixelMap[j][i] = PixelTemp[j][i];
}

void CCdg::ScrollLeft(BYTE* pcolor)
{
  BYTE PixelTemp[HEIGHT][BORDERWIDTH];
  UINT i, j;
  if (!pcolor)     //Loop the scrolling
  {
    for (i = 0;i < BORDERWIDTH;i++)
      for (j = 0;j < HEIGHT;j++)
        PixelTemp[j][i] = m_PixelMap[j][i];
  }
  else    //Fill the background
  {
    for (i = 0;i < BORDERWIDTH;i++)
      for (j = 0;j < HEIGHT;j++)
        PixelTemp[j][i] = *pcolor;
  }
  for (i = 0;i < WIDTH - BORDERWIDTH;i++)   //Fill scrolled area
    for (j = 0;j < HEIGHT;j++)
      m_PixelMap[j][i] = m_PixelMap[j][i + BORDERWIDTH];
  for (i = WIDTH - BORDERWIDTH;i < WIDTH;i++) //Fill uncovered area
    for (j = 0;j < HEIGHT;j++)
      m_PixelMap[j][i] = PixelTemp[j][i + BORDERWIDTH - WIDTH];
}

void CCdg::ScrollUp(BYTE* pcolor)
{
  BYTE PixelTemp[BORDERHEIGHT][WIDTH];
  UINT i, j;
  if (!pcolor)  //Loop the scrolling
  {
    for (i = 0;i < WIDTH;i++)
      for (j = 0 ; j < BORDERHEIGHT ;j++)
        PixelTemp[j][i] = m_PixelMap[j][i];
  }
  else    //Fill with background
  {
    for (i = 0;i < WIDTH;i++)
      for (j = 0;j < BORDERHEIGHT;j++)
        PixelTemp[j][i] = *pcolor;
  }
  for (i = 0;i < WIDTH;i++)   //Fill scrolled area
    for (j = 0;j < HEIGHT - BORDERHEIGHT;j++)
      m_PixelMap[j][i] = m_PixelMap[j + BORDERHEIGHT][i];
  for (i = 0;i < WIDTH;i++)   //Fill uncovered area
    for (j = HEIGHT - BORDERHEIGHT;j < HEIGHT;j++)
      m_PixelMap[j][i] = PixelTemp[BORDERHEIGHT - HEIGHT + j][i];
}

void CCdg::ScrollDown(BYTE* pcolor)
{
  BYTE PixelTemp[BORDERHEIGHT][WIDTH];
  UINT i, j;
  if (!pcolor)   //Loop the scrolling
  {
    for (i = 0;i < WIDTH;i++)
      for (j = HEIGHT - BORDERHEIGHT;j < HEIGHT;j++)
        PixelTemp[BORDERHEIGHT - HEIGHT + j][i] = m_PixelMap[j][i];
  }
  else     //Fill with background
  {
    for (i = 0;i < WIDTH;i++)
      for (j = HEIGHT - BORDERHEIGHT;j < HEIGHT;j++)
        PixelTemp[BORDERHEIGHT - HEIGHT + j][i] = *pcolor;
  }
  for (i = 0;i < WIDTH;i++)   //Fill scrolled area
    for (j = BORDERHEIGHT;j < HEIGHT;j++)
      m_PixelMap[j][i] = m_PixelMap[j - BORDERHEIGHT][i];
  for (i = 0;i < WIDTH;i++)  //Fill uncovered area
    for (j = 0;j < BORDERHEIGHT;j++)
      m_PixelMap[j][i] = PixelTemp[j][i];
}

void CCdg::SetAlpha()
{
  BYTE AlphaColor = ((BYTE) (*m_SubCode.data)) & 0x0F;
  m_ColorTable[AlphaColor] &= 0x0FFF;
}


void CCdg::TileBlock(bool IsXor)
{
  CDG_Tile* tile = (CDG_Tile*) & (m_SubCode.data);
  BYTE color_0 = tile->color0 & 0x0F;
  BYTE color_1 = tile->color1 & 0x0F;
  UINT row_offset = (tile->row & 0x1F) * 12;
  UINT col_offset = (tile->column & 0x3F) * 6;
  if (row_offset > HEIGHT - BORDERHEIGHT || col_offset > WIDTH - BORDERWIDTH)
    return ;
  BYTE bTemp;
  BYTE mask[6] = {0x20, 0x10, 0x08, 0x04, 0x02, 0x01};

  switch (IsXor)
  {
  case TRUE:
    for (int i = 0;i < 12;i++)
    {
      bTemp = tile->tilePixels[i] & 0x3F;
      for (int j = 0; j < 6;j++)
      {
        if (bTemp & mask[j])  //pixel xored with color1
          m_PixelMap[row_offset + i][col_offset + j] ^= color_1;
        else  //pixel xored with color0
          m_PixelMap[row_offset + i][col_offset + j] ^= color_0;
      }
    }
    break;
  case FALSE:
    for (int i = 0;i < 12;i++)
    {
      bTemp = tile->tilePixels[i] & 0x3F;
      for (int j = 0; j < 6;j++)
      {
        if (bTemp & mask[j]) //pixel set to color1
          m_PixelMap[row_offset + i][col_offset + j] = color_1;
        else //pixel set to color0
          m_PixelMap[row_offset + i][col_offset + j] = color_0;
      }
    }
    break;
  }
}


//CdgLoader
CCdgLoader::CCdgLoader()
{
  m_strFileName.Empty();
  m_CdgFileState = FILE_NOT_LOADED;
  m_pBuffer = NULL;
  m_uiFileLength = 0;
  m_uiLoadedBytes = 0;
  m_uiStreamChunk = STREAM_CHUNK;
}
CCdgLoader::~CCdgLoader()
{
  StopStream();
}

void CCdgLoader::StreamFile(CStdString strfilename)
{
  CSingleLock lock (m_CritSection);
  m_strFileName = strfilename;
  CUtil::RemoveExtension(m_strFileName);
  m_strFileName += ".cdg";
  CThread::Create(false);
}

void CCdgLoader::StopStream()
{
  CSingleLock lock (m_CritSection);
  CThread::StopThread();
  m_uiLoadedBytes = 0;
  m_CdgFileState = FILE_NOT_LOADED;
  if (m_pBuffer)
    SAFE_DELETE_ARRAY(m_pBuffer);
}

SubCode* CCdgLoader::GetCurSubCode()
{
  CSingleLock lock (m_CritSection);
  SubCode* pFirst = GetFirstLoaded();
  SubCode* pLast = GetLastLoaded();
  if (!pFirst || !pLast || !m_pSubCode) return NULL;
  if (m_pSubCode < pFirst || m_pSubCode > pLast) return NULL;
  return m_pSubCode;
}
bool CCdgLoader::SetNextSubCode()
{
  CSingleLock lock (m_CritSection);
  SubCode* pFirst = GetFirstLoaded();
  SubCode* pLast = GetLastLoaded();
  if (!pFirst || !pLast || !m_pSubCode) return false;
  if (m_pSubCode < pFirst || m_pSubCode >= pLast) return false;
  m_pSubCode++;
  return true;
}
errCode CCdgLoader::GetFileState()
{
  CSingleLock lock (m_CritSection);
  return m_CdgFileState;
}
CStdString CCdgLoader::GetFileName()
{
  CSingleLock lock (m_CritSection);
  return m_strFileName;
}
SubCode* CCdgLoader::GetFirstLoaded()
{
  if (!m_uiLoadedBytes)
    return NULL;
  return (SubCode*) m_pBuffer;
}
SubCode* CCdgLoader::GetLastLoaded()
{
  if (m_uiLoadedBytes < sizeof(SubCode) || m_uiLoadedBytes > m_uiFileLength )
    return NULL;
  return ((SubCode*) (m_pBuffer)) + m_uiLoadedBytes / sizeof(SubCode) - 1;
}
void CCdgLoader::OnStartup()
{
  CSingleLock lock (m_CritSection);
  if (!CFile::Exists(m_strFileName))
  {
    m_CdgFileState = FILE_ERR_NOT_FOUND;
    return ;
  }
  if (!m_File.Open(m_strFileName))
  {
    m_CdgFileState = FILE_ERR_OPENING;
    return ;
  }
  m_uiFileLength = (int)m_File.GetLength(); // ASSUMES FILELENGTH IS LESS THAN 2^32 bytes!!!
  if (!m_uiFileLength) return ;
  m_File.Seek(0, SEEK_SET);
  if (m_pBuffer)
    SAFE_DELETE_ARRAY(m_pBuffer);
  m_pBuffer = new BYTE[m_uiFileLength];
  if (!m_pBuffer)
  {
    m_CdgFileState = FILE_ERR_NO_MEM;
    return ;
  }
  m_uiLoadedBytes = 0;
  m_pSubCode = (SubCode*) m_pBuffer;
  m_CdgFileState = FILE_LOADING;
}

void CCdgLoader::Process()
{
  if (m_CdgFileState != FILE_LOADING && m_CdgFileState != FILE_SKIP) return ;

  if (m_uiFileLength < m_uiStreamChunk)
    m_uiLoadedBytes = m_File.Read(m_pBuffer, m_uiFileLength);
  else
  {
    UINT uiNumReadings = m_uiFileLength / m_uiStreamChunk;
    UINT uiRemainder = m_uiFileLength % m_uiStreamChunk;
    UINT uiCurReading = 0;
    while (!CThread::m_bStop && uiCurReading < uiNumReadings)
    {
      m_uiLoadedBytes += m_File.Read(m_pBuffer + m_uiLoadedBytes, m_uiStreamChunk);
      uiCurReading++;
    }
    if (uiRemainder && m_uiLoadedBytes + uiRemainder == m_uiFileLength)
      m_uiLoadedBytes += m_File.Read(m_pBuffer + m_uiLoadedBytes, uiRemainder);
  }
}

void CCdgLoader::OnExit()
{
  m_File.Close();
  if (m_uiFileLength && m_uiLoadedBytes == m_uiFileLength)
    m_CdgFileState = FILE_LOADED;
}


//CdgReader
CCdgReader::CCdgReader( CKaraokeLyrics * lyrics )
{
  m_pLoader = NULL;
  m_fAVDelay = 0.0f;
  m_uiNumReadSubCodes = 0;
  m_pLyrics = lyrics;
  m_Cdg.ClearDisplay();
}
CCdgReader::~CCdgReader()
{
  StopThread();
}


bool CCdgReader::Attach(CCdgLoader* pLoader)
{
  CSingleLock lock (m_CritSection);
  if (!m_pLoader)
    m_pLoader = pLoader;
  if (!m_pLoader) return false;
  return true;
}
void CCdgReader::DetachLoader()
{
  StopThread();
  CSingleLock lock (m_CritSection);
  m_pLoader = NULL;
}
bool CCdgReader::Start()
{
  CSingleLock lock (m_CritSection);
  if (!m_pLoader) return false;
  SetAVDelay(g_advancedSettings.m_karaokeSyncDelayCDG);
  m_uiNumReadSubCodes = 0;
  m_Cdg.ClearDisplay();
  m_FileState = FILE_LOADED;
  CThread::Create(false);
  return true;
}

void CCdgReader::SetAVDelay(float fDelay)
{
  m_fAVDelay = fDelay;
#ifdef _DEBUG
  m_fAVDelay -= DEBUG_AVDELAY_MOD;
#endif
}
float CCdgReader::GetAVDelay()
{
  return m_fAVDelay;
}
errCode CCdgReader::GetFileState()
{
  CSingleLock lock (m_CritSection);

  if (m_FileState == FILE_SKIP)
    return m_FileState;

  if (!m_pLoader) return FILE_NOT_LOADED;
  return m_pLoader->GetFileState();
}
CCdg* CCdgReader::GetCdg()
{
  return (CCdg*) &m_Cdg;
}

CStdString CCdgReader::GetFileName()
{
  CSingleLock lock (m_CritSection);
  if (m_pLoader)
    return m_pLoader->GetFileName();
  return "";
}
void CCdgReader::ReadUpToTime(float secs)
{
  if (secs < 0) return ;
  if (!(m_pLoader->GetCurSubCode())) return ;

  UINT uiFinalOffset = (UINT) (secs * PARSING_FREQ);
  if ( m_uiNumReadSubCodes >= uiFinalOffset) return ;
  UINT i;
  for (i = m_uiNumReadSubCodes; i <= uiFinalOffset; i++)
  {
    m_Cdg.ReadSubCode(m_pLoader->GetCurSubCode());
    if (m_pLoader->SetNextSubCode())
      m_uiNumReadSubCodes++;
  }
}

void CCdgReader::SkipUpToTime(float secs)
{
  if (secs < 0) return ;
  m_FileState= FILE_SKIP;

  UINT uiFinalOffset = (UINT) (secs * PARSING_FREQ);
  // is this needed?
  if ( m_uiNumReadSubCodes > uiFinalOffset) return ;
  for (UINT i = m_uiNumReadSubCodes; i <= uiFinalOffset; i++)
  {
    //m_Cdg.ReadSubCode(m_pLoader->GetCurSubCode());
    if (m_pLoader->SetNextSubCode())
      m_uiNumReadSubCodes++;
  }
  m_FileState = FILE_LOADING;
}

void CCdgReader::OnStartup()
{}

void CCdgReader::Process()
{
  double fCurTime = 0.0f;
  double fNewTime=0.f;
  CStdString strExt;
  CUtil::GetExtension(m_pLoader->GetFileName(),strExt);
  strExt = m_pLoader->GetFileName().substr(0,m_pLoader->GetFileName().size()-strExt.size());

  while (!CThread::m_bStop)
  {
    CSingleLock lock (m_CritSection);
    double fDiff;
    const CMusicInfoTag* tag = g_infoManager.GetCurrentSongTag();
    if (!tag || tag->GetURL().substr(0,strExt.size()) != strExt)
    {
      Sleep(15);
      if (CThread::m_bStop)
        return;

      CUtil::GetExtension(m_pLoader->GetFileName(),strExt);
      strExt = m_pLoader->GetFileName().substr(0,m_pLoader->GetFileName().size()-strExt.size());

      fDiff = 0.f;
    }
    else
    {
      fNewTime=m_pLyrics->getSongTime();
      fDiff = fNewTime-fCurTime-m_fAVDelay;
    }
    if (fDiff < -0.3f)
    {
      CStdString strFile = m_pLoader->GetFileName();
      m_pLoader->StopStream();
      while (m_pLoader->GetCurSubCode()) {}
      m_pLoader->StreamFile(strFile);
      m_uiNumReadSubCodes = 0;
      m_Cdg.ClearDisplay();
      fNewTime = m_pLyrics->getSongTime();
      SkipUpToTime((float)fNewTime-m_fAVDelay);
    }
    else
      ReadUpToTime((float)fNewTime-m_fAVDelay);

    fCurTime = fNewTime;
    lock.Leave();
    Sleep(15);
  }
}

void CCdgReader::OnExit()
{}
