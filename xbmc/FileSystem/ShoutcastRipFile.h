#ifndef CShoutcastRipFile_H
#define CShoutcastRipFile_H

typedef struct RecStateSt
{
  bool bRecording;
  bool bCanRecord;
  bool bTrackChanged;
  bool bFilenameSet;
  bool bStreamSet;
  bool bHasMetaData;
}
RecState;

/* prototype */
typedef struct RIP_MANAGER_INFOst RIP_MANAGER_INFO;

//if one would like to make other IFiles recordable, it would be
//great, to define a superclass for this one with differend useful
//functions for all, like FilterFileName(), Record(),...
class CShoutcastRipFile
{
public:

  CShoutcastRipFile();
  virtual ~CShoutcastRipFile();


  void SetRipManagerInfo( const RIP_MANAGER_INFO* ripInfo );
  void SetTrackname( const char* trackname );


  bool Record();
  bool CanRecord();
  void StopRecording();
  bool IsRecording();
  void Write( char *buf, unsigned long size );
  void Reset();
  void GetMusicInfoTag(MUSIC_INFO::CMusicInfoTag& tag);
protected:


private:

  void PrepareRecording( );
  void RemoveIllegalChars( char *szRemoveIllegal );
  void GetDirectoryName( char* directoryName );
  void SetFilename( const char* filePath, const char* fileName );
  void CutAfterLastChar( char* szToBeCutted, int where );
  void RemoveLastSpace( char* szToBeRemoved );


  RecState m_recState;
  int m_iTrackCount;
  char m_szFileName[1024]; //FIXME: i think, that length could be optimized here
  char m_szFilteredFileName[1024];
  char m_szStreamName[1024];
  FILE* m_ripFile;
  FILE* m_logFile;
  MUSIC_INFO::CMusicInfoTag m_Tag;
};


#endif
