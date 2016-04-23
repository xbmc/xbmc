/*
 *      Copyright (C) 2015-2016 Team KODI
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Addon_InfoTagVideo.h"

#include "Application.h"
#include "FileItem.h"
#include "GUIInfoManager.h"
#include "addons/Addon.h"
#include "addons/binary/ExceptionHandling.h"
#include "addons/binary/interfaces/AddonInterfaces.h"
#include "addons/binary/interfaces/api2/AddonInterfaceBase.h"
#include "addons/kodi-addon-dev-kit/include/kodi/api2/.internal/AddonLib_internal.hpp"
#include "settings/AdvancedSettings.h"
#include "video/VideoInfoTag.h"

using namespace ADDON;

namespace V2
{
namespace KodiAPI
{

namespace Player
{
extern "C"
{

void CAddOnInfoTagVideo::Init(struct CB_AddOnLib *interfaces)
{
  interfaces->AddonInfoTagVideo.GetFromPlayer  = V2::KodiAPI::Player::CAddOnInfoTagVideo::GetFromPlayer;
  interfaces->AddonInfoTagVideo.Release        = V2::KodiAPI::Player::CAddOnInfoTagVideo::Release;
}

bool CAddOnInfoTagVideo::GetFromPlayer(
        void*               hdl,
        void*               player,
        AddonInfoTagVideo*  tag)
{
  try
  {
    if (!hdl)
      throw ADDON::WrongValueException("CAddOnInfoTagVideo - %s - invalid data (handle='%p')",
                                        __FUNCTION__, hdl);

    CAddonInterfaces* helper = static_cast<CAddonInterfaces*>(static_cast<AddonCB*>(hdl)->addonData);
    if (!helper || !player | !tag)
      throw ADDON::WrongValueException("CAddOnInfoTagVideo - %s - invalid data (addonData='%p', player='%p', tag='%p')",
                                        __FUNCTION__, helper, player, tag);

    memset(tag, 0, sizeof(AddonInfoTagVideo));

    if (!g_application.m_pPlayer->IsPlayingVideo())
      throw ADDON::WrongValueException("CAddOnInfoTagVideo - %s: %s/%s - Kodi is not playing any video file",
                                         __FUNCTION__,
                                         TranslateType(helper->GetAddon()->Type()).c_str(),
                                         helper->GetAddon()->Name().c_str());

    const CVideoInfoTag* infoTag = g_infoManager.GetCurrentMovieTag();
    if (!infoTag)
    {
      CLog::Log(LOGERROR, "CAddOnInfoTagVideo - %s: %s/%s - Failed to get video info",
                                         __FUNCTION__,
                                         TranslateType(helper->GetAddon()->Type()).c_str(),
                                         helper->GetAddon()->Name().c_str());
      return false;
    }

    tag->m_director = strdup(StringUtils::Join(infoTag->m_director, g_advancedSettings.m_videoItemSeparator).c_str());
    tag->m_writingCredits = strdup(StringUtils::Join(infoTag->m_writingCredits, g_advancedSettings.m_videoItemSeparator).c_str());
    tag->m_genre = strdup(StringUtils::Join(infoTag->m_genre, g_advancedSettings.m_videoItemSeparator).c_str());
    tag->m_country = strdup(StringUtils::Join(infoTag->m_country, g_advancedSettings.m_videoItemSeparator).c_str());
    tag->m_tagLine = strdup(infoTag->m_strTagLine.c_str());
    tag->m_plotOutline = strdup(infoTag->m_strPlotOutline.c_str());
    tag->m_plot = strdup(infoTag->m_strPlot.c_str());
    tag->m_trailer = strdup(infoTag->m_strTrailer.c_str());
    tag->m_pictureURL = strdup(infoTag->m_strPictureURL.GetFirstThumb().m_url.c_str());
    tag->m_title = strdup(infoTag->m_strTitle.c_str());
    tag->m_type = strdup(infoTag->m_type.c_str());
    tag->m_votes = strdup(infoTag->m_strVotes.c_str());
    tag->m_cast = strdup(infoTag->GetCast(true).c_str());
    tag->m_file = strdup(infoTag->m_strFile.c_str());
    tag->m_path = strdup(infoTag->m_strPath.c_str());
    tag->m_IMDBNumber = strdup(infoTag->m_strIMDBNumber.c_str());
    tag->m_MPAARating = strdup(infoTag->m_strMPAARating.c_str());
    tag->m_year = infoTag->m_iYear;
    tag->m_rating = infoTag->GetRating().rating;
    tag->m_playCount = infoTag->m_playCount;
    tag->m_lastPlayed = strdup(infoTag->m_lastPlayed.GetAsLocalizedDateTime().c_str());
    tag->m_originalTitle = strdup(infoTag->m_strOriginalTitle.c_str());
    tag->m_premiered = strdup(infoTag->m_premiered.GetAsLocalizedDate().c_str());
    tag->m_firstAired = strdup(infoTag->m_firstAired.GetAsLocalizedDate().c_str());
    tag->m_showTitle = strdup(infoTag->m_strShowTitle.c_str());
    tag->m_season = infoTag->m_iSeason;
    tag->m_episode = infoTag->m_iEpisode;
    tag->m_dbId = infoTag->m_iDbId;
    tag->m_duration = (unsigned int)infoTag->m_duration;

    return true;
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

void CAddOnInfoTagVideo::Release(
        void*               hdl,
        AddonInfoTagVideo*  tag)
{
  try
  {
    if (!hdl)
      throw ADDON::WrongValueException("CAddOnInfoTagVideo - %s - invalid data (handle='%p')",
                                        __FUNCTION__, hdl);

    CAddonInterfaces* helper = static_cast<CAddonInterfaces*>(static_cast<AddonCB*>(hdl)->addonData);
    if (!helper || !tag)
      throw ADDON::WrongValueException("CAddOnInfoTagVideo - %s - invalid data (addonData='%p', tag='%p')",
                                        __FUNCTION__, helper, tag);
    if (tag->m_director)
      free(tag->m_director);
    if (tag->m_writingCredits)
      free(tag->m_writingCredits);
    if (tag->m_genre)
      free(tag->m_genre);
    if (tag->m_country)
      free(tag->m_country);
    if (tag->m_tagLine)
      free(tag->m_tagLine);
    if (tag->m_plotOutline)
      free(tag->m_plotOutline);
    if (tag->m_plot)
      free(tag->m_plot);
    if (tag->m_trailer)
      free(tag->m_trailer);
    if (tag->m_pictureURL)
      free(tag->m_pictureURL);
    if (tag->m_title)
      free(tag->m_title);
    if (tag->m_type)
      free(tag->m_type);
    if (tag->m_votes)
      free(tag->m_votes);
    if (tag->m_cast)
      free(tag->m_cast);
    if (tag->m_file)
      free(tag->m_file);
    if (tag->m_path)
      free(tag->m_path);
    if (tag->m_IMDBNumber)
      free(tag->m_IMDBNumber);
    if (tag->m_MPAARating)
      free(tag->m_MPAARating);
    if (tag->m_lastPlayed)
      free(tag->m_lastPlayed);
    if (tag->m_originalTitle)
      free(tag->m_originalTitle);
    if (tag->m_premiered)
      free(tag->m_premiered);
    if (tag->m_firstAired)
      free(tag->m_firstAired);
    if (tag->m_showTitle)
      free(tag->m_showTitle);

    memset(tag, 0, sizeof(AddonInfoTagVideo));
  }
  HANDLE_ADDON_EXCEPTION
}

} /* extern "C" */
} /* namespace Player */

} /* namespace KodiAPI */
} /* namespace V2 */
