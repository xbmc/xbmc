/* genres.h - EasyTAG - Jerome Couderc 2000/05/29 */
/*
 *  EasyTAG - Tag editor for MP3 and Ogg Vorbis files
 *  Copyright (C) 2000-2003  Jerome Couderc <easytag@gmail.com>
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


#ifndef __GENRES_H__
#define __GENRES_H__


/* GENRE_MAX is the last genre number that can be used */
#define GENRE_MAX ( sizeof(id3_genres)/sizeof(id3_genres[0]) - 1 )
#define ID3_INVALID_GENRE 255

/**
    \def genre_no(IndeX) 
    \param IndeX number of genre using in id3v1 
    \return pointer to genre as string
*/
#define genre_no(IndeX) ( IndeX < (sizeof(id3_genres)/sizeof(*id3_genres) ) ? id3_genres[IndeX] : "Unknown" )


/* 
 * Do not sort genres!!
 * Last Update: 2000/04/30
 */
static char *id3_genres[] =
{
    "Blues",            /* 0 */
    "Classic Rock",
    "Country",
    "Dance",
    "Disco",
    "Funk",             /* 5 */
    "Grunge",
    "Hip-Hop", 
    "Jazz",
    "Metal",
    "New Age",          /* 10 */
    "Oldies",
    "Other", 
    "Pop",
    "R&B",
    "Rap",              /* 15 */
    "Reggae", 
    "Rock",
    "Techno",
    "Industrial",
    "Alternative",      /* 20 */
    "Ska",
    "Death Metal", 
    "Pranks",
    "Soundtrack",
    "Euro-Techno",      /* 25 */
    "Ambient",
    "Trip-Hop", 
    "Vocal",
    "Jazz+Funk", 
    "Fusion",           /* 30 */
    "Trance",
    "Classical",
    "Instrumental", 
    "Acid",
    "House",            /* 35 */
    "Game",
    "Sound Clip", 
    "Gospel",
    "Noise",
    "Altern Rock",      /* 40 */
    "Bass",
    "Soul",
    "Punk",
    "Space",
    "Meditative",       /* 45 */
    "Instrumental Pop",
    "Instrumental Rock", 
    "Ethnic",
    "Gothic",
    "Darkwave",         /* 50 */
    "Techno-Industrial", 
    "Electronic", 
    "Pop-Folk",
    "Eurodance", 
    "Dream",            /* 55 */
    "Southern Rock", 
    "Comedy", 
    "Cult",
    "Gangsta",
    "Top 40",           /* 60 */
    "Christian Rap", 
    "Pop/Funk", 
    "Jungle",
    "Native American", 
    "Cabaret",          /* 65 */
    "New Wave",
    "Psychadelic", 
    "Rave",
    "Showtunes", 
    "Trailer",          /* 70 */
    "Lo-Fi",
    "Tribal",
    "Acid Punk",
    "Acid Jazz", 
    "Polka",            /* 75 */
    "Retro",
    "Musical",
    "Rock & Roll", 
    "Hard Rock", 
    "Folk",             /* 80 */
    "Folk/Rock",
    "National Folk", 
    "Swing",
    "Fast Fusion",
    "Bebob",            /* 85 */
    "Latin",
    "Revival",
    "Celtic",
    "Bluegrass",
    "Avantgarde",       /* 90 */
    "Gothic Rock",
    "Progressive Rock",
    "Psychedelic Rock", 
    "Symphonic Rock", 
    "Slow Rock",        /* 95 */
    "Big Band", 
    "Chorus",
    "Easy Listening", 
    "Acoustic", 
    "Humour",           /* 100 */
    "Speech",
    "Chanson", 
    "Opera",
    "Chamber Music", 
    "Sonata",           /* 105 */
    "Symphony",
    "Booty Bass", 
    "Primus",
    "Porn Groove", 
    "Satire",           /* 110 */
    "Slow Jam", 
    "Club",
    "Tango",
    "Samba",
    "Folklore",         /* 115 */
    "Ballad",
    "Power Ballad",
    "Rhythmic Soul",
    "Freestyle",
    "Duet",             /* 120 */
    "Punk Rock",
    "Drum Solo",
    "A Capella",
    "Euro-House",
    "Dance Hall",       /* 125 */
    "Goa",
    "Drum & Bass",
    "Club-House",
    "Hardcore",
    "Terror",           /* 130 */
    "Indie",
    "BritPop",
    "Negerpunk",
    "Polsk Punk",
    "Beat",             /* 135 */
    "Christian Gangsta Rap",
    "Heavy Metal",
    "Black Metal",
    "Crossover",
    "Contemporary Christian",/* 140 */
    "Christian Rock",
    "Merengue",
    "Salsa",
    "Thrash Metal",
    "Anime",            /* 145 */
    "JPop",
    "Synthpop"
};


#endif /* __GENRES_H__ */
