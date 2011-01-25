/*
** FAAD2 - Freeware Advanced Audio (AAC) Decoder including SBR decoding
** Copyright (C) 2003 M. Bakker, Ahead Software AG, http://www.nero.com
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
** Any non-GPL usage of this software or parts of this software is strictly
** forbidden.
**
** Commercial non-GPL licensing of this software is possible.
** For more info contact Ahead Software through Mpeg4AAClicense@nero.com.
**
** $Id: id3v2tag.c,v 1.4 2003/07/29 08:20:11 menno Exp $
**/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <resource.h>

#include <id3.h>

#include <id3v2tag.h>

HWND m_hwndList;

LPSTR ID3Frames[] =
{
    "No known frame",
    "Audio encryption",
    "Attached picture",
    "Comments",
    "Commercial frame",
    "Encryption method registration",
    "Equalization",
    "Event timing codes",
    "General encapsulated object",
    "Group identification registration",
    "Involved people list",
    "Linked information",
    "Music CD identifier",
    "MPEG location lookup table",
    "Ownership frame",
    "Private frame",
    "Play counter",
    "Popularimeter",
    "Position synchronisation frame",
    "Recommended buffer size",
    "Relative volume adjustment",
    "Reverb",
    "Synchronized lyric",
    "Synchronized tempo codes",
    "Album title",
    "BPM (beats per minute)",
    "Composer",
    "Genre", //"Content type",
    "Copyright message",
    "Date",
    "Playlist delay",
    "Encoded by",
    "Lyricist",
    "File type",
    "Time",
    "Content group description",
    "Title",
    "Subtitle",
    "Initial key",
    "Language(s)",
    "Length",
    "Media type",
    "Original album title",
    "Original filename",
    "Original lyricist(s)",
    "Original artist(s)",
    "Original release year",
    "File owner",
    "Lead performer(s)",
    "Band/orchestra/accompaniment",
    "Conductor/performer refinement",
    "Interpreted, remixed, or otherwise modified by",
    "Part of a set",
    "Publisher",
    "Track number",
    "Recording dates",
    "Internet radio station name",
    "Internet radio station owner",
    "Size",
    "ISRC (international standard recording code)",
    "Software/Hardware and settings used for encoding",
    "User defined text information",
    "Year",
    "Unique file identifier",
    "Terms of use",
    "Unsynchronized lyric",
    "Commercial information",
    "Copyright/Legal information",
    "Official audio file webpage",
    "Official artist webpage",
    "Official audio source webpage",
    "Official internet radio station homepage",
    "Payment",
    "Official publisher webpage",
    "User defined URL link",
    "Encrypted meta frame (id3v2.2.x)",
    "Compressed meta frame (id3v2.2.1)"
};

ID3GENRES ID3Genres[]=
{
    123,    "Acapella",
    34,     "Acid",
    74,     "Acid Jazz",
    73,     "Acid Punk",
    99,     "Acoustic",
    20,     "Alternative",
    40,     "AlternRock",
    26,     "Ambient",
    90,     "Avantgarde",
    116,    "Ballad",
    41,     "Bass",
    85,     "Bebob",
    96,     "Big Band",
    89,     "Bluegrass",
    0,      "Blues",
    107,    "Booty Bass",
    65,     "Cabaret",
    88,     "Celtic",
    104,    "Chamber Music",
    102,    "Chanson",
    97,     "Chorus",
    61,     "Christian Rap",
    1,      "Classic Rock",
    32,     "Classical",
    112,    "Club",
    57,     "Comedy",
    2,      "Country",
    58,     "Cult",
    3,      "Dance",
    125,    "Dance Hall",
    50,     "Darkwave",
    254,    "Data",
    22,     "Death Metal",
    4,      "Disco",
    55,     "Dream",
    122,    "Drum Solo",
    120,    "Duet",
    98,     "Easy Listening",
    52,     "Electronic",
    48,     "Ethnic",
    124,    "Euro-House",
    25,     "Euro-Techno",
    54,     "Eurodance",
    84,     "Fast Fusion",
    80,     "Folk",
    81,     "Folk-Rock",
    115,    "Folklore",
    119,    "Freestyle",
    5,      "Funk",
    30,     "Fusion",
    36,     "Game",
    59,     "Gangsta",
    38,     "Gospel",
    49,     "Gothic",
    91,     "Gothic Rock",
    6,      "Grunge",
    79,     "Hard Rock",
    7,      "Hip-Hop",
    35,     "House",
    100,    "Humour",
    19,     "Industrial",
    33,     "Instrumental",
    46,     "Instrumental Pop",
    47,     "Instrumental Rock",
    8,      "Jazz",
    29,     "Jazz+Funk",
    63,     "Jungle",
    86,     "Latin",
    71,     "Lo-Fi",
    45,     "Meditative",
    9,      "Metal",
    77,     "Musical",
    82,     "National Folk",
    64,     "Native American",
    10,     "New Age",
    66,     "New Wave",
    39,     "Noise",
    255,    "Not Set",
    11,     "Oldies",
    103,    "Opera",
    12,     "Other",
    75,     "Polka",
    13,     "Pop",
    62,     "Pop/Funk",
    53,     "Pop-Folk",
    109,    "Porn Groove",
    117,    "Power Ballad",
    23,     "Pranks",
    108,    "Primus",
    92,     "Progressive Rock",
    67,     "Psychadelic",
    93,     "Psychedelic Rock",
    43,     "Punk",
    121,    "Punk Rock",
    14,     "R&B",
    15,     "Rap",
    68,     "Rave",
    16,     "Reggae",
    76,     "Retro",
    87,     "Revival",
    118,    "Rhythmic Soul",
    17,     "Rock",
    78,     "Rock & Roll",
    114,    "Samba",
    110,    "Satire",
    69,     "Showtunes",
    21,     "Ska",
    111,    "Slow Jam",
    95,     "Slow Rock",
    105,    "Sonata",
    42,     "Soul",
    37,     "Sound Clip",
    24,     "Soundtrack",
    56,     "Southern Rock",
    44,     "Space",
    101,    "Speech",
    83,     "Swing",
    94,     "Symphonic Rock",
    106,    "Symphony",
    113,    "Tango",
    18,     "Techno",
    51,     "Techno-Industrial",
    60,     "Top 40",
    70,     "Trailer",
    31,     "Trance",
    72,     "Tribal",
    27,     "Trip-Hop",
    28,     "Vocal"
};

const int NUMFRAMES = sizeof(ID3Frames)/sizeof(ID3Frames[0]);
const int NUMGENRES = sizeof(ID3Genres)/sizeof(ID3Genres[0]);


LPSTR DupString(LPSTR lpsz)
{
    int cb = lstrlen(lpsz) + 1;
    LPSTR lpszNew = LocalAlloc(LMEM_FIXED, cb);
    if (lpszNew != NULL)
        CopyMemory(lpszNew, lpsz, cb);
    return lpszNew;
}

LPSTR GetFrameDesc(ID3_FrameID id)
{
    return DupString(ID3Frames[id]);
}

LPSTR GetGenre(LPSTR lpsz)
{
    int id = atoi(lpsz + 1);
    int i;

    if ((*(lpsz + 1) > '0') && (*(lpsz + 1) < '9'))
    {
        for (i = 0; i < NUMGENRES; i++)
        {
            if (id == ID3Genres[i].id)
                return DupString(ID3Genres[i].name);
        }
    }
    return DupString(lpsz);
}

void FillID3List(HWND hwndDlg, HWND hwndList, char *filename)
{
    ID3Tag *tag;
    ID3Frame *frame;
    ID3Field *field;
    ID3_FrameID eFrameID;
    char info[1024];
    int numFrames;
    int i;
    int iItem = 0;


    if ((tag = ID3Tag_New()) != NULL)
    {
        ID3Tag_Link(tag, filename);

        numFrames = ID3Tag_NumFrames(tag);

        for (i = 0; i < numFrames; i++)
        {
            iItem++;

            frame = ID3Tag_GetFrameNum(tag, i);
            eFrameID = ID3Frame_GetID(frame);

            switch (eFrameID)
            {
            case ID3FID_ALBUM:            case ID3FID_BPM:
            case ID3FID_COMPOSER:         case ID3FID_CONTENTTYPE:
            case ID3FID_COPYRIGHT:        case ID3FID_DATE:
            case ID3FID_PLAYLISTDELAY:    case ID3FID_ENCODEDBY:
            case ID3FID_LYRICIST:         case ID3FID_FILETYPE:
            case ID3FID_TIME:             case ID3FID_CONTENTGROUP:
            case ID3FID_TITLE:            case ID3FID_SUBTITLE:
            case ID3FID_INITIALKEY:       case ID3FID_LANGUAGE:
            case ID3FID_SONGLEN:          case ID3FID_MEDIATYPE:
            case ID3FID_ORIGALBUM:        case ID3FID_ORIGFILENAME:
            case ID3FID_ORIGLYRICIST:     case ID3FID_ORIGARTIST:
            case ID3FID_ORIGYEAR:         case ID3FID_FILEOWNER:
            case ID3FID_LEADARTIST:       case ID3FID_BAND:
            case ID3FID_CONDUCTOR:        case ID3FID_MIXARTIST:
            case ID3FID_PARTINSET:        case ID3FID_PUBLISHER:
            case ID3FID_TRACKNUM:         case ID3FID_RECORDINGDATES:
            case ID3FID_NETRADIOSTATION:  case ID3FID_NETRADIOOWNER:
            case ID3FID_SIZE:             case ID3FID_ISRC:
            case ID3FID_ENCODERSETTINGS:  case ID3FID_YEAR:
            {
                LV_ITEM lvi;
                ID3ITEM *pItem = LocalAlloc(LPTR, sizeof(ID3ITEM));

                /* Initialize LV_ITEM members that are common to all items. */
                lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
                lvi.state = 0;
                lvi.stateMask = 0;
                lvi.pszText = LPSTR_TEXTCALLBACK;   /* app. maintains text */
                lvi.iImage = 0;
                lvi.iItem = iItem;
                lvi.iSubItem = 0;

                pItem->frameId = eFrameID;
                pItem->aCols[0] = GetFrameDesc(eFrameID);

                field = ID3Frame_GetField(frame, ID3FN_TEXT);
                ID3Field_GetASCII(field, info, 1024, 1);
                if (eFrameID == ID3FID_CONTENTTYPE)
                    pItem->aCols[1] = GetGenre(info);
                else
                    pItem->aCols[1] = DupString(info);

                lvi.lParam = (LPARAM)pItem;    /* item data */

                /* Add the item. */
                ListView_InsertItem(hwndList, &lvi);

                break;
            }
            case ID3FID_USERTEXT:
            case ID3FID_COMMENT: /* Can also contain an extra language field (but not used now) */
            case ID3FID_UNSYNCEDLYRICS: /* Can also contain an extra language field (but not used now) */
            {
                LV_ITEM lvi;
                ID3ITEM *pItem = LocalAlloc(LPTR, sizeof(ID3ITEM));

                /* Initialize LV_ITEM members that are common to all items. */
                lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
                lvi.state = 0;
                lvi.stateMask = 0;
                lvi.pszText = LPSTR_TEXTCALLBACK;   /* app. maintains text */
                lvi.iImage = 0;
                lvi.iItem = iItem;
                lvi.iSubItem = 0;

                pItem->frameId = eFrameID;

                field = ID3Frame_GetField(frame, ID3FN_DESCRIPTION);
                ID3Field_GetASCII(field, info, 1024, 1);
                pItem->aCols[0] = DupString(info);

                field = ID3Frame_GetField(frame, ID3FN_TEXT);
                ID3Field_GetASCII(field, info, 1024, 1);
                pItem->aCols[1] = DupString(info);

                lvi.lParam = (LPARAM)pItem;    /* item data */

                /* Add the item. */
                ListView_InsertItem(hwndList, &lvi);

                break;
            }
            case ID3FID_WWWAUDIOFILE:       case ID3FID_WWWARTIST:
            case ID3FID_WWWAUDIOSOURCE:     case ID3FID_WWWCOMMERCIALINFO:
            case ID3FID_WWWCOPYRIGHT:       case ID3FID_WWWPUBLISHER:
            case ID3FID_WWWPAYMENT:         case ID3FID_WWWRADIOPAGE:
            {
                LV_ITEM lvi;
                ID3ITEM *pItem = LocalAlloc(LPTR, sizeof(ID3ITEM));

                /* Initialize LV_ITEM members that are common to all items. */
                lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
                lvi.state = 0;
                lvi.stateMask = 0;
                lvi.pszText = LPSTR_TEXTCALLBACK;   /* app. maintains text */
                lvi.iImage = 0;
                lvi.iItem = iItem;
                lvi.iSubItem = 0;

                pItem->frameId = eFrameID;

                pItem->aCols[0] = GetFrameDesc(eFrameID);

                field = ID3Frame_GetField(frame, ID3FN_URL);
                ID3Field_GetASCII(field, info, 1024, 1);
                pItem->aCols[1] = DupString(info);

                lvi.lParam = (LPARAM)pItem;    /* item data */

                /* Add the item. */
                ListView_InsertItem(hwndList, &lvi);

                break;
            }
            case ID3FID_WWWUSER:
            {
                LV_ITEM lvi;
                ID3ITEM *pItem = LocalAlloc(LPTR, sizeof(ID3ITEM));

                /* Initialize LV_ITEM members that are common to all items. */
                lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
                lvi.state = 0;
                lvi.stateMask = 0;
                lvi.pszText = LPSTR_TEXTCALLBACK;   /* app. maintains text */
                lvi.iImage = 0;
                lvi.iItem = iItem;
                lvi.iSubItem = 0;

                pItem->frameId = eFrameID;

                field = ID3Frame_GetField(frame, ID3FN_DESCRIPTION);
                ID3Field_GetASCII(field, info, 1024, 1);
                pItem->aCols[0] = DupString(info);

                field = ID3Frame_GetField(frame, ID3FN_URL);
                ID3Field_GetASCII(field, info, 1024, 1);
                pItem->aCols[1] = DupString(info);

                lvi.lParam = (LPARAM)pItem;    /* item data */

                /* Add the item. */
                ListView_InsertItem(hwndList, &lvi);

                break;
            }
            default:
                break;
            }
        }
        ID3Tag_Delete(tag);
    }
}

BOOL CALLBACK AddFrameProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    int i, cursel;

    switch (message) {
    case WM_INITDIALOG:
        EnableWindow(GetDlgItem(hwndDlg, IDC_COL0), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDOK), FALSE);

        /* Note: FRAMEID is the index in the combo box + 1 */
        for (i = 1; i < NUMFRAMES; i++)
        {
            SendMessage(GetDlgItem(hwndDlg, IDC_FRAMETYPE), CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)ID3Frames[i]);
        }
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_FRAMETYPE:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                cursel = SendMessage(GetDlgItem(hwndDlg, IDC_FRAMETYPE), CB_GETCURSEL, 0, 0);

                switch (cursel + 1)
                {
                case ID3FID_ALBUM:             case ID3FID_BPM:
                case ID3FID_COMPOSER:          case ID3FID_COPYRIGHT:
                case ID3FID_DATE:              case ID3FID_PLAYLISTDELAY:
                case ID3FID_ENCODEDBY:         case ID3FID_LYRICIST:
                case ID3FID_FILETYPE:          case ID3FID_TIME:
                case ID3FID_CONTENTGROUP:      case ID3FID_TITLE:
                case ID3FID_SUBTITLE:          case ID3FID_INITIALKEY:
                case ID3FID_LANGUAGE:          case ID3FID_SONGLEN:
                case ID3FID_MEDIATYPE:         case ID3FID_ORIGALBUM:
                case ID3FID_ORIGFILENAME:      case ID3FID_ORIGLYRICIST:
                case ID3FID_ORIGARTIST:        case ID3FID_ORIGYEAR:
                case ID3FID_FILEOWNER:         case ID3FID_LEADARTIST:
                case ID3FID_BAND:              case ID3FID_CONDUCTOR:
                case ID3FID_MIXARTIST:         case ID3FID_PARTINSET:
                case ID3FID_PUBLISHER:         case ID3FID_TRACKNUM:
                case ID3FID_RECORDINGDATES:    case ID3FID_NETRADIOSTATION:
                case ID3FID_NETRADIOOWNER:     case ID3FID_SIZE:
                case ID3FID_ISRC:              case ID3FID_ENCODERSETTINGS:
                case ID3FID_YEAR:              case ID3FID_WWWAUDIOFILE:
                case ID3FID_WWWARTIST:         case ID3FID_WWWAUDIOSOURCE:
                case ID3FID_WWWCOMMERCIALINFO: case ID3FID_WWWCOPYRIGHT:
                case ID3FID_WWWPUBLISHER:      case ID3FID_WWWPAYMENT:
                case ID3FID_WWWRADIOPAGE:      case ID3FID_CONTENTTYPE:
                {
                    SetDlgItemText(hwndDlg, IDC_COL0, ID3Frames[cursel+1]);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_COL0), FALSE);
                    EnableWindow(GetDlgItem(hwndDlg, IDOK), TRUE);
                    break;
                }
                case ID3FID_USERTEXT:          case ID3FID_COMMENT:
                case ID3FID_UNSYNCEDLYRICS:    case ID3FID_WWWUSER:
                {
                    SetDlgItemText(hwndDlg, IDC_COL0, ID3Frames[cursel+1]);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_COL0), TRUE);
                    EnableWindow(GetDlgItem(hwndDlg, IDOK), TRUE);
                    break;
                }
                default:
                    MessageBox(hwndDlg, "Sorry, this frame type cannot be added (yet).", "Sorry", MB_OK);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_COL0), FALSE);
                    EnableWindow(GetDlgItem(hwndDlg, IDOK), FALSE);
                    break;
                }
            }
            return TRUE;
        case IDOK:
            {
                LV_ITEM lvi;
                ID3ITEM *pItem = LocalAlloc(LPTR, sizeof(ID3ITEM));
                char *col0 = LocalAlloc(LPTR, 1024);
                char *col1 = LocalAlloc(LPTR, 1024);

                /* Initialize LV_ITEM members that are common to all items. */
                lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
                lvi.state = 0;
                lvi.stateMask = 0;
                lvi.pszText = LPSTR_TEXTCALLBACK;   /* app. maintains text */
                lvi.iImage = 0;
                lvi.iItem = ListView_GetItemCount(m_hwndList) + 1;
                lvi.iSubItem = 0;

                cursel = SendMessage(GetDlgItem(hwndDlg, IDC_FRAMETYPE), CB_GETCURSEL, 0, 0);
                pItem->frameId = cursel + 1;
                GetDlgItemText(hwndDlg, IDC_COL0, col0, 1024);
                GetDlgItemText(hwndDlg, IDC_COL1, col1, 1024);
                pItem->aCols[0] = col0;
                pItem->aCols[1] = col1;

                lvi.lParam = (LPARAM)pItem;    /* item data */

                /* Add the item. */
                ListView_InsertItem(m_hwndList, &lvi);
                ListView_Update(m_hwndList, lvi.iItem);
            }
        case IDCANCEL:
            EndDialog(hwndDlg, wParam);
            return TRUE;
        }
    }
    return FALSE;
}

BOOL List_AddFrame(HWND hwndApp, HWND hwndList)
{
    int result;

    m_hwndList = hwndList;

    result = DialogBox(hInstance_for_id3editor, MAKEINTRESOURCE(IDD_ADDFRAME),
        hwndApp, AddFrameProc);

    if (LOWORD(result) == IDOK)
        return TRUE;
    return FALSE;
}


void InsertTextFrame(HWND dlg, HWND list, int control, int item, int frame_id)
{
    LV_ITEM lvi;
    ID3ITEM *pItem = LocalAlloc(LPTR, sizeof(ID3ITEM));

    /* Initialize LV_ITEM members that are common to all items. */
    lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
    lvi.state = 0;
    lvi.stateMask = 0;
    lvi.pszText = LPSTR_TEXTCALLBACK;   /* app. maintains text */
    lvi.iImage = 0;
    lvi.iItem = item;
    lvi.iSubItem = 0;

    pItem->frameId = frame_id;
    pItem->aCols[0] = GetFrameDesc(frame_id);

    pItem->aCols[1] = LocalAlloc(LPTR, 1024);
    GetDlgItemText(dlg, control, pItem->aCols[1], 1024);

    lvi.lParam = (LPARAM)pItem;    /* item data */

    /* Add the item. */
    ListView_InsertItem(list, &lvi);
}

void AddFrameFromRAWData(HWND hwndList, int frameId, LPSTR data1, LPSTR data2)
{
    LV_ITEM lvi;
    ID3ITEM *pItem = LocalAlloc(LPTR, sizeof(ID3ITEM));
    int nextItem;

    nextItem = ListView_GetItemCount(hwndList);

    /* Initialize LV_ITEM members that are common to all items. */
    lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
    lvi.state = 0;
    lvi.stateMask = 0;
    lvi.pszText = LPSTR_TEXTCALLBACK;   /* app. maintains text */
    lvi.iImage = 0;
    lvi.iItem = nextItem;
    lvi.iSubItem = 0;

    pItem->frameId = frameId;

    pItem->aCols[0] = LocalAlloc(LPTR, 1024);
    pItem->aCols[1] = LocalAlloc(LPTR, 1024);

    lstrcpy(pItem->aCols[0], data1);
    lstrcpy(pItem->aCols[1], data2);

    lvi.lParam = (LPARAM)pItem;    /* item data */

    /* Add the item. */
    ListView_InsertItem(hwndList, &lvi);
}

HWND m_hwndDlg;
int changed;

BOOL CALLBACK AddStandardProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    int added = 0;

    switch (message) {
    case WM_INITDIALOG:
        changed = 0;
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
        {
            if (GetWindowTextLength(GetDlgItem(hwndDlg, IDC_TRACK)) > 0) {
                InsertTextFrame(hwndDlg, m_hwndList, IDC_TRACK, ListView_GetItemCount(m_hwndList)+1, ID3FID_TRACKNUM);
                added++;
            }

            if (GetWindowTextLength(GetDlgItem(hwndDlg, IDC_TITLE)) > 0) {
                InsertTextFrame(hwndDlg, m_hwndList, IDC_TITLE, ListView_GetItemCount(m_hwndList)+1, ID3FID_TITLE);
                added++;
            }

            if (GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ARTIST)) > 0) {
                InsertTextFrame(hwndDlg, m_hwndList, IDC_ARTIST, ListView_GetItemCount(m_hwndList)+1, ID3FID_LEADARTIST);
                added++;
            }

            if (GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ALBUM)) > 0) {
                InsertTextFrame(hwndDlg, m_hwndList, IDC_ALBUM, ListView_GetItemCount(m_hwndList)+1, ID3FID_ALBUM);
                added++;
            }

            if (GetWindowTextLength(GetDlgItem(hwndDlg, IDC_YEAR)) > 0) {
                InsertTextFrame(hwndDlg, m_hwndList, IDC_YEAR, ListView_GetItemCount(m_hwndList)+1, ID3FID_YEAR);
                added++;
            }

            if (GetWindowTextLength(GetDlgItem(hwndDlg, IDC_GENRE)) > 0) {
                InsertTextFrame(hwndDlg, m_hwndList, IDC_GENRE, ListView_GetItemCount(m_hwndList)+1, ID3FID_CONTENTTYPE);
                added++;
            }

            if (GetWindowTextLength(GetDlgItem(hwndDlg, IDC_COMMENT)) > 0) {
                InsertTextFrame(hwndDlg, m_hwndList, IDC_COMMENT, ListView_GetItemCount(m_hwndList)+1, ID3FID_COMMENT);
                added++;
            }

            if (GetWindowTextLength(GetDlgItem(hwndDlg, IDC_COMPOSER)) > 0) {
                InsertTextFrame(hwndDlg, m_hwndList, IDC_COMPOSER, ListView_GetItemCount(m_hwndList)+1, ID3FID_COMPOSER);
                added++;
            }

            if (GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ORIGARTIST)) > 0) {
                InsertTextFrame(hwndDlg, m_hwndList, IDC_ORIGARTIST, ListView_GetItemCount(m_hwndList)+1, ID3FID_ORIGARTIST);
                added++;
            }

            if (GetWindowTextLength(GetDlgItem(hwndDlg, IDC_COPYRIGHT)) > 0) {
                InsertTextFrame(hwndDlg, m_hwndList, IDC_COPYRIGHT, ListView_GetItemCount(m_hwndList)+1, ID3FID_COPYRIGHT);
                added++;
            }

            if (GetWindowTextLength(GetDlgItem(hwndDlg, IDC_URL)) > 0) {
                InsertTextFrame(hwndDlg, m_hwndList, IDC_URL, ListView_GetItemCount(m_hwndList)+1, ID3FID_WWWARTIST);
                added++;
            }

            if (GetWindowTextLength(GetDlgItem(hwndDlg, IDC_ENCBY)) > 0) {
                InsertTextFrame(hwndDlg, m_hwndList, IDC_ENCBY, ListView_GetItemCount(m_hwndList)+1, ID3FID_ENCODEDBY);
                added++;
            }

            if (added > 0)
                changed = 1;
        }
        case IDCANCEL:
            EndDialog(hwndDlg, changed);
            return TRUE;
        }
    }
    return FALSE;
}


BOOL List_AddStandardFrames(HWND hwndApp, HWND hwndList)
{
    int result;

    m_hwndList = hwndList;
    m_hwndDlg = hwndApp;

    result = DialogBox(hInstance_for_id3editor, MAKEINTRESOURCE(IDD_ADDSTANDARD),
        hwndApp, AddStandardProc);

    return result?TRUE:FALSE;
}


/* List_OnGetDispInfo - processes the LVN_GETDISPINFO  */
/*     notification message. */
/* pnmv - value of lParam (points to an LV_DISPINFO structure) */
void List_OnGetDispInfo(LV_DISPINFO *pnmv)
{
    /* Provide the item or subitem's text, if requested. */
    if (pnmv->item.mask & LVIF_TEXT) {
        ID3ITEM *pItem = (ID3ITEM *) (pnmv->item.lParam);
        lstrcpy(pnmv->item.pszText,
            pItem->aCols[pnmv->item.iSubItem]);
    }
}

ID3ITEM *pItem;
int editItemIndex;

BOOL CALLBACK EditTextFrameProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    LV_ITEM lvi;

    switch (message) {
    case WM_INITDIALOG:
        SetDlgItemText(hwndDlg, IDC_TEXTFRAMENAME, pItem->aCols[0]);
        SetDlgItemText(hwndDlg, IDC_EDITTEXTFRAME, pItem->aCols[1]);

        switch (pItem->frameId)
        {
        case ID3FID_ALBUM:              case ID3FID_BPM:
        case ID3FID_COMPOSER:           case ID3FID_COPYRIGHT:
        case ID3FID_DATE:               case ID3FID_PLAYLISTDELAY:
        case ID3FID_ENCODEDBY:          case ID3FID_LYRICIST:
        case ID3FID_FILETYPE:           case ID3FID_TIME:
        case ID3FID_CONTENTGROUP:       case ID3FID_TITLE:
        case ID3FID_SUBTITLE:           case ID3FID_INITIALKEY:
        case ID3FID_LANGUAGE:           case ID3FID_SONGLEN:
        case ID3FID_MEDIATYPE:          case ID3FID_ORIGALBUM:
        case ID3FID_ORIGFILENAME:       case ID3FID_ORIGLYRICIST:
        case ID3FID_ORIGARTIST:         case ID3FID_ORIGYEAR:
        case ID3FID_FILEOWNER:          case ID3FID_LEADARTIST:
        case ID3FID_BAND:               case ID3FID_CONDUCTOR:
        case ID3FID_MIXARTIST:          case ID3FID_PARTINSET:
        case ID3FID_PUBLISHER:          case ID3FID_TRACKNUM:
        case ID3FID_RECORDINGDATES:     case ID3FID_NETRADIOSTATION:
        case ID3FID_NETRADIOOWNER:      case ID3FID_SIZE:
        case ID3FID_ISRC:               case ID3FID_ENCODERSETTINGS:
        case ID3FID_YEAR:               case ID3FID_WWWAUDIOFILE:
        case ID3FID_WWWARTIST:          case ID3FID_WWWAUDIOSOURCE:
        case ID3FID_WWWCOMMERCIALINFO:  case ID3FID_WWWCOPYRIGHT:
        case ID3FID_WWWPUBLISHER:       case ID3FID_WWWPAYMENT:
        case ID3FID_WWWRADIOPAGE:       case ID3FID_CONTENTTYPE:
        {
            EnableWindow(GetDlgItem(hwndDlg, IDC_TEXTFRAMENAME), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_EDITTEXTFRAME), TRUE);
            break;
        }
        case ID3FID_USERTEXT:           case ID3FID_COMMENT:
        case ID3FID_UNSYNCEDLYRICS:     case ID3FID_WWWUSER:
        {
            EnableWindow(GetDlgItem(hwndDlg, IDC_TEXTFRAMENAME), TRUE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_EDITTEXTFRAME), TRUE);
            break;
        }
        default:
            EnableWindow(GetDlgItem(hwndDlg, IDOK), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_TEXTFRAMENAME), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_EDITTEXTFRAME), FALSE);
            break;
        }
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
        {
            GetDlgItemText(hwndDlg, IDC_TEXTFRAMENAME, pItem->aCols[0], 1024);
            GetDlgItemText(hwndDlg, IDC_EDITTEXTFRAME, pItem->aCols[1], 1024);
            lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
            lvi.state = 0;
            lvi.stateMask = 0;
            lvi.pszText = LPSTR_TEXTCALLBACK;   /* app. maintains text */
            lvi.iImage = 0;
            lvi.iItem = editItemIndex;
            lvi.iSubItem = 0;
            lvi.lParam = (LPARAM)pItem;    /* item data */

            /* Add the item. */
            ListView_SetItem(m_hwndList, &lvi);
            ListView_Update(m_hwndList, editItemIndex);
        } /* Fall through */
        case IDCANCEL:
            EndDialog(hwndDlg, wParam);
            return TRUE;
        }
    }
    return FALSE;
}


/* Double clicking means editing a frame */
BOOL List_EditData(HWND hwndApp, HWND hwndList)
{
    LV_ITEM lvi;
    BOOL result;

    /* First get the selected item */
    int index = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);

    m_hwndList = hwndList;

    if (index != -1)
    {
        lvi.mask = LVIF_PARAM;
        lvi.iItem = index;
        lvi.iSubItem = 0;

        if (ListView_GetItem(hwndList, &lvi) == TRUE)
        {
            pItem = (ID3ITEM*)lvi.lParam;
            editItemIndex = lvi.iItem;

            result = DialogBox(hInstance_for_id3editor, MAKEINTRESOURCE(IDD_EDITTEXTFRAME),
                hwndApp, EditTextFrameProc);
            if (LOWORD(result) == IDOK)
                return TRUE;
        }
    }
    return FALSE;
}


/* Delete the selected frame */
BOOL List_DeleteSelected(HWND hwndApp, HWND hwndList)
{
    int items;

    /* First get the selected item */
    int index = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);

    if (index != -1)
        ListView_DeleteItem(hwndList, index);

    items = ListView_GetItemCount(hwndList);
    if (index != -1) return TRUE;
    else return FALSE;
}


/* Save the ID3 to the file */
void List_SaveID3(HWND hwndApp, HWND hwndList, char *filename)
{
    LV_ITEM lvi;
    ID3ITEM *pItem1;
    int i, items;
    ID3Tag *tag;
    ID3Frame *frame;
    ID3Field *field;
    
    /* Strip the tag first, before completely rewriting it */
    if ((tag = ID3Tag_New()) != NULL)
    {
        ID3Tag_Link(tag, filename);
        ID3Tag_Strip(tag, ID3TT_ALL);
        ID3Tag_Clear(tag);

        if (SendMessage(GetDlgItem(hwndApp, IDC_ID3V2TAG), BM_GETCHECK, 0, 0) == BST_UNCHECKED)
        {
            /* No frames saved */
            ID3Tag_Delete(tag);
            EnableWindow(GetDlgItem(hwndApp, IDC_ID3V2TAG), FALSE);
            ListView_DeleteAllItems(hwndList);
            return;
        }

        /* First get the number of items */
        items = ListView_GetItemCount(hwndList);

        if (items > 0)
        {
            for (i = 0; i < items; i++)
            {
                lvi.mask = LVIF_PARAM;
                lvi.iItem = i;
                lvi.iSubItem = 0;

                if (ListView_GetItem(hwndList, &lvi) == TRUE)
                {
                    pItem1 = (ID3ITEM*)lvi.lParam;

                    frame = ID3Frame_NewID(pItem1->frameId);

                    switch (pItem1->frameId)
                    {
                    case ID3FID_ALBUM:            case ID3FID_BPM:
                    case ID3FID_COMPOSER:         case ID3FID_CONTENTTYPE:
                    case ID3FID_COPYRIGHT:        case ID3FID_DATE:
                    case ID3FID_PLAYLISTDELAY:    case ID3FID_ENCODEDBY:
                    case ID3FID_LYRICIST:         case ID3FID_FILETYPE:
                    case ID3FID_TIME:             case ID3FID_CONTENTGROUP:
                    case ID3FID_TITLE:            case ID3FID_SUBTITLE:
                    case ID3FID_INITIALKEY:       case ID3FID_LANGUAGE:
                    case ID3FID_SONGLEN:          case ID3FID_MEDIATYPE:
                    case ID3FID_ORIGALBUM:        case ID3FID_ORIGFILENAME:
                    case ID3FID_ORIGLYRICIST:     case ID3FID_ORIGARTIST:
                    case ID3FID_ORIGYEAR:         case ID3FID_FILEOWNER:
                    case ID3FID_LEADARTIST:       case ID3FID_BAND:
                    case ID3FID_CONDUCTOR:        case ID3FID_MIXARTIST:
                    case ID3FID_PARTINSET:        case ID3FID_PUBLISHER:
                    case ID3FID_TRACKNUM:         case ID3FID_RECORDINGDATES:
                    case ID3FID_NETRADIOSTATION:  case ID3FID_NETRADIOOWNER:
                    case ID3FID_SIZE:             case ID3FID_ISRC:
                    case ID3FID_ENCODERSETTINGS:  case ID3FID_YEAR:
                    {
                        field = ID3Frame_GetField(frame, ID3FN_TEXT);
                        ID3Field_SetASCII(field, pItem1->aCols[1]);
                        ID3Tag_AddFrame(tag, frame);
                        break;
                    }
                    case ID3FID_USERTEXT:
                    case ID3FID_COMMENT: /* Can also contain an extra language field (but not used now) */
                    case ID3FID_UNSYNCEDLYRICS: /* Can also contain an extra language field (but not used now) */
                    {
                        field = ID3Frame_GetField(frame, ID3FN_DESCRIPTION);
                        ID3Field_SetASCII(field, pItem1->aCols[0]);
                        field = ID3Frame_GetField(frame, ID3FN_TEXT);
                        ID3Field_SetASCII(field, pItem1->aCols[1]);
                        ID3Tag_AddFrame(tag, frame);
                        break;
                    }
                    case ID3FID_WWWAUDIOFILE:     case ID3FID_WWWARTIST:
                    case ID3FID_WWWAUDIOSOURCE:   case ID3FID_WWWCOMMERCIALINFO:
                    case ID3FID_WWWCOPYRIGHT:     case ID3FID_WWWPUBLISHER:
                    case ID3FID_WWWPAYMENT:       case ID3FID_WWWRADIOPAGE:
                    {
                        field = ID3Frame_GetField(frame, ID3FN_URL);
                        ID3Field_SetASCII(field, pItem1->aCols[1]);
                        ID3Tag_AddFrame(tag, frame);
                        break;
                    }
                    case ID3FID_WWWUSER:
                    {
                        field = ID3Frame_GetField(frame, ID3FN_DESCRIPTION);
                        ID3Field_SetASCII(field, pItem1->aCols[0]);
                        field = ID3Frame_GetField(frame, ID3FN_URL);
                        ID3Field_SetASCII(field, pItem1->aCols[1]);
                        ID3Tag_AddFrame(tag, frame);
                        break;
                    }
                    default:
                        break;
                    }
                }
            }
            ID3Tag_UpdateByTagType(tag, ID3TT_ID3V2);
        }

        ID3Tag_Delete(tag);
    }
}

/* Get the title from the file */
void GetID3FileTitle(char *filename, char *title, char *format)
{
    ID3Tag *tag;
    ID3Frame *frame;
    ID3Field *field;
    char buffer[255];
    int some_info = 0;
    char *in = format;
    char *out = title;
    char *bound = out + (MAX_PATH - 10 - 1);


    if ((tag = ID3Tag_New()) != NULL)
    {
        ID3Tag_Link(tag, filename);

        while (*in && out < bound)
        {
            switch (*in) {
            case '%':
                ++in;
                break;

            default:
                *out++ = *in++;
                continue;
            }

            /* handle % escape sequence */
            switch (*in++) {
            case '0':
                if ((frame = ID3Tag_FindFrameWithID(tag, ID3FID_TRACKNUM)) != NULL) {
                    int size;
                    field = ID3Frame_GetField(frame, ID3FN_TEXT);
                    size = ID3Field_GetASCII(field, buffer, 255, 1);
                    lstrcpy(out, buffer); out += size;
                    some_info = 1;
                }
                break;
            case '1':
                if ((frame = ID3Tag_FindFrameWithID(tag, ID3FID_LEADARTIST)) != NULL) {
                    int size;
                    field = ID3Frame_GetField(frame, ID3FN_TEXT);
                    size = ID3Field_GetASCII(field, buffer, 255, 1);
                    lstrcpy(out, buffer); out += size;
                    some_info = 1;
                }
                break;
            case '2':
                if ((frame = ID3Tag_FindFrameWithID(tag, ID3FID_TITLE)) != NULL) {
                    int size;
                    field = ID3Frame_GetField(frame, ID3FN_TEXT);
                    size = ID3Field_GetASCII(field, buffer, 255, 1);
                    lstrcpy(out, buffer); out += size;
                    some_info = 1;
                }
                break;
            case '3':
                if ((frame = ID3Tag_FindFrameWithID(tag, ID3FID_ALBUM)) != NULL) {
                    int size;
                    field = ID3Frame_GetField(frame, ID3FN_TEXT);
                    size = ID3Field_GetASCII(field, buffer, 255, 1);
                    lstrcpy(out, buffer); out += size;
                    some_info = 1;
                }
                break;
            case '4':
                if ((frame = ID3Tag_FindFrameWithID(tag, ID3FID_YEAR)) != NULL) {
                    int size;
                    field = ID3Frame_GetField(frame, ID3FN_TEXT);
                    size = ID3Field_GetASCII(field, buffer, 255, 1);
                    lstrcpy(out, buffer); out += size;
                    some_info = 1;
                }
                break;
            case '5':
                if ((frame = ID3Tag_FindFrameWithID(tag, ID3FID_COMMENT)) != NULL) {
                    int size;
                    field = ID3Frame_GetField(frame, ID3FN_TEXT);
                    size = ID3Field_GetASCII(field, buffer, 255, 1);
                    lstrcpy(out, buffer); out += size;
                    some_info = 1;
                }
                break;
            case '6':
                if ((frame = ID3Tag_FindFrameWithID(tag, ID3FID_CONTENTTYPE)) != NULL) {
                    int size; char *tmp;
                    field = ID3Frame_GetField(frame, ID3FN_TEXT);
                    size = ID3Field_GetASCII(field, buffer, 255, 1);
                    tmp = GetGenre(buffer);
                    lstrcpy(out, tmp); out += size;
                    some_info = 1;
                }
                break;
            case '7':
            {
                char *p=filename+lstrlen(filename);
                int len = 0;
                while (*p != '\\' && p >= filename) { p--; len++; }
                lstrcpy(out, ++p); out += len;
                some_info = 1;
                break;
            }
            }
        }

        *out = '\0';
        ID3Tag_Delete(tag);
    }

    if (!some_info)
    {
        char *p=filename+lstrlen(filename);
        while (*p != '\\' && p >= filename) p--;
        lstrcpy(title,++p);
    }
}

