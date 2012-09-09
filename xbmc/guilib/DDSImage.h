/*
 *      Copyright (C) 2005-2012 Team XBMC
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

  /*! \brief Create a DDS image file from the given an ARGB buffer
   \param file name of the file to write
   \param width width of the pixel buffer
   \param height height of the pixel buffer
   \param pitch pitch of the pixel buffer
   \param argb pixel buffer
   \param maxMSE maximum mean square error to allow, ignored if 0 (the default)
   \return true on successful image creation, false otherwise
   */
  bool Create(const std::string &file, unsigned int width, unsigned int height, unsigned int pitch, unsigned char const *argb, double maxMSE = 0);
  
  /*! \brief Decompress a DXT1/3/5 image to the given buffer
   Assumes the buffer has been allocated to at least width*height*4
   \param argb pixel buffer to write to (at least width*height*4 bytes)
   \param width width of the pixel buffer
   \param height height of the pixel buffer
   \param pitch pitch of the pixel buffer
   \param dxt compressed dxt data
   \param format format of the compressed dxt data
   \return true on success, false otherwise
   */
  static bool Decompress(unsigned char *argb, unsigned int width, unsigned int height, unsigned int pitch, unsigned char const *dxt, unsigned int format);

private:
  void Allocate(unsigned int width, unsigned int height, unsigned int format);
  const char *GetFourCC(unsigned int format) const;
  bool WriteFile(const std::string &file) const;

  /*! \brief Compress an ARGB buffer into a DXT1/3/5 image
   \param width width of the pixel buffer
   \param height height of the pixel buffer
   \param pitch pitch of the pixel buffer
   \param argb pixel buffer
   \param maxMSE maximum mean square error to allow, ignored if 0 (the default)
   \return true on successful compression within the given maxMSE, false otherwise
   */
  bool Compress(unsigned int width, unsigned int height, unsigned int pitch, unsigned char const *argb, double maxMSE = 0);

  unsigned int GetStorageRequirements(unsigned int width, unsigned int height, unsigned int format) const;
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
