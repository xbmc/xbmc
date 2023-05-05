/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "TextureFormats.h"

#include <stdint.h>
#include <string>

class CDDSImage
{
public:
  CDDSImage();
  CDDSImage(unsigned int width, unsigned int height, XB_FMT format);
  ~CDDSImage();

  unsigned int GetWidth() const;
  unsigned int GetHeight() const;
  XB_FMT GetFormat() const;
  unsigned int GetSize() const;
  unsigned char *GetData() const;

  bool ReadFile(const std::string &file);

private:
  void Allocate(unsigned int width, unsigned int height, XB_FMT format);
  static const char* GetFourCC(XB_FMT format);

  static unsigned int GetStorageRequirements(unsigned int width,
                                             unsigned int height,
                                             XB_FMT format);
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
