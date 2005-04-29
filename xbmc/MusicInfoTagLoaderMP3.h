#pragma once

#include "IMusicInfoTagLoader.h"

#include "lib/libID3/id3.h"
#include "lib/libID3/tag.h"
#include "lib/libID3/readers.h"
#include "XIStreamReader.h"
#include "cores/paplayer/ReplayGain.h"

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
    m_iFirstSample = 0;
    m_iLastSample = 0;
  };
  virtual ~CVBRMP3SeekHelper()
  {
    if (m_SeekOffset)
      delete m_SeekOffset;
  };

  __int64 GetByteOffset(float fTime)
  {
    if (fTime > m_fTotalDuration)
      fTime = m_fTotalDuration;
    float fOffset = (fTime / m_fTotalDuration) * m_iSeekOffsets;
    int iOffset = (int)floor(fOffset);
    if (iOffset > m_iSeekOffsets-1) iOffset = m_iSeekOffsets - 1;
    float fa = m_SeekOffset[iOffset];
    float fb = m_SeekOffset[iOffset + 1];
    return (__int64)(fa + (fb - fa) * (fOffset - iOffset));
  };
  
  __int64 GetTimeOffset(__int64 iBytes)
  {
    float fBytes = (float)iBytes;
    if (fBytes > m_SeekOffset[m_iSeekOffsets])
      fBytes = m_SeekOffset[m_iSeekOffsets];
    if (fBytes < m_SeekOffset[0])
      fBytes = m_SeekOffset[0];
    // run through our byte offsets searching for our times...
    int iOffset = 1;
    while (iOffset < m_iSeekOffsets && fBytes > m_SeekOffset[iOffset])
      iOffset++;
    // iOffset will be the last of the two offsets and will be bigger than 1.
    float fTimeOffset = (float)iOffset - 1 + (fBytes - m_SeekOffset[iOffset - 1])/(m_SeekOffset[iOffset] - m_SeekOffset[iOffset - 1]);
    float fTime = fTimeOffset / m_iSeekOffsets * m_fTotalDuration;
    return (__int64)(fTime * 1000.0f);
  };

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

  void SetSampleRange(int firstSample, int lastSample)
  { 
    m_iFirstSample = firstSample;
    m_iLastSample = lastSample;
  };
  int GetFirstSample() const { return m_iFirstSample; };
  int GetLastSample() const { return m_iLastSample; };

protected:
  float m_fTotalDuration;
  int m_iSeekOffsets;
  float *m_SeekOffset;
  int m_iFirstSample;
  int m_iLastSample;
};

class CMusicInfoTagLoaderMP3: public IMusicInfoTagLoader
{
public:
  CMusicInfoTagLoaderMP3(void);
  virtual ~CMusicInfoTagLoaderMP3();
  virtual bool Load(const CStdString& strFileName, CMusicInfoTag& tag);
  bool ReadTag(ID3_Tag& id3tag, CMusicInfoTag& tag);
  void GetSeekInfo(CVBRMP3SeekHelper &info);
  bool GetReplayGain(CReplayGain &info);

protected:
  int ReadDuration(CFile& file, const ID3_Tag& id3tag);
  bool ReadLAMETagInfo(BYTE *p);
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
  void  GetReplayGainInfo(const ID3_Tag *tag);

private:
  char* GetUniqueFileID(const ID3_Tag *tag, const CStdString& strUfidOwner);
  char* GetUserText(const ID3_Tag *tag, const CStdString& strDescription);
  CStdString ParseMP3Genre(const CStdString& str);

  CVBRMP3SeekHelper m_seekInfo;
  CReplayGain       m_replayGainInfo;
};
};
