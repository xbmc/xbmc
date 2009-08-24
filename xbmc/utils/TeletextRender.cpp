/*
 *      Copyright (C) 2005-2009 Team XBMC
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
 
/* 
 * Teletext display abstraction and teletext code renderer
 *
 * Code is taken from VDR Teletext Plugin 
 * initial version (c) Udo Richter
 *
 */

#include "stdafx.h"
#include "TeletextRender.h"


// Font tables

// teletext uses 7-bit numbers to identify a font set.
// There are three font sets involved:
// Primary G0, Secondary G0, and G2 font set.

// Font tables are organized in blocks of 8 fonts:

enumCharsets FontBlockG0_0000[8] = {
  CHARSET_LATIN_G0_EN,
  CHARSET_LATIN_G0_DE,
  CHARSET_LATIN_G0_SV_FI,
  CHARSET_LATIN_G0_IT,
  CHARSET_LATIN_G0_FR,
  CHARSET_LATIN_G0_PT_ES,
  CHARSET_LATIN_G0_CZ_SK,
  CHARSET_LATIN_G0
};

enumCharsets FontBlockG2Latin[8]={
  CHARSET_LATIN_G2,
  CHARSET_LATIN_G2,
  CHARSET_LATIN_G2,
  CHARSET_LATIN_G2,
  CHARSET_LATIN_G2,
  CHARSET_LATIN_G2,
  CHARSET_LATIN_G2,
  CHARSET_LATIN_G2
};

enumCharsets FontBlockG0_0001[8] = {
  CHARSET_LATIN_G0_PL,
  CHARSET_LATIN_G0_DE,
  CHARSET_LATIN_G0_SV_FI,
  CHARSET_LATIN_G0_IT,
  CHARSET_LATIN_G0_FR,
  CHARSET_LATIN_G0,
  CHARSET_LATIN_G0_CZ_SK,
  CHARSET_LATIN_G0
};

enumCharsets FontBlockG0_0010[8] = {
  CHARSET_LATIN_G0_EN,
  CHARSET_LATIN_G0_DE,
  CHARSET_LATIN_G0_SV_FI,
  CHARSET_LATIN_G0_IT,
  CHARSET_LATIN_G0_FR,
  CHARSET_LATIN_G0_PT_ES,
  CHARSET_LATIN_G0_TR,
  CHARSET_LATIN_G0
};


enumCharsets FontBlockG0_0011[8] = {
  CHARSET_LATIN_G0,
  CHARSET_LATIN_G0,
  CHARSET_LATIN_G0,
  CHARSET_LATIN_G0,
  CHARSET_LATIN_G0,
  CHARSET_LATIN_G0_SR_HR_SL,
  CHARSET_LATIN_G0,
  CHARSET_LATIN_G0_RO
};

enumCharsets FontBlockG0_0100[8] = {
  CHARSET_CYRILLIC_G0_SR_HR,
  CHARSET_LATIN_G0_DE,
  CHARSET_LATIN_G0_EE,
  CHARSET_LATIN_G0_LV_LT,
  CHARSET_CYRILLIC_G0_RU_BG,
  CHARSET_CYRILLIC_G0_UK,
  CHARSET_LATIN_G0_CZ_SK,
  CHARSET_INVALID
};

enumCharsets FontBlockG2_0100[8] = {
  CHARSET_CYRILLIC_G2,
  CHARSET_LATIN_G2,
  CHARSET_LATIN_G2,
  CHARSET_LATIN_G2,
  CHARSET_CYRILLIC_G2,
  CHARSET_CYRILLIC_G2,
  CHARSET_LATIN_G2,
  CHARSET_INVALID
};

enumCharsets FontBlockG0_0110[8] = {
  CHARSET_INVALID,
  CHARSET_INVALID,
  CHARSET_INVALID,
  CHARSET_INVALID,
  CHARSET_INVALID,
  CHARSET_INVALID,
  CHARSET_LATIN_G0_TR,
  CHARSET_GREEK_G0
};

enumCharsets FontBlockG2_0110[8] = {
  CHARSET_INVALID,
  CHARSET_INVALID,
  CHARSET_INVALID,
  CHARSET_INVALID,
  CHARSET_INVALID,
  CHARSET_INVALID,
  CHARSET_LATIN_G2,
  CHARSET_GREEK_G2
};

enumCharsets FontBlockG0_1000[8] = {
  CHARSET_LATIN_G0_EN,
  CHARSET_INVALID,
  CHARSET_INVALID,
  CHARSET_INVALID,
  CHARSET_LATIN_G0_FR,
  CHARSET_INVALID,
  CHARSET_INVALID,
  CHARSET_ARABIC_G0
};

enumCharsets FontBlockG2_1000[8] = {
  CHARSET_ARABIC_G2,
  CHARSET_INVALID,
  CHARSET_INVALID,
  CHARSET_INVALID,
  CHARSET_ARABIC_G2,
  CHARSET_INVALID,
  CHARSET_INVALID,
  CHARSET_ARABIC_G2
};

enumCharsets FontBlockG0_1010[8] = {
  CHARSET_INVALID,
  CHARSET_INVALID,
  CHARSET_INVALID,
  CHARSET_INVALID,
  CHARSET_INVALID,
  CHARSET_HEBREW_G0,
  CHARSET_INVALID,
  CHARSET_ARABIC_G0,
};

enumCharsets FontBlockG2_1010[8] = {
  CHARSET_INVALID,
  CHARSET_INVALID,
  CHARSET_INVALID,
  CHARSET_INVALID,
  CHARSET_INVALID,
  CHARSET_ARABIC_G2,
  CHARSET_INVALID,
  CHARSET_ARABIC_G2,
};

enumCharsets FontBlockInvalid[8] = {
  CHARSET_INVALID,
  CHARSET_INVALID,
  CHARSET_INVALID,
  CHARSET_INVALID,
  CHARSET_INVALID,
  CHARSET_INVALID,
  CHARSET_INVALID,
  CHARSET_INVALID
};



// The actual font table definition:
// Split the 7-bit number into upper 4 and lower 3 bits,
// use upper 4 bits for outer array,
// use lower 3 bits for inner array

struct structFontBlock {
  enumCharsets *G0Block;
  enumCharsets *G2Block;
};
    
structFontBlock FontTable[16] = {
  { FontBlockG0_0000, FontBlockG2Latin }, // 0000 block
  { FontBlockG0_0001, FontBlockG2Latin }, // 0001 block
  { FontBlockG0_0010, FontBlockG2Latin }, // 0010 block
  { FontBlockG0_0011, FontBlockG2Latin }, // 0011 block
  { FontBlockG0_0100, FontBlockG2_0100 }, // 0100 block
  { FontBlockInvalid, FontBlockInvalid }, // 0101 block
  { FontBlockG0_0110, FontBlockG2_0110 }, // 0110 block
  { FontBlockInvalid, FontBlockInvalid }, // 0111 block
  { FontBlockG0_1000, FontBlockG2_1000 }, // 1000 block
  { FontBlockInvalid, FontBlockInvalid }, // 1001 block
  { FontBlockG0_1010, FontBlockG2_1010 }, // 1010 block
  { FontBlockInvalid, FontBlockInvalid }, // 1011 block
  { FontBlockInvalid, FontBlockInvalid }, // 1100 block
  { FontBlockInvalid, FontBlockInvalid }, // 1101 block
  { FontBlockInvalid, FontBlockInvalid }, // 1110 block
  { FontBlockInvalid, FontBlockInvalid }  // 1111 block
};

inline enumCharsets GetG0Charset(int codepage) {
  return FontTable[codepage>>3].G0Block[codepage&7];
}
inline enumCharsets GetG2Charset(int codepage) {
  return FontTable[codepage>>3].G2Block[codepage&7];
}

    
cRenderTTPage::cRenderTTPage() {
  Dirty=false;
  DirtyAll=false;

  // Todo: make this configurable
  FirstG0CodePage=0;
  SecondG0CodePage=0;
}

enum enumSizeMode {
  // Possible size modifications of characters
  sizeNormal,
  sizeDoubleWidth,
  sizeDoubleHeight,
  sizeDoubleSize
};

/*
// Debug only: List of teletext spacing code short names
const char *(names[0x20])={
    "AlBk","AlRd","AlGr","AlYl","AlBl","AlMg","AlCy","AlWh",
    "Flsh","Stdy","EnBx","StBx","SzNo","SzDh","SzDw","SzDs",
    "MoBk","MoRd","MoGr","MoYl","MoBl","MoMg","MoCy","MoWh",
    "Conc","GrCn","GrSp","ESC", "BkBl","StBk","HoMo","ReMo"};
*/

void cRenderTTPage::ReadTeletextHeader(unsigned char *Header) {
  // Format of buffer:
  //   0     String "VTXV4"
  //   5     always 0x01
  //   6     magazine number
  //   7     page number
  //   8     flags
  //   9     lang
  //   10    always 0x00
  //   11    always 0x00
  //   12    teletext data, 40x24 bytes
  // Format of flags:
  //   0x80  C4 - Erase page
  //   0x40  C5 - News flash
  //   0x20  C6 - Subtitle
  //   0x10  C7 - Suppress Header
  //   0x08  C8 - Update
  //   0x04  C9 - Interrupt Sequence
  //   0x02  C9 (Bug?)
  //   0x01  C11 - Magazine Serial mode

  Flags=Header[8];
  Lang=Header[9];
}


void cRenderTTPage::RenderTeletextCode(unsigned char *PageCode) {
    int x,y;
    bool EmptyNextLine=false;
    // Skip one line, in case double height chars were/will be used

    // Get code pages:
    int LocalG0CodePage=(FirstG0CodePage & 0x78) 
            | ((Lang & 0x04)>>2) | (Lang & 0x02) | ((Lang & 0x01)<<2);
        
    enumCharsets FirstG0=GetG0Charset(LocalG0CodePage);
    enumCharsets SecondG0=GetG0Charset(SecondG0CodePage);
    // Reserved for later use:
    // enumCharsets FirstG2=GetG2Charset(LocalG0CodePage);
    
    for (y=0;y<24;(EmptyNextLine?y+=2:y++)) {
        // Start of line: Set start of line defaults
        
        // Hold Mosaics mode: Remember last mosaic char/charset 
        // for next spacing code
        bool HoldMosaics=false;
        unsigned char HoldMosaicChar=' ';
        enumCharsets HoldMosaicCharset=FirstG0;

        enumSizeMode Size=sizeNormal;
        // Font size modification
        bool SecondCharset=false;
        // Use primary or secondary G0 charset
        bool GraphicCharset=false;
        // Graphics charset used?
        bool SeparateGraphics=false;
        // Use separated vs. contiguous graphics charset
        bool NoNextChar=false;
        // Skip display of next char, for double-width
        EmptyNextLine=false;
        // Skip next line, for double-height

        cTeletextChar c;
        // auto.initialized to everything off
        c.SetFGColor(ttcWhite);
        c.SetBGColor(ttcBlack);
        c.SetCharset(FirstG0);
        
        if (y==0 && (Flags&0x10)) {
            c.SetBoxedOut(true);    
        }
        if (Flags&0x60) {
            c.SetBoxedOut(true);    
        }

        // Pre-scan for double-height and double-size codes
        for (x=0;x<40;x++) {
            if (y==0 && x<8) x=8;
            if ((PageCode[x+40*y] & 0x7f)==0x0D || (PageCode[x+40*y] & 0x7f)==0x0F)
                EmptyNextLine=true;
        }

        // Move through line
        for (x=0;x<40;x++) {
            unsigned char ttc=PageCode[x+40*y] & 0x7f;
            // skip parity check

            if (y==0 && x<8) continue;
            // no displayable data here...
            
/*          // Debug only: Output line data and spacing codes
            if (y==6) {
                if (ttc<0x20)
                    printf("%s ",names[ttc]);
                else
                    printf("%02x ",ttc);
                if (x==39) printf("\n");
            }
*/          
            
            // Handle all 'Set-At' spacing codes
            switch (ttc) {
            case 0x09: // Steady
                c.SetBlink(false);
                break;
            case 0x0C: // Normal Size
                if (Size!=sizeNormal) {
                    Size=sizeNormal;
                    HoldMosaicChar=' ';
                    HoldMosaicCharset=FirstG0;
                }                   
                break;
            case 0x18: // Conceal
                c.SetConceal(true);
                break;
            case 0x19: // Contiguous Mosaic Graphics
                SeparateGraphics=false;
                if (GraphicCharset)
                    c.SetCharset(CHARSET_GRAPHICS_G1);
                break;
            case 0x1A: // Separated Mosaic Graphics
                SeparateGraphics=true;
                if (GraphicCharset)
                    c.SetCharset(CHARSET_GRAPHICS_G1_SEP);
                break;
            case 0x1C: // Black Background
                c.SetBGColor(ttcBlack);
                break;
            case 0x1D: // New Background
                c.SetBGColor(c.GetFGColor());
                break;
            case 0x1E: // Hold Mosaic
                HoldMosaics=true;               
                break;
            }

            // temporary copy of character data:
            cTeletextChar c2=c;
            // c2 will be text character or space character or hold mosaic
            // c2 may also have temporary flags or charsets
            
            if (ttc<0x20) {
                // Spacing code, display space or hold mosaic
                if (HoldMosaics) {
                    c2.SetChar(HoldMosaicChar);
                    c2.SetCharset(HoldMosaicCharset);
                } else {
                    c2.SetChar(' ');
                }
            } else {
                // Character code               
                c2.SetChar(ttc);
                if (GraphicCharset) {
                    if (ttc&0x20) {
                        // real graphics code, remember for HoldMosaics
                        HoldMosaicChar=ttc;
                        HoldMosaicCharset=c.GetCharset();
                    } else {
                        // invalid code, pass-through to G0
                        c2.SetCharset(SecondCharset?SecondG0:FirstG0);
                    }   
                }
            }
            
            // Handle double-height and double-width extremes
            if (y>=23) {
                if (Size==sizeDoubleHeight) Size=sizeNormal;
                if (Size==sizeDoubleSize) Size=sizeDoubleWidth;
            }
            if (x>=38) {
                if (Size==sizeDoubleWidth) Size=sizeNormal;
                if (Size==sizeDoubleSize) Size=sizeDoubleHeight;
            }
            
            // Now set character code
            
            if (NoNextChar) {
                // Suppress this char due to double width last char
                NoNextChar=false;
            } else {
                switch (Size) {
                case sizeNormal:
                    // Normal sized
                    SetChar(x,y,c2);
                    if (EmptyNextLine && y<23) {
                        // Clean up next line
                        SetChar(x,y+1,c2.ToChar(' ').ToCharset(FirstG0));
                    }
                    break;
                case sizeDoubleWidth:
                    // Double width
                    SetChar(x,y,c2.ToDblWidth(dblw_Left));
                    SetChar(x+1,y,c2.ToDblWidth(dblw_Right));
                    if (EmptyNextLine && y<23) {
                        // Clean up next line
                        SetChar(x  ,y+1,c2.ToChar(' ').ToCharset(FirstG0));
                        SetChar(x+1,y+1,c2.ToChar(' ').ToCharset(FirstG0));
                    }
                    NoNextChar=true;
                    break;
                case sizeDoubleHeight:
                    // Double height
                    SetChar(x,y,c2.ToDblHeight(dblh_Top));
                    SetChar(x,y+1,c2.ToDblHeight(dblh_Bottom));
                    break;
                case sizeDoubleSize:
                    // Double Size
                    SetChar(x  ,  y,c2.ToDblHeight(dblh_Top   ).ToDblWidth(dblw_Left ));
                    SetChar(x+1,  y,c2.ToDblHeight(dblh_Top   ).ToDblWidth(dblw_Right));
                    SetChar(x  ,y+1,c2.ToDblHeight(dblh_Bottom).ToDblWidth(dblw_Left ));
                    SetChar(x+1,y+1,c2.ToDblHeight(dblh_Bottom).ToDblWidth(dblw_Right));
                    NoNextChar=true;
                    break;
                }
            }
                
            // Handle all 'Set-After' spacing codes
            switch (ttc) {
            case 0x00 ... 0x07: // Set FG color
                if (GraphicCharset) {
                    // Actual switch from graphics charset
                    HoldMosaicChar=' ';
                    HoldMosaicCharset=FirstG0;
                }
                c.SetFGColor((enumTeletextColor)ttc);
                c.SetCharset(SecondCharset?SecondG0:FirstG0);
                GraphicCharset=false;
                c.SetConceal(false);
                break;
            case 0x08: // Flash
                c.SetBlink(true);
                break;
            case 0x0A: // End Box
                c.SetBoxedOut(true);
                break;
            case 0x0B: // Start Box
                c.SetBoxedOut(false);
                break;
            case 0x0D: // Double Height
                if (Size!=sizeDoubleHeight) {
                    Size=sizeDoubleHeight;
                    HoldMosaicChar=' ';
                    HoldMosaicCharset=FirstG0;
                }                   
                break;
            case 0x0E: // Double Width
                if (Size!=sizeDoubleWidth) {
                    Size=sizeDoubleWidth;
                    HoldMosaicChar=' ';
                    HoldMosaicCharset=FirstG0;
                }                   
                break;
            case 0x0F: // Double Size
                if (Size!=sizeDoubleSize) {
                    Size=sizeDoubleSize;
                    HoldMosaicChar=' ';
                    HoldMosaicCharset=FirstG0;
                }                   
                break;
            case 0x10 ... 0x17: // Mosaic FG Color
                if (!GraphicCharset) {
                    // Actual switch to graphics charset
                    HoldMosaicChar=' ';
                    HoldMosaicCharset=FirstG0;
                }
                c.SetFGColor((enumTeletextColor)(ttc-0x10));
                c.SetCharset(SeparateGraphics?CHARSET_GRAPHICS_G1_SEP:CHARSET_GRAPHICS_G1);
                GraphicCharset=true;
                c.SetConceal(false);
                break;
            case 0x1B: // ESC Switch
                SecondCharset=!SecondCharset;
                if (!GraphicCharset) c.SetCharset(SecondCharset?SecondG0:FirstG0);
                break;
            case 0x1F: // Release Mosaic
                HoldMosaics=false;
                break;
            }
        } // end for x
    } // end for y
    
    for (x=0;x<40;x++) {
        // Clean out last line
        cTeletextChar c;
        c.SetFGColor(ttcWhite);
        c.SetBGColor(ttcBlack);
        c.SetCharset(FirstG0);
        c.SetChar(' ');
        if (Flags&0x60) {
            c.SetBoxedOut(true);    
        }
        SetChar(x,24,c);
    }       
}


