/* -----------------------------------------------------------------------------

	Copyright (c) 2006 Simon Brown                          si@sjbrown.co.uk

	Permission is hereby granted, free of charge, to any person obtaining
	a copy of this software and associated documentation files (the 
	"Software"), to	deal in the Software without restriction, including
	without limitation the rights to use, copy, modify, merge, publish,
	distribute, sublicense, and/or sell copies of the Software, and to 
	permit persons to whom the Software is furnished to do so, subject to 
	the following conditions:

	The above copyright notice and this permission notice shall be included
	in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
	OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
	CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
	TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
	SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
	
   -------------------------------------------------------------------------- */
   
/*! @file

	@brief	Test program that compresses images loaded using the PNG format.
	
	This program requires libpng for PNG input and output, and is designed to
	test the RMS error for DXT compression for a set of test images.

	This program uses the high-level image compression and decompression
	functions that process an entire image at a time.
*/

#include <iostream>
#include <string>
#include <sstream>
#include <ctime>
#include <cmath>
#include <squish.h>
#include <png.h>

#ifdef _MSC_VER
#pragma warning( disable: 4511 4512 )
#endif // def _MSC_VER

using namespace squish;

//! Simple exception class.
class Error : public std::exception
{
public:
	Error( std::string const& excuse ) : m_excuse( excuse ) {}
	~Error() throw() {}
	
	virtual char const* what() const throw() { return m_excuse.c_str(); }
	
private:
	std::string m_excuse;
};

//! Base class to make derived classes non-copyable
class NonCopyable
{
public:
	NonCopyable() {}
	
private:
	NonCopyable( NonCopyable const& );
	NonCopyable& operator=( NonCopyable const& );
};

//! Memory object.
class Mem : NonCopyable
{
public:
	Mem() : m_p( 0 ) {}
	explicit Mem( int size ) : m_p( new u8[size] ) {}
	~Mem() { delete[] m_p; }

	void Reset( int size )
	{
		u8 *p = new u8[size];
		delete m_p;
		m_p = p;
	}
	
	u8* Get() const { return m_p; }
	
private:
	u8* m_p;
};

//! File object.
class File : NonCopyable
{
public:
	explicit File( FILE* fp ) : m_fp( fp ) {}
	~File() { if( m_fp ) fclose( m_fp ); }
	
	bool IsValid() const { return m_fp != 0; }
	FILE* Get() const { return m_fp; }

private:
	FILE* m_fp;
};

//! PNG read object.
class PngReadStruct : NonCopyable
{
public:
	PngReadStruct()
	  : m_png( 0 ), 
		m_info( 0 ), 
		m_end( 0 )
	{
		m_png = png_create_read_struct( PNG_LIBPNG_VER_STRING, 0, 0, 0 );
		if( !m_png )
			throw Error( "failed to create png read struct" );	
			
		m_info = png_create_info_struct( m_png );
		m_end = png_create_info_struct( m_png );
		if( !m_info || !m_end )
		{
			png_infopp info = m_info ? &m_info : 0;
			png_infopp end = m_end ? &m_end : 0;
			png_destroy_read_struct( &m_png, info, end );
			throw Error( "failed to create png info structs" );
		}
	}
	
	~PngReadStruct() 
	{ 
		png_destroy_read_struct( &m_png, &m_info, &m_end );
	}

	png_structp GetPng() const { return m_png; }
	png_infop GetInfo() const { return m_info; }

private:
	png_structp m_png;
	png_infop m_info, m_end;
};

//! PNG write object.
class PngWriteStruct : NonCopyable
{
public:
	PngWriteStruct()
	  : m_png( 0 ), 
		m_info( 0 )
	{
		m_png = png_create_write_struct( PNG_LIBPNG_VER_STRING, 0, 0, 0 );
		if( !m_png )
			throw Error( "failed to create png read struct" );	
			
		m_info = png_create_info_struct( m_png );
		if( !m_info )
		{
			png_infopp info = m_info ? &m_info : 0;
			png_destroy_write_struct( &m_png, info );
			throw Error( "failed to create png info structs" );
		}
	}
	
	~PngWriteStruct()
	{
		png_destroy_write_struct( &m_png, &m_info );
	}
	
	png_structp GetPng() const { return m_png; }
	png_infop GetInfo() const { return m_info; }

private:
	png_structp m_png;
	png_infop m_info;
};

//! PNG rows object.
class PngRows : NonCopyable
{
public:
	PngRows( int pitch, int height ) : m_height( height )
	{
		m_rows = new png_bytep[m_height];
		for( int i = 0; i < m_height; ++i )
			m_rows[i] = new png_byte[pitch];
	}
	
	~PngRows() 
	{
		for( int i = 0; i < m_height; ++i )
			delete[] m_rows[i];
		delete[] m_rows;
	}
	
	png_bytep* Get() const { return m_rows; }

	png_bytep operator[](int y) const { return m_rows[y]; }
	
private:
	png_bytep* m_rows;
	int m_height;
};

//! Represents a DXT compressed image in memory.
struct DxtData
{
	int width;
	int height;
	int format;		//!< Either kDxt1, kDxt3 or kDxt5.
	Mem data;
	bool isColour;
	bool isAlpha;
};

//! Represents an uncompressed RGBA image in memory.
class Image
{
public:
	Image();

	void LoadPng( std::string const& fileName );
	void SavePng( std::string const& fileName ) const;

	void Decompress( DxtData const& dxt );
	void Compress( DxtData& dxt, int flags ) const;

	double GetRmsError( Image const& image ) const;

private:
	int m_width;
	int m_height;
	bool m_isColour;	//!< Either colour or luminance.
	bool m_isAlpha;		//!< Either alpha or not.
	Mem m_pixels;
};

Image::Image() 
  : m_width( 0 ), 
  	m_height( 0 ),
	m_isColour( false ),
	m_isAlpha( false )
{
}

void Image::LoadPng( std::string const& fileName )
{
	// open the source file
	File file( fopen( fileName.c_str(), "rb" ) );
	if( !file.IsValid() )
	{
		std::ostringstream oss;
		oss << "failed to open \"" << fileName << "\" for reading";
		throw Error( oss.str() );
	}
	
	// check the signature bytes
	png_byte header[8];
	size_t check = fread( header, 1, 8, file.Get() );
	if( check != 8 )
		throw Error( "file read error" );
	if( png_sig_cmp( header, 0, 8 ) )
	{
		std::ostringstream oss;
		oss << "\"" << fileName << "\" does not look like a png file";
		throw Error( oss.str() );
	}
	
	// read the image into memory
	PngReadStruct png;
	png_init_io( png.GetPng(), file.Get() );
	png_set_sig_bytes( png.GetPng(), 8 );
	png_read_png( png.GetPng(), png.GetInfo(), PNG_TRANSFORM_EXPAND, 0 );

	// get the image info
	png_uint_32 width;
	png_uint_32 height;
	int bitDepth;
	int colourType;
	png_get_IHDR( png.GetPng(), png.GetInfo(), &width, &height, &bitDepth, &colourType, 0, 0, 0 );
	
	// check the image is 8 bit
	if( bitDepth != 8 )
	{
		std::ostringstream oss;
		oss << "cannot process " << bitDepth << "-bit image (bit depth must be 8)";
		throw Error( oss.str() );
	}

	// copy the data into a contiguous array
	m_width = width;
	m_height = height;
	m_isColour = ( ( colourType & PNG_COLOR_MASK_COLOR ) != 0 );
	m_isAlpha = ( ( colourType & PNG_COLOR_MASK_ALPHA ) != 0 );
	m_pixels.Reset(4*width*height);

	// get the image rows
	png_bytep const *rows = png_get_rows( png.GetPng(), png.GetInfo() );
	if( !rows )
		throw Error( "failed to get image rows" );

	// copy the pixels into the storage
	u8 *dest = m_pixels.Get();
	for( int y = 0; y < m_height; ++y )
	{
		u8 const *src = rows[y];
		for( int x = 0; x < m_width; ++x )
		{
			if( m_isColour )
			{
				dest[0] = src[0];
				dest[1] = src[1];
				dest[2] = src[2];
				src += 3;
			}
			else
			{
				u8 lum = *src++;
				dest[0] = lum;
				dest[1] = lum;
				dest[2] = lum;
			}
			
			if( m_isAlpha )
				dest[3] = *src++;
			else
				dest[3] = 255;

			dest += 4;
		}
	}
}

void Image::SavePng( std::string const& fileName ) const
{
	// create the target rows
	int const pixelSize = ( m_isColour ? 3 : 1 ) + ( m_isAlpha ? 1 : 0 );
	PngRows rows( m_width*pixelSize, m_height );

	// fill the rows with pixel data
	u8 const *src = m_pixels.Get();
	for( int y = 0; y < m_height; ++y )
	{
		u8 *dest = rows[y];
		for( int x = 0; x < m_width; ++x )
		{
			if( m_isColour )
			{
				dest[0] = src[0];
				dest[1] = src[1];
				dest[2] = src[2];
				dest += 3;
			}
			else
				*dest++ = src[1];
			
			if( m_isAlpha )
				*dest++ = src[3];

			src += 4;
		}
	}

	// set up the image
	PngWriteStruct png;
	png_set_IHDR(
		png.GetPng(), png.GetInfo(), m_width, m_height,
		8, ( m_isColour ? PNG_COLOR_MASK_COLOR : 0) | ( m_isAlpha ? PNG_COLOR_MASK_ALPHA : 0 ), 
		PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT 
	);
	   
	// open the target file
	File file( fopen( fileName.c_str(), "wb" ) );
	if( !file.IsValid() )
	{
		std::ostringstream oss;
		oss << "failed to open \"" << fileName << "\" for writing";
		throw Error( oss.str() );
	}
	
	// write the image
	png_set_rows( png.GetPng(), png.GetInfo(), rows.Get() );
	png_init_io( png.GetPng(), file.Get() );
	png_write_png( png.GetPng(), png.GetInfo(), PNG_TRANSFORM_IDENTITY, 0 );
}

void Image::Decompress( DxtData const& dxt )
{
	// allocate storage
	m_width = dxt.width;
	m_height = dxt.height;
	m_isColour = dxt.isColour;
	m_isAlpha = dxt.isAlpha;
	m_pixels.Reset( 4*m_width*m_height );

	// use the whole image decompression function to do the work
	DecompressImage( m_pixels.Get(), m_width, m_height, dxt.data.Get(), dxt.format );
}

void Image::Compress( DxtData& dxt, int flags ) const
{
	// work out how much memory we need
	int storageSize = GetStorageRequirements( m_width, m_height, flags );

	// set the structure fields and allocate it
	dxt.width = m_width;
	dxt.height = m_height;
	dxt.format = flags & ( kDxt1 | kDxt3 | kDxt5 );
	dxt.isColour = m_isColour;
	dxt.isAlpha = m_isAlpha;
	dxt.data.Reset( storageSize );

	// use the whole image compression function to do the work
	CompressImage( m_pixels.Get(), m_width, m_height, dxt.data.Get(), flags );
}

double Image::GetRmsError( Image const& image ) const
{
	if( m_width != image.m_width || m_height != image.m_height )
		throw Error( "image dimensions mismatch when computing RMS error" );

	// accumulate colour error
	double difference = 0;
	u8 const *a = m_pixels.Get();
	u8 const *b = image.m_pixels.Get();
	for( int y = 0; y < m_height; ++y )
	{
		for( int x = 0; x < m_width; ++x )
		{
			int d0 = ( int )a[0] - ( int )b[0];
			int d1 = ( int )a[1] - ( int )b[1];
			int d2 = ( int )a[2] - ( int )b[2];
			difference += ( double )( d0*d0 + d1*d1 + d2*d2 ); 
			a += 4;
			b += 4;
		}
	}
	return std::sqrt( difference/( double )( m_width*m_height ) );
}

int main( int argc, char* argv[] )
{
	try
	{
		// parse the command-line
		std::string sourceFileName;
		std::string targetFileName;
		int format = kDxt1;
		int fit = kColourClusterFit;
		int extra = 0;
		bool help = false;
		bool arguments = true;
		bool error = false;
		for( int i = 1; i < argc; ++i )
		{
			// check for options
			char const* word = argv[i];
			if( arguments && word[0] == '-' )
			{
				for( int j = 1; word[j] != '\0'; ++j )
				{
					switch( word[j] )
					{
					case 'h': help = true; break;
					case '1': format = kDxt1; break;
					case '3': format = kDxt3; break;
					case '5': format = kDxt5; break;
					case 'r': fit = kColourRangeFit; break;
					case 'i': fit = kColourIterativeClusterFit; break;
					case 'w': extra = kWeightColourByAlpha; break;
					case '-': arguments = false; break;
					default:
						std::cerr << "squishpng error: unknown option '" << word[j] << "'" << std::endl;
						error = true;
					}
				}
			}
			else
			{
				if( sourceFileName.empty() )
					sourceFileName.assign( word );
				else if( targetFileName.empty() )
					targetFileName.assign( word );
				else
				{
					std::cerr << "squishpng error: unexpected argument \"" << word << "\"" << std::endl;
					error = true;
				}
			}
		}
		
		// check arguments
		if( sourceFileName.empty() )
		{
			std::cerr << "squishpng error: no source file given" << std::endl;
			error = true;
		}
		if( help || error )
		{
			std::cout 
				<< "SYNTAX" << std::endl
				<< "\tsquishpng [-135riw] <source> [<target>]" << std::endl 
				<< "OPTIONS" << std::endl
				<< "\t-h\tPrint this help message" << std::endl
				<< "\t-135\tSpecifies whether to use DXT1 (default), DXT3 or DXT5 compression" << std::endl
				<< "\t-r\tUse the fast but inferior range-based colour compressor" << std::endl
				<< "\t-i\tUse the very slow but slightly better iterative colour compressor" << std::endl
				<< "\t-w\tWeight colour values by alpha in the cluster colour compressor" << std::endl
				;
			
			return error ? -1 : 0;
		}

		// load the source image
		Image sourceImage;
		sourceImage.LoadPng( sourceFileName );

		// compress to DXT
		DxtData dxt;
		sourceImage.Compress( dxt, format | fit | extra );

		// decompress back
		Image targetImage;
		targetImage.Decompress( dxt );

		// compare the images
		double rmsError = sourceImage.GetRmsError( targetImage );
		std::cout << sourceFileName << " " << rmsError << std::endl;

		// save the target image if necessary
		if( !targetFileName.empty() )
			targetImage.SavePng( targetFileName );
	}
	catch( std::exception& excuse )
	{
		// complain
		std::cerr << "squishpng error: " << excuse.what() << std::endl;
		return -1;
	}
	
	// done
	return 0;
}
