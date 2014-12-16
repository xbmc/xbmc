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
 */

#include "video/VideoInfoTag.h"
#include "AddonClass.h"

#pragma once

namespace XBMCAddon
{
  namespace xbmc
  {
    class InfoTagVideo : public AddonClass
    {
    private:
      CVideoInfoTag* infoTag;

    public:
#ifndef SWIG
      InfoTagVideo(const CVideoInfoTag& tag);
#endif
      InfoTagVideo();
      virtual ~InfoTagVideo();

      /**
       * getDirector() - returns a string.
       */
      String getDirector();
      /**
       * getWritingCredits() - returns a string.
       */
      String getWritingCredits();
      /**
       * getGenre() - returns a string.
       */
      String getGenre();
      /**
       * getTagLine() - returns a string.
       */
      String getTagLine();
      /**
       * getPlotOutline() - returns a string.
       */
      String getPlotOutline();
      /**
       * getPlot() - returns a string.
       */
      String getPlot();
      /**
       * getPictureURL() - returns a string.
       */
      String getPictureURL();
      /**
       * getTitle() - returns a string.
       */
      String getTitle();
      /**
       * getVotes() - returns a string.
       */
      String getVotes();
      /**
       * getCast() - returns a string.
       */
      String getCast();
      /**
       * getFile() - returns a string.
       */
      String getFile();
      /**
       * getPath() - returns a string.
       */
      String getPath();
      /**
       * getIMDBNumber() - returns a string.
       */
      String getIMDBNumber();
      /**
       * getYear() - returns an integer.
       */
      int getYear();
      /**
       * getRating() - returns a float (double where supported)
       */
      double getRating();

      /**
       * getPlayCount() -- returns a integer.\n
       */
      int getPlayCount();

      /**
       * getLastPlayed() -- returns a string.\n
       */
      String getLastPlayed();
      /**
       * getOriginalTitle() - returns a string.
       */
      String getOriginalTitle();
      /**
       * getPremiered() - returns a string.
       */
      String getPremiered();
      /**
       * getFirstAired() - returns a string.
       */
      String getFirstAired();
    };
  }
}
