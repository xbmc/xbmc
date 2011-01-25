#ifndef ID3V1GENRE_H
#define ID3V1GENRE_H

#define ID3_NR_OF_V1_GENRES 148
/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

static const char* ID3_v1_genre_description[ID3_NR_OF_V1_GENRES] =
{
  "Blues",             //0
  "Classic Rock",      //1
  "Country",           //2
  "Dance",             //3
  "Disco",             //4
  "Funk",              //5
  "Grunge",            //6
  "Hip-Hop",           //7
  "Jazz",              //8
  "Metal",             //9
  "New Age",           //10
  "Oldies",            //11
  "Other",             //12
  "Pop",               //13
  "R&B",               //14
  "Rap",               //15
  "Reggae",            //16
  "Rock",              //17
  "Techno",            //18
  "Industrial",        //19
  "Alternative",       //20
  "Ska",               //21
  "Death Metal",       //22
  "Pranks",            //23
  "Soundtrack",        //24
  "Euro-Techno",       //25
  "Ambient",           //26
  "Trip-Hop",          //27
  "Vocal",             //28
  "Jazz+Funk",         //29
  "Fusion",            //30
  "Trance",            //31
  "Classical",         //32
  "Instrumental",      //33
  "Acid",              //34
  "House",             //35
  "Game",              //36
  "Sound Clip",        //37
  "Gospel",            //38
  "Noise",             //39
  "AlternRock",        //40
  "Bass",              //41
  "Soul",              //42
  "Punk",              //43
  "Space",             //44
  "Meditative",        //45
  "Instrumental Pop",  //46
  "Instrumental Rock", //47
  "Ethnic",            //48
  "Gothic",            //49
  "Darkwave",          //50
  "Techno-Industrial", //51
  "Electronic",        //52
  "Pop-Folk",          //53
  "Eurodance",         //54
  "Dream",             //55
  "Southern Rock",     //56
  "Comedy",            //57
  "Cult",              //58
  "Gangsta",           //59
  "Top 40",            //60
  "Christian Rap",     //61
  "Pop/Funk",          //62
  "Jungle",            //63
  "Native American",   //64
  "Cabaret",           //65
  "New Wave",          //66
  "Psychadelic",       //67
  "Rave",              //68
  "Showtunes",         //69
  "Trailer",           //70
  "Lo-Fi",             //71
  "Tribal",            //72
  "Acid Punk",         //73
  "Acid Jazz",         //74
  "Polka",             //75
  "Retro",             //76
  "Musical",           //77
  "Rock & Roll",       //78
  "Hard Rock",         //79
// following are winamp extentions
  "Folk",                  //80
  "Folk-Rock",             //81
  "National Folk",         //82
  "Swing",                 //83
  "Fast Fusion",           //84
  "Bebob",                 //85
  "Latin",                 //86
  "Revival",               //87
  "Celtic",                //88
  "Bluegrass",             //89
  "Avantgarde",            //90
  "Gothic Rock",           //91
  "Progressive Rock",      //92
  "Psychedelic Rock",      //93
  "Symphonic Rock",        //94
  "Slow Rock",             //95
  "Big Band",              //96
  "Chorus",                //97
  "Easy Listening",        //98
  "Acoustic",              //99
  "Humour",                //100
  "Speech",                //101
  "Chanson",               //102
  "Opera",                 //103
  "Chamber Music",         //104
  "Sonata",                //105
  "Symphony",              //106
  "Booty Bass",            //107
  "Primus",                //108
  "Porn Groove",           //109
  "Satire",                //110
  "Slow Jam",              //111
  "Club",                  //112
  "Tango",                 //113
  "Samba",                 //114
  "Folklore",              //115
  "Ballad",                //116
  "Power Ballad",          //117
  "Rhythmic Soul",         //118
  "Freestyle",             //119
  "Duet",                  //120
  "Punk Rock",             //121
  "Drum Solo",             //122
  "A capella",             //123
  "Euro-House",            //124
  "Dance Hall",            //125
  "Goa",                   //126
  "Drum & Bass",           //127
  "Club-House",            //128
  "Hardcore",              //129
  "Terror",                //130
  "Indie",                 //131
  "Britpop",               //132
  "Negerpunk",             //133
  "Polsk Punk",            //134
  "Beat",                  //135
  "Christian Gangsta Rap", //136
  "Heavy Metal",           //137
  "Black Metal",           //138
  "Crossover",             //139
  "Contemporary Christian",//140
  "Christian Rock ",       //141
  "Merengue",              //142
  "Salsa",                 //143
  "Thrash Metal",          //144
  "Anime",                 //145
  "JPop",                  //146
  "Synthpop"               //147
};

#define ID3_V1GENRE2DESCRIPTION(x) (x < ID3_NR_OF_V1_GENRES && x >= 0) ? ID3_v1_genre_description[x] : NULL

#endif

