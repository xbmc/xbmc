#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
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
 
#ifndef HAS_DS_PLAYER
#error DSPlayer's header file included without HAS_DS_PLAYER defined
#endif

#include "ExternalPixelShader.h"
#include "PixelShaderCompiler.h"
#include "threads/SingleLock.h"

typedef std::vector<CExternalPixelShader *> PixelShaderVector;

class CCriticalSection;
bool HasSameIndex(uint32_t index, CExternalPixelShader* p2);
bool HasSameID(uint32_t id, CExternalPixelShader* p2);

class CPixelShaderList
{
public:
  ~CPixelShaderList();

  void Load();
  void UpdateActivatedList();

  void SaveXML();
  void EnableShader(const uint32_t id, bool enabled = true);
  void DisableAll();
  
  bool Exists(const uint32_t id) { return (std::find_if(m_pixelShaders.begin(), m_pixelShaders.end(), std::bind1st(std::ptr_fun(HasSameID),
    id)) != m_pixelShaders.end()); }

  PixelShaderVector& GetPixelShaders() { return m_pixelShaders; }
  PixelShaderVector GetActivatedPixelShaders() { return m_activatedPixelShaders; }
  bool HasEnabledShaders() {
    CSingleLock lock(m_accessLock);
    return m_activatedPixelShaders.empty();
  }

  CCriticalSection m_accessLock;

private:
  PixelShaderVector m_activatedPixelShaders;
  PixelShaderVector m_pixelShaders;

  bool LoadXMLFile(const CStdString& xmlFile);
};