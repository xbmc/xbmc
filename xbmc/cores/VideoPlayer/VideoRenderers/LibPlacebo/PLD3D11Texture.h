/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <d3d11.h>
#include <libplacebo/d3d11.h>
#include <libplacebo/gpu.h>

namespace PL
{
class CPLD3D11Texture
{
public:
    CPLD3D11Texture(ID3D11Texture2D *tex, UINT plane = 0);
    CPLD3D11Texture(ID3D11Resource* res, UINT plane = 0);
    ~CPLD3D11Texture();
    

    // non-copyable
    CPLD3D11Texture(const CPLD3D11Texture&) = delete;
    CPLD3D11Texture& operator=(const CPLD3D11Texture&) = delete;

    pl_tex get() const { return m_tex[0]; }
    pl_tex getTexY() const { return m_tex[0]; }
    pl_tex getTexUV() const { return m_tex[1]; }

    bool valid() const { return m_tex != nullptr; }

private:
  // Size of the texture. 1D textures require h=d=1, 2D textures require d=1.
  int w, h, d;

    void cleanup();

    pl_tex m_tex[4];
};



}