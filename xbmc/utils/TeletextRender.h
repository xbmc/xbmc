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

#ifndef TELETEXT_TXTRENDER_H_
#define TELETEXT_TXTRENDER_H_

#include <stdio.h>

// Teletext character sets
enum enumCharsets {
    CHARSET_LATIN_G0          = 0x0000, // native latin (partially todo)
    CHARSET_LATIN_G0_CZ_SK    = 0x0100, // Czech/Slovak (todo)
    CHARSET_LATIN_G0_EN       = 0x0200, // English
    CHARSET_LATIN_G0_EE       = 0x0300, // Estonian (todo)
    CHARSET_LATIN_G0_FR       = 0x0400, // French
    CHARSET_LATIN_G0_DE       = 0x0500, // German
    CHARSET_LATIN_G0_IT       = 0x0600, // Italian
    CHARSET_LATIN_G0_LV_LT    = 0x0700, // Lettish/Lithuanian (todo)
    CHARSET_LATIN_G0_PL       = 0x0800, // Polish (todo)
    CHARSET_LATIN_G0_PT_ES    = 0x0900, // Portugese/Spanish
    CHARSET_LATIN_G0_RO       = 0x0A00, // Romanian (todo)
    CHARSET_LATIN_G0_SR_HR_SL = 0x0B00, // Serbian/Croatian/Slovenian (todo)
    CHARSET_LATIN_G0_SV_FI    = 0x0C00, // Swedish/Finnish
    CHARSET_LATIN_G0_TR       = 0x0D00, // Turkish (todo)
    CHARSET_LATIN_G2          = 0x0E00, // Latin G2 supplementary set (todo)
    CHARSET_CYRILLIC_G0_SR_HR = 0x0F00, // Serbian/Croatian (todo)
    CHARSET_CYRILLIC_G0_RU_BG = 0x1000, // Russian/Bulgarian (todo)
    CHARSET_CYRILLIC_G0_UK    = 0x1100, // Ukrainian (todo)
    CHARSET_CYRILLIC_G2       = 0x1200, // Cyrillic G2 Supplementary (todo)
    CHARSET_GREEK_G0          = 0x1300, // Greek G0 (todo)
    CHARSET_GREEK_G2          = 0x1400, // Greeek G2 (todo)
    CHARSET_ARABIC_G0         = 0x1500, // Arabic G0 (todo)
    CHARSET_ARABIC_G2         = 0x1600, // Arabic G2 (todo)
    CHARSET_HEBREW_G0         = 0x1700, // Hebrew G0 (todo)
    CHARSET_GRAPHICS_G1       = 0x1800, // G1 graphics set
    CHARSET_GRAPHICS_G1_SEP   = 0x1900, // G1 graphics set, separated
    CHARSET_GRAPHICS_G3       = 0x1A00, // G3 graphics set (todo)
    CHARSET_INVALID           = 0x1F00  // no charset defined
};

// Macro to get the lowest non-0 bit position from a bit mask
// Should evaluate to const on a const mask
#define LowestSet2Bit(mask) ((mask)&0x0001?0:1)
#define LowestSet4Bit(mask) ((mask)&0x0003?LowestSet2Bit(mask):LowestSet2Bit((mask)>>2)+2)
#define LowestSet8Bit(mask) ((mask)&0x000f?LowestSet4Bit(mask):LowestSet4Bit((mask)>>4)+4)
#define LowestSet16Bit(mask) ((mask)&0x00ff?LowestSet8Bit(mask):LowestSet8Bit((mask)>>8)+8)
#define LowestSet32Bit(mask) ((mask)&0xffff?LowestSet16Bit(mask):LowestSet16Bit((mask)>>16)+16)


// Character modifcation double height:
enum enumDblHeight {
    dblh_Normal=0x00000000, // normal height
    dblh_Top   =0x04000000, // upper half character
    dblh_Bottom=0x08000000  // lower half character
};
// Character modifcation double width:
enum enumDblWidth {
    dblw_Normal=0x00000000, // normal width
    dblw_Left  =0x10000000, // left half character
    dblw_Right =0x20000000  // right half character
};

// Teletext colors
enum enumTeletextColor {
    // level 1:
    ttcBlack=0,
    ttcRed=1,
    ttcGreen=2,
    ttcYellow=3,
    ttcBlue=4,
    ttcMagenta=5,
    ttcCyan=6,
    ttcWhite=7,
    // level 2.5:
    ttcTransparent=8,
    ttcHalfRed=9,
    ttcHalfGreen=10,
    ttcHalfYellow=11,
    ttcHalfBlue=12,
    ttcHalfMagenta=13,
    ttcHalfCyan=14,
    ttcGrey=15,
    // unnamed, level 2.5:
    ttcColor16=16, ttcColor17=17, ttcColor18=18, ttcColor19=19,
    ttcColor20=20, ttcColor21=21, ttcColor22=22, ttcColor23=23,
    ttcColor24=24, ttcColor25=25, ttcColor26=26, ttcColor27=27,
    ttcColor28=28, ttcColor29=29, ttcColor30=30, ttcColor31=31,
    
    ttcFirst=0, ttcLast=31
};
inline enumTeletextColor& operator++(enumTeletextColor& c) { return c=enumTeletextColor(int(c)+1); }
inline enumTeletextColor operator++(enumTeletextColor& c, int) { enumTeletextColor tmp(c); ++c; return tmp; }
    
class cTeletextChar {
    // Wrapper class that represents a teletext character,
    // including colors and effects. Should optimize back
    // to 4 byte unsigned int on compile.
    
protected:
    unsigned int c;

    static const unsigned int CHAR             = 0x000000FF;
    // character code
    static const unsigned int CHARSET          = 0x00001F00;
    // character set code, see below
    static const unsigned int BOXOUT           = 0x00004000;
    // 'boxed' mode hidden area
    static const unsigned int DIRTY            = 0x00008000;
    // 'dirty' bit - internal marker only
    static const unsigned int FGCOLOR          = 0x001F0000;
    // 5-bit foreground color code, 3 bit used for now
    static const unsigned int BGCOLOR          = 0x03E00000;
    // 5-bit background color code, 3 bit used for now
    static const unsigned int DBLHEIGHT        = 0x0C000000;
    // show double height
    static const unsigned int DBLWIDTH         = 0x30000000;
    // show double width (todo)
    static const unsigned int CONCEAL          = 0x40000000;
    // character concealed
    static const unsigned int BLINK            = 0x80000000;
    // blinking character

    cTeletextChar(unsigned int cc) { c=cc; }

public:
    cTeletextChar() { c=0; }
    
    // inline helper functions:
    // For each parameter encoded into the 32-bit int, there is
    // a Get...() to read, a Set...() to write, and a To...() to
    // return a modified copy
    
    inline unsigned char GetChar() 
        { return c&CHAR; }
    inline void SetChar(unsigned char chr)
        { c=(c&~CHAR)|chr; }
    inline cTeletextChar ToChar(unsigned char chr)
        { return cTeletextChar((c&~CHAR)|chr); }
        
    inline enumCharsets GetCharset() 
        { return (enumCharsets)(c&CHARSET); }
    inline void SetCharset(enumCharsets charset) 
        { c=(c&~CHARSET)|charset; }
    inline cTeletextChar ToCharset(enumCharsets charset) 
        { return cTeletextChar((c&~CHARSET)|charset); }
    
    inline enumTeletextColor GetFGColor() 
        { return (enumTeletextColor)((c&FGCOLOR) >> LowestSet32Bit(FGCOLOR)); }
    inline void SetFGColor(enumTeletextColor fgc) 
        { c=(c&~FGCOLOR) | (fgc << LowestSet32Bit(FGCOLOR)); }
    inline cTeletextChar ToFGColor(enumTeletextColor fgc) 
        { return cTeletextChar((c&~FGCOLOR) | (fgc << LowestSet32Bit(FGCOLOR))); }
    
    inline enumTeletextColor GetBGColor() 
        { return (enumTeletextColor)((c&BGCOLOR) >> LowestSet32Bit(BGCOLOR)); }
    inline void SetBGColor(enumTeletextColor bgc) 
        { c=(c&~BGCOLOR) | (bgc << LowestSet32Bit(BGCOLOR)); }
    inline cTeletextChar ToBGColor(enumTeletextColor bgc) 
        { return cTeletextChar((c&~BGCOLOR) | (bgc << LowestSet32Bit(BGCOLOR))); }
    
    inline bool GetBoxedOut() 
        { return c&BOXOUT; }
    inline void SetBoxedOut(bool BoxedOut) 
        { c=(BoxedOut)?(c|BOXOUT):(c&~BOXOUT); }
    inline cTeletextChar ToBoxedOut(bool BoxedOut) 
        { return cTeletextChar((BoxedOut)?(c|BOXOUT):(c&~BOXOUT)); }
    
    inline bool GetDirty() 
        { return c&DIRTY; }
    inline void SetDirty(bool Dirty) 
        { c=(Dirty)?(c|DIRTY):(c&~DIRTY); }
    inline cTeletextChar ToDirty(bool Dirty) 
        { return cTeletextChar((Dirty)?(c|DIRTY):(c&~DIRTY)); }
    
    inline enumDblHeight GetDblHeight() 
        { return (enumDblHeight)(c&DBLHEIGHT); }
    inline void SetDblHeight(enumDblHeight dh) 
        { c=(c&~(DBLHEIGHT)) | dh; }
    inline cTeletextChar ToDblHeight(enumDblHeight dh) 
        { return cTeletextChar((c&~(DBLHEIGHT)) | dh); }
    
    inline enumDblWidth GetDblWidth() 
        { return (enumDblWidth)(c&DBLWIDTH); }
    inline void SetDblWidth(enumDblWidth dw) 
        { c=(c&~(DBLWIDTH)) | dw; }
    inline cTeletextChar ToDblWidth(enumDblWidth dw) 
        { return cTeletextChar((c&~(DBLWIDTH)) | dw); }
    
    inline bool GetConceal() 
        { return c&CONCEAL; }
    inline void SetConceal(bool Conceal) 
        { c=(Conceal)?(c|CONCEAL):(c&~CONCEAL); }
    inline cTeletextChar ToConceal(bool Conceal) 
        { return cTeletextChar((Conceal)?(c|CONCEAL):(c&~CONCEAL)); }
    
    inline bool GetBlink() 
        { return c&BLINK; }
    inline void SetBlink(bool Blink) 
        { c=(Blink)?(c|BLINK):(c&~BLINK); }
    inline cTeletextChar ToBlink(bool Blink) 
        { return cTeletextChar((Blink)?(c|BLINK):(c&~BLINK)); }
        
    bool operator==(cTeletextChar &chr) { return c==chr.c; }
    bool operator!=(cTeletextChar &chr) { return c!=chr.c; }
};


class cRenderTTPage {
    // Abstraction of a 40x25 teletext character page
    // with all special attributes and colors
    // Additionally tracks changes by maintaining a 
    // 'dirty' flag on each character
    
protected:
    cTeletextChar Page[40][25];
    
    int Flags;
    //   0x80  C4 - Erase page
    //   0x40  C5 - News flash
    //   0x20  C6 - Subtitle
    //   0x10  C7 - Suppress Header
    //   0x08  C8 - Update
    //   0x04  C9 - Interrupt Sequence
    //   0x02  C10 - Inhibit Display
    //   0x01  C11 - Magazine Serial mode
    
    int Lang;
    // 3-bit language number from headerz
    
    bool Dirty;    // At least one character is dirty
    bool DirtyAll; // Consider all characters dirty, regardless of flag

    // Font Code pages
    int FirstG0CodePage;  // 7-bit number, lower 3 bits ignored
    int SecondG0CodePage; // 7-bit number

public: 
    cRenderTTPage();

    cTeletextChar GetChar(int x, int y) {
        // Read character content from page
        if (x<0 || x>=40 || y<0 || y>=25) {
            printf("Warning: out of bounds read access to teletext page\n");
            return cTeletextChar();
        }
        return Page[x][y].ToDirty(false);
    }

    bool IsDirty() {
        // global dirty status
        return Dirty;   
    }
    
    bool IsDirty(int x, int y) {
        // local dirty status
        if (x<0 || x>=40 || y<0 || y>=25) {
            printf("Warning: out of bounds read access to teletext page\n");
            return false;
        }
        return Page[x][y].GetDirty() | DirtyAll;    
    }

    void MakeDirty(int x, int y) {
        // force one character dirty
        if (x<0 || x>=40 || y<0 || y>=25) {
            printf("Warning: out of bounds write access to teletext page\n");
            return;
        }
        Page[x][y].SetDirty(true);
        Dirty=true;
    }
    
    void SetChar(int x, int y, cTeletextChar c) {
        // Set character at given location
        
        if (x<0 || x>=40 || y<0 || y>=25) {
            printf("Warning: out of bounds write access to teletext page\n");
            return;
        }
        if (GetChar(x,y) != c) {
            Page[x][y]=c.ToDirty(true);
            Dirty=true; 
        }           
    }

    void ReadTeletextHeader(unsigned char *Header);
    // Read header from teletext page
    // Header must be a 12 bytes buffer

    void RenderTeletextCode(unsigned char *PageCode);
    // Interprete teletext code referenced by PageCode
    // and draw the whole page content into this object
    // PageCode must be a 40*24 bytes buffer
};

#endif
