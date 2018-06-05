/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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

#pragma once

/*!
\file GUIFont.h
\brief
*/

#include "D3DResource.h"
#include "GUIFontTTF.h"
#include <list>
#include <vector>
#include <wrl/client.h>

#define ELEMENT_ARRAY_MAX_CHAR_INDEX (2000)

/*!
 \ingroup textures
 \brief
 */
class CGUIFontTTFDX : public CGUIFontTTFBase, public ID3DResource
{
public:
  explicit CGUIFontTTFDX(const std::string& strFileName);
  virtual ~CGUIFontTTFDX(void);

  bool FirstBegin() override;
  void LastEnd() override;
  CVertexBuffer CreateVertexBuffer(const std::vector<SVertex> &vertices) const override;
  void DestroyVertexBuffer(CVertexBuffer &bufferHandle) const override;

  void OnDestroyDevice(bool fatal) override;
  void OnCreateDevice() override;

  static void CreateStaticIndexBuffer(void);
  static void DestroyStaticIndexBuffer(void);

protected:
  CBaseTexture* ReallocTexture(unsigned int& newHeight) override;
  bool CopyCharToTexture(FT_BitmapGlyph bitGlyph, unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2) override;
  void DeleteHardwareTexture() override;

private:
  bool UpdateDynamicVertexBuffer(const SVertex* pSysMem, unsigned int count);
  static void AddReference(CGUIFontTTFDX* font, CD3DBuffer* pBuffer);
  static void ClearReference(CGUIFontTTFDX* font, CD3DBuffer* pBuffer);

  unsigned m_vertexWidth;
  CD3DTexture* m_speedupTexture;  // extra texture to speed up reallocations
  Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;
  std::list<CD3DBuffer*> m_buffers;

  static bool m_staticIndexBufferCreated;
  static Microsoft::WRL::ComPtr<ID3D11Buffer> m_staticIndexBuffer;
};

