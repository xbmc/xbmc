/********************************************************************
*    
* Copyright (c) 2002 Artur Polaczynski (Ar't)  All rights reserved.
*            <artii@o2.pl>        LGPL-2.1
*       $ArtId: info_mpc.h,v 1.8 2003/04/13 11:24:10 art Exp $
********************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as 
 * published by the Free Software Foundation; either version 2.1 
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#ifndef INFO_MPC_H
#define INFO_MPC_H

/** \file info_mpc.h 
    \brief Get information from MusePack file.

    Usage:
    \code
    StreamInfoMpc Info;
    
    if (info_mpc_read(fn, &Info)) {
        printf("File \"%s\" not found or is read protected!\n", fn);
        return;
    }
    printf("%",Info.fields);
    \endcode
*/


/**    All information from mpc file
 *    \struct StreamInfoMpc
**/
typedef struct
{
    unsigned int   ByteLength;       /**< file length - tags size */
    unsigned int   FileSize;         /**< real file size */
    int            SampleFreq;       /**< Sample frequency */
    unsigned int   Channels;         /**< channels =2 */
    int            Duration;         /**< duratiom in ms */
    
    unsigned int   StreamVersion;    /**< Streamversion of current file */
    unsigned int   Bitrate;          /**< bitrate of current file (bps) */
    unsigned int   Frames;           /**< number of frames contained */
    unsigned int   MS;               /**< Mid/Side Stereo (0: off, 1: on) */
    unsigned int   Profile;          /**< quality profile */
    unsigned int   MaxBand;          /**< maximum band-index used (0...31) */
    unsigned int   IS;               /**< Intensity Stereo (0: off, 1: on) */
    unsigned int   BlockSize;        /**< only needed for SV4...SV6 -> not supported */
    
    const char    *ProfileName;      /**< Profile name */
    unsigned int   EncoderVersion;   /**< version of encoder used */
    char           Encoder[256];     /**< Encoder Version in string */
    
    // ReplayGain related data
    short          GainTitle;        /**< Gain Title */
    short          GainAlbum;        /**< Gain Album */
    unsigned short PeakAlbum;        /**< Peak value of Album */
    unsigned short PeakTitle;        /**< Peak value of Title */
    unsigned short EstPeakTitle;     /**< Estimated Peak value of Title */
    
    // true gapless stuff
    unsigned int   IsTrueGapless;    /**< is true gapless used? */
    unsigned int   LastFrameSamples; /**< number of valid samples within last frame */
} StreamInfoMpc;

/** \def StreamInfo  is only for compatible before 0.4alpha4 
    \deprecated removed in 0.5
*/
#define StreamInfo StreamInfoMpc

/**
    Read all mpc info from filename 

    \param fn File name 
    \param Info StreamInfoMpc Structure for all information
    \retval 0 ok
    \retval 1 file not found or write protected
    \retval 2 not musepack audio file
*/
int
info_mpc_read(const char *fn, StreamInfoMpc *Info);

#endif /* INFO_MPC_H */
