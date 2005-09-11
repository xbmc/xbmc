
#include "stdafx.h"
#include "id3tag.h"
#include "util.h"
#include "picture.h"

#define ID3_DLL "Q:\\system\\libid3tag.dll"

using namespace MUSIC_INFO;

CID3Tag::CID3Tag()
{
  // dll stuff
  ZeroMemory(&m_dll, sizeof(ID3dll));
  m_bDllLoaded = false;

  m_tag=NULL;

  LoadDLL();
}

CID3Tag::~CID3Tag()
{
  if (m_bDllLoaded)
    CSectionLoader::UnloadDLL(ID3_DLL);
}

CStdString CID3Tag::Ucs4ToStringCharset(const id3_ucs4_t* ucs4) const
{
  if (!ucs4 || ucs4[0]==0)
    return "";

  id3_utf8_t* utf8=m_dll.id3_ucs4_utf8duplicate(ucs4);

  CStdString strValue;
  CStdString strSource=(LPCSTR)utf8;
  g_charsetConverter.utf8ToStringCharset(strSource, strValue);

  m_dll.id3_utf8_free(utf8);

  return strValue;
}

id3_ucs4_t* CID3Tag::StringCharsetToUcs4(const CStdString& str) const
{
  CStdString strUtf8;
  g_charsetConverter.stringCharsetToUtf8(str, strUtf8);

  return m_dll.id3_utf8_ucs4duplicate((id3_utf8_t*)strUtf8.c_str());
}

bool CID3Tag::Read(const CStdString& strFile)
{
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
  if (!tag.GetTitle().IsEmpty())
    tag.SetLoaded();

  tag.SetArtist(GetArtist());

  tag.SetAlbum(GetAlbum());

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

  CStdString strCoverArt, strPath;
  CUtil::GetDirectory(tag.GetURL(), strPath);
  CUtil::GetAlbumThumb(tag.GetAlbum(), strPath, strCoverArt, true);
  if (bFound)
  {
    if (!CUtil::ThumbExists(strCoverArt))
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
        if (pic.CreateAlbumThumbnailFromMemory(pPic, nBufSize, strExtension, strCoverArt))
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
  }
  else
  {
    // id3 has no cover, so add to cache
    // that it does not exist
    CUtil::ThumbCacheAdd(strCoverArt, false);
  }

  return tag.Loaded();
}

bool CID3Tag::Write(const CStdString& strFile)
{
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
  SetTrack(m_musicInfoTag.GetTrackNumber());
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
  return Ucs4ToStringCharset(m_dll.id3_metadata_getartist(m_tag));
}

CStdString CID3Tag::GetAlbum() const
{
  return Ucs4ToStringCharset(m_dll.id3_metadata_getalbum(m_tag));
}

CStdString CID3Tag::GetTitle() const
{
  return Ucs4ToStringCharset(m_dll.id3_metadata_gettitle(m_tag));
}

int CID3Tag::GetTrack() const
{
  return atoi(Ucs4ToStringCharset(m_dll.id3_metadata_gettrack(m_tag)));
}

int CID3Tag::GetPartOfSet() const
{
  return atoi(Ucs4ToStringCharset(m_dll.id3_metadata_getpartofset(m_tag)));
}

CStdString CID3Tag::GetYear() const
{
  return Ucs4ToStringCharset(m_dll.id3_metadata_getyear(m_tag));
}

CStdString CID3Tag::GetGenre() const
{
  return ParseMP3Genre(Ucs4ToStringCharset(m_dll.id3_metadata_getgenre(m_tag)));
}

CStdString CID3Tag::GetComment() const
{
  return Ucs4ToStringCharset(m_dll.id3_metadata_getcomment(m_tag));
}

CStdString CID3Tag::GetEncodedBy() const
{
  return Ucs4ToStringCharset(m_dll.id3_metadata_getencodedby(m_tag));
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
  return Ucs4ToStringCharset(m_dll.id3_metadata_getusertext(m_tag, strDescription.c_str()));
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

CStdString CID3Tag::ParseMP3Genre(const CStdString& str) const
{
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
      if (CUtil::IsNaturalNumber(t))
      {
        char * pEnd;
        long l = strtol(t.c_str(), &pEnd, 0);

        id3_ucs4_t* ucs4=m_dll.id3_latin1_ucs4duplicate((id3_latin1_t*)t.c_str());
        const id3_ucs4_t* genre=m_dll.id3_genre_name(ucs4);
        m_dll.id3_ucs4_free(ucs4);
        t=Ucs4ToStringCharset(genre);
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
      strGenre += " / ";
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

bool CID3Tag::LoadDLL()
{
  if (m_bDllLoaded)
    return true;

  DllLoader* pDll=CSectionLoader::LoadDLL(ID3_DLL);

  bool bResult = (
                    /* file interface */
                    pDll->ResolveExport("id3_file_open", (void**)&m_dll.id3_file_open) && 
                    pDll->ResolveExport("id3_file_fdopen", (void**)&m_dll.id3_file_fdopen) && 
                    pDll->ResolveExport("id3_file_close", (void**)&m_dll.id3_file_close) && 
                    pDll->ResolveExport("id3_file_tag", (void**)&m_dll.id3_file_tag) && 
                    pDll->ResolveExport("id3_file_update", (void**)&m_dll.id3_file_update) && 

                    /* tag interface */
                    pDll->ResolveExport("id3_tag_new", (void**)&m_dll.id3_tag_new) && 
                    pDll->ResolveExport("id3_tag_delete", (void**)&m_dll.id3_tag_delete) && 
                    pDll->ResolveExport("id3_tag_version", (void**)&m_dll.id3_tag_version) && 
                    pDll->ResolveExport("id3_tag_options", (void**)&m_dll.id3_tag_options) && 
                    pDll->ResolveExport("id3_tag_setlength", (void**)&m_dll.id3_tag_setlength) && 
                    pDll->ResolveExport("id3_tag_clearframes", (void**)&m_dll.id3_tag_clearframes) && 
                    pDll->ResolveExport("id3_tag_attachframe", (void**)&m_dll.id3_tag_attachframe) && 
                    pDll->ResolveExport("id3_tag_detachframe", (void**)&m_dll.id3_tag_detachframe) && 
                    pDll->ResolveExport("id3_tag_findframe", (void**)&m_dll.id3_tag_findframe) && 
                    pDll->ResolveExport("id3_tag_query", (void**)&m_dll.id3_tag_query) && 
                    pDll->ResolveExport("id3_tag_parse", (void**)&m_dll.id3_tag_parse) && 
                    pDll->ResolveExport("id3_tag_render", (void**)&m_dll.id3_tag_render) && 

                    /* frame interface */
                    pDll->ResolveExport("id3_frame_new", (void**)&m_dll.id3_frame_new) && 
                    pDll->ResolveExport("id3_frame_delete", (void**)&m_dll.id3_frame_delete) && 
                    pDll->ResolveExport("id3_frame_field", (void**)&m_dll.id3_frame_field) && 

                    /* field interface */
                    pDll->ResolveExport("id3_field_type", (void**)&m_dll.id3_field_type) && 
                    pDll->ResolveExport("id3_field_setint", (void**)&m_dll.id3_field_setint) && 
                    pDll->ResolveExport("id3_field_settextencoding", (void**)&m_dll.id3_field_settextencoding) && 
                    pDll->ResolveExport("id3_field_setstrings", (void**)&m_dll.id3_field_setstrings) && 
                    pDll->ResolveExport("id3_field_addstring", (void**)&m_dll.id3_field_addstring) && 
                    pDll->ResolveExport("id3_field_setlanguage", (void**)&m_dll.id3_field_setlanguage) && 
                    pDll->ResolveExport("id3_field_setlatin1", (void**)&m_dll.id3_field_setlatin1) && 
                    pDll->ResolveExport("id3_field_setfulllatin1", (void**)&m_dll.id3_field_setfulllatin1) && 
                    pDll->ResolveExport("id3_field_setstring", (void**)&m_dll.id3_field_setstring) && 
                    pDll->ResolveExport("id3_field_setfullstring", (void**)&m_dll.id3_field_setfullstring) && 
                    pDll->ResolveExport("id3_field_setframeid", (void**)&m_dll.id3_field_setframeid) && 
                    pDll->ResolveExport("id3_field_setbinarydata", (void**)&m_dll.id3_field_setbinarydata) && 
                    pDll->ResolveExport("id3_field_getint", (void**)&m_dll.id3_field_getint) && 
                    pDll->ResolveExport("id3_field_gettextencoding", (void**)&m_dll.id3_field_gettextencoding) && 
                    pDll->ResolveExport("id3_field_getlatin1", (void**)&m_dll.id3_field_getlatin1) && 
                    pDll->ResolveExport("id3_field_getfulllatin1", (void**)&m_dll.id3_field_getfulllatin1) && 
                    pDll->ResolveExport("id3_field_getstring", (void**)&m_dll.id3_field_getstring) && 
                    pDll->ResolveExport("id3_field_getfullstring", (void**)&m_dll.id3_field_getfullstring) && 
                    pDll->ResolveExport("id3_field_getnstrings", (void**)&m_dll.id3_field_getnstrings) && 
                    pDll->ResolveExport("id3_field_getstrings", (void**)&m_dll.id3_field_getstrings) && 
                    pDll->ResolveExport("id3_field_getframeid", (void**)&m_dll.id3_field_getframeid) && 
                    pDll->ResolveExport("id3_field_getbinarydata", (void**)&m_dll.id3_field_getbinarydata) && 

                    /* genre interface */
                    pDll->ResolveExport("id3_genre_index", (void**)&m_dll.id3_genre_index) && 
                    pDll->ResolveExport("id3_genre_name", (void**)&m_dll.id3_genre_name) && 
                    pDll->ResolveExport("id3_genre_number", (void**)&m_dll.id3_genre_number) && 

                    /* ucs4 interface */
                    pDll->ResolveExport("id3_ucs4_latin1duplicate", (void**)&m_dll.id3_ucs4_latin1duplicate) && 
                    pDll->ResolveExport("id3_ucs4_utf16duplicate", (void**)&m_dll.id3_ucs4_utf16duplicate) && 
                    pDll->ResolveExport("id3_ucs4_utf8duplicate", (void**)&m_dll.id3_ucs4_utf8duplicate) && 
                    pDll->ResolveExport("id3_ucs4_putnumber", (void**)&m_dll.id3_ucs4_putnumber) && 
                    pDll->ResolveExport("id3_ucs4_getnumber", (void**)&m_dll.id3_ucs4_getnumber) && 
                    pDll->ResolveExport("id3_ucs4_free", (void**)&m_dll.id3_ucs4_free) && 

                    /* latin1/utf16/utf8 interfaces */
                    pDll->ResolveExport("id3_latin1_ucs4duplicate", (void**)&m_dll.id3_latin1_ucs4duplicate) && 
                    pDll->ResolveExport("id3_utf16_ucs4duplicate", (void**)&m_dll.id3_utf16_ucs4duplicate) && 
                    pDll->ResolveExport("id3_utf8_ucs4duplicate", (void**)&m_dll.id3_utf8_ucs4duplicate) && 
                    pDll->ResolveExport("id3_latin1_free", (void**)&m_dll.id3_latin1_free) && 
                    pDll->ResolveExport("id3_utf16_free", (void**)&m_dll.id3_utf16_free) && 
                    pDll->ResolveExport("id3_utf8_free", (void**)&m_dll.id3_utf8_free) && 

                    /* metadata interface */
                    pDll->ResolveExport("id3_metadata_getartist", (void**)&m_dll.id3_metadata_getartist) && 
                    pDll->ResolveExport("id3_metadata_getalbum", (void**)&m_dll.id3_metadata_getalbum) && 
                    pDll->ResolveExport("id3_metadata_gettitle", (void**)&m_dll.id3_metadata_gettitle) && 
                    pDll->ResolveExport("id3_metadata_gettrack", (void**)&m_dll.id3_metadata_gettrack) && 
                    pDll->ResolveExport("id3_metadata_getpartofset", (void**)&m_dll.id3_metadata_getpartofset) && 
                    pDll->ResolveExport("id3_metadata_getyear", (void**)&m_dll.id3_metadata_getyear) && 
                    pDll->ResolveExport("id3_metadata_getgenre", (void**)&m_dll.id3_metadata_getgenre) && 
                    pDll->ResolveExport("id3_metadata_getcomment", (void**)&m_dll.id3_metadata_getcomment) && 
                    pDll->ResolveExport("id3_metadata_getencodedby", (void**)&m_dll.id3_metadata_getencodedby) && 
                    pDll->ResolveExport("id3_metadata_haspicture", (void**)&m_dll.id3_metadata_haspicture) && 
                    pDll->ResolveExport("id3_metadata_getpicturemimetype", (void**)&m_dll.id3_metadata_getpicturemimetype) && 
                    pDll->ResolveExport("id3_metadata_getpicturedata", (void**)&m_dll.id3_metadata_getpicturedata) && 
                    pDll->ResolveExport("id3_metadata_getuniquefileidentifier", (void**)&m_dll.id3_metadata_getuniquefileidentifier) && 
                    pDll->ResolveExport("id3_metadata_getusertext", (void**)&m_dll.id3_metadata_getusertext) && 
                    pDll->ResolveExport("id3_metadata_getfirstnonstandardpictype", (void**)&m_dll.id3_metadata_getfirstnonstandardpictype) && 
                    pDll->ResolveExport("id3_metadata_setartist", (void**)&m_dll.id3_metadata_setartist) && 
                    pDll->ResolveExport("id3_metadata_setalbum", (void**)&m_dll.id3_metadata_setalbum) && 
                    pDll->ResolveExport("id3_metadata_settitle", (void**)&m_dll.id3_metadata_settitle) && 
                    pDll->ResolveExport("id3_metadata_settrack", (void**)&m_dll.id3_metadata_settrack) && 
                    pDll->ResolveExport("id3_metadata_setpartofset", (void**)&m_dll.id3_metadata_setpartofset) && 
                    pDll->ResolveExport("id3_metadata_setyear", (void**)&m_dll.id3_metadata_setyear) && 
                    pDll->ResolveExport("id3_metadata_setgenre", (void**)&m_dll.id3_metadata_setgenre) && 
                    pDll->ResolveExport("id3_metadata_setencodedby", (void**)&m_dll.id3_metadata_setencodedby));

  if (!bResult)
  {
    CLog::Log(LOGERROR, "ID3Tag: Unable to load our dll %s", ID3_DLL);
    return false;
  }

  m_bDllLoaded = true;
  return true;
}
