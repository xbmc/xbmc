/*!
\file Surface.h
\brief
*/



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
#include "stdafx.h"
#include "WinSystemOSXGL.h"
#include "CocoaInterface.h"

CWinSystemOSXGL g_Windowing;


CWinSystemOSXGL::CWinSystemOSXGL()
{

}

CWinSystemOSXGL::~CWinSystemOSXGL()
{

}

bool CWinSystemOSXGL::InitRenderSystem()
{
  return CRenderSystemGL::InitRenderSystem();
}

bool CWinSystemOSXGL::PresentRender()
{
  if(!m_bWindowCreated || !m_bRenderCreated)
    return false;

  Cocoa_GL_SwapBuffers(m_glContext);
  
  return true;
}

