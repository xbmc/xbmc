
#include "stdafx.h"
#include "oggtag.h"

using namespace MUSIC_INFO;

#define CHUNK_SIZE 8192		// should suffice for most tags

COggTag::COggTag()
{
	m_nTrackNum=0;
	m_nDuration=0;
	m_nSamplesPerSec=0;
	m_nChannels=0;
	m_nBitrate=0;
	m_nSamples=0;
}

COggTag::~COggTag()
{
}

bool COggTag::ReadTag( CFile* file )
{
	m_file = file;

    char* const pBuffer=new char[CHUNK_SIZE+100]; //+100 for later...
    unsigned char* const pBufferU=(unsigned char*)pBuffer;	//unsigned char pointer to data
	m_file->Read((void *)pBuffer, CHUNK_SIZE);

    // Check we've got an ogg file
    if (pBuffer[0]!='O' || pBuffer[1]!='g' || pBuffer[2]!='g' || pBuffer[3]!='S')
		{
				delete [] pBuffer;
        return false;
		}

    int iNext=4; //Next page of data's offset
    int iOffset=0; //iOffset in file

    while (iOffset+iNext<CHUNK_SIZE)
    {
		// find the next chunk of data
        iNext=4;
        while ((pBuffer[iOffset+iNext]!='O' || pBuffer[iOffset+iNext+1]!='g' || pBuffer[iOffset+iNext+2]!='g' || pBuffer[iOffset+iNext+3]!='S') && iOffset+iNext<CHUNK_SIZE)
            iNext++;
        if (iOffset+iNext<CHUNK_SIZE)
        {
            int iStart=iOffset+28+pBuffer[iOffset+26]; //Start of header
            int Id=pBuffer[iStart-1];					// Id of header
            if (Id==1)		// Vorbis header field
            {
                if (pBuffer[iOffset+29]=='v' && pBuffer[iOffset+30]=='o') // vorbis audio header
                {
					m_nChannels = (int)*(pBufferU+iOffset+39);	//LittleEndian2int8u;
					m_nSamplesPerSec = *(int *)(pBufferU+iOffset+40);//LittleEndian2int64u;
					m_nBitrate = *(int*)(pBufferU+iOffset+48);	//LittleEndian2int64s
               }
            }
            else if (Id==3)	// Vorbis Comment field
            {
                //vendorlength, vendor, number of comments, comment length, comment
				ProcessVorbisComment(pBuffer+iStart+6);
            }
            iOffset+=iNext;
            iNext=0;
        }
    }

	// Find the last data packet
    iOffset=-1;
    int AA=0;
    while (iOffset==-1 || AA>128)
    {
		AA++;
		m_file->Seek(-CHUNK_SIZE*AA, SEEK_END);//fseek(fb, -CHUNK_SIZE*AA, SEEK_END);
		m_file->Read((void *)pBuffer, CHUNK_SIZE+100); //+100 for possible overlaps in the data pages

		iOffset=CHUNK_SIZE+100-1;
		while ((pBuffer[iOffset]!='O' || pBuffer[iOffset+1]!='g' || pBuffer[iOffset+2]!='g' || pBuffer[iOffset+3]!='S') && iOffset>=0)
			iOffset--;
	}
	// OK, grab the granule position (this is the position in samples)
    if (iOffset>=0)
    {
        m_nSamples=*(int *)(pBufferU+iOffset+6); //Granule Pos
		m_nDuration = (int)((m_nSamples*75)/m_nSamplesPerSec);	// *75 for frames
    }

	delete [] pBuffer;
	return true;
}

void COggTag::ProcessVorbisComment(const char *pBuffer)
{
    int Pos=0;						// position in the buffer
    int *I1=(int*)(pBuffer+Pos);	// length of vendor string
    Pos+=I1[0]+4;					// just pass the vendor string
    I1=(int*)(pBuffer+Pos);			// number of comments
    int Count=I1[0];
    Pos+=4;				// Start of the first comment
    char C1[CHUNK_SIZE];
    for (int I2=0; I2<Count; I2++) // Run through the comments
    {
        I1=(int*)(pBuffer+Pos);			// Length of comment
        strncpy(C1, pBuffer+Pos+4, I1[0]);
        C1[I1[0]]='\0';
		CStdString strItem;
		g_charsetConverter.utf8ToStringCharset(C1, strItem);		// convert UTF-8 to charset string
		// Parse the tag entry
		parseTagEntry( strItem );
        // Increment our position in the file buffer
        Pos+=I1[0]+4;
    }
}

int COggTag::parseTagEntry(CStdString& strTagEntry)
{
	CStdString strTagValue;
	CStdString strTagType;

	//	Split tag entry like ARTIST=Sublime
	SplitEntry( strTagEntry, strTagType, strTagValue);

	//	Save tag entry to members

	if ( strTagType == "artist" ) {
		if (m_strArtist.length())
			m_strArtist += " / " + strTagValue;
		else
			m_strArtist = strTagValue;
	}

	if ( strTagType == "title" ) {
		m_strTitle = strTagValue;
	}

	if ( strTagType == "album" ) {
		m_strAlbum = strTagValue;
	}

	if ( strTagType == "tracknumber" ) {
		m_nTrackNum = atoi( strTagValue );
	}

	if ( strTagType == "date" ) {
		m_strYear = strTagValue;
	}

	if ( strTagType == "genre" ) {
		if (m_strGenre.length())
			m_strGenre += " / " + strTagValue;
		else
			m_strGenre = strTagValue;
	}

	return 0;
}

void COggTag::SplitEntry(const CStdString& strTagEntry, CStdString& strTagType, CStdString& strTagValue)
{
	int nPos = strTagEntry.Find( '=' );

	if ( nPos > -1 ) {
		strTagValue = strTagEntry.Mid( nPos + 1 );
		strTagType = strTagEntry.Left( nPos );
		strTagType.ToLower();
	}
}
