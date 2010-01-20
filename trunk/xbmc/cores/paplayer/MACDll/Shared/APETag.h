#ifndef APE_APETAG_H
#define APE_APETAG_H

class CIO;

/*****************************************************************************************
APETag version history / supported formats

1.0 (1000) - Original APE tag spec.  Fully supported by this code.
2.0 (2000) - Refined APE tag spec (better streaming support, UTF encoding). Fully supported by this code.

Notes:
    - also supports reading of ID3v1.1 tags
    - all saving done in the APE Tag format using CURRENT_APE_TAG_VERSION
*****************************************************************************************/

/*****************************************************************************************
APETag layout

1) Header - APE_TAG_FOOTER (optional) (32 bytes)
2) Fields (array):
        Value Size (4 bytes)
        Flags (4 bytes)
        Field Name (? ANSI bytes -- requires NULL terminator -- in range of 0x20 (space) to 0x7E (tilde))
        Value ([Value Size] bytes)
3) Footer - APE_TAG_FOOTER (32 bytes)    
*****************************************************************************************/

/*****************************************************************************************
Notes

-When saving images, store the filename (no directory -- i.e. Cover.jpg) in UTF-8 followed 
by a null terminator, followed by the image data.
*****************************************************************************************/

/*****************************************************************************************
The version of the APE tag
*****************************************************************************************/
#define CURRENT_APE_TAG_VERSION                 2000

/*****************************************************************************************
"Standard" APE tag fields
*****************************************************************************************/
#define APE_TAG_FIELD_TITLE                     L"Title"
#define APE_TAG_FIELD_ARTIST                    L"Artist"
#define APE_TAG_FIELD_ALBUM                     L"Album"
#define APE_TAG_FIELD_COMMENT                   L"Comment"
#define APE_TAG_FIELD_YEAR                      L"Year"
#define APE_TAG_FIELD_TRACK                     L"Track"
#define APE_TAG_FIELD_GENRE                     L"Genre"
#define APE_TAG_FIELD_COVER_ART_FRONT           L"Cover Art (front)"
#define APE_TAG_FIELD_NOTES                     L"Notes"
#define APE_TAG_FIELD_LYRICS                    L"Lyrics"
#define APE_TAG_FIELD_COPYRIGHT                 L"Copyright"
#define APE_TAG_FIELD_BUY_URL                   L"Buy URL"
#define APE_TAG_FIELD_ARTIST_URL                L"Artist URL"
#define APE_TAG_FIELD_PUBLISHER_URL             L"Publisher URL"
#define APE_TAG_FIELD_FILE_URL                  L"File URL"
#define APE_TAG_FIELD_COPYRIGHT_URL             L"Copyright URL"
#define APE_TAG_FIELD_MJ_METADATA               L"Media Jukebox Metadata"
#define APE_TAG_FIELD_TOOL_NAME                 L"Tool Name"
#define APE_TAG_FIELD_TOOL_VERSION              L"Tool Version"
#define APE_TAG_FIELD_PEAK_LEVEL                L"Peak Level"
#define APE_TAG_FIELD_REPLAY_GAIN_RADIO         L"Replay Gain (radio)"
#define APE_TAG_FIELD_REPLAY_GAIN_ALBUM         L"Replay Gain (album)"
#define APE_TAG_FIELD_COMPOSER                  L"Composer"
#define APE_TAG_FIELD_KEYWORDS                  L"Keywords"

/*****************************************************************************************
Standard APE tag field values
*****************************************************************************************/
#define APE_TAG_GENRE_UNDEFINED                 L"Undefined"

/*****************************************************************************************
ID3 v1.1 tag
*****************************************************************************************/
#define ID3_TAG_BYTES    128
struct ID3_TAG
{
    char Header[3];             // should equal 'TAG'
    char Title[30];             // title
    char Artist[30];            // artist
    char Album[30];             // album
    char Year[4];               // year
    char Comment[29];           // comment
    unsigned char Track;        // track
    unsigned char Genre;        // genre
};

/*****************************************************************************************
Footer (and header) flags
*****************************************************************************************/
#define APE_TAG_FLAG_CONTAINS_HEADER            (1 << 31)
#define APE_TAG_FLAG_CONTAINS_FOOTER            (1 << 30)
#define APE_TAG_FLAG_IS_HEADER                  (1 << 29)

#define APE_TAG_FLAGS_DEFAULT                   (APE_TAG_FLAG_CONTAINS_FOOTER)

/*****************************************************************************************
Tag field flags
*****************************************************************************************/
#define TAG_FIELD_FLAG_READ_ONLY                (1 << 0)

#define TAG_FIELD_FLAG_DATA_TYPE_MASK           (6)
#define TAG_FIELD_FLAG_DATA_TYPE_TEXT_UTF8      (0 << 1)
#define TAG_FIELD_FLAG_DATA_TYPE_BINARY         (1 << 1)
#define TAG_FIELD_FLAG_DATA_TYPE_EXTERNAL_INFO  (2 << 1)
#define TAG_FIELD_FLAG_DATA_TYPE_RESERVED       (3 << 1)

/*****************************************************************************************
The footer at the end of APE tagged files (can also optionally be at the front of the tag)
*****************************************************************************************/
#define APE_TAG_FOOTER_BYTES    32

class APE_TAG_FOOTER
{
protected:

    char m_cID[8];              // should equal 'APETAGEX'    
    int m_nVersion;             // equals CURRENT_APE_TAG_VERSION
    int m_nSize;                // the complete size of the tag, including this footer (excludes header)
    int m_nFields;              // the number of fields in the tag
    int m_nFlags;               // the tag flags
    char m_cReserved[8];        // reserved for later use (must be zero)

public:

    APE_TAG_FOOTER(int nFields = 0, int nFieldBytes = 0)
    {
        memcpy(m_cID, "APETAGEX", 8);
        memset(m_cReserved, 0, 8);
        m_nFields = nFields;
        m_nFlags = APE_TAG_FLAGS_DEFAULT;
        m_nSize = nFieldBytes + APE_TAG_FOOTER_BYTES;
        m_nVersion = CURRENT_APE_TAG_VERSION;
    }

    int GetTotalTagBytes() { return m_nSize + (GetHasHeader() ? APE_TAG_FOOTER_BYTES : 0); }
    int GetFieldBytes() { return m_nSize - APE_TAG_FOOTER_BYTES; }
    int GetFieldsOffset() { return GetHasHeader() ? APE_TAG_FOOTER_BYTES : 0; }
    int GetNumberFields() { return m_nFields; }
    BOOL GetHasHeader() { return (m_nFlags & APE_TAG_FLAG_CONTAINS_HEADER) ? TRUE : FALSE; }
    BOOL GetIsHeader() { return (m_nFlags & APE_TAG_FLAG_IS_HEADER) ? TRUE : FALSE; }
    int GetVersion() { return m_nVersion; }

    BOOL GetIsValid(BOOL bAllowHeader)
    {
        BOOL bValid = (strncmp(m_cID, "APETAGEX", 8) == 0) && 
            (m_nVersion <= CURRENT_APE_TAG_VERSION) &&
            (m_nFields <= 65536) &&
            (GetFieldBytes() <= (1024 * 1024 * 16));
        
        if (bValid && (bAllowHeader == FALSE) && GetIsHeader())
            bValid = FALSE;

        return bValid ? TRUE : FALSE;
    }
};

/*****************************************************************************************
CAPETagField class (an APE tag is an array of these)
*****************************************************************************************/
class CAPETagField
{
public:

    // create a tag field (use nFieldBytes = -1 for null-terminated strings)
    CAPETagField(const str_utf16 * pFieldName, const void * pFieldValue, int nFieldBytes = -1, int nFlags = 0);
    
    // destructor
    ~CAPETagField();

    // gets the size of the entire field in bytes (name, value, and metadata)
    int GetFieldSize();
    
    // get the name of the field
    const str_utf16 * GetFieldName();

    // get the value of the field
    const char * GetFieldValue();
    
    // get the size of the value (in bytes)
    int GetFieldValueSize();

    // get any special flags
    int GetFieldFlags();
    
    // output the entire field to a buffer (GetFieldSize() bytes)
    int SaveField(char * pBuffer);

    // checks to see if the field is read-only
    BOOL GetIsReadOnly() { return (m_nFieldFlags & TAG_FIELD_FLAG_READ_ONLY) ? TRUE : FALSE; }
    BOOL GetIsUTF8Text() { return ((m_nFieldFlags & TAG_FIELD_FLAG_DATA_TYPE_MASK) == TAG_FIELD_FLAG_DATA_TYPE_TEXT_UTF8) ? TRUE : FALSE; }

    // set helpers (use with EXTREME caution)
    void SetFieldFlags(int nFlags) { m_nFieldFlags = nFlags; }

private:
        
    CSmartPtr<str_utf16> m_spFieldNameUTF16;
    CSmartPtr<char> m_spFieldValue;
    int m_nFieldFlags;
    int m_nFieldValueBytes;
};

/*****************************************************************************************
CAPETag class
*****************************************************************************************/
class CAPETag
{
public:

    // create an APE tag 
    // bAnalyze determines whether it will analyze immediately or on the first request
    // be careful with multiple threads / file pointer movement if you don't analyze immediately
    CAPETag(CIO * pIO, BOOL bAnalyze = TRUE);
    CAPETag(const str_utf16 * pFilename, BOOL bAnalyze = TRUE);
    
    // destructor
    ~CAPETag();

    // save the tag to the I/O source (bUseOldID3 forces it to save as an ID3v1.1 tag instead of an APE tag)
    int Save(BOOL bUseOldID3 = FALSE);
    
    // removes any tags from the file (bUpdate determines whether is should re-analyze after removing the tag)
    int Remove(BOOL bUpdate = TRUE);

    // sets the value of a field (use nFieldBytes = -1 for null terminated strings)
    // note: using NULL or "" for a string type will remove the field
    int SetFieldString(const str_utf16 * pFieldName, const str_utf16 * pFieldValue);
    int SetFieldString(const str_utf16 * pFieldName, const char * pFieldValue, BOOL bAlreadyUTF8Encoded);
    int SetFieldBinary(const str_utf16 * pFieldName, const void * pFieldValue, int nFieldBytes, int nFieldFlags);

    // gets the value of a field (returns -1 and an empty buffer if the field doesn't exist)
    int GetFieldBinary(const str_utf16 * pFieldName, void * pBuffer, int * pBufferBytes);
    int GetFieldString(const str_utf16 * pFieldName, str_utf16 * pBuffer, int * pBufferCharacters);
    int GetFieldString(const str_utf16 * pFieldName, str_ansi * pBuffer, int * pBufferCharacters, BOOL bUTF8Encode = FALSE);

    // remove a specific field
    int RemoveField(const str_utf16 * pFieldName);
    int RemoveField(int nIndex);

    // clear all the fields
    int ClearFields();
    
    // get the total tag bytes in the file from the last analyze
    // need to call Save() then Analyze() to update any changes
    int GetTagBytes();

    // fills in an ID3_TAG using the current fields (useful for quickly converting the tag)
    int CreateID3Tag(ID3_TAG * pID3Tag);

    // see whether the file has an ID3 or APE tag
    BOOL GetHasID3Tag() { if (m_bAnalyzed == FALSE) { Analyze(); } return m_bHasID3Tag;    }
    BOOL GetHasAPETag() { if (m_bAnalyzed == FALSE) { Analyze(); } return m_bHasAPETag;    }
    int GetAPETagVersion() { return GetHasAPETag() ? m_nAPETagVersion : -1;    }

    // gets a desired tag field (returns NULL if not found)
    // again, be careful, because this a pointer to the actual field in this class
    CAPETagField * GetTagField(const str_utf16 * pFieldName);
    CAPETagField * GetTagField(int nIndex);

    // options
    void SetIgnoreReadOnly(BOOL bIgnoreReadOnly) { m_bIgnoreReadOnly = bIgnoreReadOnly; }

private:

    // private functions
    int Analyze();
    int GetTagFieldIndex(const str_utf16 * pFieldName);
    int WriteBufferToEndOfIO(void * pBuffer, int nBytes);
    int LoadField(const char * pBuffer, int nMaximumBytes, int * pBytes);
    int SortFields();
    static int CompareFields(const void * pA, const void * pB);

    // helper set / get field functions
    int SetFieldID3String(const str_utf16 * pFieldName, const char * pFieldValue, int nBytes);
    int GetFieldID3String(const str_utf16 * pFieldName, char * pBuffer, int nBytes);

    // private data
    CSmartPtr<CIO> m_spIO;
    BOOL m_bAnalyzed;
    int m_nTagBytes;
    int m_nFields;
    CAPETagField * m_aryFields[256];
    BOOL m_bHasAPETag;
    int m_nAPETagVersion;
    BOOL m_bHasID3Tag;
    BOOL m_bIgnoreReadOnly;
};

#endif // #ifndef APE_APETAG_H

