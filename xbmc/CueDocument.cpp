/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

////////////////////////////////////////////////////////////////////////////////////
// Class: CueDocument
// This class handles the .cue file format.  This is produced by programs such as
// EAC and CDRwin when one extracts audio data from a CD as a continuous .WAV
// containing all the audio tracks in one big file.  The .cue file contains all the
// track and timing information.  An example file is:
//
// PERFORMER "Pink Floyd"
// TITLE "The Dark Side Of The Moon"
// FILE "The Dark Side Of The Moon.mp3" WAVE
//   TRACK 01 AUDIO
//     TITLE "Speak To Me / Breathe"
//     PERFORMER "Pink Floyd"
//     INDEX 00 00:00:00
//     INDEX 01 00:00:32
//   TRACK 02 AUDIO
//     TITLE "On The Run"
//     PERFORMER "Pink Floyd"
//     INDEX 00 03:58:72
//     INDEX 01 04:00:72
//   TRACK 03 AUDIO
//     TITLE "Time"
//     PERFORMER "Pink Floyd"
//     INDEX 00 07:31:70
//     INDEX 01 07:33:70
//
// etc.
//
// The CCueDocument class member functions extract this information, and construct
// the playlist items needed to seek to a track directly.  This works best on CBR
// compressed files - VBR files do not seek accurately enough for it to work well.
//
////////////////////////////////////////////////////////////////////////////////////

#include <cstdlib>

#include "CueDocument.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "utils/StringUtils.h"
#include "utils/CharsetConverter.h"
#include "filesystem/File.h"
#include "filesystem/Directory.h"
#include "FileItem.h"
#include "settings/AdvancedSettings.h"

#include <set>

using namespace std;
using namespace XFILE;

// Stuff for read CUE data from different sources.
class CueReader
{
public:
  virtual bool ready() const = 0;
  virtual bool ReadLine(std::string &line) = 0;
  virtual ~CueReader() {}
private:
  std::string m_sourcePath;
};

class FileReader
  : public CueReader
{
public:
  FileReader(const std::string &strFile)
  {
    m_opened = m_file.Open(strFile);
  }
  virtual bool ReadLine(std::string &line)
  {
    // Read the next line.
    while (m_file.ReadString(m_szBuffer, 1023)) // Bigger than MAX_PATH_SIZE, for usage with relax!
    {
      // Remove the white space at the beginning and end of the line.
      line = m_szBuffer;
      StringUtils::Trim(line);
      if (!line.empty())
        return true;
      // If we are here, we have an empty line so try the next line
    }
    return false;
  }
  virtual bool ready() const
  {
    return m_opened;
  }
  virtual ~FileReader()
  {
    if (m_opened)
      m_file.Close();

  }
private:
  CFile m_file;
  bool m_opened;
  char m_szBuffer[1024];
};

class BufferReader
  : public CueReader
{
public:
  BufferReader(const std::string &strContent)
    : m_data(strContent)
    , m_pos(0)
  {
  }
  virtual bool ReadLine(std::string &line)
  {
    // Read the next line.
    line.clear();
    while (m_pos < m_data.size())
    {
      // Remove the white space at the beginning of the line.
      char ch = m_data.at(m_pos++);
      if (ch == '\r' || ch == '\n') {
        StringUtils::Trim(line);
        if (!line.empty())
          return true;
      }
      else
      {
        line.push_back(ch);
      }
    }
    return false;
  }
  virtual bool ready() const
  {
    return m_data.size() > 0;
  }
private:
  std::string m_data;
  size_t m_pos;
};

CCueDocument::CCueDocument()
  : m_iYear(0)
  , m_iTrack(0)
  , m_iDiscNumber(0)
{
}

CCueDocument::~CCueDocument()
{}

////////////////////////////////////////////////////////////////////////////////////
// Function: ParseFile()
// Opens the CUE file for reading, and constructs the track database information
////////////////////////////////////////////////////////////////////////////////////
bool CCueDocument::ParseFile(const std::string &strFilePath)
{
  FileReader reader(strFilePath);
  return Parse(reader, strFilePath);
}

////////////////////////////////////////////////////////////////////////////////////
// Function: ParseTag()
// Reads CUE data from string buffer, and constructs the track database information
////////////////////////////////////////////////////////////////////////////////////
bool CCueDocument::ParseTag(const std::string &strContent)
{
  BufferReader reader(strContent);
  return Parse(reader);
}

//////////////////////////////////////////////////////////////////////////////////
// Function:GetSongs()
// Store track information into songs list.
//////////////////////////////////////////////////////////////////////////////////
void CCueDocument::GetSongs(VECSONGS &songs)
{
  for (size_t i = 0; i < m_tracks.size(); ++i)
  {
    CSong song;
    GetSong(i, song);
    songs.push_back(song);
  }
}

void CCueDocument::UpdateMediaFile(const std::string& oldMediaFile, const std::string& mediaFile)
{
  for (Tracks::iterator it = m_tracks.begin(); it != m_tracks.end(); ++it)
  {
    if (it->strFile == oldMediaFile)
      it->strFile = mediaFile;
  }
}

void CCueDocument::GetMediaFiles(vector<std::string>& mediaFiles)
{
  typedef set<std::string> TSet;
  TSet uniqueFiles;
  for (Tracks::const_iterator it = m_tracks.begin(); it != m_tracks.end(); ++it)
    uniqueFiles.insert(it->strFile);

  for (TSet::const_iterator it = uniqueFiles.begin(); it != uniqueFiles.end(); it++)
    mediaFiles.push_back(*it);
}

std::string CCueDocument::GetMediaTitle()
{
  return m_strAlbum;
}

bool CCueDocument::IsLoaded() const
{
  return !m_tracks.empty();
}

bool CCueDocument::GetSong(int aTrackNumber, CSong& aSong)
{
  if (aTrackNumber < 0 || aTrackNumber >= static_cast<int>(m_tracks.size()))
    return false;
  const CCueTrack& track = m_tracks[aTrackNumber];
  if ((track.strArtist.length() == 0) && (m_strArtist.length() > 0))
    aSong.artist = StringUtils::Split(m_strArtist, g_advancedSettings.m_musicItemSeparator);
  else
    aSong.artist = StringUtils::Split(track.strArtist, g_advancedSettings.m_musicItemSeparator);
  aSong.albumArtist = StringUtils::Split(m_strArtist, g_advancedSettings.m_musicItemSeparator);
  aSong.strAlbum = m_strAlbum;
  aSong.genre = StringUtils::Split(m_strGenre, g_advancedSettings.m_musicItemSeparator);
  aSong.iYear = m_iYear;
  aSong.iTrack = track.iTrackNumber;
  if (m_iDiscNumber > 0)
    aSong.iTrack |= (m_iDiscNumber << 16); // see CMusicInfoTag::GetDiscNumber()
  if (track.strTitle.length() == 0) // No track information for this track!
    aSong.strTitle = StringUtils::Format("Track %2d", track.iTrackNumber);
  else
    aSong.strTitle = track.strTitle;
  aSong.strFileName = track.strFile;
  aSong.iStartOffset = track.iStartTime;
  aSong.iEndOffset = track.iEndTime;
  if (aSong.iEndOffset)
    aSong.iDuration = (aSong.iEndOffset - aSong.iStartOffset + 37) / 75;
  else
    aSong.iDuration = 0;

  if (m_albumReplayGain.Valid())
    aSong.replayGain.Set(ReplayGain::ALBUM, m_albumReplayGain);

  if (track.replayGain.Valid())
    aSong.replayGain.Set(ReplayGain::TRACK, track.replayGain);
  return true;
}

// Private Functions start here

void CCueDocument::Clear()
{
  m_strArtist.clear();
  m_strAlbum.clear();
  m_strGenre.clear();
  m_iYear = 0;
  m_iTrack = 0;
  m_iDiscNumber = 0;
  m_albumReplayGain = ReplayGain::Info();
  m_tracks.clear();
}
////////////////////////////////////////////////////////////////////////////////////
// Function: Parse()
// Constructs the track database information from CUE source
////////////////////////////////////////////////////////////////////////////////////
bool CCueDocument::Parse(CueReader& reader, const std::string& strFile)
{
  Clear();
  if (!reader.ready())
    return false;

  std::string strLine;
  std::string strCurrentFile = "";
  bool bCurrentFileChanged = false;
  int time;
  int totalTracks = -1;

  // Run through the .CUE file and extract the tracks...
  while (true)
  {
    if (!reader.ReadLine(strLine))
      break;
    if (StringUtils::StartsWithNoCase(strLine, "INDEX 01"))
    {
      if (bCurrentFileChanged)
      {
        CLog::Log(LOGERROR, "Track split over multiple files, unsupported.");
        return false;
      }

      // find the end of the number section
      time = ExtractTimeFromIndex(strLine);
      if (time == -1)
      { // Error!
        CLog::Log(LOGERROR, "Mangled Time in INDEX 0x tag in CUE file!");
        return false;
      }
      if (totalTracks > 0) // Set the end time of the last track
        m_tracks[totalTracks - 1].iEndTime = time;

      if (totalTracks >= 0) // start time of the next track
        m_tracks[totalTracks].iStartTime = time;
    }
    else if (StringUtils::StartsWithNoCase(strLine, "TITLE"))
    {
      if (totalTracks == -1) // No tracks yet
        m_strAlbum = ExtractInfo(strLine.substr(5));
      else
        m_tracks[totalTracks].strTitle = ExtractInfo(strLine.substr(5));
    }
    else if (StringUtils::StartsWithNoCase(strLine, "PERFORMER"))
    {
      if (totalTracks == -1) // No tracks yet
        m_strArtist = ExtractInfo(strLine.substr(9));
      else // New Artist for this track
        m_tracks[totalTracks].strArtist = ExtractInfo(strLine.substr(9));
    }
    else if (StringUtils::StartsWithNoCase(strLine, "TRACK"))
    {
      int iTrackNumber = ExtractNumericInfo(strLine.substr(5));

      totalTracks++;

      CCueTrack track;
      m_tracks.push_back(track);
      m_tracks[totalTracks].strFile = strCurrentFile;
      if (iTrackNumber > 0)
        m_tracks[totalTracks].iTrackNumber = iTrackNumber;
      else
        m_tracks[totalTracks].iTrackNumber = totalTracks + 1;

      bCurrentFileChanged = false;
    }
    else if (StringUtils::StartsWithNoCase(strLine, "REM DISCNUMBER"))
    {
      int iDiscNumber = ExtractNumericInfo(strLine.substr(14));
      if (iDiscNumber > 0)
        m_iDiscNumber = iDiscNumber;
    }
    else if (StringUtils::StartsWithNoCase(strLine, "FILE"))
    {
      // already a file name? then the time computation will be changed
      if (!strCurrentFile.empty())
        bCurrentFileChanged = true;

      strCurrentFile = ExtractInfo(strLine.substr(4));

      // Resolve absolute paths (if needed).
      if (!strFile.empty() && !strCurrentFile.empty())
        ResolvePath(strCurrentFile, strFile);
    }
    else if (StringUtils::StartsWithNoCase(strLine, "REM DATE"))
    {
      int iYear = ExtractNumericInfo(strLine.substr(8));
      if (iYear > 0)
        m_iYear = iYear;
    }
    else if (StringUtils::StartsWithNoCase(strLine, "REM GENRE"))
    {
      m_strGenre = ExtractInfo(strLine.substr(9));
    }
    else if (StringUtils::StartsWithNoCase(strLine, "REM REPLAYGAIN_ALBUM_GAIN"))
      m_albumReplayGain.SetGain(strLine.substr(26));
    else if (StringUtils::StartsWithNoCase(strLine, "REM REPLAYGAIN_ALBUM_PEAK"))
      m_albumReplayGain.SetPeak(strLine.substr(26));
    else if (StringUtils::StartsWithNoCase(strLine, "REM REPLAYGAIN_TRACK_GAIN") && totalTracks >= 0)
      m_tracks[totalTracks].replayGain.SetGain(strLine.substr(26));
    else if (StringUtils::StartsWithNoCase(strLine, "REM REPLAYGAIN_TRACK_PEAK") && totalTracks >= 0)
      m_tracks[totalTracks].replayGain.SetPeak(strLine.substr(26));
  }

  // reset track counter to 0, and fill in the last tracks end time
  m_iTrack = 0;
  if (totalTracks >= 0)
    m_tracks[totalTracks].iEndTime = 0;
  else
    CLog::Log(LOGERROR, "No INDEX 01 tags in CUE file!");
  return (totalTracks >= 0);
}

////////////////////////////////////////////////////////////////////////////////////
// Function: ExtractInfo()
// Extracts the information in quotes from the string line, returning it in quote
////////////////////////////////////////////////////////////////////////////////////
std::string CCueDocument::ExtractInfo(const std::string &line)
{
  size_t left = line.find('\"');
  if (left != std::string::npos)
  {
    size_t right = line.find('\"', left + 1);
    if (right != std::string::npos)
    {
      std::string text = line.substr(left + 1, right - left - 1);
      g_charsetConverter.unknownToUTF8(text);
      return text;
    }
  }
  std::string text = line;
  StringUtils::Trim(text);
  g_charsetConverter.unknownToUTF8(text);
  return text;
}

////////////////////////////////////////////////////////////////////////////////////
// Function: ExtractTimeFromIndex()
// Extracts the time information from the index string index, returning it as a value in
// milliseconds.
// Assumed format is:
// MM:SS:FF where MM is minutes, SS seconds, and FF frames (75 frames in a second)
////////////////////////////////////////////////////////////////////////////////////
int CCueDocument::ExtractTimeFromIndex(const std::string &index)
{
  // Get rid of the index number and any whitespace
  std::string numberTime = index.substr(5);
  StringUtils::TrimLeft(numberTime);
  while (!numberTime.empty())
  {
    if (!StringUtils::isasciidigit(numberTime[0]))
      break;
    numberTime.erase(0, 1);
  }
  StringUtils::TrimLeft(numberTime);
  // split the resulting string
  vector<string> time = StringUtils::Split(numberTime, ":");
  if (time.size() != 3)
    return -1;

  int mins = atoi(time[0].c_str());
  int secs = atoi(time[1].c_str());
  int frames = atoi(time[2].c_str());

  return (mins*60 + secs)*75 + frames;
}

////////////////////////////////////////////////////////////////////////////////////
// Function: ExtractNumericInfo()
// Extracts the numeric info from the string info, returning it as an integer value
////////////////////////////////////////////////////////////////////////////////////
int CCueDocument::ExtractNumericInfo(const std::string &info)
{
  std::string number(info);
  StringUtils::TrimLeft(number);
  if (number.empty() || !StringUtils::isasciidigit(number[0]))
    return -1;
  return atoi(number.c_str());
}

////////////////////////////////////////////////////////////////////////////////////
// Function: ResolvePath()
// Determines whether strPath is a relative path or not, and if so, converts it to an
// absolute path using the path information in strBase
////////////////////////////////////////////////////////////////////////////////////
bool CCueDocument::ResolvePath(std::string &strPath, const std::string &strBase)
{
  std::string strDirectory = URIUtils::GetDirectory(strBase);
  std::string strFilename = URIUtils::GetFileName(strPath);

  strPath = URIUtils::AddFileToFolder(strDirectory, strFilename);

  // i *hate* windows
  if (!CFile::Exists(strPath))
  {
    CFileItemList items;
    CDirectory::GetDirectory(strDirectory,items);
    for (int i=0;i<items.Size();++i)
    {
      if (items[i]->IsPath(strPath))
      {
        strPath = items[i]->GetPath();
        return true;
      }
    }
    CLog::Log(LOGERROR,"Could not find '%s' referenced in cue, case sensitivity issue?", strPath.c_str());
    return false;
  }

  return true;
}

