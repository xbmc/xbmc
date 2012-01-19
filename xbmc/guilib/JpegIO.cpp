/*
*      Copyright (C) 2005-2011 Team XBMC
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
*  Parts of this code taken from Guido Vollbeding <http://sylvana.net/jpegcrop/exif_orientation.html>
*
*/

#include "lib/libexif/libexif.h"
#include "windowing/WindowingFactory.h"
#include "settings/GUISettings.h"
#include "settings/Settings.h"
#include "filesystem/File.h"
#include "utils/log.h"
#include "XBTF.h"
#include "JpegIO.h"

#include <setjmp.h>

#define EXIF_TAG_ORIENTATION    0x0112

struct my_error_mgr
{
  struct jpeg_error_mgr pub;    // "public" fields
  jmp_buf setjmp_buffer;        // for return to caller
};

#if JPEG_LIB_VERSION < 80

/*Versions of libjpeg prior to 8.0 did not have a pre-made mechanism for
  decoding directly from memory. Here we backport the functions from v8.
  When using v8 or higher, the built-in functions are used instead.
  Original formatting left intact for the most part for easy comparison. */

static void x_init_mem_source (j_decompress_ptr cinfo)
{
  /* no work necessary here */
}

void x_term_source (j_decompress_ptr cinfo)
{
  /* no work necessary here */
}

static boolean x_fill_mem_input_buffer (j_decompress_ptr cinfo)
{
  static JOCTET mybuffer[4];

  /* The whole JPEG data is expected to reside in the supplied memory
   * buffer, so any request for more data beyond the given buffer size
   * is treated as an error.
   */
  /* Insert a fake EOI marker */
  mybuffer[0] = (JOCTET) 0xFF;
  mybuffer[1] = (JOCTET) JPEG_EOI;

  cinfo->src->next_input_byte = mybuffer;
  cinfo->src->bytes_in_buffer = 2;

  return true;
}

static void x_skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
  struct jpeg_source_mgr * src = cinfo->src;

  /* Just a dumb implementation for now.  Could use fseek() except
   * it doesn't work on pipes.  Not clear that being smart is worth
   * any trouble anyway --- large skips are infrequent.
   */
  if (num_bytes > 0) {
    while (num_bytes > (long) src->bytes_in_buffer) {
      num_bytes -= (long) src->bytes_in_buffer;
      (void) (*src->fill_input_buffer) (cinfo);
      /* note we assume that fill_input_buffer will never return FALSE,
       * so suspension need not be handled.
       */
    }
    src->next_input_byte += (size_t) num_bytes;
    src->bytes_in_buffer -= (size_t) num_bytes;
  }
}

static void x_mem_src (j_decompress_ptr cinfo, unsigned char * inbuffer, unsigned long insize)
{
  struct jpeg_source_mgr * src;

  if (inbuffer == NULL || insize == 0)	/* Treat empty input as fatal error */
  {
    (cinfo)->err->msg_code = 0;
    (*(cinfo)->err->error_exit) ((j_common_ptr) (cinfo));
  }

  /* The source object is made permanent so that a series of JPEG images
   * can be read from the same buffer by calling jpeg_mem_src only before
   * the first one.
   */
  if (cinfo->src == NULL) {	/* first time for this JPEG object? */
    cinfo->src = (struct jpeg_source_mgr *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
				  sizeof(struct jpeg_source_mgr));
  }

  src = cinfo->src;
  src->init_source = x_init_mem_source;
  src->fill_input_buffer = x_fill_mem_input_buffer;
  src->skip_input_data = x_skip_input_data;
  src->resync_to_restart = jpeg_resync_to_restart; /* use default method */
  src->term_source = x_term_source;
  src->bytes_in_buffer = (size_t) insize;
  src->next_input_byte = (JOCTET *) inbuffer;
}
#endif

CJpegIO::CJpegIO()
{
  m_minx = 0;
  m_miny = 0;
  m_imgsize = 0;
  m_width  = 0;
  m_height = 0;
  m_orientation = 0;
  m_inputBuffSize = 0;
  m_inputBuff = NULL;
  m_texturePath = "";
}

CJpegIO::~CJpegIO()
{
  Close();
}

void CJpegIO::Close()
{
  delete [] m_inputBuff;
}

bool CJpegIO::Open(const CStdString &texturePath, unsigned int minx, unsigned int miny)
{
  m_texturePath = texturePath;
  m_minx = minx;
  m_miny = miny;

  XFILE::CFile file;
  if (file.Open(m_texturePath.c_str(), 0))
  {
    m_imgsize = (unsigned int)file.GetLength();
    m_inputBuff = new unsigned char[m_imgsize];
    m_inputBuffSize = file.Read(m_inputBuff, m_imgsize);
    file.Close();

    if ((m_imgsize != m_inputBuffSize) || (m_inputBuffSize == 0))
      return false;
  }
  else
    return false;

  struct my_error_mgr jerr;
  m_cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = jpeg_error_exit;
  jpeg_create_decompress(&m_cinfo);
#if JPEG_LIB_VERSION < 80
  x_mem_src(&m_cinfo, m_inputBuff, m_inputBuffSize);
#else
  jpeg_mem_src(&m_cinfo, m_inputBuff, m_inputBuffSize);
#endif

  if (setjmp(jerr.setjmp_buffer))
  {
    jpeg_destroy_decompress(&m_cinfo);
    return false;
  }
  else
  {
    jpeg_read_header(&m_cinfo, true);

    /*  libjpeg can scale the image for us if it is too big. It must be in the format
    num/denom, where (for our purposes) that is [1-8]/8 where 8/8 is the unscaled image.
    The only way to know how big a resulting image will be is to try a ratio and
    test its resulting size.
    If the res is greater than the one desired, use that one since there's no need
    to decode a bigger one just to squish it back down. If the res is greater than
    the gpu can hold, use the previous one.*/
    if (m_minx == 0 || m_miny == 0)
    {
      m_minx = g_settings.m_ResInfo[g_guiSettings.m_LookAndFeelResolution].iWidth;
      m_miny = g_settings.m_ResInfo[g_guiSettings.m_LookAndFeelResolution].iHeight;
    }
    m_cinfo.scale_denom = 8;
    m_cinfo.out_color_space = JCS_RGB;
    unsigned int maxtexsize = g_Windowing.GetMaxTextureSize();
    for (m_cinfo.scale_num = 1; m_cinfo.scale_num <= 8; m_cinfo.scale_num++)
    {
      jpeg_calc_output_dimensions(&m_cinfo);
      if ((m_cinfo.output_width > maxtexsize) || (m_cinfo.output_height > maxtexsize))
      {
        m_cinfo.scale_num--;
        break;
      }
      if (m_cinfo.output_width >= m_minx || m_cinfo.output_height >= m_miny)
        break;
    }
    jpeg_calc_output_dimensions(&m_cinfo);
    m_width  = m_cinfo.output_width;
    m_height = m_cinfo.output_height;

    GetExif();
    return true;
  }
}

bool CJpegIO::Decode(const unsigned char *pixels, unsigned int pitch, unsigned int format)
{
  unsigned char *dst = (unsigned char*)pixels;

  struct my_error_mgr jerr;
  m_cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = jpeg_error_exit;

  if (setjmp(jerr.setjmp_buffer))
  {
    jpeg_destroy_decompress(&m_cinfo);
    return false;
  }
  else
  {
    jpeg_start_decompress(&m_cinfo);

    if (format == XB_FMT_RGB8)
    {
      while (m_cinfo.output_scanline < m_height)
      {
        jpeg_read_scanlines(&m_cinfo, &dst, 1);
        dst += pitch;
      }
    }
    else if (format == XB_FMT_A8R8G8B8)
    {
      unsigned char* row = new unsigned char[m_width * 3];
      while (m_cinfo.output_scanline < m_height)
      {
        jpeg_read_scanlines(&m_cinfo, &row, 1);
        unsigned char *src2 = row;
        unsigned char *dst2 = dst;
        for (unsigned int x = 0; x < m_width; x++, src2 += 3)
        {
          *dst2++ = src2[2];
          *dst2++ = src2[1];
          *dst2++ = src2[0];
          *dst2++ = 0xff;
        }
        dst += pitch;
      }
      delete[] row;
    }
    else
    {
      CLog::Log(LOGWARNING, "JpegIO: Incorrect output format specified");
      jpeg_destroy_decompress(&m_cinfo);
      return false;
    }
    jpeg_finish_decompress(&m_cinfo);
  }
  jpeg_destroy_decompress(&m_cinfo);
  return true;
}

// override libjpeg's error function to avoid an exit() call
void CJpegIO::jpeg_error_exit(j_common_ptr cinfo)
{
  CStdString msg;
  msg.Format("Error %i: %s",cinfo->err->msg_code, cinfo->err->jpeg_message_table[cinfo->err->msg_code]);
  CLog::Log(LOGWARNING, "JpegIO: %s", msg.c_str());

  my_error_mgr *myerr = (my_error_mgr*)cinfo->err;
  longjmp(myerr->setjmp_buffer, 1);
}

/*helper function
 * finds the exif tag in the jpegData buffer
 * and returns the size of the exif section
 * exifPtr is set to the start of the
 * "Exif" <- tag
 * if 0 is returned - no exif tag was found
 */
unsigned int CJpegIO::findExifMarker( unsigned char *jpegData, 
                                      unsigned int dataSize, 
                                      unsigned char *&exifPtr)
{
  unsigned char *buffPtr = jpegData+2;//SKIP 0xFFD8
  unsigned char *endOfFile = jpegData + dataSize;

  if(!jpegData || dataSize < 2 || jpegData[0] != 0xFF || jpegData[1] != 0xD8)
    return 0;

  for(;;)
  {
    BYTE marker = 0;
    for (int a=0; a<7 && (buffPtr < endOfFile); a++)
    {
      marker = *buffPtr;
      if (marker != 0xFF)
        break;

      if (a >= 6)
        return 0;
      marker = 0;
      buffPtr++;
    }

    // 0xff is legal padding, but if we get that many, something's wrong.
    if (marker == 0xff)
      return 0;

    buffPtr++;//move to start of itemlen field
    if ((buffPtr + 1) >= endOfFile)
      return 0;

    // Read the length of the section.
    unsigned short itemlen = (*buffPtr++) << 8;
    itemlen += *buffPtr;

    if (itemlen < sizeof(itemlen))
      return 0;

    switch(marker)
    {
      case M_EOI:
      case M_SOS:   // stop before hitting compressed data
        return 0;
      case M_EXIF:
        // found exifdata
        //   buffPtr was pointing at the second length byte
        //   +1 for getting the exif tag
        exifPtr = buffPtr + 1;
        return itemlen;
      default://skip all other sections
        buffPtr += itemlen;
        break;
    }
  }

  return 0;
}

bool CJpegIO::GetExif()
{
  unsigned int length = 0;
  unsigned int offset = 0;
  unsigned int numberOfTags = 0;
  unsigned int tagNumber = 0;
  bool isMotorola = false;
  unsigned char *exif_data = NULL;
  unsigned const char ExifHeader[] = "Exif\0\0";

  length = findExifMarker(m_inputBuff, m_imgsize, exif_data);

  // read exif head, check for "Exif"
  //   next we want to read to current offset + length
  //   check if buffer is big enough
  if (length && memcmp(exif_data, ExifHeader, 6) == 0)
  {
    //read exif body
    exif_data += 6;
  }
  else
  {
    return false;
  }

  //check for broken files
  if ((m_inputBuff + m_imgsize) < (exif_data + length))
  {
    return false;
  }

  // Discover byte order
  if (exif_data[0] == 'I' && exif_data[1] == 'I')
    isMotorola = false;
  else if (exif_data[0] == 'M' && exif_data[1] == 'M')
    isMotorola = true;
  else
    return false;

  // Check Tag Mark
  if (isMotorola)
  {
    if (exif_data[2] != 0 || exif_data[3] != 0x2A)
      return false;
  }
  else
  {
    if (exif_data[3] != 0 || exif_data[2] != 0x2A)
      return false;
  }

  // Get first IFD offset (offset to IFD0)
  if (isMotorola)
  {
    if (exif_data[4] != 0 || exif_data[5] != 0)
      return false;
    offset = exif_data[6];
    offset <<= 8;
    offset += exif_data[7];
  }
  else
  {
    if (exif_data[7] != 0 || exif_data[6] != 0)
      return false;
    offset = exif_data[5];
    offset <<= 8;
    offset += exif_data[4];
  }

  if (offset > length - 2)
    return false; // check end of data segment

  // Get the number of directory entries contained in this IFD
  if (isMotorola)
  {
    numberOfTags = exif_data[offset];
    numberOfTags <<= 8;
    numberOfTags += exif_data[offset+1];
  }
  else
  {
    numberOfTags = exif_data[offset+1];
    numberOfTags <<= 8;
    numberOfTags += exif_data[offset];
  }

  if (numberOfTags == 0)
    return false;
  offset += 2;

  // Search for Orientation Tag in IFD0 - hey almost there! :D
  while(1)//hopefully this jpeg has correct exif data...
  {
    if (offset > length - 12)
      return false; // check end of data segment

    // Get Tag number
    if (isMotorola)
    {
      tagNumber = exif_data[offset];
      tagNumber <<= 8;
      tagNumber += exif_data[offset+1];
    }
    else
    {
      tagNumber = exif_data[offset+1];
      tagNumber <<= 8;
      tagNumber += exif_data[offset];
    }

    if (tagNumber == EXIF_TAG_ORIENTATION)
      break; //found orientation tag

    if ( --numberOfTags == 0)
      return false;//no orientation found
    offset += 12;//jump to next tag
  }

  // Get the Orientation value
  if (isMotorola)
  {
    if (exif_data[offset+8] != 0)
      return false;
    m_orientation = exif_data[offset+9];
  }
  else
  {
    if (exif_data[offset+9] != 0)
      return false;
    m_orientation = exif_data[offset+8];
  }
  if (m_orientation > 8)
  {
    m_orientation = 0;
    return false;
  }

  return true;//done
}
