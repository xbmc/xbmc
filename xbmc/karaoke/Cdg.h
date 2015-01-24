#ifndef CDG_H
#define CDG_H

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

#include "utils/Thread.h"
#include "FileSystem/File.h"
#include "utils/CriticalSection.h"

#define  HEIGHT 216  //CDG display size with overscan border
#define  WIDTH  300
#define  BORDERHEIGHT   12  //overscan border
#define  BORDERWIDTH    6
#define CDG_COLOR WORD  //Standard Cdg Format is A4R4G4B4

#define SC_MASK  0x3F
#define SC_CDG_COMMAND    0x09
#define CDG_MEMORYPRESET  1
#define CDG_BORDERPRESET  2
#define CDG_TILEBLOCKNORMAL 6
#define CDG_SCROLLPRESET  20
#define CDG_SCROLLCOPY  24
#define CDG_ALPHA  28
#define CDG_COLORTABLELO  30
#define CDG_COLORTABLEHI  31
#define CDG_TILEBLOCKXOR  38


typedef struct
{
  char command;
  char instruction;
  char parityQ[2];
  char data[16];
  char parityP[4];
}
SubCode;
typedef struct
{
  char color;    // Only lower 4 bits are used, mask with 0x0F
  char repeat;    // Only lower 4 bits are used, mask with 0x0F
  char filler[14];
}
CDG_MemPreset;
typedef struct
{
  char color;    // Only lower 4 bits are used, mask with 0x0F
  char filler[15];
}
CDG_BorderPreset;
typedef struct
{
  char color0;    // Only lower 4 bits are used, mask with 0x0F
  char color1;    // Only lower 4 bits are used, mask with 0x0F
  char row;     // Only lower 5 bits are used, mask with 0x1F
  char column;    // Only lower 6 bits are used, mask with 0x3F
  char tilePixels[12]; // Only lower 6 bits of each byte are used
}
CDG_Tile;
typedef struct
{
  char color;    // Only lower 4 bits are used, mask with 0x0F
  char hScroll;   // Only lower 6 bits are used, mask with 0x3F
  char vScroll;    // Only lower 6 bits are used, mask with 0x3F
}
CDG_Scroll;
typedef struct
{
  WORD colorSpec[8]; // AND with 0x3F3F to clear P and Q channel
}
CDG_LoadCLUT;

class CCdg
{
public:
  CCdg();
  ~CCdg();
  void ReadSubCode(SubCode* pCurSubCode);
  BYTE GetClutOffset(UINT uiRow, UINT uiCol);
  CDG_COLOR GetColor(BYTE offset);
  UINT GetHOffset();
  UINT GetVOffset();
  UINT GetNumSubCode();
  UINT GetCurSubCode();
  void SetNextSubCode();
  BYTE GetBackgroundColor();
  BYTE GetBorderColor();
  void ClearDisplay();

protected:
  BYTE m_PixelMap[HEIGHT][WIDTH];
  CDG_COLOR m_ColorTable[16];
  UINT m_hOffset;   // Horizontal scrolling display offset
  UINT m_vOffset;   //Vertical scrolling display offset
  BYTE m_BgroundColor;
  BYTE m_BorderColor;
  SubCode m_SubCode;

  void MemoryPreset();
  void BorderPreset();
  void SetAlpha();
  void Scroll(bool IsLoop);
  void ScrollLeft(BYTE* pFillColor);
  void ScrollRight(BYTE* pFillColor);
  void ScrollUp(BYTE* pFillColor);
  void ScrollDown(BYTE* pFillColor);
  void ColorTable(bool IsLo);
  void TileBlock(bool IsXor);
};


//////////////////////
//////CdgLoader///////
//////////////////////
#define STREAM_CHUNK 32768 // Cdg File is streamed in chunks of 32Kb
typedef enum
{
  FILE_ERR_NOT_FOUND,
  FILE_ERR_OPENING,
  FILE_ERR_NO_MEM,
  FILE_ERR_LOADING,
  FILE_LOADING,
  FILE_LOADED,
  FILE_NOT_LOADED,
  FILE_SKIP
} errCode;

class CCdgLoader : public CThread
{
public:
  CCdgLoader();
  ~CCdgLoader();
  void StreamFile(CStdString strFileName);
  void StopStream();
  SubCode* GetCurSubCode();
  bool SetNextSubCode();
  errCode GetFileState();
  CStdString GetFileName();
protected:
  XFILE::CFile m_File;
  CStdString m_strFileName;
  BYTE *m_pBuffer;
  SubCode *m_pSubCode;
  errCode m_CdgFileState;
  UINT m_uiFileLength;
  UINT m_uiLoadedBytes;
  UINT m_uiStreamChunk;
  CCriticalSection m_CritSection;

  SubCode* GetFirstLoaded();
  SubCode* GetLastLoaded();
  virtual void OnStartup();
  virtual void OnExit();
  virtual void Process();
};


//////////////////////
//////CdgReader///////
//////////////////////
//CDG data packets should be read at a fixed frequency to assure sync with audio:
//Audio content on cd :
//(44100 samp/(chan*second)*2 chan*16 bit/samp)/(2352*8 bits/sector)=75 sectors/second.
// CDG data on cd :
//4 packets/sector*75 sectors/second=300 packets/second = 300 Hz
#define PARSING_FREQ 300.0f

class CKaraokeLyrics;

class CCdgReader : public CThread
{
public:
  CCdgReader( CKaraokeLyrics * lyrics );
  ~CCdgReader();
  bool Attach(CCdgLoader* pLoader);
  void DetachLoader();
  bool Start();
  void SetAVDelay(float fDelay);
  float GetAVDelay();
  errCode GetFileState();
  CStdString GetFileName();
  CCdg* GetCdg();

protected:
  errCode m_FileState;
  CCdgLoader* m_pLoader;
  CKaraokeLyrics * m_pLyrics;
  CCdg m_Cdg;
  float m_fAVDelay;
  UINT m_uiNumReadSubCodes;
  CCriticalSection m_CritSection;

  void ReadUpToTime(float secs);
  void SkipUpToTime(float secs);
  virtual void OnStartup();
  virtual void OnExit();
  virtual void Process();
};

#endif // CDG_H
