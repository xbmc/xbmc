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

#include "cuedocument.h"
#include "util.h"
#include "utils/CharsetConverter.h"

CCueDocument::CCueDocument(void)
{
	m_strFilePath = "";
	m_strArtist = "";
	m_strAlbum = "";
}

CCueDocument::~CCueDocument(void)
{
}

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
	int iTrackNumber = 0;
	int time;

	// Run through the .CUE file and extract the tracks...
	while(true)
	{
		if (!ReadNextLine(strLine))
			break;
		if (strLine.Left(8) == "INDEX 01")
		{
			time = ExtractTimeFromString(strLine+8);	
			if (time == -1)
			{	// Error!
				OutputDebugString("Mangled Time in INDEX 01 tag in CUE file!\n");
				return false;
			}
			if (m_iTotalTracks == -1)	// This is the first track (Can't have had a TRACK tag yet)
				m_iTotalTracks++;
			if (m_iTotalTracks > 0)  // Set the end time of the last track
				m_Track[m_iTotalTracks-1].iEndTime = time;
			// we have had a TRACK marker since the last INDEX marker, so note it down.
			if (iTrackNumber>0)
			{
				m_Track[m_iTotalTracks].iTrackNumber = iTrackNumber;
				iTrackNumber=0;
			}
			else
			{
				m_Track[m_iTotalTracks].iTrackNumber = m_iTotalTracks+1;
			}
			if (m_iTotalTracks < MAX_CUE_TRACKS)
				m_Track[m_iTotalTracks++].iStartTime = time; // start time of the next track
			else
			{	// Warning - Max tracks exceeded!
				OutputDebugString("Max Cue Tracks (99) obtained!\n");
                break;
			}
		}
		else if (strLine.Left(5)=="TITLE")
		{
			if (m_iTotalTracks == -1) // No tracks yet
				ExtractQuoteInfo(m_strAlbum, strLine+5);
			else // New Artist for this track
				ExtractQuoteInfo(m_Track[m_iTotalTracks].strTitle, strLine+5);
		}
		else if (strLine.Left(9)=="PERFORMER")
		{
			if (m_iTotalTracks == -1) // No tracks yet
				ExtractQuoteInfo(m_strArtist, strLine+9);
			else // New Artist for this track
				ExtractQuoteInfo(m_Track[m_iTotalTracks].strArtist, strLine+9);
		}
		else if (strLine.Left(5)=="TRACK")
		{
			if (m_iTotalTracks == -1) // No tracks yet
				m_iTotalTracks = 0;	// Starting the first track
			iTrackNumber = ExtractNumericInfo(strLine+5);
		}
		else if (strLine.Left(4)=="FILE")
		{
			if (m_iTotalTracks == -1)
				ExtractQuoteInfo(m_strFilePath, strLine+4);
		}
	}
	// Resolve absolute paths (if needed).
	if (m_strFilePath.length()>0)
		ResolvePath(m_strFilePath, strFile);
	// reset track counter to 0, and fill in the last tracks end time
	m_iTrack = 0;
	if (m_iTotalTracks > 0)
		m_Track[m_iTotalTracks-1].iEndTime = 0;
	else
		OutputDebugString("No INDEX 01 tags in CUE file!\n");
	m_file.Close();
	return (m_iTotalTracks > 0);
}

//////////////////////////////////////////////////////////////////////////////////
// Function:GetNextItem()
// Returns the track information from the next item in the cuelist
//////////////////////////////////////////////////////////////////////////////////
void CCueDocument::GetSongs(VECSONGS &songs)
{
	for (int i=0; i<m_iTotalTracks; i++)
	{
		CSong song;
		if ((m_Track[i].strArtist.length()==0) && (m_strArtist.length()>0))
			song.strArtist = m_strArtist;
		else
			song.strArtist = m_Track[i].strArtist;
		song.strAlbum = m_strAlbum;
		song.iTrack = m_Track[i].iTrackNumber;
		if (m_Track[i].strTitle.length()==0) // No track information for this track!
			song.strTitle.Format("Track %2d",i+1);
		else
			song.strTitle = m_Track[i].strTitle;
		song.strFileName = m_strFilePath;
		song.iStartOffset = m_Track[i].iStartTime;
		song.iEndOffset = m_Track[i].iEndTime;
		if (song.iEndOffset)
			song.iDuration = (song.iEndOffset - song.iStartOffset+37)/75;
		else
			song.iDuration = 0;
		songs.push_back(song);
	}
}

CStdString CCueDocument::GetMediaPath()
{
	return m_strFilePath;
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
	char *pos;
	// Read the next line.
	while(m_file.ReadString(m_szBuffer, 1023)) // Bigger than MAX_PATH_SIZE, for usage with relax!
	{
		// Remove the white space at the beginning of the line.
		pos = m_szBuffer;
		while (pos && (*pos==' ' || *pos =='\t' || *pos=='\n' || *pos =='\n')) pos++;
		if (pos)
		{
			szLine = pos;
			return true;
		}
		// If we are here, we have an empty line so try the next line
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////
// Function: ExtractQuoteInfo()
// Extracts the information in quotes from the string szLine, returning it in szData
// szLine is destroyed in the process
////////////////////////////////////////////////////////////////////////////////////
bool CCueDocument::ExtractQuoteInfo(CStdString &strData, const char *strLine)
{
	char szLine[1024];
	strcpy(szLine, strLine);
	char *pos = strchr(szLine, '"');
	if(pos)
	{
		char *pos2 = strrchr(szLine,'"');
		if (pos2)
		{
			*pos2 = 0x00;
			strData = &pos[1];
			return true;
		}
	}
	strData = "";
	return false;
}

////////////////////////////////////////////////////////////////////////////////////
// Function: ExtractTimeFromString()
// Extracts the time information from the string szData, returning it as a value in
// milliseconds.
// Assumed format is:
// MM:SS:FF where MM is minutes, SS seconds, and FF frames (75 frames in a second)
////////////////////////////////////////////////////////////////////////////////////
int CCueDocument::ExtractTimeFromString(const char *szData)
{
	char szTemp[1024];
	char *pos, *pos2;
	double time;
	strcpy(szTemp,szData);
	// Get rid of any whitespace
	pos = szTemp;
	while(pos && *pos == ' ') pos++;
	pos2=pos;
	while(pos2 && *pos2 >= '0' && *pos2 <= '9') pos2++;
	if(pos2)
	{
		*pos2=0x00;
		time = atoi(pos);
		pos=++pos2;
		while(pos2 && *pos2 >= '0' && *pos2 <= '9') pos2++;
		if(pos2)
		{
			*pos2=0x00;
			time = time*60+atoi(pos);
			pos = ++pos2;
			while(pos2 && *pos2 >= '0' && *pos2 <= '9') pos2++;
			if(pos2)
			{
				*pos2=0x00;
				time = time*75+atoi(pos);
				return (int)time;
			}
		}
	}
	return -1;
}

////////////////////////////////////////////////////////////////////////////////////
// Function: ExtractNumericInfo()
// Extracts the numeric info from the string szData, returning it as an integer value
////////////////////////////////////////////////////////////////////////////////////
int CCueDocument::ExtractNumericInfo(const char *szData)
{
	char szTemp[1024];
	char *pos, *pos2;
	strcpy(szTemp,szData);
	// Get rid of any whitespace
	pos = szTemp;
	while(pos && *pos == ' ') pos++;
	pos2=pos;
	while(pos2 && *pos2 >= '0' && *pos2 <= '9') pos2++;
	if(pos2)
	{
		*pos2=0x00;
		return atoi(pos);
	}
	return -1;
}

////////////////////////////////////////////////////////////////////////////////////
// Function: ResolvePath()
// Determines whether strPath is a relative path or not, and if so, converts it to an
// absolute path using the path information in strBase
////////////////////////////////////////////////////////////////////////////////////
bool CCueDocument::ResolvePath(CStdString &strPath, const CStdString &strBase)
{
	CStdString strDirectory;
	CStdString strFilename;
	CUtil::GetDirectory(strBase, strDirectory);
	if(CUtil::IsSmb(strDirectory)) {
		g_charsetConverter.stringCharsetToUtf8(strPath,strFilename);
		strPath = strFilename;
	}
	CUtil::GetQualifiedFilename(strDirectory, strPath);
	return true;
}
