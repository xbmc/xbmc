#include "xtl.h"
#include "oggtag.h"

using namespace MUSIC_INFO;

COggTag::COggTag()
{
}

COggTag::~COggTag()
{
}

bool COggTag::ReadTag( CFile* file )
{
	m_file = file;

	//	Do we have a vorbis tag header?
	if ( !FindVobisTagHeader( ) ) {
		return false;
	}

	//	Read ventor string 
	char* vendor;
	UINT vendor_lenght = ReadLength();
	if ( vendor_lenght == 0 )
		return false;

	vendor = ReadString( vendor_lenght );
	//	Vendor can be discarded
	delete[] vendor;

	//	Read taglist lenght
	UINT user_comment_list_lenght = ReadLength();
	if ( user_comment_list_lenght == 0 )
		return false;

	//	Read tags like Artist etc.
	UINT j = 0;
	while ( j < user_comment_list_lenght ) {
		char* item;
		UINT length = ReadLength();
		if ( length == 0 )
			return false;
		item = ReadString( length );
		CStdString strItem = item;
		parseTagEntry( strItem );
		delete[] item;
		j++;
	}

	//	Read Framebit
	bool bFrameBit = ReadBit();

	if ( bFrameBit == false )
		return false;

	return true;
}

UINT COggTag::ReadLength(void)
{
	UINT nLenght;
	char buffer[4];
	m_file->Read( (void*) buffer, 4 );

	//	Convert 4-Byte as char to UINT
	nLenght = (UINT) buffer[0];
	nLenght += (UINT) buffer[1]*100;
	nLenght += (UINT) buffer[2]*10000;
	nLenght += (UINT) buffer[3]*1000000;

	if ( nLenght > 0 && nLenght <= UINT_MAX )
		return nLenght;

	return 0;
}

char* COggTag::ReadString( int nLenght )
{
	char* buffer = new char[nLenght+1];
	ZeroMemory( buffer, sizeof(char)*nLenght+1);
	m_file->Read( (void*) buffer, nLenght );
	return buffer;
}

bool COggTag::ReadBit(void)
{
	char buffer[2];
	m_file->Read( (void*) buffer, 1 );
	return (buffer[0] > 0 ? true : false);
}

bool COggTag::FindVobisTagHeader(void)
{
	bool bFound = false;
	char tag[1024];
	m_file->Read( (void*) tag, 1023 );

	//	Find vorbis header type 3 (tags)
	int i = 0;
	while ( i < 1023 ) {
		if ( tag[i]== 'v' )
			if ( tag[i+1] == 'o' && tag[i+2] == 'r' && tag[i+3] == 'b' && tag[i+4] == 'i' && tag[i+5] == 's' ) {
				if ( tag[i-1] == 3 ) {
					bFound = true;
					break;
				}
			}
			i++;
	}

	//	Set filepointer position
	//	to the tag header after the vorbis-string
	if ( bFound )
		m_file->Seek( i+6, SEEK_SET );

	return bFound;
}

int COggTag::parseTagEntry(CStdString& strTagEntry)
{
	CStdString strTagValue;
	CStdString strTagType;

	//	Split tag entry like ARTIST=Sublime
	SplitEntry( strTagEntry, strTagType, strTagValue);

	//	Save tag entry to members

	if ( strTagType == "artist" ) {
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
