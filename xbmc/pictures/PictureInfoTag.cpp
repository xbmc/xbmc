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

#include "PictureInfoTag.h"
#include "XBDateTime.h"
#include "Util.h"
#include "utils/RegExp.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/CharsetConverter.h"
#include "filesystem/File.h"

#include "system.h"
#include "lib/bson/src/bson.h"

using namespace std;
using namespace XFILE;

void CPictureInfoTag::Reset()
{
  memset(&m_exifInfo, 0, sizeof(m_exifInfo));
  memset(&m_iptcInfo, 0, sizeof(m_iptcInfo));
  m_file.clear();
  m_path.clear();
  m_databaseID = -1;
  m_size = 0;
  m_folder.clear();
  m_year = 0;
  m_camera.clear();
  m_tags.clear();
  m_isLoaded = false;
}

const CPictureInfoTag& CPictureInfoTag::operator=(const CPictureInfoTag& right)
{
  if (this == &right) return * this;
  memcpy(&m_exifInfo, &right.m_exifInfo, sizeof(m_exifInfo));
  memcpy(&m_iptcInfo, &right.m_iptcInfo, sizeof(m_iptcInfo));
  m_file = right.m_file;
  m_path = right.m_path;
  m_databaseID = right.m_databaseID;
  m_size = right.m_size;
  m_folder = right.m_folder;
  m_year = right.m_year;
  m_camera = right.m_camera;
  m_tags = right.m_tags;
  m_isLoaded = right.m_isLoaded;
  return *this;
}

bool CPictureInfoTag::GetDateTime(CDateTime &datetime) const
{
  // EXIF datetime looks like 2003:12:14 12:01:44
  CStdString strDatetime(m_exifInfo.DateTime);
  if (strDatetime.length() == 19)
  {
    int year = atoi(strDatetime.substr(0, 4).c_str());
    int month = atoi(strDatetime.substr(5, 2).c_str());
    int day = atoi(strDatetime.substr(8, 2).c_str());
    int hour = atoi(strDatetime.substr(11, 2).c_str());
    int minute = atoi(strDatetime.substr(14, 2).c_str());
    int second = atoi(strDatetime.substr(17, 2).c_str());
    // Enforce valid year, month and day
    if (year != 0 && month != 0 && day != 0)
    {
      datetime = CDateTime(year, month, day, hour, minute, second);
      return true;
    }
  }
  return false;
}

bool CPictureInfoTag::Load(const CStdString &path)
{
  m_isLoaded = false;

  DllLibExif exifDll;
  if (path.IsEmpty() || !exifDll.Load())
    return false;

  URIUtils::Split(path, m_path, m_file);
  m_databaseID = -1; // reset this

  if (exifDll.process_jpeg(path.c_str(), &m_exifInfo, &m_iptcInfo))
  {
    m_isLoaded = true;

    // Extract the year
    CStdString datetime (m_exifInfo.DateTime);
    if (datetime.length() >= 4)
      m_year = atoi(datetime.substr(0, 4).c_str());

    // Get the file size
    CFile file;
    file.Open(path);
    m_size = file.GetLength();
    file.Close();

    CStdString pathCopy(m_path);
    URIUtils::RemoveSlashAtEnd(pathCopy);
    m_folder = URIUtils::GetFileName(pathCopy);

    // Camera name
    SetCamera(m_exifInfo.CameraMake, m_exifInfo.CameraModel);

    // Parse tags
    SetTags(m_iptcInfo.Keywords);
  }
  return m_isLoaded;
}

void CPictureInfoTag::SetCamera(CStdString make, CStdString model)
{
  m_camera.clear();
  make.Trim();
  model.Trim();

  // Casify the make (might look like KODAK PICTURE GROUP)
  // This helps set the make apart from the model, and tests show that all caps
  // dramatically reduces readability
  CStdStringArray words;
  StringUtils::SplitString(make, " ", words);
  for (CStdStringArray::const_iterator it = words.begin(); it != words.end(); it++)
  {
    if (!it->length())
      continue;
    if (it->length() == 1)
    {
      m_camera += *it + " ";
      continue;
    }
    // Skip some popular abbreviated names
    if (*it == "HP" || *it == "JVC" || *it == "RIM" || it->Left(3) == "MSM" ||
        *it == "LOMO" || *it == "DHW" || *it == "HTC")
    {
      m_camera += *it + " ";
      continue;
    }
    CStdString thecap = it->substr(0, 1);
    thecap.ToUpper();
    CStdString therest = it->substr(1, string::npos);
    therest.ToLower();
    m_camera += thecap + therest + " ";
  }

  // Sometimes the Make shows up in the Model (Canon Canon PowerShot)
  // Avoid duplication
  if ((model.Left(m_camera.length() - 1).Equals(m_camera))) // ignore case
    m_camera = model;
  else
    m_camera += model;
}

void CPictureInfoTag::SetTags(CStdString csvTags)
{
  m_tags.clear();

  csvTags.Trim();
  if (!csvTags.length())
    return;

  // Try exploding by semicolon first, comma second
  CStdStringArray tags;
  StringUtils::SplitString(csvTags, ";", tags);
  if (tags.size() == 1)
  {
    tags.clear();
    StringUtils::SplitString(csvTags, ",", tags);
  }

  for (CStdStringArray::iterator it = tags.begin(); it != tags.end(); it++)
  {
    it->Trim();
    m_tags.push_back(it->c_str());
  }
}

void CPictureInfoTag::Archive(CArchive& ar)
{
  if (ar.IsStoring())
  {
    ar << m_isLoaded;
    ar << m_exifInfo.ApertureFNumber;
    ar << CStdString(m_exifInfo.CameraMake);
    ar << CStdString(m_exifInfo.CameraModel);
    ar << m_exifInfo.CCDWidth;
    ar << GetInfo(SLIDE_EXIF_COMMENT); // Store and restore the comment charset converted
    ar << CStdString(m_exifInfo.Description);
    ar << CStdString(m_exifInfo.DateTime);
    for (int i = 0; i < 10; i++)
      ar << m_exifInfo.DateTimeOffsets[i];
    ar << m_exifInfo.DigitalZoomRatio;
    ar << m_exifInfo.Distance;
    ar << m_exifInfo.ExposureBias;
    ar << m_exifInfo.ExposureMode;
    ar << m_exifInfo.ExposureProgram;
    ar << m_exifInfo.ExposureTime;
    ar << m_exifInfo.FlashUsed;
    ar << m_exifInfo.FocalLength;
    ar << m_exifInfo.FocalLength35mmEquiv;
    ar << m_exifInfo.GpsInfoPresent;
    ar << CStdString(m_exifInfo.GpsAlt);
    ar << CStdString(m_exifInfo.GpsLat);
    ar << CStdString(m_exifInfo.GpsLong);
    ar << m_exifInfo.Height;
    ar << m_exifInfo.IsColor;
    ar << m_exifInfo.ISOequivalent;
    ar << m_exifInfo.LargestExifOffset;
    ar << m_exifInfo.LightSource;
    ar << m_exifInfo.MeteringMode;
    ar << m_exifInfo.numDateTimeTags;
    ar << m_exifInfo.Orientation;
    ar << m_exifInfo.Process;
    ar << m_exifInfo.ThumbnailAtEnd;
    ar << m_exifInfo.ThumbnailOffset;
    ar << m_exifInfo.ThumbnailSize;
    ar << m_exifInfo.ThumbnailSizeOffset;
    ar << m_exifInfo.Whitebalance;
    ar << m_exifInfo.Width;

    ar << CStdString(m_iptcInfo.Author);
    ar << CStdString(m_iptcInfo.Byline);
    ar << CStdString(m_iptcInfo.BylineTitle);
    ar << CStdString(m_iptcInfo.Caption);
    ar << CStdString(m_iptcInfo.Category);
    ar << CStdString(m_iptcInfo.City);
    ar << CStdString(m_iptcInfo.Copyright);
    ar << CStdString(m_iptcInfo.CopyrightNotice);
    ar << CStdString(m_iptcInfo.Country);
    ar << CStdString(m_iptcInfo.CountryCode);
    ar << CStdString(m_iptcInfo.Credit);
    ar << CStdString(m_iptcInfo.Date);
    ar << CStdString(m_iptcInfo.Headline);
    ar << CStdString(m_iptcInfo.Keywords);
    ar << CStdString(m_iptcInfo.ObjectName);
    ar << CStdString(m_iptcInfo.ReferenceService);
    ar << CStdString(m_iptcInfo.Source);
    ar << CStdString(m_iptcInfo.SpecialInstructions);
    ar << CStdString(m_iptcInfo.State);
    ar << CStdString(m_iptcInfo.SupplementalCategories);
    ar << CStdString(m_iptcInfo.TransmissionReference);

    ar << m_file;
    ar << m_path;
    ar << m_databaseID;
    ar << m_size;
    ar << m_folder;
    ar << m_year;
    ar << m_camera;
    ar << m_tags;
  }
  else
  {
    ar >> m_isLoaded;
    ar >> m_exifInfo.ApertureFNumber;
    GetStringFromArchive(ar, m_exifInfo.CameraMake, sizeof(m_exifInfo.CameraMake));
    GetStringFromArchive(ar, m_exifInfo.CameraModel, sizeof(m_exifInfo.CameraModel));
    ar >> m_exifInfo.CCDWidth;
    GetStringFromArchive(ar, m_exifInfo.Comments, sizeof(m_exifInfo.Comments));
    m_exifInfo.CommentsCharset = EXIF_COMMENT_CHARSET_CONVERTED; // Store and restore the comment charset converted
    GetStringFromArchive(ar, m_exifInfo.Description, sizeof(m_exifInfo.Description));
    GetStringFromArchive(ar, m_exifInfo.DateTime, sizeof(m_exifInfo.DateTime));
    for (int i = 0; i < 10; i++)
      ar >> m_exifInfo.DateTimeOffsets[i];
    ar >> m_exifInfo.DigitalZoomRatio;
    ar >> m_exifInfo.Distance;
    ar >> m_exifInfo.ExposureBias;
    ar >> m_exifInfo.ExposureMode;
    ar >> m_exifInfo.ExposureProgram;
    ar >> m_exifInfo.ExposureTime;
    ar >> m_exifInfo.FlashUsed;
    ar >> m_exifInfo.FocalLength;
    ar >> m_exifInfo.FocalLength35mmEquiv;
    ar >> m_exifInfo.GpsInfoPresent;
    GetStringFromArchive(ar, m_exifInfo.GpsAlt, sizeof(m_exifInfo.GpsAlt));
    GetStringFromArchive(ar, m_exifInfo.GpsLat, sizeof(m_exifInfo.GpsLat));
    GetStringFromArchive(ar, m_exifInfo.GpsLong, sizeof(m_exifInfo.GpsLong));
    ar >> m_exifInfo.Height;
    ar >> m_exifInfo.IsColor;
    ar >> m_exifInfo.ISOequivalent;
    ar >> m_exifInfo.LargestExifOffset;
    ar >> m_exifInfo.LightSource;
    ar >> m_exifInfo.MeteringMode;
    ar >> m_exifInfo.numDateTimeTags;
    ar >> m_exifInfo.Orientation;
    ar >> m_exifInfo.Process;
    ar >> m_exifInfo.ThumbnailAtEnd;
    ar >> m_exifInfo.ThumbnailOffset;
    ar >> m_exifInfo.ThumbnailSize;
    ar >> m_exifInfo.ThumbnailSizeOffset;
    ar >> m_exifInfo.Whitebalance;
    ar >> m_exifInfo.Width;

    GetStringFromArchive(ar, m_iptcInfo.Author, sizeof(m_iptcInfo.Author));
    GetStringFromArchive(ar, m_iptcInfo.Byline, sizeof(m_iptcInfo.Byline));
    GetStringFromArchive(ar, m_iptcInfo.BylineTitle, sizeof(m_iptcInfo.BylineTitle));
    GetStringFromArchive(ar, m_iptcInfo.Caption, sizeof(m_iptcInfo.Caption));
    GetStringFromArchive(ar, m_iptcInfo.Category, sizeof(m_iptcInfo.Category));
    GetStringFromArchive(ar, m_iptcInfo.City, sizeof(m_iptcInfo.City));
    GetStringFromArchive(ar, m_iptcInfo.Copyright, sizeof(m_iptcInfo.Copyright));
    GetStringFromArchive(ar, m_iptcInfo.CopyrightNotice, sizeof(m_iptcInfo.CopyrightNotice));
    GetStringFromArchive(ar, m_iptcInfo.Country, sizeof(m_iptcInfo.Country));
    GetStringFromArchive(ar, m_iptcInfo.CountryCode, sizeof(m_iptcInfo.CountryCode));
    GetStringFromArchive(ar, m_iptcInfo.Credit, sizeof(m_iptcInfo.Credit));
    GetStringFromArchive(ar, m_iptcInfo.Date, sizeof(m_iptcInfo.Date));
    GetStringFromArchive(ar, m_iptcInfo.Headline, sizeof(m_iptcInfo.Headline));
    GetStringFromArchive(ar, m_iptcInfo.Keywords, sizeof(m_iptcInfo.Keywords));
    GetStringFromArchive(ar, m_iptcInfo.ObjectName, sizeof(m_iptcInfo.ObjectName));
    GetStringFromArchive(ar, m_iptcInfo.ReferenceService, sizeof(m_iptcInfo.ReferenceService));
    GetStringFromArchive(ar, m_iptcInfo.Source, sizeof(m_iptcInfo.Source));
    GetStringFromArchive(ar, m_iptcInfo.SpecialInstructions, sizeof(m_iptcInfo.SpecialInstructions));
    GetStringFromArchive(ar, m_iptcInfo.State, sizeof(m_iptcInfo.State));
    GetStringFromArchive(ar, m_iptcInfo.SupplementalCategories, sizeof(m_iptcInfo.SupplementalCategories));
    GetStringFromArchive(ar, m_iptcInfo.TransmissionReference, sizeof(m_iptcInfo.TransmissionReference));

    ar >> m_file;
    ar >> m_path;
    ar >> m_databaseID;
    ar >> m_size;
    ar >> m_folder;
    ar >> m_year;
    ar >> m_camera;
    ar >> m_tags;
  }
}

void CPictureInfoTag::Serialize(CVariant& value) const
{
  value["aperturefnumber"] = m_exifInfo.ApertureFNumber;
  value["cameramake"] = CStdString(m_exifInfo.CameraMake);
  value["cameramodel"] = CStdString(m_exifInfo.CameraModel);
  value["ccdwidth"] = m_exifInfo.CCDWidth;
  value["comments"] = GetInfo(SLIDE_EXIF_COMMENT); // Charset conversion
  value["description"] = CStdString(m_exifInfo.Description);
  value["datetime"] = CStdString(m_exifInfo.DateTime);
  for (int i = 0; i < 10; i++)
    value["datetimeoffsets"][i] = m_exifInfo.DateTimeOffsets[i];
  value["digitalzoomratio"] = m_exifInfo.DigitalZoomRatio;
  value["distance"] = m_exifInfo.Distance;
  value["exposurebias"] = m_exifInfo.ExposureBias;
  value["exposuremode"] = m_exifInfo.ExposureMode;
  value["exposureprogram"] = m_exifInfo.ExposureProgram;
  value["exposuretime"] = m_exifInfo.ExposureTime;
  value["flashused"] = m_exifInfo.FlashUsed;
  value["focallength"] = m_exifInfo.FocalLength;
  value["focallength35mmequiv"] = m_exifInfo.FocalLength35mmEquiv;
  value["gpsinfopresent"] = m_exifInfo.GpsInfoPresent;
  value["gpsalt"] = CStdString(m_exifInfo.GpsAlt);
  value["gpslat"] = CStdString(m_exifInfo.GpsLat);
  value["gpslong"] = CStdString(m_exifInfo.GpsLong);
  value["height"] = m_exifInfo.Height;
  value["iscolor"] = m_exifInfo.IsColor;
  value["isoequivalent"] = m_exifInfo.ISOequivalent;
  value["largestexifoffset"] = m_exifInfo.LargestExifOffset;
  value["lightsource"] = m_exifInfo.LightSource;
  value["meteringmode"] = m_exifInfo.MeteringMode;
  value["numdatetimetags"] = m_exifInfo.numDateTimeTags;
  value["orientation"] = m_exifInfo.Orientation;
  value["process"] = m_exifInfo.Process;
  value["thumbnailatend"] = m_exifInfo.ThumbnailAtEnd;
  value["thumbnailoffset"] = m_exifInfo.ThumbnailOffset;
  value["thumbnailsize"] = m_exifInfo.ThumbnailSize;
  value["thumbnailsizeoffset"] = m_exifInfo.ThumbnailSizeOffset;
  value["whitebalance"] = m_exifInfo.Whitebalance;
  value["width"] = m_exifInfo.Width;

  value["author"] = CStdString(m_iptcInfo.Author);
  value["byline"] = CStdString(m_iptcInfo.Byline);
  value["bylinetitle"] = CStdString(m_iptcInfo.BylineTitle);
  value["caption"] = CStdString(m_iptcInfo.Caption);
  value["category"] = CStdString(m_iptcInfo.Category);
  value["city"] = CStdString(m_iptcInfo.City);
  value["copyright"] = CStdString(m_iptcInfo.Copyright);
  value["copyrightnotice"] = CStdString(m_iptcInfo.CopyrightNotice);
  value["country"] = CStdString(m_iptcInfo.Country);
  value["countrycode"] = CStdString(m_iptcInfo.CountryCode);
  value["credit"] = CStdString(m_iptcInfo.Credit);
  value["date"] = CStdString(m_iptcInfo.Date);
  value["headline"] = CStdString(m_iptcInfo.Headline);
  value["keywords"] = CStdString(m_iptcInfo.Keywords);
  value["objectname"] = CStdString(m_iptcInfo.ObjectName);
  value["referenceservice"] = CStdString(m_iptcInfo.ReferenceService);
  value["source"] = CStdString(m_iptcInfo.Source);
  value["specialinstructions"] = CStdString(m_iptcInfo.SpecialInstructions);
  value["state"] = CStdString(m_iptcInfo.State);
  value["supplementalcategories"] = CStdString(m_iptcInfo.SupplementalCategories);
  value["transmissionreference"] = CStdString(m_iptcInfo.TransmissionReference);

  value["isloaded"] = m_isLoaded;
  value["path"] = m_path;
  value["file"] = m_file;
  value["databaseid"] = m_databaseID;
  value["size"] = m_size;
  value["folder"] = m_folder;
  value["year"] = m_year;
  value["camera"] = m_camera;
  value["tag"] = m_tags;
}

void CPictureInfoTag::Serialize(bson *document) const
{
  // Good BSON resources are http://api.mongodb.org/c/current/bson.html and lib/bson/docs/examples/example.c
  CStdString arrayKey;

  bson_init(document);

  bson_append_double(document, "aperturefnumber", m_exifInfo.ApertureFNumber);
  bson_append_string(document, "cameramake", m_exifInfo.CameraMake);
  bson_append_string(document, "cameramodel", m_exifInfo.CameraModel);
  bson_append_double(document, "ccdwidth", m_exifInfo.CCDWidth);
  bson_append_string(document, "comments", GetInfo(SLIDE_EXIF_COMMENT).c_str()); // Charset conversion
  bson_append_string(document, "description", m_exifInfo.Description);
  bson_append_string(document, "datetime", m_exifInfo.DateTime);
  bson_append_start_array(document, "datetimeoffsets");
  for (int i = 0; i < 10; i++)
  {
    arrayKey.Format("%d", i);
    bson_append_int(document, arrayKey.c_str(), m_exifInfo.DateTimeOffsets[i]);
  }
  bson_append_finish_array(document);
  bson_append_double(document, "digitalzoomratio", m_exifInfo.DigitalZoomRatio);
  bson_append_double(document, "distance", m_exifInfo.Distance);
  bson_append_double(document, "exposurebias", m_exifInfo.ExposureBias);
  bson_append_int(document, "exposuremode", m_exifInfo.ExposureMode);
  bson_append_int(document, "exposureprogram", m_exifInfo.ExposureProgram);
  bson_append_double(document, "exposuretime", m_exifInfo.ExposureTime);
  bson_append_int(document, "flashused", m_exifInfo.FlashUsed);
  bson_append_double(document, "focallength", m_exifInfo.FocalLength);
  bson_append_int(document, "focallength35mmequiv", m_exifInfo.FocalLength35mmEquiv);
  bson_append_int(document, "gpsinfopresent", m_exifInfo.GpsInfoPresent);
  bson_append_string(document, "gpsalt", m_exifInfo.GpsAlt);
  bson_append_string(document, "gpslat", m_exifInfo.GpsLat);
  bson_append_string(document, "gpslong", m_exifInfo.GpsLong);
  bson_append_int(document, "height", m_exifInfo.Height);
  bson_append_int(document, "iscolor", m_exifInfo.IsColor);
  bson_append_int(document, "isoequivalent", m_exifInfo.ISOequivalent);
  bson_append_long(document, "largestexifoffset", (long)m_exifInfo.LargestExifOffset);
  bson_append_int(document, "lightsource", m_exifInfo.LightSource);
  bson_append_int(document, "meteringmode", m_exifInfo.MeteringMode);
  bson_append_int(document, "numdatetimetags", m_exifInfo.numDateTimeTags);
  bson_append_int(document, "orientation", m_exifInfo.Orientation);
  bson_append_int(document, "process", m_exifInfo.Process);
  bson_append_int(document, "thumbnailatend", m_exifInfo.ThumbnailAtEnd);
  bson_append_long(document, "thumbnailoffset", (long)m_exifInfo.ThumbnailOffset);
  bson_append_long(document, "thumbnailsize", (long)m_exifInfo.ThumbnailSize);
  bson_append_int(document, "thumbnailsizeoffset", m_exifInfo.ThumbnailSizeOffset);
  bson_append_int(document, "whitebalance", m_exifInfo.Whitebalance);
  bson_append_int(document, "width", m_exifInfo.Width);

  bson_append_string(document, "author", m_iptcInfo.Author);
  bson_append_string(document, "byline", m_iptcInfo.Byline);
  bson_append_string(document, "bylinetitle", m_iptcInfo.BylineTitle);
  bson_append_string(document, "caption", m_iptcInfo.Caption);
  bson_append_string(document, "category", m_iptcInfo.Category);
  bson_append_string(document, "city", m_iptcInfo.City);
  bson_append_string(document, "copyright", m_iptcInfo.Copyright);
  bson_append_string(document, "copyrightnotice", m_iptcInfo.CopyrightNotice);
  bson_append_string(document, "country", m_iptcInfo.Country);
  bson_append_string(document, "countrycode", m_iptcInfo.CountryCode);
  bson_append_string(document, "credit", m_iptcInfo.Credit);
  bson_append_string(document, "date", m_iptcInfo.Date);
  bson_append_string(document, "headline", m_iptcInfo.Headline);
  bson_append_string(document, "keywords", m_iptcInfo.Keywords);
  bson_append_string(document, "objectname", m_iptcInfo.ObjectName);
  bson_append_string(document, "referenceservice", m_iptcInfo.ReferenceService);
  bson_append_string(document, "source", m_iptcInfo.Source);
  bson_append_string(document, "specialinstructions", m_iptcInfo.SpecialInstructions);
  bson_append_string(document, "state", m_iptcInfo.State);
  bson_append_string(document, "supplementalcategories", m_iptcInfo.SupplementalCategories);
  bson_append_string(document, "transmissionreference", m_iptcInfo.TransmissionReference);

  bson_append_bool(document, "isloaded", m_isLoaded);
  bson_append_string(document, "path", m_path.c_str());
  bson_append_string(document, "file", m_file.c_str());
  bson_append_int(document, "databaseid", m_databaseID);
  bson_append_long(document, "size", m_size);
  bson_append_string(document, "folder", m_folder.c_str());
  bson_append_int(document, "year", m_year);
  bson_append_string(document, "camera", m_camera.c_str());
  bson_append_start_array(document, "tag");
  for (unsigned int i = 0; i < m_tags.size(); i++)
  {
    arrayKey.Format("%d", i);
    bson_append_string(document, arrayKey.c_str(), m_tags[i].c_str());
  }
  bson_append_finish_array(document);

  bson_finish(document);

  // Object requesting serialization is responsible for calling bson_destroy(document)
}

void CPictureInfoTag::Deserialize(const bson *document, int dbId)
{
  bson_iterator it[1], subit[1];
  bson_type     type,  subtype;
  CStdString    key,   subkey;

  // BSON lookups are O(N), deserializing by lookup would be O(N^2), so use an iterator
  bson_iterator_init(it, document);
  type = bson_iterator_next(it);

  while (type != BSON_EOO)
  {
    key = bson_iterator_key(it);
    switch (type)
    {
    case BSON_STRING:
      if (key == "cameramake") strncpy(m_exifInfo.CameraMake, bson_iterator_string(it), sizeof(m_exifInfo.CameraMake));
      else if (key == "cameramodel") strncpy(m_exifInfo.CameraModel, bson_iterator_string(it), sizeof(m_exifInfo.CameraModel));
      else if (key == "comments") strncpy(m_exifInfo.Comments, bson_iterator_string(it), sizeof(m_exifInfo.Comments));
      else if (key == "description") strncpy(m_exifInfo.Description, bson_iterator_string(it), sizeof(m_exifInfo.Description));
      else if (key == "datetime") strncpy(m_exifInfo.DateTime, bson_iterator_string(it), sizeof(m_exifInfo.DateTime));
      else if (key == "gpsalt") strncpy(m_exifInfo.GpsAlt, bson_iterator_string(it), sizeof(m_exifInfo.GpsAlt));
      else if (key == "gpslat") strncpy(m_exifInfo.GpsLat, bson_iterator_string(it), sizeof(m_exifInfo.GpsLat));
      else if (key == "gpslong") strncpy(m_exifInfo.GpsLong, bson_iterator_string(it), sizeof(m_exifInfo.GpsLong));
      else if (key == "author") strncpy(m_iptcInfo.Author, bson_iterator_string(it), sizeof(m_iptcInfo.Author));
      else if (key == "byline") strncpy(m_iptcInfo.Byline, bson_iterator_string(it), sizeof(m_iptcInfo.Byline));
      else if (key == "bylinetitle") strncpy(m_iptcInfo.BylineTitle, bson_iterator_string(it), sizeof(m_iptcInfo.BylineTitle));
      else if (key == "caption") strncpy(m_iptcInfo.Caption, bson_iterator_string(it), sizeof(m_iptcInfo.Caption));
      else if (key == "category") strncpy(m_iptcInfo.Category, bson_iterator_string(it), sizeof(m_iptcInfo.Category));
      else if (key == "city") strncpy(m_iptcInfo.City, bson_iterator_string(it), sizeof(m_iptcInfo.City));
      else if (key == "copyright") strncpy(m_iptcInfo.Copyright, bson_iterator_string(it), sizeof(m_iptcInfo.Copyright));
      else if (key == "copyrightnotice") strncpy(m_iptcInfo.CopyrightNotice, bson_iterator_string(it), sizeof(m_iptcInfo.CopyrightNotice));
      else if (key == "country") strncpy(m_iptcInfo.Country, bson_iterator_string(it), sizeof(m_iptcInfo.Country));
      else if (key == "countrycode") strncpy(m_iptcInfo.CountryCode, bson_iterator_string(it), sizeof(m_iptcInfo.CountryCode));
      else if (key == "credit") strncpy(m_iptcInfo.Credit, bson_iterator_string(it), sizeof(m_iptcInfo.Credit));
      else if (key == "date") strncpy(m_iptcInfo.Date, bson_iterator_string(it), sizeof(m_iptcInfo.Date));
      else if (key == "headline") strncpy(m_iptcInfo.Headline, bson_iterator_string(it), sizeof(m_iptcInfo.Headline));
      else if (key == "keywords") strncpy(m_iptcInfo.Keywords, bson_iterator_string(it), sizeof(m_iptcInfo.Keywords));
      else if (key == "objectname") strncpy(m_iptcInfo.ObjectName, bson_iterator_string(it), sizeof(m_iptcInfo.ObjectName));
      else if (key == "referenceservice") strncpy(m_iptcInfo.ReferenceService, bson_iterator_string(it), sizeof(m_iptcInfo.ReferenceService));
      else if (key == "source") strncpy(m_iptcInfo.Source, bson_iterator_string(it), sizeof(m_iptcInfo.Source));
      else if (key == "specialinstructions") strncpy(m_iptcInfo.SpecialInstructions, bson_iterator_string(it), sizeof(m_iptcInfo.SpecialInstructions));
      else if (key == "state") strncpy(m_iptcInfo.State, bson_iterator_string(it), sizeof(m_iptcInfo.State));
      else if (key == "supplementalcategories") strncpy(m_iptcInfo.SupplementalCategories, bson_iterator_string(it), sizeof(m_iptcInfo.SupplementalCategories));
      else if (key == "transmissionreference") strncpy(m_iptcInfo.TransmissionReference, bson_iterator_string(it), sizeof(m_iptcInfo.TransmissionReference));
      else if (key == "path") m_path = bson_iterator_string(it);
      else if (key == "file") m_file = bson_iterator_string(it);
      else if (key == "folder") m_folder = bson_iterator_string(it);
      else if (key == "camera") m_camera = bson_iterator_string(it);
      break;
    case BSON_INT:
      if (key == "exposuremode") m_exifInfo.ExposureMode = bson_iterator_int(it);
      else if (key == "exposureprogram") m_exifInfo.ExposureProgram = bson_iterator_int(it);
      else if (key == "flashused") m_exifInfo.FlashUsed = bson_iterator_int(it);
      else if (key == "focallength35mmequiv") m_exifInfo.FocalLength35mmEquiv = bson_iterator_int(it);
      else if (key == "gpsinfopresent") m_exifInfo.GpsInfoPresent = bson_iterator_int(it);
      else if (key == "height") m_exifInfo.Height = bson_iterator_int(it);
      else if (key == "iscolor") m_exifInfo.IsColor = bson_iterator_int(it);
      else if (key == "isoequivalent") m_exifInfo.ISOequivalent = bson_iterator_int(it);
      else if (key == "lightsource") m_exifInfo.LightSource = bson_iterator_int(it);
      else if (key == "meteringmode") m_exifInfo.MeteringMode = bson_iterator_int(it);
      else if (key == "numdatetimetags") m_exifInfo.numDateTimeTags = bson_iterator_int(it);
      else if (key == "orientation") m_exifInfo.Orientation = bson_iterator_int(it);
      else if (key == "process") m_exifInfo.Process = bson_iterator_int(it);
      else if (key == "thumbnailatend") m_exifInfo.ThumbnailAtEnd = bson_iterator_int(it);
      else if (key == "thumbnailsizeoffset") m_exifInfo.ThumbnailSizeOffset = bson_iterator_int(it);
      else if (key == "whitebalance") m_exifInfo.Whitebalance = bson_iterator_int(it);
      else if (key == "width") m_exifInfo.Width = bson_iterator_int(it);
      else if (key == "year") m_year = bson_iterator_int(it);
      break;
    case BSON_LONG:
      if (key == "largestexifoffset") m_exifInfo.LargestExifOffset = (unsigned int)bson_iterator_long(it);
      else if (key == "thumbnailoffset") m_exifInfo.ThumbnailOffset = (unsigned int)bson_iterator_long(it);
      else if (key == "thumbnailsize") m_exifInfo.ThumbnailSize = (unsigned int)bson_iterator_long(it);
      else if (key == "size") m_size = (unsigned int)bson_iterator_long(it);
      break;
    case BSON_DOUBLE:
      if (key == "aperturefnumber") m_exifInfo.ApertureFNumber = (float)bson_iterator_double(it);
      else if (key == "ccdwidth") m_exifInfo.CCDWidth = (float)bson_iterator_long(it);
      else if (key == "digitalzoomratio") m_exifInfo.DigitalZoomRatio = (float)bson_iterator_long(it);
      else if (key == "distance") m_exifInfo.Distance = (float)bson_iterator_long(it);
      else if (key == "exposurebias") m_exifInfo.ExposureBias = (float)bson_iterator_long(it);
      else if (key == "exposuretime") m_exifInfo.ExposureTime = (float)bson_iterator_long(it);
      else if (key == "focallength") m_exifInfo.FocalLength = (float)bson_iterator_long(it);
      break;
    case BSON_BOOL:
      if (key == "isloaded") m_isLoaded = (bson_iterator_bool(it) != 0);
      break;
    case BSON_ARRAY:
    case BSON_OBJECT:
      bson_iterator_subiterator(it, subit);
      subtype = bson_iterator_next(subit);
      while (subtype != BSON_EOO)
      {
        subkey = bson_iterator_key(subit);
        if (subtype == BSON_INT && key == "datetimeoffsets")
        {
          // Array keys look like "0", "1", ...
          int i = atoi(subkey);
          if (0 < i && i < 10)
            m_exifInfo.DateTimeOffsets[i] = bson_iterator_int(subit);
        }
        else if (subtype == BSON_STRING && key == "tag")
        {
          m_tags.push_back(bson_iterator_string(subit));
        }
        subtype = bson_iterator_next(subit);
      }
      break;
    default:
      break;
    }
    type = bson_iterator_next(it);
  }
  m_databaseID = dbId;
}

void CPictureInfoTag::ToSortable(SortItem& sortable)
{
  
}

void CPictureInfoTag::GetStringFromArchive(CArchive &ar, char *string, size_t length)
{
  CStdString temp;
  ar >> temp;
  length = min((size_t)temp.GetLength(), length - 1);
  if (!temp.IsEmpty())
    memcpy(string, temp.c_str(), length);
  string[length] = 0;
}

const CStdString CPictureInfoTag::GetInfo(int info) const
{
  if (!m_isLoaded)
    return "";

  CStdString value;
  switch (info)
  {
  case SLIDE_RESOLUTION:
    value.Format("%d x %d", m_exifInfo.Width, m_exifInfo.Height);
    break;
  case SLIDE_COLOUR:
    value = m_exifInfo.IsColor ? "Colour" : "Black and White";
    break;
  case SLIDE_PROCESS:
    switch (m_exifInfo.Process)
    {
      case M_SOF0:
        // don't show it if its the plain old boring 'baseline' process, but do
        // show it if its something else, like 'progressive' (used on web sometimes)
        value = "Baseline";
      break;
      case M_SOF1:    value = "Extended sequential";      break;
      case M_SOF2:    value = "Progressive";      break;
      case M_SOF3:    value = "Lossless";      break;
      case M_SOF5:    value = "Differential sequential";      break;
      case M_SOF6:    value = "Differential progressive";      break;
      case M_SOF7:    value = "Differential lossless";      break;
      case M_SOF9:    value = "Extended sequential, arithmetic coding";      break;
      case M_SOF10:   value = "Progressive, arithmetic coding";     break;
      case M_SOF11:   value = "Lossless, arithmetic coding";     break;
      case M_SOF13:   value = "Differential sequential, arithmetic coding";     break;
      case M_SOF14:   value = "Differential progressive, arithmetic coding";     break;
      case M_SOF15:   value = "Differential lossless, arithmetic coding";     break;
      default:        value = "Unknown";   break;
    }
    break;
  case SLIDE_COMMENT:
  case SLIDE_EXIF_COMMENT:
    // The charset used for the UserComment is stored in CommentsCharset:
    // Ascii, Unicode (UCS2), JIS (X208-1990), Unknown (application specific)
    if (m_exifInfo.CommentsCharset == EXIF_COMMENT_CHARSET_UNICODE)
    {
      g_charsetConverter.ucs2ToUTF8(CStdString16((uint16_t*)m_exifInfo.Comments), value);
    }
    else
    {
      // Ascii doesn't need to be converted (EXIF_COMMENT_CHARSET_ASCII)
      // Archived data is already converted (EXIF_COMMENT_CHARSET_CONVERTED)
      // Unknown data can't be converted as it could be any codec (EXIF_COMMENT_CHARSET_UNKNOWN)
      // JIS data can't be converted as CharsetConverter and iconv lacks support (EXIF_COMMENT_CHARSET_JIS)
      value = m_exifInfo.Comments;
    }
    break;
  case SLIDE_EXIF_DATE_TIME:
  case SLIDE_EXIF_DATE:
    if (strlen(m_exifInfo.DateTime) >= 19 && m_exifInfo.DateTime[0] != ' ')
    {
      CStdString dateTime = m_exifInfo.DateTime;
      int year  = atoi(dateTime.Mid(0, 4).c_str());
      int month = atoi(dateTime.Mid(5, 2).c_str());
      int day   = atoi(dateTime.Mid(8, 2).c_str());
      int hour  = atoi(dateTime.Mid(11,2).c_str());
      int min   = atoi(dateTime.Mid(14,2).c_str());
      int sec   = atoi(dateTime.Mid(17,2).c_str());
      CDateTime date(year, month, day, hour, min, sec);
      if(SLIDE_EXIF_DATE_TIME == info)
          value = date.GetAsLocalizedDateTime();
      else
          value = date.GetAsLocalizedDate();
    }
    break;
  case SLIDE_EXIF_DESCRIPTION:
    value = m_exifInfo.Description;
    break;
  case SLIDE_EXIF_CAMERA_MAKE:
    value = m_exifInfo.CameraMake;
    break;
  case SLIDE_EXIF_CAMERA_MODEL:
    value = m_exifInfo.CameraModel;
    break;
//  case SLIDE_EXIF_SOFTWARE:
//    value = m_exifInfo.Software;
  case SLIDE_EXIF_APERTURE:
    if (m_exifInfo.ApertureFNumber)
      value.Format("%3.1f", m_exifInfo.ApertureFNumber);
    break;
  case SLIDE_EXIF_ORIENTATION:
    switch (m_exifInfo.Orientation)
    {
      case 1:   value = "Top Left";     break;
      case 2:   value = "Top Right";    break;
      case 3:   value = "Bottom Right"; break;
      case 4:   value = "Bottom Left";  break;
      case 5:   value = "Left Top";     break;
      case 6:   value = "Right Top";    break;
      case 7:   value = "Right Bottom"; break;
      case 8:   value = "Left Bottom";  break;
    }
    break;
  case SLIDE_EXIF_FOCAL_LENGTH:
    if (m_exifInfo.FocalLength)
    {
      value.Format("%4.2fmm", m_exifInfo.FocalLength);
      if (m_exifInfo.FocalLength35mmEquiv != 0)
        value.AppendFormat("  (35mm Equivalent = %umm)", m_exifInfo.FocalLength35mmEquiv);
    }
    break;
  case SLIDE_EXIF_FOCUS_DIST:
    if (m_exifInfo.Distance < 0)
      value = "Infinite";
    else if (m_exifInfo.Distance > 0)
      value.Format("%4.2fm", m_exifInfo.Distance);
    break;
  case SLIDE_EXIF_EXPOSURE:
    switch (m_exifInfo.ExposureProgram)
    {
      case 1:  value = "Manual";              break;
      case 2:  value = "Program (Auto)";     break;
      case 3:  value = "Aperture priority (Semi-Auto)";    break;
      case 4:  value = "Shutter priority (semi-auto)";     break;
      case 5:  value = "Creative Program (based towards depth of field)";    break;
      case 6:  value = "Action program (based towards fast shutter speed)";      break;
      case 7:  value = "Portrait Mode";    break;
      case 8:  value = "Landscape Mode";   break;
    }
    break;
  case SLIDE_EXIF_EXPOSURE_TIME:
    if (m_exifInfo.ExposureTime)
    {
      if (m_exifInfo.ExposureTime < 0.010f)
        value.Format("%6.4fs", m_exifInfo.ExposureTime);
      else
        value.Format("%5.3fs", m_exifInfo.ExposureTime);
      if (m_exifInfo.ExposureTime <= 0.5)
        value.AppendFormat(" (1/%d)", (int)(0.5 + 1/m_exifInfo.ExposureTime));
    }
    break;
  case SLIDE_EXIF_EXPOSURE_BIAS:
    if (m_exifInfo.ExposureBias != 0)
      value.Format("%4.2", m_exifInfo.ExposureBias);
    break;
  case SLIDE_EXIF_EXPOSURE_MODE:
    switch (m_exifInfo.ExposureMode)
    {
      case 0:  value = "Automatic";          break;
      case 1:  value = "Manual";             break;
      case 2:  value = "Auto bracketing";    break;
    }
    break;
  case SLIDE_EXIF_FLASH_USED:
    if (m_exifInfo.FlashUsed >= 0)
    {
      if (m_exifInfo.FlashUsed & 1)
      {
        value = "Yes";
        switch (m_exifInfo.FlashUsed)
        {
          case 0x5:  value = "Yes (Strobe light not detected)";                break;
          case 0x7:  value = "Yes (Strobe light detected)";                  break;
          case 0x9:  value = "Yes (Manual)";                  break;
          case 0xd:  value = "Yes (Manual, return light not detected)";          break;
          case 0xf:  value = "Yes (Manual, return light detected)";            break;
          case 0x19: value = "Yes (Auto)";                    break;
          case 0x1d: value = "Yes (Auto, return light not detected)";            break;
          case 0x1f: value = "Yes (Auto, return light detected)";              break;
          case 0x41: value = "Yes (Red eye reduction mode)";                  break;
          case 0x45: value = "Yes (Red eye reduction mode return light not detected)";          break;
          case 0x47: value = "Yes (Red eye reduction mode return light detected)";            break;
          case 0x49: value = "Yes (Manual, red eye reduction mode)";            break;
          case 0x4d: value = "Yes (Manual, red eye reduction mode, return light not detected)";    break;
          case 0x4f: value = "Yes (Manual, red eye reduction mode, return light detected)";      break;
          case 0x59: value = "Yes (Auto, red eye reduction mode)";              break;
          case 0x5d: value = "Yes (Auto, red eye reduction mode, return light not detected)";      break;
          case 0x5f: value = "Yes (Auto, red eye reduction mode, return light detected)";        break;
        }
      }
      else
        value = m_exifInfo.FlashUsed == 0x18 ? "No (Auto)" : "No";
    }
    break;
  case SLIDE_EXIF_WHITE_BALANCE:
    return m_exifInfo.Whitebalance ? "Manual" : "Auto";
  case SLIDE_EXIF_LIGHT_SOURCE:
    switch (m_exifInfo.LightSource)
    {
      case 1:   value = "Daylight";       break;
      case 2:   value = "Fluorescent";    break;
      case 3:   value = "Incandescent";   break;
      case 4:   value = "Flash";          break;
      case 9:   value = "Fine Weather";    break;
      case 11:  value = "Shade";          break;
      default:;   //Quercus: 17-1-2004 There are many more modes for this, check Exif2.2 specs
                  // If it just says 'unknown' or we don't know it, then
                  // don't bother showing it - it doesn't add any useful information.
    }
    break;
  case SLIDE_EXIF_METERING_MODE:
    switch (m_exifInfo.MeteringMode)
    {
      case 2:  value = "Center weight"; break;
      case 3:  value = "Spot";   break;
      case 5:  value = "Matrix"; break;
    }
    break;
  case SLIDE_EXIF_ISO_EQUIV:
    if (m_exifInfo.ISOequivalent)
      value.Format("%2d", m_exifInfo.ISOequivalent);
    break;
  case SLIDE_EXIF_DIGITAL_ZOOM:
    if (m_exifInfo.DigitalZoomRatio)
      value.Format("%1.3fx", m_exifInfo.DigitalZoomRatio);
    break;
  case SLIDE_EXIF_CCD_WIDTH:
    if (m_exifInfo.CCDWidth)
      value.Format("%4.2fmm", m_exifInfo.CCDWidth);
    break;
  case SLIDE_EXIF_GPS_LATITUDE:
    value = m_exifInfo.GpsLat;
    break;
  case SLIDE_EXIF_GPS_LONGITUDE:
    value = m_exifInfo.GpsLong;
    break;
  case SLIDE_EXIF_GPS_ALTITUDE:
    value = m_exifInfo.GpsAlt;
    break;
  case SLIDE_IPTC_SUP_CATEGORIES:   value = m_iptcInfo.SupplementalCategories;  break;
  case SLIDE_IPTC_KEYWORDS:         value = m_iptcInfo.Keywords;                break;
  case SLIDE_IPTC_CAPTION:          value = m_iptcInfo.Caption;                 break;
  case SLIDE_IPTC_AUTHOR:           value = m_iptcInfo.Author;                  break;
  case SLIDE_IPTC_HEADLINE:         value = m_iptcInfo.Headline;                break;
  case SLIDE_IPTC_SPEC_INSTR:       value = m_iptcInfo.SpecialInstructions;     break;
  case SLIDE_IPTC_CATEGORY:         value = m_iptcInfo.Category;                break;
  case SLIDE_IPTC_BYLINE:           value = m_iptcInfo.Byline;                  break;
  case SLIDE_IPTC_BYLINE_TITLE:     value = m_iptcInfo.BylineTitle;             break;
  case SLIDE_IPTC_CREDIT:           value = m_iptcInfo.Credit;                  break;
  case SLIDE_IPTC_SOURCE:           value = m_iptcInfo.Source;                  break;
  case SLIDE_IPTC_COPYRIGHT_NOTICE: value = m_iptcInfo.CopyrightNotice;         break;
  case SLIDE_IPTC_OBJECT_NAME:      value = m_iptcInfo.ObjectName;              break;
  case SLIDE_IPTC_CITY:             value = m_iptcInfo.City;                    break;
  case SLIDE_IPTC_STATE:            value = m_iptcInfo.State;                   break;
  case SLIDE_IPTC_COUNTRY:          value = m_iptcInfo.Country;                 break;
  case SLIDE_IPTC_TX_REFERENCE:     value = m_iptcInfo.TransmissionReference;   break;
  case SLIDE_IPTC_DATE:             value = m_iptcInfo.Date;                    break;
  case SLIDE_IPTC_COPYRIGHT:        value = m_iptcInfo.Copyright;               break;
  case SLIDE_IPTC_COUNTRY_CODE:     value = m_iptcInfo.CountryCode;             break;
  case SLIDE_IPTC_REF_SERVICE:      value = m_iptcInfo.ReferenceService;        break;
  default:
    break;
  }
  return value;
}

int CPictureInfoTag::TranslateString(const CStdString &info)
{
  if (info.Equals("filename")) return SLIDE_FILE_NAME;
  else if (info.Equals("path")) return SLIDE_FILE_PATH;
  else if (info.Equals("filesize")) return SLIDE_FILE_SIZE;
  else if (info.Equals("filedate")) return SLIDE_FILE_DATE;
  else if (info.Equals("slideindex")) return SLIDE_INDEX;
  else if (info.Equals("resolution")) return SLIDE_RESOLUTION;
  else if (info.Equals("slidecomment")) return SLIDE_COMMENT;
  else if (info.Equals("colour")) return SLIDE_COLOUR;
  else if (info.Equals("process")) return SLIDE_PROCESS;
  else if (info.Equals("exiftime")) return SLIDE_EXIF_DATE_TIME;
  else if (info.Equals("exifdate")) return SLIDE_EXIF_DATE;
  else if (info.Equals("exifdescription")) return SLIDE_EXIF_DESCRIPTION;
  else if (info.Equals("cameramake")) return SLIDE_EXIF_CAMERA_MAKE;
  else if (info.Equals("cameramodel")) return SLIDE_EXIF_CAMERA_MODEL;
  else if (info.Equals("exifcomment")) return SLIDE_EXIF_COMMENT;
  else if (info.Equals("exifsoftware")) return SLIDE_EXIF_SOFTWARE;
  else if (info.Equals("apreture")) return SLIDE_EXIF_APERTURE;
  else if (info.Equals("focallength")) return SLIDE_EXIF_FOCAL_LENGTH;
  else if (info.Equals("focusdistance")) return SLIDE_EXIF_FOCUS_DIST;
  else if (info.Equals("exposure")) return SLIDE_EXIF_EXPOSURE;
  else if (info.Equals("exposuretime")) return SLIDE_EXIF_EXPOSURE_TIME;
  else if (info.Equals("exposurebias")) return SLIDE_EXIF_EXPOSURE_BIAS;
  else if (info.Equals("exposuremode")) return SLIDE_EXIF_EXPOSURE_MODE;
  else if (info.Equals("flashused")) return SLIDE_EXIF_FLASH_USED;
  else if (info.Equals("whitebalance")) return SLIDE_EXIF_WHITE_BALANCE;
  else if (info.Equals("lightsource")) return SLIDE_EXIF_LIGHT_SOURCE;
  else if (info.Equals("meteringmode")) return SLIDE_EXIF_METERING_MODE;
  else if (info.Equals("isoequivalence")) return SLIDE_EXIF_ISO_EQUIV;
  else if (info.Equals("digitalzoom")) return SLIDE_EXIF_DIGITAL_ZOOM;
  else if (info.Equals("ccdwidth")) return SLIDE_EXIF_CCD_WIDTH;
  else if (info.Equals("orientation")) return SLIDE_EXIF_ORIENTATION;
  else if (info.Equals("supplementalcategories")) return SLIDE_IPTC_SUP_CATEGORIES;
  else if (info.Equals("keywords")) return SLIDE_IPTC_KEYWORDS;
  else if (info.Equals("caption")) return SLIDE_IPTC_CAPTION;
  else if (info.Equals("author")) return SLIDE_IPTC_AUTHOR;
  else if (info.Equals("healine")) return SLIDE_IPTC_HEADLINE;
  else if (info.Equals("specialinstructions")) return SLIDE_IPTC_SPEC_INSTR;
  else if (info.Equals("category")) return SLIDE_IPTC_CATEGORY;
  else if (info.Equals("byline")) return SLIDE_IPTC_BYLINE;
  else if (info.Equals("bylinetitle")) return SLIDE_IPTC_BYLINE_TITLE;
  else if (info.Equals("credit")) return SLIDE_IPTC_CREDIT;
  else if (info.Equals("source")) return SLIDE_IPTC_SOURCE;
  else if (info.Equals("copyrightnotice")) return SLIDE_IPTC_COPYRIGHT_NOTICE;
  else if (info.Equals("objectname")) return SLIDE_IPTC_OBJECT_NAME;
  else if (info.Equals("city")) return SLIDE_IPTC_CITY;
  else if (info.Equals("state")) return SLIDE_IPTC_STATE;
  else if (info.Equals("country")) return SLIDE_IPTC_COUNTRY;
  else if (info.Equals("transmissionreference")) return SLIDE_IPTC_TX_REFERENCE;
  else if (info.Equals("iptcdate")) return SLIDE_IPTC_DATE;
  else if (info.Equals("copyright")) return SLIDE_IPTC_COPYRIGHT;
  else if (info.Equals("countrycode")) return SLIDE_IPTC_COUNTRY_CODE;
  else if (info.Equals("referenceservice")) return SLIDE_IPTC_REF_SERVICE;
  else if (info.Equals("latitude")) return SLIDE_EXIF_GPS_LATITUDE;
  else if (info.Equals("longitude")) return SLIDE_EXIF_GPS_LONGITUDE;
  else if (info.Equals("altitude")) return SLIDE_EXIF_GPS_ALTITUDE;
  return 0;
}

void CPictureInfoTag::SetInfo(int info, const CStdString& value)
{
  switch (info)
  {
  case SLIDE_RESOLUTION:
    {
      vector<CStdString> dimension;
      CUtil::Tokenize(value, dimension, ",");
      if (dimension.size() == 2)
      {
        m_exifInfo.Width = atoi(dimension[0].c_str());
        m_exifInfo.Height = atoi(dimension[1].c_str());
      }
      break;
    }
  case SLIDE_EXIF_DATE_TIME:
    {
      strcpy(m_exifInfo.DateTime, value.c_str());
      break;
    }
  default:
    break;
  }
}

void CPictureInfoTag::SetLoaded(bool loaded)
{
  m_isLoaded = loaded;
}

