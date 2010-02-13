/********************************************************************
*    
* Copyright (c) 2002 Artur Polaczynski (Ar't)  All rights reserved.
*            <artii@o2.pl>        LGPL-2.1
*       $ArtId: id3v2_read.c,v 1.16 2003/04/13 11:24:10 art Exp $
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


#ifdef ID3V2_READ

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <math.h>
#ifndef __BORLANDC__
#    include <unistd.h>
#endif

#include "apetaglib.h"
#include <id3.h>

struct id3vtwo2ape {
    ID3_FrameID frame;      //ID3FID_ALBUM etc
    ID3_FieldID field_type; //ID3FN_TEXT etc
    const char *APEName;
    int         special;    // 0-no 1-???
};

#define APETAG_TYPE_COMMENT 1
#define APETAG_TYPE_USER    2
#define APETAG_TYPE_GENRE   3


struct id3vtwo2ape convert[] = {
    {ID3FID_ALBUM,             ID3FN_TEXT, APE_TAG_FIELD_ALBUM,0},
    {ID3FID_BAND,              ID3FN_TEXT, "Band",0},
    {ID3FID_BPM,               ID3FN_TEXT, "BPM",0},
    {ID3FID_COMPOSER,          ID3FN_TEXT, APE_TAG_FIELD_COMPOSER,0},
    {ID3FID_CONDUCTOR,         ID3FN_TEXT, APE_TAG_FIELD_CONDUCTOR,0},
    {ID3FID_CONTENTGROUP,      ID3FN_TEXT, "Content Group",0},
    {ID3FID_COPYRIGHT,         ID3FN_TEXT, APE_TAG_FIELD_COPYRIGHT,0},
    {ID3FID_DATE,              ID3FN_TEXT, APE_TAG_FIELD_RECORDDATE,0},
    {ID3FID_ENCODEDBY,         ID3FN_TEXT, "Encoded By",0},
    {ID3FID_ENCODERSETTINGS,   ID3FN_TEXT, "Encoder",0},
    {ID3FID_FILEOWNER,         ID3FN_TEXT, "File Owner",0},
    {ID3FID_FILETYPE,          ID3FN_TEXT, "File Type",0},
    {ID3FID_INITIALKEY,        ID3FN_TEXT, "Initial Key",0},
    {ID3FID_ISRC,              ID3FN_TEXT, APE_TAG_FIELD_ISRC,0},
    {ID3FID_LANGUAGE,          ID3FN_TEXT, "Language",0},
    {ID3FID_LEADARTIST,        ID3FN_TEXT, APE_TAG_FIELD_ARTIST,0},
    {ID3FID_LYRICIST,          ID3FN_TEXT, "Lyricist",0},
    {ID3FID_MEDIATYPE,         ID3FN_TEXT, APE_TAG_FIELD_MEDIA,0},
    {ID3FID_MIXARTIST,         ID3FN_TEXT, "Mix Artist",0},
    {ID3FID_NETRADIOOWNER,     ID3FN_TEXT, "Internet Radio Owner",0},
    {ID3FID_NETRADIOSTATION,   ID3FN_TEXT, "Internet Radio Station",0},
    {ID3FID_ORIGALBUM,         ID3FN_TEXT, "Original Album",0},
    {ID3FID_ORIGARTIST,        ID3FN_TEXT, "Original Artist",0},
    {ID3FID_ORIGFILENAME,      ID3FN_TEXT, "Original Filename",0},
    {ID3FID_ORIGLYRICIST,      ID3FN_TEXT, "Original Lyricist",0},
    {ID3FID_ORIGYEAR,          ID3FN_TEXT, "Original Artist",0},
    {ID3FID_PARTINSET,         ID3FN_TEXT, "Part",0},
    {ID3FID_PLAYLISTDELAY,     ID3FN_TEXT, "Playlist Delay",0},
    {ID3FID_PUBLISHER,         ID3FN_TEXT, APE_TAG_FIELD_PUBLISHER,0},
    {ID3FID_RECORDINGDATES,    ID3FN_TEXT, APE_TAG_FIELD_RECORDDATE,0},
    {ID3FID_SIZE,              ID3FN_TEXT, "Size",0},
//    {ID3FID_SONGLEN,           ID3FN_TEXT, "Song Length",0}, // don't like this in apetag
    {ID3FID_SUBTITLE,          ID3FN_TEXT, APE_TAG_FIELD_SUBTITLE,0},
    {ID3FID_TIME,              ID3FN_TEXT, "Time",0},
    {ID3FID_TITLE,             ID3FN_TEXT, APE_TAG_FIELD_TITLE,0},
    {ID3FID_TRACKNUM,          ID3FN_TEXT, APE_TAG_FIELD_TRACK,0},
    {ID3FID_YEAR,              ID3FN_TEXT, APE_TAG_FIELD_YEAR,0},
 
    {ID3FID_WWWAUDIOFILE,      ID3FN_URL , APE_TAG_FIELD_FILE_URL,0},
    {ID3FID_WWWARTIST,         ID3FN_URL , APE_TAG_FIELD_ARTIST_URL,0},
    {ID3FID_WWWAUDIOSOURCE,    ID3FN_URL , "Source URL",0},
    {ID3FID_WWWCOMMERCIALINFO, ID3FN_URL , APE_TAG_FIELD_BUY_URL,0},
    {ID3FID_WWWCOPYRIGHT,      ID3FN_URL , APE_TAG_FIELD_COPYRIGHT_URL,0},
    {ID3FID_WWWPUBLISHER,      ID3FN_URL , APE_TAG_FIELD_PUBLISHER_URL,0},
    {ID3FID_WWWPAYMENT,        ID3FN_URL , "Payment",0},
    {ID3FID_WWWRADIOPAGE,      ID3FN_URL , "Web Radio URL",0},
 
    {ID3FID_COMMENT,           ID3FN_TEXT, APE_TAG_FIELD_COMMENT,      APETAG_TYPE_COMMENT},
    {ID3FID_UNSYNCEDLYRICS,    ID3FN_TEXT, APE_TAG_FIELD_LYRICS,       APETAG_TYPE_COMMENT},
    {ID3FID_USERTEXT,          ID3FN_TEXT, "dummy",                    APETAG_TYPE_USER},
    {ID3FID_WWWUSER,           ID3FN_URL , APE_TAG_FIELD_RELATED_URL,  APETAG_TYPE_USER},
    {ID3FID_CONTENTTYPE,       ID3FN_TEXT, APE_TAG_FIELD_GENRE,        APETAG_TYPE_GENRE},
 
 
 
//    {ID3FID_PICTURE,          ID3FN_DATA, ,0},
//    {ID3FID_SYNCEDLYRICS,     ID3FN_DATA, APE_TAG_FIELD_LYRICS,0},
//    {ID3FID_INVOLVEDPEOPLE,   ID3FN_DATA, "Involved People",0}, // type ?
//    {ID3FID_CDID,             ID3FN_DATA, "CDID",0}, // ???
//    {ID3FID_TERMSOFUSE,       ID3FN_TEXT, "Terms Of Use",0}, // type ?
};

int
libapetag_convertID3v2toAPE(const ID3Frame * frame, 
                    char **item_, size_t *item_len, 
                    char **value_, size_t *value_len,
                    unsigned long *flags);


/*
    use ALOCATE to alocate dynamic memory for frame value
    this check size of frame and alocate mem and zeroed this mem
*/

#define  ALOCATE(FielD,ValuE,SizeValuE) \
    SizeValuE = ID3Field_Size(FielD);  \
    ValuE = (SizeValuE!=0) ? (char *) malloc((SizeValuE)+1) : NULL ; \
    if ((SizeValuE)!=0) ValuE[0] = '\0';
/*
    ALOCATE_ITEM its the same ase ALOCATE but for frame name (Item)
*/
#define  ALOCATE_ITEM(IteM, APENamE, ItemSizE) \
    ItemSizE = strlen(APENamE) ; \
    IteM = (ItemSizE!=0) ? (char *) malloc((ItemSizE)+1) : NULL ; \
    if ((ItemSizE)!=0) IteM[0] = '\0';

int
libapetag_convertID3v2toAPE (const ID3Frame * frame,
                 char **item_, size_t * item_len,
                 char **value_, size_t * value_len,
                 unsigned long *flags)
{
    ID3Field *text;
    ID3Field *desc;
    ID3Field *url;
//      ID3Field *bin;  // will be implemented some day

    char *item = NULL;
    char *value = NULL;
    
    ID3_FrameID frameid = ID3Frame_GetID (frame);
    unsigned int i;
    
    *flags = ITEM_TEXT;
    
    for (i = 0; i < sizeof (convert) / sizeof (struct id3vtwo2ape); i++)
        if (frameid == convert[i].frame)
            break;


    if (convert[i].field_type == ID3FN_TEXT) {
        switch (convert[i].special) {
        case APETAG_TYPE_COMMENT: /* Comments and unsynced lyrics */
            if ((text = ID3Frame_GetField(frame, ID3FN_TEXT)) != NULL) {
                ALOCATE(text, value, *value_len);
                ID3Field_GetASCII(text, value, *value_len);
            }
            ALOCATE_ITEM(item, convert[i].APEName, *item_len);
            strncpy(item, convert[i].APEName, *item_len);
            item[*item_len]='\0';
            //break;
            if ((text = ID3Frame_GetField (frame, ID3FN_DESCRIPTION)) != NULL) {
                char *value_ds=NULL;
                int value_len2;
                if (ID3Field_Size(text) != 0) {
                    ALOCATE(text, value_ds, value_len2);
                    ID3Field_GetASCII(text, value_ds, value_len2);
                    if ( strcmp(value_ds, STR_V1_COMMENT_DESC) == 0 ) {
                        value_len2 = 0;
                        value[0]='\0';
                    } else {
                        item = (char *) realloc( item, (*item_len) + value_len2 + 3);
                        item[(*item_len)++]='-'; item[(*item_len)]='\0';
                        strncpy(item + (*item_len),value_ds ,(value_len2 + 1));
                        (*item_len)+=value_len2;
                    }
                    free(value_ds);
                }
            }
            break;
            
        case APETAG_TYPE_USER:  /* User texts */
            if ((text = ID3Frame_GetField(frame, ID3FN_TEXT)) != NULL) {
                ALOCATE(text, value, *value_len);
                ID3Field_GetASCII(text, value, *value_len);
            }
            if ((desc = ID3Frame_GetField(frame, ID3FN_DESCRIPTION)) != NULL) {
                ALOCATE(desc, item, *item_len);
                ID3Field_GetASCII(desc, item, *item_len);
            }
            break;
            
        case APETAG_TYPE_GENRE: /* genre */ 
            if ((text = ID3Frame_GetField(frame, ID3FN_TEXT)) != NULL) {
                char *p;
                int j;
                ALOCATE(text, value, *value_len);
                ID3Field_GetASCII(text, value, *value_len);
                ALOCATE_ITEM(item, convert[i].APEName, *item_len);
                strncpy(item, convert[i].APEName, *item_len);
                value[*value_len]='\0';
                p = value;
                if (*p == '(') {
                    p++;
                    while (*p && (*p >= '0' && *p <= '9'))
                        p++;
                    if (*p && *p == ')') {
                        p++;
                    } else {
                        p = value;
                    }
                    *value_len -= (p-value); // corect lenght of frame
                    if (*p != '\0') { // copy in place
                        for (j = 0; *p != '\0'; j++) {
                            value[j] = *p;
                            p++;
                        }
                        value[j] = '\0';
                    }
                }
            }
            break;
            
        default:    /* normal text tags */
            if ((text = ID3Frame_GetField(frame, ID3FN_TEXT)) != NULL) {
                ALOCATE(text, value, *value_len);
                ID3Field_GetASCII(text, value, *value_len);
                ALOCATE_ITEM(item, convert[i].APEName, *item_len);
                strncpy(item, convert[i].APEName, *item_len);
            }
            break;
            
        } /* <- switch( convert[i].special ) */
        
        item[*item_len]='\0';
        value[*value_len]='\0';
    } else 
    if (convert[i].field_type == ID3FN_URL) {
        *flags = ITEM_URL;
        /* TODO: set ape_tag_URL in flag */
        /* user url */
        if (convert[i].special == APETAG_TYPE_USER) {
            if ((url = ID3Frame_GetField(frame, ID3FN_URL)) != NULL) {
                ALOCATE(url, value, *value_len);
                ID3Field_GetASCII(url, value, *value_len);
            }
            if ((desc = ID3Frame_GetField(frame, ID3FN_DESCRIPTION)) != NULL) {
                ALOCATE(desc, item, *item_len);
                ID3Field_GetASCII(desc, item, *item_len);
            }
            /* normal url */
        } else {
            if ((url = ID3Frame_GetField (frame, ID3FN_URL)) != NULL) {
                ALOCATE(url, value, *value_len);
                ID3Field_GetASCII(url, value, *value_len);
                ALOCATE_ITEM(item, convert[i].APEName, *item_len);
                strncpy(item, convert[i].APEName, *item_len);
            }
        }
        
        item[*item_len]='\0';
        value[*value_len]='\0';
    } else {        //convert[i].field_type
        item = NULL;
        value = NULL;
        PRINT_D (">id3v2_read>other\n");
    }
    *item_ = item;
    *value_ = value;
    
    if (!(value==NULL || (*value_len)==0) && value[(*value_len)-1]=='\0')
        (*value_len)--;

    return 0;
}

// Reads ID3v2.x tag
// idea of this come from "tag" by Case <case@mobiili.net>
int readtag_id3v2 ( apetag *mem_cnt, char* fileName )
{
    ID3Tag*             tag;
    ID3Frame*           frame;
    ID3TagIterator*     iter;
    char*               item = NULL;
    int                 itemSize;
    char*               value = NULL;
    int                 valueSize;
    unsigned long       flags;

    // first - check of id3tag v2 and init 
    if ( (tag = ID3Tag_New ()) == NULL )
    return 1;
    // on some casses its weerrryyy slooowwwwlyyy 65k file take 2-5 sec 
    ID3Tag_LinkWithFlags ( tag, fileName, ID3TT_ID3V2 );
    
    if ( tag == NULL ) {
    ID3Tag_Delete (tag);
    return 0;
    }

    if ( !ID3Tag_HasTagType ( tag, ID3TT_ID3V2 ) ) {
    ID3Tag_Delete (tag);
    return 0;
    }

    if ( (iter = ID3Tag_CreateIterator (tag)) == NULL ) {
    ID3Tag_Delete (tag);
    return 0;
    }

    while ( (frame = ID3TagIterator_GetNext (iter)) != NULL ) {

    libapetag_convertID3v2toAPE ( frame, &item, &itemSize, &value, &valueSize, &flags);

    if ( !item || !value || item[0] == '\0' || value[0] == '\0' )
        continue;
    
    if ( !value || value[0] != '\0' ) {
        PRINT_D4(">id3v2_read>[i%i]%s: [v%i]'%s'\n",itemSize,item,valueSize,value);
        if ( apefrm_getstr (mem_cnt, item) == NULL ) /* noreplece !!! */
            apefrm_add_bin (mem_cnt, flags, itemSize, item, valueSize ,value);
    }
    free ( value );
    free ( item );
    }

    ID3TagIterator_Delete (iter);
    ID3Tag_Delete (tag);

    return 0;
}

#endif // ID3V2_READ

    

#if 0
static  ID3_FrameDef ID3_FrameDefs[] =
{
  /* frames to implement */
  //                          short  long   
  // frame id                 id     id    field defs           description
  {ID3FID_AUDIOCRYPTO,       "CRA", "AENC",ID3FD_Unimplemented, "Audio encryption"},
  {ID3FID_BUFFERSIZE,        "BUF", "RBUF",ID3FD_Unimplemented, "Recommended buffer size"},
  {ID3FID_CDID,              "MCI", "MCDI",ID3FD_Unimplemented, "Music CD identifier"},
  {ID3FID_COMMERCIAL,        ""   , "COMR",ID3FD_Unimplemented, "Commercial"},
  {ID3FID_CRYPTOREG,         ""   , "ENCR",ID3FD_Registration,  "Encryption method registration"},
  {ID3FID_EQUALIZATION,      "EQU", "EQUA",ID3FD_Unimplemented, "Equalization"},
  {ID3FID_EVENTTIMING,       "ETC", "ETCO",ID3FD_Unimplemented, "Event timing codes"},
  {ID3FID_GENERALOBJECT,     "GEO", "GEOB",ID3FD_GEO,           "General encapsulated object"},
  {ID3FID_GROUPINGREG,       ""   , "GRID",ID3FD_Registration,  "Group identification registration"},
  {ID3FID_INVOLVEDPEOPLE,    "IPL", "IPLS",ID3FD_InvolvedPeople,"Involved people list"},
  {ID3FID_LINKEDINFO,        "LNK", "LINK",ID3FD_LinkedInfo,    "Linked information"},
  {ID3FID_METACOMPRESSION,   "CDM", ""    ,ID3FD_CDM,           "Compressed data meta frame"},
  {ID3FID_METACRYPTO,        "CRM", ""    ,ID3FD_Unimplemented, "Encrypted meta frame"},
  {ID3FID_MPEGLOOKUP,        "MLL", "MLLT",ID3FD_Unimplemented, "MPEG location lookup table"},
  {ID3FID_OWNERSHIP,         ""   , "OWNE",ID3FD_Unimplemented, "Ownership frame"},
  {ID3FID_PICTURE,           "PIC", "APIC",ID3FD_Picture,       "Attached picture"},
  {ID3FID_PLAYCOUNTER,       "CNT", "PCNT",ID3FD_PlayCounter,   "Play counter"},
  {ID3FID_POPULARIMETER,     "POP", "POPM",ID3FD_Popularimeter, "Popularimeter"},
  {ID3FID_POSITIONSYNC,      ""   , "POSS",ID3FD_Unimplemented, "Position synchronisation frame"},
  {ID3FID_PRIVATE,           ""   , "PRIV",ID3FD_Private,       "Private frame"},
  {ID3FID_REVERB,            "REV", "RVRB",ID3FD_Unimplemented, "Reverb"},
  {ID3FID_SYNCEDLYRICS,      "SLT", "SYLT",ID3FD_SyncLyrics,    "Synchronized lyric/text"},
  {ID3FID_SYNCEDTEMPO,       "STC", "SYTC",ID3FD_Unimplemented, "Synchronized tempo codes"},
  {ID3FID_TERMSOFUSE,        ""   , "USER",ID3FD_TermsOfUse,    "Terms of use"},
  {ID3FID_UNIQUEFILEID,      "UFI", "UFID",ID3FD_UFI,           "Unique file identifier"},
  {ID3FID_VOLUMEADJ,         "RVA", "RVAD",ID3FD_Unimplemented, "Relative volume adjustment"},

};
#endif // 0
