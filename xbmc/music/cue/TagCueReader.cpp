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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "TagCueReader.h"

TagCueReader::TagCueReader(const CStdString &cueData)
  : m_data(cueData)
  , m_currentPos(0)
{
}

bool TagCueReader::isValid() const
{
  return !m_data.IsEmpty();
}


bool TagCueReader::ReadNextLine(CStdString &line)
{
  // Read the next line.
  line = "";
  bool stop = false;
  while (m_currentPos < m_data.length())
  {
    // Remove the white space at the beginning of the line.
    char ch = m_data.at(m_currentPos++);
    stop |= (!skipChar(ch));
    if (stop)
    {
      if (ch == '\r' || ch == '\n')
      {
        if (!line.IsEmpty())
          return true;
      }
      else
        line += ch;
    }
  }
  return false;
}

TagCueReader::~TagCueReader()
{
}
