/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "InfoTagPicture.h"

#include "AddonUtils.h"
#include "guilib/guiinfo/GUIInfoLabels.h"
#include "interfaces/legacy/Exception.h"
#include "pictures/PictureInfoTag.h"
#include "utils/StringUtils.h"

using namespace XBMCAddonUtils;

namespace XBMCAddon
{
namespace xbmc
{

InfoTagPicture::InfoTagPicture(bool offscreen /* = false */)
  : infoTag(new CPictureInfoTag), offscreen(offscreen), owned(true)
{
}

InfoTagPicture::InfoTagPicture(const CPictureInfoTag* tag)
  : infoTag(new CPictureInfoTag(*tag)), offscreen(true), owned(true)
{
}

InfoTagPicture::InfoTagPicture(CPictureInfoTag* tag, bool offscreen /* = false */)
  : infoTag(tag), offscreen(offscreen), owned(false)
{
}

InfoTagPicture::~InfoTagPicture()
{
  if (owned)
    delete infoTag;
}

String InfoTagPicture::getResolution()
{
  return infoTag->GetInfo(SLIDESHOW_RESOLUTION);
}

String InfoTagPicture::getDateTimeTaken()
{
  return infoTag->GetDateTimeTaken().GetAsW3CDateTime();
}

void InfoTagPicture::setResolution(int width, int height)
{
  XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
  setResolutionRaw(infoTag, width, height);
}

void InfoTagPicture::setDateTimeTaken(const String& datetimetaken)
{
  XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
  setDateTimeTakenRaw(infoTag, datetimetaken);
}

void InfoTagPicture::setResolutionRaw(CPictureInfoTag* infoTag, const String& resolution)
{
  infoTag->SetInfo("resolution", resolution);
}

void InfoTagPicture::setResolutionRaw(CPictureInfoTag* infoTag, int width, int height)
{
  if (width <= 0)
    throw WrongTypeException("InfoTagPicture.setResolution: width must be greater than zero (0)");
  if (height <= 0)
    throw WrongTypeException("InfoTagPicture.setResolution: height must be greater than zero (0)");

  setResolutionRaw(infoTag, StringUtils::Format("{:d},{:d}", width, height));
}

void InfoTagPicture::setDateTimeTakenRaw(CPictureInfoTag* infoTag, String datetimetaken)
{
  // try to parse the datetimetaken as from W3C format and adjust it to the EXIF datetime format YYYY:MM:DD HH:MM:SS
  CDateTime w3cDateTimeTaken;
  if (w3cDateTimeTaken.SetFromW3CDateTime(datetimetaken))
  {
    datetimetaken = StringUtils::Format("{:4d}:{:2d}:{:2d} {:2d}:{:2d}:{:2d}",
                                        w3cDateTimeTaken.GetYear(), w3cDateTimeTaken.GetMonth(),
                                        w3cDateTimeTaken.GetDay(), w3cDateTimeTaken.GetHour(),
                                        w3cDateTimeTaken.GetMinute(), w3cDateTimeTaken.GetSecond());
  }

  infoTag->SetInfo("exiftime", datetimetaken);
}

} // namespace xbmc
} // namespace XBMCAddon
