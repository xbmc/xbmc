/*
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
#include <sidplay/sidendian.h>
#include "config.h"
#include "SidTuneMod.h"
#include "MD5/MD5.h"

const char *SidTuneMod::createMD5(char *md5)
{
    if (!md5)
        md5 = m_md5;
    *md5 = '\0';

    if (status)
    {   // Include C64 data.
        MD5 myMD5;
        md5_byte_t tmp[2];
        myMD5.append (cache.get()+fileOffset,info.c64dataLen);
        // Include INIT and PLAY address.
        endian_little16 (tmp,info.initAddr);
        myMD5.append    (tmp,sizeof(tmp));
        endian_little16 (tmp,info.playAddr);
        myMD5.append    (tmp,sizeof(tmp));
        // Include number of songs.
        endian_little16 (tmp,info.songs);
        myMD5.append    (tmp,sizeof(tmp));
        {   // Include song speed for each song.
            uint_least16_t currentSong = info.currentSong;
            for (uint_least16_t s = 1; s <= info.songs; s++)
            {
                selectSong (s);
                myMD5.append (&info.songSpeed,sizeof(info.songSpeed));
            }
            // Restore old song
            selectSong (currentSong);
        }
        // Deal with PSID v2NG clock speed flags: Let only NTSC
        // clock speed change the MD5 fingerprint. That way the
        // fingerprint of a PAL-speed sidtune in PSID v1, v2, and
        // PSID v2NG format is the same.
        if (info.clockSpeed == SIDTUNE_CLOCK_NTSC)
            myMD5.append (&info.clockSpeed,sizeof(info.clockSpeed));
        // NB! If the fingerprint is used as an index into a
        // song-lengths database or cache, modify above code to
        // allow for PSID v2NG files which have clock speed set to
        // SIDTUNE_CLOCK_ANY. If the SID player program fully
        // supports the SIDTUNE_CLOCK_ANY setting, a sidtune could
        // either create two different fingerprints depending on
        // the clock speed chosen by the player, or there could be
        // two different values stored in the database/cache.

        myMD5.finish();
        // Construct fingerprint.
        char *m = md5;
        for (int di = 0; di < 16; ++di)
        {
            sprintf (m, "%02x", (int) myMD5.getDigest()[di]);
            m += 2;
        }
    }
    return md5;
}
