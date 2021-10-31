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
  m_ifc.imagedecoder = new AddonInstance_ImageDecoder;
  m_ifc.imagedecoder->props = new AddonProps_ImageDecoder();
  m_ifc.imagedecoder->toKodi = new AddonToKodiFuncTable_ImageDecoder();
  m_ifc.imagedecoder->toAddon = new KodiToAddonFuncTable_ImageDecoder();
}

CImageDecoder::~CImageDecoder()
{
  DestroyInstance();

  delete m_ifc.imagedecoder->props;
  delete m_ifc.imagedecoder->toKodi;
  delete m_ifc.imagedecoder->toAddon;
  delete m_ifc.imagedecoder;
}

bool CImageDecoder::LoadImageFromMemory(unsigned char* buffer,
                                        unsigned int bufSize,
                                        unsigned int width,
                                        unsigned int height)
{
  if (!m_ifc.imagedecoder->toAddon->load_image_from_memory)
    return false;

  m_width = width;
  m_height = height;
  return m_ifc.imagedecoder->toAddon->load_image_from_memory(m_ifc.hdl, buffer, bufSize, &m_width,
                                                             &m_height);
}

bool CImageDecoder::Decode(unsigned char* const pixels,
                           unsigned int width,
                           unsigned int height,
                           unsigned int pitch,
                           unsigned int format)
{
  if (!m_ifc.imagedecoder->toAddon->decode)
    return false;

  auto it = KodiToAddonFormat.find(format & XB_FMT_MASK);
  if (it == KodiToAddonFormat.end())
    return false;

  bool result =
      m_ifc.imagedecoder->toAddon->decode(m_ifc.hdl, pixels, width, height, pitch, it->second);
  m_width = width;
  m_height = height;

  return result;
}

bool CImageDecoder::Create(const std::string& mimetype)
{
  m_ifc.imagedecoder->props->mimetype = mimetype.c_str();
  return (CreateInstance() == ADDON_STATUS_OK);
}

} /* namespace ADDON */
