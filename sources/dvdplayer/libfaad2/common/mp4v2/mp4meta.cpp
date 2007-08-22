/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is MPEG4IP.
 *
 * The Initial Developer of the Original Code is Cisco Systems Inc.
 * Portions created by Cisco Systems Inc. are
 * Copyright (C) Cisco Systems Inc. 2001.  All Rights Reserved.
 *
 * Contributor(s):
 *      M. Bakker     mbakker at nero.com
 *
 * Apple iTunes Metadata handling
 */

/**

 The iTunes tagging seems to support any tag field name
 but there are some predefined fields, also known from the QuickTime format

 predefined fields (the ones I know of until now):
 - ©nam : Name of the song/movie (string)
 - ©ART : Name of the artist/performer (string)
 - ©wrt : Name of the writer (string)
 - ©alb : Name of the album (string)
 - ©day : Year (4 bytes, e.g. "2003") (string)
 - ©too : Tool(s) used to create the file (string)
 - ©cmt : Comment (string)
 - ©gen : Custom genre (string)
 - trkn : Tracknumber (8 byte string)
           16 bit: empty
           16 bit: tracknumber
           16 bit: total tracks on album
           16 bit: empty
 - disk : Disknumber (8 byte string)
           16 bit: empty
           16 bit: disknumber
           16 bit: total number of disks
           16 bit: empty
 - gnre : Genre (16 bit genre) (ID3v1 index + 1)
 - cpil : Part of a compilation (1 byte, 1 or 0)
 - tmpo : Tempo in BPM (16 bit)
 - covr : Cover art (xx bytes binary data)
 - ---- : Free form metadata, can have any name and any data

**/

#include "mp4common.h"

bool MP4File::GetMetadataByIndex(u_int32_t index,
                                 const char** ppName,
                                 u_int8_t** ppValue, u_int32_t* pValueSize)
{
    char s[256];

    sprintf(s, "moov.udta.meta.ilst.*[%u].data.metadata", index);
    GetBytesProperty(s, ppValue, pValueSize);

    sprintf(s, "moov.udta.meta.ilst.*[%u]", index);
    MP4Atom* pParent = m_pRootAtom->FindAtom(s);
    *ppName = pParent->GetType();

    /* check for free form tagfield */
    if (memcmp(*ppName, "----", 4) == 0)
    {
        u_int8_t* pV;
        u_int32_t VSize = 0;
        char *pN;

        sprintf(s, "moov.udta.meta.ilst.*[%u].name.metadata", index);
        GetBytesProperty(s, &pV, &VSize);

        pN = (char*)malloc((VSize+1)*sizeof(char));
        memset(pN, 0, (VSize+1)*sizeof(char));
        memcpy(pN, pV, VSize*sizeof(char));

        *ppName = pN;
    }

    return true;
}

bool MP4File::CreateMetadataAtom(const char* name)
{
    char s[256];
    char t[256];

    sprintf(t, "udta.meta.ilst.%s.data", name);
    sprintf(s, "moov.udta.meta.ilst.%s.data", name);
    AddDescendantAtoms("moov", t);
    MP4Atom *pMetaAtom = m_pRootAtom->FindAtom(s);

    if (!pMetaAtom)
        return false;

    /* some fields need special flags set */
    if (name[0] == '©')
    {
        pMetaAtom->SetFlags(0x1);
    } else if ((memcmp(name, "cpil", 4) == 0) || (memcmp(name, "tmpo", 4) == 0)) {
        pMetaAtom->SetFlags(0xF);
    }

    MP4Atom *pHdlrAtom = m_pRootAtom->FindAtom("moov.udta.meta.hdlr");
    MP4StringProperty *pStringProperty = NULL;
    MP4BytesProperty *pBytesProperty = NULL;
    ASSERT(pHdlrAtom);

    pHdlrAtom->FindProperty(
        "hdlr.handlerType", (MP4Property**)&pStringProperty);
    ASSERT(pStringProperty);
    pStringProperty->SetValue("mdir");

    u_int8_t val[12];
    memset(val, 0, 12*sizeof(u_int8_t));
    val[0] = 0x61;
    val[1] = 0x70;
    val[2] = 0x70;
    val[3] = 0x6c;
    pHdlrAtom->FindProperty(
        "hdlr.reserved2", (MP4Property**)&pBytesProperty);
    ASSERT(pBytesProperty);
    pBytesProperty->SetReadOnly(false);
    pBytesProperty->SetValue(val, 12);
    pBytesProperty->SetReadOnly(true);

    return true;
}

bool MP4File::SetMetadataName(const char* value)
{
    const char *s = "moov.udta.meta.ilst.©nam.data";
    MP4BytesProperty *pMetadataProperty = NULL;
    MP4Atom *pMetaAtom = NULL;
    
    pMetaAtom = m_pRootAtom->FindAtom(s);

    if (!pMetaAtom)
    {
        if (!CreateMetadataAtom("©nam"))
            return false;

        pMetaAtom = m_pRootAtom->FindAtom(s);
    }

    pMetaAtom->FindProperty("data.metadata", (MP4Property**)&pMetadataProperty);
    ASSERT(pMetadataProperty);

    pMetadataProperty->SetValue((u_int8_t*)value, strlen(value));

    return true;
}

bool MP4File::GetMetadataName(char** value)
{
    unsigned char *val = NULL;
    u_int32_t valSize = 0;
    const char *s = "moov.udta.meta.ilst.©nam.data.metadata";
    GetBytesProperty(s, (u_int8_t**)&val, &valSize);

    if (valSize > 0)
    {
        *value = (char*)malloc((valSize+1)*sizeof(unsigned char));
        memset(*value, 0, (valSize+1)*sizeof(unsigned char));
        memcpy(*value, val, valSize*sizeof(unsigned char));
        return true;
    } else {
        *value = NULL;
        return false;
    }
}

bool MP4File::SetMetadataWriter(const char* value)
{
    const char *s = "moov.udta.meta.ilst.©wrt.data";
    MP4BytesProperty *pMetadataProperty = NULL;
    MP4Atom *pMetaAtom = NULL;
    
    pMetaAtom = m_pRootAtom->FindAtom(s);

    if (!pMetaAtom)
    {
        if (!CreateMetadataAtom("©wrt"))
            return false;

        pMetaAtom = m_pRootAtom->FindAtom(s);
    }

    pMetaAtom->FindProperty("data.metadata", (MP4Property**)&pMetadataProperty);
    ASSERT(pMetadataProperty);

    pMetadataProperty->SetValue((u_int8_t*)value, strlen(value));

    return true;
}

bool MP4File::GetMetadataWriter(char** value)
{
    unsigned char *val = NULL;
    u_int32_t valSize = 0;
    const char *s = "moov.udta.meta.ilst.©wrt.data.metadata";
    GetBytesProperty(s, (u_int8_t**)&val, &valSize);

    if (valSize > 0)
    {
        *value = (char*)malloc((valSize+1)*sizeof(unsigned char));
        memset(*value, 0, (valSize+1)*sizeof(unsigned char));
        memcpy(*value, val, valSize*sizeof(unsigned char));
        return true;
    } else {
        *value = NULL;
        return false;
    }
}

bool MP4File::SetMetadataAlbum(const char* value)
{
    const char *s = "moov.udta.meta.ilst.©alb.data";
    MP4BytesProperty *pMetadataProperty = NULL;
    MP4Atom *pMetaAtom = NULL;
    
    pMetaAtom = m_pRootAtom->FindAtom(s);

    if (!pMetaAtom)
    {
        if (!CreateMetadataAtom("©alb"))
            return false;

        pMetaAtom = m_pRootAtom->FindAtom(s);
    }

    pMetaAtom->FindProperty("data.metadata", (MP4Property**)&pMetadataProperty);
    ASSERT(pMetadataProperty);

    pMetadataProperty->SetValue((u_int8_t*)value, strlen(value));

    return true;
}

bool MP4File::GetMetadataAlbum(char** value)
{
    unsigned char *val = NULL;
    u_int32_t valSize = 0;
    const char *s = "moov.udta.meta.ilst.©alb.data.metadata";
    GetBytesProperty(s, (u_int8_t**)&val, &valSize);

    if (valSize > 0)
    {
        *value = (char*)malloc((valSize+1)*sizeof(unsigned char));
        memset(*value, 0, (valSize+1)*sizeof(unsigned char));
        memcpy(*value, val, valSize*sizeof(unsigned char));
        return true;
    } else {
        *value = NULL;
        return false;
    }
}

bool MP4File::SetMetadataArtist(const char* value)
{
    const char *s = "moov.udta.meta.ilst.©ART.data";
    MP4BytesProperty *pMetadataProperty = NULL;
    MP4Atom *pMetaAtom = NULL;
    
    pMetaAtom = m_pRootAtom->FindAtom(s);

    if (!pMetaAtom)
    {
        if (!CreateMetadataAtom("©ART"))
            return false;

        pMetaAtom = m_pRootAtom->FindAtom(s);
    }

    pMetaAtom->FindProperty("data.metadata", (MP4Property**)&pMetadataProperty);
    ASSERT(pMetadataProperty);

    pMetadataProperty->SetValue((u_int8_t*)value, strlen(value));

    return true;
}

bool MP4File::GetMetadataArtist(char** value)
{
    unsigned char *val = NULL;
    u_int32_t valSize = 0;
    const char *s = "moov.udta.meta.ilst.©ART.data.metadata";
    GetBytesProperty(s, (u_int8_t**)&val, &valSize);

    if (valSize > 0)
    {
        *value = (char*)malloc((valSize+1)*sizeof(unsigned char));
        memset(*value, 0, (valSize+1)*sizeof(unsigned char));
        memcpy(*value, val, valSize*sizeof(unsigned char));
        return true;
    } else {
        *value = NULL;
        return false;
    }
}

bool MP4File::SetMetadataTool(const char* value)
{
    const char *s = "moov.udta.meta.ilst.©too.data";
    MP4BytesProperty *pMetadataProperty = NULL;
    MP4Atom *pMetaAtom = NULL;
    
    pMetaAtom = m_pRootAtom->FindAtom(s);

    if (!pMetaAtom)
    {
        if (!CreateMetadataAtom("©too"))
            return false;

        pMetaAtom = m_pRootAtom->FindAtom(s);
    }

    pMetaAtom->FindProperty("data.metadata", (MP4Property**)&pMetadataProperty);
    ASSERT(pMetadataProperty);

    pMetadataProperty->SetValue((u_int8_t*)value, strlen(value));

    return true;
}

bool MP4File::GetMetadataTool(char** value)
{
    unsigned char *val = NULL;
    u_int32_t valSize = 0;
    const char *s = "moov.udta.meta.ilst.©too.data.metadata";
    GetBytesProperty(s, (u_int8_t**)&val, &valSize);

    if (valSize > 0)
    {
        *value = (char*)malloc((valSize+1)*sizeof(unsigned char));
        memset(*value, 0, (valSize+1)*sizeof(unsigned char));
        memcpy(*value, val, valSize*sizeof(unsigned char));
        return true;
    } else {
        *value = NULL;
        return false;
    }
}

bool MP4File::SetMetadataComment(const char* value)
{
    const char *s = "moov.udta.meta.ilst.©cmt.data";
    MP4BytesProperty *pMetadataProperty = NULL;
    MP4Atom *pMetaAtom = NULL;
    
    pMetaAtom = m_pRootAtom->FindAtom(s);

    if (!pMetaAtom)
    {
        if (!CreateMetadataAtom("©cmt"))
            return false;

        pMetaAtom = m_pRootAtom->FindAtom(s);
    }

    pMetaAtom->FindProperty("data.metadata", (MP4Property**)&pMetadataProperty);
    ASSERT(pMetadataProperty);

    pMetadataProperty->SetValue((u_int8_t*)value, strlen(value));

    return true;
}

bool MP4File::GetMetadataComment(char** value)
{
    unsigned char *val = NULL;
    u_int32_t valSize = 0;
    const char *s = "moov.udta.meta.ilst.©cmt.data.metadata";
    GetBytesProperty(s, (u_int8_t**)&val, &valSize);

    if (valSize > 0)
    {
        *value = (char*)malloc((valSize+1)*sizeof(unsigned char));
        memset(*value, 0, (valSize+1)*sizeof(unsigned char));
        memcpy(*value, val, valSize*sizeof(unsigned char));
        return true;
    } else {
        *value = NULL;
        return false;
    }
}

bool MP4File::SetMetadataYear(const char* value)
{
    const char *s = "moov.udta.meta.ilst.©day.data";
    MP4BytesProperty *pMetadataProperty = NULL;
    MP4Atom *pMetaAtom = NULL;
    
    pMetaAtom = m_pRootAtom->FindAtom(s);

    if (!pMetaAtom)
    {
        if (!CreateMetadataAtom("©day"))
            return false;

        pMetaAtom = m_pRootAtom->FindAtom(s);
    }

    pMetaAtom->FindProperty("data.metadata", (MP4Property**)&pMetadataProperty);
    ASSERT(pMetadataProperty);

    if (strlen(value) < 4)
        return false;

    pMetadataProperty->SetValue((u_int8_t*)value, 4);

    return true;
}

bool MP4File::GetMetadataYear(char** value)
{
    unsigned char *val = NULL;
    u_int32_t valSize = 0;
    const char *s = "moov.udta.meta.ilst.©day.data.metadata";
    GetBytesProperty(s, (u_int8_t**)&val, &valSize);

    if (valSize > 0)
    {
        *value = (char*)malloc((valSize+1)*sizeof(unsigned char));
        memset(*value, 0, (valSize+1)*sizeof(unsigned char));
        memcpy(*value, val, valSize*sizeof(unsigned char));
        return true;
    } else {
        *value = NULL;
        return false;
    }
}

bool MP4File::SetMetadataTrack(u_int16_t track, u_int16_t totalTracks)
{
    unsigned char t[9];
    const char *s = "moov.udta.meta.ilst.trkn.data";
    MP4BytesProperty *pMetadataProperty = NULL;
    MP4Atom *pMetaAtom = NULL;
    
    pMetaAtom = m_pRootAtom->FindAtom(s);

    if (!pMetaAtom)
    {
        if (!CreateMetadataAtom("trkn"))
            return false;

        pMetaAtom = m_pRootAtom->FindAtom(s);
    }

    memset(t, 0, 9*sizeof(unsigned char));
    t[2] = (unsigned char)(track>>8)&0xFF;
    t[3] = (unsigned char)(track)&0xFF;
    t[4] = (unsigned char)(totalTracks>>8)&0xFF;
    t[5] = (unsigned char)(totalTracks)&0xFF;

    pMetaAtom->FindProperty("data.metadata", (MP4Property**)&pMetadataProperty);
    ASSERT(pMetadataProperty);

    pMetadataProperty->SetValue((u_int8_t*)t, 8);

    return true;
}

bool MP4File::GetMetadataTrack(u_int16_t* track, u_int16_t* totalTracks)
{
    unsigned char *val = NULL;
    u_int32_t valSize = 0;
    const char *s = "moov.udta.meta.ilst.trkn.data.metadata";
    GetBytesProperty(s, (u_int8_t**)&val, &valSize);

    *track = 0;
    *totalTracks = 0;

    if (valSize != 8)
        return false;

    *track = (u_int16_t)(val[3]);
    *track += (u_int16_t)(val[2]<<8);
    *totalTracks = (u_int16_t)(val[5]);
    *totalTracks += (u_int16_t)(val[4]<<8);

    return true;
}

bool MP4File::SetMetadataDisk(u_int16_t disk, u_int16_t totalDisks)
{
    unsigned char t[9];
    const char *s = "moov.udta.meta.ilst.disk.data";
    MP4BytesProperty *pMetadataProperty = NULL;
    MP4Atom *pMetaAtom = NULL;
    
    pMetaAtom = m_pRootAtom->FindAtom(s);

    if (!pMetaAtom)
    {
        if (!CreateMetadataAtom("disk"))
            return false;

        pMetaAtom = m_pRootAtom->FindAtom(s);
    }

    memset(t, 0, 9*sizeof(unsigned char));
    t[2] = (unsigned char)(disk>>8)&0xFF;
    t[3] = (unsigned char)(disk)&0xFF;
    t[4] = (unsigned char)(totalDisks>>8)&0xFF;
    t[5] = (unsigned char)(totalDisks)&0xFF;

    pMetaAtom->FindProperty("data.metadata", (MP4Property**)&pMetadataProperty);
    ASSERT(pMetadataProperty);

    pMetadataProperty->SetValue((u_int8_t*)t, 8);

    return true;
}

bool MP4File::GetMetadataDisk(u_int16_t* disk, u_int16_t* totalDisks)
{
    unsigned char *val = NULL;
    u_int32_t valSize = 0;
    const char *s = "moov.udta.meta.ilst.disk.data.metadata";
    GetBytesProperty(s, (u_int8_t**)&val, &valSize);

    *disk = 0;
    *totalDisks = 0;

    if (valSize != 8)
        return false;

    *disk = (u_int16_t)(val[3]);
    *disk += (u_int16_t)(val[2]<<8);
    *totalDisks = (u_int16_t)(val[5]);
    *totalDisks += (u_int16_t)(val[4]<<8);

    return true;
}

static const char* ID3v1GenreList[] = {
    "Blues", "Classic Rock", "Country", "Dance", "Disco", "Funk",
    "Grunge", "Hip-Hop", "Jazz", "Metal", "New Age", "Oldies",
    "Other", "Pop", "R&B", "Rap", "Reggae", "Rock",
    "Techno", "Industrial", "Alternative", "Ska", "Death Metal", "Pranks",
    "Soundtrack", "Euro-Techno", "Ambient", "Trip-Hop", "Vocal", "Jazz+Funk",
    "Fusion", "Trance", "Classical", "Instrumental", "Acid", "House",
    "Game", "Sound Clip", "Gospel", "Noise", "AlternRock", "Bass",
    "Soul", "Punk", "Space", "Meditative", "Instrumental Pop", "Instrumental Rock",
    "Ethnic", "Gothic", "Darkwave", "Techno-Industrial", "Electronic", "Pop-Folk",
    "Eurodance", "Dream", "Southern Rock", "Comedy", "Cult", "Gangsta",
    "Top 40", "Christian Rap", "Pop/Funk", "Jungle", "Native American", "Cabaret",
    "New Wave", "Psychadelic", "Rave", "Showtunes", "Trailer", "Lo-Fi",
    "Tribal", "Acid Punk", "Acid Jazz", "Polka", "Retro", "Musical",
    "Rock & Roll", "Hard Rock", "Folk", "Folk/Rock", "National Folk", "Swing",
    "Fast-Fusion", "Bebob", "Latin", "Revival", "Celtic", "Bluegrass", "Avantgarde",
    "Gothic Rock", "Progressive Rock", "Psychedelic Rock", "Symphonic Rock", "Slow Rock", "Big Band",
    "Chorus", "Easy Listening", "Acoustic", "Humour", "Speech", "Chanson",
    "Opera", "Chamber Music", "Sonata", "Symphony", "Booty Bass", "Primus",
    "Porn Groove", "Satire", "Slow Jam", "Club", "Tango", "Samba",
    "Folklore", "Ballad", "Power Ballad", "Rhythmic Soul", "Freestyle", "Duet",
    "Punk Rock", "Drum Solo", "A capella", "Euro-House", "Dance Hall",
    "Goa", "Drum & Bass", "Club House", "Hardcore", "Terror",
    "Indie", "BritPop", "NegerPunk", "Polsk Punk", "Beat",
    "Christian Gangsta", "Heavy Metal", "Black Metal", "Crossover", "Contemporary C",
    "Christian Rock", "Merengue", "Salsa", "Thrash Metal", "Anime", "JPop",
    "SynthPop",
};

int GenreToString(char** GenreStr, const int genre)
{
    if (genre > 0 && genre <= sizeof(ID3v1GenreList)/sizeof(*ID3v1GenreList))
    {
        *GenreStr = (char*)malloc((strlen(ID3v1GenreList[genre-1])+1)*sizeof(char));
        memset(*GenreStr, 0, (strlen(ID3v1GenreList[genre-1])+1)*sizeof(char));
        strcpy(*GenreStr, ID3v1GenreList[genre-1]);
        return 0;
    } else {
        *GenreStr = (char*)malloc(2*sizeof(char));
        memset(*GenreStr, 0, 2*sizeof(char));
        return 1;
    }
}

int StringToGenre(const char* GenreStr)
{
    int i;

    for (i = 0; i < sizeof(ID3v1GenreList)/sizeof(*ID3v1GenreList); i++)
    {
        if (strcasecmp(GenreStr, ID3v1GenreList[i]) == 0)
            return i+1;
    }
    return 0;
}

bool MP4File::SetMetadataGenre(const char* value)
{
    u_int16_t genreIndex = 0;
    unsigned char t[3];
    MP4BytesProperty *pMetadataProperty = NULL;
    MP4Atom *pMetaAtom = NULL;

    genreIndex = StringToGenre(value);

    if (genreIndex != 0)
    {
        const char *s = "moov.udta.meta.ilst.gnre.data";
        pMetaAtom = m_pRootAtom->FindAtom(s);

        if (!pMetaAtom)
        {
            if (!CreateMetadataAtom("gnre"))
                return false;

            pMetaAtom = m_pRootAtom->FindAtom(s);
        }

        memset(t, 0, 3*sizeof(unsigned char));
        t[0] = (unsigned char)(genreIndex>>8)&0xFF;
        t[1] = (unsigned char)(genreIndex)&0xFF;

        pMetaAtom->FindProperty("data.metadata", (MP4Property**)&pMetadataProperty);
        ASSERT(pMetadataProperty);

        pMetadataProperty->SetValue((u_int8_t*)t, 2);

        return true;
    } else {
        const char *s2 = "moov.udta.meta.ilst.©gen.data";
        pMetaAtom = m_pRootAtom->FindAtom(s2);

        if (!pMetaAtom)
        {
            if (!CreateMetadataAtom("©gen"))
                return false;

            pMetaAtom = m_pRootAtom->FindAtom(s2);
        }

        pMetaAtom->FindProperty("data.metadata", (MP4Property**)&pMetadataProperty);
        ASSERT(pMetadataProperty);

        pMetadataProperty->SetValue((u_int8_t*)value, strlen(value));

        return true;
    }

    return false;
}

bool MP4File::GetMetadataGenre(char** value)
{
    u_int16_t genreIndex = 0;
    unsigned char *val = NULL;
    u_int32_t valSize = 0;
    const char *t = "moov.udta.meta.ilst.gnre";
    const char *s = "moov.udta.meta.ilst.gnre.data.metadata";

    MP4Atom *gnre = FindAtom(t);

    if (gnre)
    {
        GetBytesProperty(s, (u_int8_t**)&val, &valSize);

        if (valSize != 2)
            return false;

        genreIndex = (u_int16_t)(val[1]);
        genreIndex += (u_int16_t)(val[0]<<8);

        GenreToString(value, genreIndex);

        return true;
    } else {
        const char *s2 = "moov.udta.meta.ilst.©gen.data.metadata";

        val = NULL;
        valSize = 0;

        GetBytesProperty(s2, (u_int8_t**)&val, &valSize);

        if (valSize > 0)
        {
            *value = (char*)malloc((valSize+1)*sizeof(unsigned char));
            memset(*value, 0, (valSize+1)*sizeof(unsigned char));
            memcpy(*value, val, valSize*sizeof(unsigned char));
            return true;
        } else {
            *value = NULL;
            return false;
        }
    }

    return false;
}

bool MP4File::SetMetadataTempo(u_int16_t tempo)
{
    unsigned char t[3];
    const char *s = "moov.udta.meta.ilst.tmpo.data";
    MP4BytesProperty *pMetadataProperty = NULL;
    MP4Atom *pMetaAtom = NULL;
    
    pMetaAtom = m_pRootAtom->FindAtom(s);

    if (!pMetaAtom)
    {
        if (!CreateMetadataAtom("tmpo"))
            return false;

        pMetaAtom = m_pRootAtom->FindAtom(s);
    }

    memset(t, 0, 3*sizeof(unsigned char));
    t[0] = (unsigned char)(tempo>>8)&0xFF;
    t[1] = (unsigned char)(tempo)&0xFF;

    pMetaAtom->FindProperty("data.metadata", (MP4Property**)&pMetadataProperty);
    ASSERT(pMetadataProperty);

    pMetadataProperty->SetValue((u_int8_t*)t, 2);

    return true;
}

bool MP4File::GetMetadataTempo(u_int16_t* tempo)
{
    unsigned char *val = NULL;
    u_int32_t valSize = 0;
    const char *s = "moov.udta.meta.ilst.tmpo.data.metadata";
    GetBytesProperty(s, (u_int8_t**)&val, &valSize);

    *tempo = 0;

    if (valSize != 2)
        return false;

    *tempo = (u_int16_t)(val[1]);
    *tempo += (u_int16_t)(val[0]<<8);

    return true;
}

bool MP4File::SetMetadataCompilation(u_int8_t compilation)
{
    const char *s = "moov.udta.meta.ilst.cpil.data";
    MP4BytesProperty *pMetadataProperty = NULL;
    MP4Atom *pMetaAtom = NULL;
    
    pMetaAtom = m_pRootAtom->FindAtom(s);

    if (!pMetaAtom)
    {
        if (!CreateMetadataAtom("cpil"))
            return false;

        pMetaAtom = m_pRootAtom->FindAtom(s);
    }

    pMetaAtom->FindProperty("data.metadata", (MP4Property**)&pMetadataProperty);
    ASSERT(pMetadataProperty);

    compilation &= 0x1;
    pMetadataProperty->SetValue((u_int8_t*)&compilation, 1);

    return true;
}

bool MP4File::GetMetadataCompilation(u_int8_t* compilation)
{
    unsigned char *val = NULL;
    u_int32_t valSize = 0;
    const char *s = "moov.udta.meta.ilst.cpil.data.metadata";
    GetBytesProperty(s, (u_int8_t**)&val, &valSize);

    *compilation = 0;

    if (valSize != 1)
        return false;

    *compilation = (u_int16_t)(val[0]);

    return true;
}

bool MP4File::SetMetadataCoverArt(u_int8_t *coverArt, u_int32_t size)
{
    const char *s = "moov.udta.meta.ilst.covr.data";
    MP4BytesProperty *pMetadataProperty = NULL;
    MP4Atom *pMetaAtom = NULL;
    
    pMetaAtom = m_pRootAtom->FindAtom(s);

    if (!pMetaAtom)
    {
        if (!CreateMetadataAtom("covr"))
            return false;

        pMetaAtom = m_pRootAtom->FindAtom(s);
    }

    pMetaAtom->FindProperty("data.metadata", (MP4Property**)&pMetadataProperty);
    ASSERT(pMetadataProperty);

    pMetadataProperty->SetValue(coverArt, size);

    return true;
}

bool MP4File::GetMetadataCoverArt(u_int8_t **coverArt, u_int32_t *size)
{
    const char *s = "moov.udta.meta.ilst.covr.data.metadata";
    GetBytesProperty(s, coverArt, size);

    if (size == 0)
        return false;

    return true;
}

bool MP4File::SetMetadataFreeForm(char *name, u_int8_t* pValue, u_int32_t valueSize)
{
    MP4Atom *pMetaAtom = NULL;
    MP4BytesProperty *pMetadataProperty = NULL;
    char s[256];
    int i = 0;
    bool nameExists = false;

    while (1)
    {
        MP4BytesProperty *pMetadataProperty;

        sprintf(s, "moov.udta.meta.ilst.----[%u].name", i);

        MP4Atom *pTagAtom = m_pRootAtom->FindAtom(s);

        if (!pTagAtom)
            break;

        pTagAtom->FindProperty("name.metadata", (MP4Property**)&pMetadataProperty);
        if (pMetadataProperty)
        {
            u_int8_t* pV;
            u_int32_t VSize = 0;

            pMetadataProperty->GetValue(&pV, &VSize);

            if (VSize != 0)
            {
                if (memcmp(pV, name, VSize) == 0)
                {
                    sprintf(s, "moov.udta.meta.ilst.----[%u].data.metadata", i);
                    SetBytesProperty(s, pValue, valueSize);

                    return true;
                }
            }
        }

        i++;
    }

    /* doesn't exist yet, create it */
    char t[256];

    sprintf(t, "udta.meta.ilst.----[%u]", i);
    sprintf(s, "moov.udta.meta.ilst.----[%u].data", i);
    AddDescendantAtoms("moov", t);

    pMetaAtom = m_pRootAtom->FindAtom(s);

    if (!pMetaAtom)
        return false;

    pMetaAtom->SetFlags(0x1);

    MP4Atom *pHdlrAtom = m_pRootAtom->FindAtom("moov.udta.meta.hdlr");
    MP4StringProperty *pStringProperty = NULL;
    MP4BytesProperty *pBytesProperty = NULL;
    ASSERT(pHdlrAtom);

    pHdlrAtom->FindProperty(
        "hdlr.handlerType", (MP4Property**)&pStringProperty);
    ASSERT(pStringProperty);
    pStringProperty->SetValue("mdir");

    u_int8_t val[12];
    memset(val, 0, 12*sizeof(u_int8_t));
    val[0] = 0x61;
    val[1] = 0x70;
    val[2] = 0x70;
    val[3] = 0x6c;
    pHdlrAtom->FindProperty(
        "hdlr.reserved2", (MP4Property**)&pBytesProperty);
    ASSERT(pBytesProperty);
    pBytesProperty->SetReadOnly(false);
    pBytesProperty->SetValue(val, 12);
    pBytesProperty->SetReadOnly(true);

    pMetaAtom = m_pRootAtom->FindAtom(s);
    pMetaAtom->FindProperty("data.metadata", (MP4Property**)&pMetadataProperty);
    ASSERT(pMetadataProperty);
    pMetadataProperty->SetValue(pValue, valueSize);

    sprintf(s, "moov.udta.meta.ilst.----[%u].name", i);
    pMetaAtom = m_pRootAtom->FindAtom(s);
    pMetaAtom->FindProperty("name.metadata", (MP4Property**)&pMetadataProperty);
    ASSERT(pMetadataProperty);
    pMetadataProperty->SetValue((u_int8_t*)name, strlen(name));

    sprintf(s, "moov.udta.meta.ilst.----[%u].mean", i);
    pMetaAtom = m_pRootAtom->FindAtom(s);
    pMetaAtom->FindProperty("mean.metadata", (MP4Property**)&pMetadataProperty);
    ASSERT(pMetadataProperty);
    pMetadataProperty->SetValue((u_int8_t*)"com.apple.iTunes", 16); /* ?? */

    return true;
}

bool MP4File::GetMetadataFreeForm(char *name, u_int8_t** ppValue, u_int32_t *pValueSize)
{
    char s[256];
    int i = 0;

    while (1)
    {
        MP4BytesProperty *pMetadataProperty;

        sprintf(s, "moov.udta.meta.ilst.----[%u].name", i);

        MP4Atom *pTagAtom = m_pRootAtom->FindAtom(s);

        if (!pTagAtom)
            return false;

        pTagAtom->FindProperty("name.metadata", (MP4Property**)&pMetadataProperty);
        if (pMetadataProperty)
        {
            u_int8_t* pV;
            u_int32_t VSize = 0;

            pMetadataProperty->GetValue(&pV, &VSize);

            if (VSize != 0)
            {
                if (memcmp(pV, name, VSize) == 0)
                {
                    sprintf(s, "moov.udta.meta.ilst.----[%u].data.metadata", i);
                    GetBytesProperty(s, ppValue, pValueSize);

                    return true;
                }
            }
        }

        i++;
    }
}

bool MP4File::MetadataDelete()
{
    MP4Atom *pMetaAtom = NULL;
    char s[256];

    sprintf(s, "moov.udta.meta");
    pMetaAtom = m_pRootAtom->FindAtom(s);

    /* if it exists, delete it */
    if (pMetaAtom)
    {
        MP4Atom *pParent = pMetaAtom->GetParentAtom();

        pParent->DeleteChildAtom(pMetaAtom);

        delete pMetaAtom;

        return true;
    }

    return false;
}
