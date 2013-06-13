#include "PlexDirectoryTypeParserTrack.h"

#include "music/Song.h"
#include "utils/StringUtils.h"

void
CPlexDirectoryTypeParserTrack::Process(CFileItem &item, CFileItem &mediaContainer, TiXmlElement *itemElement)
{
  CSong song;
  
  song.strFileName = item.GetPath();
  song.strTitle = item.GetLabel();
  song.strThumb = item.GetArt(PLEX_ART_THUMB);
  song.strComment = item.GetProperty("summary").asString();
  
  if (item.HasProperty("originallyAvailableAt"))
  {
    std::vector<std::string> s = StringUtils::Split(item.GetProperty("originallyAvailableAt").asString(), "-");
    if (s.size() > 0)
      song.iYear = boost::lexical_cast<int>(s[0]);
  }
  
  ParseMediaNodes(item, itemElement);

  /* Now we have the Media nodes, we need to "borrow" some properties from it */
  if (item.m_mediaItems.size() > 0)
  {
    CFileItemPtr firstMedia = item.m_mediaItems[0];
    item.m_mapProperties.insert(firstMedia->m_mapProperties.begin(), firstMedia->m_mapProperties.end());
    
    /* also forward art, this is the mediaTags */
    item.AppendArt(firstMedia->GetArt());
  }
}
