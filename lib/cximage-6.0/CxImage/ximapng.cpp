/*
 * File:	ximapng.cpp
 * Purpose:	Platform Independent PNG Image Class Loader and Writer
 * 07/Aug/2001 Davide Pizzolato - www.xdp.it
 * CxImage version 6.0.0 02/Feb/2008
 */

#include "ximapng.h"

#if CXIMAGE_SUPPORT_PNG

#include "ximaiter.h"

////////////////////////////////////////////////////////////////////////////////
void CxImagePNG::ima_png_error(png_struct *png_ptr, char *message)
{
	strcpy(info.szLastError,message);
#ifdef USE_NEW_LIBPNG_API
	longjmp(png_jmpbuf(png_ptr), 1);
#else
	longjmp(png_ptr->jmpbuf, 1);
#endif
}
////////////////////////////////////////////////////////////////////////////////
#if CXIMAGE_SUPPORT_DECODE
////////////////////////////////////////////////////////////////////////////////
void CxImagePNG::expand2to4bpp(BYTE* prow)
{
	BYTE *psrc,*pdst;
	BYTE pos,idx;
	for(long x=head.biWidth-1;x>=0;x--){
		psrc = prow + ((2*x)>>3);
		pdst = prow + ((4*x)>>3);
		pos = (BYTE)(2*(3-x%4));
		idx = (BYTE)((*psrc & (0x03<<pos))>>pos);
		pos = (BYTE)(4*(1-x%2));
		*pdst &= ~(0x0F<<pos);
		*pdst |= (idx & 0x0F)<<pos;
	}
}
////////////////////////////////////////////////////////////////////////////////
bool CxImagePNG::Decode(CxFile *hFile)
{
	png_struct *png_ptr;
	png_info *info_ptr;
	BYTE *row_pointers=NULL;
	CImageIterator iter(this);

  cx_try
  {
    /* Create and initialize the png_struct with the desired error handler
    * functions.  If you want to use the default stderr and longjump method,
    * you can supply NULL for the last three parameters.  We also supply the
    * the compiler header file version, so that we know if the application
    * was compiled with a compatible version of the library.  REQUIRED    */
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,(void *)NULL,NULL,NULL);
	if (png_ptr == NULL)  cx_throw("Failed to create PNG structure");

	/* Allocate/initialize the memory for image information.  REQUIRED. */
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
		cx_throw("Failed to initialize PNG info structure");
	}

    /* Set error handling if you are using the setjmp/longjmp method (this is
    * the normal method of doing things with libpng).  REQUIRED unless you
    * set up your own error handlers in the png_create_read_struct() earlier. */
#ifdef USE_NEW_LIBPNG_API
	if (setjmp(png_jmpbuf(png_ptr))) {
#else
	if (setjmp(png_ptr->jmpbuf)) {
#endif
		/* Free all of the memory associated with the png_ptr and info_ptr */
		delete [] row_pointers;
		row_pointers = nullptr;
		png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
		cx_throw("");
	}

	// use custom I/O functions
    png_set_read_fn(png_ptr, hFile, /*(png_rw_ptr)*/user_read_data);
	png_set_error_fn(png_ptr,info.szLastError,/*(png_error_ptr)*/user_error_fn,NULL);

	/* read the file information */
	png_read_info(png_ptr, info_ptr);

#ifdef USE_NEW_LIBPNG_API
	png_uint_32 _width,_height;
	int _bit_depth,_color_type,_interlace_type,_compression_type,_filter_type;
	png_get_IHDR(png_ptr,info_ptr,&_width,&_height,&_bit_depth,&_color_type,
		&_interlace_type,&_compression_type,&_filter_type);

	if (info.nEscape == -1){
		head.biWidth = _width;
		head.biHeight= _height;
		info.dwType = CXIMAGE_FORMAT_PNG;
		longjmp(png_jmpbuf(png_ptr), 1);
	}
#else
	if (info.nEscape == -1){
		head.biWidth = info_ptr->width;
		head.biHeight= info_ptr->height;
		info.dwType = CXIMAGE_FORMAT_PNG;
		longjmp(png_ptr->jmpbuf, 1);
	}
#endif

	/* calculate new number of channels */
	int channels=0;
#ifdef USE_NEW_LIBPNG_API
	switch(_color_type){
#else
	switch(info_ptr->color_type){
#endif
	case PNG_COLOR_TYPE_GRAY:
	case PNG_COLOR_TYPE_PALETTE:
		channels = 1;
		break;
	case PNG_COLOR_TYPE_GRAY_ALPHA:
		channels = 2;
		break;
	case PNG_COLOR_TYPE_RGB:
		channels = 3;
		break;
	case PNG_COLOR_TYPE_RGB_ALPHA:
		channels = 4;
		break;
	default:
		strcpy(info.szLastError,"unknown PNG color type");
#ifdef USE_NEW_LIBPNG_API
		longjmp(png_jmpbuf(png_ptr), 1);
#else
		longjmp(png_ptr->jmpbuf, 1);
#endif
	}

	//find the right pixel depth used for cximage
#ifdef USE_NEW_LIBPNG_API
	int pixel_depth = _bit_depth * png_get_channels(png_ptr,info_ptr);
#else
	int pixel_depth = info_ptr->pixel_depth;
#endif
	if (channels == 1 && pixel_depth>8) pixel_depth=8;
	if (channels == 2) pixel_depth=8;
	if (channels >= 3) pixel_depth=24;

#ifdef USE_NEW_LIBPNG_API
	if (!Create(_width, _height, pixel_depth, CXIMAGE_FORMAT_PNG)){
		longjmp(png_jmpbuf(png_ptr), 1);
#else
	if (!Create(info_ptr->width, info_ptr->height, pixel_depth, CXIMAGE_FORMAT_PNG)){
		longjmp(png_ptr->jmpbuf, 1);
#endif
	}

	/* get metrics */
#ifdef USE_NEW_LIBPNG_API
	png_uint_32 _x_pixels_per_unit,_y_pixels_per_unit;
	int _phys_unit_type;
	png_get_pHYs(png_ptr,info_ptr,&_x_pixels_per_unit,&_y_pixels_per_unit,&_phys_unit_type);
	switch (_phys_unit_type)
	{
	case PNG_RESOLUTION_UNKNOWN:
		SetXDPI(_x_pixels_per_unit);
		SetYDPI(_y_pixels_per_unit);
		break;
	case PNG_RESOLUTION_METER:
		SetXDPI((long)floor(_x_pixels_per_unit * 254.0 / 10000.0 + 0.5));
		SetYDPI((long)floor(_y_pixels_per_unit * 254.0 / 10000.0 + 0.5));
		break;
	}
#else
	switch (info_ptr->phys_unit_type)
	{
	case PNG_RESOLUTION_UNKNOWN:
		SetXDPI(info_ptr->x_pixels_per_unit);
		SetYDPI(info_ptr->y_pixels_per_unit);
		break;
	case PNG_RESOLUTION_METER:
		SetXDPI((long)floor(info_ptr->x_pixels_per_unit * 254.0 / 10000.0 + 0.5));
		SetYDPI((long)floor(info_ptr->y_pixels_per_unit * 254.0 / 10000.0 + 0.5));
		break;
	}
#endif

#ifdef USE_NEW_LIBPNG_API
	int _num_palette;
	png_colorp _palette;
	png_uint_32 _palette_ret;
	_palette_ret = png_get_PLTE(png_ptr,info_ptr,&_palette,&_num_palette);
	if (_palette_ret && _num_palette>0){
		SetPalette((rgb_color*)_palette,_num_palette);
		SetClrImportant(_num_palette);
	} else if (_bit_depth ==2) { //<DP> needed for 2 bpp grayscale PNGs
#else
	if (info_ptr->num_palette>0){
		SetPalette((rgb_color*)info_ptr->palette,info_ptr->num_palette);
		SetClrImportant(info_ptr->num_palette);
	} else if (info_ptr->bit_depth ==2) { //<DP> needed for 2 bpp grayscale PNGs
#endif
		SetPaletteColor(0,0,0,0);
		SetPaletteColor(1,85,85,85);
		SetPaletteColor(2,170,170,170);
		SetPaletteColor(3,255,255,255);
	} else SetGrayPalette(); //<DP> needed for grayscale PNGs
	
#ifdef USE_NEW_LIBPNG_API
	int nshift = max(0,(_bit_depth>>3)-1)<<3;
#else
	int nshift = max(0,(info_ptr->bit_depth>>3)-1)<<3;
#endif

#ifdef USE_NEW_LIBPNG_API
	png_bytep _trans_alpha;
	int _num_trans;
	png_color_16p _trans_color;
	png_uint_32 _trans_ret;
	_trans_ret = png_get_tRNS(png_ptr,info_ptr,&_trans_alpha,&_num_trans,&_trans_color);
	if (_trans_ret && _num_trans!=0){ //palette transparency
		if (_num_trans==1){
			if (_color_type == PNG_COLOR_TYPE_PALETTE){
#else
	if (info_ptr->num_trans!=0){ //palette transparency
		if (info_ptr->num_trans==1){
			if (info_ptr->color_type == PNG_COLOR_TYPE_PALETTE){
#endif
#ifdef USE_NEW_LIBPNG_API
				info.nBkgndIndex = _trans_color->index;
#else
#if PNG_LIBPNG_VER > 10399
				info.nBkgndIndex = info_ptr->trans_color.index;
#else
				info.nBkgndIndex = info_ptr->trans_values.index;
#endif
#endif
			} else{
#ifdef USE_NEW_LIBPNG_API
				info.nBkgndIndex = _trans_color->gray>>nshift;
#else
#if PNG_LIBPNG_VER > 10399
				info.nBkgndIndex = info_ptr->trans_color.gray>>nshift;
#else
				info.nBkgndIndex = info_ptr->trans_values.gray>>nshift;
#endif
#endif
			}
		}
#ifdef USE_NEW_LIBPNG_API
		if (_num_trans>1 && _trans_alpha!=NULL){
#else
		if (info_ptr->num_trans>1){
#endif
			RGBQUAD* pal=GetPalette();
			if (pal){
				DWORD ip;
#ifdef USE_NEW_LIBPNG_API
				for (ip=0;ip<min(head.biClrUsed,(unsigned long)_num_trans);ip++)
					pal[ip].rgbReserved=_trans_alpha[ip];
#else
				for (ip=0;ip<min(head.biClrUsed,(unsigned long)info_ptr->num_trans);ip++)
#if PNG_LIBPNG_VER > 10399
					pal[ip].rgbReserved=info_ptr->trans_alpha[ip];
#else
					pal[ip].rgbReserved=info_ptr->trans[ip];
#endif
#endif
#ifdef USE_NEW_LIBPNG_API
				for (ip=_num_trans;ip<head.biClrUsed;ip++){
#else
				for (ip=info_ptr->num_trans;ip<head.biClrUsed;ip++){
#endif
					pal[ip].rgbReserved=255;
				}
				info.bAlphaPaletteEnabled=true;
			}
		}
	}

	if (channels == 3){ //check RGB binary transparency
		png_bytep trans;
		int num_trans;
		png_color_16 *image_background;
		if (png_get_tRNS(png_ptr, info_ptr, &trans, &num_trans, &image_background)){
#ifdef USE_NEW_LIBPNG_API
			info.nBkgndColor.rgbRed   = (BYTE)(_trans_color->red>>nshift);
			info.nBkgndColor.rgbGreen = (BYTE)(_trans_color->green>>nshift);
			info.nBkgndColor.rgbBlue  = (BYTE)(_trans_color->blue>>nshift);
#else
#if PNG_LIBPNG_VER > 10399
			info.nBkgndColor.rgbRed   = (BYTE)(info_ptr->trans_color.red>>nshift);
			info.nBkgndColor.rgbGreen = (BYTE)(info_ptr->trans_color.green>>nshift);
			info.nBkgndColor.rgbBlue  = (BYTE)(info_ptr->trans_color.blue>>nshift);
#else
			info.nBkgndColor.rgbRed   = (BYTE)(info_ptr->trans_values.red>>nshift);
			info.nBkgndColor.rgbGreen = (BYTE)(info_ptr->trans_values.green>>nshift);
			info.nBkgndColor.rgbBlue  = (BYTE)(info_ptr->trans_values.blue>>nshift);
#endif
#endif
			info.nBkgndColor.rgbReserved = 0;
			info.nBkgndIndex = 0;
		}
	}

	int alpha_present = (channels - 1) % 2;
	if (alpha_present){
#if CXIMAGE_SUPPORT_ALPHA	// <vho>
		AlphaCreate();
#else
		png_set_strip_alpha(png_ptr);
#endif //CXIMAGE_SUPPORT_ALPHA
	}

	// <vho> - flip the RGB pixels to BGR (or RGBA to BGRA)
#ifdef USE_NEW_LIBPNG_API
	if (_color_type & PNG_COLOR_MASK_COLOR){
#else
	if (info_ptr->color_type & PNG_COLOR_MASK_COLOR){
#endif
		png_set_bgr(png_ptr);
	}

	// <vho> - handle cancel
#ifdef USE_NEW_LIBPNG_API
	if (info.nEscape) longjmp(png_jmpbuf(png_ptr), 1);
#else
	if (info.nEscape) longjmp(png_ptr->jmpbuf, 1);
#endif

	// row_bytes is the width x number of channels x (bit-depth / 8)
#ifdef USE_NEW_LIBPNG_API
	row_pointers = new BYTE[png_get_rowbytes(png_ptr,info_ptr) + 8];
#else
	row_pointers = new BYTE[info_ptr->rowbytes + 8];
#endif

	// turn on interlace handling
	int number_passes = png_set_interlace_handling(png_ptr);

	if (number_passes>1){
		SetCodecOption(1);
	} else {
		SetCodecOption(0);
	}

#ifdef USE_NEW_LIBPNG_API
	int chan_offset = _bit_depth >> 3;
#else
	int chan_offset = info_ptr->bit_depth >> 3;
#endif
#ifdef USE_NEW_LIBPNG_API
	int pixel_offset = (_bit_depth * png_get_channels(png_ptr,info_ptr)) >> 3;
#else
	int pixel_offset = info_ptr->pixel_depth >> 3;
#endif

	for (int pass=0; pass < number_passes; pass++) {
		iter.Upset();
		int y=0;
		do	{

			// <vho> - handle cancel
#ifdef USE_NEW_LIBPNG_API
			if (info.nEscape) longjmp(png_jmpbuf(png_ptr), 1);
#else
			if (info.nEscape) longjmp(png_ptr->jmpbuf, 1);
#endif

#if CXIMAGE_SUPPORT_ALPHA	// <vho>
			if (AlphaIsValid()) {

				//compute the correct position of the line
				long ax,ay;
				ay = head.biHeight-1-y;
				BYTE* prow= iter.GetRow(ay);

				//recover data from previous scan
#ifdef USE_NEW_LIBPNG_API
				if (_interlace_type && pass>0 && pass!=7){
#else
				if (info_ptr->interlace_type && pass>0 && pass!=7){
#endif
					for(ax=0;ax<head.biWidth;ax++){
						long px = ax * pixel_offset;
						if (channels == 2){
							row_pointers[px] = prow[ax];
							row_pointers[px+chan_offset]=AlphaGet(ax,ay);
						} else {
							long qx = ax * 3;
							row_pointers[px]              =prow[qx];
							row_pointers[px+chan_offset]  =prow[qx+1];
							row_pointers[px+chan_offset*2]=prow[qx+2];
							row_pointers[px+chan_offset*3]=AlphaGet(ax,ay);
						}
					}
				}

				//read next row
				png_read_row(png_ptr, row_pointers, NULL);

				//RGBA -> RGB + A
				for(ax=0;ax<head.biWidth;ax++){
					long px = ax * pixel_offset;
					if (channels == 2){
						prow[ax] = row_pointers[px];
						AlphaSet(ax,ay,row_pointers[px+chan_offset]);
					} else {
						long qx = ax * 3;
						prow[qx]  =row_pointers[px];
						prow[qx+1]=row_pointers[px+chan_offset];
						prow[qx+2]=row_pointers[px+chan_offset*2];
						AlphaSet(ax,ay,row_pointers[px+chan_offset*3]);
					}
				}
			} else
#endif // CXIMAGE_SUPPORT_ALPHA		// vho
			{
				//recover data from previous scan
#ifdef USE_NEW_LIBPNG_API
				if (_interlace_type && pass>0){
					iter.GetRow(row_pointers, png_get_rowbytes(png_ptr,info_ptr));
					//re-expand buffer for images with bit depth > 8
					if (_bit_depth > 8){
#else
				if (info_ptr->interlace_type && pass>0){
					iter.GetRow(row_pointers, info_ptr->rowbytes);
					//re-expand buffer for images with bit depth > 8
					if (info_ptr->bit_depth > 8){
#endif
						for(long ax=(head.biWidth*channels-1);ax>=0;ax--)
							row_pointers[ax*chan_offset] = row_pointers[ax];
					}
				}

				//read next row
				png_read_row(png_ptr, row_pointers, NULL);

				//shrink 16 bit depth images down to 8 bits
#ifdef USE_NEW_LIBPNG_API
				if (_bit_depth > 8){
#else
				if (info_ptr->bit_depth > 8){
#endif
					for(long ax=0;ax<(head.biWidth*channels);ax++)
						row_pointers[ax] = row_pointers[ax*chan_offset];
				}

				//copy the pixels
#ifdef USE_NEW_LIBPNG_API
				iter.SetRow(row_pointers, png_get_rowbytes(png_ptr,info_ptr));
#else
				iter.SetRow(row_pointers, info_ptr->rowbytes);
#endif
				//<DP> expand 2 bpp images only in the last pass
#ifdef USE_NEW_LIBPNG_API
				if (_bit_depth==2 && pass==(number_passes-1))
#else
				if (info_ptr->bit_depth==2 && pass==(number_passes-1))
#endif
					expand2to4bpp(iter.GetRow());

				//go on
				iter.PrevRow();
			}

			y++;
		} while(y<head.biHeight);
	}

	delete [] row_pointers;
	row_pointers = nullptr;

	/* read the rest of the file, getting any additional chunks in info_ptr */
	png_read_end(png_ptr, info_ptr);

	/* clean up after the read, and free any memory allocated - REQUIRED */
	png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);

  } cx_catch {
	if (strcmp(message,"")) strncpy(info.szLastError,message,255);
	if (info.nEscape == -1 && info.dwType == CXIMAGE_FORMAT_PNG) return true;
	return false;
  }
	/* that's it */
	return true;
}
////////////////////////////////////////////////////////////////////////////////
#endif //CXIMAGE_SUPPORT_DECODE
////////////////////////////////////////////////////////////////////////////////
#if CXIMAGE_SUPPORT_ENCODE
////////////////////////////////////////////////////////////////////////////////
bool CxImagePNG::Encode(CxFile *hFile)
{
	if (EncodeSafeCheck(hFile)) return false;

	CImageIterator iter(this);
	BYTE trans[256];	//for transparency (don't move)
	png_struct *png_ptr;
	png_info *info_ptr;

  cx_try
  {
   /* Create and initialize the png_struct with the desired error handler
    * functions.  If you want to use the default stderr and longjump method,
    * you can supply NULL for the last three parameters.  We also check that
    * the library version is compatible with the one used at compile time,
    * in case we are using dynamically linked libraries.  REQUIRED.
    */
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,(void *)NULL,NULL,NULL);
	if (png_ptr == NULL) cx_throw("Failed to create PNG structure");

	/* Allocate/initialize the image information data.  REQUIRED */
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL){
		png_destroy_write_struct(&png_ptr,  (png_infopp)NULL);
		cx_throw("Failed to initialize PNG info structure");
	}

   /* Set error handling.  REQUIRED if you aren't supplying your own
    * error hadnling functions in the png_create_write_struct() call.
    */
#ifdef USE_NEW_LIBPNG_API
	if (setjmp(png_jmpbuf(png_ptr))){
#else
	if (setjmp(png_ptr->jmpbuf)){
		/* If we get here, we had a problem reading the file */
		if (info_ptr->palette) free(info_ptr->palette);
#endif
		png_destroy_write_struct(&png_ptr,  (png_infopp)&info_ptr);
		cx_throw("Error saving PNG file");
	}
            
	/* set up the output control */
	//png_init_io(png_ptr, hFile);

	// use custom I/O functions
	png_set_write_fn(png_ptr,hFile,/*(png_rw_ptr)*/user_write_data,/*(png_flush_ptr)*/user_flush_data);

	/* set the file information here */
#ifdef USE_NEW_LIBPNG_API
	/* use variables to hold the values so it isnt necessary to png_get them later */
	png_uint_32 _width,_height;
	int _bit_depth,_color_type,_interlace_type,_compression_type,_filter_type;
	png_byte _channels,_pixel_depth;

	_width = GetWidth();
	_height = GetHeight();
	_pixel_depth = (BYTE)GetBpp();
	_channels = (GetBpp()>8) ? (BYTE)3: (BYTE)1;
	_bit_depth = (BYTE)(GetBpp()/_channels);
	_compression_type = PNG_COMPRESSION_TYPE_DEFAULT;
	_filter_type = PNG_FILTER_TYPE_DEFAULT;
#else
	info_ptr->width = GetWidth();
	info_ptr->height = GetHeight();
	info_ptr->pixel_depth = (BYTE)GetBpp();
	info_ptr->channels = (GetBpp()>8) ? (BYTE)3: (BYTE)1;
	info_ptr->bit_depth = (BYTE)(GetBpp()/info_ptr->channels);
	info_ptr->compression_type = info_ptr->filter_type = 0;
	info_ptr->valid = 0;
#endif

	switch(GetCodecOption(CXIMAGE_FORMAT_PNG)){
	case 1:
#ifdef USE_NEW_LIBPNG_API
		_interlace_type = PNG_INTERLACE_ADAM7;
#else
		info_ptr->interlace_type = PNG_INTERLACE_ADAM7;
#endif
		break;
	default:
#ifdef USE_NEW_LIBPNG_API
		_interlace_type = PNG_INTERLACE_NONE;
#else
		info_ptr->interlace_type = PNG_INTERLACE_NONE;
#endif
	}

	/* set compression level */
	//png_set_compression_level(png_ptr, Z_BEST_COMPRESSION);

	bool bGrayScale = IsGrayScale();

	if (GetNumColors()){
		if (bGrayScale){
#ifdef USE_NEW_LIBPNG_API
			_color_type = PNG_COLOR_TYPE_GRAY;
#else
			info_ptr->color_type = PNG_COLOR_TYPE_GRAY;
#endif
		} else {
#ifdef USE_NEW_LIBPNG_API
			_color_type = PNG_COLOR_TYPE_PALETTE;
#else
			info_ptr->color_type = PNG_COLOR_TYPE_PALETTE;
#endif
		}
	} else {
#ifdef USE_NEW_LIBPNG_API
		_color_type = PNG_COLOR_TYPE_RGB;
#else
		info_ptr->color_type = PNG_COLOR_TYPE_RGB;
#endif
	}
#if CXIMAGE_SUPPORT_ALPHA
	if (AlphaIsValid()){
#ifdef USE_NEW_LIBPNG_API
		_color_type |= PNG_COLOR_MASK_ALPHA;
		_channels++;
		_bit_depth = 8;
		_pixel_depth += 8;
#else
		info_ptr->color_type |= PNG_COLOR_MASK_ALPHA;
		info_ptr->channels++;
		info_ptr->bit_depth = 8;
		info_ptr->pixel_depth += 8;
#endif
	}
#endif

	/* set background */
	png_color_16 image_background={ 0, 255, 255, 255, 0 };
	RGBQUAD tc = GetTransColor();
	if (info.nBkgndIndex>=0) {
		image_background.blue  = tc.rgbBlue;
		image_background.green = tc.rgbGreen;
		image_background.red   = tc.rgbRed;
	}
	png_set_bKGD(png_ptr, info_ptr, &image_background);

	/* set metrics */
	png_set_pHYs(png_ptr, info_ptr, head.biXPelsPerMeter, head.biYPelsPerMeter, PNG_RESOLUTION_METER);

#ifdef USE_NEW_LIBPNG_API
	png_set_IHDR(png_ptr,info_ptr,_width,_height,_bit_depth,_color_type,_interlace_type,
		_compression_type,_filter_type);
#else
	png_set_IHDR(png_ptr, info_ptr, info_ptr->width, info_ptr->height, info_ptr->bit_depth,
				info_ptr->color_type, info_ptr->interlace_type,
				PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
#endif

	//<DP> simple transparency
	if (info.nBkgndIndex >= 0){
#if PNG_LIBPNG_VER <= 10499
		info_ptr->num_trans = 1;
		info_ptr->valid |= PNG_INFO_tRNS;
#endif
#ifdef USE_NEW_LIBPNG_API
		png_color_16 _trans_color;
		_trans_color.index = (BYTE)info.nBkgndIndex;
		_trans_color.red   = tc.rgbRed;
		_trans_color.green = tc.rgbGreen;
		_trans_color.blue  = tc.rgbBlue;
		_trans_color.gray  = _trans_color.index;
		png_set_tRNS(png_ptr,info_ptr,(png_bytep)trans,1,&_trans_color);
#else
#if PNG_LIBPNG_VER > 10399
		info_ptr->trans_alpha = trans;
		info_ptr->trans_color.index = (BYTE)info.nBkgndIndex;
		info_ptr->trans_color.red   = tc.rgbRed;
		info_ptr->trans_color.green = tc.rgbGreen;
		info_ptr->trans_color.blue  = tc.rgbBlue;
		info_ptr->trans_color.gray  = info_ptr->trans_color.index;
#else
		info_ptr->trans = trans;
		info_ptr->trans_values.index = (BYTE)info.nBkgndIndex;
		info_ptr->trans_values.red   = tc.rgbRed;
		info_ptr->trans_values.green = tc.rgbGreen;
		info_ptr->trans_values.blue  = tc.rgbBlue;
		info_ptr->trans_values.gray  = info_ptr->trans_values.index;
#endif
#endif

		// the transparency indexes start from 0 for non grayscale palette
		if (!bGrayScale && head.biClrUsed && info.nBkgndIndex)
			SwapIndex(0,(BYTE)info.nBkgndIndex);
	}

	/* set the palette if there is one */
#ifdef USE_NEW_LIBPNG_API
	png_colorp _palette = NULL;
	if (GetPalette()){
		/* png_set_PLTE() will be called once the palette is ready */
#else
	if (GetPalette()){
		if (!bGrayScale){
			info_ptr->valid |= PNG_INFO_PLTE;
		}
#endif

		int nc = GetClrImportant();
		if (nc==0) nc = GetNumColors();

		if (info.bAlphaPaletteEnabled){
			for(WORD ip=0; ip<nc;ip++)
				trans[ip]=GetPaletteColor((BYTE)ip).rgbReserved;
#if PNG_LIBPNG_VER <= 10499
			info_ptr->num_trans = (WORD)nc;
			info_ptr->valid |= PNG_INFO_tRNS;
#endif
#ifdef USE_NEW_LIBPNG_API
			png_set_tRNS(png_ptr,info_ptr,(png_bytep)trans,nc,NULL);
#else
#if PNG_LIBPNG_VER > 10399
			info_ptr->trans_alpha = trans;
#else
			info_ptr->trans = trans;
#endif
#endif
		}

		// copy the palette colors
#ifdef USE_NEW_LIBPNG_API
		_palette = new png_color[nc];
		for (int i=0; i<nc; i++)
			GetPaletteColor(i, &_palette[i].red, &_palette[i].green, &_palette[i].blue);

		png_set_PLTE(png_ptr,info_ptr,_palette,nc);
#else
		info_ptr->palette = new png_color[nc];
		info_ptr->num_palette = (png_uint_16) nc;
		for (int i=0; i<nc; i++)
			GetPaletteColor(i, &info_ptr->palette[i].red, &info_ptr->palette[i].green, &info_ptr->palette[i].blue);
#endif
	}  

#if CXIMAGE_SUPPORT_ALPHA	// <vho>
	//Merge the transparent color with the alpha channel
	if (AlphaIsValid() && head.biBitCount==24 && info.nBkgndIndex>=0){
		for(long y=0; y < head.biHeight; y++){
			for(long x=0; x < head.biWidth ; x++){
				RGBQUAD c=GetPixelColor(x,y,false);
				if (*(long*)&c==*(long*)&tc)
					AlphaSet(x,y,0);
	}	}	}
#endif // CXIMAGE_SUPPORT_ALPHA	// <vho>

#ifdef USE_NEW_LIBPNG_API
	int row_size = max(info.dwEffWidth, (_width * _channels * _bit_depth / 8));
#else
	int row_size = max(info.dwEffWidth, info_ptr->width*info_ptr->channels*(info_ptr->bit_depth/8));
	info_ptr->rowbytes = row_size;
#endif
	BYTE *row_pointers = new BYTE[row_size];

	/* write the file information */
	png_write_info(png_ptr, info_ptr);

	//interlace handling
	int num_pass = png_set_interlace_handling(png_ptr);
	for (int pass = 0; pass < num_pass; pass++){
		//write image
		iter.Upset();
		long ay=head.biHeight-1;
		RGBQUAD c;
		do	{
#if CXIMAGE_SUPPORT_ALPHA	// <vho>
			if (AlphaIsValid()){
				for (long ax=head.biWidth-1; ax>=0;ax--){
					c = BlindGetPixelColor(ax,ay);
#ifdef USE_NEW_LIBPNG_API
					int px = ax * _channels;
#else
					int px = ax * info_ptr->channels;
#endif
					if (!bGrayScale){
						row_pointers[px++]=c.rgbRed;
						row_pointers[px++]=c.rgbGreen;
					}
					row_pointers[px++]=c.rgbBlue;
					row_pointers[px] = AlphaGet(ax,ay);
				}
				png_write_row(png_ptr, row_pointers);
				ay--;
			}
			else
#endif //CXIMAGE_SUPPORT_ALPHA	// <vho>
			{
				iter.GetRow(row_pointers, row_size);
#ifdef USE_NEW_LIBPNG_API
				if (_color_type == PNG_COLOR_TYPE_RGB) //HACK BY OP
#else
				if (info_ptr->color_type == PNG_COLOR_TYPE_RGB) //HACK BY OP
#endif
					RGBtoBGR(row_pointers, row_size);
				png_write_row(png_ptr, row_pointers);
			}
		} while(iter.PrevRow());
	}

	delete [] row_pointers;
	row_pointers = nullptr;

	//if necessary, restore the original palette
	if (!bGrayScale && head.biClrUsed && info.nBkgndIndex>0)
		SwapIndex((BYTE)info.nBkgndIndex,0);

	/* It is REQUIRED to call this to finish writing the rest of the file */
	png_write_end(png_ptr, info_ptr);

	/* if you malloced the palette, free it here */
#ifdef USE_NEW_LIBPNG_API
	if (_palette){
		delete [] (_palette);
#else
	if (info_ptr->palette){
		delete [] (info_ptr->palette);
		info_ptr->palette = NULL;
#endif
	}

	/* clean up after the write, and free any memory allocated */
	png_destroy_write_struct(&png_ptr, (png_infopp)&info_ptr);

  } cx_catch {
	if (strcmp(message,"")) strncpy(info.szLastError,message,255);
	return FALSE;
  }
	/* that's it */
	return TRUE;
}
////////////////////////////////////////////////////////////////////////////////
#endif // CXIMAGE_SUPPORT_ENCODE
////////////////////////////////////////////////////////////////////////////////
#endif // CXIMAGE_SUPPORT_PNG
