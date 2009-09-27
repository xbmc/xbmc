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

	@brief	Example program that converts between the PNG and DXT formats.
	
	This program requires libpng for PNG input and output, and is designed
	to show how to prepare data for the squish library when it is not simply
	a contiguous block of memory.
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
	explicit Mem( int size ) : m_p( new u8[size] ) {}
	~Mem() { delete[] m_p; }
	
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
	PngRows( int width, int height, int stride ) : m_width( width ), m_height( height )
	{
		m_rows = ( png_bytep* )malloc( m_height*sizeof( png_bytep ) );
		for( int i = 0; i < m_height; ++i )
			m_rows[i] = ( png_bytep )malloc( m_width*stride );
	}
	
	~PngRows() 
	{
		for( int i = 0; i < m_height; ++i )
			free( m_rows[i] );
		free( m_rows );
	}
	
	png_bytep* Get() const { return m_rows; }
	
private:
	png_bytep* m_rows;
	int m_width, m_height;
};

class PngImage
{
public:
	explicit PngImage( std::string const& fileName );

	int GetWidth() const { return m_width; }
	int GetHeight() const { return m_height; }
	int GetStride() const { return m_stride; }
	bool IsColour() const { return m_colour; }
	bool IsAlpha() const { return m_alpha; }
	
	u8 const* GetRow( int row ) const { return ( u8* )m_rows[row]; }

private:
	PngReadStruct m_png;

	int m_width;
	int m_height;
	int m_stride;
	bool m_colour;
	bool m_alpha;
	
	png_bytep* m_rows;
};

PngImage::PngImage( std::string const& fileName )
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
	fread( header, 1, 8, file.Get() );
	if( png_sig_cmp( header, 0, 8 ) )
	{
		std::ostringstream oss;
		oss << "\"" << fileName << "\" does not look like a png file";
		throw Error( oss.str() );
	}
	
	// read the image into memory
	png_init_io( m_png.GetPng(), file.Get() );
	png_set_sig_bytes( m_png.GetPng(), 8 );
	png_read_png( m_png.GetPng(), m_png.GetInfo(), PNG_TRANSFORM_EXPAND, 0 );

	// get the image info
	png_uint_32 width;
	png_uint_32 height;
	int bitDepth;
	int colourType;
	png_get_IHDR( m_png.GetPng(), m_png.GetInfo(), &width, &height, &bitDepth, &colourType, 0, 0, 0 );
	
	// check the image is 8 bit
	if( bitDepth != 8 )
	{
		std::ostringstream oss;
		oss << "cannot process " << bitDepth << "-bit image (bit depth must be 8)";
		throw Error( oss.str() );
	}
	
	// save the info
	m_width = width;
	m_height = height;
	m_colour = ( ( colourType & PNG_COLOR_MASK_COLOR ) != 0 );
	m_alpha = ( ( colourType & PNG_COLOR_MASK_ALPHA ) != 0 );
	m_stride = ( m_colour ? 3 : 1 ) + ( m_alpha ? 1 : 0 );

	// get the image rows
	m_rows = png_get_rows( m_png.GetPng(), m_png.GetInfo() );
	if( !m_rows )
		throw Error( "failed to get image rows" );
}

static void Compress( std::string const& sourceFileName, std::string const& targetFileName, int flags )
{
	// load the source image
	PngImage sourceImage( sourceFileName );

	// get the image info
	int width = sourceImage.GetWidth();
	int height = sourceImage.GetHeight();
	int stride = sourceImage.GetStride();
	bool colour = sourceImage.IsColour();
	bool alpha = sourceImage.IsAlpha();

	// check the image dimensions
	if( ( width % 4 ) != 0 || ( height % 4 ) != 0 )
	{
		std::ostringstream oss;
		oss << "cannot compress " << width << "x" << height
			<< "image (dimensions must be multiples of 4)";
		throw Error( oss.str() );
	}
	
	// create the target data
	int bytesPerBlock = ( ( flags & kDxt1 ) != 0 ) ? 8 : 16;
	int targetDataSize = bytesPerBlock*width*height/16;
	Mem targetData( targetDataSize );
	
	// loop over blocks and compress them
	clock_t start = std::clock();
	u8* targetBlock = targetData.Get();
	for( int y = 0; y < height; y += 4 )
	{
		// process a row of blocks
		for( int x = 0; x < width; x += 4 )
		{
			// get the block data
			u8 sourceRgba[16*4];
			for( int py = 0, i = 0; py < 4; ++py )
			{
				u8 const* row = sourceImage.GetRow( y + py ) + x*stride;
				for( int px = 0; px < 4; ++px, ++i )
				{
					// get the pixel colour 
					if( colour )
					{
						for( int j = 0; j < 3; ++j )
							sourceRgba[4*i + j] = *row++;
					}
					else
					{
						for( int j = 0; j < 3; ++j )
							sourceRgba[4*i + j] = *row;
						++row;
					}
					
					// skip alpha for now
					if( alpha )
						sourceRgba[4*i + 3] = *row++;
					else
						sourceRgba[4*i + 3] = 255;
				}
			}
			
			// compress this block
			Compress( sourceRgba, targetBlock, flags );
			
			// advance
			targetBlock += bytesPerBlock;			
		}
	}
	clock_t end = std::clock();
	double duration = ( double )( end - start ) / CLOCKS_PER_SEC;
	std::cout << "time taken: " << duration << " seconds" << std::endl;
	
	// open the target file
	File targetFile( fopen( targetFileName.c_str(), "wb" ) );
	if( !targetFile.IsValid() )
	{
		std::ostringstream oss;
		oss << "failed to open \"" << sourceFileName << "\" for writing";
		throw Error( oss.str() );
	}
	
	// write the header
	fwrite( &width, sizeof( int ), 1, targetFile.Get() );
	fwrite( &height, sizeof( int ), 1, targetFile.Get() );
	
	// write the data
	fwrite( targetData.Get(), 1, targetDataSize, targetFile.Get() );
}

static void Decompress( std::string const& sourceFileName, std::string const& targetFileName, int flags )
{
	// open the source file
	File sourceFile( fopen( sourceFileName.c_str(), "rb" ) );
	if( !sourceFile.IsValid() )
	{
		std::ostringstream oss;
		oss << "failed to open \"" << sourceFileName << "\" for reading";
		throw Error( oss.str() );
	}
	
	// get the width and height
	int width, height;
	fread( &width, sizeof( int ), 1, sourceFile.Get() ); 
	fread( &height, sizeof( int ), 1, sourceFile.Get() );
	
	// work out the data size
	int bytesPerBlock = ( ( flags & kDxt1 ) != 0 ) ? 8 : 16;
	int sourceDataSize = bytesPerBlock*width*height/16;
	Mem sourceData( sourceDataSize );
	
	// read the source data
	fread( sourceData.Get(), 1, sourceDataSize, sourceFile.Get() );
		
	// create the target rows
	PngRows targetRows( width, height, 4 );
	
	// loop over blocks and compress them
	u8 const* sourceBlock = sourceData.Get();
	for( int y = 0; y < height; y += 4 )
	{
		// process a row of blocks
		for( int x = 0; x < width; x += 4 )
		{
			// decompress back
			u8 targetRgba[16*4];
			Decompress( targetRgba, sourceBlock, flags );
			
			// write the data into the target rows
			for( int py = 0, i = 0; py < 4; ++py )
			{
				u8* row = ( u8* )targetRows.Get()[y + py] + x*4;
				for( int px = 0; px < 4; ++px, ++i )
				{	
					for( int j = 0; j < 4; ++j )
						*row++ = targetRgba[4*i + j];
				}
			}
			
			// advance
			sourceBlock += bytesPerBlock;
		}
	}
	
	// create the target PNG
	PngWriteStruct targetPng;

	// set up the image
	png_set_IHDR(
		targetPng.GetPng(), targetPng.GetInfo(), width, height,
		8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT 
	);
	   
	// open the target file
	File targetFile( fopen( targetFileName.c_str(), "wb" ) );
	if( !targetFile.IsValid() )
	{
		std::ostringstream oss;
		oss << "failed to open \"" << targetFileName << "\" for writing";
		throw Error( oss.str() );
	}
	
	// write the image
	png_set_rows( targetPng.GetPng(), targetPng.GetInfo(), targetRows.Get() );
	png_init_io( targetPng.GetPng(), targetFile.Get() );
	png_write_png( targetPng.GetPng(), targetPng.GetInfo(), PNG_TRANSFORM_IDENTITY, 0 );
}

static void Diff( std::string const& sourceFileName, std::string const& targetFileName )
{
	// load the images
	PngImage sourceImage( sourceFileName );
	PngImage targetImage( targetFileName );
	
	// get the image info
	int width = sourceImage.GetWidth();
	int height = sourceImage.GetHeight();
	int sourceStride = sourceImage.GetStride();
	int targetStride = targetImage.GetStride();
	int stride = std::min( sourceStride, targetStride );

	// check they match
	if( width != targetImage.GetWidth() || height != targetImage.GetHeight() )
		throw Error( "source and target dimensions do not match" );
		
	// work out the error
	double error = 0.0;
	for( int y = 0; y < height; ++y )
	{
		u8 const* sourceRow = sourceImage.GetRow( y );
		u8 const* targetRow = targetImage.GetRow( y );
		for( int x = 0; x < width; ++x )
		{	
			u8 const* sourcePixel = sourceRow + x*sourceStride;
			u8 const* targetPixel = targetRow + x*targetStride;
			for( int i = 0; i < stride; ++i )
			{
				int diff = ( int )sourcePixel[i] - ( int )targetPixel[i];
				error += ( double )( diff*diff );
			}
		}
	}
	error = std::sqrt( error / ( width*height ) );
	
	// print it out
	std::cout << "rms error: " << error << std::endl;
}

enum Mode
{
	kCompress, 
	kDecompress,
	kDiff
};

int main( int argc, char* argv[] )
{
	try
	{
		// parse the command-line
		std::string sourceFileName;
		std::string targetFileName;
		Mode mode = kCompress;
		int method = kDxt1;
		int metric = kColourMetricPerceptual;
		int fit = kColourClusterFit;
		int extra = 0;
		bool help = false;
		bool arguments = true;
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
					case 'c': mode = kCompress; break;
					case 'd': mode = kDecompress; break;
					case 'e': mode = kDiff; break;
					case '1': method = kDxt1; break;
					case '3': method = kDxt3; break;
					case '5': method = kDxt5; break;
					case 'u': metric = kColourMetricUniform; break;
					case 'r': fit = kColourRangeFit; break;
					case 'i': fit = kColourIterativeClusterFit; break;
					case 'w': extra = kWeightColourByAlpha; break;
					case '-': arguments = false; break;
					default:
						std::cerr << "unknown option '" << word[j] << "'" << std::endl;
						return -1;
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
					std::cerr << "unexpected argument \"" << word << "\"" << std::endl;
				}
			}
		}
		
		// check arguments
		if( help )
		{
			std::cout 
				<< "SYNTAX" << std::endl
				<< "\tsquishpng [-cde135] <source> <target>" << std::endl 
				<< "OPTIONS" << std::endl
				<< "\t-c\tCompress source png to target raw dxt (default)" << std::endl
				<< "\t-135\tSpecifies whether to use DXT1 (default), DXT3 or DXT5 compression" << std::endl
				<< "\t-u\tUse a uniform colour metric during colour compression" << std::endl
				<< "\t-r\tUse the fast but inferior range-based colour compressor" << std::endl
				<< "\t-i\tUse the very slow but slightly better iterative colour compressor" << std::endl
				<< "\t-w\tWeight colour values by alpha in the cluster colour compressor" << std::endl
				<< "\t-d\tDecompress source raw dxt to target png" << std::endl
				<< "\t-e\tDiff source and target png" << std::endl
				;
			
			return 0;
		}
		if( sourceFileName.empty() )
		{
			std::cerr << "no source file given" << std::endl;
			return -1;
		}
		if( targetFileName.empty() )
		{
			std::cerr << "no target file given" << std::endl;
			return -1;
		}

		// do the work
		switch( mode )
		{
		case kCompress:
			Compress( sourceFileName, targetFileName, method | metric | fit | extra );
			break;
		
		case kDecompress:
			Decompress( sourceFileName, targetFileName, method );
			break;
			
		case kDiff:
			Diff( sourceFileName, targetFileName );
			break;
			
		default:
			std::cerr << "unknown mode" << std::endl;
			throw std::exception();
		}
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
