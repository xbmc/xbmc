/*
 * File:	ximajpg.cpp
 * Purpose:	Platform Independent JPEG Image Class Loader and Writer
 * 07/Aug/2001 Davide Pizzolato - www.xdp.it
 * CxImage version 6.0.0 02/Feb/2008
 */
 
#include "ximajpg.h"

#if CXIMAGE_SUPPORT_JPG

#ifdef _LINUX
#include <jmorecfg.h>
#else
#include "../jpeg/jmorecfg.h"
#endif

#include "ximaiter.h"
         
#include <setjmp.h>

struct jpg_error_mgr {
	struct jpeg_error_mgr pub;	/* "public" fields */
	jmp_buf setjmp_buffer;		/* for return to caller */
	char* buffer;				/* error message <CSC>*/
};
typedef jpg_error_mgr *jpg_error_ptr;

////////////////////////////////////////////////////////////////////////////////
// Here's the routine that will replace the standard error_exit method:
////////////////////////////////////////////////////////////////////////////////
static void
ima_jpeg_error_exit (j_common_ptr cinfo)
{
	/* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
	jpg_error_ptr myerr = (jpg_error_ptr) cinfo->err;
	/* Create the message */
	myerr->pub.format_message (cinfo, myerr->buffer);
	/* Send it to stderr, adding a newline */
	/* Return control to the setjmp point */
	longjmp(myerr->setjmp_buffer, 1);
}
////////////////////////////////////////////////////////////////////////////////
CxImageJPG::CxImageJPG(): CxImage(CXIMAGE_FORMAT_JPG)
{
#if CXIMAGEJPG_SUPPORT_EXIF
	m_exif = NULL;
	memset(&m_exifinfo, 0, sizeof(EXIFINFO));
#endif
}
////////////////////////////////////////////////////////////////////////////////
CxImageJPG::~CxImageJPG()
{
#if CXIMAGEJPG_SUPPORT_EXIF
	if (m_exif) delete m_exif;
#endif
}
////////////////////////////////////////////////////////////////////////////////
#if CXIMAGEJPG_SUPPORT_EXIF
bool CxImageJPG::DecodeExif(CxFile * hFile)
{
	m_exif = new CxExifInfo(&m_exifinfo);
	if (m_exif){
		long pos=hFile->Tell();
		m_exif->DecodeExif(hFile);
		hFile->Seek(pos,SEEK_SET);
		
#ifdef XBMC
		memcpy(&info.ExifInfo, m_exif->m_exifinfo, sizeof(EXIFINFO));
#endif
		
		return m_exif->m_exifinfo->IsExif;
	} else {
		return false;
	}
}
#ifdef XBMC
bool CxImageJPG::GetExifThumbnail(const char *filename, const char *outname, int type)
{
  CxIOFile file;
  if (!file.Open(filename, "rb")) return false;
	CxExifInfo exif(&m_exifinfo);
	exif.DecodeExif(&file);
  if (m_exifinfo.IsExif && m_exifinfo.ThumbnailPointer && m_exifinfo.ThumbnailSize > 0)
  { // have a thumbnail - check whether it needs rotating or resizing
    // TODO: Write a fast routine to read the jpeg header to get the width and height
    CxImage image(m_exifinfo.ThumbnailPointer, m_exifinfo.ThumbnailSize, CXIMAGE_FORMAT_JPG);
    if (image.IsValid())
    {
      if (image.GetWidth() > 256 || image.GetHeight() > 256)
      { // resize the image
//        float amount = 256.0f / max(image.GetWidth(), image.GetHeight());
//        image.Resample((long)(image.GetWidth() * amount), (long)(image.GetHeight() * amount), 0);
      }
      if (m_exifinfo.Orientation != 1)
        image.RotateExif(m_exifinfo.Orientation);
      return image.Save(outname, CXIMAGE_FORMAT_JPG);
    }
    // nice and fast, but we can't resize :(
    /*
    FILE *hFileWrite;
    if ((hFileWrite=fopen(outname, "wb")) != NULL)
    {
      fwrite(m_exifinfo.ThumbnailPointer, m_exifinfo.ThumbnailSize, 1, hFileWrite);
      fclose(hFileWrite);
      return true;
    }*/
  }
  return false;
}
#endif
#endif //CXIMAGEJPG_SUPPORT_EXIF
////////////////////////////////////////////////////////////////////////////////
#if CXIMAGE_SUPPORT_DECODE
////////////////////////////////////////////////////////////////////////////////
#ifdef XBMC
#define RESAMPLE_FACTOR_OF_2_ON_LOAD
#define MAX_PICTURE_AREA 2048*2048 // 4MP == 16MB
#undef RESAMPLE_IF_TOO_BIG
bool CxImageJPG::Decode(CxFile * hFile, int &iMaxWidth, int &iMaxHeight)
#else
bool CxImageJPG::Decode(CxFile * hFile)
#endif
{
#ifdef XBMC
#define BYTES_TO_LOOK 4096

  // Attempt to seek to the first SOI within the first 4k of the file
  BYTE startBuffer[BYTES_TO_LOOK];
  BYTE *pos = startBuffer;
  bool failed = true;
  int size = hFile->Read(startBuffer, 1, BYTES_TO_LOOK);
  while (size > 3)
  {
    if (*pos == 0xFF && *(pos+1) == 0xD8 && *(pos+2) == 0xFF)
    { // found SOI
      hFile->Seek(pos - startBuffer, SEEK_SET);
      failed = false;
      break;
    }
    pos++;
    size--;
  }
  if (failed)
    return false;
#endif

	bool is_exif = false;
#if CXIMAGEJPG_SUPPORT_EXIF
	is_exif = DecodeExif(hFile);
#endif

	CImageIterator iter(this);
	/* This struct contains the JPEG decompression parameters and pointers to
	* working space (which is allocated as needed by the JPEG library).
	*/
	struct jpeg_decompress_struct cinfo;
	/* We use our private extension JPEG error handler. <CSC> */
	struct jpg_error_mgr jerr;
	jerr.buffer=info.szLastError;
	/* More stuff */
#ifdef RESAMPLE_FACTOR_OF_2_ON_LOAD
	JSAMPARRAY buffer[4];
#else
	JSAMPARRAY buffer;	/* Output row buffer */
#endif
	int row_stride;		/* physical row width in output buffer */

	/* In this example we want to open the input file before doing anything else,
	* so that the setjmp() error recovery below can assume the file is open.
	* VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
	* requires it in order to read binary files.
	*/

	/* Step 1: allocate and initialize JPEG decompression object */
	/* We set up the normal JPEG error routines, then override error_exit. */
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = ima_jpeg_error_exit;

	/* Establish the setjmp return context for my_error_exit to use. */
	if (setjmp(jerr.setjmp_buffer)) {
		/* If we get here, the JPEG code has signaled an error.
		* We need to clean up the JPEG object, close the input file, and return.
		*/
		jpeg_destroy_decompress(&cinfo);
		return 0;
	}
	/* Now we can initialize the JPEG decompression object. */
	jpeg_create_decompress(&cinfo);

	/* Step 2: specify data source (eg, a file) */
	//jpeg_stdio_src(&cinfo, infile);
	CxFileJpg src(hFile);
    cinfo.src = &src;

	/* Step 3: read file parameters with jpeg_read_header() */
	(void) jpeg_read_header(&cinfo, TRUE);

	/* Step 4 <chupeev> handle decoder options*/
	if ((GetCodecOption(CXIMAGE_FORMAT_JPG) & DECODE_GRAYSCALE) != 0)
		cinfo.out_color_space = JCS_GRAYSCALE;
	if ((GetCodecOption(CXIMAGE_FORMAT_JPG) & DECODE_QUANTIZE) != 0) {
		cinfo.quantize_colors = TRUE;
		cinfo.desired_number_of_colors = GetJpegQuality();
	}
	if ((GetCodecOption(CXIMAGE_FORMAT_JPG) & DECODE_DITHER) != 0)
		cinfo.dither_mode = m_nDither;
	if ((GetCodecOption(CXIMAGE_FORMAT_JPG) & DECODE_ONEPASS) != 0)
		cinfo.two_pass_quantize = FALSE;
	if ((GetCodecOption(CXIMAGE_FORMAT_JPG) & DECODE_NOSMOOTH) != 0)
		cinfo.do_fancy_upsampling = FALSE;

//<DP>: Load true color images as RGB (no quantize) 
/* Step 4: set parameters for decompression */
/*  if (cinfo.jpeg_color_space!=JCS_GRAYSCALE) {
 *	cinfo.quantize_colors = TRUE;
 *	cinfo.desired_number_of_colors = 128;
 *}
 */ //</DP>

	// Set the scale <ignacio>
	cinfo.scale_denom = GetJpegScale();
	cinfo.scale_num = 1;

	// Borrowed the idea from GIF implementation <ignacio>
	if (info.nEscape == -1) {
		// Return output dimensions only
		jpeg_calc_output_dimensions(&cinfo);
		head.biWidth = cinfo.output_width;
		head.biHeight = cinfo.output_height;
		info.dwType = CXIMAGE_FORMAT_JPG;
		jpeg_destroy_decompress(&cinfo);
		return true;
	}
  // check if we should load it downsampled by a factor of 2,4 or 8
#ifdef RESAMPLE_FACTOR_OF_2_ON_LOAD
	int iWidth = cinfo.image_width;
	int iHeight = cinfo.image_height;
	float fAR = (float)iWidth/(float)iHeight;
	if (iMaxWidth > 0 && iMaxHeight == 0)
	{ // scale by area
		iMaxWidth = (int)sqrt(iMaxWidth * fAR);
		iMaxHeight = (int)(iMaxWidth / fAR);
	}
	if (iMaxWidth > 0 && iMaxHeight > 0)
	{
		if (iWidth > iMaxWidth)
		{
			iWidth = iMaxWidth;
			iHeight = (int)((float)iWidth/fAR);
		}
		if (iHeight > iMaxHeight)
		{
			iHeight = iMaxHeight;
			iWidth = (int)((float)iHeight*fAR);
		}
		unsigned int power = 2;
		cinfo.scale_denom = 1;
		while (power <= 8 && cinfo.image_width >= iWidth * power && cinfo.image_height >= iHeight * power)
		{
			cinfo.scale_denom = power;
			power *= 2;
		}
		jpeg_calc_output_dimensions(&cinfo);
	}
#endif

	/* Step 5: Start decompressor */
	jpeg_start_decompress(&cinfo);

	/* We may need to do some setup of our own at this point before reading
	* the data.  After jpeg_start_decompress() we have the correct scaled
	* output image dimensions available, as well as the output colormap
	* if we asked for color quantization.
	*/
	
#ifdef RESAMPLE_FACTOR_OF_2_ON_LOAD
	// save our image width and height
	iMaxWidth = cinfo.image_width;
	iMaxHeight = cinfo.image_height;
	// check if we're going to exceed our maximum picture size
	bool bScale(false);
#ifdef RESAMPLE_IF_TOO_BIG
	if (cinfo.output_width * cinfo.output_height > MAX_PICTURE_AREA)
	{
		iWidth = (int)sqrt(MAX_PICTURE_AREA * fAR);
		iHeight = (int)(iWidth / fAR);
		bScale = true;
	}
	else
#endif
	{
		iWidth = cinfo.output_width;
		iHeight = cinfo.output_height;
	}
	Create(iWidth, iHeight, 8*cinfo.num_components, CXIMAGE_FORMAT_JPG);
#else
	//Create the image using output dimensions <ignacio>
	//Create(cinfo.image_width, cinfo.image_height, 8*cinfo.output_components, CXIMAGE_FORMAT_JPG);
	Create(cinfo.output_width, cinfo.output_height, 8*cinfo.output_components, CXIMAGE_FORMAT_JPG);
#endif

	if (!pDib) longjmp(jerr.setjmp_buffer, 1);  //<DP> check if the image has been created

	if (is_exif){
#if CXIMAGEJPG_SUPPORT_EXIF
	if ((m_exifinfo.Xresolution != 0.0) && (m_exifinfo.ResolutionUnit != 0))
		SetXDPI((long)(m_exifinfo.Xresolution/m_exifinfo.ResolutionUnit));
	if ((m_exifinfo.Yresolution != 0.0) && (m_exifinfo.ResolutionUnit != 0))
		SetYDPI((long)(m_exifinfo.Yresolution/m_exifinfo.ResolutionUnit));
#endif
	} else {
		switch (cinfo.density_unit) {
		case 0:	// [andy] fix for aspect ratio...
			if((cinfo.Y_density > 0) && (cinfo.X_density > 0)){
				SetYDPI((long)(GetXDPI()*(float(cinfo.Y_density)/float(cinfo.X_density))));
			}
			break;
		case 2: // [andy] fix: cinfo.X/Y_density is pixels per centimeter
			SetXDPI((long)floor(cinfo.X_density * 2.54 + 0.5));
			SetYDPI((long)floor(cinfo.Y_density * 2.54 + 0.5));
			break;
		default:
			SetXDPI(cinfo.X_density);
			SetYDPI(cinfo.Y_density);
		}
	}

	if (cinfo.out_color_space==JCS_GRAYSCALE){
		SetGrayPalette();
		head.biClrUsed =256;
	} else {
		if (cinfo.quantize_colors){
			SetPalette(cinfo.actual_number_of_colors, cinfo.colormap[0], cinfo.colormap[1], cinfo.colormap[2]);
			head.biClrUsed=cinfo.actual_number_of_colors;
		} else {
			head.biClrUsed=0;
		}
	}

	/* JSAMPLEs per row in output buffer */
	row_stride = cinfo.output_width * cinfo.output_components;

#ifdef RESAMPLE_FACTOR_OF_2_ON_LOAD
	/* Make 4 row buffers to do our resampling on */
	for (int i=0; i<4; i++)
	{
		buffer[i] = (*cinfo.mem->alloc_sarray)
		((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);
	}
#else
	/* Make a one-row-high sample array that will go away when done with image */
	buffer = (*cinfo.mem->alloc_sarray)
		((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);
#endif
	/* Step 6: while (scan lines remain to be read) */
	/*           jpeg_read_scanlines(...); */
	/* Here we use the library's state variable cinfo.output_scanline as the
	* loop counter, so that we don't have to keep track ourselves.
	*/
	iter.Upset();
#ifdef RESAMPLE_FACTOR_OF_2_ON_LOAD
	int row_position = 0;
	int next_row_slot = 0;
	int num_scanlines_read = 0;

	float xScale = (float)cinfo.output_width/(float)iWidth;
	float yScale = (float)cinfo.output_height/(float)iHeight;
	int num = cinfo.num_components;

	float f_y, f_x, a, b, rr[4], r1, r2;
	int i_y, i_x, xx, yy;

	for (int y=0;y<iHeight;y++)
	{
		f_y = (float) y * yScale - 0.5f;
		i_y = (int) floor(f_y);
		a = f_y - (float)floor(f_y);
		// the rows we need are:
		// i_y-1, i_y, i_y+1, i_y+2
		int iNeededRow = i_y+2;
		if (iNeededRow < 0) iNeededRow = 0;
		if (iNeededRow >= (int)cinfo.output_height) iNeededRow = cinfo.image_height-1;
		bool bFinished(false);
#else
	while (cinfo.output_scanline < cinfo.output_height) {
#endif
		if (info.nEscape) longjmp(jerr.setjmp_buffer, 1); // <vho> - cancel decoding
		
#ifdef RESAMPLE_FACTOR_OF_2_ON_LOAD
		// decode as many rows as we need
		if (bScale)
		{
			while (iNeededRow >= row_position)
			{
				num_scanlines_read += jpeg_read_scanlines(&cinfo, buffer[next_row_slot], 1);;
				// rotate around
				next_row_slot++;
				if (next_row_slot >= 4)
					next_row_slot = 0;
				row_position++;
			}
		}
		else
		{
			(void) jpeg_read_scanlines(&cinfo, buffer[0], 1);
			num_scanlines_read++;
		}
#else
		(void) jpeg_read_scanlines(&cinfo, buffer, 1);
#endif
		// info.nProgress = (long)(100*cinfo.output_scanline/cinfo.output_height);
		//<DP> Step 6a: CMYK->RGB */ 
#ifdef RESAMPLE_FACTOR_OF_2_ON_LOAD
		// do bicubic resampling
		BYTE *dst=iter.GetRow();
		for (int x = 0; x < iWidth; x++)
		{
			if (bScale)
			{
				// resample (bicubic)
				f_x = (float) x * xScale - 0.5f;
				i_x = (int) floor(f_x);
				b   = f_x - (float)floor(f_x);

				for (int k=0; k<num; k++)
					rr[k] = 0;
				// cycle around the necessary y rows...
				for (int m=-1; m<3; m++)
				{
					r1 = KernelBSpline((float) m - a);
					yy = i_y+m;
					if (yy<0) yy=0;
					if (yy>=(int)cinfo.image_height) yy = cinfo.image_height-1;
					// calculate which y row we should be using
					int row = next_row_slot;
					for (int i=row_position; i>yy; i--)
					{
						row--;
						if (row < 0) row = 3;
					}
					for(int n=-1; n<3; n++)
					{
						r2 = r1 * KernelBSpline(b - (float)n);
						xx = i_x+n;
						if (xx<0) xx=0;
						if (xx>=(int)cinfo.image_width) xx=cinfo.image_width-1;
						BYTE *pSrc = buffer[row][0];
						if ((cinfo.num_components==4)&&(cinfo.quantize_colors==FALSE))
						{
							float k = (float)pSrc[cinfo.num_components*xx+3];
							rr[0] += (k * pSrc[cinfo.num_components*xx+2])/255.0f*r2;
							rr[1] += (k * pSrc[cinfo.num_components*xx+1])/255.0f*r2;
							rr[2] += (k * pSrc[cinfo.num_components*xx+0])/255.0f*r2;
						}
						else
						{
							for (int k=0; k<num; k++)
								rr[k] += pSrc[cinfo.num_components*xx+k]*r2;
						}
					}
				}
			}
			else
			{
				BYTE *pSrc = buffer[0][0];
				if ((cinfo.num_components==4)&&(cinfo.quantize_colors==FALSE))
				{
					float k = (float)pSrc[cinfo.num_components*x+3];
					rr[0] = (k * pSrc[cinfo.num_components*x+2])/255.0f;
					rr[1] = (k * pSrc[cinfo.num_components*x+1])/255.0f;
					rr[2] = (k * pSrc[cinfo.num_components*x+0])/255.0f;
				}
				else
				{
					for (int k=0; k<num; k++)
						rr[k] = (float)pSrc[cinfo.num_components*x+k];
				}
			}
			if ((cinfo.num_components==4)&&(cinfo.quantize_colors==FALSE))
			{
				for (int k=0; k<3; k++)
					dst[3*x+k] = (BYTE)rr[k];
			}
			else
			{
				for (int k=0; k<num; k++)
					dst[num*x+k] = (BYTE)rr[k];
			}
		}
#else
		if ((cinfo.num_components==4)&&(cinfo.quantize_colors==FALSE)){
			BYTE k,*dst,*src;
			dst=iter.GetRow();
			src=buffer[0];
			for(long x3=0,x4=0; x3<(long)info.dwEffWidth && x4<row_stride; x3+=3, x4+=4){
				k=src[x4+3];
				dst[x3]  =(BYTE)((k * src[x4+2])/255);
				dst[x3+1]=(BYTE)((k * src[x4+1])/255);
				dst[x3+2]=(BYTE)((k * src[x4+0])/255);
			}
		} else {
			/* Assume put_scanline_someplace wants a pointer and sample count. */
			iter.SetRow(buffer[0], row_stride);
		}
#endif
			iter.PrevRow();
	}
#ifdef RESAMPLE_FACTOR_OF_2_ON_LOAD
	while (cinfo.output_scanline < cinfo.output_height)
	{
		(void) jpeg_read_scanlines(&cinfo, buffer[0], 1);
		num_scanlines_read++;
	}
#endif

	/* Step 7: Finish decompression */
	(void) jpeg_finish_decompress(&cinfo);
	/* We can ignore the return value since suspension is not possible
	* with the stdio data source.
	*/

	//<DP> Step 7A: Swap red and blue components
	// not necessary if swapped red and blue definition in jmorecfg.h;ln322 <W. Morrison>
	if ((cinfo.num_components==3)&&(cinfo.quantize_colors==FALSE)){
		BYTE* r0=GetBits();
		for(long y=0;y<head.biHeight;y++){
			if (info.nEscape) longjmp(jerr.setjmp_buffer, 1); // <vho> - cancel decoding
			RGBtoBGR(r0,3*head.biWidth);
			r0+=info.dwEffWidth;
		}
	}

	/* Step 8: Release JPEG decompression object */
	/* This is an important step since it will release a good deal of memory. */
	jpeg_destroy_decompress(&cinfo);

	/* At this point you may want to check to see whether any corrupt-data
	* warnings occurred (test whether jerr.pub.num_warnings is nonzero).
	*/

	/* And we're done! */
	return true;
}
////////////////////////////////////////////////////////////////////////////////
#endif //CXIMAGE_SUPPORT_DECODE
////////////////////////////////////////////////////////////////////////////////
#if CXIMAGE_SUPPORT_ENCODE
////////////////////////////////////////////////////////////////////////////////
bool CxImageJPG::Encode(CxFile * hFile)
{
	if (EncodeSafeCheck(hFile)) return false;

	if (head.biClrUsed!=0 && !IsGrayScale()){
		strcpy(info.szLastError,"JPEG can save only RGB or GreyScale images");
		return false;
	}	

	// necessary for EXIF, and for roll backs
	long pos=hFile->Tell();

	/* This struct contains the JPEG compression parameters and pointers to
	* working space (which is allocated as needed by the JPEG library).
	* It is possible to have several such structures, representing multiple
	* compression/decompression processes, in existence at once.  We refer
	* to any one struct (and its associated working data) as a "JPEG object".
	*/
	struct jpeg_compress_struct cinfo;
	/* This struct represents a JPEG error handler.  It is declared separately
	* because applications often want to supply a specialized error handler
	* (see the second half of this file for an example).  But here we just
	* take the easy way out and use the standard error handler, which will
	* print a message on stderr and call exit() if compression fails.
	* Note that this struct must live as long as the main JPEG parameter
	* struct, to avoid dangling-pointer problems.
	*/
	//struct jpeg_error_mgr jerr;
	/* We use our private extension JPEG error handler. <CSC> */
	struct jpg_error_mgr jerr;
	jerr.buffer=info.szLastError;
	/* More stuff */
	int row_stride;		/* physical row width in image buffer */
	JSAMPARRAY buffer;		/* Output row buffer */

	/* Step 1: allocate and initialize JPEG compression object */
	/* We have to set up the error handler first, in case the initialization
	* step fails.  (Unlikely, but it could happen if you are out of memory.)
	* This routine fills in the contents of struct jerr, and returns jerr's
	* address which we place into the link field in cinfo.
	*/
	//cinfo.err = jpeg_std_error(&jerr); <CSC>
	/* We set up the normal JPEG error routines, then override error_exit. */
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = ima_jpeg_error_exit;

	/* Establish the setjmp return context for my_error_exit to use. */
	if (setjmp(jerr.setjmp_buffer)) {
		/* If we get here, the JPEG code has signaled an error.
		* We need to clean up the JPEG object, close the input file, and return.
		*/
		strcpy(info.szLastError, jerr.buffer); //<CSC>
		jpeg_destroy_compress(&cinfo);
		return 0;
	}
	
	/* Now we can initialize the JPEG compression object. */
	jpeg_create_compress(&cinfo);
	/* Step 2: specify data destination (eg, a file) */
	/* Note: steps 2 and 3 can be done in either order. */
	/* Here we use the library-supplied code to send compressed data to a
	* stdio stream.  You can also write your own code to do something else.
	* VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
	* requires it in order to write binary files.
	*/

	//jpeg_stdio_dest(&cinfo, outfile);
	CxFileJpg dest(hFile);
    cinfo.dest = &dest;

	/* Step 3: set parameters for compression */
	/* First we supply a description of the input image.
	* Four fields of the cinfo struct must be filled in:
	*/
	cinfo.image_width = GetWidth(); 	// image width and height, in pixels
	cinfo.image_height = GetHeight();

	if (IsGrayScale()){
		cinfo.input_components = 1;			// # of color components per pixel
		cinfo.in_color_space = JCS_GRAYSCALE; /* colorspace of input image */
	} else {
		cinfo.input_components = 3; 	// # of color components per pixel
		cinfo.in_color_space = JCS_RGB; /* colorspace of input image */
	}

	/* Now use the library's routine to set default compression parameters.
	* (You must set at least cinfo.in_color_space before calling this,
	* since the defaults depend on the source color space.)
	*/
	jpeg_set_defaults(&cinfo);
	/* Now you can set any non-default parameters you wish to.
	* Here we just illustrate the use of quality (quantization table) scaling:
	*/

//#ifdef C_ARITH_CODING_SUPPORTED
	if ((GetCodecOption(CXIMAGE_FORMAT_JPG) & ENCODE_ARITHMETIC) != 0)
		cinfo.arith_code = TRUE;
//#endif

//#ifdef ENTROPY_OPT_SUPPORTED
	if ((GetCodecOption(CXIMAGE_FORMAT_JPG) & ENCODE_OPTIMIZE) != 0)
		cinfo.optimize_coding = TRUE;
//#endif

	if ((GetCodecOption(CXIMAGE_FORMAT_JPG) & ENCODE_GRAYSCALE) != 0)
		jpeg_set_colorspace(&cinfo, JCS_GRAYSCALE);

	if ((GetCodecOption(CXIMAGE_FORMAT_JPG) & ENCODE_SMOOTHING) != 0)
		cinfo.smoothing_factor = m_nSmoothing;

	jpeg_set_quality(&cinfo, GetJpegQuality(), (GetCodecOption(CXIMAGE_FORMAT_JPG) & ENCODE_BASELINE) != 0);

//#ifdef C_PROGRESSIVE_SUPPORTED
	if ((GetCodecOption(CXIMAGE_FORMAT_JPG) & ENCODE_PROGRESSIVE) != 0)
		jpeg_simple_progression(&cinfo);
//#endif

#ifdef C_LOSSLESS_SUPPORTED
	if ((GetCodecOption(CXIMAGE_FORMAT_JPG) & ENCODE_LOSSLESS) != 0)
		jpeg_simple_lossless(&cinfo, m_nPredictor, m_nPointTransform);
#endif

	//SetCodecOption(ENCODE_SUBSAMPLE_444 | GetCodecOption(CXIMAGE_FORMAT_JPG),CXIMAGE_FORMAT_JPG);

		// 2x2, 1x1, 1x1 (4:1:1) : High (default sub sampling)
		cinfo.comp_info[0].h_samp_factor = 2;
		cinfo.comp_info[0].v_samp_factor = 2;
		cinfo.comp_info[1].h_samp_factor = 1;
		cinfo.comp_info[1].v_samp_factor = 1;
		cinfo.comp_info[2].h_samp_factor = 1;
		cinfo.comp_info[2].v_samp_factor = 1;

	if ((GetCodecOption(CXIMAGE_FORMAT_JPG) & ENCODE_SUBSAMPLE_422) != 0){
		// 2x1, 1x1, 1x1 (4:2:2) : Medium
		cinfo.comp_info[0].h_samp_factor = 2;
		cinfo.comp_info[0].v_samp_factor = 1;
		cinfo.comp_info[1].h_samp_factor = 1;
		cinfo.comp_info[1].v_samp_factor = 1;
		cinfo.comp_info[2].h_samp_factor = 1;
		cinfo.comp_info[2].v_samp_factor = 1;
	}

	if ((GetCodecOption(CXIMAGE_FORMAT_JPG) & ENCODE_SUBSAMPLE_444) != 0){
		// 1x1 1x1 1x1 (4:4:4) : None
		cinfo.comp_info[0].h_samp_factor = 1;
		cinfo.comp_info[0].v_samp_factor = 1;
		cinfo.comp_info[1].h_samp_factor = 1;
		cinfo.comp_info[1].v_samp_factor = 1;
		cinfo.comp_info[2].h_samp_factor = 1;
		cinfo.comp_info[2].v_samp_factor = 1;
	}

	cinfo.density_unit=1;
	cinfo.X_density=(unsigned short)GetXDPI();
	cinfo.Y_density=(unsigned short)GetYDPI();

	/* Step 4: Start compressor */
	/* TRUE ensures that we will write a complete interchange-JPEG file.
	* Pass TRUE unless you are very sure of what you're doing.
	*/
	jpeg_start_compress(&cinfo, TRUE);

	/* Step 5: while (scan lines remain to be written) */
	/*           jpeg_write_scanlines(...); */
	/* Here we use the library's state variable cinfo.next_scanline as the
	* loop counter, so that we don't have to keep track ourselves.
	* To keep things simple, we pass one scanline per call; you can pass
	* more if you wish, though.
	*/
	row_stride = info.dwEffWidth;	/* JSAMPLEs per row in image_buffer */

	//<DP> "8+row_stride" fix heap deallocation problem during debug???
	buffer = (*cinfo.mem->alloc_sarray)
		((j_common_ptr) &cinfo, JPOOL_IMAGE, 8+row_stride, 1);

	CImageIterator iter(this);

	iter.Upset();
	while (cinfo.next_scanline < cinfo.image_height) {
		// info.nProgress = (long)(100*cinfo.next_scanline/cinfo.image_height);
		iter.GetRow(buffer[0], row_stride);
		// not necessary if swapped red and blue definition in jmorecfg.h;ln322 <W. Morrison>
		if (head.biClrUsed==0){				 // swap R & B for RGB images
			RGBtoBGR(buffer[0], row_stride); // Lance : 1998/09/01 : Bug ID: EXP-2.1.1-9
		}
		iter.PrevRow();
		(void) jpeg_write_scanlines(&cinfo, buffer, 1);
	}

	/* Step 6: Finish compression */
	jpeg_finish_compress(&cinfo);

	/* Step 7: release JPEG compression object */
	/* This is an important step since it will release a good deal of memory. */
	jpeg_destroy_compress(&cinfo);


#if CXIMAGEJPG_SUPPORT_EXIF
	if (m_exif && m_exif->m_exifinfo->IsExif){
		// discard useless sections (if any) read from original image
		m_exif->DiscardAllButExif();
		// read new created image, to split the sections
		hFile->Seek(pos,SEEK_SET);
		m_exif->DecodeExif(hFile,EXIF_READ_IMAGE);
		// save back the image, adding EXIF section
		hFile->Seek(pos,SEEK_SET);
		m_exif->EncodeExif(hFile);
	}
#endif


	/* And we're done! */
	return true;
}
////////////////////////////////////////////////////////////////////////////////
#endif // CXIMAGE_SUPPORT_ENCODE
////////////////////////////////////////////////////////////////////////////////
#endif // CXIMAGE_SUPPORT_JPG

