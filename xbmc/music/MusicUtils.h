/*
 *      Copyright (C) 2018 Team Kodi
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

#pragma once

#include "FileItem.h"

namespace MUSIC_UTILS
{
  /*! \brief Show a dialog to allow the selection of type of art from a list.
  Input is a fileitem list, with each item having an "arttype" property
  e.g. "thumb", current art URL (if art exists), and label. One of these art types
  can be selected, or a new art type added. The new art type is added as a new item
  in the list, as well as retuned as the selected art type.
  \param artitems [in/out] a fileitem list to display
  \return the selected art type e.g. "fanart" or empty string when cancelled.
  \sa FillArtTypesList
  */
  std::string ShowSelectArtTypeDialog(CFileItemList& artitems);

  /*! \brief Helper function to build a list of art types for a music library item.
  This fetches the possible types of art for a song, album or artist, and the
  current art URL (if the item has art of that type), for display on a dialog.
  \param musicitem a music CFileItem (song, album or artist)
  \param artitems [out] a fileitem list,  each item having "arttype" property
  e.g. "thumb", current art URL (if art exists), and localized label (for common arttypes)
  \return true if art types are retrieved, false if none is found.
  \sa ShowSelectArtTypeDialog
  */
  bool FillArtTypesList(CFileItem& musicitem, CFileItemList& artlist);

  /*! \brief Helper function to asynchronously update art in the music database
  and then refresh the album & artist art of the currently playing song.
  For the song, album or artist this adds a job to the queue to update the art table
  modifying, adding or deleting that type of art. Changes to album or artist art are
  then passed to the currently playing song (if there is one).
  \param item a shared pointer to a music CFileItem (song, album or artist)
  \param strType the type of art e.g. "fanart" or "thumb" etc.
  \param strArt art URL, when empty the entry for that type of art is deleted.
  */
  void UpdateArtJob(const CFileItemPtr pItem, const std::string& strType, const std::string& strArt);

  /*! \brief Show a dialog to allow the selection of user rating.
  \param iSelected the rating to show initially
  \return the selected rating, 0 (no rating), 1 to 10 or -1 no rating selected
  */
  int ShowSelectRatingDialog(int iSelected);

/*! \brief Helper function to asynchronously update the user rating of a song
\param pItem pointer to song item being rated
\param userrating the userrating 0 = no rating, 1 to 10
*/
  void UpdateSongRatingJob(const CFileItemPtr pItem, int userrating);
}
