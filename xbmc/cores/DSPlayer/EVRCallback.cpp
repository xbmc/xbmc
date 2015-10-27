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

#include "EvrCallback.h"

CEvrCallback *CEvrCallback::m_pSingleton = NULL;

CEvrCallback::CEvrCallback()
{
  m_pAllocatorCallback = NULL;  
  m_pPaintCallback = NULL;
  m_renderOnEvr = false;
  ResetRenderCount();
  m_currentVideoLayer = EVR_LAYER_UNDER;
}

CEvrCallback::~CEvrCallback()
{
  m_pAllocatorCallback = NULL;
  m_pPaintCallback = NULL;
}

CEvrCallback* CEvrCallback::Get()
{
  return (m_pSingleton) ? m_pSingleton : (m_pSingleton = new CEvrCallback());
}

void CEvrCallback::IncRenderCount()
{ 
  if (!ReadyEvr())
    return;

  m_currentVideoLayer == EVR_LAYER_UNDER ? m_renderUnderCount += 1 : m_renderOverCount += 1;
}

void CEvrCallback::ResetRenderCount()
{
  m_renderUnderCount = 0;  
  m_renderOverCount = 0;
}

bool CEvrCallback::GuiVisible(EVR_RENDER_LAYER layer)
{
  bool result = false;
  switch (layer)
  {
  case EVR_LAYER_UNDER:
    result = m_renderUnderCount > 0;
    break;
  case EVR_LAYER_OVER:
    result = m_renderOverCount > 0;
    break;
  case EVR_LAYER_ALL:
    result = m_renderOverCount + m_renderUnderCount > 0;
    break;
  }
  return result;
}

bool CEvrCallback::UsingEvr()
{
  return (m_pAllocatorCallback != NULL);
}

bool CEvrCallback::ReadyEvr()
{
  return (m_pAllocatorCallback != NULL && m_renderOnEvr);
}

// IEvrAllocatorCallback

CRect  CEvrCallback::GetActiveVideoRect()
{
  CRect activeVideoRect(0, 0, 0, 0);

  if (UsingEvr())
    activeVideoRect = m_pAllocatorCallback->GetActiveVideoRect();

  return activeVideoRect;
}

// IEvrPaintCallback
void CEvrCallback::RenderToUnderTexture()
{
  if (m_pPaintCallback && ReadyEvr())
    m_pPaintCallback->RenderToUnderTexture();
}

void CEvrCallback::RenderToOverTexture()
{
  if (m_pPaintCallback && ReadyEvr())
    m_pPaintCallback->RenderToOverTexture();
}

void CEvrCallback::EndRender()
{
  if (m_pPaintCallback && ReadyEvr())
    m_pPaintCallback->EndRender();
}

#endif