/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUIFont.h
\brief
*/

#include "D3DResource.h"
#include "GUIFontTTF.h"

#include <list>
#include <memory>
#include <vector>

#include <wrl/client.h>

/*!
 \ingroup textures
 \brief
 */
class CGUIFontTTFDX : public CGUIFontTTF, public ID3DResource
{
public:
  explicit CGUIFontTTFDX(const std::string& fontIdent);
  virtual ~CGUIFontTTFDX(void);

  bool FirstBegin() override;
  void LastEnd() override;
  CVertexBuffer CreateVertexBuffer(const std::vector<SVertex>& vertices) const override;
  void DestroyVertexBuffer(CVertexBuffer& bufferHandle) const override;

  void OnDestroyDevice(bool fatal) override;
  void OnCreateDevice() override;

  static void CreateStaticIndexBuffer(void);
  static void DestroyStaticIndexBuffer(void);

protected:
  std::unique_ptr<CTexture> ReallocTexture(unsigned int& newHeight) override;
  bool CopyCharToTexture(FT_BitmapGlyph bitGlyph,
                         unsigned int x1,
                         unsigned int y1,
                         unsigned int x2,
                         unsigned int y2) override;
  void DeleteHardwareTexture() override;

private:
  bool UpdateDynamicVertexBuffer(const SVertex* pSysMem, unsigned int count);
  static void AddReference(CGUIFontTTFDX* font, CD3DBuffer* pBuffer);
  static void ClearReference(CGUIFontTTFDX* font, CD3DBuffer* pBuffer);

  unsigned m_vertexWidth{0};
  std::unique_ptr<CD3DTexture> m_speedupTexture; // extra texture to speed up reallocations
  Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;
  std::list<CD3DBuffer*> m_buffers;

  static bool m_staticIndexBufferCreated;
  static Microsoft::WRL::ComPtr<ID3D11Buffer> m_staticIndexBuffer;
};
