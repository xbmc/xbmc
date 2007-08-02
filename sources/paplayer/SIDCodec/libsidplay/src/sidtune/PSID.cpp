/*
 * /home/ms/files/source/libsidtune/RCS/PSID.cpp,v
 *
 * PlaySID one-file format support.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "SidTuneCfg.h"
#include "SidTune.h"
#include "sidendian.h"

#define PSID_ID 0x50534944
#define RSID_ID 0x52534944

// Header has been extended for 'RSID' format
// The following changes are present:
//     id = 'RSID'
//     version = 2 only
//     play, load and speed reserved 0
//     psidspecific flag reserved 0
//     init cannot be under ROMS/IO
//     load cannot be less than 0x0801 (start of basic)

struct psidHeader           // all values big-endian
{
    char id[4];             // 'PSID' (ASCII)
    uint8_t version[2];     // 0x0001 or 0x0002
    uint8_t data[2];        // 16-bit offset to binary data in file
    uint8_t load[2];        // 16-bit C64 address to load file to
    uint8_t init[2];        // 16-bit C64 address of init subroutine
    uint8_t play[2];        // 16-bit C64 address of play subroutine
    uint8_t songs[2];       // number of songs
    uint8_t start[2];       // start song out of [1..256]
    uint8_t speed[4];       // 32-bit speed info
                            // bit: 0=50 Hz, 1=CIA 1 Timer A (default: 60 Hz)
    char name[32];          // ASCII strings, 31 characters long and
    char author[32];        // terminated by a trailing zero
    char released[32];      //
    uint8_t flags[2];       // only version 0x0002
    uint8_t relocStartPage; // only version 0x0002B
    uint8_t relocPages;     // only version 0x0002B
    uint8_t reserved[2];    // only version 0x0002
};

enum
{
    PSID_MUS       = 1 << 0,
    PSID_SPECIFIC  = 1 << 1, // These two are mutally exclusive
    PSID_BASIC     = 1 << 1, 
    PSID_CLOCK     = 3 << 2,
    PSID_SIDMODEL  = 3 << 4
};

enum
{
    PSID_CLOCK_UNKNOWN = 0,
    PSID_CLOCK_PAL     = 1 << 2,
    PSID_CLOCK_NTSC    = 1 << 3,
    PSID_CLOCK_ANY     = PSID_CLOCK_PAL | PSID_CLOCK_NTSC
};

enum
{
    PSID_SIDMODEL_UNKNOWN = 0,
    PSID_SIDMODEL_6581    = 1 << 4,
    PSID_SIDMODEL_8580    = 1 << 5,
    PSID_SIDMODEL_ANY     = PSID_SIDMODEL_6581 | PSID_SIDMODEL_8580
};

static const char _sidtune_format_psid[] = "PlaySID one-file format (PSID)";
static const char _sidtune_format_rsid[] = "Real C64 one-file format (RSID)";
static const char _sidtune_unknown_psid[] = "Unsupported PSID version";
static const char _sidtune_unknown_rsid[] = "Unsupported RSID version";
static const char _sidtune_truncated[] = "ERROR: File is most likely truncated";
static const char _sidtune_invalid[] = "ERROR: File contains invalid data";

static const int _sidtune_psid_maxStrLen = 31;


SidTune::LoadStatus SidTune::PSID_fileSupport(Buffer_sidtt<const uint_least8_t>& dataBuf)
{
    int clock, compatibility;
    uint_least32_t speed;
    uint_least32_t bufLen = dataBuf.len();
#ifdef SIDTUNE_PSID2NG
    clock = SIDTUNE_CLOCK_UNKNOWN;
#else
    clock = info.clockSpeed;
#endif
    compatibility = SIDTUNE_COMPATIBILITY_C64;

    // Require minimum size to allow access to the first few bytes.
    // Require a valid ID and version number.
    const psidHeader* pHeader = (const psidHeader*)dataBuf.get();

    // File format check
    if (bufLen<6)
        return LOAD_NOT_MINE;
    if (endian_big32((const uint_least8_t*)pHeader->id)==PSID_ID)
    {
       switch (endian_big16(pHeader->version))
       {
       case 1:
           compatibility = SIDTUNE_COMPATIBILITY_PSID;
           // Deliberate run on
       case 2:
           break;
       default:
           info.formatString = _sidtune_unknown_psid;
           return LOAD_ERROR;
       }
       info.formatString = _sidtune_format_psid;
    }
    else if (endian_big32((const uint_least8_t*)pHeader->id)==RSID_ID)
    {
       if (endian_big16(pHeader->version) != 2)
       {
           info.formatString = _sidtune_unknown_rsid;
           return LOAD_ERROR;
       }
       info.formatString = _sidtune_format_rsid;
       compatibility = SIDTUNE_COMPATIBILITY_R64;
    }
    else
    {
        return LOAD_NOT_MINE;
    }

    // Due to security concerns, input must be at least as long as version 1
    // header plus 16-bit C64 load address. That is the area which will be
    // accessed.
    if ( bufLen < (sizeof(psidHeader)+2) )
    {
        info.formatString = _sidtune_truncated;
        return LOAD_ERROR;
    }

    fileOffset         = endian_big16(pHeader->data);
    info.loadAddr      = endian_big16(pHeader->load);
    info.initAddr      = endian_big16(pHeader->init);
    info.playAddr      = endian_big16(pHeader->play);
    info.songs         = endian_big16(pHeader->songs);
    info.startSong     = endian_big16(pHeader->start);
    info.sidChipBase1  = 0xd400;
    info.sidChipBase2  = 0;
    info.compatibility = compatibility;
    speed              = endian_big32(pHeader->speed);

    if (info.songs > SIDTUNE_MAX_SONGS)
    {
        info.songs = SIDTUNE_MAX_SONGS;
    }

    info.musPlayer      = false;
    info.sidModel       = SIDTUNE_SIDMODEL_UNKNOWN;
    info.relocPages     = 0;
    info.relocStartPage = 0;
    if ( endian_big16(pHeader->version) >= 2 )
    {
        uint_least16_t flags = endian_big16(pHeader->flags);
        if (flags & PSID_MUS)
        {   // MUS tunes run at any speed
            clock = SIDTUNE_CLOCK_ANY;
            info.musPlayer = true;
        }

#ifdef SIDTUNE_PSID2NG
        // This flags is only available for the appropriate
        // file formats
        switch (compatibility)
        {
        case SIDTUNE_COMPATIBILITY_C64:
            if (flags & PSID_SPECIFIC)
                info.compatibility = SIDTUNE_COMPATIBILITY_PSID;
            break;
        case SIDTUNE_COMPATIBILITY_R64:
            if (flags & PSID_BASIC)
                info.compatibility = SIDTUNE_COMPATIBILITY_BASIC;
            break;
        }

        if (flags & PSID_CLOCK_PAL)
            clock |= SIDTUNE_CLOCK_PAL;
        if (flags & PSID_CLOCK_NTSC)
            clock |= SIDTUNE_CLOCK_NTSC;
        info.clockSpeed = clock;

        info.sidModel = SIDTUNE_SIDMODEL_UNKNOWN;
        if (flags & PSID_SIDMODEL_6581)
            info.sidModel |= SIDTUNE_SIDMODEL_6581;
        if (flags & PSID_SIDMODEL_8580)
            info.sidModel |= SIDTUNE_SIDMODEL_8580;

        info.relocStartPage = pHeader->relocStartPage;
        info.relocPages     = pHeader->relocPages;
#endif // SIDTUNE_PSID2NG
    }

    // Check reserved fields to force real c64 compliance
    // as required by the RSID specification
    if (compatibility == SIDTUNE_COMPATIBILITY_R64)
    {
        if ((info.loadAddr != 0) ||
            (info.playAddr != 0) ||
            (speed != 0))
        {
            info.formatString = _sidtune_invalid;
            return LOAD_ERROR;
        }
        // Real C64 tunes appear as CIA
        speed = ~0;
    }
    // Create the speed/clock setting table.
    convertOldStyleSpeedToTables(speed, clock);

    // Copy info strings, so they will not get lost.
    info.numberOfInfoStrings = 3;
    // Name
    strncpy(&infoString[0][0],pHeader->name,_sidtune_psid_maxStrLen);
    info.infoString[0] = &infoString[0][0];
    // Author
    strncpy(&infoString[1][0],pHeader->author,_sidtune_psid_maxStrLen);
    info.infoString[1] = &infoString[1][0];
    // Released
    strncpy(&infoString[2][0],pHeader->released,_sidtune_psid_maxStrLen);
    info.infoString[2] = &infoString[2][0];

    if ( info.musPlayer )
        return MUS_load (dataBuf);
    return LOAD_OK;
}


bool SidTune::PSID_fileSupportSave(std::ofstream& fMyOut, const uint_least8_t* dataBuffer)
{
    psidHeader myHeader;
    endian_big32((uint_least8_t*)myHeader.id,PSID_ID);
    endian_big16(myHeader.version,2);
    endian_big16(myHeader.data,sizeof(psidHeader));
    endian_big16(myHeader.songs,info.songs);
    endian_big16(myHeader.start,info.startSong);

    uint_least32_t speed = 0, check = 0;
    uint_least32_t maxBugSongs = ((info.songs <= 32) ? info.songs : 32);
    for (uint_least32_t s = 0; s < maxBugSongs; s++)
    {
        if (songSpeed[s] == SIDTUNE_SPEED_CIA_1A)
            speed |= (1<<s);
        check |= (1<<s);
    }
    endian_big32(myHeader.speed,speed);

    uint_least16_t tmpFlags = 0;
    if ( info.musPlayer )
    {
        endian_big16(myHeader.load,0);
        endian_big16(myHeader.init,0);
        endian_big16(myHeader.play,0);
        myHeader.relocStartPage = 0;
        myHeader.relocPages     = 0;
        tmpFlags |= PSID_MUS;
    }
    else
    {
        endian_big16(myHeader.load,0);
        endian_big16(myHeader.init,info.initAddr);
        myHeader.relocStartPage = info.relocStartPage;
        myHeader.relocPages     = info.relocPages;

        switch (info.compatibility)
        {
        case SIDTUNE_COMPATIBILITY_BASIC:
            tmpFlags |= PSID_BASIC;
        case SIDTUNE_COMPATIBILITY_R64:
            endian_big32((uint_least8_t*)myHeader.id,RSID_ID);
            endian_big16(myHeader.play,0);
            endian_big32(myHeader.speed,0);
            break;
        case SIDTUNE_COMPATIBILITY_PSID:
            tmpFlags |= PSID_SPECIFIC;
        default:
            endian_big16(myHeader.play,info.playAddr);
            break;
        }
    }

    for ( uint i = 0; i < 32; i++ )
    {
        myHeader.name[i] = 0;
        myHeader.author[i] = 0;
        myHeader.released[i] = 0;
    }

    // @FIXME@ Need better solution.  Make it possible to override MUS strings
    if ( info.numberOfInfoStrings == 3 )
    {
        strncpy( myHeader.name, info.infoString[0], _sidtune_psid_maxStrLen);
        strncpy( myHeader.author, info.infoString[1], _sidtune_psid_maxStrLen);
        strncpy( myHeader.released, info.infoString[2], _sidtune_psid_maxStrLen);
    }

    tmpFlags |= (info.clockSpeed << 2);
    tmpFlags |= (info.sidModel << 4);
    endian_big16(myHeader.flags,tmpFlags);
    endian_big16(myHeader.reserved,0);

    fMyOut.write( (char*)&myHeader, sizeof(psidHeader) );

    if (info.musPlayer)
        fMyOut.write( (const char*)dataBuffer, info.dataFileLen );  // !cast!
    else
    {   // Save C64 lo/hi load address (little-endian).
        uint_least8_t saveAddr[2];
        saveAddr[0] = info.loadAddr & 255;
        saveAddr[1] = info.loadAddr >> 8;
        fMyOut.write( (char*)saveAddr, 2 );  // !cast!

        // Data starts at: bufferaddr + fileoffset
        // Data length: datafilelen - fileoffset
        fMyOut.write( (const char*)dataBuffer + fileOffset, info.dataFileLen - fileOffset );  // !cast!
    }

    if ( !fMyOut )
        return false;
    else
        return true;
}
