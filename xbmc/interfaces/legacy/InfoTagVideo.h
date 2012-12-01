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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
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

      String getDirector();
      String getWritingCredits();
      String getGenre();
      String getTagLine();
      String getPlotOutline();
      String getPlot();
      String getPictureURL();
      String getTitle();
      String getVotes();
      String getCast();
      String getFile();
      String getPath();
      String getIMDBNumber();
      int getYear();
      double getRating();

      /**
       * getPlayCount() -- returns a integer.
       */
      int getPlayCount();

      /**
       * getLastPlayed() -- returns a string.
       */
      String getLastPlayed();
      String getOriginalTitle();
      String getPremiered();
      String getFirstAired();
    };
  }
}
