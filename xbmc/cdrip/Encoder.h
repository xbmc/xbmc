#ifndef _ENCODER_H
#define _ENCODER_H

#define ENC_ARTIST  11
#define ENC_TITLE   12
#define ENC_ALBUM   13
#define ENC_YEAR    14
#define ENC_COMMENT 15
#define ENC_TRACK   16
#define ENC_GENRE   17

#define WRITEBUFFER_SIZE 131072 // 128k buffer

class CEncoder
{
public:
  virtual ~CEncoder() {}
  virtual bool Init(const char* strFile, int iInChannels, int iInRate, int iInBits) = 0;
  virtual int Encode(int nNumBytesRead, BYTE* pbtStream) = 0;
  virtual bool Close() = 0;

  void SetComment(const char* str) { m_strComment = str; }
  void SetArtist(const char* str) { m_strArtist = str; }
  void SetTitle(const char* str) { m_strTitle = str; }
  void SetAlbum(const char* str) { m_strAlbum = str; }
  void SetGenre(const char* str) { m_strGenre = str; }
  void SetTrack(const char* str) { m_strTrack = str; }
  void SetYear(const char* str) { m_strYear = str; }

protected:
  bool FileCreate(const char* filename);
  bool FileClose();
  int FileWrite(LPCVOID pBuffer, DWORD iBytes);

  int WriteStream(LPCVOID pBuffer, DWORD iBytes);
  int FlushStream();

  // tag info
  CStdString m_strComment;
  CStdString m_strArtist;
  CStdString m_strTitle;
  CStdString m_strAlbum;
  CStdString m_strGenre;
  CStdString m_strTrack;
  CStdString m_strYear;

  CStdString m_strFile;

  HANDLE m_hFile;
  int m_iInChannels;
  int m_iInSampleRate;
  int m_iInBitsPerSample;

  BYTE m_btWriteBuffer[WRITEBUFFER_SIZE]; // 128k buffer for writing to disc
  DWORD m_dwWriteBufferPointer;
};

#endif // _ENCODER_H

