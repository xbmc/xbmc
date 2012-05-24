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

#include "CueSheetIndexList.h"

void CueSheetIndexList::setIndex(unsigned index, unsigned value)
{
  m_positions[index] = value;
}

CueSheetIndexList::CueSheetIndexList()
{
  reset();
}


CueSheetIndexList::~CueSheetIndexList()
{
}

void CueSheetIndexList::reset()
{
  for(unsigned n=0; n<count; n++)
    m_positions[n]=0;
}

bool CueSheetIndexList::isEmpty() const
{
  for(unsigned n=0; n < count; n++)
  {
    if (m_positions[n] != m_positions[1])
      return false;
  }
	return true;
}

bool CueSheetIndexList::isValid() const
{
  if (m_positions[1] < m_positions[0])
    return false;
	for(unsigned n = 2; n < count && m_positions[n] > 0; n++)
  {
		if (m_positions[n] < m_positions[n-1])
      return false;
	}
	return true;
}

bool CueSheetIndexList::updatePreGap(unsigned pregap)
{
  if (pregap && m_positions[0] == 0)
    m_positions[0] = m_positions[1] - pregap;
  return isValid();
}

unsigned CueSheetIndexList::start() const
{
  return m_positions[1];
}

unsigned CueSheetIndexList::pregap() const
{
  if (m_positions[0] > 0)
    return m_positions[1] - m_positions[0];
  return m_positions[0];
}
