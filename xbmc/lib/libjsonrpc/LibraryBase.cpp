#include "LibraryBase.h"
#include "../Util.h"

using namespace Json;

void CLibraryBase::FillVideoDetails(const CVideoInfoTag *videoInfo, const Value& parameterObject, Value &result)
{
  if (videoInfo->IsEmpty())
    return;

  if (!videoInfo->m_strTitle.IsEmpty())
    result["title"] = videoInfo->m_strTitle.c_str();

  const Value fields = parameterObject["fields"];
  for (unsigned int i = 0; i < fields.size(); i++)
  {
    CStdString field = fields[i].asString();

    if (field.Equals("file") && !videoInfo->m_strFileNameAndPath.IsEmpty())
      result["file"] = videoInfo->m_strTitle.c_str();

    if (field.Equals("genre") && !videoInfo->m_strGenre.IsEmpty())
      result["genre"] = videoInfo->m_strGenre.c_str();
    if (field.Equals("tagline") && !videoInfo->m_strTagLine.IsEmpty())
      result["tagline"] = videoInfo->m_strTagLine.c_str();
    if (field.Equals("plot") && !videoInfo->m_strPlot.IsEmpty())
      result["plot"] = videoInfo->m_strPlot.c_str();

    if (field.Equals("runtime") && !videoInfo->m_strRuntime.IsEmpty())
      result["runtime"] = videoInfo->m_strRuntime.c_str();
  }
}

void CLibraryBase::FillMusicDetails(const CMusicInfoTag *musicInfo, const Value& parameterObject, Value &result)
{
  if (!musicInfo->GetTitle().IsEmpty())
    result["title"] =  musicInfo->GetTitle().c_str();
  else if (!musicInfo->GetAlbum().IsEmpty())
    result["album"] =  musicInfo->GetAlbum().c_str();
  else if (!musicInfo->GetArtist().IsEmpty())
    result["artist"] =  musicInfo->GetArtist().c_str();

  const Json::Value fields = parameterObject["fields"];

  for (unsigned int i = 0; i < fields.size(); i++)
  {
    CStdString field = fields[i].asString();

    if (field.Equals("file") && !musicInfo->GetURL().IsEmpty())
      result["file"] =  musicInfo->GetURL().c_str();
    if (field.Equals("title") && !musicInfo->GetTitle().IsEmpty())
      result["title"] =  musicInfo->GetTitle().c_str();
    if (field.Equals("album") && !musicInfo->GetAlbum().IsEmpty())
      result["album"] =  musicInfo->GetAlbum().c_str();
    if (field.Equals("artist") && !musicInfo->GetArtist().IsEmpty())
      result["artist"] =  musicInfo->GetArtist().c_str();
    if (field.Equals("albumartist") && !musicInfo->GetAlbumArtist().IsEmpty())
      result["albumartist"] =  musicInfo->GetAlbumArtist().c_str();

    if (field.Equals("genre") && !musicInfo->GetGenre().IsEmpty())
      result["genre"] =  musicInfo->GetGenre().c_str();

    if (field.Equals("tracknumber"))
      result["tracknumber"] = (int)musicInfo->GetTrackNumber();
    if (field.Equals("discnumber"))
      result["discnumber"] = (int)musicInfo->GetDiscNumber();
    if (field.Equals("trackanddiscnumber"))
      result["trackanddiscnumber"] = (int)musicInfo->GetTrackAndDiskNumber();
    if (field.Equals("duration"))
      result["duration"] = (int)musicInfo->GetDuration();
    if (field.Equals("year"))
      result["year"] = (int)musicInfo->GetYear();

//  void GetReleaseDate(SYSTEMTIME& dateTime) const;
    if (field.Equals("albumartist") && !musicInfo->GetYearString().IsEmpty())
      result["albumartist"] =  musicInfo->GetYearString().c_str();

    if (field.Equals("musicbrainztrackid") && !musicInfo->GetMusicBrainzTrackID().IsEmpty())
      result["musicbrainztrackid"] =  musicInfo->GetMusicBrainzTrackID().c_str();
    if (field.Equals("musicbrainzartistid") && !musicInfo->GetMusicBrainzArtistID().IsEmpty())
      result["musicbrainzartistid"] =  musicInfo->GetMusicBrainzArtistID().c_str();
    if (field.Equals("musicbrainzalbumid") && !musicInfo->GetMusicBrainzAlbumID().IsEmpty())
      result["musicbrainzalbumid"] =  musicInfo->GetMusicBrainzAlbumID().c_str();
    if (field.Equals("musicbrainzalbumartistid") && !musicInfo->GetMusicBrainzAlbumArtistID().IsEmpty())
      result["musicbrainzalbumartistid"] =  musicInfo->GetMusicBrainzAlbumArtistID().c_str();
    if (field.Equals("musicbrainztrmidid") && !musicInfo->GetMusicBrainzTRMID().IsEmpty())
      result["musicbrainztrmidid"] =  musicInfo->GetMusicBrainzTRMID().c_str();

    if (field.Equals("comment") && !musicInfo->GetComment().IsEmpty())
      result["comment"] =  musicInfo->GetComment().c_str();
    if (field.Equals("lyrics") && !musicInfo->GetLyrics().IsEmpty())
      result["lyrics"] =  musicInfo->GetLyrics().c_str();

    if (field.Equals("rating"))
      result["rating"] = (int)musicInfo->GetRating();
  }
}

void CLibraryBase::HandleFileItemList(const CStdString &id, const CStdString &resultname, CFileItemList &items, unsigned int &start, unsigned int &end, const Value& parameterObject, Value &result)
{
  unsigned int size   = (unsigned int)items.Size();
  start = parameterObject.get("start", 0).asUInt();
  end   = parameterObject.get("end", size).asUInt();
  end = end > size ? size : end;

  Sort(items, parameterObject);

  result["start"] = start;
  result["end"]   = end;
  result["total"] = size;

  for (unsigned int i = start; i < end; i++)
  {
    Value object;
    CFileItemPtr item = items.Get(i);

    if (item->HasMusicInfoTag())
      object[id] = (int)item->GetMusicInfoTag()->GetDatabaseId();
    else if (item->HasVideoInfoTag())
      object[id] = item->GetVideoInfoTag()->m_iDbId;
    else
    {
//      CUtil::RemoveSlashAtEnd(item->m_strPath);
      object[id] = item->m_strPath.c_str();
    }

    if (!item->GetThumbnailImage().IsEmpty())
      object["thumbnail"] = item->GetThumbnailImage().c_str();

    if (item->HasVideoInfoTag())
      FillVideoDetails(item->GetVideoInfoTag(), parameterObject, object);
    if (item->HasMusicInfoTag())
      FillMusicDetails(item->GetMusicInfoTag(), parameterObject, object);

    result[resultname].append(object);
  }
}

bool CLibraryBase::ParseSortMethods(const CStdString &method, const CStdString &order, SORT_METHOD &sortmethod, SORT_ORDER &sortorder)
{
  if (order.Equals("ascending"))
    sortorder = SORT_ORDER_ASC;
  else if (order.Equals("descending"))
    sortorder = SORT_ORDER_DESC;
  else
    return false;

  if (method.Equals("none"))
    sortmethod = SORT_METHOD_NONE;
  else if (method.Equals("label"))
    sortmethod = SORT_METHOD_LABEL;
  else if (method.Equals("labelignorethe"))
    sortmethod = SORT_METHOD_LABEL_IGNORE_THE;
  else if (method.Equals("date"))
    sortmethod = SORT_METHOD_DATE;
  else if (method.Equals("size"))
    sortmethod = SORT_METHOD_SIZE;
  else if (method.Equals("file"))
    sortmethod = SORT_METHOD_FILE;
  else if (method.Equals("drivetype"))
    sortmethod = SORT_METHOD_DRIVE_TYPE;
  else if (method.Equals("track"))
    sortmethod = SORT_METHOD_TRACKNUM;
  else if (method.Equals("duration"))
    sortmethod = SORT_METHOD_DURATION;
  else if (method.Equals("title"))
    sortmethod = SORT_METHOD_TITLE;
  else if (method.Equals("titleignorethe"))
    sortmethod = SORT_METHOD_TITLE_IGNORE_THE;
  else if (method.Equals("artist"))
    sortmethod = SORT_METHOD_ARTIST;
  else if (method.Equals("artistignorethe"))
    sortmethod = SORT_METHOD_ARTIST_IGNORE_THE;
  else if (method.Equals("album"))
    sortmethod = SORT_METHOD_ALBUM;
  else if (method.Equals("albumignorethe"))
    sortmethod = SORT_METHOD_ALBUM_IGNORE_THE;
  else if (method.Equals("genre"))
    sortmethod = SORT_METHOD_GENRE;
  else if (method.Equals("year"))
    sortmethod = SORT_METHOD_YEAR;
  else if (method.Equals("videorating"))
    sortmethod = SORT_METHOD_VIDEO_RATING;
  else if (method.Equals("programcount"))
    sortmethod = SORT_METHOD_PROGRAM_COUNT;
  else if (method.Equals("playlist"))
    sortmethod = SORT_METHOD_PLAYLIST_ORDER;
  else if (method.Equals("episode"))
    sortmethod = SORT_METHOD_EPISODE;
  else if (method.Equals("title"))
    sortmethod = SORT_METHOD_VIDEO_TITLE;
  else if (method.Equals("sorttitle"))
    sortmethod = SORT_METHOD_VIDEO_SORT_TITLE;
  else if (method.Equals("sorttitleignorethe"))
    sortmethod = SORT_METHOD_VIDEO_SORT_TITLE_IGNORE_THE;
  else if (method.Equals("productioncode"))
    sortmethod = SORT_METHOD_PRODUCTIONCODE;
  else if (method.Equals("songrating"))
    sortmethod = SORT_METHOD_SONG_RATING;
  else if (method.Equals("mpaarating"))
    sortmethod = SORT_METHOD_MPAA_RATING;
  else if (method.Equals("videoruntime"))
    sortmethod = SORT_METHOD_VIDEO_RUNTIME;
  else if (method.Equals("studio"))
    sortmethod = SORT_METHOD_STUDIO;
  else if (method.Equals("studioignorethe"))
    sortmethod = SORT_METHOD_STUDIO_IGNORE_THE;
  else if (method.Equals("fullpath"))
    sortmethod = SORT_METHOD_FULLPATH;
  else if (method.Equals("unsorted"))
    sortmethod = SORT_METHOD_UNSORTED;
  else if (method.Equals("max"))
    sortmethod = SORT_METHOD_MAX;
  else
    return false;

  return true;
}

void CLibraryBase::Sort(CFileItemList &items, const Value& parameterObject)
{
  if (parameterObject.isMember("sortmethod") && parameterObject.isMember("sortorder"))
  {
    CStdString method = parameterObject.get("sortmethod", "none").asString();
    CStdString order  = parameterObject.get("sortorder",  "ascending").asString();
    method = method.ToLower();
    order  = order.ToLower();

    SORT_METHOD sortmethod;
    SORT_ORDER sortorder;

    if (ParseSortMethods(method, order, sortmethod, sortorder))
      items.Sort(sortmethod, sortorder);
  }
}
