#include "All.h"
#include "ID3Genres.h"
#include "APETag.h"
#include "CharacterHelper.h"
#include "IO.h"
#include IO_HEADER_FILE

#include <wchar.h>

/*****************************************************************************************
CAPETagField
*****************************************************************************************/

CAPETagField::CAPETagField(const str_utf16 * pFieldName, const void * pFieldValue, int nFieldBytes, int nFlags)
{
    // field name
    m_spFieldNameUTF16.Assign(new str_utf16 [wcslen(pFieldName) + 1], TRUE);
    memcpy(m_spFieldNameUTF16, pFieldName, (wcslen(pFieldName) + 1) * sizeof(str_utf16));
    
    // data (we'll always allocate two extra bytes and memset to 0 so we're safely NULL terminated)
    m_nFieldValueBytes = max(nFieldBytes, 0);
    m_spFieldValue.Assign(new char [m_nFieldValueBytes + 2], TRUE);
    memset(m_spFieldValue, 0, m_nFieldValueBytes + 2);
    if (m_nFieldValueBytes > 0)
        memcpy(m_spFieldValue, pFieldValue, m_nFieldValueBytes);

    // flags
    m_nFieldFlags = nFlags;
}

CAPETagField::~CAPETagField()
{
}
    
int CAPETagField::GetFieldSize()
{
    CSmartPtr<char> spFieldNameANSI(GetANSIFromUTF16(m_spFieldNameUTF16), TRUE); 
    return (strlen(spFieldNameANSI) + 1) + m_nFieldValueBytes + 4 + 4;
}

const str_utf16 * CAPETagField::GetFieldName()
{
    return m_spFieldNameUTF16;
}

const char * CAPETagField::GetFieldValue()
{
    return m_spFieldValue;
}

int CAPETagField::GetFieldValueSize()
{
    return m_nFieldValueBytes;
}

int CAPETagField::GetFieldFlags()
{
    return m_nFieldFlags;
}

int CAPETagField::SaveField(char * pBuffer)
{
    *((int *) pBuffer) = m_nFieldValueBytes;
    pBuffer += 4;
    *((int *) pBuffer) = m_nFieldFlags;
    pBuffer += 4;
    
    CSmartPtr<char> spFieldNameANSI((char *) GetANSIFromUTF16(m_spFieldNameUTF16), TRUE); 
    strcpy(pBuffer, spFieldNameANSI);
    pBuffer += strlen(spFieldNameANSI) + 1;

    memcpy(pBuffer, m_spFieldValue, m_nFieldValueBytes);

    return GetFieldSize();
}


/*****************************************************************************************
CAPETag
*****************************************************************************************/

CAPETag::CAPETag(const str_utf16 * pFilename, BOOL bAnalyze)
{
    m_spIO.Assign(new IO_CLASS_NAME);
    m_spIO->Open(pFilename);

    m_bAnalyzed = FALSE;
    m_nFields = 0;
    m_nTagBytes = 0;
    m_bIgnoreReadOnly = FALSE;
    
    if (bAnalyze)
    {
        Analyze();
    }
}

CAPETag::CAPETag(CIO * pIO, BOOL bAnalyze)
{
    m_spIO.Assign(pIO, FALSE, FALSE); // we don't own the IO source
    m_bAnalyzed = FALSE;
    m_nFields = 0;
    m_nTagBytes = 0;
    
    if (bAnalyze)
    {
        Analyze();
    }
}

CAPETag::~CAPETag()
{
    ClearFields();
}

int CAPETag::GetTagBytes()
{
    if (m_bAnalyzed == FALSE) { Analyze(); }

    return m_nTagBytes;
}

CAPETagField * CAPETag::GetTagField(int nIndex)
{
    if (m_bAnalyzed == FALSE) { Analyze(); }

    if ((nIndex >= 0) && (nIndex < m_nFields))
    {
        return m_aryFields[nIndex];
    }

    return NULL;
}

int CAPETag::Save(BOOL bUseOldID3)
{
    if (Remove(FALSE) != ERROR_SUCCESS)
        return -1;
    
    if (m_nFields == 0) { return ERROR_SUCCESS; }

    int nRetVal = -1;

    if (bUseOldID3 == FALSE)
    {
        int z = 0;

        // calculate the size of the whole tag
        int nFieldBytes = 0;
        for (z = 0; z < m_nFields; z++)
            nFieldBytes += m_aryFields[z]->GetFieldSize();

        // sort the fields
        SortFields();

        // build the footer
        APE_TAG_FOOTER APETagFooter(m_nFields, nFieldBytes);

        // make a buffer for the tag
        int nTotalTagBytes = APETagFooter.GetTotalTagBytes();
        CSmartPtr<char> spRawTag(new char [nTotalTagBytes], TRUE);

        // save the fields
        int nLocation = 0;
        for (z = 0; z < m_nFields; z++)
            nLocation += m_aryFields[z]->SaveField(&spRawTag[nLocation]);

        // add the footer to the buffer
        memcpy(&spRawTag[nLocation], &APETagFooter, APE_TAG_FOOTER_BYTES);
        nLocation += APE_TAG_FOOTER_BYTES;

        // dump the tag to the I/O source
        nRetVal = WriteBufferToEndOfIO(spRawTag, nTotalTagBytes);
    }
    else
    {
        // build the ID3 tag
        ID3_TAG ID3Tag;
        CreateID3Tag(&ID3Tag);
        nRetVal = WriteBufferToEndOfIO(&ID3Tag, sizeof(ID3_TAG));
    }

    return nRetVal;
}

int CAPETag::WriteBufferToEndOfIO(void * pBuffer, int nBytes)
{
    int nOriginalPosition = m_spIO->GetPosition();
    
    unsigned int nBytesWritten = 0;
    m_spIO->Seek(0, FILE_END);

    int nRetVal = m_spIO->Write(pBuffer, nBytes, &nBytesWritten);
    
    m_spIO->Seek(nOriginalPosition, FILE_BEGIN);

    return nRetVal;
}

int CAPETag::Analyze()
{
    // clean-up
    ID3_TAG ID3Tag;
    ClearFields();
    m_nTagBytes = 0;

    m_bAnalyzed = TRUE;

    // store the original location
    int nOriginalPosition = m_spIO->GetPosition();
    
    // check for a tag
    unsigned int nBytesRead;
    int nRetVal;
    m_bHasID3Tag = FALSE;
    m_bHasAPETag = FALSE;
    m_nAPETagVersion = -1;
    m_spIO->Seek(-ID3_TAG_BYTES, FILE_END);
    nRetVal = m_spIO->Read((unsigned char *) &ID3Tag, sizeof(ID3_TAG), &nBytesRead);
    
    if ((nBytesRead == sizeof(ID3_TAG)) && (nRetVal == 0))
    {
        if (ID3Tag.Header[0] == 'T' && ID3Tag.Header[1] == 'A' && ID3Tag.Header[2] == 'G') 
        {
            m_bHasID3Tag = TRUE;
            m_nTagBytes += ID3_TAG_BYTES;
        }
    }
    
    // set the fields
    if (m_bHasID3Tag)
    {
        SetFieldID3String(APE_TAG_FIELD_ARTIST, ID3Tag.Artist, 30);
        SetFieldID3String(APE_TAG_FIELD_ALBUM, ID3Tag.Album, 30);
        SetFieldID3String(APE_TAG_FIELD_TITLE, ID3Tag.Title, 30);
        SetFieldID3String(APE_TAG_FIELD_COMMENT, ID3Tag.Comment, 28);
        SetFieldID3String(APE_TAG_FIELD_YEAR, ID3Tag.Year, 4);
        
        char cTemp[16]; sprintf(cTemp, "%d", ID3Tag.Track);
        SetFieldString(APE_TAG_FIELD_TRACK, cTemp, FALSE);

        if ((ID3Tag.Genre == GENRE_UNDEFINED) || (ID3Tag.Genre >= GENRE_COUNT)) 
            SetFieldString(APE_TAG_FIELD_GENRE, APE_TAG_GENRE_UNDEFINED);
        else 
            SetFieldString(APE_TAG_FIELD_GENRE, g_ID3Genre[ID3Tag.Genre]);
    }

    // try loading the APE tag
    if (m_bHasID3Tag == FALSE)
    {
        APE_TAG_FOOTER APETagFooter;
        m_spIO->Seek(-int(APE_TAG_FOOTER_BYTES), FILE_END);
        nRetVal = m_spIO->Read((unsigned char *) &APETagFooter, APE_TAG_FOOTER_BYTES, &nBytesRead);
        if ((nBytesRead == APE_TAG_FOOTER_BYTES) && (nRetVal == 0))
        {
            if (APETagFooter.GetIsValid(FALSE))
            {
                m_bHasAPETag = TRUE;
                m_nAPETagVersion = APETagFooter.GetVersion();

                int nRawFieldBytes = APETagFooter.GetFieldBytes();
                m_nTagBytes += APETagFooter.GetTotalTagBytes();
                
                CSmartPtr<char> spRawTag(new char [nRawFieldBytes], TRUE);
                m_spIO->Seek(-(APETagFooter.GetTotalTagBytes() - APETagFooter.GetFieldsOffset()), FILE_END);
                nRetVal = m_spIO->Read((unsigned char *) spRawTag.GetPtr(), nRawFieldBytes, &nBytesRead);

                if ((nRetVal == 0) && (nRawFieldBytes == int(nBytesRead)))
                {
                    // parse out the raw fields
                    int nLocation = 0;
                    for (int z = 0; z < APETagFooter.GetNumberFields(); z++)
                    {
                        int nMaximumFieldBytes = nRawFieldBytes - nLocation;
                        
                        int nBytes = 0;
                        if (LoadField(&spRawTag[nLocation], nMaximumFieldBytes, &nBytes) != ERROR_SUCCESS)
                        {
                            // if LoadField(...) fails, it means that the tag is corrupt (accidently or intentionally)
                            // we'll just bail out -- leaving the fields we've already set
                            break;
                        }
                        nLocation += nBytes;
                    }
                }
            }
        }
    }

    // restore the file pointer
    m_spIO->Seek(nOriginalPosition, FILE_BEGIN);
    
    return ERROR_SUCCESS;
}

int CAPETag::ClearFields()
{
    for (int z = 0; z < m_nFields; z++)
    {
        SAFE_DELETE(m_aryFields[z])
    }
    
    m_nFields = 0;

    return ERROR_SUCCESS;
}

int CAPETag::GetTagFieldIndex(const str_utf16 * pFieldName)
{
    if (m_bAnalyzed == FALSE) { Analyze(); }
    if (pFieldName == NULL) return -1;

    for (int z = 0; z < m_nFields; z++)
    {
        if (wcsicmp(m_aryFields[z]->GetFieldName(), pFieldName) == 0)
            return z;
    }

    return -1;

}

CAPETagField * CAPETag::GetTagField(const str_utf16 * pFieldName)
{
    int nIndex = GetTagFieldIndex(pFieldName);
    return (nIndex != -1) ? m_aryFields[nIndex] : NULL;
}

int CAPETag::GetFieldString(const str_utf16 * pFieldName, str_ansi * pBuffer, int * pBufferCharacters, BOOL bUTF8Encode)
{
    int nOriginalCharacters = *pBufferCharacters;
    str_utf16 * pUTF16 = new str_utf16 [*pBufferCharacters + 1];
    pUTF16[0] = 0;

    int nRetVal = GetFieldString(pFieldName, pUTF16, pBufferCharacters);
    if (nRetVal == ERROR_SUCCESS)
    {
        CSmartPtr<str_ansi> spANSI(bUTF8Encode ? (str_ansi *) GetUTF8FromUTF16(pUTF16) : GetANSIFromUTF16(pUTF16), TRUE);
        if (int(strlen(spANSI)) > nOriginalCharacters)
        {
            memset(pBuffer, 0, nOriginalCharacters * sizeof(str_ansi));
            *pBufferCharacters = 0;
            nRetVal = ERROR_UNDEFINED;
        }
        else
        {
            strcpy(pBuffer, spANSI);
            *pBufferCharacters = strlen(spANSI);
        }
    }
    
    delete [] pUTF16;

    return nRetVal;
}


int CAPETag::GetFieldString(const str_utf16 * pFieldName, str_utf16 * pBuffer, int * pBufferCharacters)
{
    if (m_bAnalyzed == FALSE) { Analyze(); }

    int nRetVal = ERROR_UNDEFINED;

    if (*pBufferCharacters > 0)
    {
        CAPETagField * pAPETagField = GetTagField(pFieldName);
        if (pAPETagField == NULL)
        {
            // the field doesn't exist -- return an empty string
            memset(pBuffer, 0, *pBufferCharacters * sizeof(str_utf16));
            *pBufferCharacters = 0;
        }
        else if (pAPETagField->GetIsUTF8Text() || (m_nAPETagVersion < 2000))
        {
            // get the value in UTF-16 format
            CSmartPtr<str_utf16> spUTF16;
            if (m_nAPETagVersion >= 2000)
                spUTF16.Assign(GetUTF16FromUTF8((str_utf8 *) pAPETagField->GetFieldValue()), TRUE);
            else
                spUTF16.Assign(GetUTF16FromANSI(pAPETagField->GetFieldValue()), TRUE);

            // get the number of characters
            int nCharacters = (wcslen(spUTF16) + 1);
            if (nCharacters > *pBufferCharacters)
            {
                // we'll fail here, because it's not clear what would get returned (null termination, size, etc.)
                // and we really don't want to cause buffer overruns on the client side
                *pBufferCharacters = nCharacters;
            }
            else
            {
                // just copy in
                *pBufferCharacters = nCharacters;
                memcpy(pBuffer, spUTF16.GetPtr(), *pBufferCharacters * sizeof(str_utf16));
                nRetVal = ERROR_SUCCESS;
            }
        }
        else
        {
            // memset the whole buffer to NULL (so everything left over is NULL terminated)
            memset(pBuffer, 0, *pBufferCharacters * sizeof(str_utf16));

            // do a binary dump (need to convert from wchar's to bytes)
            int nBufferBytes = (*pBufferCharacters - 1) * sizeof(str_utf16);
            nRetVal = GetFieldBinary(pFieldName, pBuffer, &nBufferBytes);
            *pBufferCharacters = (nBufferBytes / sizeof(str_utf16)) + 1;
        }
    }

    return nRetVal;
}

int CAPETag::GetFieldBinary(const str_utf16 * pFieldName, void * pBuffer, int * pBufferBytes)
{
    if (m_bAnalyzed == FALSE) { Analyze(); }
    
    int nRetVal = ERROR_UNDEFINED;

    if (*pBufferBytes > 0)
    {
        CAPETagField * pAPETagField = GetTagField(pFieldName);
        if (pAPETagField == NULL)
        {
            memset(pBuffer, 0, *pBufferBytes);
            *pBufferBytes = 0;
        }
        else
        {
            if (pAPETagField->GetFieldValueSize() > *pBufferBytes)
            {
                // we'll fail here, because partial data may be worse than no data
                memset(pBuffer, 0, *pBufferBytes);
                *pBufferBytes = pAPETagField->GetFieldValueSize();
            }
            else
            {
                // memcpy
                *pBufferBytes = pAPETagField->GetFieldValueSize();
                memcpy(pBuffer, pAPETagField->GetFieldValue(), *pBufferBytes);
                nRetVal = ERROR_SUCCESS;
            }
        }
    }

    return nRetVal;
}

int CAPETag::CreateID3Tag(ID3_TAG * pID3Tag)
{
    // error check 
    if (pID3Tag == NULL) { return -1; }
    if (m_bAnalyzed == FALSE) { Analyze(); }
    if (m_nFields == 0) { return -1; }

    // empty
    ZeroMemory(pID3Tag, ID3_TAG_BYTES);

    // header
    pID3Tag->Header[0] = 'T'; pID3Tag->Header[1] = 'A'; pID3Tag->Header[2] = 'G';
    
    // standard fields
    GetFieldID3String(APE_TAG_FIELD_ARTIST, pID3Tag->Artist, 30);
    GetFieldID3String(APE_TAG_FIELD_ALBUM, pID3Tag->Album, 30);
    GetFieldID3String(APE_TAG_FIELD_TITLE, pID3Tag->Title, 30);
    GetFieldID3String(APE_TAG_FIELD_COMMENT, pID3Tag->Comment, 28);
    GetFieldID3String(APE_TAG_FIELD_YEAR, pID3Tag->Year, 4);

    // track number
    str_utf16 cBuffer[256] = { 0 }; int nBufferCharacters = 255;
    GetFieldString(APE_TAG_FIELD_TRACK, cBuffer, &nBufferCharacters);
    pID3Tag->Track = (unsigned char) wcstol(cBuffer,NULL,0);
    
    // genre
    cBuffer[0] = 0; nBufferCharacters = 255;
    GetFieldString(APE_TAG_FIELD_GENRE, cBuffer, &nBufferCharacters);
        
    // convert the genre string to an index
    pID3Tag->Genre = 255;
    int nGenreIndex = 0;
    BOOL bFound = FALSE;
    while ((nGenreIndex < GENRE_COUNT) && (bFound == FALSE))
    {
        if (wcsicmp(cBuffer, g_ID3Genre[nGenreIndex]) == 0)
        {
            pID3Tag->Genre = nGenreIndex;
            bFound = TRUE;
        }
        
        nGenreIndex++;
    }

    return ERROR_SUCCESS;
}

int CAPETag::LoadField(const char * pBuffer, int nMaximumBytes, int * pBytes)
{
    // set bytes to 0
    if (pBytes) *pBytes = 0;

    // size and flags
    int nLocation = 0;
    int nFieldValueSize = *((int *) &pBuffer[nLocation]);
    nLocation += 4;
    int nFieldFlags = *((int *) &pBuffer[nLocation]);
    nLocation += 4;
    
    // safety check (so we can't get buffer overflow attacked)
    int nMaximumRead = nMaximumBytes - 8 - nFieldValueSize;
    BOOL bSafe = TRUE;
    for (int z = 0; (z < nMaximumRead) && (bSafe == TRUE); z++)
    {
        int nCharacter = pBuffer[nLocation + z];
        if (nCharacter == 0)
            break;
        if ((nCharacter < 0x20) || (nCharacter > 0x7E))
            bSafe = FALSE;
    }
    if (bSafe == FALSE)
        return -1;

    // name
    int nNameCharacters = strlen(&pBuffer[nLocation]);
    CSmartPtr<str_utf8> spNameUTF8(new str_utf8 [nNameCharacters + 1], TRUE);
    memcpy(spNameUTF8, &pBuffer[nLocation], (nNameCharacters + 1) * sizeof(str_utf8));
    nLocation += nNameCharacters + 1;
    CSmartPtr<str_utf16> spNameUTF16(GetUTF16FromUTF8(spNameUTF8.GetPtr()), TRUE);

    // value
    CSmartPtr<char> spFieldBuffer(new char [nFieldValueSize], TRUE);
    memcpy(spFieldBuffer, &pBuffer[nLocation], nFieldValueSize);
    nLocation += nFieldValueSize;

    // update the bytes
    if (pBytes) *pBytes = nLocation;

    // set
    return SetFieldBinary(spNameUTF16.GetPtr(), spFieldBuffer, nFieldValueSize, nFieldFlags);
}

int CAPETag::SetFieldString(const str_utf16 * pFieldName, const str_utf16 * pFieldValue)
{
    // remove if empty
    if ((pFieldValue == NULL) || (wcslen(pFieldValue) <= 0))
        return RemoveField(pFieldName);

    // UTF-8 encode the value and call the UTF-8 SetField(...)
    CSmartPtr<str_utf8> spFieldValueUTF8(GetUTF8FromUTF16((str_utf16 *) pFieldValue), TRUE);
    return SetFieldString(pFieldName, (const char *) spFieldValueUTF8.GetPtr(), TRUE);
}

int CAPETag::SetFieldString(const str_utf16 * pFieldName, const char * pFieldValue, BOOL bAlreadyUTF8Encoded)
{
    // remove if empty
    if ((pFieldValue == NULL) || (strlen(pFieldValue) <= 0))
        return RemoveField(pFieldName);

    // get the length and call the binary SetField(...)
    if (bAlreadyUTF8Encoded == FALSE)
    {
        CSmartPtr<char> spUTF8((char *) GetUTF8FromANSI(pFieldValue), TRUE);
        int nFieldBytes = strlen(spUTF8.GetPtr());
        return SetFieldBinary(pFieldName, spUTF8.GetPtr(), nFieldBytes, TAG_FIELD_FLAG_DATA_TYPE_TEXT_UTF8);
    }
    else
    {
        int nFieldBytes = strlen(pFieldValue);
        return SetFieldBinary(pFieldName, pFieldValue, nFieldBytes, TAG_FIELD_FLAG_DATA_TYPE_TEXT_UTF8);
    }
}

int CAPETag::SetFieldBinary(const str_utf16 * pFieldName, const void * pFieldValue, int nFieldBytes, int nFieldFlags)
{
    if (m_bAnalyzed == FALSE) { Analyze(); }
    if (pFieldName == NULL) return -1;

    // check to see if we're trying to remove the field (by setting it to NULL or an empty string)
    BOOL bRemoving = (pFieldValue == NULL) || (nFieldBytes <= 0);

    // get the index
    int nFieldIndex = GetTagFieldIndex(pFieldName);
    if (nFieldIndex != -1)
    {
        // existing field

        // fail if we're read-only (and not ignoring the read-only flag)
        if ((m_bIgnoreReadOnly == FALSE) && (m_aryFields[nFieldIndex]->GetIsReadOnly()))
            return -1;
        
        // erase the existing field
        SAFE_DELETE(m_aryFields[nFieldIndex])

        if (bRemoving)
        {
            return RemoveField(nFieldIndex);
        }
    }
    else
    {
        if (bRemoving)
            return ERROR_SUCCESS;

        nFieldIndex = m_nFields;
        m_nFields++;
    }
    
    // create the field and add it to the field array
    m_aryFields[nFieldIndex] = new CAPETagField(pFieldName, pFieldValue, nFieldBytes, nFieldFlags);

    return ERROR_SUCCESS;
}

int CAPETag::RemoveField(int nIndex)
{
    if ((nIndex >= 0) && (nIndex < m_nFields))
    {
        SAFE_DELETE(m_aryFields[nIndex])
        memmove(&m_aryFields[nIndex], &m_aryFields[nIndex + 1], (256 - nIndex - 1) * sizeof(CAPETagField *));
        m_nFields--;
        return ERROR_SUCCESS;
    }

    return -1;
}

int CAPETag::RemoveField(const str_utf16 * pFieldName)
{
    return RemoveField(GetTagFieldIndex(pFieldName));
}

int CAPETag::Remove(BOOL bUpdate)
{
    // variables
    unsigned int nBytesRead = 0;
    int nRetVal = 0;
    int nOriginalPosition = m_spIO->GetPosition();

    BOOL bID3Removed = TRUE;
    BOOL bAPETagRemoved = TRUE;

    BOOL bFailedToRemove = FALSE;

    while (bID3Removed || bAPETagRemoved)
    {
        bID3Removed = FALSE;
        bAPETagRemoved = FALSE;

        // ID3 tag
        if (m_spIO->GetSize() > ID3_TAG_BYTES)
        {
            char cTagHeader[3];
            m_spIO->Seek(-ID3_TAG_BYTES, FILE_END);
            nRetVal = m_spIO->Read(cTagHeader, 3, &nBytesRead);
            if ((nRetVal == 0) && (nBytesRead == 3))
            {
                if (strncmp(cTagHeader, "TAG", 3) == 0)
                {
                    m_spIO->Seek(-ID3_TAG_BYTES, FILE_END);
                    if (m_spIO->SetEOF() != 0)
                        bFailedToRemove = TRUE;
                    else
                        bID3Removed = TRUE;
                }
            }
        }


        // APE Tag
        if (m_spIO->GetSize() > APE_TAG_FOOTER_BYTES && bFailedToRemove == FALSE)
        {
            APE_TAG_FOOTER APETagFooter;
            m_spIO->Seek(-int(APE_TAG_FOOTER_BYTES), FILE_END);
            nRetVal = m_spIO->Read(&APETagFooter, APE_TAG_FOOTER_BYTES, &nBytesRead);
            if ((nRetVal == 0) && (nBytesRead == APE_TAG_FOOTER_BYTES))
            {
                if (APETagFooter.GetIsValid(TRUE))
                {
                    m_spIO->Seek(-APETagFooter.GetTotalTagBytes(), FILE_END);

                    if (m_spIO->SetEOF() != 0)
                        bFailedToRemove = TRUE;
                    else
                        bAPETagRemoved = TRUE;
                }
            }
        }

    }
    
    m_spIO->Seek(nOriginalPosition, FILE_BEGIN);

    if (bUpdate && bFailedToRemove == FALSE)
    {
        Analyze();
    }

    return bFailedToRemove ? -1 : 0;
}

int CAPETag::SetFieldID3String(const str_utf16 * pFieldName, const char * pFieldValue, int nBytes)
{
    // allocate a buffer and terminate it
    CSmartPtr<str_ansi> spBuffer(new str_ansi [nBytes + 1], TRUE);
    spBuffer[nBytes] = 0;
    
    // make a capped copy of the string
    memcpy(spBuffer.GetPtr(), pFieldValue, nBytes);
    
    // remove trailing white-space
    char * pEnd = &spBuffer[nBytes];
    while (((*pEnd == ' ') || (*pEnd == 0)) && pEnd >= &spBuffer[0]) { *pEnd-- = 0; }

    // set the field
    SetFieldString(pFieldName, spBuffer, FALSE);
    
    return ERROR_SUCCESS;
}

int CAPETag::GetFieldID3String(const str_utf16 * pFieldName, char * pBuffer, int nBytes)
{
    int nBufferCharacters = 255; str_utf16 cBuffer[256] = {0};
    GetFieldString(pFieldName, cBuffer, &nBufferCharacters);

    CSmartPtr<str_ansi> spBufferANSI(GetANSIFromUTF16(cBuffer), TRUE);

    memset(pBuffer, 0, nBytes);
    strncpy(pBuffer, spBufferANSI.GetPtr(), nBytes);

    return ERROR_SUCCESS;
}

int CAPETag::SortFields()
{
    // sort the tag fields by size (so that the smallest fields are at the front of the tag)
    qsort(m_aryFields, m_nFields, sizeof(CAPETagField *), CompareFields);

    return ERROR_SUCCESS;
}

int CAPETag::CompareFields(const void * pA, const void * pB)
{
    CAPETagField * pFieldA = *((CAPETagField **) pA);
    CAPETagField * pFieldB = *((CAPETagField **) pB);

    return (pFieldA->GetFieldSize() - pFieldB->GetFieldSize());
}
