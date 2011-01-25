/********************************************************************
*    
* Copyright (c) 2002 Artur Polaczynski (Ar't)  All rights reserved.
*            <artii@o2.pl>        LGPL-2.1
*       $ArtId: info_mac.h,v 1.6 2003/04/13 11:24:10 art Exp $
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


#ifndef INFO_MAC_H
#define INFO_MAC_H

/** \file info_mac.h 
    \brief Get information from MonkeyAudio file.

    Usage:
    \code
    StreamInfoMac Info;
    
    if (info_mac_read(fn, &Info)) {
        printf("File \"%s\" not found or is read protected!\n", fn);
        return;
    }
    printf("%",Info.fields...);
    \endcode
*/

/** 
    \name Compression level
*/
/*\{*/
#define COMPRESSION_LEVEL_FAST        1000 /**< fast */
#define COMPRESSION_LEVEL_NORMAL      2000 /**< optimal average time/compression ratio */
#define COMPRESSION_LEVEL_HIGH        3000 /**< higher compression ratio */
#define COMPRESSION_LEVEL_EXTRA_HIGH  4000 /**< very slowly */
#define COMPRESSION_LEVEL_INSANE      5000 /**< ??? */
/*\}*/

/**    All information from mac file
 *    \struct StreamInfoMac 
**/
typedef struct 
{
    unsigned int ByteLength;        /**< file length - tags size */
    unsigned int FileSize;          /**< real file size */
    int          SampleFreq;        /**< sample frequency */
    unsigned int Channels;          /**< number of chanels */
    int          Duration;          /**< duratiom in ms */
    
    unsigned int Version;           /**< version of current file */
    unsigned int Bitrate;           /**< bitrate of current file (bps) */
    unsigned int Compresion;        /**< compresion profile */
    unsigned int Flags;             /**< flags */
    
    unsigned int Frames;            /**< number of frames */
    unsigned int SamplesPerFrame;   /**< samples per frame */
    unsigned int Samples;           /**< number of samples */
    unsigned int BitsPerSample;     /**< bits per sample */
    unsigned int UncompresedSize;   /**< uncomprese size of file */
    float        CompresionRatio;   /**< compresion ratio */
    
    unsigned int PeakLevel;         /**< peak level */
    float        PeakRatio;         /**< peak ratio */
    
    const char  *CompresionName;    /**< compresion profile as string */
    
    unsigned int EncoderVersion;    /**< version of encoder used */
} StreamInfoMac;

/**
    Read all mac info from filename 

    \param fn File name 
    \param Info StreamInfoMac Structure for all information
    \retval 0 ok
    \retval 1 file not found or write protected
    \retval 2 not monkey's audio file
*/
int
info_mac_read(const char *fn, StreamInfoMac * Info);

#endif /* INFO_MAC_H */
