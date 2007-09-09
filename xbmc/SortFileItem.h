#pragma once

#include "FileItem.h"


struct SSortFileItem
{
  // Sort by file - this sorts by entire filepath, followed by the startoffset (for .cue items)
  static bool FileAscending(CFileItem *left, CFileItem *right);
  static bool FileDescending(CFileItem *left, CFileItem *right);

  // Sort by date
  static bool DateAscending(CFileItem *left, CFileItem *right);
  static bool DateDescending(CFileItem *left, CFileItem *right);

  // Sort by filesize
  static bool SizeAscending(CFileItem *left, CFileItem *right);
  static bool SizeDescending(CFileItem *left, CFileItem *right);

  // Sort by Drive type, and then by label
  static bool DriveTypeAscending(CFileItem *left, CFileItem *right);
  static bool DriveTypeDescending(CFileItem *left, CFileItem *right);

  // Sort by Label - the NoThe methods remove the "The " in front of items
  static bool LabelAscending(CFileItem *left, CFileItem *right);
  static bool LabelDescending(CFileItem *left, CFileItem *right);
  static bool LabelAscendingNoThe(CFileItem *left, CFileItem *right);
  static bool LabelDescendingNoThe(CFileItem *left, CFileItem *right);

  // Sort by Title
  static bool SongTitleAscending(CFileItem *left, CFileItem *right);
  static bool SongTitleDescending(CFileItem *left, CFileItem *right);
  static bool SongTitleAscendingNoThe(CFileItem *left, CFileItem *right);
  static bool SongTitleDescendingNoThe(CFileItem *left, CFileItem *right);

  // Sort by Movie Title
  static bool MovieTitleAscending(CFileItem *left, CFileItem *right);
  static bool MovieTitleDescending(CFileItem *left, CFileItem *right);

  // Sort by album (then artist, then tracknumber)
  static bool SongAlbumAscending(CFileItem *left, CFileItem *right);
  static bool SongAlbumDescending(CFileItem *left, CFileItem *right);
  static bool SongAlbumAscendingNoThe(CFileItem *left, CFileItem *right);
  static bool SongAlbumDescendingNoThe(CFileItem *left, CFileItem *right);

  // Sort by artist (then album, then tracknumber)
  static bool SongArtistAscending(CFileItem *left, CFileItem *right);
  static bool SongArtistDescending(CFileItem *left, CFileItem *right);
  static bool SongArtistAscendingNoThe(CFileItem *left, CFileItem *right);
  static bool SongArtistDescendingNoThe(CFileItem *left, CFileItem *right);

  // Sort by artist (then title)
  static bool MusicVideoArtistAscending(CFileItem *left, CFileItem *right);
  static bool MusicVideoArtistDescending(CFileItem *left, CFileItem *right);
  static bool MusicVideoArtistAscendingNoThe(CFileItem *left, CFileItem *right);
  static bool MusicVideoArtistDescendingNoThe(CFileItem *left, CFileItem *right);

  // Sort by genre
  static bool SongGenreAscending(CFileItem *left, CFileItem *right);
  static bool SongGenreDescending(CFileItem *left, CFileItem *right);

  // Sort by track number
  static bool SongTrackNumAscending(CFileItem *left, CFileItem *right);
  static bool SongTrackNumDescending(CFileItem *left, CFileItem *right);

  // Sort by episode number
  static bool EpisodeNumAscending(CFileItem *left, CFileItem *right);
  static bool EpisodeNumDescending(CFileItem *left, CFileItem *right);

  // Sort by song duration
  static bool SongDurationAscending(CFileItem *left, CFileItem *right);
  static bool SongDurationDescending(CFileItem *left, CFileItem *right);

  // Sort by program count
  static bool ProgramCountAscending(CFileItem *left, CFileItem *right);
  static bool ProgramCountDescending(CFileItem *left, CFileItem *right);

  // Sort by Movie Year, and if equal, sort by label
  static bool MovieYearAscending(CFileItem *left, CFileItem *right);
  static bool MovieYearDescending(CFileItem *left, CFileItem *right);

  // Sort by Movie Rating, and if equal, sort by label
  static bool MovieRatingAscending(CFileItem *left, CFileItem *right);
  static bool MovieRatingDescending(CFileItem *left, CFileItem *right);

  // Sort by MPAA Rating, and if equal, sort by label
  static bool MPAARatingAscending(CFileItem *left, CFileItem *right);
  static bool MPAARatingDescending(CFileItem *left, CFileItem *right);

  // Sort by Production Code
  static bool ProductionCodeAscending(CFileItem *left, CFileItem *right);
  static bool ProductionCodeDescending(CFileItem *left, CFileItem *right);

  // Sort by Song Rating (0 -> 5)
  static bool SongRatingAscending(CFileItem *left, CFileItem *right);
  static bool SongRatingDescending(CFileItem *left, CFileItem *right);

  // Sort by Movie Runtime, and if equal, sort by label
  static bool MovieRuntimeAscending(CFileItem *left, CFileItem *right);
  static bool MovieRuntimeDescending(CFileItem *left, CFileItem *right);

  // Sort by Studio, and if equal, sort by label
  static bool StudioAscending(CFileItem *left, CFileItem *right);
  static bool StudioDescending(CFileItem *left, CFileItem *right);
  static bool StudioAscendingNoThe(CFileItem *left, CFileItem *right);
  static bool StudioDescendingNoThe(CFileItem *left, CFileItem *right);

};
