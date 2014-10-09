#pragma once

#pragma warning(disable : 4200)

//#include <winioctl.h> // platform sdk
#include "ntddcdvd.h"
#include <fstream>

class CDVDSession
{
protected:
  HANDLE m_hDrive;

  DVD_SESSION_ID m_session;
  bool BeginSession();
  void EndSession();

  BYTE m_SessionKey[5];
  bool Authenticate();

  BYTE m_DiscKey[6], m_TitleKey[6];
  bool GetDiscKey();
  bool GetTitleKey(int lba, BYTE* pKey);

public:
  CDVDSession();
  virtual ~CDVDSession();

  bool Open(LPCTSTR path);
  void Close();

  operator HANDLE() {return m_hDrive;}
  operator DVD_SESSION_ID() {return m_session;}

  bool SendKey(DVD_KEY_TYPE KeyType, BYTE* pKeyData);
  bool ReadKey(DVD_KEY_TYPE KeyType, BYTE* pKeyData, int lba = 0);
};

class CLBAFile : private std::fstream
{
public:
  CLBAFile();
  virtual ~CLBAFile();

  bool is_open();

  bool open(LPCTSTR path);
  void close();

  int GetLength();
  int GetPosition();
  int seekg(streampos pos);
  bool read(char* buff);
};

class CVobFile : public CDVDSession
{
  // all files
  typedef struct {CStdString fn; int size;} file_t;
  std::vector<file_t> m_files;
  int m_iFile;
  int m_pos, m_size, m_offset;

  // currently opened file
  CLBAFile m_file;

  // attribs
  bool m_fDVD, m_fHasDiscKey, m_fHasTitleKey;

public:
  CVobFile();
  virtual ~CVobFile();

  bool IsDVD();
  bool HasDiscKey(BYTE* key);
  bool HasTitleKey(BYTE* key);

  bool Open(CStdString fn, std::vector<CStdString>& files /* out */); // vts ifo
  bool Open(std::vector<CStdString>& files, int offset = -1); // vts vobs, video vob offset in lba
  void Close();

  int GetLength();
  int GetPosition();
  int Seek(int pos);
  bool Read(BYTE* buff);
};
