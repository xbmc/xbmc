/*
 *      Copyright (C) 2005-2011 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "utils/XMLUtils.h"

struct DatabaseSettings
{
  CStdString type;
  CStdString host;
  CStdString port;
  CStdString user;
  CStdString pass;
  CStdString name;
};

struct TVShowRegexp
{
  bool byDate;
  CStdString regexp;
  TVShowRegexp(bool d, const CStdString& r)
  {
    byDate = d;
    regexp = r;
  }
};

typedef std::vector<TVShowRegexp> SETTINGS_TVSHOWLIST;

class CLibrarySettings
{
  public:
    CLibrarySettings();
    CLibrarySettings(TiXmlElement *pRootElement);
    void Clear();
    bool ShowPlaylistAsFolders();
    bool UseDDSFanArt();
    bool MusicLibraryAllItemsOnBottom();
    bool MusicLibraryAlbumsSortByArtistThenYear();
    bool MusicLibraryHideAllItems();
    bool VideoLibraryCleanOnUpdate();
    bool VideoScannerIgnoreErrors();
    bool PrioritiseAPEv2tags();
    bool VideoLibraryHideEmptySeries();
    bool VideoLibraryExportAutoThumbs();
    bool VideoLibraryImportWatchedState();
    bool VideoLibraryHideAllItems();
    bool VideoLibraryAllItemsOnBottom();
    bool VideoLibraryHideRecentlyAddedItems();
    int ThumbSize();
    int FanartHeight();
    int PlaylistTimeout();
    int PlaylistRetries();
    int VideoLibraryRecentlyAddedItems();
    int MusicLibraryRecentlyAddedItems();
    int SongInfoDuration();
    CStdString MusicItemSeparator();
    CStdString VideoItemSeparator();
    CStdString FanartImages();
    CStdString DVDThumbs();
    CStdString MusicThumbs();
    CStdString MusicLibraryAlbumFormat();
    CStdString MusicLibraryAlbumFormatRight();
    CStdString TVShowMultiPartEnumRegExp();
    CStdString CDDBAddress();
    CStdStringArray VideoStackRegExps();
    DatabaseSettings DatabaseMusic() { return m_databaseMusic; };
    DatabaseSettings DatabaseVideo() { return m_databaseVideo; };
    std::vector<CStdString> VecTokens();
    std::vector<CStdString> MusicTagsFromFileFilters();
    SETTINGS_TVSHOWLIST TVShowEnumRegExps();
  private:
    void Initialise();
    void GetCustomRegexps(TiXmlElement *pRootElement, CStdStringArray& settings);
    void GetCustomExtensions(TiXmlElement *pRootElement, CStdString& extensions);
    void GetCustomTVRegexps(TiXmlElement *pRootElement, SETTINGS_TVSHOWLIST& settings);
    bool m_bVideoLibraryHideAllItems;
    bool m_bVideoLibraryAllItemsOnBottom;
    int m_iVideoLibraryRecentlyAddedItems;
    bool m_bVideoLibraryHideRecentlyAddedItems;
    bool m_bVideoLibraryHideEmptySeries;
    bool m_bVideoLibraryCleanOnUpdate;
    bool m_bVideoLibraryExportAutoThumbs;
    bool m_bVideoLibraryImportWatchedState;
    bool m_bVideoScannerIgnoreErrors;
    bool m_playlistAsFolders;
    bool m_useDDSFanart;
    bool m_detectAsUdf;
    int m_playlistRetries;
    int m_playlistTimeout;
    int m_thumbSize;
    int m_fanartHeight;
    int m_songInfoDuration;
    bool m_prioritiseAPEv2tags;
    bool m_bMusicLibraryHideAllItems;
    int m_iMusicLibraryRecentlyAddedItems;
    bool m_bMusicLibraryAllItemsOnBottom;
    bool m_bMusicLibraryAlbumsSortByArtistThenYear;
    CStdString m_musicItemSeparator;
    CStdString m_videoItemSeparator;
    CStdString m_strMusicLibraryAlbumFormat;
    CStdString m_strMusicLibraryAlbumFormatRight;
    CStdString m_musicThumbs;
    CStdString m_dvdThumbs;
    CStdString m_fanartImages;
    CStdString m_tvshowMultiPartEnumRegExp;
    CStdString m_cddbAddress;
    CStdStringArray m_videoStackRegExps;
    DatabaseSettings m_databaseMusic; // advanced music database setup
    DatabaseSettings m_databaseVideo; // advanced video database setup
    std::vector<CStdString> m_vecTokens; // cleaning strings tied to language
    std::vector<CStdString> m_musicTagsFromFileFilters;
    SETTINGS_TVSHOWLIST m_tvshowEnumRegExps;
};