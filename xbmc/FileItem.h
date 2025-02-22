/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
 \file FileItem.h
 \brief
 */

#include "LockMode.h"
#include "SourceType.h"
#include "XBDateTime.h"
#include "guilib/GUIListItem.h"
#include "threads/CriticalSection.h"
#include "utils/IArchivable.h"
#include "utils/ISerializable.h"
#include "utils/ISortable.h"
#include "utils/SortUtils.h"

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

class CMediaSource;
enum class VideoDbContentType;

namespace ADDON
{
class IAddon;
}

namespace MUSIC_INFO
{
  class CMusicInfoTag;
}
class CVideoInfoTag;
class CPictureInfoTag;

namespace KODI
{
namespace GAME
{
  class CGameInfoTag;
}
}

namespace PVR
{
class CPVRChannel;
class CPVRChannelGroupMember;
class CPVREpgInfoTag;
class CPVREpgSearchFilter;
class CPVRProvider;
class CPVRRecording;
class CPVRTimerInfoTag;
}

class CAlbum;
class CArtist;
class CSong;
class CGenre;

class CURL;
class CVariant;

class CFileItemList;
class CCueDocument;
typedef std::shared_ptr<CCueDocument> CCueDocumentPtr;

class IEvent;
typedef std::shared_ptr<const IEvent> EventPtr;

/* special startoffset used to indicate that we wish to resume */
#define STARTOFFSET_RESUME (-1)

class CBookmark;

enum class FileFolderType
{
  ALWAYS = 1 << 0,
  ONCLICK = 1 << 1,
  ONBROWSE = 1 << 2,

  MASK_ALL = 0xff,
  MASK_ONCLICK = ALWAYS | ONCLICK,
  MASK_ONBROWSE = ALWAYS | ONCLICK | ONBROWSE,
};

enum class MultipleEpisodes
{
  DONT_GROUP_MULTIPLE_EPISODES = 0,
  GROUP_MULTIPLE_EPISODES
};

/*!
  \brief Represents a file on a share
  \sa CFileItemList
  */
class CFileItem :
  public CGUIListItem, public IArchivable, public ISerializable, public ISortable
{
public:
  CFileItem(void);
  CFileItem(const CFileItem& item);
  explicit CFileItem(const CGUIListItem& item);
  explicit CFileItem(const std::string& strLabel);
  explicit CFileItem(const char* strLabel);
  CFileItem(const CURL& path, bool bIsFolder);
  CFileItem(const std::string& strPath, bool bIsFolder);
  explicit CFileItem(const CSong& song);
  CFileItem(const CSong& song, const MUSIC_INFO::CMusicInfoTag& music);
  CFileItem(const CURL &path, const CAlbum& album);
  CFileItem(const std::string &path, const CAlbum& album);
  explicit CFileItem(const CArtist& artist);
  explicit CFileItem(const CGenre& genre);
  explicit CFileItem(const MUSIC_INFO::CMusicInfoTag& music);
  explicit CFileItem(const CVideoInfoTag& movie);
  explicit CFileItem(const std::shared_ptr<PVR::CPVREpgInfoTag>& tag);
  explicit CFileItem(const std::shared_ptr<PVR::CPVREpgSearchFilter>& filter);
  explicit CFileItem(const std::shared_ptr<PVR::CPVRChannelGroupMember>& channelGroupMember);
  explicit CFileItem(const std::shared_ptr<PVR::CPVRRecording>& record);
  explicit CFileItem(const std::shared_ptr<PVR::CPVRTimerInfoTag>& timer);
  explicit CFileItem(const std::string& path, const std::shared_ptr<PVR::CPVRProvider>& provider);
  explicit CFileItem(const CMediaSource& share);
  explicit CFileItem(std::shared_ptr<const ADDON::IAddon> addonInfo);
  explicit CFileItem(const EventPtr& eventLogEntry);

  ~CFileItem(void) override;
  CGUIListItem* Clone() const override { return new CFileItem(*this); }

  const CURL GetURL() const;
  void SetURL(const CURL& url);
  bool IsURL(const CURL& url) const;
  const std::string& GetPath() const { return m_strPath; }
  void SetPath(const std::string& path) { m_strPath = path; }
  bool IsPath(const std::string& path, bool ignoreURLOptions = false) const;

  const CURL GetDynURL() const;
  void SetDynURL(const CURL& url);
  const std::string &GetDynPath() const;
  void SetDynPath(const std::string &path);

  /*! \brief reset class to it's default values as per construction.
   Free's all allocated memory.
   \sa Initialize
   */
  void Reset();
  CFileItem& operator=(const CFileItem& item);
  void Archive(CArchive& ar) override;
  void Serialize(CVariant& value) const override;
  void ToSortable(SortItem &sortable, Field field) const override;
  void ToSortable(SortItem &sortable, const Fields &fields) const;
  bool IsFileItem() const override { return true; }

  bool Exists(bool bUseCache = true) const;

  /*!
   \brief Check whether an item is a picture item. Note that this returns true for
    anything with a picture info tag, so that may include eg. folders.
   \return true if item is picture, false otherwise.
   */
  bool IsPicture() const;

  /*!
   \brief Check whether an item is 'deleted' (for example, a trashed pvr recording).
   \return true if item is 'deleted', false otherwise.
   */
  bool IsDeleted() const;

  bool IsGame() const;
  bool IsLibraryFolder() const;
  bool IsPythonScript() const;
  bool IsPlugin() const;
  bool IsScript() const;
  bool IsAddonsPath() const;
  bool IsSourcesPath() const;
  bool IsNFO() const;
  bool IsDiscImage() const;
  bool IsOpticalMediaFile() const;
  bool IsBluray() const;
  bool IsRAR() const;
  bool IsAPK() const;
  bool IsZIP() const;
  bool IsCBZ() const;
  bool IsCBR() const;
  bool IsISO9660() const;
  bool IsDVD() const;
  bool IsOnDVD() const;
  bool IsHD() const;
  bool IsNfs() const;
  bool IsSmb() const;
  bool IsURL() const;
  bool IsStack() const;
  bool IsFavourite() const;
  bool IsMultiPath() const;
  bool IsEPG() const;
  bool IsPVRChannel() const;
  bool IsPVRChannelGroup() const;
  bool IsPVRRecording() const;
  bool IsUsablePVRRecording() const;
  bool IsDeletedPVRRecording() const;
  bool IsInProgressPVRRecording() const;
  bool IsPVRTimer() const;
  bool IsPVRProvider() const;
  bool IsType(const char *ext) const;
  bool IsVirtualDirectoryRoot() const;
  bool IsReadOnly() const;
  bool CanQueue() const;
  void SetCanQueue(bool bYesNo);
  bool IsParentFolder() const;
  bool IsFileFolder(FileFolderType types = FileFolderType::MASK_ALL) const;
  bool IsRemovable() const;
  bool IsPVR() const;
  bool IsLiveTV() const;
  bool IsRSS() const;
  bool IsAndroidApp() const;

  bool HasVideoVersions() const;
  bool HasVideoExtras() const;

  void RemoveExtension();
  void CleanString();
  void SetFileSizeLabel();
  void SetLabel(const std::string &strLabel) override;
  VideoDbContentType GetVideoContentType() const;
  bool IsLabelPreformatted() const { return m_bLabelPreformatted; }
  void SetLabelPreformatted(bool bYesNo) { m_bLabelPreformatted=bYesNo; }
  bool SortsOnTop() const { return m_specialSort == SortSpecialOnTop; }
  bool SortsOnBottom() const { return m_specialSort == SortSpecialOnBottom; }
  void SetSpecialSort(SortSpecial sort) { m_specialSort = sort; }

  inline bool HasMusicInfoTag() const
  {
    return m_musicInfoTag != NULL;
  }

  MUSIC_INFO::CMusicInfoTag* GetMusicInfoTag();

  inline const MUSIC_INFO::CMusicInfoTag* GetMusicInfoTag() const
  {
    return m_musicInfoTag;
  }

  bool HasVideoInfoTag() const;

  CVideoInfoTag* GetVideoInfoTag();

  const CVideoInfoTag* GetVideoInfoTag() const;

  inline bool HasEPGInfoTag() const
  {
    return m_epgInfoTag.get() != NULL;
  }

  inline const std::shared_ptr<PVR::CPVREpgInfoTag> GetEPGInfoTag() const
  {
    return m_epgInfoTag;
  }

  bool HasEPGSearchFilter() const { return m_epgSearchFilter != nullptr; }

  const std::shared_ptr<PVR::CPVREpgSearchFilter> GetEPGSearchFilter() const
  {
    return m_epgSearchFilter;
  }

  inline bool HasPVRChannelGroupMemberInfoTag() const
  {
    return m_pvrChannelGroupMemberInfoTag.get() != nullptr;
  }

  inline const std::shared_ptr<PVR::CPVRChannelGroupMember> GetPVRChannelGroupMemberInfoTag() const
  {
    return m_pvrChannelGroupMemberInfoTag;
  }

  bool HasPVRChannelInfoTag() const;
  const std::shared_ptr<PVR::CPVRChannel> GetPVRChannelInfoTag() const;

  inline bool HasPVRRecordingInfoTag() const
  {
    return m_pvrRecordingInfoTag.get() != NULL;
  }

  inline const std::shared_ptr<PVR::CPVRRecording> GetPVRRecordingInfoTag() const
  {
    return m_pvrRecordingInfoTag;
  }

  inline bool HasPVRTimerInfoTag() const
  {
    return m_pvrTimerInfoTag != NULL;
  }

  inline const std::shared_ptr<PVR::CPVRTimerInfoTag> GetPVRTimerInfoTag() const
  {
    return m_pvrTimerInfoTag;
  }

  inline bool HasPVRProviderInfoTag() const { return m_pvrProviderInfoTag != nullptr; }

  inline const std::shared_ptr<PVR::CPVRProvider> GetPVRProviderInfoTag() const
  {
    return m_pvrProviderInfoTag;
  }

  /*!
   \brief return the item to play. will be almost 'this', but can be different (e.g. "Play recording" from PVR EPG grid window)
   \return the item to play
   */
  CFileItem GetItemToPlay() const;

  /*!
   \brief Test if this item has a valid resume point set.
   \return True if this item has a resume point and it is set, false otherwise.
   */
  bool IsResumePointSet() const;

  /*!
   \brief Return the current resume time.
   \return The time in seconds from the start to resume playing from.
   */
  double GetCurrentResumeTime() const;

  /*!
   \brief Return the current resume time and part.
   \param startOffset will be filled with the resume time offset in seconds if item has a resume point set, is unchanged otherwise
   \param partNumber will be filled with the part number if item has a resume point set, is unchanged otherwise
   \return True if the item has a resume point set, false otherwise.
   */
  bool GetCurrentResumeTimeAndPartNumber(int64_t& startOffset, int& partNumber) const;

  /*!
   * \brief Test if this item type can be resumed.
   * \return True if this item is a folder and has at least one child with a partway resume bookmark
   * or at least one unwatched child or if it is not a folder, if it has a partway resume bookmark,
   * false otherwise.
   */
  bool IsResumable() const;

  /*!
   * \brief Get the offset where start the playback.
   * \return The offset value as ms.
   *         Can return also special value -1, see define STARTOFFSET_RESUME.
   */
  int64_t GetStartOffset() const { return m_lStartOffset; }

  /*!
   * \brief Set the offset where start the playback.
   * \param offset Set the offset value as ms,
                   or the special value STARTOFFSET_RESUME.
   */
  void SetStartOffset(const int64_t offset) { m_lStartOffset = offset; }

  /*!
   * \brief Get the end offset.
   * \return The offset value as ms.
   */
  int64_t GetEndOffset() const { return m_lEndOffset; }

  /*!
   * \brief Set the end offset.
   * \param offset Set the offset as ms.
   */
  void SetEndOffset(const int64_t offset) { m_lEndOffset = offset; }

  inline bool HasPictureInfoTag() const
  {
    return m_pictureInfoTag != NULL;
  }

  inline const CPictureInfoTag* GetPictureInfoTag() const
  {
    return m_pictureInfoTag;
  }

  bool HasAddonInfo() const { return m_addonInfo != nullptr; }
  const std::shared_ptr<const ADDON::IAddon> GetAddonInfo() const { return m_addonInfo; }

  inline bool HasGameInfoTag() const
  {
    return m_gameInfoTag != NULL;
  }

  KODI::GAME::CGameInfoTag* GetGameInfoTag();

  inline const KODI::GAME::CGameInfoTag* GetGameInfoTag() const
  {
    return m_gameInfoTag;
  }

  CPictureInfoTag* GetPictureInfoTag();

  /*! \brief Assemble the filename of a particular piece of local artwork for an item,
             and check for file existence.
   \param artFile the art file to search for.
   \param useFolder whether to look in the folder for the art file. Defaults to false.
   \return the path to the local artwork if it exists, empty otherwise.
   \sa GetLocalArt
   */
  std::string FindLocalArt(const std::string &artFile, bool useFolder) const;

  /*! \brief Whether or not to skip searching for local art.
   \return true if local art should be skipped for this item, false otherwise.
   \sa GetLocalArt, FindLocalArt
   */
  bool SkipLocalArt() const;

  /*! \brief Get the thumb for the item, but hide it to prevent spoilers if
             the user has set 'Show information for unwatched items' appropriately.
   \param item the item to get the thumb image for.
   \return fanart or spoiler overlay if item is an unwatched episode, thumb art otherwise.
   */
  std::string GetThumbHideIfUnwatched(const CFileItem* item) const;

  // Gets the correct movie title
  std::string GetMovieName(bool bUseFolderNames = false) const;

  /*! \brief Find the base movie path (i.e. the item the user expects us to use to lookup the movie)
   For folder items, with "use foldernames for lookups" it returns the folder.
   Regardless of settings, for VIDEO_TS/BDMV it returns the parent of the VIDEO_TS/BDMV folder (if present)

   \param useFolderNames whether we're using foldernames for lookups
   \return the base movie folder
   */
  std::string GetBaseMoviePath(bool useFolderNames) const;

  // Gets the user thumb, if it exists
  std::string GetUserMusicThumb(bool alwaysCheckRemote = false, bool fallbackToFolder = false) const;

  /*! \brief Get the path where we expect local metadata to reside.
   For a folder, this is just the existing path (eg tvshow folder)
   For a file, this is the parent path, with exceptions made for VIDEO_TS and BDMV files

   Three cases are handled:

     /foo/bar/movie_name/file_name          -> /foo/bar/movie_name/
     /foo/bar/movie_name/VIDEO_TS/file_name -> /foo/bar/movie_name/
     /foo/bar/movie_name/BDMV/file_name     -> /foo/bar/movie_name/

     \sa URIUtils::GetParentPath
   */
  std::string GetLocalMetadataPath() const;

  bool LoadMusicTag();
  bool LoadGameTag();

  /*! \brief Load detailed data for an item constructed with only a path and a folder flag
   Fills item's video info tag, sets item properties.

   \return true on success, false otherwise.
   */
  bool LoadDetails();

  /* Returns the content type of this item if known */
  const std::string& GetMimeType() const { return m_mimetype; }

  /* sets the mime-type if known beforehand */
  void SetMimeType(const std::string& mimetype) { m_mimetype = mimetype; } ;

  /*! \brief Resolve the MIME type based on file extension or a web lookup
   If m_mimetype is already set (non-empty), this function has no effect. For
   http:// and shout:// streams, this will query the stream (blocking operation).
   Set lookup=false to skip any internet lookups and always return immediately.
   */
  void FillInMimeType(bool lookup = true);

  /*!
  \brief Some sources do not support HTTP HEAD request to determine i.e. mime type
  \return false if HEAD requests have to be avoided
  */
  bool ContentLookup() { return m_doContentLookup; }

  /*!
   \brief (Re)set the mime-type for internet files if allowed (m_doContentLookup)
   Some sources do not support HTTP HEAD request to determine i.e. mime type
   */
  void SetMimeTypeForInternetFile();

  /*!
   *\brief Lookup via HTTP HEAD request might not be needed, use this setter to
   * disable ContentLookup.
   */
  void SetContentLookup(bool enable) { m_doContentLookup = enable; }

  /* general extra info about the contents of the item, not for display */
  void SetExtraInfo(const std::string& info) { m_extrainfo = info; }
  const std::string& GetExtraInfo() const { return m_extrainfo; }

  /*! \brief Update an item with information from another item
   We take metadata information from the given item and supplement the current item
   with that info.  If tags exist in the new item we use the entire tag information.
   Properties are appended, and labels, thumbnail and icon are updated if non-empty
   in the given item.
   \param item the item used to supplement information
   \param replaceLabels whether to replace labels (defaults to true)
   \param replaceEpisodes whether to list all episodes on multi-episode disc (defaults to not)
   */
  void UpdateInfo(
      const CFileItem& item,
      bool replaceLabels = true,
      MultipleEpisodes replaceEpisodes = MultipleEpisodes::DONT_GROUP_MULTIPLE_EPISODES);

  /*! \brief Merge an item with information from another item
  We take metadata/art information from the given item and supplement the current
  item with that info. If tags exist in the new item we only merge the missing
  tag information. Properties are appended, and labels are updated if non-empty
  in the given item.
  */
  void MergeInfo(const CFileItem &item);

  bool IsSamePath(const CFileItem *item) const;

  bool IsAlbum() const;

  /*! \brief Sets details using the information from the CVideoInfoTag object
   Sets the videoinfotag and uses its information to set the label and path.
   \param video video details to use and set
   */
  void SetFromVideoInfoTag(const CVideoInfoTag &video);

  /*! \brief Sets details using the information from the CMusicInfoTag object
  Sets the musicinfotag and uses its information to set the label and path.
  \param music music details to use and set
  */
  void SetFromMusicInfoTag(const MUSIC_INFO::CMusicInfoTag &music);

  /*! \brief Sets details using the information from the CAlbum object
   Sets the album in the music info tag and uses its information to set the
   label and album-specific properties.
   \param album album details to use and set
   */
  void SetFromAlbum(const CAlbum &album);
  /*! \brief Sets details using the information from the CSong object
   Sets the song in the music info tag and uses its information to set the
   label, path, song-specific properties and artwork.
   \param song song details to use and set
   */
  void SetFromSong(const CSong &song);

  bool m_bIsShareOrDrive;    ///< is this a root share/drive
  /// If \e m_bIsShareOrDrive is \e true, use to get the share type.
  /// Types see: CMediaSource::m_iDriveType
  SourceType m_iDriveType;
  CDateTime m_dateTime;             ///< file creation date & time
  int64_t m_dwSize;             ///< file size (0 for folders)
  std::string m_strDVDLabel;
  std::string m_strTitle;
  int m_iprogramCount;
  int m_idepth;
  int m_lStartPartNumber;
  LockMode m_iLockMode;
  std::string m_strLockCode;
  int m_iHasLock; // 0 - no lock 1 - lock, but unlocked 2 - locked
  int m_iBadPwdCount;
  bool m_multipleTitles;

  void SetCueDocument(const CCueDocumentPtr& cuePtr);
  void LoadEmbeddedCue();
  bool HasCueDocument() const;
  bool LoadTracksFromCueDocument(CFileItemList& scannedItems);

private:
  /*! \brief initialize all members of this class (not CGUIListItem members) to default values.
   Called from constructors, and from Reset()
   \sa Reset, CGUIListItem
   */
  void Initialize();

  /*! \brief Recalculate item's MIME type if it is not set or is set to "application/octet-stream".
   Resolve the MIME type based on file extension or a web lookup.
   \sa FillInMimeType
   */
  void UpdateMimeType(bool lookup = true);

  /*!
   \brief Return the current resume point for this item.
   \return The resume point.
   */
  CBookmark GetResumePoint() const;

  /*!
   \brief Fill item's music tag from given epg tag.
   */
  void FillMusicInfoTag(const std::shared_ptr<const PVR::CPVREpgInfoTag>& tag);

  std::string m_strPath;            ///< complete path to item
  std::string m_strDynPath;

  SortSpecial m_specialSort;
  bool m_bIsParentFolder;
  bool m_bCanQueue;
  bool m_bLabelPreformatted;
  std::string m_mimetype;
  std::string m_extrainfo;
  bool m_doContentLookup;
  MUSIC_INFO::CMusicInfoTag* m_musicInfoTag;
  CVideoInfoTag* m_videoInfoTag;
  std::shared_ptr<PVR::CPVREpgInfoTag> m_epgInfoTag;
  std::shared_ptr<PVR::CPVREpgSearchFilter> m_epgSearchFilter;
  std::shared_ptr<PVR::CPVRRecording> m_pvrRecordingInfoTag;
  std::shared_ptr<PVR::CPVRTimerInfoTag> m_pvrTimerInfoTag;
  std::shared_ptr<PVR::CPVRChannelGroupMember> m_pvrChannelGroupMemberInfoTag;
  std::shared_ptr<PVR::CPVRProvider> m_pvrProviderInfoTag;
  CPictureInfoTag* m_pictureInfoTag;
  std::shared_ptr<const ADDON::IAddon> m_addonInfo;
  KODI::GAME::CGameInfoTag* m_gameInfoTag;
  EventPtr m_eventLogEntry;
  bool m_bIsAlbum;
  int64_t m_lStartOffset;
  int64_t m_lEndOffset;

  CCueDocumentPtr m_cueDocument;
};

/*!
  \brief A shared pointer to CFileItem
  \sa CFileItem
  */
typedef std::shared_ptr<CFileItem> CFileItemPtr;

/*!
  \brief A vector of pointer to CFileItem
  \sa CFileItem
  */
typedef std::vector< CFileItemPtr > VECFILEITEMS;

/*!
  \brief Iterator for VECFILEITEMS
  \sa CFileItemList
  */
typedef std::vector< CFileItemPtr >::iterator IVECFILEITEMS;

/*!
  \brief A map of pointers to CFileItem
  \sa CFileItem
  */
typedef std::map<std::string, CFileItemPtr > MAPFILEITEMS;

/*!
  \brief Pair for MAPFILEITEMS
  \sa MAPFILEITEMS
  */
typedef std::pair<std::string, CFileItemPtr > MAPFILEITEMSPAIR;

typedef bool (*FILEITEMLISTCOMPARISONFUNC) (const CFileItemPtr &pItem1, const CFileItemPtr &pItem2);
typedef void (*FILEITEMFILLFUNC)(CFileItemPtr& item);
