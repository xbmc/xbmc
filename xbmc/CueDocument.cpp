/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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
}

CCueDocument::~CCueDocument(void)
{}

////////////////////////////////////////////////////////////////////////////////////
// Function: Parse()
// Opens the .cue file for reading, and constructs the track database information
////////////////////////////////////////////////////////////////////////////////////
bool CCueDocument::Parse(const CStdString &strFile)
{
  if (!m_file.Open(strFile))
    return false;

  CStdString strLine;
  m_iTotalTracks = -1;
  CStdString strCurrentFile = "";
  bool bCurrentFileChanged = false;
  int time;

  // Run through the .CUE file and extract the tracks...
  while (true)
  {
    if (!ReadNextLine(strLine))
      break;
    if (strLine.Left(8) == "INDEX 01")
    {
      if (bCurrentFileChanged)
      {
        OutputDebugString("Track split over multiple files, unsupported");
        return false;
      }

      // find the end of the number section
      time = ExtractTimeFromIndex(strLine);
      if (time == -1)
      { // Error!
        OutputDebugString("Mangled Time in INDEX 0x tag in CUE file!\n");
        return false;
      }
      if (m_iTotalTracks > 0)  // Set the end time of the last track
        m_Track[m_iTotalTracks - 1].iEndTime = time;

      if (m_iTotalTracks >= 0)
        m_Track[m_iTotalTracks].iStartTime = time; // start time of the next track
    }
    else if (strLine.Left(5) == "TITLE")
    {
      if (m_iTotalTracks == -1) // No tracks yet
        ExtractQuoteInfo(strLine, m_strAlbum);
      else if (!ExtractQuoteInfo(strLine, m_Track[m_iTotalTracks].strTitle))
      {
        // lets manage tracks titles without quotes
        CStdString titleNoQuote = strLine.Mid(5);
        titleNoQuote.TrimLeft();
        if (!titleNoQuote.IsEmpty())
        {
          g_charsetConverter.unknownToUTF8(titleNoQuote);
          m_Track[m_iTotalTracks].strTitle = titleNoQuote;
        }
      }
    }
    else if (strLine.Left(9) == "PERFORMER")
    {
      if (m_iTotalTracks == -1) // No tracks yet
        ExtractQuoteInfo(strLine, m_strArtist);
      else // New Artist for this track
        ExtractQuoteInfo(strLine, m_Track[m_iTotalTracks].strArtist);
    }
    else if (strLine.Left(5) == "TRACK")
    {
      int iTrackNumber = ExtractNumericInfo(strLine.c_str() + 5);

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
    else if (strLine.Left(4) == "FILE")
    {
      // already a file name? then the time computation will be changed
      if(strCurrentFile.size() > 0)
        bCurrentFileChanged = true;

      ExtractQuoteInfo(strLine, strCurrentFile);

      // Resolve absolute paths (if needed).
      if (strCurrentFile.length() > 0)
        ResolvePath(strCurrentFile, strFile);
    }
    else if (strLine.Left(8) == "REM DATE")
    {
      int iYear = ExtractNumericInfo(strLine.c_str() + 8);
      if (iYear > 0)
        m_iYear = iYear;
    }
    else if (strLine.Left(9) == "REM GENRE")
    {
      if (!ExtractQuoteInfo(strLine, m_strGenre))
      {
        CStdString genreNoQuote = strLine.Mid(9);
        genreNoQuote.TrimLeft();
        if (!genreNoQuote.IsEmpty())
        {
          g_charsetConverter.unknownToUTF8(genreNoQuote);
          m_strGenre = genreNoQuote;
        }
      }
    }
    else if (strLine.Left(25) == "REM REPLAYGAIN_ALBUM_GAIN")
      m_replayGainAlbumGain = (float)atof(strLine.Mid(26));
    else if (strLine.Left(25) == "REM REPLAYGAIN_ALBUM_PEAK")
      m_replayGainAlbumPeak = (float)atof(strLine.Mid(26));
    else if (strLine.Left(25) == "REM REPLAYGAIN_TRACK_GAIN" && m_iTotalTracks >= 0)
      m_Track[m_iTotalTracks].replayGainTrackGain = (float)atof(strLine.Mid(26));
    else if (strLine.Left(25) == "REM REPLAYGAIN_TRACK_PEAK" && m_iTotalTracks >= 0)
      m_Track[m_iTotalTracks].replayGainTrackPeak = (float)atof(strLine.Mid(26));
  }

  // reset track counter to 0, and fill in the last tracks end time
  m_iTrack = 0;
  if (m_iTotalTracks >= 0)
    m_Track[m_iTotalTracks].iEndTime = 0;
  else
    OutputDebugString("No INDEX 01 tags in CUE file!\n");
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
    if (m_Track[i].strTitle.length() == 0) // No track information for this track!
      song.strTitle.Format("Track %2d", i + 1);
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

void CCueDocument::GetMediaFiles(vector<CStdString>& mediaFiles)
{
  set<CStdString> uniqueFiles;
  for (int i = 0; i < m_iTotalTracks; i++)
    uniqueFiles.insert(m_Track[i].strFile);

  for (set<CStdString>::iterator it = uniqueFiles.begin(); it != uniqueFiles.end(); it++)
    mediaFiles.push_back(*it);
}

CStdString CCueDocument::GetMediaTitle()
{
  return m_strAlbum;
}

// Private Functions start here

////////////////////////////////////////////////////////////////////////////////////
// Function: ReadNextLine()
// Returns the next non-blank line of the textfile, stripping any whitespace from
// the left.
////////////////////////////////////////////////////////////////////////////////////
bool CCueDocument::ReadNextLine(CStdString &szLine)
{
  // Read the next line.
  while (m_file.ReadString(m_szBuffer, 1023)) // Bigger than MAX_PATH_SIZE, for usage with relax!
  {
    // Remove the white space at the beginning and end of the line.
    szLine = m_szBuffer;
    szLine.Trim();
    if (!szLine.empty())
      return true;
    // If we are here, we have an empty line so try the next line
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////////
// Function: ExtractQuoteInfo()
// Extracts the information in quotes from the string line, returning it in quote
////////////////////////////////////////////////////////////////////////////////////
bool CCueDocument::ExtractQuoteInfo(const CStdString &line, CStdString &quote)
{
  quote.Empty();
  int left = line.Find('\"');
  if (left < 0) return false;
  int right = line.Find('\"', left + 1);
  if (right < 0) return false;
  quote = line.Mid(left + 1, right - left - 1);
  g_charsetConverter.unknownToUTF8(quote);
  return true;
}

////////////////////////////////////////////////////////////////////////////////////
// Function: ExtractTimeFromIndex()
// Extracts the time information from the index string index, returning it as a value in
// milliseconds.
// Assumed format is:
// MM:SS:FF where MM is minutes, SS seconds, and FF frames (75 frames in a second)
////////////////////////////////////////////////////////////////////////////////////
int CCueDocument::ExtractTimeFromIndex(const CStdString &index)
{
  // Get rid of the index number and any whitespace
  CStdString numberTime = index.Mid(5);
  numberTime.TrimLeft();
  while (!numberTime.IsEmpty())
  {
    if (!isdigit(numberTime[0]))
      break;
    numberTime.erase(0, 1);
  }
  numberTime.TrimLeft();
  // split the resulting string
  CStdStringArray time;
  StringUtils::SplitString(numberTime, ":", time);
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
int CCueDocument::ExtractNumericInfo(const CStdString &info)
{
  CStdString number(info);
  number.TrimLeft();
  if (number.IsEmpty() || !isdigit(number[0]))
    return -1;
  return atoi(number.c_str());
}

////////////////////////////////////////////////////////////////////////////////////
// Function: ResolvePath()
// Determines whether strPath is a relative path or not, and if so, converts it to an
// absolute path using the path information in strBase
////////////////////////////////////////////////////////////////////////////////////
bool CCueDocument::ResolvePath(CStdString &strPath, const CStdString &strBase)
{
  CStdString strDirectory;
  URIUtils::GetDirectory(strBase, strDirectory);

  CStdString strFilename = strPath;
  URIUtils::GetFileName(strFilename);

  URIUtils::AddFileToFolder(strDirectory, strFilename, strPath);

  // i *hate* windows
  if (!CFile::Exists(strPath))
  {
    CFileItemList items;
    CDirectory::GetDirectory(strDirectory,items);
    for (int i=0;i<items.Size();++i)
    {
      if (items[i]->GetPath().Equals(strPath))
      {
        strPath = items[i]->GetPath();
        return true;
      }
    }
    CLog::Log(LOGERROR,"Could not find FILE referenced in cue, case sensitivity issue?");
    return false;
  }

  return true;
}
