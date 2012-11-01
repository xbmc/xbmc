/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "LCD.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "CharsetConverter.h"
#include "log.h"
#include "XMLUtils.h"

using namespace std;

void ILCD::StringToLCDCharSet(CStdString& strText)
{

  //0 = HD44780, 1=KS0073
  unsigned int iLCDContr = 0;
  //the timeline is using blocks
  //a block is used at address 0xA0, smallBlocks at address 0xAC-0xAF

  unsigned char LCD[2][256] = {
                                { //HD44780 charset ROM code A00
                                  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
                                  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
                                  0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
                                  0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
                                  0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
                                  0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0xa4, 0x5d, 0x5e, 0x5f,
                                  0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
                                  0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0xb0, 0x20,
                                  0x20, 0x20, 0x2c, 0x20, 0x22, 0x20, 0x20, 0x20, 0x5e, 0x20, 0x53, 0x3c, 0x20, 0x20, 0x5a, 0x20,
                                  0x20, 0x27, 0x27, 0x22, 0x22, 0xa5, 0xb0, 0xb0, 0xb0, 0x20, 0x73, 0x3e, 0x20, 0x20, 0x7a, 0x59,
                                  0xff, 0x21, 0x20, 0x20, 0x20, 0x5c, 0x7c, 0x20, 0x22, 0x20, 0x20, 0xff, 0x0E, 0x0A, 0x09, 0x08, // Custom characters
                                  0xdf, 0x20, 0x20, 0x20, 0x27, 0xe4, 0x20, 0xa5, 0x20, 0x20, 0xdf, 0x3e, 0x20, 0x20, 0x20, 0x3f,
                                  0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x20, 0x43, 0x45, 0x45, 0x45, 0x45, 0x49, 0x49, 0x49, 0x49,
                                  0x44, 0x4e, 0x4f, 0x4f, 0x4f, 0x4f, 0x4f, 0x78, 0x30, 0x55, 0x55, 0x55, 0x55, 0x59, 0x20, 0xe2,
                                  0x61, 0x61, 0x61, 0x61, 0xe1, 0x61, 0x20, 0x63, 0x65, 0x65, 0x65, 0x65, 0x69, 0x69, 0x69, 0x69,
                                  0x6f, 0x6e, 0x6f, 0x6f, 0x6f, 0x6f, 0x6f, 0xfd, 0x6f, 0x75, 0x75, 0xfb, 0xf5, 0x79, 0x20, 0x79
                                },

                                { //KS0073
                                  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
                                  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
                                  0x20, 0x21, 0x22, 0x23, 0xa2, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
                                  0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
                                  0xa0, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
                                  0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0xfa, 0xfb, 0xfc, 0x1d, 0xc4,
                                  0x27, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
                                  0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0xfd, 0xfe, 0xff, 0xce, 0x20,
                                  0x20, 0x20, 0x2c, 0xd5, 0x22, 0x20, 0x20, 0x20, 0x1d, 0x20, 0xf3, 0x3c, 0x20, 0x20, 0xf4, 0x20,
                                  0x20, 0x27, 0x27, 0x22, 0x22, 0xdd, 0x2d, 0x2d, 0xce, 0x20, 0xf8, 0x3e, 0x20, 0x20, 0xf9, 0x59,
                                  0x1f, 0x40, 0xb1, 0xa1, 0x24, 0xa3, 0xfe, 0x5f, 0x20, 0x20, 0x20, 0xd0, 0xd1, 0xd2, 0xd3, 0xd4, // Custom characters
                                  0x80, 0x8c, 0x82, 0x83, 0x27, 0x8f, 0x20, 0xdd, 0x20, 0x81, 0x80, 0x15, 0x8b, 0x8a, 0xbe, 0x60,
                                  0xae, 0xe2, 0xae, 0xae, 0x5b, 0xae, 0xbc, 0xa9, 0xc5, 0xbf, 0xc6, 0x45, 0xcc, 0xe3, 0xcc, 0x49,
                                  0x44, 0x5d, 0x4f, 0xe0, 0xec, 0x4f, 0x5c, 0x78, 0xab, 0xee, 0xe5, 0xee, 0x5e, 0xe6, 0x20, 0xbe,
                                  0x7f, 0xe7, 0xaf, 0xaf, 0x7b, 0xaf, 0xbd, 0xc8, 0xa4, 0xa5, 0xc7, 0x65, 0xa7, 0xe8, 0x69, 0x69,
                                  0x6f, 0x7d, 0xa8, 0xe9, 0xed, 0x6f, 0x7c, 0x25, 0xac, 0xa6, 0xea, 0xef, 0x7e, 0xeb, 0x70, 0x79
                                }
                              };

  unsigned char cLCD;
  int iSize = (int)strText.size();

  for (int i = 0; i < iSize; ++i)
  {
    cLCD = strText.at(i);
    cLCD = LCD[iLCDContr][cLCD];
    strText.SetAt(i, cLCD);
  }
}

/* TEMPLATE: TRANSLATION-TABLE FOR FUTURE LCD-TYPE-CHARSETS
{
0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
}
*/

unsigned char ILCD::GetLCDCharsetCharacter( UINT _nCharacter, int _nCharset )
{
  unsigned char arrCharsets[CUSTOM_CHARSET_MAX][8][8] = { { // Xbmc default, currently implemented elsewhere
                                                            {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                                            {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                                            {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                                            {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                                            {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                                            {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                                            {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                                            {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                                          },
                                                          { // Small Char
                                                            {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01}, //    |    _
                                                            {0x1f, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x1f}, //   _     _|
                                                            {0x1f, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11}, //  | |
                                                            {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x1f}, //         _|
                                                            {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1f}, //  |_|    _
                                                            {0x1f, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1f}, //   _    |_
                                                            {0x1f, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01}, //    |    _
                                                            {0x1f, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1f}, //        |_|
                                                          },
                                                          { // Medium Char                                    //   _
                                                            {0x1f, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, //
                                                            {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x1f}, //         _
                                                            {0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x1f, 0x1f}, //  |_     _
                                                            {0x1f, 0x1f, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18}, //   _      |
                                                            {0x1f, 0x1f, 0x18, 0x18, 0x18, 0x18, 0x1f, 0x1f}, //  |_     _
                                                            {0x1f, 0x1f, 0x03, 0x03, 0x03, 0x03, 0x1f, 0x1f}, //  _      _|
                                                            {0x1f, 0x1f, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03}, //   |
                                                            {0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x1f, 0x1f}, //         _|
                                                          },
                                                          { // Big Char
                                                            {0x01, 0x03, 0x03, 0x07, 0x07, 0x0f, 0x0f, 0x1f}, // topleft corner
                                                            {0x10, 0x18, 0x18, 0x1c, 0x1c, 0x1e, 0x1e, 0x1f}, // topright corner
                                                            {0x1f, 0x1e, 0x1e, 0x1c, 0x1c, 0x18, 0x18, 0x10}, // bottomright corner
                                                            {0x1f, 0x0f, 0x0f, 0x07, 0x07, 0x03, 0x03, 0x01}, // bottomleft corner
                                                            {0x1f, 0x1f, 0x1f, 0x1f, 0x00, 0x00, 0x00, 0x00}, // upper half block
                                                            // Free to use
                                                            {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                                            {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                                            {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                                          } };
  if ( _nCharset == -1 )
    _nCharset = m_eCurrentCharset;


  if ( ( _nCharacter >= 64 ) || ( _nCharset >= CUSTOM_CHARSET_MAX ) )
    return 0;

  return arrCharsets[_nCharset][_nCharacter/8][_nCharacter%8];

}


CStdString ILCD::GetProgressBar(double tCurrent, double tTotal)
{
  unsigned char cLCDsmallBlocks = 0xb0; //this char (0xAC-0xAF) will be translated in LCD.cpp to the smallBlock
  unsigned char cLCDbigBlock = 0xab;  //this char will be translated in LCD.cpp to the right bigBlock
  int iBigBlock = 5;      // a big block is a combination of 5 small blocks
  int iColumns = GetColumns()-2;

  if (iColumns > 0)
  {
    double dBlockSize = tTotal * 0.99 / iColumns / iBigBlock; // mult with 0.99 to show the last bar

    CStdString strProgressBar = "[";
    for (int i = 1;i <= iColumns;i++)
    {
      //set full blocks
      if (tCurrent >= i * iBigBlock * dBlockSize)
      {
        strProgressBar += char(cLCDbigBlock);
      }
      //set a part of a block at the end, when needed
      else if (tCurrent > (i - 1) * iBigBlock * dBlockSize + dBlockSize)
      {
        double tmpTest = tCurrent - ((int)(tCurrent / (iBigBlock * dBlockSize)) * iBigBlock * dBlockSize );
        tmpTest = (tmpTest / dBlockSize);
        if (tmpTest >= iBigBlock) tmpTest = iBigBlock - 1;
        strProgressBar += char(cLCDsmallBlocks - (int)tmpTest);
      }
      //fill up the rest with blanks
      else
      {
        strProgressBar += " ";
      }
    }
    strProgressBar += "]";
    g_charsetConverter.unknownToUTF8(strProgressBar);
    return strProgressBar;
  }
  else return "";
}

void ILCD::SetCharset( UINT _nCharset )
{
  if ( _nCharset < CUSTOM_CHARSET_MAX )
    m_eCurrentCharset = _nCharset;
}


CStdString ILCD::GetBigDigit( UINT _nCharset, int _nDigit, UINT _nLine, UINT _nMinSize, UINT _nMaxSize, bool _bSpacePadding )
  // Get the partial digit(s) for the given line
  // that is needed to build a big character
{
  UINT nCurrentSize;
  UINT nCurrentValue;
  UINT nValue;
  CStdString strDigits;
  CStdString strCurrentDigit;



  // If the charset doesn't match our
  // custom chars, return with nothing
  if ( ( _nCharset == CUSTOM_CHARSET_DEFAULT ) // The XBMC 'icon' charset
    || ( _nCharset >= CUSTOM_CHARSET_MAX ) )
    return "";


  unsigned int arrSizes[CUSTOM_CHARSET_MAX][2] = { { 1, 1 },
                                                   { 1, 2 },
                                                   { 2, 2 },
                                                   { 3, 4 },
                                                 };

  // Return with nothing if the linenumber falls outside our char 'height'
  if ( _nLine > arrSizes[ _nCharset ][1] )
    return "";

  // Define the 2x1 line characters
  unsigned char arrMedNumbers[10][2][1] = { {{0x0a}, {0x0c}}, // 0
                                            {{0x08}, {0x08}}, // 1 // 0xaf
                                            {{0x0e}, {0x0d}}, // 2
                                            {{0x09}, {0x0b}}, // 3
                                            {{0x0c}, {0x08}}, // 4
                                            {{0x0d}, {0x0b}}, // 5
                                            {{0x0d}, {0x0c}}, // 6
                                            {{0x0e}, {0x08}}, // 7
                                            {{0x0f}, {0x0c}}, // 8
                                            {{0x0f}, {0x0b}}, // 9
                                          };

  // Define the 2x2 bold line characters
  unsigned char arrMedBoldNumbers[10][2][2] = { { {0x0b, 0x0e},    // 0
                                                  {0x0a, 0x0f}, }, // 0
                                                { {0x0e, 0x20},    // 1
                                                  {0x0f, 0x09}, }, // 1
                                                { {0x08, 0x0d},    // 2
                                                  {0x0c, 0x09}, }, // 2
                                                { {0x08, 0x0d},    // 3
                                                  {0x09, 0x0f}, }, // 3
                                                { {0x0a, 0x0f},    // 4
                                                  {0x20, 0x0e}, }, // 4
                                                { {0x0c, 0x08},    // 5
                                                  {0x09, 0x0d}, }, // 5
                                                { {0x0b, 0x08},    // 6
                                                  {0x0c, 0x0d}, }, // 6
                                                { {0x08, 0x0e},    // 7
                                                  {0x20, 0x0e}, }, // 7
                                                { {0x0c, 0x0d},    // 8
                                                  {0x0a, 0x0f}, }, // 8
                                                { {0x0c, 0x0d},    // 9
                                                  {0x09, 0x0f}, }, // 9
                                          };

  // Define the 4 line characters (this could be more readable, but that may take up to 3 screens)
  unsigned char arrBigNumbers[10][4][3] = { { {0x08, 0xa0, 0x09},    // 0
                                              {0xa0, 0x20, 0xa0},    // 0
                                              {0xa0, 0x20, 0xa0},    // 0
                                              {0x0b, 0xa0, 0x0a}, }, // 0
                                            { {0x08, 0xa0, 0x20},    // 1
                                              {0x20, 0xa0, 0x20},    // 1
                                              {0x20, 0xa0, 0x20},    // 1
                                              {0xa0, 0xa0, 0xa0}, }, // 1
                                            { {0x08, 0xa0, 0x09},    // 2
                                              {0x20, 0x08, 0x0a},    // 2
                                              {0x08, 0x0a, 0x20},    // 2
                                              {0xa0, 0xa0, 0xa0}, }, // 2
                                            { {0x08, 0xa0, 0x09},    // 3
                                              {0x20, 0x20, 0xa0},    // 3
                                              {0x20, 0x0c, 0xa0},    // 3
                                              {0x0b, 0xa0, 0x0a}, }, // 3
                                            { {0xa0, 0x20, 0xa0},    // 4
                                              {0xa0, 0xa0, 0xa0},    // 4
                                              {0x20, 0x20, 0xa0},    // 4
                                              {0x20, 0x20, 0xa0}, }, // 4
                                            { {0xa0, 0xa0, 0xa0},    // 5
                                              {0xa0, 0x20, 0x20},    // 5
                                              {0x20, 0x0c, 0xa0},    // 5
                                              {0xa0, 0xa0, 0x0a}, }, // 5
                                            { {0x08, 0xa0, 0x09},    // 6
                                              {0xa0, 0x20, 0x20},    // 6
                                              {0xa0, 0x0c, 0xa0},    // 6
                                              {0x0b, 0xa0, 0x0a}, }, // 6
                                            { {0xa0, 0xa0, 0xa0},    // 7
                                              {0x20, 0x20, 0xa0},    // 7
                                              {0x20, 0x20, 0xa0},    // 7
                                              {0x20, 0x20, 0xa0}, }, // 7
                                            { {0x08, 0xa0, 0x09},    // 8
                                              {0xa0, 0x20, 0xa0},    // 8
                                              {0xa0, 0x0c, 0xa0},    // 8
                                              {0x0b, 0xa0, 0x0a}, }, // 8
                                            { {0x08, 0xa0, 0x09},    // 9
                                              {0xa0, 0x20, 0xa0},    // 9
                                              {0x20, 0x0c, 0xa0},    // 9
                                              {0x0b, 0xa0, 0x0a}, }, // 9
                                          };


  if ( _nDigit < 0 )
  {
    // TODO: Add a '-' sign

    _nDigit = -_nDigit;
  }

  // Set the current size, and value (base numer)
  nCurrentSize = 1;
  nCurrentValue = 10;

  // Build the characters
  strDigits = "";

  while ( ( nCurrentSize <= _nMinSize ) || ( (UINT)_nDigit >= nCurrentValue && (nCurrentSize <= _nMaxSize || _nMaxSize == 0) )  )
  {
    // Determine current value
    nValue = ( _nDigit % nCurrentValue ) / ( nCurrentValue / 10 );

    // Reset current digit
    strCurrentDigit = "";
    for ( UINT nX = 0; nX < arrSizes[ _nCharset ][0]; nX++ )
    {
      // Add a space if we have more than one digit, and the given
      // digit is smaller than the current value (base numer) we are dealing with
      if ( _bSpacePadding && ((nCurrentValue / 10) > (UINT)_nDigit ) && ( nCurrentSize > 1 ) )
      {
        strCurrentDigit += " ";
      }
      // TODO: make sure this is not hardcoded
      else
      {
        switch ( _nCharset )
        {
        case CUSTOM_CHARSET_SMALLCHAR:
          strCurrentDigit += arrMedNumbers[ nValue ][ _nLine ][ nX ];
          break;
        case CUSTOM_CHARSET_MEDIUMCHAR:
          strCurrentDigit += arrMedBoldNumbers[ nValue ][ _nLine ][ nX ];
          break;
        case  CUSTOM_CHARSET_BIGCHAR:
          strCurrentDigit += arrBigNumbers[ nValue ][ _nLine ][ nX ];
          break;
        }
      }

    }
    // Add as partial string
    // Note that is it reversed, I.E. 'LSB' is added first
    strDigits = strCurrentDigit + strDigits;

    // Increase the size and base number
    nCurrentSize++;
    nCurrentValue *= 10;
  }

  // Update the character mode
  m_eCurrentCharset = _nCharset;

  // Return the created digit part
  return strDigits;
}

void ILCD::Initialize()
{
  CStdString lcdPath;
  lcdPath = g_settings.GetUserDataItem("LCD.xml");
  LoadSkin(lcdPath);
  m_eCurrentCharset = CUSTOM_CHARSET_DEFAULT;
  m_disableOnPlay = DISABLE_ON_PLAY_NONE;
  m_eCurrentCharset = CUSTOM_CHARSET_DEFAULT;

  // Big number blocks, used for screensaver clock
  // Note, the big block isn't here, it's in the LCD's ROM
}

bool ILCD::IsConnected()
{
  return true;
}

void ILCD::LoadSkin(const CStdString &xmlFile)
{
  Reset();

  bool condensed = TiXmlBase::IsWhiteSpaceCondensed();
  TiXmlBase::SetCondenseWhiteSpace(false);
  CXBMCTinyXML doc;
  if (!doc.LoadFile(xmlFile.c_str()))
  {
    CLog::Log(LOGERROR, "Unable to load LCD skin file %s", xmlFile.c_str());
    TiXmlBase::SetCondenseWhiteSpace(condensed);
    return;
  }

  TiXmlElement *element = doc.RootElement();
  if (!element || strcmp(element->Value(), "lcd") != 0)
  {
    TiXmlBase::SetCondenseWhiteSpace(condensed);
    return;
  }

  // load our settings
  CStdString disableOnPlay;
  XMLUtils::GetString(element, "disableonplay", disableOnPlay);
  if (disableOnPlay.Find("video") != -1)
    m_disableOnPlay |= DISABLE_ON_PLAY_VIDEO;
  if (disableOnPlay.Find("music") != -1)
    m_disableOnPlay |= DISABLE_ON_PLAY_MUSIC;

  TiXmlElement *mode = element->FirstChildElement();
  while (mode)
  {
    if (strcmpi(mode->Value(), "music") == 0)
    { // music mode
      LoadMode(mode, LCD_MODE_MUSIC);
    }
    else if (strcmpi(mode->Value(), "video") == 0)
    { // video mode
      LoadMode(mode, LCD_MODE_VIDEO);
    }
    else if (strcmpi(mode->Value(), "general") == 0)
    { // general mode
      LoadMode(mode, LCD_MODE_GENERAL);
    }
    else if (strcmpi(mode->Value(), "navigation") == 0)
    { // navigation mode
      LoadMode(mode, LCD_MODE_NAVIGATION);
    }
    else if (strcmpi(mode->Value(), "screensaver") == 0)
    { // screensaver mode
      LoadMode(mode, LCD_MODE_SCREENSAVER);
    }
    else if (strcmpi(mode->Value(), "xbelaunch") == 0)
    { // xbe launch mode
      LoadMode(mode, LCD_MODE_XBE_LAUNCH);
    }
    else if (strcmpi(mode->Value(), "pvrtv") == 0)
    { // pvr tv mode
      LoadMode(mode, LCD_MODE_PVRTV);
    }
    else if (strcmpi(mode->Value(), "pvrradio") == 0)
    { // pvr radio mode
      LoadMode(mode, LCD_MODE_PVRRADIO);
    }
    mode = mode->NextSiblingElement();
  }
  TiXmlBase::SetCondenseWhiteSpace(condensed);
}

void ILCD::LoadMode(TiXmlNode *node, LCD_MODE mode)
{
  if (!node) return;
  TiXmlNode *line = node->FirstChild("line");
  while (line)
  {
    if (line->FirstChild())
      m_lcdMode[mode].push_back(CGUIInfoLabel(line->FirstChild()->Value()));
    line = line->NextSibling("line");
  }
}

void ILCD::Reset()
{
  m_disableOnPlay = DISABLE_ON_PLAY_NONE;
  for (unsigned int i = 0; i < LCD_MODE_MAX; i++)
    m_lcdMode[i].clear();
}

void ILCD::Render(LCD_MODE mode)
{
  unsigned int outLine = 0;
  unsigned int inLine = 0;
  while (outLine < 4 && inLine < m_lcdMode[mode].size())
  {
    CStdString line = m_lcdMode[mode][inLine++].GetLabel(0);
    CGUITextLayout::Filter(line);
    if (!line.IsEmpty())
    {
      g_charsetConverter.utf8ToStringCharset(line);
      SetLine(outLine++, line);
    }
  }
  // fill remainder with empty space
  while (outLine < 4)
    SetLine(outLine++, "");
}

void ILCD::DisableOnPlayback(bool playingVideo, bool playingAudio)
{
  if ((playingVideo && (m_disableOnPlay & DISABLE_ON_PLAY_VIDEO)) ||
      (playingAudio && (m_disableOnPlay & DISABLE_ON_PLAY_MUSIC)))
    SetBackLight(0);
}
