/*
 * C64 PRG file format support.
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

#include "SidTuneCfg.h"
#include "SidTune.h"
#include "SidTuneTools.h"

static const char _sidtune_format_prg[] = "Tape image file (PRG)";
static const char _sidtune_truncated[] = "ERROR: File is most likely truncated";


SidTune::LoadStatus SidTune::PRG_fileSupport(const char *fileName,
                                             Buffer_sidtt<const uint_least8_t>& dataBuf)
{
    const char *ext = SidTuneTools::fileExtOfPath(const_cast<char *>(fileName));
    if ( (MYSTRICMP(ext,".prg")!=0) &&
         (MYSTRICMP(ext,".c64")!=0) )
    {
        return LOAD_NOT_MINE;
    }

    info.formatString = _sidtune_format_prg;
    if (dataBuf.len() < 2)
    {
        info.formatString = _sidtune_truncated;
        return LOAD_ERROR;
    }

    // Automatic settings
    info.songs         = 1;
    info.startSong     = 1;
    info.compatibility = SIDTUNE_COMPATIBILITY_BASIC;
    info.numberOfInfoStrings = 0;

    // Create the speed/clock setting table.
    convertOldStyleSpeedToTables(~0, info.clockSpeed);
    return LOAD_OK;
}
