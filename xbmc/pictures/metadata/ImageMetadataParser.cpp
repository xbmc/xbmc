/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ImageMetadataParser.h"

#include "filesystem/File.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <cctype>
#include <limits>
#include <vector>

#if (EXIV2_MAJOR_VERSION == 0) && (EXIV2_MINOR_VERSION < 28)
#define EXIV_toUint32 toLong
#define EXIV_toInt64 toLong
#else
#define EXIV_toUint32 toUint32
#define EXIV_toInt64 toInt64
#endif

using namespace XFILE;

namespace
{
std::string GetGPSString(const Exiv2::Value& value)
{
  const Exiv2::Rational degrees = value.toRational(0);
  const Exiv2::Rational minutes = value.toRational(1);
  Exiv2::Rational seconds = value.toRational(2);

  const int32_t dd = degrees.first;
  int32_t mm{0};
  float ss{0.0};
  if (minutes.second > 0)
  {
    const int32_t rem = minutes.first % minutes.second;
    mm = minutes.first / minutes.second;
    if ((seconds.first == 0) && (seconds.second == 1) &&
        (rem <= std::numeric_limits<int32_t>::max() / 60))
    {
      seconds.first = 60 * rem;
      seconds.second = minutes.second;
    }
  }
  if (seconds.second > 0)
  {
    ss = static_cast<float>(seconds.first) / seconds.second;
  }
  return StringUtils::Format("{}° {}' {:.3f}\"", dd, mm, ss);
}

bool IsOnlySpaces(const std::string& str)
{
  return std::all_of(str.begin(), str.end(), [](unsigned char c) { return std::isspace(c); });
}
} // namespace

CImageMetadataParser::CImageMetadataParser() : m_imageMetadata(std::make_unique<ImageMetadata>())
{
}

std::unique_ptr<ImageMetadata> CImageMetadataParser::ExtractMetadata(const std::string& picFileName)
{
  // read image file to a buffer so it can be fed to libexiv2
  CFile file;
  std::vector<uint8_t> outputBuffer;
  ssize_t readbytes = file.LoadFile(picFileName, outputBuffer);
  if (readbytes <= 0)
  {
    return nullptr;
  }

  // read image metadata
  auto image = Exiv2::ImageFactory::open(outputBuffer.data(), readbytes);
  image->readMetadata();

  CImageMetadataParser parser;

  // extract metadata
  parser.ExtractCommonMetadata(*image);

  parser.ExtractExif(image->exifData());
  parser.ExtractIPTC(image->iptcData());

  return std::move(parser.m_imageMetadata);
}

void CImageMetadataParser::ExtractCommonMetadata(Exiv2::Image& image)
{
  //! TODO: all these elements are generic should be moved out of the exif struct
  m_imageMetadata->height = image.pixelHeight();
  m_imageMetadata->width = image.pixelWidth();
  m_imageMetadata->fileComment = image.comment();
#if (EXIV2_MAJOR_VERSION >= 0) && (EXIV2_MINOR_VERSION >= 28) && (EXIV2_PATCH_VERSION >= 2)
  if (image.imageType() == Exiv2::ImageType::jpeg)
  {
    auto jpegImage = dynamic_cast<Exiv2::JpegImage*>(&image);
    m_imageMetadata->isColor = jpegImage->numColorComponents() == 3;
    m_imageMetadata->encodingProcess = jpegImage->encodingProcess();
  }
#endif
}

void CImageMetadataParser::ExtractExif(Exiv2::ExifData& exifData)
{
  for (auto it = exifData.begin(); it != exifData.end(); ++it)
  {
    const std::string exifKey = it->key();
    if (exifKey == "Exif.Image.Make")
    {
      m_imageMetadata->exifInfo.CameraMake = it->value().toString();
    }
    else if (exifKey == "Exif.Image.ImageDescription")
    {
      const std::string value = it->value().toString();
      if (!IsOnlySpaces(value))
      {
        m_imageMetadata->exifInfo.Description = value;
      }
    }
    else if (exifKey == "Exif.Image.Model")
    {
      m_imageMetadata->exifInfo.CameraModel = it->value().toString();
    }
    else if (exifKey == "Exif.Image.Orientation")
    {
      const int orientationValue = it->value().EXIV_toUint32();
      if (orientationValue < 0 || orientationValue > 8)
      {
        CLog::LogF(LOGWARNING, "Exif: Undefined rotation value {}",
                   m_imageMetadata->exifInfo.Orientation);
        continue;
      }
      m_imageMetadata->exifInfo.Orientation = orientationValue;
    }
    else if ((exifKey == "Exif.Image.DateTime" || exifKey == "Exif.Photo.DateTimeDigitized"))
    {
      if (m_imageMetadata->exifInfo.DateTime.empty())
        m_imageMetadata->exifInfo.DateTime = it->value().toString();
    }
    else if (exifKey == "Exif.Photo.ExposureTime")
    {
      m_imageMetadata->exifInfo.ExposureTime = it->value().toFloat();
    }
    else if (exifKey == "Exif.Photo.FNumber")
    {
      m_imageMetadata->exifInfo.ApertureFNumber = it->value().toFloat();
    }
    else if (exifKey == "Exif.Photo.ExposureProgram")
    {
      m_imageMetadata->exifInfo.ExposureProgram = it->value().EXIV_toUint32();
    }
    else if (exifKey == "Exif.Photo.ISOSpeedRatings")
    {
      m_imageMetadata->exifInfo.ISOequivalent = it->value().EXIV_toUint32();
    }
    else if (exifKey == "Exif.Photo.DateTimeOriginal")
    {
      m_imageMetadata->exifInfo.DateTime = it->value().toString();
    }
    else if (exifKey == "Exif.Photo.ApertureValue")
    {
      m_imageMetadata->exifInfo.ApertureFNumber =
          static_cast<double>(it->value().toFloat()) * log(2.0) * 0.5;
    }
    else if (exifKey == "Exif.Photo.MaxApertureValue")
    {
      // More relevant info always comes earlier, so only use this field if we don't
      // have appropriate aperture information yet.
      if (m_imageMetadata->exifInfo.ApertureFNumber == 0)
        m_imageMetadata->exifInfo.ApertureFNumber =
            static_cast<double>(it->value().toFloat()) * log(2.0) * 0.5;
    }
    else if (exifKey == "Exif.Photo.ShutterSpeedValue")
    {
      // More complicated way of expressing exposure time, so only use
      // this value if we don't already have it from somewhere else.
      if (m_imageMetadata->exifInfo.ExposureTime == 0)
      {
        m_imageMetadata->exifInfo.ExposureTime =
            1 / exp(static_cast<double>(it->value().toFloat()) * log(2.0));
      }
    }
    else if (exifKey == "Exif.Photo.ExposureBiasValue")
    {
      m_imageMetadata->exifInfo.ExposureBias = it->value().toFloat();
    }
    else if (exifKey == "Exif.Photo.MeteringMode")
    {
      m_imageMetadata->exifInfo.MeteringMode = it->value().EXIV_toUint32();
    }
    else if (exifKey == "Exif.Photo.Flash")
    {
      m_imageMetadata->exifInfo.FlashUsed = it->value().EXIV_toUint32();
    }
    else if (exifKey == "Exif.Photo.FocalLength")
    {
      m_imageMetadata->exifInfo.FocalLength = it->value().toFloat();
    }
    else if (exifKey == "Exif.Photo.SubjectDistance")
    {
      m_imageMetadata->exifInfo.Distance = it->value().toFloat();
    }
    else if (exifKey == "Exif.Image.XPComment")
    {
      m_imageMetadata->exifInfo.XPComment = it->value().toString();
    }
    else if (exifKey == "Exif.Photo.UserComment")
    {
      m_imageMetadata->exifInfo.Comments = it->value().toString();
    }
    else if (exifKey == "Exif.Photo.PixelXDimension" || exifKey == "Exif.Photo.PixelYDimension")
    {
      // Use largest of height and width to deal with images that have been
      // rotated to portrait format.
      {
        const int value = static_cast<int>(it->value().EXIV_toInt64());
        if (m_imageWidth < value)
        {
          m_imageWidth = value;
        }
      }
    }
    else if (exifKey == "Exif.Photo.FocalPlaneXResolution")
    {
      m_focalPlaneXRes = it->value().toFloat();
    }
    else if (exifKey == "Exif.Photo.FocalPlaneResolutionUnit")
    {
      const uint32_t value = it->value().EXIV_toUint32();
      // see: https://exiftool.org/TagNames/EXIF.html
      switch (value)
      {
        case 1:
          m_focalPlaneUnits = 0;
          break; // None
        case 2:
          m_focalPlaneUnits = 25.4;
          break; // inch
        case 3:
          m_focalPlaneUnits = 10;
          break; // centimeter
        case 4:
          m_focalPlaneUnits = 1;
          break; // millimeter
        case 5:
          m_focalPlaneUnits = .001;
          break; // micrometer
      }
    }
    else if (exifKey == "Exif.Photo.ExposureMode")
    {
      m_imageMetadata->exifInfo.ExposureMode = it->value().EXIV_toUint32();
    }
    else if (exifKey == "Exif.Photo.WhiteBalance")
    {
      m_imageMetadata->exifInfo.Whitebalance = it->value().EXIV_toUint32();
    }
    else if (exifKey == "Exif.Photo.LightSource")
    {
      m_imageMetadata->exifInfo.LightSource = it->value().EXIV_toUint32();
    }
    else if (exifKey == "Exif.Photo.DigitalZoomRatio")
    {
      m_imageMetadata->exifInfo.DigitalZoomRatio = it->value().toFloat();
    }
    else if (exifKey == "Exif.Photo.FocalLengthIn35mmFilm")
    {
      // The focal length equivalent 35 mm is a 2.2 tag (defined as of April 2002)
      // if its present, use it to compute equivalent focal length instead of
      // computing it from sensor geometry and actual focal length.
      m_imageMetadata->exifInfo.FocalLength35mmEquiv = it->value().EXIV_toUint32();
    }
    else if (exifKey == "Exif.GPSInfo.GPSLatitudeRef")
    {
      if (m_imageMetadata->exifInfo.GpsLat.empty())
      {
        m_imageMetadata->exifInfo.GpsLat = StringUtils::Format(" {}", it->value().toString());
      }
      else
      {
        m_imageMetadata->exifInfo.GpsLat =
            StringUtils::Format("{} {}", m_imageMetadata->exifInfo.GpsLat, it->value().toString());
      }
    }
    else if (exifKey == "Exif.GPSInfo.GPSLongitudeRef")
    {
      if (m_imageMetadata->exifInfo.GpsLong.empty())
      {
        m_imageMetadata->exifInfo.GpsLong = StringUtils::Format(" {}", it->value().toString());
      }
      else
      {
        m_imageMetadata->exifInfo.GpsLong =
            StringUtils::Format("{} {}", m_imageMetadata->exifInfo.GpsLong, it->value().toString());
      }
    }
    else if (exifKey == "Exif.GPSInfo.GPSLatitude")
    {
      m_imageMetadata->exifInfo.GpsLat =
          GetGPSString(it->value()) + m_imageMetadata->exifInfo.GpsLat;
    }
    else if (exifKey == "Exif.GPSInfo.GPSLongitude")
    {
      m_imageMetadata->exifInfo.GpsLong =
          GetGPSString(it->value()) + m_imageMetadata->exifInfo.GpsLong;
    }
    else if (exifKey == "Exif.GPSInfo.GPSAltitude")
    {
      m_imageMetadata->exifInfo.GpsAlt += StringUtils::Format("{}m", it->value().toFloat());
    }
    else if (exifKey == "Exif.GPSInfo.GPSAltitudeRef")
    {
      auto value = it->value().EXIV_toUint32();
      if (value == 1) // below sea level
      {
        m_imageMetadata->exifInfo.GpsAlt = "-" + m_imageMetadata->exifInfo.GpsAlt;
      }
    }
  }

  // Compute the CCD width, in millimeters.
  if (m_focalPlaneXRes != 0)
  {
    // Note: With some cameras, its not possible to compute this correctly because
    // they don't adjust the indicated focal plane resolution units when using less
    // than maximum resolution, so the CCDWidth value comes out too small.
    m_imageMetadata->exifInfo.CCDWidth =
        static_cast<float>(m_imageWidth * m_focalPlaneUnits / m_focalPlaneXRes);
  }

  if (m_imageMetadata->exifInfo.FocalLength)
  {
    if (m_imageMetadata->exifInfo.FocalLength35mmEquiv == 0)
    {
      // Compute 35 mm equivalent focal length based on sensor geometry if we haven't
      // already got it explicitly from a tag.
      if (m_imageMetadata->exifInfo.CCDWidth != 0.0f)
      {
        m_imageMetadata->exifInfo.FocalLength35mmEquiv = static_cast<int>(
            (m_imageMetadata->exifInfo.FocalLength / m_imageMetadata->exifInfo.CCDWidth * 36 +
             0.5f));
      }
    }
  }
}

void CImageMetadataParser::ExtractIPTC(Exiv2::IptcData& iptcData)
{
  for (auto it = iptcData.begin(); it != iptcData.end(); ++it)
  {
    const std::string iptcKey = it->key();
    if (iptcKey == "Iptc.Application2.RecordVersion")
    {
      m_imageMetadata->iptcInfo.RecordVersion = it->value().toString();
    }
    else if (iptcKey == "Iptc.Application2.SuppCategory")
    {
      m_imageMetadata->iptcInfo.SupplementalCategories = it->value().toString();
    }
    else if (iptcKey == "Iptc.Application2.Keywords")
    {
      if (m_imageMetadata->iptcInfo.Keywords.empty())
      {
        m_imageMetadata->iptcInfo.Keywords = it->value().toString();
      }
      else
      {
        m_imageMetadata->iptcInfo.Keywords += ", " + it->value().toString();
      }
    }
    else if (iptcKey == "Iptc.Application2.Caption")
    {
      m_imageMetadata->iptcInfo.Caption = it->value().toString();
    }
    else if (iptcKey == "Iptc.Application2.Writer")
    {
      m_imageMetadata->iptcInfo.Author = it->value().toString();
    }
    else if (iptcKey == "Iptc.Application2.Headline")
    {
      m_imageMetadata->iptcInfo.Headline = it->value().toString();
    }
    else if (iptcKey == "Iptc.Application2.SpecialInstructions")
    {
      m_imageMetadata->iptcInfo.SpecialInstructions = it->value().toString();
    }
    else if (iptcKey == "Iptc.Application2.Category")
    {
      m_imageMetadata->iptcInfo.Category = it->value().toString();
    }
    else if (iptcKey == "Iptc.Application2.Byline")
    {
      m_imageMetadata->iptcInfo.Byline = it->value().toString();
    }
    else if (iptcKey == "Iptc.Application2.BylineTitle")
    {
      m_imageMetadata->iptcInfo.BylineTitle = it->value().toString();
    }
    else if (iptcKey == "Iptc.Application2.Credit")
    {
      m_imageMetadata->iptcInfo.Credit = it->value().toString();
    }
    else if (iptcKey == "Iptc.Application2.Source")
    {
      m_imageMetadata->iptcInfo.Source = it->value().toString();
    }
    else if (iptcKey == "Iptc.Application2.Copyright")
    {
      m_imageMetadata->iptcInfo.CopyrightNotice = it->value().toString();
    }
    else if (iptcKey == "Iptc.Application2.ObjectName")
    {
      m_imageMetadata->iptcInfo.ObjectName = it->value().toString();
    }
    else if (iptcKey == "Iptc.Application2.City")
    {
      m_imageMetadata->iptcInfo.City = it->value().toString();
    }
    else if (iptcKey == "Iptc.Application2.ProvinceState")
    {
      m_imageMetadata->iptcInfo.State = it->value().toString();
    }
    else if (iptcKey == "Iptc.Application2.CountryName")
    {
      m_imageMetadata->iptcInfo.Country = it->value().toString();
    }
    else if (iptcKey == "Iptc.Application2.TransmissionReference")
    {
      m_imageMetadata->iptcInfo.TransmissionReference = it->value().toString();
    }
    else if (iptcKey == "Iptc.Application2.Urgency")
    {
      m_imageMetadata->iptcInfo.Urgency = it->value().toString();
    }
    else if (iptcKey == "Iptc.Application2.CountryCode")
    {
      m_imageMetadata->iptcInfo.CountryCode = it->value().toString();
    }
    else if (iptcKey == "Iptc.Application2.ReferenceService")
    {
      m_imageMetadata->iptcInfo.ReferenceService = it->value().toString();
    }
    else if (iptcKey == "Iptc.Application2.SubLocation")
    {
      m_imageMetadata->iptcInfo.SubLocation = it->value().toString();
    }
    else if (iptcKey == "Iptc.Application2.ImageType")
    {
      m_imageMetadata->iptcInfo.ImageType = it->value().toString();
    }
    else if (iptcKey == "Iptc.Application2.DateCreated")
    {
      m_imageMetadata->iptcInfo.Date = it->value().toString();
    }
    else if (iptcKey == "Iptc.Application2.DateCreated")
    {
      m_imageMetadata->iptcInfo.Date = it->value().toString();
    }
    else if (iptcKey == "Iptc.Application2.TimeCreated")
    {
      m_imageMetadata->iptcInfo.TimeCreated = it->value().toString();
    }
  }
}
