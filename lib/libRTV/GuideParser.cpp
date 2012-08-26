
/*
** ReplayTV 5000
** Guide Parser
** by Lee Thompson <thompsonl@logh.net> Dan Frumin <rtv@frumin.com>, Todd Larason <jtl@molehill.org>
**
** This program is distributed in the hope that it will be useful, 
** but WITHOUT ANY WARRANTY; without even the implied warranty of 
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
** 
** If you distribute a modified version of this program, the program MUST tell the user 
** in the usage summary how the program has been modified, and who performed the modification.
** Modified source code MUST be distributed with the modified program. 
**
** For additional detail on the Replay 4000/4500/5000 internals, protocols and structures I highly recommend
** visiting Todd's site: http://www.molehill.org/twiki/bin/view/Replay/WebHome
**
***************************************************************************************************************
**
** IMPORTANT NOTE: This is the last version of GuideParser with this codebase. 
**
** GuideParser was originally just a research project to learn the Guide Snapshot format.  This research
** phase is pretty much at a close and this code is very ugly. 
**
** Others wishing to continue development of GuideParser with this code are welcome to do so under the above
** limitations.
**
***************************************************************************************************************
**
** Revision History:
** 
** 1.02 - LT   Updated for 5.0 Software
** 1.01 - LT   ReplayTV 5000
** 1.0 - LT    Final version with this codebase.
**             Organized the structures a bit better.
**             Fixed some annoying bugs.
**             Added -combined option which does a mini replayChannel lookup, best used with -ls
** .11 - LT    Whoops!  ReplayChannel had the wrong number for szThemeInfo, should've been 64 not 60! (Thanks Clem!)
**             Todd corrected a mis-assigned bitmask for TV rating "TV-G"
**             Todd's been knocking himself out with research... as a result no more unknowns in tagProgramInfo!
**             Added Genre lookups
** .10 - LT    Works on FreeBSD now (thanks Andy!)  
**             Fixed a minor display bug.   
**             Supports MPAA "G" ratings.
**             Added -d option for dumping unknowns as numbers.
**             Added -x (Xtra) option for displaying TMSIDs and so forth.   
**             Added -nowrap to supress word wrap.
**             Added STDIN support!
**             Added Input Source lookup.
**             Better organized sub-structures. 
**             Channel/Show Output now uses the word wrapper for everything.
**             Some minor tweaks.
**             Less unknowns.   (Thanks in large part to Todd)
**             Released on Mar 10th 2002
** .09 - LT    Wrote an int64 Big->Small Endian converter.   
**             UNIX/WIN32 stuff sorted out.  
**             Cleaned up some code.  
**             Dependency on stdafx.h removed.
**             Improved Word Wrapper.   
**             Fixed some bugs.
**             Less unknowns.
**             Released on Jan 13th 2002
** .08 - LT    More fixes!
**             Better wordwrapping!
**             Less unknowns!      
**             Released on Jan 10th 2002
** .07 - LT    Categories :)   
**             Dan added the extraction command line switches and code.
**             Released on Jan 8th 2002
** .06 - LT	   Timezone fixes
**             Released on Jan 5th 2002
** .05 - LT    Checking to see if guide file is damaged (some versions of ReplayPC have a problem in this area)
**			   Found a case where GuideHeader.replayshows is not accurate!   
**             Lots of other fixes.
**             Released on Jan 4th 2002
** .04 - LT    Day of Week implemented.
**             Categories are loaded but lookup isn't working.
** .03 - LT	   Private build.
** .02 - LT	   Private build.
** .01 - LT    Private build.  
**             Initial Version     
**             01/03/2002
**
**
***************************************************************************************************************
**
** TO DO:
**
** 1. Rewrite!
**
** 
** BUGS
**  Piping from replaypc to guideparser gets a few replaychannels in and somehow loses it's alignment -- looks
**  like the 'file' is being truncated.
**
***************************************************************************************************************
**  FreeBSD fixes by Andrew Tulloch <andrew@biliskner.demon.co.uk>
***************************************************************************************************************
*/

#ifdef _XBOX
#include <Xtl.h>
#pragma pack(1)                          // don't use byte alignments on the structs
#endif

#ifdef _WIN32
#if !defined(AFX_STDAFX_H__11AD8D81_EA54_4CD0_9A74_44F7170DE43F__INCLUDED_)
#define AFX_STDAFX_H__11AD8D81_EA54_4CD0_9A74_44F7170DE43F__INCLUDED_
#endif

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#pragma pack(1)                          // don't use byte alignments on the structs

#define WIN32_LEAN_AND_MEAN	

#if defined (_WIN32) && !defined(_XBOX)
#include <winsock2.h>
#endif

#include <winnt.h>
#include <winbase.h>
#endif

#if (defined(__unix__) || defined(__APPLE__))
#include <netinet/in.h>
#endif

#include <string.h>
#include <memory.h>
#include <math.h>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>

#if defined (_WIN32) && !defined(_XBOX)
#pragma comment(lib, "ws2_32.lib")      // load winsock2 (we only use it for endian conversion)
#define FILEPOS fpos_t
#endif

#ifdef __APPLE__
#include <AvailabilityMacros.h>
#endif

#if defined(__unix__) || defined(__APPLE__)
typedef unsigned char BYTE;
typedef unsigned long long DWORD64;
typedef unsigned long DWORD;
typedef unsigned short WORD;
#define FILEPOS unsigned long
#define _tzset() tzset()
#define _tzname tzname
#define _timezone timezone
#define MoveMemory(dst,src,len) memmove(dst,src,len)
#define HIWORD(x) (WORD)(x >> 16)
#endif

extern int _daylight;
#if defined(__APPLE__) && (MAC_OS_X_VERSION_MAX_ALLOWED > 1040) 
extern long _timezone;
#endif
#ifndef _WIN32
extern char *_tzname[2];
#endif

#define SHOWSTRUCTURESIZES  1           // this is for debugging...
#undef SHOWSTRUCTURESIZES		        // ...uncomment this line for structure sizes to be shown

// 5.0
#define REPLAYSHOW      512             // needed size of replayshow structure
#define GUIDEHEADER     840             // needed size of guideheader structure
#define PROGRAMINFO     272             // needed size of programinfo structure
#define CHANNELINFO     80              // needed size of channelinfo structure

// 4.5
#define GUIDEHEADER45   808             // needed size of guideheader structure

// 4.3
#define GUIDEHEADER43   808             // needed size of guideheader structure
#define REPLAYSHOW43    444				// needed size of replayshow structure

#define MAXCHANNELS     99

#define lpszTitle	    "ReplayTV 5000 (Alpha) GuideParser v1.02 (Build 17)"
#define lpszCopyright	"(C) 2002 Lee Thompson, Dan Frumin, Todd Larason, and Andrew Tulloch"
#define lpszComment     ""

//-------------------------------------------------------------------------
// Data Structures
//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
//******************** COMMENTS *******************************************
//-------------------------------------------------------------------------
/*
** General ReplayTV Notes
**
**  The RTV4K/5K platforms are capable of supporting NTSC, PAL and SECAM so
**  in theory there may be one or more bytes always set a certain way as they
**  reflect NTSC.
**
**  On the subject of recorded time: The ReplayTV units appear to 
**  automatically pad shows by 3 seconds.   e.g. If you schedule a show that
**  airs at 6:00 PM, the recording will  actually begin at 5:59:57 PM.
**  Because of this, even an unpadded show will have a 3 second discrepency
**  between ReplayShow.recorded and ReplayShow.eventtime.
**
**
** Categories
**
**  There appears to be an *internal* maximum of 32 but the units appear
**  to only use 17 total.
** 
** Secondary Offset (GuideHeader)
**
**  If you jump to this absolute location this repeats the first 12 bytes of 
**  this header, null terminates and then 4 unknown DWORDs. 
**
**  You will then be at the first byte of the first ReplayChannel  record.
**
**
** Genre Codes
**  Todd Larason's been slowly figuring these out, you can see the list at
**  http://www.molehill.org/twiki/bin/view/Replay/RnsGetCG2
**
*/

/**************************************************************************
**
** GUIDE HEADER
**
**
** Most of the  unknowns in the GuideHeader are garbage/junk.   Why 
** these aren't initialized to null I have no idea.
**
** They may also be a memory structure that has no meaning on a 'remote unit'.
**
**************************************************************************/

// 5000 OS Version 5.0
typedef struct tagGuideSnapshotHeader { 
    WORD osversion;             // OS Version (5 for 4.5, 3 for 4.3, 0 on 5.0)
    WORD snapshotversion;       // Snapshot Version (1) (2 on 5.0)    This might be better termed as snapshot version
    DWORD structuresize;		// Should always be 64
    DWORD unknown1;		        // 0x00000002
    DWORD unknown2;             // 0x00000005
    DWORD channelcount;		    // Number of Replay Channels
    DWORD channelcountcheck;    // Should always be equal to channelcount, it's incremented as the snapshot is built.
    DWORD unknown3;             // 0x00000000
    DWORD groupdataoffset;		// Offset of Group Data
    DWORD channeloffset;		// Offset of First Replay Channel  (If you don't care about categories, jump to this)
    DWORD showoffset;			// Offset of First Replay Show (If you don't care about ReplayChannels, jump to this)
    DWORD snapshotsize;         // Total Size of the Snapshot
    DWORD unknown4;             // 0x00000001
    DWORD unknown5;             // 0x00000004
    DWORD flags;	    		// Careful, this is uninitialized, ignore invalid bits
    DWORD unknown6;             // 0x00000002
    DWORD unknown7;             // 0x00000000
} GuideSnapshotHeader;

typedef struct tagGroupData {
    DWORD structuresize;	    // Should always be 776
    DWORD categories;			// Number of Categories (MAX: 32 [ 0 - 31 ] )
    DWORD category[32];			// category lookup 2 ^ number, position order = text
    DWORD categoryoffset[32];	// Offsets for the GuideHeader.catbuffer
    char catbuffer[512];		// GuideHeader.categoryoffset contains the starting position of each category.
} GroupData;

typedef struct tagGuideHeader {
    struct tagGuideSnapshotHeader GuideSnapshotHeader;
    struct tagGroupData GroupData;
} GuideHeader;

// 4000 OS Version 4.3 / 5000 OS Version 4.5
typedef struct tagGuideSnapshotHeader45 { 
    WORD osversion;             // Major Revision (5 for Replay5000s, 3 for Replay 4000/4500s)
    WORD snapshotversion;       // Minor Revision (1)
    DWORD structuresize;		// Should always be 32
    DWORD channelcount;			// Number of Replay Channels
    DWORD channelcountcheck;	// Number of Replay Channels (Copy 2; should always match)
    DWORD groupdataoffset;		// Offset of Group Data
    DWORD channeloffset;		// Offset of First Replay Channel  (If you don't care about categories, jump to this)
    DWORD showoffset;			// Offset of First Replay Show (If you don't care about ReplayChannels, jump to this)
    DWORD flags;	    		// Careful, this is uninitialized, ignore invalid bits
} GuideSnapshotHeader45; 

// GroupData structure is the same for 4.3/4.5 and 5.0

typedef struct tagGuideHeader45 {
    struct tagGuideSnapshotHeader45 GuideSnapshotHeader;
    struct tagGroupData GroupData;
} GuideHeader45;

/*******************************************************************************
**
** SUB-STRUCTURES
**
*******************************************************************************/

//-------------------------------------------------------------------------
// Movie Extended Info Structure

typedef struct tagMovieInfo {
    WORD mpaa;                 		// MPAA Rating
    WORD stars;                		// Star Rating * 10  (i.e. value of 20 = 2 stars)
    WORD year;                 		// Release Year
    WORD runtime;              		// Strange HH:MM format
} MovieInfo;


//-------------------------------------------------------------------------
// Has Parts Info Structure

typedef struct tagPartsInfo {
    WORD partnumber;           		// Part X [Of Y]
    WORD maxparts;            		// [Part X] Of Y
} PartsInfo;


//--------------------------------------------------------------------------
// Program Info Structure

typedef struct tagProgramInfo { 
    DWORD structuresize;		// Should always be 272 (0x0110)
    DWORD autorecord;			// If non-zero it is an automatic recording, otherwise it is a manual recording.
    DWORD isvalid;		    	// Not sure what it actually means! (should always be 1 in exported guide)
    DWORD tuning;			    // Tuning (Channel Number) for the Program
    DWORD flags;		    	// Program Flags 
    DWORD eventtime;			// Scheduled Time of the Show
    DWORD tmsID;			    // Tribune Media Services ID (inherited from ChannelInfo)
    WORD minutes;			    // Minutes (add with show padding for total)
    BYTE genre1;                // Genre Code 1
    BYTE genre2;                // Genre Code 2
    BYTE genre3;                // Genre Code 3
    BYTE genre4;                // Genre Code 4
    WORD recLen;      	        // Record Length of Description Block
    BYTE titleLen;			    // Length of Title
    BYTE episodeLen;		    // Length of Episode
    BYTE descriptionLen;		// Length of Description
    BYTE actorLen;			    // Length of Actors
    BYTE guestLen;			    // Length of Guest
    BYTE suzukiLen;			    // Length of Suzuki String (Newer genre tags)
    BYTE producerLen;			// Length of Producer
    BYTE directorLen;			// Length of Director
    char szDescription[228];	// This can have parts/movie sub-structure
} ProgramInfo;	


//--------------------------------------------------------------------------
// Channel Info Structure

typedef struct tagChannelInfo {
    DWORD structuresize;		// Should always be 80 (0x50)
    DWORD usetuner;			    // If non-zero the tuner is used, otherwise it's a direct input.
    DWORD isvalid;			    // Record valid if non-zero
    DWORD tmsID;			    // Tribune Media Services ID
    WORD channel;			    // Channel Number
    BYTE device;			    // Device
    BYTE tier;                  // Cable/Satellite Service Tier
    char szChannelName[16];		// Channel Name
    char szChannelLabel[32];	// Channel Description
    char cablesystem[8];		// Cable system ID
    DWORD channelindex;			// Channel Index (USUALLY tagChannelinfo.channel repeated)
} ChannelInfo;


/*******************************************************************************
**
** REPLAY SHOW
**
*******************************************************************************/

typedef struct tagReplayShow { 
    DWORD created;			        	// ReplayChannel ID (tagReplayChannel.created)
    DWORD recorded;			        	// Filename/Time of Recording (aka ShowID)
    DWORD inputsource;		        	// Replay Input Source
    DWORD quality;			         	// Recording Quality Level
    DWORD guaranteed;		    		// (0xFFFFFFFF if guaranteed)
    DWORD playbackflags;	    		// Not well understood yet.
    struct tagChannelInfo ChannelInfo;
    struct tagProgramInfo ProgramInfo;
    DWORD ivsstatus;		    		// Always 1 in a snapshot outside of a tagReplayChannel
    DWORD guideid;		        		// The show_id on the original ReplayTV for IVS shows.  Otherwise 0
    DWORD downloadid;   	    		// Valid only during actual transfer of index or mpeg file; format/meaning still unknown 
    DWORD timessent;		    		// Times sent using IVS
    DWORD seconds;		        		// Show Duration in Seconds (this is the exact actual length of the recording)
    DWORD GOP_count;		    		// MPEG Group of Picture Count
    DWORD GOP_highest;	     			// Highest GOP number seen, 0 on a snapshot.
    DWORD GOP_last;		         		// Last GOP number seen, 0 on a snapshot.
    DWORD checkpointed;	     			// 0 in possibly-out-of-date in-memory copies; always -1 in snapshots 
    DWORD intact;				        // 0xffffffff in a snapshot; 0 means a deleted show 
    DWORD upgradeflag;	    		    // Always 0 in a snapshot
    DWORD instance;		    		    // Episode Instance Counter (0 offset)
    WORD unused;	            		// Not preserved when padding values are set, presumably not used 
    BYTE beforepadding;	    			// Before Show Padding
    BYTE afterpadding;	    			// After Show Padding
    DWORD64 indexsize;	          		// Size of NDX file (IVS Shows Only)
    DWORD64 mpegsize;	    			// Size of MPG file (IVS Shows Only)
    char szReserved[68];	    		// Show Label (NOT PRESENT in 4k 4.3 version, work around it)
} ReplayShow;


//-------------------------------------------------------------------------
// WIN32 Functions
//-------------------------------------------------------------------------

#ifdef _WIN32

//-------------------------------------------------------------------------
void UnixTimeToFileTime(time_t t, LPFILETIME pft)
{
    LONGLONG ll;
    
    ll = Int32x32To64(t, 10000000) + 116444736000000000;
    pft->dwLowDateTime = (DWORD)ll;
    pft->dwHighDateTime = (DWORD)(ll >> 32);
}


//-------------------------------------------------------------------------
void UnixTimeToSystemTime(time_t t, LPSYSTEMTIME pst)
{
    FILETIME ft;
    
    UnixTimeToFileTime(t, &ft);
    FileTimeToSystemTime(&ft, pst);
}

//-------------------------------------------------------------------------
char * UnixTimeToString(time_t t)
{
    static char sbuf[17];
    char szTimeZone[32];
    SYSTEMTIME st;
    int tzbias;
    
    _tzset();
    
    memset(szTimeZone,0,sizeof(szTimeZone));
    
    MoveMemory(szTimeZone,_tzname[0],1);		
    strcat(szTimeZone,"T");
   
#if (MAC_OS_X_VERSION_MAX_ALLOWED <= 1040) && !defined(_WIN32)
    struct timezone tz;
    gettimeofday(NULL, &tz);
    tzbias = tz.tz_minuteswest;
#else
    tzbias = _timezone;
#endif
    
    UnixTimeToSystemTime(t - tzbias, &st);
    sprintf(sbuf, "%d-%.2d-%.2d %.2d:%.2d:%.2d",
        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    return sbuf;
}


#endif

//-------------------------------------------------------------------------
// UNIX Functions
//-------------------------------------------------------------------------

#if defined(__unix__) || defined(__APPLE__)
char * UnixTimeToString(time_t t)
{
    static char sbuf[17];
    struct tm * tm;
    
    tm = localtime(&t);
    strftime(sbuf, sizeof(sbuf), "%Y-%m-%d %H:%M:%S", tm);
    return sbuf;
}
#endif


//-------------------------------------------------------------------------
// Common Functions
//-------------------------------------------------------------------------


//-------------------------------------------------------------------------
// Lookup Tables
//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
void DisplayQuality(int quality, char *szQuality)
{
    switch( quality )	
    {
    case 0:
        strcpy( szQuality, "High" );
        break;
    case 1:
        strcpy( szQuality, "Medium" );
        break;
    case 2:
        strcpy( szQuality, "Standard" );
        break;
    default:
        strcpy( szQuality, "" );
        break;
    }
}

//-------------------------------------------------------------------------
void DisplayExtendedTVRating(int tvrating, char *szRating, bool append)
{

    if (append == false)
    {
        strcpy( szRating, "");
    }
    
    if (tvrating &  0x00020000)
    {
        if (strlen(szRating) > 0)
        {
            strcat( szRating, " ");
        }


        strcat( szRating, "S" );    // Sex
    }
    
    if (tvrating &  0x00040000)
    {
        if (strlen(szRating) > 0)
        {
            strcat( szRating, " ");
        }


        strcat( szRating, "V" );    // Violence
    }
    
    if (tvrating &  0x00080000)
    {
        if (strlen(szRating) > 0)
        {
            strcat( szRating, " ");
        }


        strcat( szRating, "L" );    // Language
    }
    
    if (tvrating &  0x00100000)
    {
        if (strlen(szRating) > 0)
        {
            strcat( szRating, " ");
        }


        strcat( szRating, "D" );    // Drug Use
    }
    
    if (tvrating &  0x00200000)
    {
        if (strlen(szRating) > 0)
        {
            strcat( szRating, " ");
        }


        strcat( szRating, "F" );    // Fantasy Violence
    }
    
}

//-------------------------------------------------------------------------
void DisplayTVRating(int tvrating, char *szRating)
{
    strcpy( szRating, "");
    
    if (tvrating &  0x00008000)
    {
        strcpy( szRating, "TV-Y" );
    }
    
    if (tvrating &  0x00010000)
    {
        strcpy( szRating, "TV-Y7" );
    }
    
    if (tvrating &  0x00001000)
    {
        strcpy( szRating, "TV-G" );
    }
    
    if (tvrating &  0x00004000)
    {
        strcpy( szRating, "TV-PG" );
    }
    
    if (tvrating &  0x00002000)
    {
        strcpy( szRating, "TV-MA" );
    }
    
    if (tvrating &  0x00000800)
    {
        strcpy( szRating, "TV-14" );
    }
    
}

//-------------------------------------------------------------------------
void DisplayExtendedMPAARating(int mpaarating, char *szRating, bool append)
{
    if (append == false)
    {
        strcpy( szRating, "");
    }
    
    if (mpaarating & 0x00400000)
    {
        if (strlen(szRating) > 0)
        {
            strcat( szRating, " ");
        }

        strcat( szRating, "AC" );    // Adult Content
    }

    if (mpaarating & 0x00800000)
    {
        if (strlen(szRating) > 0)
        {
            strcat( szRating, " ");
        }

        strcat( szRating, "BN" );   // Brief Nudity
    }

    if (mpaarating & 0x01000000)
    {
        if (strlen(szRating) > 0)
        {
            strcat( szRating, " ");
        }

        strcat( szRating, "GL" );   // Graphic Language
    }

    if (mpaarating & 0x02000000)
    {
        if (strlen(szRating) > 0)
        {
            strcat( szRating, " ");
        }

        strcat( szRating, "GV" );   // Graphic Violence
    }

    if (mpaarating & 0x04000000)
    {
        if (strlen(szRating) > 0)
        {
            strcat( szRating, " ");
        }

        strcat( szRating, "AL" );   // Adult Language
    }

    if (mpaarating & 0x08000000)
    {
        if (strlen(szRating) > 0)
        {
            strcat( szRating, " ");
        }

        strcat( szRating, "MV" );   // Mild Violence
    }

    if (mpaarating & 0x10000000)
    {
        if (strlen(szRating) > 0)
        {
            strcat( szRating, " ");
        }

        strcat( szRating, "N" );    // Nudity
    }

    if (mpaarating & 0x20000000)
    {
        if (strlen(szRating) > 0)
        {
            strcat( szRating, " ");
        }

        strcat( szRating, "RP" );   // Rape
    }

    if (mpaarating & 0x40000000)
    {
        if (strlen(szRating) > 0)
        {
            strcat( szRating, " ");
        }

        strcat( szRating, "SC " );   // Sexual Content
    }

    if (mpaarating & 0x80000000)
    {
        if (strlen(szRating) > 0)
        {
            strcat( szRating, " ");
        }

        strcat( szRating, "V" );    // Violence
    }

}


//-------------------------------------------------------------------------
void DisplayMPAARating(int mpaarating, char *szRating)
{
    strcpy( szRating, "");

    if (mpaarating & 0x00000001)
    {
        strcat( szRating, "AO" );       // unverified
    }

    if (mpaarating & 0x00000002)
    {
        strcat( szRating, "G" );   
    }

    if (mpaarating & 0x00000004)
    {
        strcat( szRating, "NC-17" );    
    }

    if (mpaarating & 0x00000008)
    {
        strcat( szRating, "NR" );   
    }

    if (mpaarating & 0x00000010)
    {
        strcat( szRating, "PG" );   
    }

    if (mpaarating & 0x00000020)
    {
        strcat( szRating, "PG-13" );   
    }

    if (mpaarating & 0x00000040)
    {
        strcat( szRating, "R" );   
    }

}

//-------------------------------------------------------------------------
// Data Presentation Functions
//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
void ConvertCodepage(char *szString)
{
    unsigned int i = 0;
    char ch;
    
    if (szString[0] == 0) 
    {
        return;
    }
    
    for( i = 0; i < strlen(szString); ++i )
    {
        ch = szString[i];   
        
        // Windows Codepage Translation to DOS/UNIX etc
        
        if (ch == (char)146)
        {
            szString[i] = '\'';
        }
        
        if (ch == (char)147)
        {
            szString[i] = '\"';
        }
        
        if (ch == (char)148)
        {
            szString[i] = '\"';
        }

        
#ifdef _WIN32
        // Windows (Console) specific translations         
#endif
        
#if defined(__unix__) || defined(__APPLE__)
        // UNIX specific translations         
#endif
        
    }
    
    return;
    
}

//-------------------------------------------------------------------------
// Misc. Functions
//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
int CalculateMinutes( int seconds )
{
    int retval = 0;
    double result1 = 0.0;
    double result2 = 0.0;
    
    if (seconds < 1) 
    {
        return 0;
    }
    
    result1 = seconds / 60;
    result2 = seconds % 60;
    
    if (result2 > .0)                   // this will round up if there's any remainder, 
        // could also make this .5 etc
    {
        retval = (int)result1 + 1;
    }
    else
    {
        retval = (int)result1;
    }
    
    return retval;
    
}

//-------------------------------------------------------------------------
// ENDIAN CONVERSION FUNCTIONS
//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
// LONGLONG/INT64 Endian Conversion
//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
#if _MSC_VER < 1700
DWORD64 ntohll(DWORD64 llValue)
{
    DWORD64 retval = 0;

    // This is really cheesy but it works so that's all that matters 
    // If someone out there wants to replace this with something cooler,
    // please feel free to do so!

    char szBuffer[17];
    char szInt64[17];

    memset(szBuffer,0,sizeof(szBuffer));
    memset(szInt64,0,sizeof(szInt64));

    MoveMemory(szBuffer,&llValue,sizeof(llValue));
            
    size_t cc = sizeof(llValue) - 1;
    size_t i = 0;
            
    for( i = 0; i < sizeof(llValue); ++i )
    {
        szInt64[i] = szBuffer[cc];
        --cc;
    }
            
    MoveMemory(&retval,szInt64,sizeof(retval));

    return retval;
}
#endif // _MSC_VER < 1700
//-------------------------------------------------------------------------
void ConvertProgramInfoEndian(struct tagProgramInfo * strProgramInfo)
{

    strProgramInfo->autorecord = ntohl(strProgramInfo->autorecord);
    strProgramInfo->eventtime = ntohl(strProgramInfo->eventtime);
    strProgramInfo->flags = ntohl(strProgramInfo->flags);
    strProgramInfo->isvalid = ntohl(strProgramInfo->isvalid);
    strProgramInfo->minutes = ntohs(strProgramInfo->minutes);
    strProgramInfo->recLen = ntohs(strProgramInfo->recLen);
    strProgramInfo->structuresize = ntohl(strProgramInfo->structuresize);
    strProgramInfo->tuning = ntohl(strProgramInfo->tuning);
    strProgramInfo->tmsID = ntohl(strProgramInfo->tmsID);

    return;
}

//-------------------------------------------------------------------------
void ConvertMovieInfoEndian(struct tagMovieInfo * strMovieInfo)
{
    strMovieInfo->mpaa = ntohs(strMovieInfo->mpaa);
    strMovieInfo->runtime = ntohs(strMovieInfo->runtime);
    strMovieInfo->stars = ntohs(strMovieInfo->stars);
    strMovieInfo->year = ntohs(strMovieInfo->year);

    return;
}

//-------------------------------------------------------------------------
void ConvertPartsInfoEndian(struct tagPartsInfo * strPartsInfo)
{
    strPartsInfo->maxparts = ntohs(strPartsInfo->maxparts);
    strPartsInfo->partnumber = ntohs(strPartsInfo->partnumber);

    return;
}
//-------------------------------------------------------------------------
void ConvertReplayShowEndian(struct tagReplayShow * strReplayShow)
{
    strReplayShow->checkpointed = ntohl(strReplayShow->checkpointed);
    strReplayShow->created = ntohl(strReplayShow->created);
    strReplayShow->downloadid = ntohl(strReplayShow->downloadid);
    strReplayShow->GOP_count = ntohl(strReplayShow->GOP_count);    
    strReplayShow->GOP_highest = ntohl(strReplayShow->GOP_highest);    
    strReplayShow->GOP_last = ntohl(strReplayShow->GOP_last);    
    strReplayShow->guaranteed = ntohl(strReplayShow->guaranteed);
    strReplayShow->guideid = ntohl(strReplayShow->guideid);
    strReplayShow->indexsize = ntohll(strReplayShow->indexsize);
    strReplayShow->inputsource = ntohl(strReplayShow->inputsource);
    strReplayShow->instance = ntohl(strReplayShow->instance);
    strReplayShow->intact = ntohl(strReplayShow->intact);
    strReplayShow->ivsstatus = ntohl(strReplayShow->ivsstatus);
    strReplayShow->mpegsize = ntohll(strReplayShow->mpegsize);
    strReplayShow->playbackflags = ntohl(strReplayShow->playbackflags);
    strReplayShow->quality = ntohl(strReplayShow->quality);
    strReplayShow->recorded = ntohl(strReplayShow->recorded);
    strReplayShow->seconds = ntohl(strReplayShow->seconds);
    strReplayShow->timessent = ntohl(strReplayShow->timessent);
    strReplayShow->upgradeflag = ntohl(strReplayShow->upgradeflag);
    strReplayShow->unused = ntohs(strReplayShow->unused);

    return;
}

//-------------------------------------------------------------------------
void ConvertReplayGuideEndian(struct tagGuideHeader * strGuideHeader)
{
    
    strGuideHeader->GroupData.categories = ntohl(strGuideHeader->GroupData.categories);
    strGuideHeader->GroupData.structuresize = ntohl(strGuideHeader->GroupData.structuresize);

    strGuideHeader->GuideSnapshotHeader.snapshotsize = ntohl(strGuideHeader->GuideSnapshotHeader.snapshotsize);
    strGuideHeader->GuideSnapshotHeader.channeloffset = ntohl(strGuideHeader->GuideSnapshotHeader.channeloffset);
    strGuideHeader->GuideSnapshotHeader.flags = ntohl(strGuideHeader->GuideSnapshotHeader.flags);
    strGuideHeader->GuideSnapshotHeader.groupdataoffset = ntohl(strGuideHeader->GuideSnapshotHeader.groupdataoffset);
    strGuideHeader->GuideSnapshotHeader.osversion = ntohs(strGuideHeader->GuideSnapshotHeader.osversion);
    strGuideHeader->GuideSnapshotHeader.snapshotversion = ntohs(strGuideHeader->GuideSnapshotHeader.snapshotversion);
    strGuideHeader->GuideSnapshotHeader.channelcount = ntohl(strGuideHeader->GuideSnapshotHeader.channelcount);
    strGuideHeader->GuideSnapshotHeader.channelcountcheck = ntohl(strGuideHeader->GuideSnapshotHeader.channelcountcheck);
    strGuideHeader->GuideSnapshotHeader.showoffset = ntohl(strGuideHeader->GuideSnapshotHeader.showoffset);

    strGuideHeader->GuideSnapshotHeader.unknown1 = ntohl(strGuideHeader->GuideSnapshotHeader.unknown1);
    strGuideHeader->GuideSnapshotHeader.unknown2 = ntohl(strGuideHeader->GuideSnapshotHeader.unknown2);
    strGuideHeader->GuideSnapshotHeader.unknown3 = ntohl(strGuideHeader->GuideSnapshotHeader.unknown3);
    strGuideHeader->GuideSnapshotHeader.unknown4 = ntohl(strGuideHeader->GuideSnapshotHeader.unknown4);
    strGuideHeader->GuideSnapshotHeader.unknown5 = ntohl(strGuideHeader->GuideSnapshotHeader.unknown5);
    strGuideHeader->GuideSnapshotHeader.unknown6 = ntohl(strGuideHeader->GuideSnapshotHeader.unknown6);
    strGuideHeader->GuideSnapshotHeader.unknown7 = ntohl(strGuideHeader->GuideSnapshotHeader.unknown7);

    for( unsigned int cc = 0; cc < strGuideHeader->GroupData.categories; ++cc )
    {
        strGuideHeader->GroupData.category[cc] = ntohl(strGuideHeader->GroupData.category[cc]);
        strGuideHeader->GroupData.categoryoffset[cc] = ntohl(strGuideHeader->GroupData.categoryoffset[cc]);
    }
    

    {
        strGuideHeader->GuideSnapshotHeader.structuresize = ntohl(strGuideHeader->GuideSnapshotHeader.structuresize);
    }

    return;
}

//-------------------------------------------------------------------------
void ConvertReplayGuide45To50Endian(struct tagGuideHeader45 * strGuideHeader45, struct tagGuideHeader * strGuideHeader)
{
    
    strGuideHeader->GroupData.categories = ntohl(strGuideHeader45->GroupData.categories);
    strGuideHeader->GroupData.structuresize = ntohl(strGuideHeader45->GroupData.structuresize);

    strGuideHeader->GuideSnapshotHeader.snapshotsize = 0;
    strGuideHeader->GuideSnapshotHeader.channeloffset = ntohl(strGuideHeader45->GuideSnapshotHeader.channeloffset);
    strGuideHeader->GuideSnapshotHeader.flags = ntohl(strGuideHeader45->GuideSnapshotHeader.flags);
    strGuideHeader->GuideSnapshotHeader.groupdataoffset = ntohl(strGuideHeader45->GuideSnapshotHeader.groupdataoffset);
    strGuideHeader->GuideSnapshotHeader.osversion = ntohs(strGuideHeader45->GuideSnapshotHeader.osversion);
    strGuideHeader->GuideSnapshotHeader.snapshotversion = ntohs(strGuideHeader45->GuideSnapshotHeader.snapshotversion);
    strGuideHeader->GuideSnapshotHeader.channelcount = ntohl(strGuideHeader45->GuideSnapshotHeader.channelcount);
    strGuideHeader->GuideSnapshotHeader.channelcountcheck = ntohl(strGuideHeader45->GuideSnapshotHeader.channelcountcheck);
    strGuideHeader->GuideSnapshotHeader.showoffset = ntohl(strGuideHeader45->GuideSnapshotHeader.showoffset);

    strGuideHeader->GuideSnapshotHeader.unknown1 = 0;
    strGuideHeader->GuideSnapshotHeader.unknown2 = 0;
    strGuideHeader->GuideSnapshotHeader.unknown3 = 0;
    strGuideHeader->GuideSnapshotHeader.unknown4 = 0;
    strGuideHeader->GuideSnapshotHeader.unknown5 = 0;
    strGuideHeader->GuideSnapshotHeader.unknown6 = 0;
    strGuideHeader->GuideSnapshotHeader.unknown7 = 0;

    for( unsigned int cc = 0; cc < strGuideHeader->GroupData.categories; ++cc )
    {
        strGuideHeader->GroupData.category[cc] = ntohl(strGuideHeader45->GroupData.category[cc]);
        strGuideHeader->GroupData.categoryoffset[cc] = ntohl(strGuideHeader45->GroupData.categoryoffset[cc]);
    }
    

    {
        strGuideHeader->GuideSnapshotHeader.structuresize = ntohl(strGuideHeader45->GuideSnapshotHeader.structuresize);
    }

    return;
}

//-------------------------------------------------------------------------
// GuideParser
//-------------------------------------------------------------------------

extern "C" {
int GuideParser(char * szOutputBuffer, const char * szInput, const size_t InputSize)
{
    const char * fptr, * junkptr;
    bool moreshows, receivedshow;
    bool isguaranteed, isrepeat, isletterbox, ismovie, hasparts, iscc, isppv, isstereo, issap;
    char szBuffer[1024], szQuality[16];
    char szEpisodeTitle[128];
    char szStarRating[6], szMPAARating[10], szOriginalDescriptionBuffer[226];
    char szTVRating[32];
    char szShowTitle[128];
    char szExtTV[64];
    char szExtMPAA[64];
    int i, curshow, cc;
	  size_t readsize;
    ReplayShow strReplayShow;
    GuideHeader strGuideHeader;
    GuideHeader45 strGuideHeader45;
    MovieInfo strMovieInfo;
    PartsInfo strPartsInfo; 
    
    // Initialize
    
    memset(szBuffer,0,sizeof(szBuffer));
    memset(szQuality,0,sizeof(szQuality));
    memset(szEpisodeTitle,0,sizeof(szEpisodeTitle));
    memset(szOriginalDescriptionBuffer,0,sizeof(szOriginalDescriptionBuffer));
    memset(szStarRating,0,sizeof(szStarRating));
    memset(szMPAARating,0,sizeof(szMPAARating));
    memset(szExtTV,0,sizeof(szExtTV));
    memset(szExtMPAA,0,sizeof(szExtMPAA));
	  sprintf(szOutputBuffer,"%s", "");

    // Display Initial Header
    
    //printf("%s\n",lpszTitle);
    //printf("%s\n\n",lpszCopyright);
    //printf("%s\n",lpszComment);
    
    // Structure Sizes

#ifdef SHOWSTRUCTURESIZES

    //printf("struct strGuideHeader is %d bytes\n",sizeof(tagGuideHeader));
    //printf("struct strMovieInfo is %d bytes\n",sizeof(tagMovieInfo));
    //printf("struct strPartsInfo is %d bytes\n",sizeof(tagPartsInfo));
    //printf("struct strProgramInfo is %d bytes\n",sizeof(tagProgramInfo));
    //printf("struct strChannelInfo is %d bytes\n",sizeof(tagChannelInfo));
    //printf("struct strReplayShow is %d bytes\n",sizeof(tagReplayShow));
    //printf("\n");
#endif

#ifdef _DEBUG
    // Check to make sure structures are required sizes, this is mostly for those of you tinkering with this
    // and - of course - me :).

    if (sizeof(ProgramInfo) != PROGRAMINFO)
    {
        //printf("Error in ProgramInfo structure, needs to be %u instead of %u\n",PROGRAMINFO,sizeof(ProgramInfo));
    }

    if (sizeof(ChannelInfo) != CHANNELINFO)
    {
        //printf("Error in ChannelInfo structure, needs to be %u instead of %u\n",CHANNELINFO,sizeof(ChannelInfo));
    }

    
    if (sizeof(ReplayShow) != REPLAYSHOW)
    {
        //printf("Error in ReplayShow structure, needs to be %u instead of %u\n",REPLAYSHOW,sizeof(ReplayShow));
    }
    
    if (sizeof(GuideHeader) != GUIDEHEADER)
    {
        //printf("Error in GuideHeader structure, needs to be %u instead of %u\n",GUIDEHEADER,sizeof(GuideHeader));
    }

	// 4.5
    if (sizeof(GuideHeader45) != GUIDEHEADER45)
    {
        //printf("Error in GuideHeader45 structure, needs to be %u instead of %u\n",GUIDEHEADER45,sizeof(GuideHeader45));
    }
#endif

    curshow = 0;
    i = 0;
    junkptr = szInput;
    fptr = szInput;

	DWORD ver;

	memcpy(&ver, fptr, sizeof(ver));
	ver = ntohl(ver);
	//printf("V %d.%d!\n", HIWORD(ver), LOWORD(ver));

    //-------------------------------------------------------------------------
    // Replay Header
    //-------------------------------------------------------------------------

    //printf("Reading ReplayTV Guide Header\n");

	switch(HIWORD(ver))
	{
		case 0:	// 5k 5.0
			memcpy( &strGuideHeader, fptr, sizeof(strGuideHeader) );
			ConvertReplayGuideEndian(&strGuideHeader);
			break;
		case 3: // 4k 4.3
		case 5: // 5k 4.5
			memcpy( &strGuideHeader45, fptr, sizeof(strGuideHeader45) );
			ConvertReplayGuide45To50Endian(&strGuideHeader45, &strGuideHeader);
			break;
		default: // Unknown: try 5k 5.0
			//printf("Unknown ReplayTV version: %d.%d!\n", HIWORD(ver), LOWORD(ver));
			memcpy( &strGuideHeader, fptr, sizeof(strGuideHeader) );
			ConvertReplayGuideEndian(&strGuideHeader);
			break;
	}

    if (strGuideHeader.GuideSnapshotHeader.channelcount != strGuideHeader.GuideSnapshotHeader.channelcountcheck) 
    {
        //printf("Guide Snapshot might be damaged. (%u/%u)\n",strGuideHeader.GuideSnapshotHeader.channelcount,strGuideHeader.GuideSnapshotHeader.channelcountcheck);
    }
    
    // jump to the first show record
    
    // fptr = (long)strGuideHeader.GuideSnapshotHeader.showoffset + junkptr; 
    fptr = (long)strGuideHeader.GuideSnapshotHeader.showoffset + junkptr;
            
    //-------------------------------------------------------------------------
    // ReplayShows
    //-------------------------------------------------------------------------
    
    
    //printf("\n\nReading ReplayShows\n\n");
    
    moreshows = true;
    curshow = 0;

	strcat(szOutputBuffer, "<REPLAYGUIDE>\n");
    do
    {
		readsize = sizeof(strReplayShow);
		// 4k 4.3 doesn't have the extra bytes for szReserved at the end of the struct
		if ( HIWORD( ver ) == 3 ) readsize -= sizeof(strReplayShow.szReserved);

		// Need to check if ptr is at, beyond or near end of input (i.e. within sizeof(strReplayshow))
        if ( (InputSize - (fptr - szInput) ) < readsize)
        {
            moreshows = false;
        }
        else
        {
			memcpy( &strReplayShow, fptr, readsize);

            curshow++;
            
            memset(szTVRating,0,sizeof(szTVRating));
            memset(szBuffer,0,sizeof(szBuffer));
            memset(szEpisodeTitle,0,sizeof(szEpisodeTitle));
            memset(szShowTitle,0,sizeof(szShowTitle));
            memset(szMPAARating,0,sizeof(szMPAARating));
            memset(szOriginalDescriptionBuffer,0,sizeof(szOriginalDescriptionBuffer));
            strcpy(szStarRating,"*****");
            memset(szExtTV,0,sizeof(szExtTV));
            memset(szExtMPAA,0,sizeof(szExtMPAA));

            // Convert Endian

            ConvertReplayShowEndian(&strReplayShow);
            ConvertProgramInfoEndian(&strReplayShow.ProgramInfo);

            /*
            // this is for this hack, this seems to be no longer used so copy it from eventtime

            if (strReplayShow.recorded == 0) {
                    strReplayShow.recorded = strReplayShow.ProgramInfo.eventtime;
            }
            */
            
            // Process ProgramInfo

            MoveMemory(szOriginalDescriptionBuffer,strReplayShow.ProgramInfo.szDescription,sizeof(szOriginalDescriptionBuffer));

            // Process Flags 
            
            iscc = false;
            issap = false;
            isstereo = false;
            ismovie = false;
            isppv = false;
            hasparts = false;
            isletterbox = false;
            isrepeat = false;   
            receivedshow = false;
            isguaranteed = false;
            cc = 0;                         // Offset for Extended Data

            if (strReplayShow.guaranteed == 0xFFFFFFFF) 
            {
                isguaranteed = true;
            }

            if (strReplayShow.guideid != 0)
            {
                receivedshow = true;
            }
            

            if (strReplayShow.ProgramInfo.flags & 0x00000001) 
            {
                iscc = true;
            }

            if (strReplayShow.ProgramInfo.flags & 0x00000002) 
            {
                isstereo = true;
            }

            if (strReplayShow.ProgramInfo.flags & 0x00000004) 
            {
                isrepeat = true;
            }

            if (strReplayShow.ProgramInfo.flags & 0x00000008) 
            {
                issap = true;
            }
            
            if (strReplayShow.ProgramInfo.flags & 0x00000010) 
            {
                isletterbox = true;
            }
            
            if (strReplayShow.ProgramInfo.flags & 0x00000020) 
            {
                // Movie, load extra 8 bytes into MovieInfo structure.
                
                ismovie = true;
                MoveMemory(&strMovieInfo,szOriginalDescriptionBuffer + cc,sizeof(strMovieInfo));
                ConvertMovieInfoEndian(&strMovieInfo);
                
                memset(szBuffer,0,sizeof(szBuffer));
                cc =  cc + sizeof(strMovieInfo);
                MoveMemory(szBuffer,szOriginalDescriptionBuffer + cc,sizeof(szOriginalDescriptionBuffer) - cc);
                
                // this won't do the half stars, it's too cheesy for that!
                
                i = strMovieInfo.stars / 10;
                szStarRating[i] = 0;
                
                // MPAA Rating 
                
                DisplayMPAARating(strMovieInfo.mpaa,szMPAARating);
                DisplayExtendedMPAARating(strMovieInfo.mpaa,szExtMPAA,false);
                
                if (strlen(szMPAARating) > 0) 
                {
                    if (strlen(szExtMPAA) > 0)
                    {
                        strcat( szMPAARating, " (" );
                        strcat( szMPAARating, szExtMPAA );
                        strcat( szMPAARating, ")" );
                    }
				}
			}
            
            
            if (strReplayShow.ProgramInfo.flags & 0x00000040) 
            {
                
                // Multiple Parts
                
                hasparts = true;
                MoveMemory(&strPartsInfo,szOriginalDescriptionBuffer + cc,sizeof(strPartsInfo));
                ConvertPartsInfoEndian(&strPartsInfo);
                
                memset(szBuffer,0,sizeof(szBuffer));
                
                cc = cc + sizeof(strPartsInfo);
                MoveMemory(szBuffer,szOriginalDescriptionBuffer + cc,sizeof(szOriginalDescriptionBuffer) - cc);
            }

            if (strReplayShow.ProgramInfo.flags & 0x00000080) 
            {
                isppv = true;
            }
            
            if (!ismovie)
            {
                /*
                **
                ** NOTE: 
                ** This is what the Replay does.  However, the TMS data does support a mix
                ** so, if you want, you can remove "if (!ismovie)".
                */

                DisplayTVRating(strReplayShow.ProgramInfo.flags,szTVRating);
                DisplayExtendedTVRating(strReplayShow.ProgramInfo.flags,szExtTV,false);
            }

            if (strlen(szTVRating) > 0) 
            {
                if (strlen(szExtTV) > 0)
                {
                    strcat( szTVRating, " (" );
                    strcat( szTVRating, szExtTV );
                    strcat( szTVRating, ")" );
                }
            }


            
            i = cc;             // set offset (needed if it's been mangled by HasParts or IsMovie)           
            
            strncpy(szShowTitle,strReplayShow.ProgramInfo.szDescription + i,strReplayShow.ProgramInfo.titleLen);
            i = i + strReplayShow.ProgramInfo.titleLen;
            
            strncpy(szEpisodeTitle,strReplayShow.ProgramInfo.szDescription + i,strReplayShow.ProgramInfo.episodeLen);
            i = i + strReplayShow.ProgramInfo.episodeLen;
            
            // Convert Codepages
            
            ConvertCodepage(strReplayShow.ProgramInfo.szDescription);
            ConvertCodepage(szShowTitle);
            ConvertCodepage(szEpisodeTitle);
                
            // Build Strings

            DisplayQuality(strReplayShow.quality,szQuality);

            // Display Record
            
			strcat(szOutputBuffer, "<ITEM>\n");
			strcat(szOutputBuffer,"\t<DISPLAYNAME>");

            if (szEpisodeTitle[0] > 31) 
            {
                sprintf(szBuffer,"%s \"%s\"",szShowTitle,szEpisodeTitle);
				strcat(szOutputBuffer, szBuffer);
            }
            else
            {
                if (ismovie) 
                {

                    if (szStarRating[0] != 0)
                    {
                        if (strMovieInfo.stars % 10)
                        {
                            sprintf(szBuffer,"%s (%s1/2, %s, %d",szShowTitle,szStarRating,szMPAARating,strMovieInfo.year);
							strcat(szOutputBuffer, szBuffer);
                        }
                        else
                        {
                            sprintf(szBuffer,"%s (%s, %s, %d",szShowTitle,szStarRating,szMPAARating,strMovieInfo.year);
							strcat(szOutputBuffer, szBuffer);
                        }
                    }
                    else
                    {
                        sprintf(szBuffer,"%s (%s, %d",szShowTitle,szMPAARating,strMovieInfo.year);
						strcat(szOutputBuffer, szBuffer);
                    }
                }
                else
                {
					strcat(szOutputBuffer, szShowTitle);
                }
			}

            // display flags

            cc = 0;
            i = 0;

            if (iscc) 
            {
                cc++;
            }
            if (isstereo)
            {
                cc++;
            }
            if (isrepeat)
            {
                cc++;
            }
            if (issap)
            {
                cc++;
            }
            if (isppv)
            {
                cc++;
            }

            if (szTVRating[0] > 31)
            {
                cc++;
            }

            if (isletterbox)
            {
                cc++;
            }

            if (cc > 0)
            {
                if (ismovie)
                {
                    strcat(szOutputBuffer,", ");
                }
                else
                {   
                    strcat(szOutputBuffer," (");
                }
                i = 1;
            }
            else
            {
                if (ismovie)
                {
                    strcat(szOutputBuffer,")");
                }
            }

            if (szTVRating[0] > 31)
            {
                strcat(szOutputBuffer,szTVRating);
                cc--;
                if (cc)
                {
                    strcat(szOutputBuffer,", ");
                }
            }

            if (iscc)
            {
                strcat(szOutputBuffer,"CC");
                cc--;
                if (cc)
                {
                    strcat(szOutputBuffer,", ");
                }
            }

            if (isstereo)
            {
                strcat(szOutputBuffer,"Stereo");
                cc--;
                if (cc)
                {
                    strcat(szOutputBuffer,", ");
                }
            }

            if (isrepeat)
            {
                strcat(szOutputBuffer,"Repeat");
                cc--;
                if (cc)
                {
                    strcat(szOutputBuffer,", ");
                }
            }

            if (issap)
            {
                strcat(szOutputBuffer,"SAP");
                cc--;
                if (cc)
                {
                    strcat(szOutputBuffer,", ");
                }
            }

            if (isppv)
            {
                strcat(szOutputBuffer,"PPV");
                cc--;
                if (cc)
                {
                    strcat(szOutputBuffer,", ");
                }
            }

            if (isletterbox)
            {
                strcat(szOutputBuffer,"Letterboxed");
                cc--;
                if (cc)
                {
                    strcat(szOutputBuffer,", ");
                }
            }

            if (i > 0)
            {
                strcat(szOutputBuffer,")");
            }
            
            if (hasparts)
            {
                sprintf(szBuffer,"Part %d of %d",strPartsInfo.partnumber,strPartsInfo.maxparts);
				strcat(szOutputBuffer, szBuffer);
            }

			strcat(szOutputBuffer,"</DISPLAYNAME>\n");

			sprintf(szBuffer,"\t<QUALITY>%s</QUALITY>\n",szQuality);
			strcat(szOutputBuffer, szBuffer);

			sprintf(szBuffer,"\t<RECORDED>%s</RECORDED>\n",UnixTimeToString(strReplayShow.recorded));
			strcat(szOutputBuffer, szBuffer);
            
			sprintf(szBuffer,"\t<PATH>Video/%ld.mpg</PATH>\n",strReplayShow.recorded);
			strcat(szOutputBuffer, szBuffer);

			strcat(szOutputBuffer,"\t<DURATION>");
            i = strReplayShow.ProgramInfo.minutes + strReplayShow.afterpadding + strReplayShow.beforepadding;	
            cc = CalculateMinutes(strReplayShow.seconds);       
            if (cc != i)
            {
                // This can happen if the cable goes out during recording or if you hit STOP etc.
                sprintf(szBuffer,"%d minutes ( %d scheduled )",cc,i);			
            }
            else
            {
                sprintf(szBuffer,"%d minutes",cc);			
            }
			strcat(szOutputBuffer, szBuffer);
			strcat(szOutputBuffer,"</DURATION>\n");

			sprintf(szBuffer,"\t<SIZE>%d</SIZE>\n",cc);
			strcat(szOutputBuffer, szBuffer);

			strcat(szOutputBuffer,"</ITEM>\n");

			fptr += readsize;
        }
	}
    while ( moreshows == true );
	strcat(szOutputBuffer, "</REPLAYGUIDE>\n");
    
    // Done, shut it down
    
    //printf("Done!  %u ReplayShows parsed!\n", curshow);
            
    return 0;
    
}
}

