/*
 *  Copyright (C) 2013 Arne Morten Kvarving
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ImageDecoder.h"

#include "guilib/TextureFormats.h"

static const std::map<int, ImageFormat> KodiToAddonFormat = {
    {XB_FMT_A8R8G8B8, ADDON_IMG_FMT_A8R8G8B8},
    {XB_FMT_A8, ADDON_IMG_FMT_A8},
    {XB_FMT_RGBA8, ADDON_IMG_FMT_RGBA8},
    {XB_FMT_RGB8, ADDON_IMG_FMT_RGB8}};

namespace ADDON
{

CImageDecoder::CImageDecoder(const AddonInfoPtr& addonInfo)
  : IAddonInstanceHandler(ADDON_INSTANCE_IMAGEDECODER, addonInfo)
{
  // Create all interface parts independent to make API changes easier if
  // something is added
  m_struct.props = new AddonProps_ImageDecoder();
  m_struct.toKodi = new AddonToKodiFuncTable_ImageDecoder();
  m_struct.toAddon = new KodiToAddonFuncTable_ImageDecoder();
}

CImageDecoder::~CImageDecoder()
{
  DestroyInstance();

  delete m_struct.props;
  delete m_struct.toKodi;
  delete m_struct.toAddon;
}

bool CImageDecoder::LoadImageFromMemory(unsigned char* buffer, unsigned int bufSize,
                                        unsigned int width, unsigned int height)
{
  if (!m_struct.toAddon->load_image_from_memory)
    return false;

  m_width = width;
  m_height = height;
  return m_struct.toAddon->load_image_from_memory(&m_struct, buffer, bufSize, &m_width, &m_height);
}

bool CImageDecoder::Decode(unsigned char* const pixels, unsigned int width,
                           unsigned int height, unsigned int pitch,
                           unsigned int format)
{
  if (!m_struct.toAddon->decode)
    return false;

  auto it = KodiToAddonFormat.find(format & XB_FMT_MASK);
  if (it == KodiToAddonFormat.end())
    return false;

  bool result = m_struct.toAddon->decode(&m_struct, pixels, width, height, pitch, it->second);
  m_width = width;
  m_height = height;

  return result;
}

bool CImageDecoder::Create(const std::string& mimetype)
{
  m_struct.props->mimetype = mimetype.c_str();
  m_struct.toKodi->kodi_instance = this;
  return (CreateInstance(&m_struct) == ADDON_STATUS_OK);
}

} /*namespace ADDON*/
