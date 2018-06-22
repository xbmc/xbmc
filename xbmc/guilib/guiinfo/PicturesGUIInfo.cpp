/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include "guilib/guiinfo/PicturesGUIInfo.h"

#include <map>

#include "FileItem.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "pictures/GUIWindowSlideShow.h"
#include "pictures/PictureInfoTag.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include "guilib/guiinfo/GUIInfo.h"
#include "guilib/guiinfo/GUIInfoLabels.h"

using namespace KODI::GUILIB::GUIINFO;

static const std::map<int, int> listitem2slideshow_map =
{
  { LISTITEM_PICTURE_RESOLUTION       , SLIDESHOW_RESOLUTION },
  { LISTITEM_PICTURE_LONGDATE         , SLIDESHOW_EXIF_LONG_DATE },
  { LISTITEM_PICTURE_LONGDATETIME     , SLIDESHOW_EXIF_LONG_DATE_TIME },
  { LISTITEM_PICTURE_DATE             , SLIDESHOW_EXIF_DATE },
  { LISTITEM_PICTURE_DATETIME         , SLIDESHOW_EXIF_DATE_TIME },
  { LISTITEM_PICTURE_COMMENT          , SLIDESHOW_COMMENT },
  { LISTITEM_PICTURE_CAPTION          , SLIDESHOW_IPTC_CAPTION },
  { LISTITEM_PICTURE_DESC             , SLIDESHOW_EXIF_DESCRIPTION },
  { LISTITEM_PICTURE_KEYWORDS         , SLIDESHOW_IPTC_KEYWORDS },
  { LISTITEM_PICTURE_CAM_MAKE         , SLIDESHOW_EXIF_CAMERA_MAKE },
  { LISTITEM_PICTURE_CAM_MODEL        , SLIDESHOW_EXIF_CAMERA_MODEL },
  { LISTITEM_PICTURE_APERTURE         , SLIDESHOW_EXIF_APERTURE },
  { LISTITEM_PICTURE_FOCAL_LEN        , SLIDESHOW_EXIF_FOCAL_LENGTH },
  { LISTITEM_PICTURE_FOCUS_DIST       , SLIDESHOW_EXIF_FOCUS_DIST },
  { LISTITEM_PICTURE_EXP_MODE         , SLIDESHOW_EXIF_EXPOSURE_MODE },
  { LISTITEM_PICTURE_EXP_TIME         , SLIDESHOW_EXIF_EXPOSURE_TIME },
  { LISTITEM_PICTURE_ISO              , SLIDESHOW_EXIF_ISO_EQUIV },
  { LISTITEM_PICTURE_AUTHOR           , SLIDESHOW_IPTC_AUTHOR },
  { LISTITEM_PICTURE_BYLINE           , SLIDESHOW_IPTC_BYLINE },
  { LISTITEM_PICTURE_BYLINE_TITLE     , SLIDESHOW_IPTC_BYLINE_TITLE },
  { LISTITEM_PICTURE_CATEGORY         , SLIDESHOW_IPTC_CATEGORY },
  { LISTITEM_PICTURE_CCD_WIDTH        , SLIDESHOW_EXIF_CCD_WIDTH },
  { LISTITEM_PICTURE_CITY             , SLIDESHOW_IPTC_CITY },
  { LISTITEM_PICTURE_URGENCY          , SLIDESHOW_IPTC_URGENCY },
  { LISTITEM_PICTURE_COPYRIGHT_NOTICE , SLIDESHOW_IPTC_COPYRIGHT_NOTICE },
  { LISTITEM_PICTURE_COUNTRY          , SLIDESHOW_IPTC_COUNTRY },
  { LISTITEM_PICTURE_COUNTRY_CODE     , SLIDESHOW_IPTC_COUNTRY_CODE },
  { LISTITEM_PICTURE_CREDIT           , SLIDESHOW_IPTC_CREDIT },
  { LISTITEM_PICTURE_IPTCDATE         , SLIDESHOW_IPTC_DATE },
  { LISTITEM_PICTURE_DIGITAL_ZOOM     , SLIDESHOW_EXIF_DIGITAL_ZOOM, },
  { LISTITEM_PICTURE_EXPOSURE         , SLIDESHOW_EXIF_EXPOSURE },
  { LISTITEM_PICTURE_EXPOSURE_BIAS    , SLIDESHOW_EXIF_EXPOSURE_BIAS },
  { LISTITEM_PICTURE_FLASH_USED       , SLIDESHOW_EXIF_FLASH_USED },
  { LISTITEM_PICTURE_HEADLINE         , SLIDESHOW_IPTC_HEADLINE },
  { LISTITEM_PICTURE_COLOUR           , SLIDESHOW_COLOUR },
  { LISTITEM_PICTURE_LIGHT_SOURCE     , SLIDESHOW_EXIF_LIGHT_SOURCE },
  { LISTITEM_PICTURE_METERING_MODE    , SLIDESHOW_EXIF_METERING_MODE },
  { LISTITEM_PICTURE_OBJECT_NAME      , SLIDESHOW_IPTC_OBJECT_NAME },
  { LISTITEM_PICTURE_ORIENTATION      , SLIDESHOW_EXIF_ORIENTATION },
  { LISTITEM_PICTURE_PROCESS          , SLIDESHOW_PROCESS },
  { LISTITEM_PICTURE_REF_SERVICE      , SLIDESHOW_IPTC_REF_SERVICE },
  { LISTITEM_PICTURE_SOURCE           , SLIDESHOW_IPTC_SOURCE },
  { LISTITEM_PICTURE_SPEC_INSTR       , SLIDESHOW_IPTC_SPEC_INSTR },
  { LISTITEM_PICTURE_STATE            , SLIDESHOW_IPTC_STATE },
  { LISTITEM_PICTURE_SUP_CATEGORIES   , SLIDESHOW_IPTC_SUP_CATEGORIES },
  { LISTITEM_PICTURE_TX_REFERENCE     , SLIDESHOW_IPTC_TX_REFERENCE },
  { LISTITEM_PICTURE_WHITE_BALANCE    , SLIDESHOW_EXIF_WHITE_BALANCE },
  { LISTITEM_PICTURE_IMAGETYPE        , SLIDESHOW_IPTC_IMAGETYPE },
  { LISTITEM_PICTURE_SUBLOCATION      , SLIDESHOW_IPTC_SUBLOCATION },
  { LISTITEM_PICTURE_TIMECREATED      , SLIDESHOW_IPTC_TIMECREATED },
  { LISTITEM_PICTURE_GPS_LAT          , SLIDESHOW_EXIF_GPS_LATITUDE },
  { LISTITEM_PICTURE_GPS_LON          , SLIDESHOW_EXIF_GPS_LONGITUDE },
  { LISTITEM_PICTURE_GPS_ALT          , SLIDESHOW_EXIF_GPS_ALTITUDE }
};

CPicturesGUIInfo::CPicturesGUIInfo()
{
}

CPicturesGUIInfo::~CPicturesGUIInfo()
{
}

void CPicturesGUIInfo::SetCurrentSlide(CFileItem *item)
{
  if (item)
  {
    CPictureInfoTag* tag = item->GetPictureInfoTag();  // creates item if not yet set, so no nullptr checks needed
    if (!tag->Loaded()) // If picture metadata has not been loaded yet, load it now
      tag->Load(item->GetPath());

    m_currentSlide.reset(new CFileItem(*item));
  }
  else
  {
    m_currentSlide.reset();
  }
}

const CFileItem* CPicturesGUIInfo::GetCurrentSlide() const
{
  return m_currentSlide.get();
}

bool CPicturesGUIInfo::InitCurrentItem(CFileItem *item)
{
  return false;
}

bool CPicturesGUIInfo::GetLabel(std::string& value, const CFileItem *item, int contextWindow, const CGUIInfo &info, std::string *fallback) const
{
  ///////////////////////////////////////////////////////////////////////////////////////////////
  // LISTITEM_* / SLIDESHOW_* (values directly supported by PictureInfoTag)
  ///////////////////////////////////////////////////////////////////////////////////////////////
  if (item->IsPicture() && info.m_info >= LISTITEM_PICTURE_START && info.m_info <= LISTITEM_PICTURE_END)
  {
    const auto& it = listitem2slideshow_map.find(info.m_info);
    if (it != listitem2slideshow_map.end())
    {
      if (item->HasPictureInfoTag())
      {
        value = item->GetPictureInfoTag()->GetInfo(it->second);
        return true;
      }
    }
    else
    {
      CLog::Log(LOGERROR, "CPicturesGUIInfo::GetLabel - cannot map LISTITEM (%d) to SLIDESHOW label!", info.m_info);
      return false;
    }
  }
  else if (m_currentSlide && info.m_info >= SLIDESHOW_LABELS_START && info.m_info <= SLIDESHOW_LABELS_END)
  {
    value = m_currentSlide->GetPictureInfoTag()->GetInfo(info.m_info);
    return true;
  }

  if (item->IsPicture())
  {
    /////////////////////////////////////////////////////////////////////////////////////////////////
    // LISTITEM_*
    /////////////////////////////////////////////////////////////////////////////////////////////////
    switch (info.m_info)
    {
      case LISTITEM_PICTURE_PATH:
      {
        if (!item->IsZIP() || item->IsRAR() || item->IsCBZ() || item->IsCBR())
        {
          value = item->GetPath();
          return true;
        }
        break;
      }
    }
  }

  if (item->HasPictureInfoTag())
  {
    switch (info.m_info)
    {
      ///////////////////////////////////////////////////////////////////////////////////////////////
      // SLIDESHOW_* (values not directly supported by PictureInfoTag)
      ///////////////////////////////////////////////////////////////////////////////////////////////
      case SLIDESHOW_FILE_NAME:
      {
        value = URIUtils::GetFileName(item->GetPath());
        return true;
      }
      case SLIDESHOW_FILE_PATH:
      {
        const std::string path = URIUtils::GetDirectory(item->GetPath());
        value = CURL(path).GetWithoutUserDetails();
        return true;
      }
      case SLIDESHOW_FILE_SIZE:
      {
        if (!item->m_bIsFolder || item->m_dwSize)
        {
          value = StringUtils::SizeToString(item->m_dwSize);
          return true;
        }
        break;
      }
      case SLIDESHOW_FILE_DATE:
      {
        if (item->m_dateTime.IsValid())
        {
          value = item->m_dateTime.GetAsLocalizedDate();
          return true;
        }
        break;
      }
    }
  }

  switch (info.m_info)
  {
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // SLIDESHOW_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case SLIDESHOW_INDEX:
    {
      CGUIWindowSlideShow *slideshow = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIWindowSlideShow>(WINDOW_SLIDESHOW);
      if (slideshow && slideshow->NumSlides())
      {
        value = StringUtils::Format("%d/%d", slideshow->CurrentSlide(), slideshow->NumSlides());
        return true;
      }
      break;
    }
  }

  return false;
}

bool CPicturesGUIInfo::GetInt(int& value, const CGUIListItem *gitem, int contextWindow, const CGUIInfo &info) const
{
  return false;
}

bool CPicturesGUIInfo::GetBool(bool& value, const CGUIListItem *gitem, int contextWindow, const CGUIInfo &info) const
{
  switch (info.m_info)
  {
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // SLIDESHOW_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case SLIDESHOW_ISPAUSED:
    {
      CGUIWindowSlideShow *slideShow = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIWindowSlideShow>(WINDOW_SLIDESHOW);
      value = (slideShow && slideShow->IsPaused());
      return true;
    }
    case SLIDESHOW_ISRANDOM:
    {
      CGUIWindowSlideShow *slideShow = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIWindowSlideShow>(WINDOW_SLIDESHOW);
      value = (slideShow && slideShow->IsShuffled());
      return true;
    }
    case SLIDESHOW_ISACTIVE:
    {
      CGUIWindowSlideShow *slideShow = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIWindowSlideShow>(WINDOW_SLIDESHOW);
      value = (slideShow && slideShow->InSlideShow());
      return true;
    }
    case SLIDESHOW_ISVIDEO:
    {
      CGUIWindowSlideShow *slideShow = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIWindowSlideShow>(WINDOW_SLIDESHOW);
      value = (slideShow && slideShow->GetCurrentSlide() && slideShow->GetCurrentSlide()->IsVideo());
      return true;
    }
  }

  return false;
}
