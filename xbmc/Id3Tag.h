#include "tag.h"
#pragma once

#include "dlllibid3tag.h"

namespace MUSIC_INFO
{

class CID3Tag : public CTag
{
public:
  CID3Tag(void);
  virtual ~CID3Tag(void);
  virtual bool Read(const CStdString& strFile);
  virtual bool Write(const CStdString& strFile);

  CStdString ParseMP3Genre(const CStdString& str) const;

protected:
  bool Parse();
  void ParseReplayGainInfo();

  CStdString GetArtist() const;
  CStdString GetAlbum() const;
  CStdString GetAlbumArtist() const;
  CStdString GetTitle() const;
  int GetTrack() const;
  int GetPartOfSet() const;
  CStdString GetYear() const;
  CStdString GetGenre() const;
  CStdString GetComment() const;
  char       GetRating() const;
  CStdString GetEncodedBy() const;
  CStdString GetLyrics() const;

  bool HasPicture(id3_picture_type pictype) const;
  CStdString GetPictureMimeType(id3_picture_type pictype) const;
  const BYTE* GetPictureData(id3_picture_type pictype, id3_length_t* length) const;
  const BYTE* GetUniqueFileIdentifier(const CStdString& strOwnerIdentifier, id3_length_t* lenght) const;
  CStdString GetUserText(const CStdString& strDescription) const;
  bool GetFirstNonStandardPictype(id3_picture_type* pictype) const;

  void SetArtist(const CStdString& strValue);
  void SetAlbum(const CStdString& strValue);
  void SetAlbumArtist(const CStdString& strValue);
  void SetTitle(const CStdString& strValue);
  void SetTrack(int n);
  void SetPartOfSet(int n);
  void SetYear(const CStdString& strValue);
  void SetGenre(const CStdString& strValue);
  void SetEncodedBy(const CStdString& strValue);
  void SetComment(const CStdString& strValue);
  void SetRating(char rating);

  CStdString ToStringCharset(const id3_ucs4_t* ucs4, id3_field_textencoding encoding) const;
  id3_ucs4_t* StringCharsetToUcs4(const CStdString& str) const;

  mutable DllLibID3Tag m_dll;

  id3_tag* m_tag;
};
};
