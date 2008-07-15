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

#include "include.h"
#include "DirectInput.h"

CDirectInput g_directInput;

CDirectInput::CDirectInput()
{
  m_lpdi = NULL;
  m_initialized = false;
}

CDirectInput::~CDirectInput()
{
  if (m_lpdi)
    m_lpdi->Release();
  m_lpdi = NULL;
}

HRESULT CDirectInput::Initialize(HWND hWnd)
{
  if (m_initialized)
    return S_OK;

  if (FAILED(DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, 
    IID_IDirectInput8, (void**)&m_lpdi, NULL)))
    return -1;
  
  m_initialized = true;
  return S_OK;
}

LPDIRECTINPUT CDirectInput::Get() const
{
  return m_lpdi;
}
