#pragma once

#include "IMusicInfoTagLoader.h"

#include "lib/libID3/id3.h"
#include "lib/libID3/tag.h"
#include "lib/libID3/readers.h"
#include "XIStreamReader.h"

using namespace MUSIC_INFO;

namespace MUSIC_INFO
{

class CMusicInfoTagLoaderMP3: public IMusicInfoTagLoader
{
public:
  CMusicInfoTagLoaderMP3(void);
  virtual ~CMusicInfoTagLoaderMP3();
  virtual bool Load(const CStdString& strFileName, CMusicInfoTag& tag);
  bool ReadTag(ID3_Tag& id3tag, CMusicInfoTag& tag);

protected:
  int ReadDuration(CFile& file, const ID3_Tag& id3tag);
  bool IsMp3FrameHeader(unsigned long head);
  char* GetString(const ID3_Frame *frame, ID3_FieldID fldName);
  char* GetArtist(const ID3_Tag *tag);
  char* GetAlbum(const ID3_Tag *tag);
  char* GetTitle(const ID3_Tag *tag);
  char* GetMusicBrainzTrackID(const ID3_Tag *tag);
  char* GetMusicBrainzArtistID(const ID3_Tag *tag);
  char* GetMusicBrainzAlbumID(const ID3_Tag *tag);
  char* GetMusicBrainzAlbumArtistID(const ID3_Tag *tag);
  char* GetMusicBrainzTRMID(const ID3_Tag *tag);

private:
  char* GetUniqueFileID(const ID3_Tag *tag, const CStdString& strUfidOwner);
  char* GetUserText(const ID3_Tag *tag, const CStdString& strDescription);
  CStdString ParseMP3Genre(const CStdString& str);
};
};
