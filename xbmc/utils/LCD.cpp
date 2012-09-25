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
