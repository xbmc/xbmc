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

#include "FileCueReader.h"


bool FileCueReader::isValid() const
{
  return m_opened;
}

FileCueReader::FileCueReader(const CStdString &strFile)
{
  m_opened = m_file.Open(strFile);
}

bool FileCueReader::ReadNextLine(CStdString &szLine)
{
  char *pos;
  // Read the next line.
  while (m_file.ReadString(m_szBuffer, 1023)) // Bigger than MAX_PATH_SIZE, for usage with relax!
  {
    // Remove the white space at the beginning of the line.
    pos = m_szBuffer;
    while (pos && skipChar(*pos)) pos++;
    if (pos)
    {
      szLine = pos;
      return true;
    }
    // If we are here, we have an empty line so try the next line
  }
  return false;
}

FileCueReader::~FileCueReader()
{
  if (m_opened)
    m_file.Close();
}