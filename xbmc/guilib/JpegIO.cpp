/*
*      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  Parts of this code taken from Guido Vollbeding <http://sylvana.net/jpegcrop/exif_orientation.html>
 *
*/

#include "windowing/WindowingFactory.h"
#include "settings/AdvancedSettings.h"
#include "filesystem/File.h"
#include "utils/log.h"
#include "XBTF.h"
#include "JpegIO.h"
#include "utils/StringUtils.h"
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

#define OUTPUT_BUF_SIZE  4096
typedef struct {
  struct jpeg_destination_mgr pub; /* public fields */

  unsigned char ** outbuffer;   /* target buffer */
  unsigned long * outsize;
  unsigned char * newbuffer;    /* newly allocated buffer */
  JOCTET * buffer;              /* start of buffer */
  size_t bufsize;
} x_mem_destination_mgr;

typedef x_mem_destination_mgr * x_mem_dest_ptr;

static void x_init_mem_destination (j_compress_ptr cinfo)
{
  /* no work necessary here */
}

static boolean x_empty_mem_output_buffer (j_compress_ptr cinfo)
{
  size_t nextsize;
  JOCTET * nextbuffer;
  x_mem_dest_ptr dest = (x_mem_dest_ptr) cinfo->dest;

  /* Try to allocate new buffer with double size */
  nextsize = dest->bufsize * 2;
  nextbuffer = (JOCTET*) malloc(nextsize);

  if (nextbuffer == NULL)
  {
    (cinfo)->err->msg_code = 0;
    (cinfo)->err->msg_parm.i[0] = 10;
    (*(cinfo)->err->error_exit) ((j_common_ptr) (cinfo));
  }

  memcpy(nextbuffer, dest->buffer, dest->bufsize);

  if (dest->newbuffer != NULL)
    free(dest->newbuffer);

  dest->newbuffer = nextbuffer;

  dest->pub.next_output_byte = nextbuffer + dest->bufsize;
  dest->pub.free_in_buffer = dest->bufsize;

  dest->buffer = nextbuffer;
  dest->bufsize = nextsize;

  return TRUE;
}

static void x_term_mem_destination (j_compress_ptr cinfo)
{
  x_mem_dest_ptr dest = (x_mem_dest_ptr) cinfo->dest;

  *dest->outbuffer = dest->buffer;
  *dest->outsize = dest->bufsize - dest->pub.free_in_buffer;
}

static void x_jpeg_mem_dest (j_compress_ptr cinfo,
               unsigned char ** outbuffer, unsigned long * outsize)
{
  x_mem_dest_ptr dest;

  if (outbuffer == NULL || outsize == NULL)     /* sanity check */
  {
    (cinfo)->err->msg_code = 0;
    (*(cinfo)->err->error_exit) ((j_common_ptr) (cinfo));
  }

  /* The destination object is made permanent so that multiple JPEG images
   * can be written to the same buffer without re-executing jpeg_mem_dest.
   */
  if (cinfo->dest == NULL) {    /* first time for this JPEG object? */
    cinfo->dest = (struct jpeg_destination_mgr *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
                                  sizeof(x_mem_destination_mgr));
  }

  dest = (x_mem_dest_ptr) cinfo->dest;
  dest->pub.init_destination = x_init_mem_destination;
  dest->pub.empty_output_buffer = x_empty_mem_output_buffer;
  dest->pub.term_destination = x_term_mem_destination;
  dest->outbuffer = outbuffer;
  dest->outsize = outsize;
  dest->newbuffer = NULL;

  if (*outbuffer == NULL || *outsize == 0) {
    /* Allocate initial buffer */
    dest->newbuffer = *outbuffer = (unsigned char*)malloc(OUTPUT_BUF_SIZE);
    if (dest->newbuffer == NULL)
    {
      (cinfo)->err->msg_code = 0;
      (cinfo)->err->msg_parm.i[0] = 10;
      (*(cinfo)->err->error_exit) ((j_common_ptr) (cinfo));
    }
    *outsize = OUTPUT_BUF_SIZE;
  }

  dest->pub.next_output_byte = dest->buffer = *outbuffer;
  dest->pub.free_in_buffer = dest->bufsize = *outsize;
}
#endif

CJpegIO::CJpegIO()
{
  m_width  = 0;
  m_height = 0;
  m_orientation = 0;
  m_inputBuffSize = 0;
  m_inputBuff = NULL;
  memset(&m_cinfo, 0, sizeof(m_cinfo));
  m_thumbnailbuffer = NULL;
}

CJpegIO::~CJpegIO()
{
  Close();
}

void CJpegIO::Close()
{
  free(m_inputBuff);
  m_inputBuff = NULL;
  m_inputBuffSize = 0;
  ReleaseThumbnailBuffer();
}

bool CJpegIO::Open(const std::string &texturePath, unsigned int minx, unsigned int miny, bool read)
{
  Close();

  m_texturePath = texturePath;

  XFILE::CFile file;
  XFILE::auto_buffer buf;
  if (file.LoadFile(texturePath, buf) <= 0)
    return false;

  m_inputBuffSize = buf.size();
  m_inputBuff = (unsigned char*)buf.detach();

  return Read(m_inputBuff, m_inputBuffSize, minx, miny);
}

bool CJpegIO::Read(unsigned char* buffer, unsigned int bufSize, unsigned int minx, unsigned int miny)
{
  struct my_error_mgr jerr;
  m_cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = jpeg_error_exit;

  if (buffer == NULL || !bufSize )
    return false;

  jpeg_create_decompress(&m_cinfo);
#if JPEG_LIB_VERSION < 80
  x_mem_src(&m_cinfo, buffer, bufSize);
#else
  jpeg_mem_src(&m_cinfo, buffer, bufSize);
#endif

  if (setjmp(jerr.setjmp_buffer))
  {
    jpeg_destroy_decompress(&m_cinfo);
    return false;
  }
  else
  {
    jpeg_save_markers (&m_cinfo, JPEG_APP0 + 1, 0xFFFF);
    jpeg_read_header(&m_cinfo, true);

    /*  libjpeg can scale the image for us if it is too big. It must be in the format
    num/denom, where (for our purposes) that is [1-8]/8 where 8/8 is the unscaled image.
    The only way to know how big a resulting image will be is to try a ratio and
    test its resulting size.
    If the res is greater than the one desired, use that one since there's no need
    to decode a bigger one just to squish it back down. If the res is greater than
    the gpu can hold, use the previous one.*/
    if (minx == 0 || miny == 0)
    {
      miny = g_advancedSettings.m_imageRes;
      if (g_advancedSettings.m_fanartRes > g_advancedSettings.m_imageRes)
      { // a separate fanart resolution is specified - check if the image is exactly equal to this res
        if (m_cinfo.image_width == (unsigned int)g_advancedSettings.m_fanartRes * 16/9 &&
            m_cinfo.image_height == (unsigned int)g_advancedSettings.m_fanartRes)
        { // special case for fanart res
          miny = g_advancedSettings.m_fanartRes;
        }
      }
      minx = miny * 16/9;
    }

    m_cinfo.scale_denom = 8;
    m_cinfo.out_color_space = JCS_RGB;
    unsigned int maxtexsize = g_Windowing.GetMaxTextureSize();
    for (unsigned int scale = 1; scale <= 8; scale++)
    {
      m_cinfo.scale_num = scale;
      jpeg_calc_output_dimensions(&m_cinfo);
      if ((m_cinfo.output_width > maxtexsize) || (m_cinfo.output_height > maxtexsize))
      {
        m_cinfo.scale_num--;
        break;
      }
      if (m_cinfo.output_width >= minx && m_cinfo.output_height >= miny)
        break;
    }
    jpeg_calc_output_dimensions(&m_cinfo);
    m_width  = m_cinfo.output_width;
    m_height = m_cinfo.output_height;

    if (m_cinfo.marker_list)
      m_orientation = GetExifOrientation(m_cinfo.marker_list->data, m_cinfo.marker_list->data_length);
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

bool CJpegIO::CreateThumbnail(const std::string& sourceFile, const std::string& destFile, int minx, int miny, bool rotateExif)
{
  //Copy sourceFile to buffer, pass to CreateThumbnailFromMemory for decode+re-encode
  if (!Open(sourceFile, minx, miny, false))
    return false;

  return CreateThumbnailFromMemory(m_inputBuff, m_inputBuffSize, destFile, minx, miny);
}

bool CJpegIO::CreateThumbnailFromMemory(unsigned char* buffer, unsigned int bufSize, const std::string& destFile, unsigned int minx, unsigned int miny)
{
  //Decode a jpeg residing in buffer, pass to CreateThumbnailFromSurface for re-encode
  unsigned int pitch = 0;
  unsigned char *sourceBuf = NULL;

  if (!Read(buffer, bufSize, minx, miny))
    return false;
  pitch = Width() * 3;
  sourceBuf = new unsigned char [Height() * pitch];

  if (!Decode(sourceBuf,pitch,XB_FMT_RGB8))
  {
    delete [] sourceBuf;
    return false;
  }
  if (!CreateThumbnailFromSurface(sourceBuf, Width(), Height() , XB_FMT_RGB8, pitch, destFile))
  {
    delete [] sourceBuf;
    return false;
  }
  delete [] sourceBuf;
  return true;
}

bool CJpegIO::CreateThumbnailFromSurface(unsigned char* buffer, unsigned int width, unsigned int height, unsigned int format, unsigned int pitch, const std::string& destFile)
{
  //Encode raw data from buffer, save to destFile
  struct jpeg_compress_struct cinfo;
  struct my_error_mgr jerr;
  JSAMPROW row_pointer[1];
  long unsigned int outBufSize = width * height;
  unsigned char* result;
  unsigned char* src = buffer;
  unsigned char* rgbbuf;

  if(buffer == NULL)
  {
    CLog::Log(LOGERROR, "JpegIO::CreateThumbnailFromSurface no buffer");
    return false;
  }

  result = (unsigned char*) malloc(outBufSize); //Initial buffer. Grows as-needed.
  if (result == NULL)
  {
    CLog::Log(LOGERROR, "JpegIO::CreateThumbnailFromSurface error allocating memory for image buffer");
    return false;
  }

  if(format == XB_FMT_RGB8)
  {
    rgbbuf = buffer;
  }
  else if(format == XB_FMT_A8R8G8B8)
  {
    // create a copy for bgra -> rgb.
    rgbbuf = new unsigned char [(width * height * 3)];
    unsigned char* dst = rgbbuf;
    for (unsigned int y = 0; y < height; y++)
    {
      unsigned char* dst2 = dst;
      unsigned char* src2 = src;
      for (unsigned int x = 0; x < width; x++, src2 += 4)
      {
        *dst2++ = src2[2];
        *dst2++ = src2[1];
        *dst2++ = src2[0];
      }
      dst += width * 3;
      src += pitch;
    }
  }
  else
  {
    CLog::Log(LOGWARNING, "JpegIO::CreateThumbnailFromSurface Unsupported format");
    free(result);
    return false;
  }

  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = jpeg_error_exit;
  jpeg_create_compress(&cinfo);

  if (setjmp(jerr.setjmp_buffer))
  {
    jpeg_destroy_compress(&cinfo);
    free(result);
    if(format != XB_FMT_RGB8)
      delete [] rgbbuf;
    return false;
  }
  else
  {
#if JPEG_LIB_VERSION < 80
    x_jpeg_mem_dest(&cinfo, &result, &outBufSize);
#else
    jpeg_mem_dest(&cinfo, &result, &outBufSize);
#endif
    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, 90, TRUE);
    jpeg_start_compress(&cinfo, TRUE);

    while (cinfo.next_scanline < cinfo.image_height)
    {
      row_pointer[0] = &rgbbuf[cinfo.next_scanline * width * 3];
      jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
  }
  if(format != XB_FMT_RGB8)
    delete [] rgbbuf;

  XFILE::CFile file;
  const bool ret = file.OpenForWrite(destFile, true) && file.Write(result, outBufSize) == static_cast<ssize_t>(outBufSize);
  free(result);

  return ret;
}

// override libjpeg's error function to avoid an exit() call
void CJpegIO::jpeg_error_exit(j_common_ptr cinfo)
{
  std::string msg = StringUtils::Format("Error %i: %s",cinfo->err->msg_code, cinfo->err->jpeg_message_table[cinfo->err->msg_code]);
  CLog::Log(LOGWARNING, "JpegIO: %s", msg.c_str());

  my_error_mgr *myerr = (my_error_mgr*)cinfo->err;
  longjmp(myerr->setjmp_buffer, 1);
}

unsigned int CJpegIO::GetExifOrientation(unsigned char* exif_data, unsigned int exif_data_size)
{
  unsigned int offset = 0;
  unsigned int numberOfTags = 0;
  unsigned int tagNumber = 0;
  bool isMotorola = false;
  unsigned const char ExifHeader[] = "Exif\0\0";
  unsigned int orientation = 0;

  // read exif head, check for "Exif"
  //   next we want to read to current offset + length
  //   check if buffer is big enough
  if (exif_data_size && memcmp(exif_data, ExifHeader, 6) == 0)
  {
    //read exif body
    exif_data += 6;
  }
  else
  {
    return 0;
  }

  // Discover byte order
  if (exif_data[0] == 'I' && exif_data[1] == 'I')
    isMotorola = false;
  else if (exif_data[0] == 'M' && exif_data[1] == 'M')
    isMotorola = true;
  else
    return 0;

  // Check Tag Mark
  if (isMotorola)
  {
    if (exif_data[2] != 0 || exif_data[3] != 0x2A)
      return 0;
  }
  else
  {
    if (exif_data[3] != 0 || exif_data[2] != 0x2A)
      return 0;
  }

  // Get first IFD offset (offset to IFD0)
  if (isMotorola)
  {
    if (exif_data[4] != 0 || exif_data[5] != 0)
      return 0;
    offset = exif_data[6];
    offset <<= 8;
    offset += exif_data[7];
  }
  else
  {
    if (exif_data[7] != 0 || exif_data[6] != 0)
      return 0;
    offset = exif_data[5];
    offset <<= 8;
    offset += exif_data[4];
  }

  if (offset > exif_data_size - 2)
    return 0; // check end of data segment

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
    return 0;
  offset += 2;

  // Search for Orientation Tag in IFD0 - hey almost there! :D
  while(1)//hopefully this jpeg has correct exif data...
  {
    if (offset > exif_data_size - 12)
      return 0; // check end of data segment

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
      return 0;//no orientation found
    offset += 12;//jump to next tag
  }

  // Get the Orientation value
  if (isMotorola)
  {
    if (exif_data[offset+8] != 0)
      return 0;
    orientation = exif_data[offset+9];
  }
  else
  {
    if (exif_data[offset+9] != 0)
      return 0;
    orientation = exif_data[offset+8];
  }
  if (orientation > 8)
    orientation = 0;

  return orientation;//done
}

bool CJpegIO::LoadImageFromMemory(unsigned char* buffer, unsigned int bufSize, unsigned int width, unsigned int height)
{
  return Read(buffer, bufSize, width, height);
}

bool CJpegIO::CreateThumbnailFromSurface(unsigned char* bufferin, unsigned int width, unsigned int height, unsigned int format, unsigned int pitch, const std::string& destFile, 
                                         unsigned char* &bufferout, unsigned int &bufferoutSize)
{
  //Encode raw data from buffer, save to destbuffer
  struct jpeg_compress_struct cinfo;
  struct my_error_mgr jerr;
  JSAMPROW row_pointer[1];
  long unsigned int outBufSize = width * height;
  unsigned char* src = bufferin;
  unsigned char* rgbbuf;

  if(bufferin == NULL)
  {
    CLog::Log(LOGERROR, "JpegIO::CreateThumbnailFromSurface no buffer");
    return false;
  }

  m_thumbnailbuffer = (unsigned char*) malloc(outBufSize); //Initial buffer. Grows as-needed.
  if (m_thumbnailbuffer == NULL)
  {
    CLog::Log(LOGERROR, "JpegIO::CreateThumbnailFromSurface error allocating memory for image buffer");
    return false;
  }

  if(format == XB_FMT_RGB8)
  {
    rgbbuf = bufferin;
  }
  else if(format == XB_FMT_A8R8G8B8)
  {
    // create a copy for bgra -> rgb.
    rgbbuf = new unsigned char [(width * height * 3)];
    unsigned char* dst = rgbbuf;
    for (unsigned int y = 0; y < height; y++)
    {

      unsigned char* dst2 = dst;
      unsigned char* src2 = src;
      for (unsigned int x = 0; x < width; x++, src2 += 4)
      {
        *dst2++ = src2[2];
        *dst2++ = src2[1];
        *dst2++ = src2[0];
      }
      dst += width * 3;
      src += pitch;
    }
  }
  else
  {
    CLog::Log(LOGWARNING, "JpegIO::CreateThumbnailFromSurface Unsupported format");
    free(m_thumbnailbuffer);
    return false;
  }

  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = jpeg_error_exit;
  jpeg_create_compress(&cinfo);

  if (setjmp(jerr.setjmp_buffer))
  {
    jpeg_destroy_compress(&cinfo);
    free(m_thumbnailbuffer);
    if(format != XB_FMT_RGB8)
      delete [] rgbbuf;
    return false;
  }
  else
  {
#if JPEG_LIB_VERSION < 80
    x_jpeg_mem_dest(&cinfo, &m_thumbnailbuffer, &outBufSize);
#else
    jpeg_mem_dest(&cinfo, &m_thumbnailbuffer, &outBufSize);
#endif
    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, 90, TRUE);
    jpeg_start_compress(&cinfo, TRUE);

    while (cinfo.next_scanline < cinfo.image_height)
    {
      row_pointer[0] = &rgbbuf[cinfo.next_scanline * width * 3];
      jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
  }
  if(format != XB_FMT_RGB8)
    delete [] rgbbuf;

  bufferout = m_thumbnailbuffer;
  bufferoutSize = outBufSize;

  return true;
}

void CJpegIO::ReleaseThumbnailBuffer()
{
  if(m_thumbnailbuffer != NULL)
  {
    free(m_thumbnailbuffer);
    m_thumbnailbuffer = NULL;
  }
}
