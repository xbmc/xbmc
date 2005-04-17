#pragma once

#include "IMusicInfoTagLoader.h"

#include "lib/libID3/id3.h"
#include "lib/libID3/tag.h"
#include "lib/libID3/readers.h"
#include "XIStreamReader.h"

using namespace MUSIC_INFO;

namespace MUSIC_INFO
{

class CVBRMP3SeekHelper
{
public:
  CVBRMP3SeekHelper()
  {
    m_iSeekOffsets = 0;
    m_fTotalDuration = 0.0f;
    m_SeekOffset = NULL;
  };
  virtual ~CVBRMP3SeekHelper()
  {
    if (m_SeekOffset)
      delete m_SeekOffset;
  };

  int GetByteOffset(float fTime)
  {
    if (fTime > m_fTotalDuration)
      fTime = m_fTotalDuration;
    float fOffset = (fTime / m_fTotalDuration) * m_iSeekOffsets;
    int iOffset = (int)floor(fOffset);
    if (iOffset > m_iSeekOffsets-1) iOffset = m_iSeekOffsets - 1;
    float fa = m_SeekOffset[iOffset];
    float fb = m_SeekOffset[iOffset + 1];
    return (int)(fa + (fb - fa) * (fOffset - iOffset));
  }

  void SetDuration(float fDuration) { m_fTotalDuration = fDuration; };
  float GetDuration() const { return m_fTotalDuration; };

  void SetOffsets(int iSeekOffsets, const float *offsets)
  {
    m_iSeekOffsets = iSeekOffsets;
    if (m_SeekOffset) delete[] m_SeekOffset;
    m_SeekOffset = new float[m_iSeekOffsets + 1];
    for (int i = 0; i <= m_iSeekOffsets; i++)
      m_SeekOffset[i] = offsets[i];
  };

  int GetNumOffsets() const { return m_iSeekOffsets; };
  const float *GetOffsets() const { return m_SeekOffset; };

protected:
  float m_fTotalDuration;
  int m_iSeekOffsets;
  float *m_SeekOffset;
};

class CMusicInfoTagLoaderMP3: public IMusicInfoTagLoader
{
public:
  CMusicInfoTagLoaderMP3(void);
  virtual ~CMusicInfoTagLoaderMP3();
  virtual bool Load(const CStdString& strFileName, CMusicInfoTag& tag);
  bool ReadTag(ID3_Tag& id3tag, CMusicInfoTag& tag);
  bool GetSeekInfo(CVBRMP3SeekHelper &info);

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

  CVBRMP3SeekHelper m_seekInfo;
};
};
