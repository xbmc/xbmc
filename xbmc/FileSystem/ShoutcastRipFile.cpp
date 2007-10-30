
#include "stdafx.h"
#include "ShoutcastRipFile.h"
#include "../Id3Tag.h"

#define MAX_RECORDED_TRACKS 999

#if !defined(WIN32)
#define WIN32 1
#endif

#include "../lib/libshout/rip_manager.h"



CShoutcastRipFile::CShoutcastRipFile()
{
  m_recState.bRecording = false;
  m_recState.bCanRecord = false;
  m_recState.bTrackChanged = false;
  m_recState.bFilenameSet = false;
  m_recState.bStreamSet = false;
  m_iTrackCount = 1;
  m_szFileName[0] = '\0';
  m_szFilteredFileName[0] = '\0';
  m_szStreamName[0] = '\0';
  m_ripFile = NULL;
}

CShoutcastRipFile::~CShoutcastRipFile()
{
  m_recState.bRecording = false;
  m_recState.bCanRecord = false;
  m_recState.bTrackChanged = false;
  m_recState.bFilenameSet = false;
  m_recState.bStreamSet = false;
  m_iTrackCount = 1;
  m_szFileName[0] = '\0';
  m_szFilteredFileName[0] = '\0';
  m_szStreamName[0] = '\0';
  if ( m_ripFile != NULL )
  {
    fclose( m_ripFile );
    m_ripFile = NULL;
  }
  if ( m_logFile != NULL )
  {
    fclose( m_logFile );
    m_logFile = NULL;
  }
}

void CShoutcastRipFile::Reset()
{
  m_recState.bRecording = false;
  m_recState.bCanRecord = false;
  m_recState.bTrackChanged = false;
  m_recState.bFilenameSet = false;
  m_recState.bStreamSet = false;
  m_iTrackCount = 1;
  m_szFileName[0] = '\0';
  m_szFilteredFileName[0] = '\0';
  m_szStreamName[0] = '\0';
  if ( m_ripFile != NULL )
  {
    fclose( m_ripFile );
    m_ripFile = NULL;
  }
  if ( m_logFile != NULL )
  {
    fclose( m_logFile );
    m_logFile = NULL;
  }
}

//SetRipManagerInfo() will be called frequently, depending on meta size
void CShoutcastRipFile::SetRipManagerInfo( const RIP_MANAGER_INFO* ripInfo )
{
  RIP_MANAGER_INFO info;
  memcpy(&info, ripInfo, sizeof(info));
  if ( m_recState.bFilenameSet && m_recState.bStreamSet ) //wait for RM to give all info needed
  {
    //we got everything we need
    //look if we have some metainformation (change of trackname)
    //RM returns Trackname like StreamName, if there is no Meta-Info
    //(or maybe he is not capable sometimes?!?)
    if ( strcmp( m_szFileName, m_szStreamName ) != 0 )
      m_recState.bHasMetaData = true;
    else
      m_recState.bHasMetaData = false;
    m_recState.bCanRecord = true;  //now we are ready for recording...
  }
  else
  {
    //put relevant data to members
    if ( !m_recState.bFilenameSet )  //check for filename
    {
      strcpy( m_szFileName, info.filename );
      if ( m_szFileName[0] != '\0' ) //recheck, just to be sure...
      {
        m_recState.bFilenameSet = true;
      }
    }
    if ( !m_recState.bStreamSet ) //and for stream name
    {
      strcpy( m_szStreamName, info.streamname );
      if ( m_szStreamName[0] != '\0' ) //recheck, just to be sure...
      {
        m_recState.bStreamSet = true;
      }
    }
  }
}

bool CShoutcastRipFile::Record()
{
  //open file for logging now
  if ( m_logFile == NULL ) //will be done only the first time or if path not set!
  {
    char logFilename[1024];
    CStdString strHomePath = g_guiSettings.GetString("mymusic.recordingpath");
    sprintf(logFilename, "%s\\recordings.log", strHomePath.c_str() );
    m_logFile = fopen( logFilename, "at+");
  }

  PrepareRecording();
  m_ripFile = fopen( m_szFilteredFileName, "wb+" );

  char logRecording[2048];
  //we log this, for users that want to change id3 tags afterward...
  sprintf(logRecording, "%s     ( StreamName: %s )     ( TrackName: %s )\n",
          m_szFilteredFileName,
          m_szStreamName,
          m_szFileName );
  if ( m_logFile != NULL )
  {
    fwrite(logRecording, strlen(logRecording), 1, m_logFile);
    fflush( m_logFile );     //Flush, if file isn't proberly closed afterwards (when user turns off xbox)
  }
  //  XTRACE(logRecording);
  if ( m_ripFile != NULL )
  {
    m_recState.bRecording = true;
  }
  else
  {
    m_recState.bRecording = false;
  }

  return m_recState.bRecording;
}

bool CShoutcastRipFile::CanRecord()
{

  return m_recState.bCanRecord;
}

void CShoutcastRipFile::StopRecording()
{
  m_recState.bRecording = false; //stop writing, so we can write ID3 info
  if (m_ripFile)
  {
    fclose(m_ripFile);
    m_ripFile = NULL;
    // Write collected ID3 Data to file
    CID3Tag id3tag;
    id3tag.SetMusicInfoTag(m_Tag);
    id3tag.Write(m_szFilteredFileName);
  }
}

bool CShoutcastRipFile::IsRecording()
{
  return m_recState.bRecording;
}

void CShoutcastRipFile::Write( char *buf, unsigned long size )
{
  if ( m_recState.bRecording && m_ripFile != NULL )
  {
    fwrite(buf, size, 1, m_ripFile);
  }
}

void CShoutcastRipFile::SetTrackname( const char *trackName )
{
  if ( m_recState.bRecording && m_recState.bHasMetaData)  //if we are already recording, swap the file
  {
    StopRecording();
    memset( m_szFileName, '\0', sizeof(m_szFileName)); //clear buffer
    strcpy( m_szFileName, trackName ); //swap
    m_iTrackCount++;
    Record();
  }
  else
  {
    memset( m_szFileName, '\0', sizeof(m_szFileName)); //clear buffer
    strcpy( m_szFileName, trackName ); //swap only filenames
    if ( strcmp( m_szFileName, m_szStreamName ) != 0 ) //sometimes it takes a moment for RM to get the real TrackName
      m_recState.bHasMetaData = true;
  }
}

void CShoutcastRipFile::PrepareRecording( )
{
  //"init"
  m_Tag.Clear();
  memset( m_szFilteredFileName, '\0', sizeof(m_szFilteredFileName)); //clear buffer


  //Get the directory
  //first copy the fileName to preserve info of the original RM message
  strcpy(m_szFilteredFileName, m_szFileName );
  char directoryName[1124];
  GetDirectoryName(directoryName);
  RemoveIllegalChars( directoryName );



  //sometimes it happens, that .mp3 is already at the end, then strip it
  int lastFour = strlen( m_szFilteredFileName ) - 4;
  if ( strcmp( &m_szFilteredFileName[lastFour], ".mp3") == 0 )
  {
    m_szFilteredFileName[lastFour] = '\0';
  }
  RemoveLastSpace( m_szFilteredFileName );
  RemoveIllegalChars( m_szFilteredFileName ); //first remove all unsupported chars
  //now distinguish between
  if ( m_recState.bHasMetaData )
  {
    //The filename of RM will be something like "Oasis - Champagne Supernova", thus
    //So, we will make a file i.e "f:music\Record\Limbik Frequencies\Oasis - Champagne Supernova.mp3"

    CStdString strHomePath = g_guiSettings.GetString("mymusic.recordingpath");
    char szFilePath[1024];
    sprintf( szFilePath, "%s\\%s", strHomePath.c_str(), directoryName );
    SetFilename( szFilePath, m_szFilteredFileName );
    //get the artist and trackname
    char szArtist[1124];
    char szTrackName[1124];
    char szTokens[1024];
    char* cursor;
    bool tokenUsed = false;
    bool foundArtist = false;
    bool foundTrackName = false;
    strcpy( szTokens, m_szFileName );
    cursor = strtok(szTokens, "-");
    while (cursor != NULL)
    {
      tokenUsed = false;
      //we will look for two strings,
      //sometimes there's a track number, wich we will throw away
      if ( !foundArtist )
      {
        if ( atoi( cursor ) == 0 )
        {
          //its a string
          strcpy(szArtist, cursor );
          foundArtist = true;
          tokenUsed = true;
        }
      }
      if ( !foundTrackName && !tokenUsed )
      {
        if ( atoi( cursor ) == 0 )
        {
          //its a string
          strcpy(szTrackName, cursor );
          foundTrackName = true;
          tokenUsed = true;
        }
      }
      cursor = strtok (NULL, "-");
    }
    if ( foundTrackName )         //lets hope we found a track name, else it will be unknown
      m_Tag.SetTitle(szTrackName);
    if ( foundArtist )          //lets hope we found an artist, else it will be unknown
      m_Tag.SetArtist(szArtist);
    m_Tag.SetAlbum(directoryName);  //Jazzmusique (Album is like Directory)
  }
  else
  {
    //here we will make a file i.e "f:music\Record\Jazzmusique\Jazzmusique - 3.mp3"
    CStdString strHomePath = g_guiSettings.GetString("mymusic.recordingpath");
    char szFilePath[1024];
    char szTitle[1124];             //i.e.

    //Set the filename
    sprintf( szFilePath, "%s\\%s", strHomePath.c_str(), directoryName );
    SetFilename( szFilePath, directoryName ); //file name like Directory

    //set the remaining tags
    char szTemp[1124];
    strcpy(szTemp, directoryName );
    strcat(szTemp, " %i");
    sprintf(szTitle, szTemp, m_iTrackCount);
    m_Tag.SetTitle(szTitle );       //Jazzmusique 3
    m_Tag.SetAlbum(directoryName );      //Jazzmusique (Album is like Directory)
    m_Tag.SetArtist("Shoutcast");     //Shoutcast
  }
}

void CShoutcastRipFile::SetFilename( const char* filePath, const char* fileName )
{
  INT i;
  WIN32_FIND_DATA wfd;
  HANDLE hFind;

  char szNewFileName[1024];
  char szTempFilePath[1024];

  const int MY_MAX_PATH = 42;

  //cut the received fileName to max.
  char szMaxFileName[MY_MAX_PATH];
  memset( szMaxFileName, '\0', sizeof(szMaxFileName)); //clear buffer
  if ( strlen( fileName ) > MY_MAX_PATH - 11 )    //-11: Don't forget .mp3 and 100 - !!!
  {
    memmove(szMaxFileName, fileName, MY_MAX_PATH - 11 ); //then copy the appropriate size
  }
  else
  {
    strcpy(szMaxFileName, fileName );       //else, just copy the string
  }

  strcpy( szTempFilePath, filePath );

  //first look if we need to create the directory
  memset(&wfd, 0, sizeof(wfd));
  hFind = FindFirstFile(szTempFilePath, &wfd);
  //lets create a directory if it doesn't exist
  if ( wfd.cFileName[0] == 0 )
  {
    CreateDirectory( szTempFilePath, NULL );
  }

  for ( i = m_iTrackCount; i <= MAX_RECORDED_TRACKS; i++ )
  {
    if ( m_recState.bHasMetaData )
    {
      strcat( szTempFilePath, "\\%i - %s.mp3" );  //will be "TRACKNUMBER - FILENAME.mp3"
      sprintf(szNewFileName, szTempFilePath, i, szMaxFileName );
    }
    else
    {
      strcat( szTempFilePath, "\\%s - %i.mp3" );  //will be "FILENAME - TRACKNUMBER.mp3"
      sprintf(szNewFileName, szTempFilePath, szMaxFileName, i );
    }
    memset(&wfd, 0, sizeof(wfd));
    hFind = FindFirstFile(szNewFileName, &wfd);
    if ( wfd.cFileName[0] == 0 )
    {
      strcpy( m_szFilteredFileName, szNewFileName );
      FindClose( hFind );
      //set the appropriate trackNumber
      m_Tag.SetTrackNumber(i);
      if ( !m_recState.bHasMetaData )
        m_iTrackCount = i;

      return ;
    }
    else
    {
      strcpy( szTempFilePath, filePath ); //file exists, reset
    }
  }
  //if its the MAX_RECORDED_TRACKS. file, we overwrite it
  strcpy( m_szFilteredFileName, szNewFileName );
  FindClose( hFind );
  return ;
}

void CShoutcastRipFile::RemoveIllegalChars( char *szRemoveIllegal )
{
  static char legalChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890!#$%&'()-@[]^_`{}~ ";
  char *cursor;
  for (cursor = szRemoveIllegal; *(cursor += strspn(cursor, legalChars)); /**/ )
    *cursor = '_';
}


//Sets the directory to a stripped stream name.
//Here is where most problems occure: It is hard to implement something fast
//which works for all streams and results in a meaningful name...
void CShoutcastRipFile::GetDirectoryName( char* directoryName )
{
  char szStrippedStreamName[30]; //max 30 chars
  memmove(szStrippedStreamName, m_szStreamName, 30);
  // First start only at the place, where the first alphabetical character occures
  //this is for example: "...::::Beatblender::::...."
  char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  int pos = strcspn (szStrippedStreamName, alphabet);;
  memmove(directoryName, szStrippedStreamName + pos, 29 - pos );
  directoryName[29] = '\0'; //the last one should be \0

  //sometimes streams look like "Streamname - Shitty info",
  //then remove that Shitty info
  CutAfterLastChar(directoryName, '-' );

  //or then theylook like "Streamname: Shitty info - other shitty info", :-(
  //then remove that Shitty info
  CutAfterLastChar(directoryName, ':' );

  RemoveLastSpace(directoryName);        //also remove the last space, else if used as path -> crash
}

void CShoutcastRipFile::CutAfterLastChar( char* szToBeCutted, int where )
{
  char * cursor;
  cursor = strchr( szToBeCutted, where );
  if ( cursor != NULL )
    szToBeCutted[cursor - szToBeCutted] = '\0';
}

void CShoutcastRipFile::RemoveLastSpace( char* szToBeRemoved )
{
  if ( szToBeRemoved[strlen(szToBeRemoved) - 1] == ' ' )
    szToBeRemoved[strlen(szToBeRemoved) - 1] = '\0';
}

void CShoutcastRipFile::GetMusicInfoTag(CMusicInfoTag& tag)
{
  if ( m_recState.bFilenameSet && m_recState.bStreamSet )
  {
    tag.Clear();
    //now distinguish between
    if ( m_recState.bHasMetaData )
    {
      //The filename of RM will be something like "Oasis - Champagne Supernova", thus
      //So, we will make a file i.e "f:music\Record\Limbik Frequencies\Oasis - Champagne Supernova.mp3"
      //get the artist and trackname
      char szArtist[1124];
      char szTrackName[1124];
      char szTokens[1024];
      char* cursor;
      bool tokenUsed = false;
      bool foundArtist = false;
      bool foundTrackName = false;
      strcpy( szTokens, m_szFileName );
      cursor = strtok(szTokens, "-");
      if (!cursor) cursor = strtok(szTokens, ",");
      while (cursor != NULL)
      {
        tokenUsed = false;
        //we will look for two strings,
        //sometimes there's a track number, wich we will throw away
        if ( !foundArtist )
        {
          if ( atoi( cursor ) == 0 )
          {
            //its a string
            strcpy(szArtist, cursor );
            foundArtist = true;
            tokenUsed = true;
          }
        }
        if ( !foundTrackName && !tokenUsed )
        {
          if ( atoi( cursor ) == 0 )
          {
            //its a string
            strcpy(szTrackName, cursor );
            foundTrackName = true;
            tokenUsed = true;
          }
        }
        cursor = strtok (NULL, "-");
        //if (!cursor) cursor=strtok(NULL,",");
      }
      if ( foundTrackName )         //lets hope we found a track name, else it will be unknown
        tag.SetTitle(szTrackName);
      else
        tag.SetTitle(m_szStreamName);
      if ( foundArtist )          //lets hope we found an artist, else it will be unknown
        tag.SetArtist(szArtist);
      tag.SetAlbum(m_szStreamName);  //Jazzmusique (Album is like Directory)
    }
    else
    {
      tag.Clear();
      tag.SetTitle(m_szStreamName);
    }

  }
}
