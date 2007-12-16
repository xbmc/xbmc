/*
 *      Copyright (C) 2005-2007 Team XboxMediaCenter
 *      http://www.xboxmediacenter.com
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "id3tag.h"
#include "util.h"
#include "picture.h"

using namespace MUSIC_INFO;

CID3Tag::CID3Tag()
{
  m_tag=NULL;
}

CID3Tag::~CID3Tag()
{
}

CStdString CID3Tag::ToStringCharset(const id3_ucs4_t* ucs4, id3_field_textencoding encoding) const
{
  if (!ucs4 || ucs4[0]==0)
    return "";

  CStdString strValue;

  if (encoding==ID3_FIELD_TEXTENCODING_ISO_8859_1)
  { // TODO: UTF-8: Should these be converted to UTF-8 ?
    id3_latin1_t* latin1=m_dll.id3_ucs4_latin1duplicate(ucs4);
    //strValue=(LPCSTR)latin1;
    g_charsetConverter.stringCharsetToUtf8((LPCSTR)latin1, strValue);
    m_dll.id3_latin1_free(latin1);
  }
  else
  {
    // convert to UTF-8
    id3_utf8_t* utf8 = m_dll.id3_ucs4_utf8duplicate(ucs4);
    strValue = (LPCSTR)utf8;
    m_dll.id3_utf8_free(utf8);
  }

  return strValue;
}

id3_ucs4_t* CID3Tag::StringCharsetToUcs4(const CStdString& str) const
{
  // our StringCharset is UTF-8
  return m_dll.id3_utf8_ucs4duplicate((id3_utf8_t*)str.c_str());
}

bool CID3Tag::Read(const CStdString& strFile)
{
  if (!m_dll.IsLoaded())
    m_dll.Load();

  CTag::Read(strFile);

  id3_file* id3file = m_dll.id3_file_open(strFile.c_str(), ID3_FILE_MODE_READONLY);
  if (!id3file)
    return false;

  m_tag = m_dll.id3_file_tag(id3file);
  if (!m_tag)
    return false;

  m_musicInfoTag.SetURL(strFile);

  Parse();

  m_dll.id3_file_close(id3file);
  return true;
}

bool CID3Tag::Parse()
{
  ParseReplayGainInfo();

  CMusicInfoTag& tag=m_musicInfoTag;

  tag.SetTrackNumber(GetTrack());

  tag.SetPartOfSet(GetPartOfSet());

  tag.SetGenre(GetGenre());

  tag.SetTitle(GetTitle());

  tag.SetArtist(GetArtist());

  tag.SetAlbum(GetAlbum());

  tag.SetAlbumArtist(GetAlbumArtist());

  tag.SetComment(GetComment());

  tag.SetLyrics(GetLyrics());

  tag.SetRating(GetRating());

  if (!tag.GetTitle().IsEmpty() || !tag.GetArtist().IsEmpty() || !tag.GetAlbum().IsEmpty())
    tag.SetLoaded();

  SYSTEMTIME dateTime;
  dateTime.wYear = atoi(GetYear());
  tag.SetReleaseDate(dateTime);

  id3_length_t lenght;
  const LPCSTR pb=(LPCSTR)GetUniqueFileIdentifier("http://musicbrainz.org", &lenght);
  if (pb)
  {
    CStdString strTrackId(pb, lenght);
    tag.SetMusicBrainzTrackID(strTrackId);
  }

  tag.SetMusicBrainzArtistID(GetUserText("MusicBrainz Artist Id"));
  tag.SetMusicBrainzAlbumID(GetUserText("MusicBrainz Album Id"));
  tag.SetMusicBrainzAlbumArtistID(GetUserText("MusicBrainz Album Artist Id"));
  tag.SetMusicBrainzTRMID(GetUserText("MusicBrainz TRM Id"));

  // extract Cover Art and save as album thumb
  bool bFound = false;
  id3_picture_type pictype = ID3_PICTURE_TYPE_COVERFRONT;
  if (HasPicture(pictype))
  {
    bFound = true;
  }
  else
  {
    pictype = ID3_PICTURE_TYPE_OTHER;
    if (HasPicture(pictype))
      bFound = true;
    else if (GetFirstNonStandardPictype(&pictype))
      bFound = true;
  }

  // if we don't have an album tag, cache with the full file path so that
  // other non-tagged files don't get this album image
  CStdString strCoverArt;
  if (!tag.GetAlbum().IsEmpty() && (!tag.GetAlbumArtist().IsEmpty() || !tag.GetArtist().IsEmpty()))
    strCoverArt = CUtil::GetCachedAlbumThumb(tag.GetAlbum(), tag.GetAlbumArtist().IsEmpty() ? tag.GetArtist() : tag.GetAlbumArtist());
  else
    strCoverArt = CUtil::GetCachedMusicThumb(tag.GetURL());
  if (bFound && !CUtil::ThumbExists(strCoverArt))
  {
    CStdString strExtension=GetPictureMimeType(pictype);

    int nPos = strExtension.Find('/');
    if (nPos > -1)
      strExtension.Delete(0, nPos + 1);

    id3_length_t nBufSize = 0;
    const BYTE* pPic = GetPictureData(pictype, &nBufSize );
    if (pPic != NULL && nBufSize > 0)
    {
      CPicture pic;
      if (pic.CreateThumbnailFromMemory(pPic, nBufSize, strExtension, strCoverArt))
      {
        CUtil::ThumbCacheAdd(strCoverArt, true);
      }
      else
      {
        CUtil::ThumbCacheAdd(strCoverArt, false);
        CLog::Log(LOGERROR, "Tag loader mp3: Unable to create album art for %s (extension=%s, size=%d)", tag.GetURL().c_str(), strExtension.c_str(), nBufSize);
      }
    }
  }

  return tag.Loaded();
}

bool CID3Tag::Write(const CStdString& strFile)
{
  if (!m_dll.IsLoaded())
    m_dll.Load();

  CTag::Read(strFile);

  id3_file* id3file = m_dll.id3_file_open(strFile.c_str(), ID3_FILE_MODE_READWRITE);
  if (!id3file)
    return false;

  m_tag = m_dll.id3_file_tag(id3file);
  if (!m_tag)
    return false;

  SetTitle(m_musicInfoTag.GetTitle());
  SetArtist(m_musicInfoTag.GetArtist());
  SetAlbum(m_musicInfoTag.GetAlbum());
  SetAlbumArtist(m_musicInfoTag.GetAlbumArtist());
  SetTrack(m_musicInfoTag.GetTrackNumber());
  SetGenre(m_musicInfoTag.GetGenre());
  SetYear(m_musicInfoTag.GetYearString());
  SetEncodedBy("XboxMediaCenter");

  m_dll.id3_tag_options(m_tag, ID3_TAG_OPTION_COMPRESSION, 0);
  m_dll.id3_tag_options(m_tag, ID3_TAG_OPTION_CRC, 0);
  m_dll.id3_tag_options(m_tag, ID3_TAG_OPTION_UNSYNCHRONISATION, 0);
  m_dll.id3_tag_options(m_tag, ID3_TAG_OPTION_ID3V1, 1);

  bool success=(m_dll.id3_file_update(id3file)!=-1) ? true : false;

  m_dll.id3_file_close(id3file);

  return success;
}

CStdString CID3Tag::GetArtist() const
{
  id3_field_textencoding encoding=ID3_FIELD_TEXTENCODING_ISO_8859_1;
  const id3_ucs4_t* ucs4=m_dll.id3_metadata_getartist(m_tag, &encoding);
  return ToStringCharset(ucs4, encoding);
}

CStdString CID3Tag::GetAlbum() const
{
  id3_field_textencoding encoding=ID3_FIELD_TEXTENCODING_ISO_8859_1;
  const id3_ucs4_t* ucs4=m_dll.id3_metadata_getalbum(m_tag, &encoding);
  return ToStringCharset(ucs4, encoding);
}

CStdString CID3Tag::GetAlbumArtist() const
{
  id3_field_textencoding encoding=ID3_FIELD_TEXTENCODING_ISO_8859_1;
  const id3_ucs4_t* ucs4=m_dll.id3_metadata_getalbumartist(m_tag, &encoding);
  return ToStringCharset(ucs4, encoding);
}

CStdString CID3Tag::GetTitle() const
{
  id3_field_textencoding encoding=ID3_FIELD_TEXTENCODING_ISO_8859_1;
  const id3_ucs4_t* ucs4=m_dll.id3_metadata_gettitle(m_tag, &encoding);
  return ToStringCharset(ucs4, encoding);
}

int CID3Tag::GetTrack() const
{
  id3_field_textencoding encoding=ID3_FIELD_TEXTENCODING_ISO_8859_1;
  const id3_ucs4_t* ucs4=m_dll.id3_metadata_gettrack(m_tag, &encoding);
  return atoi(ToStringCharset(ucs4, encoding));
}

int CID3Tag::GetPartOfSet() const
{
  id3_field_textencoding encoding=ID3_FIELD_TEXTENCODING_ISO_8859_1;
  const id3_ucs4_t* ucs4=m_dll.id3_metadata_getpartofset(m_tag, &encoding);
  return atoi(ToStringCharset(ucs4, encoding));
}

CStdString CID3Tag::GetYear() const
{
  id3_field_textencoding encoding=ID3_FIELD_TEXTENCODING_ISO_8859_1;
  const id3_ucs4_t* ucs4=m_dll.id3_metadata_getyear(m_tag, &encoding);
  return ToStringCharset(ucs4, encoding);
}

CStdString CID3Tag::GetGenre() const
{
  id3_field_textencoding encoding=ID3_FIELD_TEXTENCODING_ISO_8859_1;
  id3_ucs4_list_t* list=m_dll.id3_metadata_getgenres(m_tag, &encoding);
  CStdString genre;
  if (list)
  {
    for (unsigned int i = 0; i < list->nstrings; i++)
    {
      CStdString strGenre=ToStringCharset(list->strings[i], encoding);
      if (!strGenre.IsEmpty())
      {
        if (!genre.IsEmpty())
          genre += g_advancedSettings.m_musicItemSeparator;
        genre += ParseMP3Genre(strGenre);
      }
    }
    m_dll.id3_ucs4_list_free(list);
  }
  return genre;
}

CStdString CID3Tag::GetComment() const
{
  id3_field_textencoding encoding=ID3_FIELD_TEXTENCODING_ISO_8859_1;
  const id3_ucs4_t* ucs4=m_dll.id3_metadata_getcomment(m_tag, &encoding);
  return ToStringCharset(ucs4, encoding);
}

CStdString CID3Tag::GetEncodedBy() const
{
  id3_field_textencoding encoding=ID3_FIELD_TEXTENCODING_ISO_8859_1;
  const id3_ucs4_t* ucs4=m_dll.id3_metadata_getencodedby(m_tag, &encoding);
  return ToStringCharset(ucs4, encoding);
}

CStdString CID3Tag::GetLyrics() const
{
  id3_field_textencoding encoding=ID3_FIELD_TEXTENCODING_ISO_8859_1;
  const id3_ucs4_t* ucs4;
  struct id3_frame *frame;
  union id3_field *field;
  frame = m_dll.id3_tag_findframe (m_tag, "USLT", 0);
  if (!frame) return "";
  
  /* Find the encoding used, stored in frame 0 */
  field = m_dll.id3_frame_field (frame, 0);

  if (field && (m_dll.id3_field_type (field) == ID3_FIELD_TYPE_TEXTENCODING))
    encoding = m_dll.id3_field_gettextencoding(field);


  /* The last field contains the data */
  field = m_dll.id3_frame_field (frame, frame->nfields-1);
  if (!field) return "";
   
  if(field->type != ID3_FIELD_TYPE_STRINGFULL) return "";

  ucs4 = m_dll.id3_field_getfullstring (field);

  return ToStringCharset(ucs4, encoding);
}
char CID3Tag::GetRating() const
{
  return m_dll.id3_metadata_getrating(m_tag);
}

bool CID3Tag::HasPicture(id3_picture_type pictype) const
{
  return (m_dll.id3_metadata_haspicture(m_tag, pictype)>0 ? true : false);
}

CStdString CID3Tag::GetPictureMimeType(id3_picture_type pictype) const
{
  return (LPCSTR)m_dll.id3_metadata_getpicturemimetype(m_tag, pictype);
}

const BYTE* CID3Tag::GetPictureData(id3_picture_type pictype, id3_length_t* length) const
{
  return m_dll.id3_metadata_getpicturedata(m_tag, pictype, length);
}

const BYTE* CID3Tag::GetUniqueFileIdentifier(const CStdString& strOwnerIdentifier, id3_length_t* lenght) const
{
  return m_dll.id3_metadata_getuniquefileidentifier(m_tag, strOwnerIdentifier.c_str(), lenght);
}

CStdString CID3Tag::GetUserText(const CStdString& strDescription) const
{
  return ToStringCharset(m_dll.id3_metadata_getusertext(m_tag, strDescription.c_str()), ID3_FIELD_TEXTENCODING_ISO_8859_1);
}

bool CID3Tag::GetFirstNonStandardPictype(id3_picture_type* pictype) const
{
  return (m_dll.id3_metadata_getfirstnonstandardpictype(m_tag, pictype)>0 ? true : false);
}

void CID3Tag::SetArtist(const CStdString& strValue)
{
  id3_ucs4_t* ucs4=StringCharsetToUcs4(strValue);
  m_dll.id3_metadata_setartist(m_tag, ucs4);
  m_dll.id3_ucs4_free(ucs4);
}

void CID3Tag::SetAlbum(const CStdString& strValue)
{
  id3_ucs4_t* ucs4=StringCharsetToUcs4(strValue);
  m_dll.id3_metadata_setalbum(m_tag, ucs4);
  m_dll.id3_ucs4_free(ucs4);
}

void CID3Tag::SetAlbumArtist(const CStdString& strValue)
{
  id3_ucs4_t* ucs4=StringCharsetToUcs4(strValue);
  m_dll.id3_metadata_setalbumartist(m_tag, ucs4);
  m_dll.id3_ucs4_free(ucs4);
}

void CID3Tag::SetTitle(const CStdString& strValue)
{
  id3_ucs4_t* ucs4=StringCharsetToUcs4(strValue);
  m_dll.id3_metadata_settitle(m_tag, ucs4);
  m_dll.id3_ucs4_free(ucs4);
}

void CID3Tag::SetTrack(int n)
{
  CStdString strValue;
  strValue.Format("%d", n);
  id3_ucs4_t* ucs4=StringCharsetToUcs4(strValue);
  m_dll.id3_metadata_settrack(m_tag, ucs4);
  m_dll.id3_ucs4_free(ucs4);
}

void CID3Tag::SetPartOfSet(int n)
{
  CStdString strValue;
  strValue.Format("%d", n);
  id3_ucs4_t* ucs4=StringCharsetToUcs4(strValue);
  m_dll.id3_metadata_setpartofset(m_tag, ucs4);
  m_dll.id3_ucs4_free(ucs4);
}

void CID3Tag::SetYear(const CStdString& strValue)
{
  id3_ucs4_t* ucs4=StringCharsetToUcs4(strValue);
  m_dll.id3_metadata_setyear(m_tag, ucs4);
  m_dll.id3_ucs4_free(ucs4);
}

void CID3Tag::SetGenre(const CStdString& strValue)
{
  id3_ucs4_t* ucs4=StringCharsetToUcs4(strValue);
  m_dll.id3_metadata_setgenre(m_tag, ucs4);
  m_dll.id3_ucs4_free(ucs4);
}

void CID3Tag::SetEncodedBy(const CStdString& strValue)
{
  id3_ucs4_t* ucs4=StringCharsetToUcs4(strValue);
  m_dll.id3_metadata_setencodedby(m_tag, ucs4);
  m_dll.id3_ucs4_free(ucs4);
}

void CID3Tag::SetComment(const CStdString& strValue)
{
  id3_ucs4_t* ucs4=StringCharsetToUcs4(strValue);
  m_dll.id3_metadata_setcomment(m_tag, ucs4);
  m_dll.id3_ucs4_free(ucs4);
}

void CID3Tag::SetRating(char rating)
{
  m_dll.id3_metadata_setrating(m_tag, rating);
}

CStdString CID3Tag::ParseMP3Genre(const CStdString& str) const
{
  if (!m_dll.IsLoaded())
    m_dll.Load();

  CStdString strTemp = str;
  set<CStdString> setGenres;

  while (!strTemp.IsEmpty())
  {
    // remove any leading spaces
    int i = strTemp.find_first_not_of(" ");
    if (i > 0) strTemp.erase(0, i);

    // pull off the first character
    char p = strTemp[0];

    // start off looking for (something)
    if (p == '(')
    {
      strTemp.erase(0, 1);

      // now look for ((something))
      p = strTemp[0];
      if (p == '(')
      {
        // remove ((something))
        i = strTemp.find_first_of("))");
        strTemp.erase(0, i + 2);
      }
    }

    // no parens, so we have a start of a string
    // push chars into temp string until valid terminator found
    // valid terminators are ) or , or ;
    else
    {
      CStdString t;
      while ((!strTemp.IsEmpty()) && (p != ')') && (p != ',') && (p != ';'))
      {
        strTemp.erase(0, 1);
        t.push_back(p);
        p = strTemp[0];
      }
      // loop exits when terminator is found
      // be sure to remove the terminator
      strTemp.erase(0, 1);

      // remove any leading or trailing white space
      // from temp string
      t.Trim();
      if (!t.size()) continue;

      // if the temp string is natural number try to convert it to a genre string
      if (StringUtils::IsNaturalNumber(t))
      {
        char * pEnd;
        long l = strtol(t.c_str(), &pEnd, 0);

        id3_ucs4_t* ucs4=m_dll.id3_latin1_ucs4duplicate((id3_latin1_t*)t.c_str());
        const id3_ucs4_t* genre=m_dll.id3_genre_name(ucs4);
        m_dll.id3_ucs4_free(ucs4);
        t=ToStringCharset(genre, ID3_FIELD_TEXTENCODING_ISO_8859_1);
      }

      // convert RX to Remix as per ID3 V2.3 spec
      else if ((t == "RX") || (t == "Rx") || (t == "rX") || (t == "rx"))
      {
        t = "Remix";
      }

      // convert CR to Cover as per ID3 V2.3 spec
      else if ((t == "CR") || (t == "Cr") || (t == "cR") || (t == "cr"))
      {
        t = "Cover";
      }

      // insert genre name in set
      setGenres.insert(t);
    }

  }

  // return a " / " seperated string
  CStdString strGenre;
  set<CStdString>::iterator it;
  for (it = setGenres.begin(); it != setGenres.end(); it++)
  {
    CStdString strTemp = *it;
    if (!strGenre.IsEmpty())
      strGenre += g_advancedSettings.m_musicItemSeparator;
    strGenre += strTemp;
  }
  return strGenre;
}


void CID3Tag::ParseReplayGainInfo()
{
  CStdString strGain = GetUserText("replaygain_track_gain");
  if (!strGain.IsEmpty())
  {
    m_replayGain.iTrackGain = (int)(atof(strGain.c_str()) * 100 + 0.5);
    m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_TRACK_INFO;
  }
  strGain = GetUserText("replaygain_album_gain");
  if (!strGain.IsEmpty())
  {
    m_replayGain.iAlbumGain = (int)(atof(strGain.c_str()) * 100 + 0.5);
    m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_ALBUM_INFO;
  }
  strGain = GetUserText("replaygain_track_peak");
  if (!strGain.IsEmpty())
  {
    m_replayGain.fTrackPeak = (float)atof(strGain.c_str());
    m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_TRACK_PEAK;
  }
  strGain = GetUserText("replaygain_album_peak");
  if (!strGain.IsEmpty())
  {
    m_replayGain.fAlbumPeak = (float)atof(strGain.c_str());
    m_replayGain.iHasGainInfo |= REPLAY_GAIN_HAS_ALBUM_PEAK;
  }
}
