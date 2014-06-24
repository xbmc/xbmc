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

CCueDocument::CCueDocument(void)
{
  m_strArtist = "";
  m_strAlbum = "";
  m_strGenre = "";
  m_iYear = 0;
  m_replayGainAlbumPeak = 0.0f;
  m_replayGainAlbumGain = 0.0f;
  m_iTotalTracks = 0;
  m_iTrack = 0;
  m_iDiscNumber = 0;
}

CCueDocument::~CCueDocument(void)
{}

////////////////////////////////////////////////////////////////////////////////////
// Function: Parse()
// Opens the .cue file for reading, and constructs the track database information
////////////////////////////////////////////////////////////////////////////////////
bool CCueDocument::Parse(const std::string &strFile)
{
  if (!m_file.Open(strFile))
    return false;

  std::string strLine;
  m_iTotalTracks = -1;
  std::string strCurrentFile = "";
  bool bCurrentFileChanged = false;
  int time;

  // Run through the .CUE file and extract the tracks...
  while (true)
  {
    if (!ReadNextLine(strLine))
      break;
    if (StringUtils::StartsWithNoCase(strLine,"INDEX 01"))
    {
      if (bCurrentFileChanged)
      {
        CLog::Log(LOGERROR, "Track split over multiple files, unsupported ('%s')", strFile.c_str());
        return false;
      }

      // find the end of the number section
      time = ExtractTimeFromIndex(strLine);
      if (time == -1)
      { // Error!
        CLog::Log(LOGERROR, "Mangled Time in INDEX 0x tag in CUE file!");
        return false;
      }
      if (m_iTotalTracks > 0)  // Set the end time of the last track
        m_Track[m_iTotalTracks - 1].iEndTime = time;

      if (m_iTotalTracks >= 0)
        m_Track[m_iTotalTracks].iStartTime = time; // start time of the next track
    }
    else if (StringUtils::StartsWithNoCase(strLine,"TITLE"))
    {
      if (m_iTotalTracks == -1) // No tracks yet
        m_strAlbum = ExtractInfo(strLine.substr(5));
      else
        m_Track[m_iTotalTracks].strTitle = ExtractInfo(strLine.substr(5));
    }
    else if (StringUtils::StartsWithNoCase(strLine,"PERFORMER"))
    {
      if (m_iTotalTracks == -1) // No tracks yet
        m_strArtist = ExtractInfo(strLine.substr(9));
      else // New Artist for this track
        m_Track[m_iTotalTracks].strArtist = ExtractInfo(strLine.substr(9));
    }
    else if (StringUtils::StartsWithNoCase(strLine,"TRACK"))
    {
      int iTrackNumber = ExtractNumericInfo(strLine.substr(5));

      m_iTotalTracks++;

      CCueTrack track;
      m_Track.push_back(track);
      m_Track[m_iTotalTracks].strFile = strCurrentFile;

      if (iTrackNumber > 0)
        m_Track[m_iTotalTracks].iTrackNumber = iTrackNumber;
      else
        m_Track[m_iTotalTracks].iTrackNumber = m_iTotalTracks + 1;

      bCurrentFileChanged = false;
    }
    else if (StringUtils::StartsWithNoCase(strLine,"REM DISCNUMBER"))
    {
      int iDiscNumber = ExtractNumericInfo(strLine.substr(14));
      if (iDiscNumber > 0)
        m_iDiscNumber = iDiscNumber;
    }
    else if (StringUtils::StartsWithNoCase(strLine,"FILE"))
    {
      // already a file name? then the time computation will be changed
      if(strCurrentFile.size() > 0)
        bCurrentFileChanged = true;

      strCurrentFile = ExtractInfo(strLine.substr(4));

      // Resolve absolute paths (if needed).
      if (strCurrentFile.length() > 0)
        ResolvePath(strCurrentFile, strFile);
    }
    else if (StringUtils::StartsWithNoCase(strLine,"REM DATE"))
    {
      int iYear = ExtractNumericInfo(strLine.substr(8));
      if (iYear > 0)
        m_iYear = iYear;
    }
    else if (StringUtils::StartsWithNoCase(strLine,"REM GENRE"))
    {
      m_strGenre = ExtractInfo(strLine.substr(9));
    }
    else if (StringUtils::StartsWithNoCase(strLine,"REM REPLAYGAIN_ALBUM_GAIN"))
      m_replayGainAlbumGain = (float)atof(strLine.substr(26).c_str());
    else if (StringUtils::StartsWithNoCase(strLine,"REM REPLAYGAIN_ALBUM_PEAK"))
      m_replayGainAlbumPeak = (float)atof(strLine.substr(26).c_str());
    else if (StringUtils::StartsWithNoCase(strLine,"REM REPLAYGAIN_TRACK_GAIN") && m_iTotalTracks >= 0)
      m_Track[m_iTotalTracks].replayGainTrackGain = (float)atof(strLine.substr(26).c_str());
    else if (StringUtils::StartsWithNoCase(strLine,"REM REPLAYGAIN_TRACK_PEAK") && m_iTotalTracks >= 0)
      m_Track[m_iTotalTracks].replayGainTrackPeak = (float)atof(strLine.substr(26).c_str());
  }

  // reset track counter to 0, and fill in the last tracks end time
  m_iTrack = 0;
  if (m_iTotalTracks >= 0)
    m_Track[m_iTotalTracks].iEndTime = 0;
  else
    CLog::Log(LOGERROR, "No INDEX 01 tags in CUE file!");
  m_file.Close();
  if (m_iTotalTracks >= 0)
  {
    m_iTotalTracks++;
  }
  return (m_iTotalTracks > 0);
}

//////////////////////////////////////////////////////////////////////////////////
// Function:GetNextItem()
// Returns the track information from the next item in the cuelist
//////////////////////////////////////////////////////////////////////////////////
void CCueDocument::GetSongs(VECSONGS &songs)
{
  for (int i = 0; i < m_iTotalTracks; i++)
  {
    CSong song;
    if ((m_Track[i].strArtist.length() == 0) && (m_strArtist.length() > 0))
      song.artist = StringUtils::Split(m_strArtist, g_advancedSettings.m_musicItemSeparator);
    else
      song.artist = StringUtils::Split(m_Track[i].strArtist, g_advancedSettings.m_musicItemSeparator);
    song.albumArtist = StringUtils::Split(m_strArtist, g_advancedSettings.m_musicItemSeparator);
    song.strAlbum = m_strAlbum;
    song.genre = StringUtils::Split(m_strGenre, g_advancedSettings.m_musicItemSeparator);
    song.iYear = m_iYear;
    song.iTrack = m_Track[i].iTrackNumber;
    if ( m_iDiscNumber > 0 )  
      song.iTrack |= (m_iDiscNumber << 16); // see CMusicInfoTag::GetDiscNumber()
    if (m_Track[i].strTitle.length() == 0) // No track information for this track!
      song.strTitle = StringUtils::Format("Track %2d", i + 1);
    else
      song.strTitle = m_Track[i].strTitle;
    song.strFileName =  m_Track[i].strFile;
    song.iStartOffset = m_Track[i].iStartTime;
    song.iEndOffset = m_Track[i].iEndTime;
    if (song.iEndOffset)
      song.iDuration = (song.iEndOffset - song.iStartOffset + 37) / 75;
    else
      song.iDuration = 0;
    // TODO: replayGain goes here
    songs.push_back(song);
  }
}

void CCueDocument::GetMediaFiles(vector<std::string>& mediaFiles)
{
  set<std::string> uniqueFiles;
  for (int i = 0; i < m_iTotalTracks; i++)
    uniqueFiles.insert(m_Track[i].strFile);

  for (set<std::string>::iterator it = uniqueFiles.begin(); it != uniqueFiles.end(); it++)
    mediaFiles.push_back(*it);
}

std::string CCueDocument::GetMediaTitle()
{
  return m_strAlbum;
}

// Private Functions start here

////////////////////////////////////////////////////////////////////////////////////
// Function: ReadNextLine()
// Returns the next non-blank line of the textfile, stripping any whitespace from
// the left.
////////////////////////////////////////////////////////////////////////////////////
bool CCueDocument::ReadNextLine(std::string &szLine)
{
  // Read the next line.
  while (m_file.ReadString(m_szBuffer, 1023)) // Bigger than MAX_PATH_SIZE, for usage with relax!
  {
    // Remove the white space at the beginning and end of the line.
    szLine = m_szBuffer;
    StringUtils::Trim(szLine);
    if (!szLine.empty())
      return true;
    // If we are here, we have an empty line so try the next line
  }
  return false;
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

