/*
 *      Copyright (C) 2014 Team Kodi
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

#include "JPGDecoder.h"

#include "SimpleFS.h"

#include <cstdint>
#include <limits>
#include <memory>

#include <jpeglib.h>

JPGDecoder::JPGDecoder()
{
  m_extensions.emplace_back(".jpg");
  m_extensions.emplace_back(".jpeg");
}

bool JPGDecoder::CanDecode(const std::string &filename)
{
  CFile fp;
  bool ret = false;
  unsigned char magic[2];
  if (fp.Open(filename))
  {

    //JPEG image files begin with FF D8 and end with FF D9.
    // check for FF D8 big + little endian on start
    uint64_t readbytes = fp.Read(magic, 2);
    if (readbytes == 2)
    {
      if ((magic[0] == 0xd8 && magic[1] == 0xff) ||
          (magic[1] == 0xd8 && magic[0] == 0xff))
        ret = true;
    }

    if (ret)
    {
      ret = false;
      //check on FF D9 big + little endian on end
      uint64_t fileSize = fp.GetFileSize();
      fp.Seek(fileSize - 2);
      readbytes = fp.Read(magic, 2);
      if (readbytes == 2)
      {
        if ((magic[0] == 0xd9 && magic[1] == 0xff) ||
           (magic[1] == 0xd9 && magic[0] == 0xff))
          ret = true;
      }
    }
  }

  return ret;
}

bool JPGDecoder::LoadFile(const std::string &filename, DecodedFrames &frames)
{
  #define WIDTHBYTES(bits) ((((bits) + 31) / 32) * 4)

  CFile arq;
  if (!arq.Open(filename))
    return false;

  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;

  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&cinfo);

  jpeg_stdio_src(&cinfo, arq.getFP());
  jpeg_read_header(&cinfo, TRUE);
  jpeg_start_decompress(&cinfo);

  // Image Size is calculated as width * height * bytes per pixel = 4
  // Since image_width and image_height can be at most 0xFFFF, this is safe from overflows
  const std::uint64_t ImageSize =
      static_cast<std::uint64_t>(cinfo.image_width) * cinfo.image_height * 4;

  // Check if the conversion to std::size_t is lossless
  if (ImageSize > std::numeric_limits<std::size_t>::max())
    return false;

  DecodedFrame frame;

  frame.rgbaImage.pixels.resize(static_cast<std::size_t>(ImageSize));

  std::vector<unsigned char> scanlinebuff;
  scanlinebuff.resize(3 * cinfo.image_width);

  unsigned char* dst = (unsigned char*)frame.rgbaImage.pixels.data();
  while (cinfo.output_scanline < cinfo.output_height)
  {
    unsigned char* src2 = scanlinebuff.data();

    jpeg_read_scanlines(&cinfo, &src2, 1);

    unsigned char *dst2 = dst;
    for (unsigned int x = 0; x < cinfo.image_width; x++, src2 += 3)
    {
      *dst2++ = src2[2];
      *dst2++ = src2[1];
      *dst2++ = src2[0];
      *dst2++ = 0xff;
    }
    dst += cinfo.image_width * 4;
  }

  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);

  frame.rgbaImage.height = cinfo.image_height;
  frame.rgbaImage.width = cinfo.image_width;
  frame.rgbaImage.bbp = 32;
  frame.rgbaImage.pitch = 4 * cinfo.image_width;

  frames.frameList.push_back(frame);

  return true;
}
