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

#include <string>
#include <stdint.h>

class CDDSImage
{
public:
  CDDSImage();
  CDDSImage(unsigned int width, unsigned int height, unsigned int format);
  ~CDDSImage();

  unsigned int GetWidth() const;
  unsigned int GetHeight() const;
  unsigned int GetFormat() const;
  unsigned int GetSize() const;
  unsigned char *GetData() const;

  bool ReadFile(const std::string &file);

private:
  void Allocate(unsigned int width, unsigned int height, unsigned int format);
  static const char *GetFourCC(unsigned int format);

  static unsigned int GetStorageRequirements(unsigned int width, unsigned int height, unsigned int format);
  enum {
    ddsd_caps        = 0x00000001,
    ddsd_height      = 0x00000002,
    ddsd_width       = 0x00000004,
    ddsd_pitch       = 0x00000008,
    ddsd_pixelformat = 0x00001000,
    ddsd_mipmapcount = 0x00020000,
    ddsd_linearsize  = 0x00080000,
    ddsd_depth       = 0x00800000
  };

  enum {
    ddpf_alphapixels = 0x00000001,
    ddpf_fourcc      = 0x00000004,
    ddpf_rgb         = 0x00000040
  };

  enum {
    ddscaps_complex = 0x00000008,
    ddscaps_texture = 0x00001000,
    ddscaps_mipmap  = 0x00400000
  };

  #pragma pack(push, 2)
  typedef struct
  {
    uint32_t size;
    uint32_t flags;
    uint32_t fourcc;
    uint32_t rgbBitCount;
    uint32_t rBitMask;
    uint32_t gBitMask;
    uint32_t bBitMask;
    uint32_t aBitMask;
  } ddpixelformat;

#define DDPF_ALPHAPIXELS 0x00000001
#define DDPF_ALPHA       0x00000002
#define DDPF_FOURCC      0x00000004
#define DDPF_RGB         0x00000040
#define DDPF_YUV         0x00000200
#define DDPF_LUMINANCE   0x00020000

  typedef struct
  {
    uint32_t flags1;
    uint32_t flags2;
    uint32_t reserved[2];
  } ddcaps2;

  typedef struct
  {
    uint32_t      size;
    uint32_t      flags;
    uint32_t      height;
    uint32_t      width;
    uint32_t      linearSize;
    uint32_t      depth;
    uint32_t      mipmapcount;
    uint32_t      reserved[11];
    ddpixelformat pixelFormat;
    ddcaps2       caps;
    uint32_t      reserved2;
  } ddsurfacedesc2;
  #pragma pack(pop)

  ddsurfacedesc2 m_desc;
  unsigned char *m_data;
};
