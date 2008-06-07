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

#endif CDG_H
