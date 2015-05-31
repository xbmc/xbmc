/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
 *
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

#ifdef HAS_DS_PLAYER

#include "MadvrCallback.h"

CMadvrCallback *CMadvrCallback::m_pSingleton = NULL;

CMadvrCallback::CMadvrCallback()
{
  m_pMadvr = NULL;  
  m_renderOnMadvr = false;
  m_isInitMadvr = false;
}

CMadvrCallback::~CMadvrCallback()
{
}

CMadvrCallback* CMadvrCallback::Get()
{
  //return (m_pSingleton) ? m_pSingleton : (m_pSingleton = new CMadvrCallback());
  if (m_pSingleton)
    return m_pSingleton;
  else
    return m_pSingleton = new CMadvrCallback();
}


bool CMadvrCallback::UsingMadvr()
{
  if (m_pMadvr == NULL)
    return false;
  return true;
}

bool CMadvrCallback::ReadyMadvr()
{
  return ((m_pMadvr != NULL) && m_renderOnMadvr);
}

bool CMadvrCallback::IsEnteringExclusiveMadvr()
{
  return ((m_pMadvr != NULL) && CMadvrCallback::Get()->GetCallback()->IsEnteringExclusive());
}

#endif