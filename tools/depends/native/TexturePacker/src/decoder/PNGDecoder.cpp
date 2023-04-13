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

#include "PNGDecoder.h"

#include "SimpleFS.h"

#include <png.h>

#ifndef png_jmpbuf
#  define png_jmpbuf(png_ptr) ((png_ptr)->jmpbuf)
#endif

PNGDecoder::PNGDecoder()
{
  m_extensions.emplace_back(".png");
}

/* Check to see if a file is a PNG file using png_sig_cmp().  png_sig_cmp()
 * returns zero if the image is a PNG and nonzero if it isn't a PNG.
 *
 * The function check_if_png() shown here, but not used, returns nonzero (true)
 * if the file can be opened and is a PNG, 0 (false) otherwise.
 *
 * If this call is successful, and you are going to keep the file open,
 * you should call png_set_sig_bytes(png_ptr, PNG_BYTES_TO_CHECK); once
 * you have created the png_ptr, so that libpng knows your application
 * has read that many bytes from the start of the file.  Make sure you
 * don't call png_set_sig_bytes() with more than 8 bytes read or give it
 * an incorrect number of bytes read, or you will either have read too
 * many bytes (your fault), or you are telling libpng to read the wrong
 * number of magic bytes (also your fault).
 *
 * Many applications already read the first 2 or 4 bytes from the start
 * of the image to determine the file type, so it would be easiest just
 * to pass the bytes to png_sig_cmp() or even skip that if you know
 * you have a PNG file, and call png_set_sig_bytes().
 */
bool PNGDecoder::CanDecode(const std::string &filename)
{
  #define PNG_BYTES_TO_CHECK 4
  CFile fp;
  char buf[PNG_BYTES_TO_CHECK];

  /* Open the prospective PNG file. */
  if (!fp.Open(filename))
    return false;

  /* Read in some of the signature bytes */
  if (fp.Read(buf, PNG_BYTES_TO_CHECK) != PNG_BYTES_TO_CHECK)
  {
    fprintf(stderr, "error reading header ...\n");
    return false;
  }
  fp.Close();

  /* Compare the first PNG_BYTES_TO_CHECK bytes of the signature.
   Return nonzero (true) if they match */
  return(!png_sig_cmp((png_bytep)buf, (png_size_t)0, PNG_BYTES_TO_CHECK));
}

bool PNGDecoder::LoadFile(const std::string &filename, DecodedFrames &frames)
{
  png_byte header[8];

  CFile fp;
  if (!fp.Open(filename))
  {
    perror(filename.c_str());
    return false;
  }

  // read the header
  fp.Read(header, 8);

  if (png_sig_cmp(header, 0, 8))
  {
    fprintf(stderr, "error: %s is not a PNG.\n", filename.c_str());
    return false;
  }

  png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr)
  {
    fprintf(stderr, "error: png_create_read_struct returned 0.\n");
    return false;
  }

  // create png info struct
  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
  {
    fprintf(stderr, "error: png_create_info_struct returned 0.\n");
    png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
    return false;
  }

  // create png info struct
  png_infop end_info = png_create_info_struct(png_ptr);
  if (!end_info)
  {
    fprintf(stderr, "error: png_create_info_struct returned 0.\n");
    png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
    return false;
  }

  // the code in this if statement gets called if libpng encounters an error
  if (setjmp(png_jmpbuf(png_ptr))) {
    fprintf(stderr, "error from libpng\n");
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    return false;
  }

  // init png reading
  png_init_io(png_ptr, fp.getFP());

  // let libpng know you already read the first 8 bytes
  png_set_sig_bytes(png_ptr, 8);

  // read all the info up to the image data
  png_read_info(png_ptr, info_ptr);

  // variables to pass to get info
  int bit_depth, color_type;
  png_uint_32 temp_width, temp_height;

  // get info about png
  png_get_IHDR(png_ptr, info_ptr, &temp_width, &temp_height, &bit_depth, &color_type,
               NULL, NULL, NULL);

  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
  {
    png_set_tRNS_to_alpha(png_ptr);
  }

  //set it to 32bit pixeldepth
  png_color_8 sig_bit;
  sig_bit.red = 32;
  sig_bit.green = 32;
  sig_bit.blue = 32;
  // if the image has an alpha channel then
  sig_bit.alpha = 32;
  png_set_sBIT(png_ptr, info_ptr, &sig_bit);


  /* Add filler (or alpha) byte (before/after each RGB triplet) */
  png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);


  if (color_type == PNG_COLOR_TYPE_RGB ||
      color_type == PNG_COLOR_TYPE_RGB_ALPHA)
    png_set_bgr(png_ptr);

  // convert indexed color to rgb
  if (color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_palette_to_rgb(png_ptr);

  /* swap the RGBA or GA data to ARGB or AG (or BGRA to ABGR) */
  //png_set_swap_alpha(png_ptr);

  //libsquish only eats 32bit RGBA, must convert grayscale into this format
  if (color_type == PNG_COLOR_TYPE_GRAY ||
      color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
  {
    png_set_expand_gray_1_2_4_to_8(png_ptr);
    png_set_gray_to_rgb(png_ptr);
  }

  // Update the png info struct.
  png_read_update_info(png_ptr, info_ptr);

  // Row size in bytes.
  int rowbytes = png_get_rowbytes(png_ptr, info_ptr);

  // glTexImage2d requires rows to be 4-byte aligned
  //  rowbytes += 3 - ((rowbytes-1) % 4);

  DecodedFrame frame;

  // Allocate the image_data as a big block, to be given to opengl
  frame.rgbaImage.pixels.resize(rowbytes * temp_height * sizeof(png_byte) + 15);

  // row_pointers is for pointing to image_data for reading the png with libpng
  std::vector<png_bytep> row_pointers;
  row_pointers.resize(temp_height);

  // set the individual row_pointers to point at the correct offsets of image_data
  for (unsigned int i = 0; i < temp_height; i++)
  {
    row_pointers[i] = frame.rgbaImage.pixels.data() + i * rowbytes;
  }

  // read the png into image_data through row_pointers
  png_read_image(png_ptr, row_pointers.data());

  frame.rgbaImage.height = temp_height;
  frame.rgbaImage.width = temp_width;
  frame.rgbaImage.bbp = 32;
  frame.rgbaImage.pitch = 4 * temp_width;

  frames.frameList.push_back(frame);
  // clean up
  png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);

  return true;
}
