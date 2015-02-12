/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h"
#include "filesystem/File.h"
#include "settings/DisplaySettings.h"
#include "guilib/GraphicContext.h"
#include "guilib/Texture.h"
#include "guilib/GUITexture.h"
#include "settings/AdvancedSettings.h"
#include "utils/MathUtils.h"
#include "utils/log.h"
#include "utils/auto_buffer.h"
#include "karaokelyricscdg.h"


CKaraokeLyricsCDG::CKaraokeLyricsCDG( const std::string& cdgFile )
  : CKaraokeLyrics()
  , m_cdgFile(cdgFile)
{
  m_pCdgTexture = 0;
  m_streamIdx = -1;
  m_bgAlpha = 0xff000000;
  m_fgAlpha = 0xff000000;
  m_hOffset = 0;
  m_vOffset = 0;
  m_borderColor = 0;
  m_bgColor = 0;
  
  memset( m_cdgScreen, 0, sizeof(m_cdgScreen) );

  for ( int i = 0; i < 16; i++ )
    m_colorTable[i] = 0;
}

CKaraokeLyricsCDG::~CKaraokeLyricsCDG()
{
  Shutdown();
}

bool CKaraokeLyricsCDG::HasBackground()
{
  return true;
}

bool CKaraokeLyricsCDG::HasVideo()
{
  return false;
}

void CKaraokeLyricsCDG::GetVideoParameters(std::string& path, int64_t & offset)
{
  // no bg video
}

BYTE CKaraokeLyricsCDG::getPixel( int x, int y )
{
  unsigned int offset = x + y * CDG_FULL_WIDTH;

  if ( x >= (int) CDG_FULL_WIDTH || y >= (int) CDG_FULL_HEIGHT )
	  return m_borderColor;
  
  if ( x < 0 || y < 0 || offset >= CDG_FULL_HEIGHT * CDG_FULL_WIDTH )
  {
	CLog::Log( LOGERROR, "CDG renderer: requested pixel (%d,%d) is out of boundary", x, y );
	return 0;
  }
  
  return m_cdgScreen[offset];
}

void CKaraokeLyricsCDG::setPixel( int x, int y, BYTE color )
{
  unsigned int offset = x + y * CDG_FULL_WIDTH;

  if ( x < 0 || y < 0 || offset >= CDG_FULL_HEIGHT * CDG_FULL_WIDTH )
  {
	CLog::Log( LOGERROR, "CDG renderer: set pixel (%d,%d) is out of boundary", x, y );
	return;
  }

  m_cdgScreen[offset] = color;
}


bool CKaraokeLyricsCDG::InitGraphics()
{
  // set the background to be completely transparent if we use visualisations, or completely solid if not
  if ( g_advancedSettings.m_karaokeAlwaysEmptyOnCdgs )
    m_bgAlpha = 0xff000000;
  else
    m_bgAlpha = 0;

  if (!m_pCdgTexture)
  {
	m_pCdgTexture = new CTexture( CDG_FULL_WIDTH, CDG_FULL_HEIGHT, XB_FMT_A8R8G8B8 );
  }

  if ( !m_pCdgTexture )
  {
    CLog::Log(LOGERROR, "CDG renderer: failed to create texture" );
    return false;
  }

  return true;
}

void CKaraokeLyricsCDG::Shutdown()
{
  m_cdgStream.clear();

  if ( m_pCdgTexture )
  {
    delete m_pCdgTexture;
    m_pCdgTexture = NULL;
  }
}


void CKaraokeLyricsCDG::Render()
{
  // Do not render if we have no texture
  if ( !m_pCdgTexture )
    return;

  // Time to update?
  unsigned int songTime = (unsigned int) MathUtils::round_int( (getSongTime() + g_advancedSettings.m_karaokeSyncDelayCDG) * 1000 );
  unsigned int packets_due = songTime * 300 / 1000;

  if ( UpdateBuffer( packets_due ) )
  {
    XUTILS::auto_buffer buf(CDG_FULL_HEIGHT * CDG_FULL_WIDTH*sizeof(DWORD));
    DWORD* const pixelbuf = (DWORD*)buf.get();

	  // Update our texture content
	  for ( UINT y = 0; y < CDG_FULL_HEIGHT; y++ )
	  {
		DWORD *texel = (DWORD *) (pixelbuf + y * CDG_FULL_WIDTH);

		for ( UINT x = 0; x < CDG_FULL_WIDTH; x++ )
		{
		  BYTE colorindex = getPixel( x + m_hOffset, y + m_vOffset );
		  DWORD TexColor = m_colorTable[ colorindex ];

		  // Is it transparent color?
		  if ( TexColor != 0xFFFFFFFF )
		  {
			TexColor &= 0x00FFFFFF;

			if ( colorindex == m_bgColor )
				TexColor |= m_bgAlpha;
			else
				TexColor |= m_fgAlpha;
		  }
		  else
			  TexColor = 0x00000000;

		  *texel++ = TexColor;
		}
	  }

	  m_pCdgTexture->Update( CDG_FULL_WIDTH, CDG_FULL_HEIGHT, CDG_FULL_WIDTH * 4, XB_FMT_A8R8G8B8, (BYTE*) pixelbuf, false );
  }

  // Convert texture coordinates to (0..1)
  CRect texCoords((float)CDG_BORDER_WIDTH / CDG_FULL_WIDTH,
				  (float)CDG_BORDER_HEIGHT  / CDG_FULL_HEIGHT,
				  (float)(CDG_FULL_WIDTH - CDG_BORDER_WIDTH) / CDG_FULL_WIDTH,
				  (float)(CDG_FULL_HEIGHT - CDG_BORDER_HEIGHT) / CDG_FULL_HEIGHT);

  // Get screen coordinates
  const RESOLUTION_INFO info = g_graphicsContext.GetResInfo();
  CRect vertCoords((float)info.Overscan.left,
                   (float)info.Overscan.top,
                   (float)info.Overscan.right,
                   (float)info.Overscan.bottom);

  CGUITexture::DrawQuad(vertCoords, 0xffffffff, m_pCdgTexture, &texCoords);
}

void CKaraokeLyricsCDG::cmdMemoryPreset( const char * data )
{
  CDG_MemPreset* preset = (CDG_MemPreset*) data;

  if ( preset->repeat & 0x0F )
	return;  // No need for multiple clearings

  m_bgColor = preset->color & 0x0F;

  for ( unsigned int i = CDG_BORDER_WIDTH; i < CDG_FULL_WIDTH - CDG_BORDER_WIDTH; i++ )
	for ( unsigned int  j = CDG_BORDER_HEIGHT; j < CDG_FULL_HEIGHT - CDG_BORDER_HEIGHT; j++ )
	  setPixel( i, j, m_bgColor );

  //CLog::Log( LOGDEBUG, "CDG: screen color set to %d", m_bgColor );
}

void CKaraokeLyricsCDG::cmdBorderPreset( const char * data )
{
  CDG_BorderPreset* preset = (CDG_BorderPreset*) data;

  m_borderColor = preset->color & 0x0F;

  for ( unsigned int i = 0; i < CDG_BORDER_WIDTH; i++ )
  {
	for ( unsigned int j = 0; j < CDG_FULL_HEIGHT; j++ )
	{
	  setPixel( i, j, m_borderColor );
	  setPixel( CDG_FULL_WIDTH - i - 1, j, m_borderColor );
	}
  }

  for ( unsigned int i = 0; i < CDG_FULL_WIDTH; i++ )
  {
	for ( unsigned int j = 0; j < CDG_BORDER_HEIGHT; j++ )
	{
	  setPixel( i, j, m_borderColor );
	  setPixel( i, CDG_FULL_HEIGHT - j - 1, m_borderColor );
	}
  }

  //CLog::Log( LOGDEBUG, "CDG: border color set to %d", borderColor );
}

void CKaraokeLyricsCDG::cmdTransparentColor( const char * data )
{
	int index = data[0] & 0x0F;
	m_colorTable[index] = 0xFFFFFFFF;
}

void CKaraokeLyricsCDG::cmdLoadColorTable( const char * data, int index )
{
  CDG_LoadColorTable* table = (CDG_LoadColorTable*) data;

  for ( int i = 0; i < 8; i++ )
  {
	UINT colourEntry = ((table->colorSpec[2 * i] & CDG_MASK) << 8);
	colourEntry = colourEntry + (table->colorSpec[(2 * i) + 1] & CDG_MASK);
	colourEntry = ((colourEntry & 0x3F00) >> 2) | (colourEntry & 0x003F);

	BYTE red = ((colourEntry & 0x0F00) >> 8) * 17;
	BYTE green = ((colourEntry & 0x00F0) >> 4) * 17;
	BYTE blue = ((colourEntry & 0x000F)) * 17;

	m_colorTable[index+i] = (red << 16) | (green << 8) | blue;

	//CLog::Log( LOGDEBUG, "CDG: loadColors: color %d -> %02X %02X %02X (%08X)", index + i, red, green, blue, m_colorTable[index+i] );
  }
}

void CKaraokeLyricsCDG::cmdTileBlock( const char * data )
{
  CDG_Tile* tile = (CDG_Tile*) data;
  UINT offset_y = (tile->row & 0x1F) * 12;
  UINT offset_x = (tile->column & 0x3F) * 6;

  //CLog::Log( LOGERROR, "TileBlockXor: %d, %d", offset_x, offset_y );

  if ( offset_x + 6 >= CDG_FULL_WIDTH || offset_y + 12 >= CDG_FULL_HEIGHT )
	return;

  // In the XOR variant, the color values are combined with the color values that are
  // already onscreen using the XOR operator.  Since CD+G only allows a maximum of 16
  // colors, we are XORing the pixel values (0-15) themselves, which correspond to
  // indexes into a color lookup table.  We are not XORing the actual R,G,B values.
  BYTE color_0 = tile->color0 & 0x0F;
  BYTE color_1 = tile->color1 & 0x0F;

  BYTE mask[6] = { 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };

  for ( int i = 0; i < 12; i++ )
  {
	BYTE bTemp = tile->tilePixels[i] & 0x3F;

	for ( int j = 0; j < 6; j++ )
	{
	  if ( bTemp & mask[j] )
		setPixel( offset_x + j, offset_y + i, color_1 );
	  else
		setPixel( offset_x + j, offset_y + i, color_0 );
	}
  }
}

void CKaraokeLyricsCDG::cmdTileBlockXor( const char * data )
{
  CDG_Tile* tile = (CDG_Tile*) data;
  UINT offset_y = (tile->row & 0x1F) * 12;
  UINT offset_x = (tile->column & 0x3F) * 6;

  //CLog::Log( LOGERROR, "TileBlockXor: %d, %d", offset_x, offset_y );

  if ( offset_x + 6 >= CDG_FULL_WIDTH || offset_y + 12 >= CDG_FULL_HEIGHT )
	return;

  // In the XOR variant, the color values are combined with the color values that are
  // already onscreen using the XOR operator.  Since CD+G only allows a maximum of 16
  // colors, we are XORing the pixel values (0-15) themselves, which correspond to
  // indexes into a color lookup table.  We are not XORing the actual R,G,B values.
  BYTE color_0 = tile->color0 & 0x0F;
  BYTE color_1 = tile->color1 & 0x0F;

  BYTE mask[6] = { 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };

  for ( int i = 0; i < 12; i++ )
  {
	BYTE bTemp = tile->tilePixels[i] & 0x3F;

	for ( int j = 0; j < 6; j++ )
	{
	  // Find the original color index
	  BYTE origindex = getPixel( offset_x + j, offset_y + i );

	  if ( bTemp & mask[j] )  //pixel xored with color1
		setPixel( offset_x + j, offset_y + i, origindex ^ color_1 );
	  else
		setPixel( offset_x + j, offset_y + i, origindex ^ color_0 );
	}
  }
}

// Based on http://cdg2video.googlecode.com/svn/trunk/cdgfile.cpp
void CKaraokeLyricsCDG::cmdScroll( const char * data, bool copy )
{
    int colour, hScroll, vScroll;
    int hSCmd, hOffset, vSCmd, vOffset;
    int vScrollPixels, hScrollPixels;
    
    // Decode the scroll command parameters
    colour  = data[0] & 0x0F;
    hScroll = data[1] & 0x3F;
    vScroll = data[2] & 0x3F;

    hSCmd = (hScroll & 0x30) >> 4;
    hOffset = (hScroll & 0x07);
    vSCmd = (vScroll & 0x30) >> 4;
    vOffset = (vScroll & 0x0F);

    m_hOffset = hOffset < 5 ? hOffset : 5;
    m_vOffset = vOffset < 11 ? vOffset : 11;

    // Scroll Vertical - Calculate number of pixels
    vScrollPixels = 0;
	
    if (vSCmd == 2) 
    {
        vScrollPixels = - 12;
    } 
    else  if (vSCmd == 1) 
    {
        vScrollPixels = 12;
    }

    // Scroll Horizontal- Calculate number of pixels
    hScrollPixels = 0;

	if (hSCmd == 2) 
    {
        hScrollPixels = - 6;
    } 
    else if (hSCmd == 1) 
    {
        hScrollPixels = 6;
    }

    if (hScrollPixels == 0 && vScrollPixels == 0) 
    {
        return;
    }

    // Perform the actual scroll.
    unsigned char temp[CDG_FULL_HEIGHT][CDG_FULL_WIDTH];
    int vInc = vScrollPixels + CDG_FULL_HEIGHT;
    int hInc = hScrollPixels + CDG_FULL_WIDTH;
    unsigned int ri; // row index
    unsigned int ci; // column index

    for (ri = 0; ri < CDG_FULL_HEIGHT; ++ri) 
    {
        for (ci = 0; ci < CDG_FULL_WIDTH; ++ci) 
        {   
            temp[(ri + vInc) % CDG_FULL_HEIGHT][(ci + hInc) % CDG_FULL_WIDTH] = getPixel( ci, ri );
        }
    }

    // if copy is false, we were supposed to fill in the new pixels
    // with a new colour. Go back and do that now.

    if (!copy)
    {
        if (vScrollPixels > 0) 
        {
            for (ci = 0; ci < CDG_FULL_WIDTH; ++ci) 
            {
                for (ri = 0; ri < (unsigned int)vScrollPixels; ++ri) {
                    temp[ri][ci] = colour;
                }
            }
        }
        else if (vScrollPixels < 0) 
        {
            for (ci = 0; ci < CDG_FULL_WIDTH; ++ci) 
            {
                for (ri = CDG_FULL_HEIGHT + vScrollPixels; ri < CDG_FULL_HEIGHT; ++ri) {
                    temp[ri][ci] = colour;
                }
            }
        }
        
        if (hScrollPixels > 0) 
        {
            for (ci = 0; ci < (unsigned int)hScrollPixels; ++ci) 
            {
                for (ri = 0; ri < CDG_FULL_HEIGHT; ++ri) {
                    temp[ri][ci] = colour;
                }
            }
        } 
        else if (hScrollPixels < 0) 
        {
            for (ci = CDG_FULL_WIDTH + hScrollPixels; ci < CDG_FULL_WIDTH; ++ci) 
            {
                for (ri = 0; ri < CDG_FULL_HEIGHT; ++ri) {
                    temp[ri][ci] = colour;
                }
            }
        }
    }

    // Now copy the temporary buffer back to our array
    for (ri = 0; ri < CDG_FULL_HEIGHT; ++ri) 
    {
        for (ci = 0; ci < CDG_FULL_WIDTH; ++ci) 
        {
			setPixel( ci, ri, temp[ri][ci] );
        }
    }
}

bool CKaraokeLyricsCDG::UpdateBuffer( unsigned int packets_due )
{
  bool screen_changed = false;

  // Are we done?
  if ( m_streamIdx == -1 )
	return false;

  // Was the stream position reversed? In this case we have to "replay" the whole stream
  // as the screen is a state machine, and "clear" may not be there.
  if ( m_streamIdx > 0 && m_cdgStream[ m_streamIdx-1 ].packetnum > packets_due )
  {
	  CLog::Log( LOGDEBUG, "CDG renderer: packet number changed backward (%d played, %d asked", m_cdgStream[ m_streamIdx-1 ].packetnum, packets_due );
	  m_streamIdx = 0;
  }

  // Process all packets already due
  while ( m_cdgStream[ m_streamIdx ].packetnum <= packets_due )
  {
	SubCode& sc = m_cdgStream[ m_streamIdx ].subcode;

	// Execute the instruction
	switch ( sc.instruction & CDG_MASK )
	{
		case CDG_INST_MEMORY_PRESET:
			cmdMemoryPreset( sc.data );
			screen_changed = true;
			break;

		case CDG_INST_BORDER_PRESET:
			cmdBorderPreset( sc.data );
			screen_changed = true;
			break;

		case CDG_INST_LOAD_COL_TBL_0_7:
			cmdLoadColorTable( sc.data, 0 );
			break;

		case CDG_INST_LOAD_COL_TBL_8_15:
			cmdLoadColorTable( sc.data, 8 );
			break;

		case CDG_INST_DEF_TRANSP_COL:
			cmdTransparentColor( sc.data );
			break;

		case CDG_INST_TILE_BLOCK:
			cmdTileBlock( sc.data );
			screen_changed = true;
			break;

		case CDG_INST_TILE_BLOCK_XOR:
			cmdTileBlockXor( sc.data );
			screen_changed = true;
			break;

		case CDG_INST_SCROLL_PRESET:
			cmdScroll( sc.data, false );
			screen_changed = true;
			break;

		case CDG_INST_SCROLL_COPY:
			cmdScroll( sc.data, true );
			screen_changed = true;
			break;

		default: // this shouldn't happen as we validated the stream in Load()
			break;
	}

	m_streamIdx++;

	if ( m_streamIdx >= (int) m_cdgStream.size() )
	{
	  m_streamIdx = -1;
	  break;
	}
  }

  return screen_changed;
}

bool CKaraokeLyricsCDG::Load()
{
  // Read the whole CD+G file into memory array
  XFILE::CFile file;

  m_cdgStream.clear();

  XFILE::auto_buffer buf;
  if (file.LoadFile(m_cdgFile, buf) <= 0)
  {
    CLog::Log(LOGERROR, "CDG loader: can't load CDG file \"%s\"", m_cdgFile.c_str());
    return false;
  }

  file.Close();

  // Parse the CD+G stream
  int buggy_commands = 0;
  
  for (unsigned int offset = 0; offset < buf.size(); offset += sizeof(SubCode))
  {
    SubCode * sc = (SubCode *)(buf.get() + offset);

	  if ( ( sc->command & CDG_MASK) == CDG_COMMAND )
	  {
		  CDGPacket packet;

		  // Validate the command and instruction
		  switch ( sc->instruction & CDG_MASK )
		  {
			  case CDG_INST_MEMORY_PRESET:
			  case CDG_INST_BORDER_PRESET:
			  case CDG_INST_LOAD_COL_TBL_0_7:
			  case CDG_INST_LOAD_COL_TBL_8_15:
			  case CDG_INST_TILE_BLOCK_XOR:
			  case CDG_INST_TILE_BLOCK:
			  case CDG_INST_DEF_TRANSP_COL:
			  case CDG_INST_SCROLL_PRESET:
			  case CDG_INST_SCROLL_COPY:
				memcpy( &packet.subcode, sc, sizeof(SubCode) );
				packet.packetnum = offset / sizeof( SubCode );
				m_cdgStream.push_back( packet );
				break;
			  
                          default:
				  buggy_commands++;
				  break;
		  }
	  }
  }

  // Init the screen
  memset( m_cdgScreen, 0, sizeof(m_cdgScreen) );

  // Init color table
  for ( int i = 0; i < 16; i++ )
	m_colorTable[i] = 0;

  m_streamIdx = 0;
  m_borderColor = 0;
  m_bgColor = 0;
  m_hOffset = 0;
  m_vOffset = 0;
  
  if ( buggy_commands == 0 )
	CLog::Log( LOGDEBUG, "CDG loader: CDG file %s has been loading successfully, %d useful packets, %dKb used",
				m_cdgFile.c_str(), (int)m_cdgStream.size(), (int)(m_cdgStream.size() * sizeof(CDGPacket) / 1024) );
 else
	CLog::Log( LOGDEBUG, "CDG loader: CDG file %s was damaged, %d errors ignored, %d useful packets, %dKb used",
				m_cdgFile.c_str(), buggy_commands, (int)m_cdgStream.size(), (int)(m_cdgStream.size() * sizeof(CDGPacket) / 1024) );

  return true;
}
