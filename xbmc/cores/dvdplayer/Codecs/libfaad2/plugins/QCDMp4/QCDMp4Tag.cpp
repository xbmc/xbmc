#include <mp4.h>
#include <faad.h>
#include "QCDTagsDLL.h"


//..............................................................................
// Global Variables

typedef struct tag
{
    char *item;
    char *value;
} tag;

typedef struct medialib_tags
{
    struct tag *tags;
    unsigned int count;
} medialib_tags;

int tag_add_field(medialib_tags *tags, const char *item, const char *value)
{
    void *backup = (void *)tags->tags;

    if (!item || (item && !*item) || !value) return 0;

    tags->tags = (struct tag *)realloc(tags->tags, (tags->count+1) * sizeof(tag));
    if (!tags->tags) {
        if (backup) free(backup);
        return 0;
    }
    else
    {
        int i_len = strlen(item);
        int v_len = strlen(value);

        tags->tags[tags->count].item = (char *)malloc(i_len+1);
        tags->tags[tags->count].value = (char *)malloc(v_len+1);

        if (!tags->tags[tags->count].item || !tags->tags[tags->count].value)
        {
            if (!tags->tags[tags->count].item) free (tags->tags[tags->count].item);
            if (!tags->tags[tags->count].value) free (tags->tags[tags->count].value);
            tags->tags[tags->count].item = NULL;
            tags->tags[tags->count].value = NULL;
            return 0;
        }

        memcpy(tags->tags[tags->count].item, item, i_len);
        memcpy(tags->tags[tags->count].value, value, v_len);
        tags->tags[tags->count].item[i_len] = '\0';
        tags->tags[tags->count].value[v_len] = '\0';

        tags->count++;
        return 1;
    }
}

int tag_set_field(medialib_tags *tags, const char *item, const char *value)
{
    unsigned int i;

    if (!item || (item && !*item) || !value) return 0;

    for (i = 0; i < tags->count; i++)
    {
        if (!stricmp(tags->tags[i].item, item))
        {
            void *backup = (void *)tags->tags[i].value;
            int v_len = strlen(value);

            tags->tags[i].value = (char *)realloc(tags->tags[i].value, v_len+1);
            if (!tags->tags[i].value)
            {
                if (backup) free(backup);
                return 0;
            }

            memcpy(tags->tags[i].value, value, v_len);
            tags->tags[i].value[v_len] = '\0';

            return 1;
        }
    }

    return tag_add_field(tags, item, value);
}

void tag_delete(medialib_tags *tags)
{
    unsigned int i;

    for (i = 0; i < tags->count; i++)
    {
        if (tags->tags[i].item) free(tags->tags[i].item);
        if (tags->tags[i].value) free(tags->tags[i].value);
    }

    if (tags->tags) free(tags->tags);

    tags->tags = NULL;
    tags->count = 0;
}

int ReadMP4Tag(MP4FileHandle file, medialib_tags *tags)
{
    unsigned __int32 valueSize;
    unsigned __int8 *pValue;
    char *pName;
    unsigned int i = 0;

    do {
        pName = 0;
        pValue = 0;
        valueSize = 0;

        MP4GetMetadataByIndex(file, i, (const char **)&pName, &pValue, &valueSize);

        if (valueSize > 0)
        {
            char *val = (char *)malloc(valueSize+1);
            if (!val) return 0;
            memcpy(val, pValue, valueSize);
            val[valueSize] = '\0';

            if (pName[0] == '\xa9')
            {
                if (memcmp(pName, "©nam", 4) == 0)
                {
                    tag_add_field(tags, "title", val);
                } else if (memcmp(pName, "©ART", 4) == 0) {
                    tag_add_field(tags, "artist", val);
                } else if (memcmp(pName, "©wrt", 4) == 0) {
                    tag_add_field(tags, "writer", val);
                } else if (memcmp(pName, "©alb", 4) == 0) {
                    tag_add_field(tags, "album", val);
                } else if (memcmp(pName, "©day", 4) == 0) {
                    tag_add_field(tags, "date", val);
                } else if (memcmp(pName, "©too", 4) == 0) {
                    tag_add_field(tags, "tool", val);
                } else if (memcmp(pName, "©cmt", 4) == 0) {
                    tag_add_field(tags, "comment", val);
                } else if (memcmp(pName, "©gen", 4) == 0) {
                    tag_add_field(tags, "genre", val);
                } else {
                    tag_add_field(tags, pName, val);
                }
            } else if (memcmp(pName, "gnre", 4) == 0) {
                char *t=0;
                if (MP4GetMetadataGenre(file, &t))
                {
                    tag_add_field(tags, "genre", t);
                }
            } else if (memcmp(pName, "trkn", 4) == 0) {
                unsigned __int16 trkn = 0, tot = 0;
                char t[200];
                if (MP4GetMetadataTrack(file, &trkn, &tot))
                {
                    if (tot > 0)
                        wsprintf(t, "%d/%d", trkn, tot);
                    else
                        wsprintf(t, "%d", trkn);
                    tag_add_field(tags, "tracknumber", t);
                }
            } else if (memcmp(pName, "disk", 4) == 0) {
                unsigned __int16 disk = 0, tot = 0;
                char t[200];
                if (MP4GetMetadataDisk(file, &disk, &tot))
                {
                    if (tot > 0)
                        wsprintf(t, "%d/%d", disk, tot);
                    else
                        wsprintf(t, "%d", disk);
                    tag_add_field(tags, "disc", t);
                }
            } else if (memcmp(pName, "cpil", 4) == 0) {
                unsigned __int8 cpil = 0;
                char t[200];
                if (MP4GetMetadataCompilation(file, &cpil))
                {
                    wsprintf(t, "%d", cpil);
                    tag_add_field(tags, "compilation", t);
                }
            } else if (memcmp(pName, "tmpo", 4) == 0) {
                unsigned __int16 tempo = 0;
                char t[200];
                if (MP4GetMetadataTempo(file, &tempo))
                {
                    wsprintf(t, "%d BPM", tempo);
                    tag_add_field(tags, "tempo", t);
                }
            } else if (memcmp(pName, "NDFL", 4) == 0) {
                /* Removed */
            } else {
                tag_add_field(tags, pName, val);
            }

            free(val);
        }

        i++;
    } while (valueSize > 0);

    return 1;
}

int mp4_set_metadata(MP4FileHandle file, const char *item, const char *val)
{
    if (!item || (item && !*item) || !val || (val && !*val)) return 0;

    if (!stricmp(item, "track") || !stricmp(item, "tracknumber"))
    {
        unsigned __int16 trkn, tot;
        int t1 = 0, t2 = 0;
        sscanf(val, "%d/%d", &t1, &t2);
        trkn = t1, tot = t2;
        if (!trkn) return 1;
        if (MP4SetMetadataTrack(file, trkn, tot)) return 1;
    }
    else if (!stricmp(item, "disc") || !stricmp(item, "disknumber"))
    {
        unsigned __int16 disk, tot;
        int t1 = 0, t2 = 0;
        sscanf(val, "%d/%d", &t1, &t2);
        disk = t1, tot = t2;
        if (!disk) return 1;
        if (MP4SetMetadataDisk(file, disk, tot)) return 1;
    }
    else if (!stricmp(item, "compilation"))
    {
        unsigned __int8 cpil = atoi(val);
        if (!cpil) return 1;
        if (MP4SetMetadataCompilation(file, cpil)) return 1;
    }
    else if (!stricmp(item, "tempo"))
    {
        unsigned __int16 tempo = atoi(val);
        if (!tempo) return 1;
        if (MP4SetMetadataTempo(file, tempo)) return 1;
    }
    else if (!stricmp(item, "artist"))
    {
        if (MP4SetMetadataArtist(file, val)) return 1;
    }
    else if (!stricmp(item, "writer"))
    {
        if (MP4SetMetadataWriter(file, val)) return 1;
    }
    else if (!stricmp(item, "title"))
    {
        if (MP4SetMetadataName(file, val)) return 1;
    }
    else if (!stricmp(item, "album"))
    {
        if (MP4SetMetadataAlbum(file, val)) return 1;
    }
    else if (!stricmp(item, "date") || !stricmp(item, "year"))
    {
        if (MP4SetMetadataYear(file, val)) return 1;
    }
    else if (!stricmp(item, "comment"))
    {
        if (MP4SetMetadataComment(file, val)) return 1;
    }
    else if (!stricmp(item, "genre"))
    {
        if (MP4SetMetadataGenre(file, val)) return 1;
    }
    else if (!stricmp(item, "tool"))
    {
        if (MP4SetMetadataTool(file, val)) return 1;
    }
    else
    {
        if (MP4SetMetadataFreeForm(file, (char *)item, (u_int8_t *)val, (u_int32_t)strlen(val) + 1)) return 1;
    }

    return 0;
}

void WriteMP4Tag(MP4FileHandle file, const medialib_tags *tags)
{
    unsigned int i;

    for (i = 0; i < tags->count; i++)
    {
        const char *item = tags->tags[i].item;
        const char *value = tags->tags[i].value;

        if (value && *value)
        {
            mp4_set_metadata(file, item, value);
        }
    }
}

QCDModInitTag	ModInitTag;

medialib_tags tags;

BOOL uSetDlgItemText(void *tagHandle, int fieldId, const char *str);
UINT uGetDlgItemText(void *tagHandle, int fieldId, char *str, int max);

//------------------------------------------------------------------------------

PLUGIN_API QCDModInitTag* TAGEDITORDLL_ENTRY_POINT()
{	
	ModInitTag.size			= sizeof(QCDModInitTag);
	ModInitTag.version		= PLUGIN_API_VERSION;
	ModInitTag.ShutDown		= ShutDown_Tag;

	ModInitTag.Read			= Read_Tag;
	ModInitTag.Write		= Write_Tag;	// Leave null for operations that plugin does not support
	ModInitTag.Strip		= Strip_Tag;	// ie: if plugin only reads tags, leave Write and Strip null

	ModInitTag.description	= "MP4 Tags";
	ModInitTag.defaultexts	= "MP4:M4A";

	return &ModInitTag;
}

//-----------------------------------------------------------------------------

void ShutDown_Tag(int flags)
{
	// TODO:
	// prepare plugin to be unloaded. All allocations should be freed.
	// flags param is unused
	tag_delete(&tags);
}

//-----------------------------------------------------------------------------

bool Read_Tag(LPCSTR filename, void* tagHandle)
{
	// TODO:
	// read metadata from tag and set each field to tagHandle
	// only TAGFIELD_* are supported (see QCDModTagEditor.h)

	// example of how to set value to tagHandle
	// use SetFieldA for ASCII or MultiBytes strings.
	// use SetFieldW for UNICODE strings
	//
	//	ModInitTag.SetFieldW(tagHandle, TAGFIELD_COMPOSER, szwValue);

	// return true for successfull read, false for failure

	MP4FileHandle file = MP4_INVALID_FILE_HANDLE;
	char *pVal, dummy1[1024];
	short dummy, dummy2;
	u_int32_t valueSize = 0;

#ifdef DEBUG_OUTPUT
	in_mp4_DebugOutput("mp4_tag_read");
#endif

	file = MP4Read(filename, 0);

	if (file == MP4_INVALID_FILE_HANDLE)
		return false;

	/* get Metadata */

	pVal = NULL;
	MP4GetMetadataName(file, &pVal);
	uSetDlgItemText(tagHandle, TAGFIELD_TITLE, pVal);

	pVal = NULL;
	MP4GetMetadataArtist(file, &pVal);
	uSetDlgItemText(tagHandle, TAGFIELD_ARTIST, pVal);

	pVal = NULL;
	MP4GetMetadataWriter(file, &pVal);
	uSetDlgItemText(tagHandle, TAGFIELD_COMPOSER, pVal);

	pVal = NULL;
	MP4GetMetadataComment(file, &pVal);
	uSetDlgItemText(tagHandle, TAGFIELD_COMMENT, pVal);

	pVal = NULL;
	MP4GetMetadataAlbum(file, &pVal);
	uSetDlgItemText(tagHandle, TAGFIELD_ALBUM, pVal);

	pVal = NULL;
	MP4GetMetadataGenre(file, &pVal);
	uSetDlgItemText(tagHandle, TAGFIELD_GENRE, pVal);

	//dummy = 0;
	//MP4GetMetadataTempo(file, &dummy);
	//if (dummy)
	//{
	//	wsprintf(dummy1, "%d", dummy);
	//	SetDlgItemText(hwndDlg,IDC_METATEMPO, dummy1);
	//}

	dummy = 0; dummy2 = 0;
	MP4GetMetadataTrack(file, (unsigned __int16*)&dummy, (unsigned __int16*)&dummy2);
	if (dummy)
	{
		wsprintf(dummy1, "%d", dummy);
		ModInitTag.SetFieldA(tagHandle, TAGFIELD_TRACK, dummy1);
	}
	//if (dumm2)
	//{
	//	wsprintf(dummy1, "%d", dummy2);
	//	SetDlgItemText(hwndDlg,IDC_METATRACK2, dummy1);
	//}

	//dummy = 0; dummy2 = 0;
	//MP4GetMetadataDisk(file, &dummy, &dummy2);
	//if (dummy)
	//{
	//	wsprintf(dummy1, "%d", dummy);
	//	SetDlgItemText(hwndDlg,IDC_METADISK1, dummy1);
	//}
	//if (dummy)
	//{
	//	wsprintf(dummy1, "%d", dummy2);
	//	SetDlgItemText(hwndDlg,IDC_METADISK2, dummy1);
	//}

	pVal = NULL;
	if (MP4GetMetadataYear(file, &pVal))
		uSetDlgItemText(tagHandle, TAGFIELD_YEAR, pVal);

	//dummy3 = 0;
	//MP4GetMetadataCompilation(file, &dummy3);
	//if (dummy3)
	//	SendMessage(GetDlgItem(hwndDlg, IDC_METACOMPILATION), BM_SETCHECK, BST_CHECKED, 0);

	pVal = NULL;
	MP4GetMetadataTool(file, &pVal);
	uSetDlgItemText(tagHandle, TAGFIELD_ENCODER, pVal);

	pVal = NULL;
	MP4GetMetadataFreeForm(file, "CONDUCTOR", (unsigned __int8**)&pVal, &valueSize);
	uSetDlgItemText(tagHandle, TAGFIELD_CONDUCTOR, pVal);

	pVal = NULL;
	MP4GetMetadataFreeForm(file, "ORCHESTRA", (unsigned __int8**)&pVal, &valueSize);
	uSetDlgItemText(tagHandle, TAGFIELD_ORCHESTRA, pVal);

	pVal = NULL;
	MP4GetMetadataFreeForm(file, "YEARCOMPOSED", (unsigned __int8**)&pVal, &valueSize);
	uSetDlgItemText(tagHandle, TAGFIELD_YEARCOMPOSED, pVal);

	pVal = NULL;
	MP4GetMetadataFreeForm(file, "ORIGARTIST", (unsigned __int8**)&pVal, &valueSize);
	uSetDlgItemText(tagHandle, TAGFIELD_ORIGARTIST, pVal);

	pVal = NULL;
	MP4GetMetadataFreeForm(file, "LABEL", (unsigned __int8**)&pVal, &valueSize);
	uSetDlgItemText(tagHandle, TAGFIELD_LABEL, pVal);

	pVal = NULL;
	MP4GetMetadataFreeForm(file, "COPYRIGHT", (unsigned __int8**)&pVal, &valueSize);
	uSetDlgItemText(tagHandle, TAGFIELD_COPYRIGHT, pVal);

	pVal = NULL;
	MP4GetMetadataFreeForm(file, "CDDBTAGID", (unsigned __int8**)&pVal, &valueSize);
	uSetDlgItemText(tagHandle, TAGFIELD_CDDBTAGID, pVal);

	/* ! Metadata */

	MP4Close(file);

	return true;
}

//-----------------------------------------------------------------------------

bool Write_Tag(LPCSTR filename, void* tagHandle)
{
	// TODO:
	// read metadata from tagHandle and set each field to supported tag
	// only TAGFIELD_* are supported (see QCDModTagEditor.h)

	// example of how to get value from tagHandle
	// use SetFieldA for ASCII or MultiBytes strings.
	// use SetFieldW for UNICODE strings
	//
	// szwValue = ModInitTag.GetFieldW(tagHandle, TAGFIELD_ORCHESTRA);

	// write tag to file

	MP4FileHandle file = MP4_INVALID_FILE_HANDLE;
    char dummy1[1024];
    char temp[1024];
    short dummy, dummy2;

#ifdef DEBUG_OUTPUT
    in_mp4_DebugOutput("mp4_tag_write");
#endif

	/* save Metadata changes */

	tag_delete(&tags);
	file = MP4Read(filename, 0);
	if (file != MP4_INVALID_FILE_HANDLE)
	{
		ReadMP4Tag(file, &tags);
		MP4Close(file);

		file = MP4Modify(filename, 0, 0);
		if (file != MP4_INVALID_FILE_HANDLE)
		{
			MP4MetadataDelete(file);
			MP4Close(file);
		}
	}

	file = MP4Modify(filename, 0, 0);
	if (file == MP4_INVALID_FILE_HANDLE)
	{
		tag_delete(&tags);
		//EndDialog(hwndDlg, wParam);
		return false;
	}

	uGetDlgItemText(tagHandle, TAGFIELD_TITLE, dummy1, 1024);
	tag_set_field(&tags, "title", dummy1);

	uGetDlgItemText(tagHandle, TAGFIELD_COMPOSER, dummy1, 1024);
	tag_set_field(&tags, "writer", dummy1);

	uGetDlgItemText(tagHandle, TAGFIELD_ARTIST, dummy1, 1024);
	tag_set_field(&tags, "artist", dummy1);

	uGetDlgItemText(tagHandle, TAGFIELD_ALBUM, dummy1, 1024);
	tag_set_field(&tags, "album", dummy1);

	uGetDlgItemText(tagHandle, TAGFIELD_COMMENT, dummy1, 1024);
	tag_set_field(&tags, "comment", dummy1);

	uGetDlgItemText(tagHandle, TAGFIELD_GENRE, dummy1, 1024);
	tag_set_field(&tags, "genre", dummy1);

	uGetDlgItemText(tagHandle, TAGFIELD_YEAR, dummy1, 1024);
	tag_set_field(&tags, "year", dummy1);

	dummy = 0;
	MP4GetMetadataTrack(file, (unsigned __int16*)&dummy, (unsigned __int16*)&dummy2);
	memcpy(dummy1, ModInitTag.GetFieldA(tagHandle, TAGFIELD_TRACK), sizeof(dummy1));
	dummy = atoi(dummy1);
	wsprintf(temp, "%d/%d", dummy, dummy2);
	tag_set_field(&tags, "track", temp);

	//GetDlgItemText(hwndDlg, IDC_METADISK1, dummy1, 1024);
	//dummy = atoi(dummy1);
	//GetDlgItemText(hwndDlg, IDC_METADISK2, dummy1, 1024);
	//dummy2 = atoi(dummy1);
	//wsprintf(temp, "%d/%d", dummy, dummy2);
	//tag_set_field(&tags, "disc", temp);

	//GetDlgItemText(hwndDlg, IDC_METATEMPO, dummy1, 1024);
	//tag_set_field(&tags, "tempo", dummy1);

	//dummy3 = SendMessage(GetDlgItem(hwndDlg, IDC_METACOMPILATION), BM_GETCHECK, 0, 0);
	//tag_set_field(&tags, "compilation", (dummy3 ? "1" : "0"));

	uGetDlgItemText(tagHandle, TAGFIELD_ENCODER, dummy1, 1024);
	tag_set_field(&tags, "tool", dummy1);

	uGetDlgItemText(tagHandle, TAGFIELD_CONDUCTOR, dummy1, 1024);
	tag_set_field(&tags, "CONDUCTOR", dummy1);

	uGetDlgItemText(tagHandle, TAGFIELD_ORCHESTRA, dummy1, 1024);
	tag_set_field(&tags, "ORCHESTRA", dummy1);

	uGetDlgItemText(tagHandle, TAGFIELD_YEARCOMPOSED, dummy1, 1024);
	tag_set_field(&tags, "YEARCOMPOSED", dummy1);

	uGetDlgItemText(tagHandle, TAGFIELD_ORIGARTIST, dummy1, 1024);
	tag_set_field(&tags, "ORIGARTIST", dummy1);

	uGetDlgItemText(tagHandle, TAGFIELD_LABEL, dummy1, 1024);
	tag_set_field(&tags, "LABEL", dummy1);

	uGetDlgItemText(tagHandle, TAGFIELD_COPYRIGHT, dummy1, 1024);
	tag_set_field(&tags, "COPYRIGHT", dummy1);

	uGetDlgItemText(tagHandle, TAGFIELD_CDDBTAGID, dummy1, 1024);
	tag_set_field(&tags, "CDDBTAGID", dummy1);

	WriteMP4Tag(file, &tags);

	MP4Close(file);

	MP4Optimize(filename, NULL, 0);
	/* ! */

	return true;
}

//-----------------------------------------------------------------------------

bool Strip_Tag(LPCSTR filename)
{
	// TODO:
	// remove tag from file.
	// do whatever is need to remove the supported tag from filename

	// return true for successfull strip, false for failure

	MP4FileHandle file;

	file = MP4Modify(filename, 0, 0);
	if (file == MP4_INVALID_FILE_HANDLE)
		return false;
	
	MP4MetadataDelete(file);

	MP4Close(file);

	return true;
}

//-----------------------------------------------------------------------------

/* Convert UNICODE to UTF-8
   Return number of bytes written */
int unicodeToUtf8 ( const WCHAR* lpWideCharStr, char* lpMultiByteStr, int cwcChars )
{
    const unsigned short*   pwc = (unsigned short *)lpWideCharStr;
    unsigned char*          pmb = (unsigned char  *)lpMultiByteStr;
    const unsigned short*   pwce;
    size_t  cBytes = 0;

    if ( cwcChars >= 0 ) {
        pwce = pwc + cwcChars;
    } else {
        pwce = (unsigned short *)((size_t)-1);
    }

    while ( pwc < pwce ) {
        unsigned short  wc = *pwc++;

        if ( wc < 0x00000080 ) {
            *pmb++ = (char)wc;
            cBytes++;
        } else
        if ( wc < 0x00000800 ) {
            *pmb++ = (char)(0xC0 | ((wc >>  6) & 0x1F));
            cBytes++;
            *pmb++ = (char)(0x80 |  (wc        & 0x3F));
            cBytes++;
        } else
        if ( wc < 0x00010000 ) {
            *pmb++ = (char)(0xE0 | ((wc >> 12) & 0x0F));
            cBytes++;
            *pmb++ = (char)(0x80 | ((wc >>  6) & 0x3F));
            cBytes++;
            *pmb++ = (char)(0x80 |  (wc        & 0x3F));
            cBytes++;
        }
        if ( wc == L'\0' )
            return cBytes;
    }

    return cBytes;
}

/* Convert UTF-8 coded string to UNICODE
   Return number of characters converted */
int utf8ToUnicode ( const char* lpMultiByteStr, WCHAR* lpWideCharStr, int cmbChars )
{
    const unsigned char*    pmb = (unsigned char  *)lpMultiByteStr;
    unsigned short*         pwc = (unsigned short *)lpWideCharStr;
    const unsigned char*    pmbe;
    size_t  cwChars = 0;

    if ( cmbChars >= 0 ) {
        pmbe = pmb + cmbChars;
    } else {
        pmbe = (unsigned char *)((size_t)-1);
    }

    while ( pmb < pmbe ) {
        char            mb = *pmb++;
        unsigned int    cc = 0;
        unsigned int    wc;

        while ( (cc < 7) && (mb & (1 << (7 - cc)))) {
            cc++;
        }

        if ( cc == 1 || cc > 6 )                    // illegal character combination for UTF-8
            continue;

        if ( cc == 0 ) {
            wc = mb;
        } else {
            wc = (mb & ((1 << (7 - cc)) - 1)) << ((cc - 1) * 6);
            while ( --cc > 0 ) {
                if ( pmb == pmbe )                  // reached end of the buffer
                    return cwChars;
                mb = *pmb++;
                if ( ((mb >> 6) & 0x03) != 2 )      // not part of multibyte character
                    return cwChars;
                wc |= (mb & 0x3F) << ((cc - 1) * 6);
            }
        }

        if ( wc & 0xFFFF0000 )
            wc = L'?';
        *pwc++ = wc;
        cwChars++;
        if ( wc == L'\0' )
            return cwChars;
    }

    return cwChars;
}

/* convert Windows ANSI to UTF-8 */
int ConvertANSIToUTF8 ( const char* ansi, char* utf8 )
{
    WCHAR*  wszValue;          // Unicode value
    size_t  ansi_len;
    size_t  len;

    *utf8 = '\0';
    if ( ansi == NULL )
        return 0;

    ansi_len = strlen ( ansi );

    if ( (wszValue = (WCHAR *)malloc ( (ansi_len + 1) * 2 )) == NULL )
        return 0;

    /* Convert ANSI value to Unicode */
    if ( (len = MultiByteToWideChar ( CP_ACP, 0, ansi, ansi_len + 1, wszValue, (ansi_len + 1) * 2 )) == 0 ) {
        free ( wszValue );
        return 0;
    }

    /* Convert Unicode value to UTF-8 */
    if ( (len = unicodeToUtf8 ( wszValue, utf8, -1 )) == 0 ) {
        free ( wszValue );
        return 0;
    }

    free ( wszValue );

    return len-1;
}

/* convert UTF-8 to Windows ANSI */
int ConvertUTF8ToANSI ( const char* utf8, char* ansi )
{
    WCHAR*  wszValue;          // Unicode value
    size_t  utf8_len;
    size_t  len;

    *ansi = '\0';
    if ( utf8 == NULL )
        return 0;

    utf8_len = strlen ( utf8 );

    if ( (wszValue = (WCHAR *)malloc ( (utf8_len + 1) * 2 )) == NULL )
        return 0;

    /* Convert UTF-8 value to Unicode */
    if ( (len = utf8ToUnicode ( utf8, wszValue, utf8_len + 1 )) == 0 ) {
        free ( wszValue );
        return 0;
    }

    /* Convert Unicode value to ANSI */
    if ( (len = WideCharToMultiByte ( CP_ACP, 0, wszValue, -1, ansi, (utf8_len + 1) * 2, NULL, NULL )) == 0 ) {
        free ( wszValue );
        return 0;
    }

    free ( wszValue );

    return len-1;
}

BOOL uSetDlgItemText(void *tagHandle, int fieldId, const char *str)
{
    char *temp;
    size_t len;
    int r;

    if (!str) return FALSE;
	if (!*str) return FALSE;
    len = strlen(str);
    temp = (char *)malloc(len+1);
    if (!temp) return FALSE;
	memset(temp, '\0', len+1);
    r = ConvertUTF8ToANSI(str, temp);
    if (r > 0)
        ModInitTag.SetFieldA(tagHandle, fieldId, temp);
    free(temp);

    return r>0 ? TRUE : FALSE;
}

UINT uGetDlgItemText(void *tagHandle, int fieldId, char *str, int max)
{
    char *temp, *utf8;;
    int len;

	const char *p;

    if (!str || !max) return 0;
    len = strlen( ModInitTag.GetFieldA(tagHandle, fieldId) );
    temp = (char *)malloc(len+1);
    if (!temp) return 0;
    utf8 = (char *)malloc((len+1)*4);
    if (!utf8)
    {
        free(temp);
        return 0;
    }

	memset(temp, '\0', len+1);
	memset(utf8, '\0', (len+1)*4);
	memset(str, '\0', max);
	p = ModInitTag.GetFieldA(tagHandle, fieldId);
	memcpy(temp, p, len+1);
    if (len > 0)
    {
        len = ConvertANSIToUTF8(temp, utf8);
        if (len > max-1)
        {
            len = max-1;
            utf8[max] = '\0';
        }
        memcpy(str, utf8, len+1);
    }

    free(temp);
    free(utf8);

    return len;
}
