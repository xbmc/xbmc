#include "../stdafx.h"
#include "xbeheader.h"
#include "../autoptrhandle.h"
using namespace AUTOPTR;

CXBE::CXBE()
{
  // Assume 256K is enough for header information
  m_iHeaderSize = ( 256 * 1024 );
  m_pHeader = new char[ m_iHeaderSize ] ; 

  m_iImageSize = ( 256 * 1024 );
  m_pImage = new char[m_iImageSize];

}
CXBE::~CXBE()
{
  if (m_pHeader) delete [] m_pHeader;

  m_pHeader=NULL;
  if (m_pImage) delete [] m_pImage;
  m_pImage=NULL;
}

bool CXBE::ExtractIcon(const CStdString& strFilename, const CStdString& strIcon)
{
  // Open the local file
  CAutoPtrHandle hFile( CreateFile( strFilename.c_str(),
                      GENERIC_READ,
                      0,
                      NULL,
                      OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL,
                      NULL ) );

  if (!hFile.isValid() ) return false;

  // Read the header
  DWORD dwRead;
  if ( !::ReadFile( (HANDLE)hFile,
                        m_pHeader,
                        min( m_iHeaderSize,
                        ( int )GetFileSize( (HANDLE)hFile, ( LPDWORD )NULL ) ),
                        &dwRead,
                        NULL ) )
  {
    return false;
  }

  // Header read. Copy information about header
  memcpy( &m_XBEInfo.Header, m_pHeader, sizeof( m_XBEInfo.Header ) );
  
  // Header read. Copy information about certificate
  int    iNumSections;
  char * pszSectionName;

  // Assume 256K is enough for image information

  // Initialize
  iNumSections = m_XBEInfo.Header.sections;
 // Position at the first section
  m_XBEInfo.pSection_Header = ( _xbe_info_::section_header * )( m_pHeader + ( m_XBEInfo.Header.section_headers_addr -
                                                                              m_XBEInfo.Header.base ) );

  // Sections read. Parse all the section headers
  while ( iNumSections-- > ( int )0 )
  {
    // Is this section an inserted file?
    if ( ( int )m_XBEInfo.pSection_Header->Flags.inserted_file == ( int )1 )
    {
      // Inserted file. Position at the name of the section
      pszSectionName = ( m_pHeader + ( m_XBEInfo.pSection_Header->section_name_addr -
                                        m_XBEInfo.Header.base ) );

      // Title image?
      if ( strcmp( pszSectionName, "$$XTIMAGE" ) == ( int )0 )
      {
        // Position at 'title image' in the XBE itself
        if ( SetFilePointer( (HANDLE)hFile,
                              m_XBEInfo.pSection_Header->raw_addr,
                              NULL,
                              FILE_BEGIN ) == ( DWORD )0xFFFFFFFF )
        {
            // Unable to set file position. Break out of loop
            return false;
        }

        // Read the image
        if ( ::ReadFile( (HANDLE)hFile,
                          m_pImage,
                          min( m_iImageSize,
                          ( int )m_XBEInfo.pSection_Header->sizeof_raw ),
                          &dwRead,
                          NULL ) == ( BOOL )FALSE )
        {
          // Image not read. Break out of loop
          return false;
        }

        // The m_pImage variable now contains the image data itself
        CAutoPtrHandle hIcon ( CreateFile( strIcon.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL ) );
        if (!hIcon.isValid() ) return false;
        
				DWORD dwWrote;
				WriteFile( (HANDLE)hIcon,m_pImage,dwRead,&dwWrote,NULL);
        return true;
      }
    }

    // Position at the next section
    m_XBEInfo.pSection_Header++;
  }
  return false;
}
