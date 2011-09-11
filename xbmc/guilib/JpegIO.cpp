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
*/

#include "pictures/DllImageLib.h"
#include "pictures/DllLibExif.h"
#include "windowing/WindowingFactory.h"
#include "settings/GUISettings.h"
#include "settings/Settings.h"
#include "filesystem/File.h"
#include "utils/log.h"
#include "JpegIO.h"

static void jpeg_error_exit (j_common_ptr cinfo)
{
  CStdString msg;
  msg.Format("Error %i: %s",cinfo->err->msg_code, cinfo->err->jpeg_message_table[cinfo->err->msg_code]);
  throw msg;
}

CJpegIO::CJpegIO()
{
  m_minx = 0;
  m_miny = 0;
  m_imgsize = 0;
  m_width = 0;
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

bool CJpegIO::Open(const CStdString& texturePath,  unsigned int minx, unsigned int miny)
{
  m_texturePath = texturePath;
  m_minx = minx;
  m_miny = miny;

  XFILE::CFile file;
  if (file.Open(m_texturePath.c_str(), 0))
  {
    m_imgsize = file.GetLength();
    m_inputBuff = new unsigned char[m_imgsize];
    m_inputBuffSize = file.Read(m_inputBuff, m_imgsize);
    file.Close();

    if ((m_imgsize != m_inputBuffSize) || (m_inputBuffSize == 0))
      return false;
  }
  else
    return false;

  m_cinfo.err = jpeg_std_error(&m_jerr);
  m_jerr.error_exit = jpeg_error_exit;
  jpeg_create_decompress(&m_cinfo);
  jpeg_mem_src(&m_cinfo, m_inputBuff, m_inputBuffSize);

  try
  {
    jpeg_read_header(&m_cinfo, TRUE);

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
    m_cinfo.scale_denom=8;
    unsigned int maxtexsize = g_Windowing.GetMaxTextureSize();
    for (m_cinfo.scale_num = 1;m_cinfo.scale_num <=8 ;m_cinfo.scale_num++)
    {
      jpeg_calc_output_dimensions (&m_cinfo);
      if ((m_cinfo.output_width * m_cinfo.output_height) > (maxtexsize * maxtexsize))
      {
        m_cinfo.scale_num--;
        break;
      }
      if ( m_cinfo.output_width >= m_minx && m_cinfo.output_height >= m_miny)
        break;
    }
    jpeg_calc_output_dimensions (&m_cinfo);
    m_width = m_cinfo.output_width;
    m_height = m_cinfo.output_height;
    m_pitch = (((m_cinfo.output_width + 1)* 3 / 4) * 4); //align to 4-bytes

    GetExif();
    return true;
  }
  catch (CStdString &msg)
  {
    CLog::Log(LOGWARNING, "JpegIO: %s", msg.c_str());
    jpeg_destroy_decompress(&m_cinfo);
    return false;
  }
}

bool CJpegIO::GetExif()
{
  ExifInfo_t exifInfo;
  IPTCInfo_t iptcInfo;
  DllLibExif exifDll;
  if (!exifDll.Load())
  {
    return false;
  }
  exifDll.process_jpeg(m_texturePath.c_str(), &exifInfo, &iptcInfo);
  if (exifInfo.Orientation)
  {
    m_orientation = exifInfo.Orientation;
    return true;
  }
  return false;
}

bool CJpegIO::Decode(const unsigned char *pixels)
{
  //requires a pre-allocated buffer of size pitch*3
  unsigned char *dst = (unsigned char *) pixels;
  try
  {
    jpeg_start_decompress( &m_cinfo );
    while( m_cinfo.output_scanline < m_height )
    {
      jpeg_read_scanlines( &m_cinfo, &dst, 1 );
      dst+=m_pitch;
    }
    jpeg_finish_decompress( &m_cinfo );
  }
  catch (CStdString &msg)
  {
    CLog::Log(LOGWARNING, "JpegIO: %s", msg.c_str());
    jpeg_destroy_decompress(&m_cinfo);
    return false;
  }
  jpeg_destroy_decompress( &m_cinfo );
  return true;
}
