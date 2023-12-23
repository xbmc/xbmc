/*
 *  Copyright (C) 2013 Arne Morten Kvarving
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ImageDecoder.h"

#include "guilib/TextureFormats.h"
#include "pictures/PictureInfoTag.h"
#include "utils/StringUtils.h"

using namespace ADDON;
using namespace KODI::ADDONS;

namespace
{

constexpr std::array<std::tuple<unsigned int, ADDON_IMG_FMT, size_t>, 4> KodiToAddonFormat = {
    {{XB_FMT_A8R8G8B8, ADDON_IMG_FMT_A8R8G8B8, sizeof(uint8_t) * 4},
     {XB_FMT_A8, ADDON_IMG_FMT_A8, sizeof(uint8_t) * 1},
     {XB_FMT_RGBA8, ADDON_IMG_FMT_RGBA8, sizeof(uint8_t) * 4},
     {XB_FMT_RGB8, ADDON_IMG_FMT_RGB8, sizeof(uint8_t) * 3}}};

} /* namespace */

CImageDecoder::CImageDecoder(const AddonInfoPtr& addonInfo, const std::string& mimetype)
  : IAddonInstanceHandler(ADDON_INSTANCE_IMAGEDECODER, addonInfo), m_mimetype(mimetype)
{
  // Create all interface parts independent to make API changes easier if
  // something is added
  m_ifc.imagedecoder = new AddonInstance_ImageDecoder;
  m_ifc.imagedecoder->toAddon = new KodiToAddonFuncTable_ImageDecoder();
  m_ifc.imagedecoder->toKodi = new AddonToKodiFuncTable_ImageDecoder();

  if (CreateInstance() != ADDON_STATUS_OK)
    return;

  m_created = true;
}

CImageDecoder::~CImageDecoder()
{
  DestroyInstance();

  delete m_ifc.imagedecoder->toKodi;
  delete m_ifc.imagedecoder->toAddon;
  delete m_ifc.imagedecoder;
}

bool CImageDecoder::SupportsFile(const std::string& filename)
{
  // Create in case not available, possible as this done by IAddonSupportCheck
  if (!m_created || !m_ifc.imagedecoder->toAddon->supports_file)
    return false;

  return m_ifc.imagedecoder->toAddon->supports_file(m_ifc.hdl, filename.c_str());
}

bool CImageDecoder::LoadInfoTag(const std::string& fileName, CPictureInfoTag* tag)
{
  if (!m_created || !m_ifc.imagedecoder->toAddon->read_tag || !tag)
    return false;

  KODI_ADDON_IMAGEDECODER_INFO_TAG ifcTag = {};
  bool ret = m_ifc.imagedecoder->toAddon->read_tag(m_ifc.hdl, fileName.c_str(), &ifcTag);
  if (ret)
  {
    /*!
     * List of values currently not used on addon interface.
     *
     * struct ExifInfo:
     *   - int Process{};
     *   - float CCDWidth{};
     *   - int Whitebalance{};
     *   - int CommentsCharset{};
     *   - int XPCommentsCharset{};
     *   - std::string Comments;
     *   - std::string FileComment;
     *   - std::string XPComment;
     *   - unsigned ThumbnailOffset{};
     *   - unsigned ThumbnailSize{};
     *   - unsigned LargestExifOffset{};
     *   - char ThumbnailAtEnd{};
     *   - int ThumbnailSizeOffset{};
     *   - std::vector<int> DateTimeOffsets;
     *
     * struct IPTCInfo:
     *   - std::string RecordVersion;
     *   - std::string SupplementalCategories;
     *   - std::string Keywords;
     *   - std::string Caption;
     *   - std::string Headline;
     *   - std::string SpecialInstructions;
     *   - std::string Category;
     *   - std::string Byline;
     *   - std::string BylineTitle;
     *   - std::string Credit;
     *   - std::string Source;
     *   - std::string ObjectName;
     *   - std::string City;
     *   - std::string State;
     *   - std::string Country;
     *   - std::string TransmissionReference;
     *   - std::string Date;
     *   - std::string Urgency;
     *   - std::string ReferenceService;
     *   - std::string CountryCode;
     *   - std::string SubLocation;
     *   - std::string ImageType;
     *
     * @todo Rework @ref CPictureInfoTag to not limit on fixed structures ExifInfo & IPTCInfo.
     */

    tag->m_exifInfo.Width = ifcTag.width;
    tag->m_exifInfo.Height = ifcTag.height;
    tag->m_exifInfo.Distance = ifcTag.distance;
    tag->m_exifInfo.Orientation = ifcTag.orientation;
    tag->m_exifInfo.IsColor = ifcTag.color == ADDON_IMG_COLOR_COLORED ? 1 : 0;
    tag->m_exifInfo.ApertureFNumber = ifcTag.aperture_f_number;
    tag->m_exifInfo.FlashUsed = ifcTag.flash_used ? 1 : 0;
    tag->m_exifInfo.LightSource = ifcTag.light_source;
    tag->m_exifInfo.FocalLength = ifcTag.focal_length;
    tag->m_exifInfo.FocalLength35mmEquiv = ifcTag.focal_length_in_35mm_format;
    tag->m_exifInfo.MeteringMode = ifcTag.metering_mode;
    tag->m_exifInfo.DigitalZoomRatio = ifcTag.digital_zoom_ratio;
    tag->m_exifInfo.ExposureTime = ifcTag.exposure_time;
    tag->m_exifInfo.ExposureBias = ifcTag.exposure_bias;
    tag->m_exifInfo.ExposureProgram = ifcTag.exposure_program;
    tag->m_exifInfo.ExposureMode = ifcTag.exposure_mode;
    tag->m_exifInfo.ISOequivalent = static_cast<int>(ifcTag.iso_speed);
    CDateTime dt;
    dt.SetFromUTCDateTime(ifcTag.time_created);
    tag->m_iptcInfo.TimeCreated = dt.GetAsLocalizedDateTime();
    tag->m_dateTimeTaken = dt;
    tag->m_exifInfo.GpsInfoPresent = ifcTag.gps_info_present;
    if (tag->m_exifInfo.GpsInfoPresent)
    {
      tag->m_exifInfo.GpsLat =
          StringUtils::Format("{}{:.0f}°{:.0f}'{:.2f}\"", ifcTag.latitude_ref, ifcTag.latitude[0],
                              ifcTag.latitude[1], ifcTag.latitude[2]);
      tag->m_exifInfo.GpsLong =
          StringUtils::Format("{}{:.0f}°{:.0f}'{:.2f}\"", ifcTag.longitude_ref, ifcTag.longitude[0],
                              ifcTag.longitude[1], ifcTag.longitude[2]);
      tag->m_exifInfo.GpsAlt =
          StringUtils::Format("{}{:.2f} m", ifcTag.altitude_ref ? '-' : '+', ifcTag.altitude);
    }

    if (ifcTag.camera_manufacturer)
    {
      tag->m_exifInfo.CameraMake = ifcTag.camera_manufacturer;
      free(ifcTag.camera_manufacturer);
    }
    if (ifcTag.camera_model)
    {
      tag->m_exifInfo.CameraModel = ifcTag.camera_model;
      free(ifcTag.camera_model);
    }
    if (ifcTag.author)
    {
      tag->m_iptcInfo.Author = ifcTag.author;
      free(ifcTag.author);
    }
    if (ifcTag.description)
    {
      tag->m_exifInfo.Description = ifcTag.description;
      free(ifcTag.description);
    }
    if (ifcTag.copyright)
    {
      tag->m_iptcInfo.CopyrightNotice = ifcTag.copyright;
      free(ifcTag.copyright);
    }
  }

  return ret;
}

bool CImageDecoder::LoadImageFromMemory(unsigned char* buffer,
                                        unsigned int bufSize,
                                        unsigned int width,
                                        unsigned int height)
{
  if (!m_created || !m_ifc.imagedecoder->toAddon->load_image_from_memory)
    return false;

  m_width = width;
  m_height = height;
  return m_ifc.imagedecoder->toAddon->load_image_from_memory(m_ifc.hdl, m_mimetype.c_str(), buffer,
                                                             bufSize, &m_width, &m_height);
}

bool CImageDecoder::Decode(unsigned char* const pixels,
                           unsigned int width,
                           unsigned int height,
                           unsigned int pitch,
                           unsigned int format)
{
  if (!m_created || !m_ifc.imagedecoder->toAddon->decode)
    return false;

  const auto it = std::find_if(KodiToAddonFormat.begin(), KodiToAddonFormat.end(),
                               [format](auto& p) { return std::get<0>(p) == format; });
  if (it == KodiToAddonFormat.end())
    return false;

  const ADDON_IMG_FMT addonFmt = std::get<1>(*it);
  const size_t size = width * height * std::get<2>(*it);
  const bool result =
      m_ifc.imagedecoder->toAddon->decode(m_ifc.hdl, pixels, size, width, height, pitch, addonFmt);
  m_width = width;
  m_height = height;

  return result;
}
