#include "stdafx.h"
#include "Cdg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CCdg::CCdg()
{
  m_hOffset = NULL;
  m_vOffset = NULL;
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
  BYTE AlphaColor = ((BYTE) (m_SubCode.data)) & 0x0F;
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
