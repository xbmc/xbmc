/*
 * /home/ms/files/source/libsidtune/RCS/InfoFile.cpp,v
 *
 * SIDPLAY INFOFILE format support.
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

#include "config.h"

#ifdef HAVE_EXCEPTIONS
#   include <new>
#endif
#include <iostream>
#include <iomanip>
#include <ctype.h>
#include <string.h>

#if defined(HAVE_SSTREAM)
#   include <sstream>
#else
#   include <strstream>
#endif

#include "SidTune.h"
#include "SidTuneTools.h"
#include "sidendian.h"


static const char text_format[] = "Raw plus SIDPLAY ASCII text file (SID)";

static const char text_truncatedError[] = "SIDTUNE ERROR: SID file is truncated";
static const char text_noMemError[] = "SIDTUNE ERROR: Not enough free memory";
static const char text_invalidError[] = "SIDTUNE ERROR: File contains invalid data";

static const char keyword_id[] = "SIDPLAY INFOFILE";

static const char keyword_name[] = "NAME=";            // No white-space characters 
static const char keyword_author[] = "AUTHOR=";        // in these keywords, because
static const char keyword_copyright[] = "COPYRIGHT=";  // we want to use a white-space (depreciated)
static const char keyword_released[] = "RELEASED=";    // we want to use a white-space
static const char keyword_address[] = "ADDRESS=";      // eating string stream to
static const char keyword_songs[] = "SONGS=";          // parse most of the header.
static const char keyword_speed[] = "SPEED=";
static const char keyword_musPlayer[] = "SIDSONG=YES";
static const char keyword_reloc[] = "RELOC=";
static const char keyword_clock[] = "CLOCK=";
static const char keyword_sidModel[] = "SIDMODEL=";
static const char keyword_compatibility[] = "COMPATIBILITY=";

static const uint_least16_t sidMinFileSize = 1+sizeof(keyword_id);  // Just to avoid a first segm.fault.
static const uint_least16_t parseChunkLen = 80;                     // Enough for all keywords incl. their values.


SidTune::LoadStatus SidTune::SID_fileSupport(Buffer_sidtt<const uint_least8_t>& dataBuf,
                                             Buffer_sidtt<const uint_least8_t>& sidBuf)
{
    uint_least32_t sidBufLen = sidBuf.len();
    // Make sure SID buffer pointer is not zero.
    // Check for a minimum file size. If it is smaller, we will not proceed.
    if (sidBufLen<sidMinFileSize)
    {
        return LOAD_NOT_MINE;
    }

    const char* pParseBuf = (const char*)sidBuf.get();
    // First line has to contain the exact identification string.
    if ( SidTuneTools::myStrNcaseCmp( pParseBuf, keyword_id ) == 0 )
    {
        // At least the ID was found, so set a default error message.
        info.formatString = text_truncatedError;
        
        // Defaults.
        fileOffset = 0;                // no header in separate data file
        info.sidChipBase1 = 0xd400;
        info.sidChipBase2 = 0;
        info.musPlayer = false;
        info.numberOfInfoStrings = 0;
        uint_least32_t oldStyleSpeed = 0;

        // Flags for required entries.
        bool hasAddress = false,
            hasName = false,
            hasAuthor = false,
            hasReleased = false,
            hasSongs = false,
            hasSpeed = false,
            hasInitAddr = false;
    
        // Using a temporary instance of an input string chunk.
#ifdef HAVE_EXCEPTIONS
        char* pParseChunk = new(std::nothrow) char[parseChunkLen+1];
#else
        char* pParseChunk = new char[parseChunkLen+1];
#endif
        if ( pParseChunk == 0 )
        {
            info.formatString = text_noMemError;
            return LOAD_ERROR;
        }
        
        // Parse as long we have not collected all ``required'' entries.
        //while ( !hasAddress || !hasName || !hasAuthor || !hasCopyright
        //        || !hasSongs || !hasSpeed )

        // Above implementation is wrong, we need to get all known
        // fields and then check if all ``required'' ones were found.
        for (;;)
        {
            // Skip to next line. Leave loop, if none.
            if (( pParseBuf = SidTuneTools::returnNextLine( pParseBuf )) == 0 )
            {
                break;
            }
            // And get a second pointer to the following line.
            const char* pNextLine = SidTuneTools::returnNextLine( pParseBuf );
            uint_least32_t restLen;
            if ( pNextLine != 0 )
            {
                // Calculate number of chars between current pos and next line.
                restLen = (uint_least32_t)(pNextLine - pParseBuf);
            }
            else
            {
                // Calculate number of chars between current pos and end of buf.
                restLen = sidBufLen - (uint_least32_t)(pParseBuf - (char*)sidBuf.get());
            }
#ifdef HAVE_SSTREAM
            std::string sParse( pParseBuf, restLen );            
            // Create whitespace eating (!) input string stream.
            std::istringstream parseStream( sParse );
            // A second one just for copying.
            std::istringstream parseCopyStream( sParse );
#else
            std::istrstream parseStream((char *) pParseBuf, restLen );
            std::istrstream parseCopyStream((char *) pParseBuf, restLen );
#endif
            if ( !parseStream || !parseCopyStream )
            {
                break;
            }
            // Now copy the next X characters except white-spaces.
            for ( uint_least16_t i = 0; i < parseChunkLen; i++ )
            {
                char c;
                parseCopyStream >> c;
                pParseChunk[i] = c;
            }
            pParseChunk[parseChunkLen]=0;
            // Now check for the possible keywords.
            // ADDRESS
            if ( SidTuneTools::myStrNcaseCmp( pParseChunk, keyword_address ) == 0 )
            {
                SidTuneTools::skipToEqu( parseStream );
                info.loadAddr = (uint_least16_t)SidTuneTools::readHex( parseStream );
                info.initAddr = info.loadAddr;
                hasInitAddr = true;
                if ( parseStream )
                {
                    info.initAddr = (uint_least16_t)SidTuneTools::readHex( parseStream );
                    if ( !parseStream )
                        break;
                    info.playAddr = (uint_least16_t)SidTuneTools::readHex( parseStream );
                    hasAddress = true;
                }
            }
            // NAME
            else if ( SidTuneTools::myStrNcaseCmp( pParseChunk, keyword_name ) == 0 )
            {
                SidTuneTools::copyStringValueToEOL(pParseBuf,&infoString[0][0],SIDTUNE_MAX_CREDIT_STRLEN);
                info.infoString[0] = &infoString[0][0];
                hasName = true;
            }
            // AUTHOR
            else if ( SidTuneTools::myStrNcaseCmp( pParseChunk, keyword_author ) == 0 )
            {
                SidTuneTools::copyStringValueToEOL(pParseBuf,&infoString[1][0],SIDTUNE_MAX_CREDIT_STRLEN);
                info.infoString[1] = &infoString[1][0];
                hasAuthor = true;
            }
            // COPYRIGHT
            else if ( SidTuneTools::myStrNcaseCmp( pParseChunk, keyword_copyright ) == 0 )
            {
                SidTuneTools::copyStringValueToEOL(pParseBuf,&infoString[2][0],SIDTUNE_MAX_CREDIT_STRLEN);
                info.infoString[2] = &infoString[2][0];
                hasReleased = true;
            }
            // RELEASED
            else if ( SidTuneTools::myStrNcaseCmp( pParseChunk, keyword_released ) == 0 )
            {
                SidTuneTools::copyStringValueToEOL(pParseBuf,&infoString[2][0],SIDTUNE_MAX_CREDIT_STRLEN);
                info.infoString[2] = &infoString[2][0];
                hasReleased = true;
            }
            // SONGS
            else if ( SidTuneTools::myStrNcaseCmp( pParseChunk, keyword_songs ) == 0 )
            {
                SidTuneTools::skipToEqu( parseStream );
                info.songs = (uint_least16_t)SidTuneTools::readDec( parseStream );
                info.startSong = (uint_least16_t)SidTuneTools::readDec( parseStream );
                hasSongs = true;
            }
            // SPEED
            else if ( SidTuneTools::myStrNcaseCmp( pParseChunk, keyword_speed ) == 0 )
            {
                SidTuneTools::skipToEqu( parseStream );
                oldStyleSpeed = SidTuneTools::readHex(parseStream);
                hasSpeed = true;
            }
            // SIDSONG
            else if ( SidTuneTools::myStrNcaseCmp( pParseChunk, keyword_musPlayer ) == 0 )
            {
                info.musPlayer = true;
            }
            // RELOC
            else if ( SidTuneTools::myStrNcaseCmp( pParseChunk, keyword_reloc ) == 0 )
            {
                info.relocStartPage = (uint_least8_t)SidTuneTools::readHex( parseStream );
                if ( !parseStream )
                    break;
                info.relocPages = (uint_least8_t)SidTuneTools::readHex( parseStream );
            }
            // CLOCK
            else if ( SidTuneTools::myStrNcaseCmp( pParseChunk, keyword_clock ) == 0 )
            {
                char clock[8];
                SidTuneTools::copyStringValueToEOL(pParseBuf,clock,sizeof(clock));
                if ( SidTuneTools::myStrNcaseCmp( clock, "UNKNOWN" ) == 0 )
                    info.clockSpeed = SIDTUNE_CLOCK_UNKNOWN;
                else if ( SidTuneTools::myStrNcaseCmp( clock, "PAL" ) == 0 )
                    info.clockSpeed = SIDTUNE_CLOCK_PAL;
                else if ( SidTuneTools::myStrNcaseCmp( clock, "NTSC" ) == 0 )
                    info.clockSpeed = SIDTUNE_CLOCK_NTSC;
                else if ( SidTuneTools::myStrNcaseCmp( clock, "ANY" ) == 0 )
                    info.clockSpeed = SIDTUNE_CLOCK_ANY;
            }
            // SIDMODEL
            else if ( SidTuneTools::myStrNcaseCmp( pParseChunk, keyword_sidModel ) == 0 )
            {
                char model[8];
                SidTuneTools::copyStringValueToEOL(pParseBuf,model,sizeof(model));
                if ( SidTuneTools::myStrNcaseCmp( model, "UNKNOWN" ) == 0 )
                    info.sidModel = SIDTUNE_SIDMODEL_UNKNOWN;
                else if ( SidTuneTools::myStrNcaseCmp( model, "6581" ) == 0 )
                    info.sidModel = SIDTUNE_SIDMODEL_6581;
                else if ( SidTuneTools::myStrNcaseCmp( model, "8580" ) == 0 )
                    info.sidModel = SIDTUNE_SIDMODEL_8580;
                else if ( SidTuneTools::myStrNcaseCmp( model, "ANY" ) == 0 )
                    info.sidModel = SIDTUNE_SIDMODEL_ANY;
            }
            // COMPATIBILITY
            else if ( SidTuneTools::myStrNcaseCmp( pParseChunk, keyword_compatibility ) == 0 )
            {
                char comp[6];
                SidTuneTools::copyStringValueToEOL(pParseBuf,comp,sizeof(comp));
                if ( SidTuneTools::myStrNcaseCmp( comp, "C64" ) == 0 )
                    info.compatibility = SIDTUNE_COMPATIBILITY_C64;
                else if ( SidTuneTools::myStrNcaseCmp( comp, "PSID" ) == 0 )
                    info.compatibility = SIDTUNE_COMPATIBILITY_PSID;
                else if ( SidTuneTools::myStrNcaseCmp( comp, "R64" ) == 0 )
                    info.compatibility = SIDTUNE_COMPATIBILITY_R64;
                else if ( SidTuneTools::myStrNcaseCmp( comp, "BASIC" ) == 0 )
                    info.compatibility = SIDTUNE_COMPATIBILITY_BASIC;
            }
        }

        delete[] pParseChunk;
        
        if ( !(hasName && hasAuthor && hasReleased && hasSongs) )
        {   // Something is missing (or damaged ?).
            // Error string set above.
            return LOAD_ERROR;
        }

        switch ( info.compatibility )
        {
        case SIDTUNE_COMPATIBILITY_PSID:
        case SIDTUNE_COMPATIBILITY_C64:
            if ( !(hasAddress && hasSpeed) )
                return LOAD_ERROR; // Error set above
            break;

        case SIDTUNE_COMPATIBILITY_R64:
            if ( !(hasInitAddr || hasAddress) )
                return LOAD_ERROR; // Error set above
            // Allow user to provide single address
            if ( !hasAddress )
                info.loadAddr = 0;
            else if ( info.loadAddr || info.playAddr )
            {
                info.formatString = text_invalidError;
                return LOAD_ERROR;
            }
            // Deliberate run on

        case SIDTUNE_COMPATIBILITY_BASIC:
            oldStyleSpeed = ~0;
        }

        // Create the speed/clock setting table.
        convertOldStyleSpeedToTables(oldStyleSpeed, info.clockSpeed);
        info.numberOfInfoStrings = 3;
        // We finally accept the input data.
        info.formatString = text_format;
        if ( info.musPlayer && !dataBuf.isEmpty() )
            return MUS_load (dataBuf);
        return LOAD_OK;
    }
    return LOAD_NOT_MINE;
}


bool SidTune::SID_fileSupportSave( std::ofstream& toFile )
{
    toFile << keyword_id << std::endl;
    int compatibility = info.compatibility;

    if ( info.musPlayer )
    {
        compatibility = SIDTUNE_COMPATIBILITY_C64;
    }

    switch ( compatibility )
    {
    case SIDTUNE_COMPATIBILITY_PSID:
    case SIDTUNE_COMPATIBILITY_C64:
        toFile << keyword_address << std::setfill('0')
            << std::hex << std::setw(4) << 0 << ',';

        if ( info.musPlayer )
        {
            toFile << std::setw(4) << 0 << ','
                   << std::setw(4) << 0 << std::endl;
        }
        else
        {
            toFile << std::hex << std::setw(4) << info.initAddr << ','
                   << std::hex << std::setw(4) << info.playAddr << std::endl;
        }

        {
            uint_least32_t oldStyleSpeed = 0;
            int maxBugSongs = ((info.songs <= 32) ? info.songs : 32);
            for (int s = 0; s < maxBugSongs; s++)
            {
                if (songSpeed[s] == SIDTUNE_SPEED_CIA_1A)
                {
                    oldStyleSpeed |= (1<<s);
                }
            }
            toFile
                << keyword_speed << std::hex << std::setw(8)
                << oldStyleSpeed << std::endl;
        }
        break;
    case SIDTUNE_COMPATIBILITY_R64:
        toFile << keyword_address << std::hex << std::setw(4)
            << std::setfill('0') << info.initAddr << std::endl;
        break;
    }

    toFile << keyword_songs << std::dec << (int)info.songs << ","
           << (int)info.startSong << std::endl;

    // @FIXME@ Need better solution.  Make it possible to override MUS strings
    if ( info.numberOfInfoStrings == 3 )
    {
        toFile << keyword_name << info.infoString[0] << std::endl
               << keyword_author << info.infoString[1] << std::endl
               << keyword_released << info.infoString[2] << std::endl;
    }
    else
    {
        toFile << keyword_name << std::endl << keyword_author << std::endl
               << keyword_released << std::endl;
    }

    if ( info.musPlayer )
    {
        toFile << keyword_musPlayer << std::endl;
    }
    else
    {
        if ( compatibility == SIDTUNE_COMPATIBILITY_PSID )
        {
            toFile << keyword_compatibility << "PSID" << std::endl;
        }
        else if ( compatibility == SIDTUNE_COMPATIBILITY_R64 )
        {
            toFile << keyword_compatibility << "R64" << std::endl;
        }
        else if ( compatibility == SIDTUNE_COMPATIBILITY_BASIC )
        {
            toFile << keyword_compatibility << "BASIC" << std::endl;
        }
        if ( info.relocStartPage )
        {
            toFile
                << keyword_reloc << std::setfill('0')
                << std::hex << std::setw(2) << (int) info.relocStartPage << ","
                << std::hex << std::setw(2) << (int) info.relocPages << std::endl;
        }
    }
    if ( info.clockSpeed != SIDTUNE_CLOCK_UNKNOWN )
    {
        toFile << keyword_clock;
        switch (info.clockSpeed)
        {
        case SIDTUNE_CLOCK_PAL:
            toFile << "PAL";
            break;
        case SIDTUNE_CLOCK_NTSC:
            toFile << "NTSC";
            break;
        case SIDTUNE_CLOCK_ANY:
            toFile << "ANY";
            break;
        }
        toFile << std::endl;
    }
    if ( info.sidModel != SIDTUNE_SIDMODEL_UNKNOWN )
    {
        toFile << keyword_sidModel;
        switch (info.sidModel)
        {
        case SIDTUNE_SIDMODEL_6581:
            toFile << "6581";
            break;
        case SIDTUNE_SIDMODEL_8580:
            toFile << "8580";
            break;
        case SIDTUNE_SIDMODEL_ANY:
            toFile << "ANY";
            break;
        }
        toFile << std::endl;
    }

    if ( !toFile )
    {
        return false;
    }
    else
    {
        return true;
    }
}
