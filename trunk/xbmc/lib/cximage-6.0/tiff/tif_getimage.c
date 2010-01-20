/* $Header: /cvsroot/osrs/libtiff/libtiff/tif_getimage.c,v 1.15 2001/09/24 19:40:37 warmerda Exp $ */

/*
 * Copyright (c) 1991-1997 Sam Leffler
 * Copyright (c) 1991-1997 Silicon Graphics, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and 
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Sam Leffler and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Sam Leffler and Silicon Graphics.
 * 
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
 * 
 * IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
 * OF THIS SOFTWARE.
 */

/*
 * TIFF Library
 *
 * Read and return a packed RGBA image.
 */

typedef unsigned char byte;
typedef unsigned char BYTE;

#pragma warning (disable : 4550)

#include "tiffiop.h"
#include <assert.h>
#include <stdio.h>

static	int gtTileContig(TIFFRGBAImage*, uint32*, uint32, uint32);
static	int gtTileSeparate(TIFFRGBAImage*, uint32*, uint32, uint32);
static	int gtStripContig(TIFFRGBAImage*, uint32*, uint32, uint32);
static	int gtStripSeparate(TIFFRGBAImage*, uint32*, uint32, uint32);
static	int pickTileContigCase(TIFFRGBAImage*);
static	int pickTileSeparateCase(TIFFRGBAImage*);

static	const char photoTag[] = "PhotometricInterpretation";

/*
 * Check the image to see if TIFFReadRGBAImage can deal with it.
 * 1/0 is returned according to whether or not the image can
 * be handled.  If 0 is returned, emsg contains the reason
 * why it is being rejected.
 */
int
TIFFRGBAImageOK(TIFF* tif, char emsg[1024])
{
    TIFFDirectory* td = &tif->tif_dir;
    uint16 photometric;
    int colorchannels;

    switch (td->td_bitspersample) {
    case 1: case 2: case 4:
    case 8: case 16:
	break;
    default:
	sprintf(emsg, "Sorry, can not handle images with %d-bit samples",
	    td->td_bitspersample);
	return (0);
    }
    colorchannels = td->td_samplesperpixel - td->td_extrasamples;
    if (!TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &photometric)) {
	switch (colorchannels) {
	case 1:
	    photometric = PHOTOMETRIC_MINISBLACK;
	    break;
	case 3:
	    photometric = PHOTOMETRIC_RGB;
	    break;
	default:
	    sprintf(emsg, "Missing needed %s tag", photoTag);
	    return (0);
	}
    }
    switch (photometric) {
    case PHOTOMETRIC_MINISWHITE:
    case PHOTOMETRIC_MINISBLACK:
    case PHOTOMETRIC_PALETTE:
	if (td->td_planarconfig == PLANARCONFIG_CONTIG 
            && td->td_samplesperpixel != 1
            && td->td_bitspersample < 8 ) {
	    sprintf(emsg,
                    "Sorry, can not handle contiguous data with %s=%d, "
                    "and %s=%d and Bits/Sample=%d",
                    photoTag, photometric,
                    "Samples/pixel", td->td_samplesperpixel,
                    td->td_bitspersample);
	    return (0);
	}
        /*
        ** We should likely validate that any extra samples are either
        ** to be ignored, or are alpha, and if alpha we should try to use
        ** them.  But for now we won't bother with this. 
        */
	break;
    case PHOTOMETRIC_YCBCR:
	if (td->td_planarconfig != PLANARCONFIG_CONTIG) {
	    sprintf(emsg, "Sorry, can not handle YCbCr images with %s=%d",
		"Planarconfiguration", td->td_planarconfig);
	    return (0);
	}
	break;
    case PHOTOMETRIC_RGB: 
	if (colorchannels < 3) {
	    sprintf(emsg, "Sorry, can not handle RGB image with %s=%d",
		"Color channels", colorchannels);
	    return (0);
	}
	break;
#ifdef CMYK_SUPPORT
    case PHOTOMETRIC_SEPARATED:
	if (td->td_inkset != INKSET_CMYK) {
	    sprintf(emsg, "Sorry, can not handle separated image with %s=%d",
		"InkSet", td->td_inkset);
	    return (0);
	}
	if (td->td_samplesperpixel < 4) {
	    sprintf(emsg, "Sorry, can not handle separated image with %s=%d",
		"Samples/pixel", td->td_samplesperpixel);
	    return (0);
	}
	break;
#endif
    case PHOTOMETRIC_LOGL:
	if (td->td_compression != COMPRESSION_SGILOG) {
	    sprintf(emsg, "Sorry, LogL data must have %s=%d",
		"Compression", COMPRESSION_SGILOG);
	    return (0);
	}
	break;
    case PHOTOMETRIC_LOGLUV:
	if (td->td_compression != COMPRESSION_SGILOG &&
		td->td_compression != COMPRESSION_SGILOG24) {
	    sprintf(emsg, "Sorry, LogLuv data must have %s=%d or %d",
		"Compression", COMPRESSION_SGILOG, COMPRESSION_SGILOG24);
	    return (0);
	}
	if (td->td_planarconfig != PLANARCONFIG_CONTIG) {
	    sprintf(emsg, "Sorry, can not handle LogLuv images with %s=%d",
		"Planarconfiguration", td->td_planarconfig);
	    return (0);
	}
	break;
    default:
	sprintf(emsg, "Sorry, can not handle image with %s=%d",
	    photoTag, photometric);
	return (0);
    }
    return (1);
}

void
TIFFRGBAImageEnd(TIFFRGBAImage* img)
{
    if (img->Map)
	_TIFFfree(img->Map), img->Map = NULL;
    if (img->BWmap)
	_TIFFfree(img->BWmap), img->BWmap = NULL;
    if (img->PALmap)
	_TIFFfree(img->PALmap), img->PALmap = NULL;
    if (img->ycbcr)
	_TIFFfree(img->ycbcr), img->ycbcr = NULL;

    if( img->redcmap ) {
        _TIFFfree( img->redcmap );
        _TIFFfree( img->greencmap );
        _TIFFfree( img->bluecmap );
    }
}

static int
isCCITTCompression(TIFF* tif)
{
    uint16 compress;
    TIFFGetField(tif, TIFFTAG_COMPRESSION, &compress);
    return (compress == COMPRESSION_CCITTFAX3 ||
	    compress == COMPRESSION_CCITTFAX4 ||
	    compress == COMPRESSION_CCITTRLE ||
	    compress == COMPRESSION_CCITTRLEW);
}

int
TIFFRGBAImageBegin(TIFFRGBAImage* img, TIFF* tif, int stop, char emsg[1024])
{
    uint16* sampleinfo;
    uint16 extrasamples;
    uint16 planarconfig;
    uint16 compress;
    int colorchannels;
    uint16	*red_orig, *green_orig, *blue_orig;
    int		n_color;

    /* Initialize to normal values */
    img->row_offset = 0;
    img->col_offset = 0;
    img->redcmap = NULL;
    img->greencmap = NULL;
    img->bluecmap = NULL;
    
    img->tif = tif;
    img->stoponerr = stop;
    TIFFGetFieldDefaulted(tif, TIFFTAG_BITSPERSAMPLE, &img->bitspersample);
    /*switch (img->bitspersample) { // - VK -
    case 1: case 2: case 4:
    case 8: case 16:
	break;
    default:
	sprintf(emsg, "Sorry, can not image with %d-bit samples",
	    img->bitspersample);
	return (0);
    }*/
    img->alpha = 1;
    TIFFGetFieldDefaulted(tif, TIFFTAG_SAMPLESPERPIXEL, &img->samplesperpixel);
    TIFFGetFieldDefaulted(tif, TIFFTAG_EXTRASAMPLES,
	&extrasamples, &sampleinfo);
    if (extrasamples == 1)
	switch (sampleinfo[0]) {
	case EXTRASAMPLE_ASSOCALPHA:	/* data is pre-multiplied */
	case EXTRASAMPLE_UNASSALPHA:	/* data is not pre-multiplied */
	    img->alpha = sampleinfo[0];
	    break;
	}
    colorchannels = img->samplesperpixel - extrasamples;
    TIFFGetFieldDefaulted(tif, TIFFTAG_COMPRESSION, &compress);
    TIFFGetFieldDefaulted(tif, TIFFTAG_PLANARCONFIG, &planarconfig);
    if (!TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &img->photometric)) {
	switch (colorchannels) {
	case 1:
	    if (isCCITTCompression(tif))
		img->photometric = PHOTOMETRIC_MINISWHITE;
	    else
		img->photometric = PHOTOMETRIC_MINISBLACK;
	    break;
	case 3:
	    img->photometric = PHOTOMETRIC_RGB;
	    break;
	default:
	    sprintf(emsg, "Missing needed %s tag", photoTag);
	    return (0);
	}
    }
    switch (img->photometric) {
    case PHOTOMETRIC_PALETTE:
	if (!TIFFGetField(tif, TIFFTAG_COLORMAP,
	    &red_orig, &green_orig, &blue_orig)) {
	    TIFFError(TIFFFileName(tif), "Missing required \"Colormap\" tag");
	    return (0);
	}

        /* copy the colormaps so we can modify them */
        n_color = (1L << img->bitspersample);
        img->redcmap = (uint16 *) _TIFFmalloc(sizeof(uint16)*n_color);
        img->greencmap = (uint16 *) _TIFFmalloc(sizeof(uint16)*n_color);
        img->bluecmap = (uint16 *) _TIFFmalloc(sizeof(uint16)*n_color);
        if( !img->redcmap || !img->greencmap || !img->bluecmap ) {
	    TIFFError(TIFFFileName(tif), "Out of memory for colormap copy");
	    return (0);
        }

        memcpy( img->redcmap, red_orig, n_color * 2 );
        memcpy( img->greencmap, green_orig, n_color * 2 );
        memcpy( img->bluecmap, blue_orig, n_color * 2 );
        
	/* fall thru... */
    case PHOTOMETRIC_MINISWHITE:
    case PHOTOMETRIC_MINISBLACK:
	if (planarconfig == PLANARCONFIG_CONTIG 
            && img->samplesperpixel != 1
            && img->bitspersample < 8 ) {
	    sprintf(emsg,
                    "Sorry, can not handle contiguous data with %s=%d, "
                    "and %s=%d and Bits/Sample=%d",
                    photoTag, img->photometric,
                    "Samples/pixel", img->samplesperpixel,
                    img->bitspersample);
	    return (0);
	}
	break;
    case PHOTOMETRIC_YCBCR:
	if (planarconfig != PLANARCONFIG_CONTIG) {
	    sprintf(emsg, "Sorry, can not handle YCbCr images with %s=%d",
		"Planarconfiguration", planarconfig);
	    return (0);
	}
	/* It would probably be nice to have a reality check here. */
	if (planarconfig == PLANARCONFIG_CONTIG)
	    /* can rely on libjpeg to convert to RGB */
	    /* XXX should restore current state on exit */
	    switch (compress) {
		case COMPRESSION_OJPEG:
		case COMPRESSION_JPEG:
		    TIFFSetField(tif, TIFFTAG_JPEGCOLORMODE, JPEGCOLORMODE_RGB);
		    img->photometric = PHOTOMETRIC_RGB;
                    break;

                default:
                    /* do nothing */;
                    break;
	    }
	break;
    case PHOTOMETRIC_RGB: 
	if (colorchannels < 3) {
	    sprintf(emsg, "Sorry, can not handle RGB image with %s=%d",
		"Color channels", colorchannels);
	    return (0);
	}
	break;
    case PHOTOMETRIC_SEPARATED: {
	uint16 inkset;
	TIFFGetFieldDefaulted(tif, TIFFTAG_INKSET, &inkset);
	if (inkset != INKSET_CMYK) {
	    sprintf(emsg, "Sorry, can not handle separated image with %s=%d",
		"InkSet", inkset);
	    return (0);
	}
	if (img->samplesperpixel < 4) {
	    sprintf(emsg, "Sorry, can not handle separated image with %s=%d",
		"Samples/pixel", img->samplesperpixel);
	    return (0);
	}
	break;
    }
    case PHOTOMETRIC_LOGL:
	if (compress != COMPRESSION_SGILOG) {
	    sprintf(emsg, "Sorry, LogL data must have %s=%d",
		"Compression", COMPRESSION_SGILOG);
	    return (0);
	}
	TIFFSetField(tif, TIFFTAG_SGILOGDATAFMT, SGILOGDATAFMT_8BIT);
	img->photometric = PHOTOMETRIC_MINISBLACK;	/* little white lie */
	img->bitspersample = 8;
	break;
    case PHOTOMETRIC_LOGLUV:
	if (compress != COMPRESSION_SGILOG && compress != COMPRESSION_SGILOG24) {
	    sprintf(emsg, "Sorry, LogLuv data must have %s=%d or %d",
		"Compression", COMPRESSION_SGILOG, COMPRESSION_SGILOG24);
	    return (0);
	}
	if (planarconfig != PLANARCONFIG_CONTIG) {
	    sprintf(emsg, "Sorry, can not handle LogLuv images with %s=%d",
		"Planarconfiguration", planarconfig);
	    return (0);
	}
	TIFFSetField(tif, TIFFTAG_SGILOGDATAFMT, SGILOGDATAFMT_8BIT);
	img->photometric = PHOTOMETRIC_RGB;		/* little white lie */
	img->bitspersample = 8;
	break;
    default:
	sprintf(emsg, "Sorry, can not handle image with %s=%d",
	    photoTag, img->photometric);
	return (0);
    }
    img->Map = NULL;
    img->BWmap = NULL;
    img->PALmap = NULL;
    img->ycbcr = NULL;
    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &img->width);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &img->height);
    TIFFGetFieldDefaulted(tif, TIFFTAG_ORIENTATION, &img->orientation);
    img->isContig =
	!(planarconfig == PLANARCONFIG_SEPARATE && colorchannels > 1);
    if (img->isContig) {
	img->get = TIFFIsTiled(tif) ? gtTileContig : gtStripContig;
	(void) pickTileContigCase(img);
    } else {
	img->get = TIFFIsTiled(tif) ? gtTileSeparate : gtStripSeparate;
	(void) pickTileSeparateCase(img);
    }
    return (1);
}

int
TIFFRGBAImageGet(TIFFRGBAImage* img, uint32* raster, uint32 w, uint32 h)
{
    if (img->get == NULL) {
	TIFFError(TIFFFileName(img->tif), "No \"get\" routine setup");
	return (0);
    }
    if (img->put.any == NULL) {
	TIFFError(TIFFFileName(img->tif),
	    "No \"put\" routine setupl; probably can not handle image format");
	return (0);
    }
    return (*img->get)(img, raster, w, h);
}

/*
 * Read the specified image into an ABGR-format raster.
 */
int
TIFFReadRGBAImage(TIFF* tif,
    uint32 rwidth, uint32 rheight, uint32* raster, int stop)
{
    char emsg[1024];
    TIFFRGBAImage img;
    int ok;

    if (TIFFRGBAImageBegin(&img, tif, stop, emsg)) {
	/* XXX verify rwidth and rheight against width and height */
	ok = TIFFRGBAImageGet(&img, raster+(rheight-img.height)*rwidth,
	    rwidth, img.height);
	TIFFRGBAImageEnd(&img);
    } else {
	TIFFError(TIFFFileName(tif), emsg);
	ok = 0;
    }
    return (ok);
}

static uint32
setorientation(TIFFRGBAImage* img, uint32 h)
{
    TIFF* tif = img->tif;
    uint32 y;

    switch (img->orientation) {
    case ORIENTATION_BOTRIGHT:
    case ORIENTATION_RIGHTBOT:	/* XXX */
    case ORIENTATION_LEFTBOT:	/* XXX */
	TIFFWarning(TIFFFileName(tif), "using bottom-left orientation");
	img->orientation = ORIENTATION_BOTLEFT;
	/* fall thru... */
    case ORIENTATION_BOTLEFT:
	y = 0;
	break;
    case ORIENTATION_TOPRIGHT:
    case ORIENTATION_RIGHTTOP:	/* XXX */
    case ORIENTATION_LEFTTOP:	/* XXX */
    default:
	TIFFWarning(TIFFFileName(tif), "using top-left orientation");
	img->orientation = ORIENTATION_TOPLEFT;
	/* fall thru... */
    case ORIENTATION_TOPLEFT:
	y = h-1;
	break;
    }
    return (y);
}

/*
 * Get an tile-organized image that has
 *	PlanarConfiguration contiguous if SamplesPerPixel > 1
 * or
 *	SamplesPerPixel == 1
 */	
static int
gtTileContig(TIFFRGBAImage* img, uint32* raster, uint32 w, uint32 h)
{
    TIFF* tif = img->tif;
    tileContigRoutine put = img->put.contig;
    uint16 orientation;
    uint32 col, row, y, rowstoread, ret = 1;
    uint32 pos;
    uint32 tw, th;
    u_char* buf;
    int32 fromskew, toskew;
    uint32 nrow;
 
    buf = (u_char*) _TIFFmalloc(TIFFTileSize(tif));
    if (buf == 0) {
	TIFFError(TIFFFileName(tif), "No space for tile buffer");
	return (0);
    }
    TIFFGetField(tif, TIFFTAG_TILEWIDTH, &tw);
    TIFFGetField(tif, TIFFTAG_TILELENGTH, &th);
    y = setorientation(img, h);
    orientation = img->orientation;
    toskew = -(int32) (orientation == ORIENTATION_TOPLEFT ? tw+w : tw-w);
    for (row = 0; row < h; row += nrow) 
    {
        rowstoread = th - (row + img->row_offset) % th;
    	nrow = (row + rowstoread > h ? h - row : rowstoread);
	for (col = 0; col < w; col += tw) 
        {
            if (TIFFReadTile(tif, buf, col+img->col_offset,
                             row+img->row_offset, 0, 0) < 0 && img->stoponerr)
            {
                ret = 0;
                break;
            }

            pos = ((row+img->row_offset) % th) * TIFFTileRowSize(tif);

    	    if (col + tw > w) 
            {
                /*
                 * Tile is clipped horizontally.  Calculate
                 * visible portion and skewing factors.
                 */
                uint32 npix = w - col;
                fromskew = tw - npix;
                (*put)(img, raster+y*w+col, col, y,
                       npix, nrow, fromskew, toskew + fromskew, buf + pos);
            }
            else 
            {
                (*put)(img, raster+y*w+col, col, y, tw, nrow, 0, toskew, buf + pos);
            }
        }

        y += (orientation == ORIENTATION_TOPLEFT ? -(int32) nrow : (int32) nrow);
    }
    _TIFFfree(buf);
    return (ret);
}

/*
 * Get an tile-organized image that has
 *	 SamplesPerPixel > 1
 *	 PlanarConfiguration separated
 * We assume that all such images are RGB.
 */	
static int
gtTileSeparate(TIFFRGBAImage* img, uint32* raster, uint32 w, uint32 h)
{
    TIFF* tif = img->tif;
    tileSeparateRoutine put = img->put.separate;
    uint16 orientation;
    uint32 col, row, y, rowstoread;
    uint32 pos;
    uint32 tw, th;
    u_char* buf;
    u_char* r;
    u_char* g;
    u_char* b;
    u_char* a;
    tsize_t tilesize;
    int32 fromskew, toskew;
    int alpha = img->alpha;
    uint32 nrow;
    int ret = 1;

    tilesize = TIFFTileSize(tif);
    buf = (u_char*) _TIFFmalloc(4*tilesize);
    if (buf == 0) {
	TIFFError(TIFFFileName(tif), "No space for tile buffer");
	return (0);
    }
    r = buf;
    g = r + tilesize;
    b = g + tilesize;
    a = b + tilesize;
    if (!alpha)
	memset(a, 0xff, tilesize);
    TIFFGetField(tif, TIFFTAG_TILEWIDTH, &tw);
    TIFFGetField(tif, TIFFTAG_TILELENGTH, &th);
    y = setorientation(img, h);
    orientation = img->orientation;
    toskew = -(int32) (orientation == ORIENTATION_TOPLEFT ? tw+w : tw-w);
    for (row = 0; row < h; row += nrow) 
    {
        rowstoread = th - (row + img->row_offset) % th;
    	nrow = (row + rowstoread > h ? h - row : rowstoread);
        for (col = 0; col < w; col += tw) 
        {
            if (TIFFReadTile(tif, r, col+img->col_offset,
                             row+img->row_offset,0,0) < 0 && img->stoponerr)
            {
                ret = 0;
                break;
            }
            if (TIFFReadTile(tif, g, col+img->col_offset,
                             row+img->row_offset,0,1) < 0 && img->stoponerr)
            {
                ret = 0;
                break;
            }
            if (TIFFReadTile(tif, b, col+img->col_offset,
                             row+img->row_offset,0,2) < 0 && img->stoponerr)
            {
                ret = 0;
                break;
            }
            if (alpha && TIFFReadTile(tif,a,col+img->col_offset,
                                      row+img->row_offset,0,3) < 0 && img->stoponerr)
            {
                ret = 0;
                break;
            }

            pos = ((row+img->row_offset) % th) * TIFFTileRowSize(tif);

            if (col + tw > w) 
            {
                /*
                 * Tile is clipped horizontally.  Calculate
                 * visible portion and skewing factors.
                 */
                uint32 npix = w - col;
                fromskew = tw - npix;
                (*put)(img, raster+y*w+col, col, y,
                       npix, nrow, fromskew, toskew + fromskew, 
                       r + pos, g + pos, b + pos, a + pos);
            } 
            else 
            {
                (*put)(img, raster+y*w+col, col, y,
                       tw, nrow, 0, toskew, r + pos, g + pos, b + pos, a + pos);
            }
        }

        y += (orientation == ORIENTATION_TOPLEFT ?-(int32) nrow : (int32) nrow);
    }
    _TIFFfree(buf);
    return (ret);
}

/*
 * Get a strip-organized image that has
 *	PlanarConfiguration contiguous if SamplesPerPixel > 1
 * or
 *	SamplesPerPixel == 1
 */	
static int
gtStripContig(TIFFRGBAImage* img, uint32* raster, uint32 w, uint32 h)
{
    TIFF* tif = img->tif;
    tileContigRoutine put = img->put.contig;
    uint16 orientation;
    uint32 row, y, nrow, rowstoread;
    uint32 pos;
    u_char* buf;
    uint32 rowsperstrip;
    uint32 imagewidth = img->width;
    tsize_t scanline;
    int32 fromskew, toskew;
    int ret = 1;

    buf = (u_char*) _TIFFmalloc(TIFFStripSize(tif));
    if (buf == 0) {
	TIFFError(TIFFFileName(tif), "No space for strip buffer");
	return (0);
    }
    y = setorientation(img, h);
    orientation = img->orientation;
    toskew = -(int32) (orientation == ORIENTATION_TOPLEFT ? w+w : w-w);
    TIFFGetFieldDefaulted(tif, TIFFTAG_ROWSPERSTRIP, &rowsperstrip);
    scanline = TIFFScanlineSize(tif);
    fromskew = (w < imagewidth ? imagewidth - w : 0);
    for (row = 0; row < h; row += nrow) 
    {
        rowstoread = rowsperstrip - (row + img->row_offset) % rowsperstrip;
        nrow = (row + rowstoread > h ? h - row : rowstoread);
        if (TIFFReadEncodedStrip(tif,
                                 TIFFComputeStrip(tif,row+img->row_offset, 0),
                                 buf, 
                                 ((row + img->row_offset)%rowsperstrip + nrow) * scanline) < 0
            && img->stoponerr)
        {
            ret = 0;
            break;
        }

        pos = ((row + img->row_offset) % rowsperstrip) * scanline;
        (*put)(img, raster+y*w, 0, y, w, nrow, fromskew, toskew, buf + pos);
        y += (orientation == ORIENTATION_TOPLEFT ?-(int32) nrow : (int32) nrow);
    }
    _TIFFfree(buf);
    return (ret);
}

/*
 * Get a strip-organized image with
 *	 SamplesPerPixel > 1
 *	 PlanarConfiguration separated
 * We assume that all such images are RGB.
 */
static int
gtStripSeparate(TIFFRGBAImage* img, uint32* raster, uint32 w, uint32 h)
{
    TIFF* tif = img->tif;
    tileSeparateRoutine put = img->put.separate;
    uint16 orientation;
    u_char *buf;
    u_char *r, *g, *b, *a;
    uint32 row, y, nrow, rowstoread;
    uint32 pos;
    tsize_t scanline;
    uint32 rowsperstrip, offset_row;
    uint32 imagewidth = img->width;
    tsize_t stripsize;
    int32 fromskew, toskew;
    int alpha = img->alpha;
    int	ret = 1;

    stripsize = TIFFStripSize(tif);
    r = buf = (u_char *)_TIFFmalloc(4*stripsize);
    if (buf == 0) {
	TIFFError(TIFFFileName(tif), "No space for tile buffer");
	return (0);
    }
    g = r + stripsize;
    b = g + stripsize;
    a = b + stripsize;
    if (!alpha)
	memset(a, 0xff, stripsize);
    y = setorientation(img, h);
    orientation = img->orientation;
    toskew = -(int32) (orientation == ORIENTATION_TOPLEFT ? w+w : w-w);
    TIFFGetFieldDefaulted(tif, TIFFTAG_ROWSPERSTRIP, &rowsperstrip);
    scanline = TIFFScanlineSize(tif);
    fromskew = (w < imagewidth ? imagewidth - w : 0);
    for (row = 0; row < h; row += nrow) 
    {
        rowstoread = rowsperstrip - (row + img->row_offset) % rowsperstrip;    	
        nrow = (row + rowstoread > h ? h - row : rowstoread);
        offset_row = row + img->row_offset;
    	if (TIFFReadEncodedStrip(tif, TIFFComputeStrip(tif, offset_row, 0),
                                 r, ((row + img->row_offset)%rowsperstrip + nrow) * scanline) < 0 
            && img->stoponerr)
        {
            ret = 0;
            break;
        }
        if (TIFFReadEncodedStrip(tif, TIFFComputeStrip(tif, offset_row, 1),
                                 g, ((row + img->row_offset)%rowsperstrip + nrow) * scanline) < 0 
            && img->stoponerr)
        {
            ret = 0;
            break;
        }
        if (TIFFReadEncodedStrip(tif, TIFFComputeStrip(tif, offset_row, 2),
                                 b, ((row + img->row_offset)%rowsperstrip + nrow) * scanline) < 0 
            && img->stoponerr)
        {
            ret = 0;
            break;
        }
        if (alpha &&
            (TIFFReadEncodedStrip(tif, TIFFComputeStrip(tif, offset_row, 3),
                                  a, ((row + img->row_offset)%rowsperstrip + nrow) * scanline) < 0 
             && img->stoponerr))
        {
            ret = 0;
            break;
        }

        pos = ((row + img->row_offset) % rowsperstrip) * scanline;
        (*put)(img, raster+y*w, 0, y, w, nrow, fromskew, toskew, r + pos, g + pos, 
               b + pos, a + pos);
        y += (orientation == ORIENTATION_TOPLEFT ? -(int32) nrow : (int32) nrow);
    }
    _TIFFfree(buf);
    return (ret);
}

/*
 * The following routines move decoded data returned
 * from the TIFF library into rasters filled with packed
 * ABGR pixels (i.e. suitable for passing to lrecwrite.)
 *
 * The routines have been created according to the most
 * important cases and optimized.  pickTileContigCase and
 * pickTileSeparateCase analyze the parameters and select
 * the appropriate "put" routine to use.
 */
#define	REPEAT8(op)	REPEAT4(op); REPEAT4(op)
#define	REPEAT4(op)	REPEAT2(op); REPEAT2(op)
#define	REPEAT2(op)	op; op
#define	CASE8(x,op)			\
    switch (x) {			\
    case 7: op; case 6: op; case 5: op;	\
    case 4: op; case 3: op; case 2: op;	\
    case 1: op;				\
    }
#define	CASE4(x,op)	switch (x) { case 3: op; case 2: op; case 1: op; }
#define	NOP

#define	UNROLL8(w, op1, op2) {		\
    uint32 _x;				\
    for (_x = w; _x >= 8; _x -= 8) {	\
	op1;				\
	REPEAT8(op2);			\
    }					\
    if (_x > 0) {			\
	op1;				\
	CASE8(_x,op2);			\
    }					\
}
#define	UNROLL4(w, op1, op2) {		\
    uint32 _x;				\
    for (_x = w; _x >= 4; _x -= 4) {	\
	op1;				\
	REPEAT4(op2);			\
    }					\
    if (_x > 0) {			\
	op1;				\
	CASE4(_x,op2);			\
    }					\
}
#define	UNROLL2(w, op1, op2) {		\
    uint32 _x;				\
    for (_x = w; _x >= 2; _x -= 2) {	\
	op1;				\
	REPEAT2(op2);			\
    }					\
    if (_x) {				\
	op1;				\
	op2;				\
    }					\
}
    
#define	SKEW(r,g,b,skew)	{ r += skew; g += skew; b += skew; }
#define	SKEW4(r,g,b,a,skew)	{ r += skew; g += skew; b += skew; a+= skew; }

#define A1 ((uint32)(0xffL<<24))
#define	PACK(r,g,b)	\
	((uint32)(r)|((uint32)(g)<<8)|((uint32)(b)<<16)|A1)
#define	PACK4(r,g,b,a)	\
	((uint32)(r)|((uint32)(g)<<8)|((uint32)(b)<<16)|((uint32)(a)<<24))
#define W2B(v) (((v)>>8)&0xff)
#define	PACKW(r,g,b)	\
	((uint32)W2B(r)|((uint32)W2B(g)<<8)|((uint32)W2B(b)<<16)|A1)
#define	PACKW4(r,g,b,a)	\
	((uint32)W2B(r)|((uint32)W2B(g)<<8)|((uint32)W2B(b)<<16)|((uint32)W2B(a)<<24))

#define	DECLAREContigPutFunc(name) \
static void name(\
    TIFFRGBAImage* img, \
    uint32* cp, \
    uint32 x, uint32 y, \
    uint32 w, uint32 h, \
    int32 fromskew, int32 toskew, \
    u_char* pp \
)

/*
 * 8-bit palette => colormap/RGB
 */
DECLAREContigPutFunc(put8bitcmaptile)
{
    uint32** PALmap = img->PALmap;
    int samplesperpixel = img->samplesperpixel;

    (void) y;
    while (h-- > 0) {
	for (x = w; x-- > 0;)
        {
	    *cp++ = PALmap[*pp][0];
            pp += samplesperpixel;
        }
	cp += toskew;
	pp += fromskew;
    }
}

/*
 * 4-bit palette => colormap/RGB
 */
DECLAREContigPutFunc(put4bitcmaptile)
{
    uint32** PALmap = img->PALmap;

    (void) x; (void) y;
    fromskew /= 2;
    while (h-- > 0) {
	uint32* bw;
	UNROLL2(w, bw = PALmap[*pp++], *cp++ = *bw++);
	cp += toskew;
	pp += fromskew;
    }
}

/*
 * 2-bit palette => colormap/RGB
 */
DECLAREContigPutFunc(put2bitcmaptile)
{
    uint32** PALmap = img->PALmap;

    (void) x; (void) y;
    fromskew /= 4;
    while (h-- > 0) {
	uint32* bw;
	UNROLL4(w, bw = PALmap[*pp++], *cp++ = *bw++);
	cp += toskew;
	pp += fromskew;
    }
}

/*
 * 1-bit palette => colormap/RGB
 */
DECLAREContigPutFunc(put1bitcmaptile)
{
    uint32** PALmap = img->PALmap;

    (void) x; (void) y;
    fromskew /= 8;
    while (h-- > 0) {
	uint32* bw;
	UNROLL8(w, bw = PALmap[*pp++], *cp++ = *bw++);
	cp += toskew;
	pp += fromskew;
    }
}

/*
 * 8-bit greyscale => colormap/RGB
 */
DECLAREContigPutFunc(putgreytile)
{
    int samplesperpixel = img->samplesperpixel;
    uint32** BWmap = img->BWmap;

    (void) y;
    while (h-- > 0) {
	for (x = w; x-- > 0;)
        {
	    *cp++ = BWmap[*pp][0];
            pp += samplesperpixel;
        }
	cp += toskew;
	pp += fromskew;
    }
}

/*
 * 16-bit greyscale => colormap/RGB
 */
DECLAREContigPutFunc(put16bitbwtile)
{
    int samplesperpixel = img->samplesperpixel;
    uint32** BWmap = img->BWmap;

    (void) y;
    while (h-- > 0) {
        uint16 *wp = (uint16 *) pp;

	for (x = w; x-- > 0;)
        {
            /* use high order byte of 16bit value */

	    *cp++ = BWmap[*wp >> 8][0];
            pp += 2 * samplesperpixel;
            wp += samplesperpixel;
        }
	cp += toskew;
	pp += fromskew;
    }
}

/*
 * 1-bit bilevel => colormap/RGB
 */
DECLAREContigPutFunc(put1bitbwtile)
{
    uint32** BWmap = img->BWmap;

    (void) x; (void) y;
    fromskew /= 8;
    while (h-- > 0) {
	uint32* bw;
	UNROLL8(w, bw = BWmap[*pp++], *cp++ = *bw++);
	cp += toskew;
	pp += fromskew;
    }
}

/*
 * 2-bit greyscale => colormap/RGB
 */
DECLAREContigPutFunc(put2bitbwtile)
{
    uint32** BWmap = img->BWmap;

    (void) x; (void) y;
    fromskew /= 4;
    while (h-- > 0) {
	uint32* bw;
	UNROLL4(w, bw = BWmap[*pp++], *cp++ = *bw++);
	cp += toskew;
	pp += fromskew;
    }
}

/*
 * 4-bit greyscale => colormap/RGB
 */
DECLAREContigPutFunc(put4bitbwtile)
{
    uint32** BWmap = img->BWmap;

    (void) x; (void) y;
    fromskew /= 2;
    while (h-- > 0) {
	uint32* bw;
	UNROLL2(w, bw = BWmap[*pp++], *cp++ = *bw++);
	cp += toskew;
	pp += fromskew;
    }
}

/* + VK
 * 2-bit packed samples, no Map => RGB
 */
DECLAREContigPutFunc(putRGBcontig2bittile)
{
	uint32 d;
	byte r, g, b;
    int samplesperpixel = img->samplesperpixel;
	int _x;
	BYTE *from;

/*#define put_rgb(r,g,b) *cp++ = PACK( (r<<6), (g<<6), (b<<6) );	\
	if(--_x <= 0) { cp += toskew; _x = w; }*/
#define put_rgb(r,g,b) *cp++ = PACK( (r<<6), (g<<6), (b<<6) );

    (void) x; (void) y;
	if (samplesperpixel == 3) {
		while (h-- > 0) {
			from = pp;
			for (_x=w; _x > 0; _x -= 4) {
				d = *(uint32*)from;
				from += 3;
					r = (BYTE)( d & 3 ); g = (BYTE)( (d >> 2) & 3 ); b = (BYTE)( (d >> 4) & 3 );
					put_rgb( r, g, b );
				if (_x > 1) {
					r = (BYTE)( (d>>6) & 3 ); g = (BYTE)( (d >> 8) & 3 ); b = (BYTE)( (d >> 10) & 3 );
					put_rgb( r, g, b );
				}
				if (_x > 2) {
					r = (BYTE)( (d>>12) & 3 ); g = (BYTE)( (d >> 14) & 3 ); b = (BYTE)( (d >> 16) & 3 );
					put_rgb( r, g, b );
				}
				if (_x > 3) {
					r = (BYTE)( (d>>18) & 3 ); g = (BYTE)( (d >> 20) & 3 ); b = (BYTE)( (d >> 22) & 3 );
					put_rgb( r, g, b );
				}
			}
			cp += toskew;
			pp += (w * samplesperpixel * 2 + 7)/8 + fromskew;
		}
	} else if (samplesperpixel == 4) { // VK: not tested!
		while (h-- > 0) {
			for (_x = w; _x >= 0; _x --) {
				r = *pp & 3; g = (*pp >> 2) & 3; b = (*pp >> 4) & 3;
				*cp++ = PACK( r<<6, g<<6, b<<6 );
				pp ++;
			}
			cp += toskew;
			pp += fromskew;
		}
	}
}

/* + VK
 * 2-bit packed samples, w/ Map => RGB
 */
DECLAREContigPutFunc(putRGBcontig2bitMaptile)
{
	uint32 d;
	byte r, g, b;
    int samplesperpixel = img->samplesperpixel;
	int _x;
	BYTE *from;
	TIFFRGBValue* Map = img->Map;

/*#define put_rgb(r,g,b) *cp++ = PACK( (r<<6), (g<<6), (b<<6) );	\
	if(--_x <= 0) { cp += toskew; _x = w; }*/
#define put_rgb2map(r,g,b) *cp++ = PACK( Map[g], Map[r], Map[b] );

    (void) x; (void) y;
	if (samplesperpixel == 3) {
		while (h-- > 0) {
			from = pp;
			for (_x=w; _x > 0; _x -= 4) {
				d = *(uint32*)from;
				from += 3;
					r = (BYTE)( (d>>6) & 3 ); g = (BYTE)( (d >> 4) & 3 ); b = (BYTE)( (d >> 2) & 3 );
					put_rgb2map( r, g, b );
				if (_x > 1) {
					r = (BYTE)( (d>>0) & 3 ); g = (BYTE)( (d >> 14) & 3 ); b = (BYTE)( (d >> 12) & 3 );
					put_rgb2map( r, g, b );
				}
				if (_x > 2) {
					r = (BYTE)( (d>>10) & 3 ); g = (BYTE)( (d >> 8) & 3 ); b = (BYTE)( (d >> 22) & 3 );
					put_rgb2map( r, g, b );
				}
				if (_x > 3) {
					r = (BYTE)( (d>>20) & 3 ); g = (BYTE)( (d >> 18) & 3 ); b = (BYTE)( (d >> 16) & 3 );
					put_rgb2map( r, g, b );
				}
			}
			cp += toskew;
			pp += (w * samplesperpixel * 2 + 7)/8 + fromskew;
		}
	} else if (samplesperpixel == 4) { // VK: not tested!
		while (h-- > 0) {
			for (_x = w; _x >= 0; _x --) {
				r = *pp & 3; g = (*pp >> 2) & 3; b = (*pp >> 4) & 3;
				*cp++ = PACK( r<<6, g<<6, b<<6 );
				pp ++;
			}
			cp += toskew;
			pp += fromskew;
		}
	}
}

/* + VK
 * 4-bit packed samples, no Map => RGB
 */
DECLAREContigPutFunc(putRGBcontig4bittile)
{
	uint32 d;
	byte r, g, b;
    int samplesperpixel = img->samplesperpixel;
	int _x;
	BYTE *from;

/*#define put_rgb(r,g,b) *cp++ = PACK( (r<<6), (g<<6), (b<<6) );	\
	if(--_x <= 0) { cp += toskew; _x = w; }*/
#define put_rgb4(r,g,b) *cp++ = PACK( ((r<<4)|r), ((g<<4)|g), ((b<<4)|b) );

    (void) x; (void) y;
	if (samplesperpixel == 3) {
		while (h-- > 0) {
			from = pp;
			for (_x=w; _x > 0; _x -= 2) {
				d = *(uint32*)from;
				from += 3;
					g = (BYTE)( d & 15 ); b = (BYTE)( (d >> 4) & 15 ); r = (BYTE)( (d >> 12) & 15 );
					put_rgb4( r, g, b );
				if (_x > 1) {
					b = (BYTE)( (d>>8) & 15 ); r = (BYTE)( (d >> 16) & 15 ); g = (BYTE)( (d >> 20) & 15 );
					put_rgb4( r, g, b );
				}
			}
			cp += toskew;
			pp += (w * samplesperpixel * 4 + 7)/8 + fromskew;
		}
	} else if (samplesperpixel == 4) { // VK: not tested!
		while (h-- > 0) {
			for (_x = w; _x >= 0; _x --) {
				g = *pp & 15; r = (*pp++ >> 4) & 15; b = (*pp++ >> 4) & 15;
				*cp++ = PACK( r<<4, g<<4, b<<4 );
			}
			cp += toskew;
			pp += fromskew;
		}
	}
}

/* + VK
 * 4-bit packed samples, w/ Map => RGB
 */
DECLAREContigPutFunc(putRGBcontig4bitMaptile)
{
	uint32 d;
	byte r, g, b;
    int samplesperpixel = img->samplesperpixel;
	int _x;
	BYTE *from;
	TIFFRGBValue* Map = img->Map;

/*#define put_rgb(r,g,b) *cp++ = PACK( (r<<6), (g<<6), (b<<6) );	\
	if(--_x <= 0) { cp += toskew; _x = w; }*/
#define put_rgb4map(r,g,b) *cp++ = PACK( Map[r], Map[g], Map[b] );

    (void) x; (void) y;
	if (samplesperpixel == 3) {
		while (h-- > 0) {
			from = pp;
			for (_x=w; _x > 0; _x -= 2) {
				d = *(uint32*)from;
				from += 3;
					g = (BYTE)( d & 15 ); r = (BYTE)( (d >> 4) & 15 ); b = (BYTE)( (d >> 12) & 15 );
					put_rgb4map( r, g, b );
				if (_x > 1) {
					r = (BYTE)( (d>>8) & 15 ); b = (BYTE)( (d >> 16) & 15 ); g = (BYTE)( (d >> 20) & 15 );
					put_rgb4map( r, g, b );
				}
			}
			cp += toskew;
			pp += (w * samplesperpixel * 4 + 7)/8 + fromskew;
		}
	} else if (samplesperpixel == 4) { // VK: not tested!
		while (h-- > 0) {
			for (_x = w; _x >= 0; _x --) {
				g = *pp & 15; r = (*pp++ >> 4) & 15; b = (*pp++ >> 4) & 15;
				*cp++ = PACK(Map[r<<4], Map[g<<4], Map[b<<4] );
			}
			cp += toskew;
			pp += fromskew;
		}
	}
}

/* + VK
 * 10-bit packed samples, no Map => RGB
 */
DECLAREContigPutFunc(putRGBcontig10bittile)
{
	uint32 d;
	uint16 r, g, b;
    int samplesperpixel = img->samplesperpixel;
	int _x;
	BYTE *from;

#define put_rgb10(r,g,b) *cp++ = PACK( (BYTE)(r>>2), (BYTE)(g>>2), (BYTE)(b>>2) );

    (void) x; (void) y;
	if (samplesperpixel == 3) {
		while (h-- > 0) {
			from = pp;
			for (_x=w; _x > 0; _x -= 2) {
				d = *(uint32*)from;
				from += 3;
					g = (BYTE)( d & 15 ); b = (BYTE)( (d >> 4) & 15 ); r = (BYTE)( (d >> 12) & 15 );
					put_rgb4( r, g, b );
				if (_x > 1) {
					b = (BYTE)( (d>>8) & 15 ); r = (BYTE)( (d >> 16) & 15 ); g = (BYTE)( (d >> 20) & 15 );
					put_rgb4( r, g, b );
				}
			}
			cp += toskew;
			pp += (w * samplesperpixel * 4 + 7)/8 + fromskew;
		}
	} else if (samplesperpixel == 4) { // VK: not tested!
		while (h-- > 0) {
			for (_x = w; _x >= 0; _x --) {
				g = *pp & 15; r = (*pp++ >> 4) & 15; b = (*pp++ >> 4) & 15;
				*cp++ = PACK( r<<4, g<<4, b<<4 );
			}
			cp += toskew;
			pp += fromskew;
		}
	}
}
/*
 * 8-bit packed samples, no Map => RGB
 */
DECLAREContigPutFunc(putRGBcontig8bittile)
{
    int samplesperpixel = img->samplesperpixel;

    (void) x; (void) y;
    fromskew *= samplesperpixel;
    while (h-- > 0) {
	UNROLL8(w, NOP,
	    *cp++ = PACK(pp[0], pp[1], pp[2]);
	    pp += samplesperpixel);
	cp += toskew;
	pp += fromskew;
    }
}

/*
 * 8-bit packed samples, w/ Map => RGB
 */
DECLAREContigPutFunc(putRGBcontig8bitMaptile)
{
    TIFFRGBValue* Map = img->Map;
    int samplesperpixel = img->samplesperpixel;

    (void) y;
    fromskew *= samplesperpixel;
    while (h-- > 0) {
	for (x = w; x-- > 0;) {
	    *cp++ = PACK(Map[pp[0]], Map[pp[1]], Map[pp[2]]);
	    pp += samplesperpixel;
	}
	pp += fromskew;
	cp += toskew;
    }
}

/*
 * 8-bit packed samples => RGBA w/ associated alpha
 * (known to have Map == NULL)
 */
DECLAREContigPutFunc(putRGBAAcontig8bittile)
{
    int samplesperpixel = img->samplesperpixel;

    (void) x; (void) y;
    fromskew *= samplesperpixel;
    while (h-- > 0) {
	UNROLL8(w, NOP,
	    *cp++ = PACK4(pp[0], pp[1], pp[2], pp[3]);
	    pp += samplesperpixel);
	cp += toskew;
	pp += fromskew;
    }
}

/*
 * 8-bit packed samples => RGBA w/ unassociated alpha
 * (known to have Map == NULL)
 */
DECLAREContigPutFunc(putRGBUAcontig8bittile)
{
    int samplesperpixel = img->samplesperpixel;

    (void) y;
    fromskew *= samplesperpixel;
    while (h-- > 0) {
	uint32 r, g, b, a;
	for (x = w; x-- > 0;) {
	    a = pp[3];
	    r = (pp[0] * a) / 255;
	    g = (pp[1] * a) / 255;
	    b = (pp[2] * a) / 255;
	    *cp++ = PACK4(r,g,b,a);
	    pp += samplesperpixel;
	}
	cp += toskew;
	pp += fromskew;
    }
}

/*
 * 16-bit packed samples => RGB
 */
DECLAREContigPutFunc(putRGBcontig16bittile)
{
    int samplesperpixel = img->samplesperpixel;
    uint16 *wp = (uint16 *)pp;

    (void) y;
    fromskew *= samplesperpixel;
    while (h-- > 0) {
	for (x = w; x-- > 0;) {
	    *cp++ = PACKW(wp[0], wp[1], wp[2]);
	    wp += samplesperpixel;
	}
	cp += toskew;
	wp += fromskew;
    }
}

/* + VK
 * n-bit packed samples => RGB
 */
DECLAREContigPutFunc(putRGBcontigAllbittile)
{
    int samplesperpixel = img->samplesperpixel;
	int bpp = img->bitspersample;
	BYTE r, g, b;
	BYTE * pp1;
    uint16 *wp = (uint16 *)pp;
	uint32 d;

    (void) y;
    fromskew *= samplesperpixel;

	if (bpp <= 8) {
		pp1 = pp;
		while (h-- > 0) {
			int offbits = 0;
			pp = pp1;
			for (x = w; x-- > 0;) {
				d = (*pp << 24) | (pp[1] << 16) | (pp[2] << 8) | pp[3];
				r = (BYTE) ( d >> (24 - offbits) );
				offbits += bpp;
				g = (BYTE) ( d >> (24 - offbits) );
				offbits += bpp;
				b = (BYTE) ( d >> (24 - offbits) );
				offbits += bpp;
				*cp++ = PACK(r, g, b);
				while (offbits >= 8) {
					pp++;
					offbits -= 8;
				}
			}
			cp += toskew;
			pp1 += (bpp * samplesperpixel * w + 7)/8 + fromskew;
		}
	} else if (bpp <= 24) {
		pp1 = pp;
		while (h-- > 0) {
			int offbits = 0;
			pp = pp1;
			for (x = w; x-- > 0;) {
				d = (*pp << 24) | (pp[1] << 16) | (pp[2] << 8) | pp[3];
				r = (BYTE) ( d >> (24 - offbits) );
				offbits += bpp;
				while (offbits >= 8) {
					pp++;
					offbits -= 8;
				}
				d = (*pp << 24) | (pp[1] << 16) | (pp[2] << 8) | pp[3];
				g = (BYTE) ( d >> (24 - offbits) );
				offbits += bpp;
				while (offbits >= 8) {
					pp++;
					offbits -= 8;
				}
				d = (*pp << 24) | (pp[1] << 16) | (pp[2] << 8) | pp[3];
				b = (BYTE) ( d >> (24 - offbits) );
				offbits += bpp;
				while (offbits >= 8) {
					pp++;
					offbits -= 8;
				}
				*cp++ = PACK(r, g, b);
			}
			cp += toskew;
			pp1 += (bpp * samplesperpixel * w + 7)/8 + fromskew;
		}
	} else {
		pp1 = pp;
		while (h-- > 0) {
			int offbits = 0;
			pp = pp1;
			for (x = w; x-- > 0;) {
				d = *(uint32*)pp;
				r = (BYTE) ( d >> (24 - offbits) );
				offbits += bpp;
				while (offbits >= 8) {
					pp++;
					offbits -= 8;
				}
				d = *(uint32*)pp;
				g = (BYTE) ( d >> (24 - offbits) );
				offbits += bpp;
				while (offbits >= 8) {
					pp++;
					offbits -= 8;
				}
				d = *(uint32*)pp;
				b = (BYTE) ( d >> (24 - offbits) );
				offbits += bpp;
				while (offbits >= 8) {
					pp++;
					offbits -= 8;
				}
				*cp++ = PACK(r, g, b);
			}
			cp += toskew;
			pp1 += (bpp * samplesperpixel * w + 7)/8 + fromskew;
		}
	}
}

/* + VK 
 * 16-bit packed CMYK samples => RGB 
 */
DECLAREContigPutFunc(put_CMYKmap_contig16bittile)
{
    int samplesperpixel = img->samplesperpixel;
	TIFFRGBValue* Map = img->Map; // for Map only
    uint16 *wp = (uint16 *)pp;
	uint32 r, g, b, k;

    (void) y;
    fromskew *= samplesperpixel;
    while (h-- > 0) {
	for (x = w; x-- > 0;) {
	    k = 65535 - wp[3];
	    r = (k*(65535-wp[0]))/65535;
	    g = (k*(65535-wp[1]))/65535;
	    b = (k*(65535-wp[2]))/65535;

		*cp++ = PACK(Map[r>>8], Map[g>>8], Map[b>>8]);
				//PACKW(r, g, b);
	    wp += samplesperpixel;
	}
	cp += toskew;
	wp += fromskew;
    }
}

/* + VK - not tested? see above (put_CMYKmap_contig16bittile)
 * 16-bit packed CMYK samples => RGB // + VK
 */
DECLAREContigPutFunc(put_CMYK_contig16bittile)
{
    int samplesperpixel = img->samplesperpixel;
    uint16 *wp = (uint16 *)pp;
	uint16 r, g, b, k;

    (void) y;
    fromskew *= samplesperpixel;
    while (h-- > 0) {
	for (x = w; x-- > 0;) {
	    k = 65535 - wp[3];
	    r = (k*(65535-wp[0]))/65535;
	    g = (k*(65535-wp[1]))/65535;
	    b = (k*(65535-wp[2]))/65535;

		*cp++ = PACKW(r, g, b);
	    wp += samplesperpixel;
	}
	cp += toskew;
	wp += fromskew;
    }
}

/*
 * 16-bit packed samples => RGBA w/ associated alpha
 * (known to have Map == NULL)
 */
DECLAREContigPutFunc(putRGBAAcontig16bittile)
{
    int samplesperpixel = img->samplesperpixel;
    uint16 *wp = (uint16 *)pp;

    (void) y;
    fromskew *= samplesperpixel;
    while (h-- > 0) {
	for (x = w; x-- > 0;) {
	    *cp++ = PACKW4(wp[0], wp[1], wp[2], wp[3]);
	    wp += samplesperpixel;
	}
	cp += toskew;
	wp += fromskew;
    }
}

/*
 * 16-bit packed samples => RGBA w/ unassociated alpha
 * (known to have Map == NULL)
 */
DECLAREContigPutFunc(putRGBUAcontig16bittile)
{
    int samplesperpixel = img->samplesperpixel;
    uint16 *wp = (uint16 *)pp;

    (void) y;
    fromskew *= samplesperpixel;
    while (h-- > 0) {
	uint32 r,g,b,a;
	/*
	 * We shift alpha down four bits just in case unsigned
	 * arithmetic doesn't handle the full range.
	 * We still have plenty of accuracy, since the output is 8 bits.
	 * So we have (r * 0xffff) * (a * 0xfff)) = r*a * (0xffff*0xfff)
	 * Since we want r*a * 0xff for eight bit output,
	 * we divide by (0xffff * 0xfff) / 0xff == 0x10eff.
	 */
	for (x = w; x-- > 0;) {
	    a = wp[3] >> 4; 
	    r = (wp[0] * a) / 0x10eff;
	    g = (wp[1] * a) / 0x10eff;
	    b = (wp[2] * a) / 0x10eff;
	    *cp++ = PACK4(r,g,b,a);
	    wp += samplesperpixel;
	}
	cp += toskew;
	wp += fromskew;
    }
}

/*
 * 8-bit packed CMYK samples w/o Map => RGB
 *
 * NB: The conversion of CMYK->RGB is *very* crude.
 */
DECLAREContigPutFunc(putRGBcontig8bitCMYKtile)
{
    int samplesperpixel = img->samplesperpixel;
    uint16 r, g, b, k;

    (void) x; (void) y;
    fromskew *= samplesperpixel;
    while (h-- > 0) {
	UNROLL8(w, NOP,
	    k = 255 - pp[3];
	    r = (k*(255-pp[0]))/255;
	    g = (k*(255-pp[1]))/255;
	    b = (k*(255-pp[2]))/255;
	    *cp++ = PACK(r, g, b);
	    pp += samplesperpixel);
	cp += toskew;
	pp += fromskew;
    }
}

/*
 * 8-bit packed CMYK samples w/Map => RGB
 *
 * NB: The conversion of CMYK->RGB is *very* crude.
 */
DECLAREContigPutFunc(putRGBcontig8bitCMYKMaptile)
{
    int samplesperpixel = img->samplesperpixel;
    TIFFRGBValue* Map = img->Map;
    uint16 r, g, b, k;

    (void) y;
    fromskew *= samplesperpixel;
    while (h-- > 0) {
	for (x = w; x-- > 0;) {
	    k = 255 - pp[3];
	    r = (k*(255-pp[0]))/255;
	    g = (k*(255-pp[1]))/255;
	    b = (k*(255-pp[2]))/255;
	    *cp++ = PACK(Map[r], Map[g], Map[b]);
	    pp += samplesperpixel;
	}
	pp += fromskew;
	cp += toskew;
    }
}

#define	DECLARESepPutFunc(name) \
static void name(\
    TIFFRGBAImage* img,\
    uint32* cp,\
    uint32 x, uint32 y, \
    uint32 w, uint32 h,\
    int32 fromskew, int32 toskew,\
    u_char* r, u_char* g, u_char* b, u_char* a\
)

/*
 * 8-bit unpacked samples => RGB
 */
DECLARESepPutFunc(putRGBseparate8bittile)
{
    (void) img; (void) x; (void) y; (void) a;
    while (h-- > 0) {
	UNROLL8(w, NOP, *cp++ = PACK(*r++, *g++, *b++));
	SKEW(r, g, b, fromskew);
	cp += toskew;
    }
}

/*
 * 8-bit unpacked samples => RGB
 */
DECLARESepPutFunc(putRGBseparate8bitMaptile)
{
    TIFFRGBValue* Map = img->Map;

    (void) y; (void) a;
    while (h-- > 0) {
	for (x = w; x > 0; x--)
	    *cp++ = PACK(Map[*r++], Map[*g++], Map[*b++]);
	SKEW(r, g, b, fromskew);
	cp += toskew;
    }
}

/*
 * 8-bit unpacked samples => RGBA w/ associated alpha
 */
DECLARESepPutFunc(putRGBAAseparate8bittile)
{
    (void) img; (void) x; (void) y;
    while (h-- > 0) {
	UNROLL8(w, NOP, *cp++ = PACK4(*r++, *g++, *b++, *a++));
	SKEW4(r, g, b, a, fromskew);
	cp += toskew;
    }
}

/*
 * 8-bit unpacked samples => RGBA w/ unassociated alpha
 */
DECLARESepPutFunc(putRGBUAseparate8bittile)
{
    (void) img; (void) y;
    while (h-- > 0) {
	uint32 rv, gv, bv, av;
	for (x = w; x-- > 0;) {
	    av = *a++;
	    rv = (*r++ * av) / 255;
	    gv = (*g++ * av) / 255;
	    bv = (*b++ * av) / 255;
	    *cp++ = PACK4(rv,gv,bv,av);
	}
	SKEW4(r, g, b, a, fromskew);
	cp += toskew;
    }
}

/*
 * 16-bit unpacked samples => RGB
 */
DECLARESepPutFunc(putRGBseparate16bittile)
{
    uint16 *wr = (uint16*) r;
    uint16 *wg = (uint16*) g;
    uint16 *wb = (uint16*) b;

    (void) img; (void) y; (void) a;
    while (h-- > 0) {
	for (x = 0; x < w; x++)
	    *cp++ = PACKW(*wr++, *wg++, *wb++);
	SKEW(wr, wg, wb, fromskew);
	cp += toskew;
    }
}

/* + VK
 * n-bit packed samples => RGB
 */
DECLARESepPutFunc(putRGBseparateAllbittile)
{
	int bpp = img->bitspersample;
    BYTE *pr = (BYTE*) r;
    BYTE *pg = (BYTE*) g;
    BYTE *pb = (BYTE*) b;
	BYTE *pr1 = pr;
	BYTE *pg1 = pg;
	BYTE *pb1 = pb;
	uint32 dr, dg, db;
	BYTE rr, gg, bb;

    (void) img; (void) y; (void) a;
	if (bpp <= 24) {
		while (h-- > 0) {
			int offbits = 0;
			pr = pr1; pg = pg1; pb = pb1;
			for (x = 0; x < w; x++) {
				dr = (*pr << 24)|(pr[1]<<16)|(pr[2]<<8)|pr[3];
				dg = (*pg << 24)|(pg[1]<<16)|(pg[2]<<8)|pg[3];
				db = (*pb << 24)|(pb[1]<<16)|(pb[2]<<8)|pb[3];
				rr = (BYTE) (dr >> (24 - offbits));
				gg = (BYTE) (dg >> (24 - offbits));
				bb = (BYTE) (db >> (24 - offbits));
				*cp++ = PACK(rr, gg, bb);
				offbits += bpp;
				while (offbits >= 8) {
					pr++; pg++; pb++;
					offbits -= 8;
				}
			}
			pr1 += (bpp * w + 7)/8 + fromskew;
			pg1 += (bpp * w + 7)/8 + fromskew;
			pb1 += (bpp * w + 7)/8 + fromskew;
			cp += toskew;
		}
	} else if (bpp <= 32) {
		while (h-- > 0) {
			int offbits = 0;
			pr = pr1; pg = pg1; pb = pb1;
			for (x = 0; x < w; x++) {
				dr = *(uint32*)pr;
				dg = *(uint32*)pg;
				db = *(uint32*)pb;
				rr = (BYTE) (dr >> (24 - offbits));
				gg = (BYTE) (dg >> (24 - offbits));
				bb = (BYTE) (db >> (24 - offbits));
				*cp++ = PACK(rr, gg, bb);
				offbits += bpp;
				while (offbits >= 8) {
					pr++; pg++; pb++;
					offbits -= 8;
				}
			}
			pr1 += (bpp * w + 7)/8 + fromskew;
			pg1 += (bpp * w + 7)/8 + fromskew;
			pb1 += (bpp * w + 7)/8 + fromskew;
			cp += toskew;
		}
	} else { // rare case, when bpp > 32
		while (h-- > 0) {
			int offbits = 0;
			pr = pr1; pg = pg1; pb = pb1;
			for (x = 0; x < w; x++) {
#define SWAW( x ) x = (x>>16) | (x<<16)
				dr = *(uint32*)(pr + bpp/8 - 4); SWAW( dr ); 
				dg = *(uint32*)(pg + bpp/8 - 4); SWAW( dg ); 
				db = *(uint32*)(pb + bpp/8 - 4); SWAW( db ); 
				rr = (BYTE) (dr >> offbits);
				gg = (BYTE) (dg >> offbits);
				bb = (BYTE) (db >> offbits);
				*cp++ = PACK(rr, gg, bb);
				offbits += bpp;
				while (offbits >= 8) {
					pr++; pg++; pb++;
					offbits -= 8;
				}
			}
			pr1 += (bpp * w + 7)/8 + fromskew;
			pg1 += (bpp * w + 7)/8 + fromskew;
			pb1 += (bpp * w + 7)/8 + fromskew;
			cp += toskew;
		}
	}
}

/* + VK
 * n-bit packed samples (planar) => RGB
 */
DECLARESepPutFunc(putRGBplanarseparateAllbittile)
{
	int bpp = img->bitspersample;
    BYTE *pr = (BYTE*) r;
    BYTE *pg = (BYTE*) g;
    BYTE *pb = (BYTE*) b;
	BYTE *pr1 = pr;
	BYTE *pg1 = pg;
	BYTE *pb1 = pb;
	uint32 dr, dg, db;
	BYTE rr, gg, bb;

    (void) img; (void) y; (void) a;
	if (bpp <= 8) {
		while (h-- > 0) {
			int offbits = 0;
			pr = pr1; pg = pg1; pb = pb1;
			for (x = 0; x < w; x++) {
				dr = *(uint32*)pr;
				dg = *(uint32*)pg;
				db = *(uint32*)pb;
				/*dr = (*pr << 24)|(pr[1]<<16)|(pr[2]<<8)|pr[3];
				dg = (*pg << 24)|(pg[1]<<16)|(pg[2]<<8)|pg[3];
				db = (*pb << 24)|(pb[1]<<16)|(pb[2]<<8)|pb[3];*/
				rr = (BYTE) ((dr >> (32 - offbits - offbits)) << (8 - bpp)) ^ 0xFF;
				gg = (BYTE) ((dg >> (32 - offbits - offbits)) << (8 - bpp)) ^ 0xFF;
				bb = (BYTE) ((db >> (32 - offbits - offbits)) << (8 - bpp)) ^ 0xFF;
				*cp++ = PACK(rr, gg, bb);
				offbits += bpp;
				while (offbits >= 8) {
					pr++; pg++; pb++;
					offbits -= 8;
				}
			}
			pr1 += (bpp * w + 7)/8 + fromskew;
			pg1 += (bpp * w + 7)/8 + fromskew;
			pb1 += (bpp * w + 7)/8 + fromskew;
			cp += toskew;
		}
	} else { 
		while (h-- > 0) {
			int offbits = 0;
			pr = pr1; pg = pg1; pb = pb1;
			for (x = 0; x < w; x++) {
				dr = *(uint32*)pr;
				dg = *(uint32*)pg;
				db = *(uint32*)pb;
				/*dr = (*pr << 24)|(pr[1]<<16)|(pr[2]<<8)|pr[3];
				dg = (*pg << 24)|(pg[1]<<16)|(pg[2]<<8)|pg[3];
				db = (*pb << 24)|(pb[1]<<16)|(pb[2]<<8)|pb[3];*/
				rr = (BYTE) (dr >> (offbits + bpp - 8)) ^ 0xFF;
				gg = (BYTE) (dg >> (offbits + bpp - 8)) ^ 0xFF;
				bb = (BYTE) (db >> (offbits + bpp - 8)) ^ 0xFF;
				*cp++ = PACK(rr, gg, bb);
				offbits += bpp;
				while (offbits >= 8) {
					pr++; pg++; pb++;
					offbits -= 8;
				}
			}
			pr1 += (bpp * w + 7)/8 + fromskew;
			pg1 += (bpp * w + 7)/8 + fromskew;
			pb1 += (bpp * w + 7)/8 + fromskew;
			cp += toskew;
		}
	} 
}

/*
 * 16-bit unpacked samples => RGBA w/ associated alpha
 */
DECLARESepPutFunc(putRGBAAseparate16bittile)
{
    uint16 *wr = (uint16*) r;
    uint16 *wg = (uint16*) g;
    uint16 *wb = (uint16*) b;
    uint16 *wa = (uint16*) a;

    (void) img; (void) y;
    while (h-- > 0) {
	for (x = 0; x < w; x++)
	    *cp++ = PACKW4(*wr++, *wg++, *wb++, *wa++);
	SKEW4(wr, wg, wb, wa, fromskew);
	cp += toskew;
    }
}

/*
 * 16-bit unpacked samples => RGBA w/ unassociated alpha
 */
DECLARESepPutFunc(putRGBUAseparate16bittile)
{
    uint16 *wr = (uint16*) r;
    uint16 *wg = (uint16*) g;
    uint16 *wb = (uint16*) b;
    uint16 *wa = (uint16*) a;

    (void) img; (void) y;
    while (h-- > 0) {
	uint32 r,g,b,a;
	/*
	 * We shift alpha down four bits just in case unsigned
	 * arithmetic doesn't handle the full range.
	 * We still have plenty of accuracy, since the output is 8 bits.
	 * So we have (r * 0xffff) * (a * 0xfff)) = r*a * (0xffff*0xfff)
	 * Since we want r*a * 0xff for eight bit output,
	 * we divide by (0xffff * 0xfff) / 0xff == 0x10eff.
	 */
	for (x = w; x-- > 0;) {
	    a = *wa++ >> 4; 
	    r = (*wr++ * a) / 0x10eff;
	    g = (*wg++ * a) / 0x10eff;
	    b = (*wb++ * a) / 0x10eff;
	    *cp++ = PACK4(r,g,b,a);
	}
	SKEW4(wr, wg, wb, wa, fromskew);
	cp += toskew;
    }
}

/*
 * YCbCr -> RGB conversion and packing routines.  The colorspace
 * conversion algorithm comes from the IJG v5a code; see below
 * for more information on how it works.
 */

#define	YCbCrtoRGB(dst, yc) {						\
    int Y = (yc);							\
    dst = PACK(								\
	clamptab[Y+Crrtab[Cr]],						\
	clamptab[Y + (int)((Cbgtab[Cb]+Crgtab[Cr])>>16)],		\
	clamptab[Y+Cbbtab[Cb]]);					\
}
#define	YCbCrSetup							\
    TIFFYCbCrToRGB* ycbcr = img->ycbcr;					\
    int* Crrtab = ycbcr->Cr_r_tab;					\
    int* Cbbtab = ycbcr->Cb_b_tab;					\
    int32* Crgtab = ycbcr->Cr_g_tab;					\
    int32* Cbgtab = ycbcr->Cb_g_tab;					\
    TIFFRGBValue* clamptab = ycbcr->clamptab

/*
 * 8-bit packed YCbCr samples => RGB 
 * This function is generic for different sampling sizes, 
 * and can handle blocks sizes that aren't multiples of the
 * sampling size.  However, it is substantially less optimized
 * than the specific sampling cases.  It is used as a fallback
 * for difficult blocks.
 */
#ifdef notdef
static void putcontig8bitYCbCrGenericTile( 
    TIFFRGBAImage* img, 
    uint32* cp, 
    uint32 x, uint32 y, 
    uint32 w, uint32 h, 
    int32 fromskew, int32 toskew, 
    u_char* pp,
    int h_group, 
    int v_group )

{
    YCbCrSetup;
    
    uint32* cp1 = cp+w+toskew;
    uint32* cp2 = cp1+w+toskew;
    uint32* cp3 = cp2+w+toskew;
    int32 incr = 3*w+4*toskew;
    int     Cb, Cr;
    int     group_size = v_group * h_group + 2;

    (void) y;
    fromskew = (fromskew * group_size) / h_group;

    for( yy = 0; yy < h; yy++ )
    {
        u_char *pp_line;
        int     y_line_group = yy / v_group;
        int     y_remainder = yy - y_line_group * v_group;

        pp_line = pp + v_line_group * 

        
        for( xx = 0; xx < w; xx++ )
        {
            Cb = pp
        }
    }
    for (; h >= 4; h -= 4) {
	x = w>>2;
	do {
	    Cb = pp[16];
	    Cr = pp[17];

	    YCbCrtoRGB(cp [0], pp[ 0]);
	    YCbCrtoRGB(cp [1], pp[ 1]);
	    YCbCrtoRGB(cp [2], pp[ 2]);
	    YCbCrtoRGB(cp [3], pp[ 3]);
	    YCbCrtoRGB(cp1[0], pp[ 4]);
	    YCbCrtoRGB(cp1[1], pp[ 5]);
	    YCbCrtoRGB(cp1[2], pp[ 6]);
	    YCbCrtoRGB(cp1[3], pp[ 7]);
	    YCbCrtoRGB(cp2[0], pp[ 8]);
	    YCbCrtoRGB(cp2[1], pp[ 9]);
	    YCbCrtoRGB(cp2[2], pp[10]);
	    YCbCrtoRGB(cp2[3], pp[11]);
	    YCbCrtoRGB(cp3[0], pp[12]);
	    YCbCrtoRGB(cp3[1], pp[13]);
	    YCbCrtoRGB(cp3[2], pp[14]);
	    YCbCrtoRGB(cp3[3], pp[15]);

	    cp += 4, cp1 += 4, cp2 += 4, cp3 += 4;
	    pp += 18;
	} while (--x);
	cp += incr, cp1 += incr, cp2 += incr, cp3 += incr;
	pp += fromskew;
    }
}
#endif

/*
 * 8-bit packed YCbCr samples w/ 4,4 subsampling => RGB
 */
DECLAREContigPutFunc(putcontig8bitYCbCr44tile)
{
    YCbCrSetup;
    uint32* cp1 = cp+w+toskew;
    uint32* cp2 = cp1+w+toskew;
    uint32* cp3 = cp2+w+toskew;
    int32 incr = 3*w+4*toskew;

    (void) y;
    /* adjust fromskew */
    fromskew = (fromskew * 18) / 4;
    if ((h & 3) == 0 && (w & 3) == 0) {				        
        for (; h >= 4; h -= 4) {
            x = w>>2;
            do {
                int Cb = pp[16];
                int Cr = pp[17];

                YCbCrtoRGB(cp [0], pp[ 0]);
                YCbCrtoRGB(cp [1], pp[ 1]);
                YCbCrtoRGB(cp [2], pp[ 2]);
                YCbCrtoRGB(cp [3], pp[ 3]);
                YCbCrtoRGB(cp1[0], pp[ 4]);
                YCbCrtoRGB(cp1[1], pp[ 5]);
                YCbCrtoRGB(cp1[2], pp[ 6]);
                YCbCrtoRGB(cp1[3], pp[ 7]);
                YCbCrtoRGB(cp2[0], pp[ 8]);
                YCbCrtoRGB(cp2[1], pp[ 9]);
                YCbCrtoRGB(cp2[2], pp[10]);
                YCbCrtoRGB(cp2[3], pp[11]);
                YCbCrtoRGB(cp3[0], pp[12]);
                YCbCrtoRGB(cp3[1], pp[13]);
                YCbCrtoRGB(cp3[2], pp[14]);
                YCbCrtoRGB(cp3[3], pp[15]);

                cp += 4, cp1 += 4, cp2 += 4, cp3 += 4;
                pp += 18;
            } while (--x);
            cp += incr, cp1 += incr, cp2 += incr, cp3 += incr;
            pp += fromskew;
        }
    } else {
        while (h > 0) {
            for (x = w; x > 0;) {
                int Cb = pp[16];
                int Cr = pp[17];
                switch (x) {
                default:
                    switch (h) {
                    default: YCbCrtoRGB(cp3[3], pp[15]); /* FALLTHROUGH */
                    case 3:  YCbCrtoRGB(cp2[3], pp[11]); /* FALLTHROUGH */
                    case 2:  YCbCrtoRGB(cp1[3], pp[ 7]); /* FALLTHROUGH */
                    case 1:  YCbCrtoRGB(cp [3], pp[ 3]); /* FALLTHROUGH */
                    }                                    /* FALLTHROUGH */
                case 3:
                    switch (h) {
                    default: YCbCrtoRGB(cp3[2], pp[14]); /* FALLTHROUGH */
                    case 3:  YCbCrtoRGB(cp2[2], pp[10]); /* FALLTHROUGH */
                    case 2:  YCbCrtoRGB(cp1[2], pp[ 6]); /* FALLTHROUGH */
                    case 1:  YCbCrtoRGB(cp [2], pp[ 2]); /* FALLTHROUGH */
                    }                                    /* FALLTHROUGH */
                case 2:
                    switch (h) {
                    default: YCbCrtoRGB(cp3[1], pp[13]); /* FALLTHROUGH */
                    case 3:  YCbCrtoRGB(cp2[1], pp[ 9]); /* FALLTHROUGH */
                    case 2:  YCbCrtoRGB(cp1[1], pp[ 5]); /* FALLTHROUGH */
                    case 1:  YCbCrtoRGB(cp [1], pp[ 1]); /* FALLTHROUGH */
                    }                                    /* FALLTHROUGH */
                case 1:
                    switch (h) {
                    default: YCbCrtoRGB(cp3[0], pp[12]); /* FALLTHROUGH */
                    case 3:  YCbCrtoRGB(cp2[0], pp[ 8]); /* FALLTHROUGH */
                    case 2:  YCbCrtoRGB(cp1[0], pp[ 4]); /* FALLTHROUGH */
                    case 1:  YCbCrtoRGB(cp [0], pp[ 0]); /* FALLTHROUGH */
                    }                                    /* FALLTHROUGH */
                }
                if (x < 4) {
                    cp += x; cp1 += x; cp2 += x; cp3 += x;
                    x = 0;
                }
                else {
                    cp += 4; cp1 += 4; cp2 += 4; cp3 += 4;
                    x -= 4;
                }
                pp += 18;
            }
            if (h <= 4)
                break;
            h -= 4;
            cp += incr, cp1 += incr, cp2 += incr, cp3 += incr;
            pp += fromskew;
        }
    }
}

/*
 * 8-bit packed YCbCr samples w/ 4,2 subsampling => RGB
 */
DECLAREContigPutFunc(putcontig8bitYCbCr42tile)
{
    YCbCrSetup;
    uint32* cp1 = cp+w+toskew;
    int32 incr = 2*toskew+w;

    (void) y;
    fromskew = (fromskew * 10) / 4;
    if ((h & 3) == 0 && (w & 1) == 0) {
        for (; h >= 2; h -= 2) {
            x = w>>2;
            do {
                int Cb = pp[8];
                int Cr = pp[9];
                
                YCbCrtoRGB(cp [0], pp[0]);
                YCbCrtoRGB(cp [1], pp[1]);
                YCbCrtoRGB(cp [2], pp[2]);
                YCbCrtoRGB(cp [3], pp[3]);
                YCbCrtoRGB(cp1[0], pp[4]);
                YCbCrtoRGB(cp1[1], pp[5]);
                YCbCrtoRGB(cp1[2], pp[6]);
                YCbCrtoRGB(cp1[3], pp[7]);
                
                cp += 4, cp1 += 4;
                pp += 10;
            } while (--x);
            cp += incr, cp1 += incr;
            pp += fromskew;
        }
    } else {
        while (h > 0) {
            for (x = w; x > 0;) {
                int Cb = pp[8];
                int Cr = pp[9];
                switch (x) {
                default:
                    switch (h) {
                    default: YCbCrtoRGB(cp1[3], pp[ 7]); /* FALLTHROUGH */
                    case 1:  YCbCrtoRGB(cp [3], pp[ 3]); /* FALLTHROUGH */
                    }                                    /* FALLTHROUGH */
                case 3:
                    switch (h) {
                    default: YCbCrtoRGB(cp1[2], pp[ 6]); /* FALLTHROUGH */
                    case 1:  YCbCrtoRGB(cp [2], pp[ 2]); /* FALLTHROUGH */
                    }                                    /* FALLTHROUGH */
                case 2:
                    switch (h) {
                    default: YCbCrtoRGB(cp1[1], pp[ 5]); /* FALLTHROUGH */
                    case 1:  YCbCrtoRGB(cp [1], pp[ 1]); /* FALLTHROUGH */
                    }                                    /* FALLTHROUGH */
                case 1:
                    switch (h) {
                    default: YCbCrtoRGB(cp1[0], pp[ 4]); /* FALLTHROUGH */
                    case 1:  YCbCrtoRGB(cp [0], pp[ 0]); /* FALLTHROUGH */
                    }                                    /* FALLTHROUGH */
                }
                if (x < 4) {
                    cp += x; cp1 += x;
                    x = 0;
                }
                else {
                    cp += 4; cp1 += 4;
                    x -= 4;
                }
                pp += 10;
            }
            if (h <= 2)
                break;
            h -= 2;
            cp += incr, cp1 += incr;
            pp += fromskew;
        }
    }
}

/*
 * 8-bit packed YCbCr samples w/ 4,1 subsampling => RGB
 */
DECLAREContigPutFunc(putcontig8bitYCbCr41tile)
{
    YCbCrSetup;

    (void) y;
    /* XXX adjust fromskew */
    do {
	x = w>>2;
	do {
	    int Cb = pp[4];
	    int Cr = pp[5];

	    YCbCrtoRGB(cp [0], pp[0]);
	    YCbCrtoRGB(cp [1], pp[1]);
	    YCbCrtoRGB(cp [2], pp[2]);
	    YCbCrtoRGB(cp [3], pp[3]);

	    cp += 4;
	    pp += 6;
	} while (--x);

        if( (w&3) != 0 )
        {
	    int Cb = pp[4];
	    int Cr = pp[5];

            switch( (w&3) ) {
              case 3: YCbCrtoRGB(cp [2], pp[2]);
              case 2: YCbCrtoRGB(cp [1], pp[1]);
              case 1: YCbCrtoRGB(cp [0], pp[0]);
              case 0: break;
            }

            cp += (w&3);
            pp += 6;
        }

	cp += toskew;
	pp += fromskew;
    } while (--h);

}

/*
 * 8-bit packed YCbCr samples w/ 2,2 subsampling => RGB
 */
DECLAREContigPutFunc(putcontig8bitYCbCr22tile)
{
    YCbCrSetup;
    uint32* cp1 = cp+w+toskew;
    int32 incr = 2*toskew+w;

    (void) y;
    fromskew = (fromskew * 6) / 2;
    if ((h & 1) == 0 && (w & 1) == 0) {
        for (; h >= 2; h -= 2) {
            x = w>>1;
            do {
                int Cb = pp[4];
                int Cr = pp[5];

                YCbCrtoRGB(cp [0], pp[0]);
                YCbCrtoRGB(cp [1], pp[1]);
                YCbCrtoRGB(cp1[0], pp[2]);
                YCbCrtoRGB(cp1[1], pp[3]);

                cp += 2, cp1 += 2;
                pp += 6;
            } while (--x);
            cp += incr, cp1 += incr;
            pp += fromskew;
        }
    } else {
        while (h > 0) {
            for (x = w; x > 0;) {
                int Cb = pp[4];
                int Cr = pp[5];
                switch (x) {
                default:
                    switch (h) {
                    default: YCbCrtoRGB(cp1[1], pp[ 3]); /* FALLTHROUGH */
                    case 1:  YCbCrtoRGB(cp [1], pp[ 1]); /* FALLTHROUGH */
                    }                                    /* FALLTHROUGH */
                case 1:
                    switch (h) {
                    default: YCbCrtoRGB(cp1[0], pp[ 2]); /* FALLTHROUGH */
                    case 1:  YCbCrtoRGB(cp [0], pp[ 0]); /* FALLTHROUGH */
                    }                                    /* FALLTHROUGH */
                }
                if (x < 2) {
                    cp += x; cp1 += x;
                    x = 0;
                }
                else {
                    cp += 2; cp1 += 2;
                    x -= 2;
                }
                pp += 6;
            }
            if (h <= 2)
                break;
            h -= 2;
            cp += incr, cp1 += incr;
            pp += fromskew;
        }
    }
}

/*
 * 8-bit packed YCbCr samples w/ 2,1 subsampling => RGB
 */
DECLAREContigPutFunc(putcontig8bitYCbCr21tile)
{
    YCbCrSetup;

    (void) y;
    fromskew = (fromskew * 4) / 2;
    do {
	x = w>>1;
	do {
	    int Cb = pp[2];
	    int Cr = pp[3];

	    YCbCrtoRGB(cp[0], pp[0]); 
	    YCbCrtoRGB(cp[1], pp[1]);

	    cp += 2;
	    pp += 4;
	} while (--x);

        if( (w&1) != 0 )
        {
	    int Cb = pp[2];
	    int Cr = pp[3];
            
            YCbCrtoRGB(cp [0], pp[0]);

	    cp += 1;
	    pp += 4;
        }

	cp += toskew;
	pp += fromskew;
    } while (--h);
}

/*
 * 8-bit packed YCbCr samples w/ no subsampling => RGB
 */
DECLAREContigPutFunc(putcontig8bitYCbCr11tile)
{
    YCbCrSetup;

    (void) y;
    fromskew *= 3;
    do {
        x = w; /* was x = w>>1; patched 2000/09/25 warmerda@home.com */ 
	do {
	    int Cb = pp[1];
	    int Cr = pp[2];

	    YCbCrtoRGB(*cp++, pp[0]);

	    pp += 3;
	} while (--x);
	cp += toskew;
	pp += fromskew;
    } while (--h);
}
#undef	YCbCrSetup
#undef	YCbCrtoRGB

#define	LumaRed			coeffs[0]
#define	LumaGreen		coeffs[1]
#define	LumaBlue		coeffs[2]
#define	SHIFT			16
#define	FIX(x)			((int32)((x) * (1L<<SHIFT) + 0.5))
#define	ONE_HALF		((int32)(1<<(SHIFT-1)))

/*
 * Initialize the YCbCr->RGB conversion tables.  The conversion
 * is done according to the 6.0 spec:
 *
 *    R = Y + Cr*(2 - 2*LumaRed)
 *    B = Y + Cb*(2 - 2*LumaBlue)
 *    G =   Y
 *        - LumaBlue*Cb*(2-2*LumaBlue)/LumaGreen
 *        - LumaRed*Cr*(2-2*LumaRed)/LumaGreen
 *
 * To avoid floating point arithmetic the fractional constants that
 * come out of the equations are represented as fixed point values
 * in the range 0...2^16.  We also eliminate multiplications by
 * pre-calculating possible values indexed by Cb and Cr (this code
 * assumes conversion is being done for 8-bit samples).
 */
static void
TIFFYCbCrToRGBInit(TIFFYCbCrToRGB* ycbcr, TIFF* tif)
{
    TIFFRGBValue* clamptab;
    float* coeffs;
    int i;

    clamptab = (TIFFRGBValue*)(
	(tidata_t) ycbcr+TIFFroundup(sizeof (TIFFYCbCrToRGB), sizeof (long)));
    _TIFFmemset(clamptab, 0, 256);		/* v < 0 => 0 */
    ycbcr->clamptab = (clamptab += 256);
    for (i = 0; i < 256; i++)
	clamptab[i] = i;
    _TIFFmemset(clamptab+256, 255, 2*256);	/* v > 255 => 255 */
    TIFFGetFieldDefaulted(tif, TIFFTAG_YCBCRCOEFFICIENTS, &coeffs);
    _TIFFmemcpy(ycbcr->coeffs, coeffs, 3*sizeof (float));
    { float f1 = 2-2*LumaRed;		int32 D1 = FIX(f1);
      float f2 = LumaRed*f1/LumaGreen;	int32 D2 = -FIX(f2);
      float f3 = 2-2*LumaBlue;		int32 D3 = FIX(f3);
      float f4 = LumaBlue*f3/LumaGreen;	int32 D4 = -FIX(f4);
      int x;

      ycbcr->Cr_r_tab = (int*) (clamptab + 3*256);
      ycbcr->Cb_b_tab = ycbcr->Cr_r_tab + 256;
      ycbcr->Cr_g_tab = (int32*) (ycbcr->Cb_b_tab + 256);
      ycbcr->Cb_g_tab = ycbcr->Cr_g_tab + 256;
      /*
       * i is the actual input pixel value in the range 0..255
       * Cb and Cr values are in the range -128..127 (actually
       * they are in a range defined by the ReferenceBlackWhite
       * tag) so there is some range shifting to do here when
       * constructing tables indexed by the raw pixel data.
       *
       * XXX handle ReferenceBlackWhite correctly to calculate
       *     Cb/Cr values to use in constructing the tables.
       */
      for (i = 0, x = -128; i < 256; i++, x++) {
	  ycbcr->Cr_r_tab[i] = (int)((D1*x + ONE_HALF)>>SHIFT);
	  ycbcr->Cb_b_tab[i] = (int)((D3*x + ONE_HALF)>>SHIFT);
	  ycbcr->Cr_g_tab[i] = D2*x;
	  ycbcr->Cb_g_tab[i] = D4*x + ONE_HALF;
      }
    }
}
#undef	SHIFT
#undef	ONE_HALF
#undef	FIX
#undef	LumaBlue
#undef	LumaGreen
#undef	LumaRed

static tileContigRoutine
initYCbCrConversion(TIFFRGBAImage* img)
{
    uint16 hs, vs;

    if (img->ycbcr == NULL) {
	img->ycbcr = (TIFFYCbCrToRGB*) _TIFFmalloc(
	      TIFFroundup(sizeof (TIFFYCbCrToRGB), sizeof (long))
	    + 4*256*sizeof (TIFFRGBValue)
	    + 2*256*sizeof (int)
	    + 2*256*sizeof (int32)
	);
	if (img->ycbcr == NULL) {
	    TIFFError(TIFFFileName(img->tif),
		"No space for YCbCr->RGB conversion state");
	    return (NULL);
	}
	TIFFYCbCrToRGBInit(img->ycbcr, img->tif);
    } else {
	float* coeffs;

	TIFFGetFieldDefaulted(img->tif, TIFFTAG_YCBCRCOEFFICIENTS, &coeffs);
	if (_TIFFmemcmp(coeffs, img->ycbcr->coeffs, 3*sizeof (float)) != 0)
	    TIFFYCbCrToRGBInit(img->ycbcr, img->tif);
    }
    /*
     * The 6.0 spec says that subsampling must be
     * one of 1, 2, or 4, and that vertical subsampling
     * must always be <= horizontal subsampling; so
     * there are only a few possibilities and we just
     * enumerate the cases.
     */
    TIFFGetFieldDefaulted(img->tif, TIFFTAG_YCBCRSUBSAMPLING, &hs, &vs);
    switch ((hs<<4)|vs) {
    case 0x44: return (putcontig8bitYCbCr44tile);
    case 0x42: return (putcontig8bitYCbCr42tile);
    case 0x41: return (putcontig8bitYCbCr41tile);
    case 0x22: return (putcontig8bitYCbCr22tile);
    case 0x21: return (putcontig8bitYCbCr21tile);
    case 0x11: return (putcontig8bitYCbCr11tile);
    }
    return (NULL);
}

/*
 * Greyscale images with less than 8 bits/sample are handled
 * with a table to avoid lots of shifts and masks.  The table
 * is setup so that put*bwtile (below) can retrieve 8/bitspersample
 * pixel values simply by indexing into the table with one
 * number.
 */
static int
makebwmap(TIFFRGBAImage* img)
{
    TIFFRGBValue* Map = img->Map;
    int bitspersample = img->bitspersample;
    int nsamples = 8 / bitspersample;
    int i;
    uint32* p;

    if( nsamples == 0 )
        nsamples = 1;

    img->BWmap = (uint32**) _TIFFmalloc(
	256*sizeof (uint32 *)+(256*nsamples*sizeof(uint32)));
    if (img->BWmap == NULL) {
	TIFFError(TIFFFileName(img->tif), "No space for B&W mapping table");
	return (0);
    }
    p = (uint32*)(img->BWmap + 256);
    for (i = 0; i < 256; i++) {
	TIFFRGBValue c;
	img->BWmap[i] = p;
	switch (bitspersample) {
#define	GREY(x)	c = Map[x]; *p++ = PACK(c,c,c);
	case 1:
	    GREY(i>>7);
	    GREY((i>>6)&1);
	    GREY((i>>5)&1);
	    GREY((i>>4)&1);
	    GREY((i>>3)&1);
	    GREY((i>>2)&1);
	    GREY((i>>1)&1);
	    GREY(i&1);
	    break;
	case 2:
	    GREY(i>>6);
	    GREY((i>>4)&3);
	    GREY((i>>2)&3);
	    GREY(i&3);
	    break;
	case 4:
	    GREY(i>>4);
	    GREY(i&0xf);
	    break;
	case 8:
        case 16:
	    GREY(i);
	    break;
	}
#undef	GREY
    }
    return (1);
}

/*
 * Construct a mapping table to convert from the range
 * of the data samples to [0,255] --for display.  This
 * process also handles inverting B&W images when needed.
 */ 
static int
setupMap(TIFFRGBAImage* img)
{
    int32 x, range;

    range = (int32)((1L<<img->bitspersample)-1);
    
    /* treat 16 bit the same as eight bit */
    if( img->bitspersample /*== 16*/ > 8 ) // * VK
        range = (int32) 255;

    img->Map = (TIFFRGBValue*) _TIFFmalloc((range+1) * sizeof (TIFFRGBValue));
    if (img->Map == NULL) {
	TIFFError(TIFFFileName(img->tif),
	    "No space for photometric conversion table");
	return (0);
    }
    if (img->photometric == PHOTOMETRIC_MINISWHITE) {
	for (x = 0; x <= range; x++)
	    img->Map[x] = (TIFFRGBValue) (((range - x) * 255) / range);
    } else {
	for (x = 0; x <= range; x++)
	    img->Map[x] = (TIFFRGBValue) ((x * 255) / range);
    }
    if (img->bitspersample <= 16 &&
	(img->photometric == PHOTOMETRIC_MINISBLACK ||
	 img->photometric == PHOTOMETRIC_MINISWHITE)) {
	/*
	 * Use photometric mapping table to construct
	 * unpacking tables for samples <= 8 bits.
	 */
	if (!makebwmap(img))
	    return (0);
	/* no longer need Map, free it */
	_TIFFfree(img->Map), img->Map = NULL;
    }
    return (1);
}

static int
checkcmap(TIFFRGBAImage* img)
{
    uint16* r = img->redcmap;
    uint16* g = img->greencmap;
    uint16* b = img->bluecmap;
    long n = 1L<<img->bitspersample;

    while (n-- > 0)
	if (*r++ >= 256 || *g++ >= 256 || *b++ >= 256)
	    return (16);
    return (8);
}

static void
cvtcmap(TIFFRGBAImage* img)
{
    uint16* r = img->redcmap;
    uint16* g = img->greencmap;
    uint16* b = img->bluecmap;
    long i;

    for (i = (1L<<img->bitspersample)-1; i >= 0; i--) {
#define	CVT(x)		((uint16)((x)>>8))
	r[i] = CVT(r[i]);
	g[i] = CVT(g[i]);
	b[i] = CVT(b[i]);
#undef	CVT
    }
}

/*
 * Palette images with <= 8 bits/sample are handled
 * with a table to avoid lots of shifts and masks.  The table
 * is setup so that put*cmaptile (below) can retrieve 8/bitspersample
 * pixel values simply by indexing into the table with one
 * number.
 */
static int
makecmap(TIFFRGBAImage* img)
{
    int bitspersample = img->bitspersample;
    int nsamples = 8 / bitspersample;
    uint16* r = img->redcmap;
    uint16* g = img->greencmap;
    uint16* b = img->bluecmap;
    uint32 *p;
    int i;

    img->PALmap = (uint32**) _TIFFmalloc(
	256*sizeof (uint32 *)+(256*nsamples*sizeof(uint32)));
    if (img->PALmap == NULL) {
	TIFFError(TIFFFileName(img->tif), "No space for Palette mapping table");
	return (0);
    }
    p = (uint32*)(img->PALmap + 256);
    for (i = 0; i < 256; i++) {
	TIFFRGBValue c;
	img->PALmap[i] = p;
#define	CMAP(x)	c = x; *p++ = PACK(r[c]&0xff, g[c]&0xff, b[c]&0xff);
	switch (bitspersample) {
	case 1:
	    CMAP(i>>7);
	    CMAP((i>>6)&1);
	    CMAP((i>>5)&1);
	    CMAP((i>>4)&1);
	    CMAP((i>>3)&1);
	    CMAP((i>>2)&1);
	    CMAP((i>>1)&1);
	    CMAP(i&1);
	    break;
	case 2:
	    CMAP(i>>6);
	    CMAP((i>>4)&3);
	    CMAP((i>>2)&3);
	    CMAP(i&3);
	    break;
	case 4:
	    CMAP(i>>4);
	    CMAP(i&0xf);
	    break;
	case 8:
	    CMAP(i);
	    break;
	}
#undef CMAP
    }
    return (1);
}

/* 
 * Construct any mapping table used
 * by the associated put routine.
 */
static int
buildMap(TIFFRGBAImage* img)
{
    switch (img->photometric) {
    case PHOTOMETRIC_RGB:
    case PHOTOMETRIC_YCBCR:
    case PHOTOMETRIC_SEPARATED:
	if (img->bitspersample == 8)
	    break;
	/* fall thru... */
    case PHOTOMETRIC_MINISBLACK:
    case PHOTOMETRIC_MINISWHITE:
	if (!setupMap(img))
	    return (0);
	break;
    case PHOTOMETRIC_PALETTE:
	/*
	 * Convert 16-bit colormap to 8-bit (unless it looks
	 * like an old-style 8-bit colormap).
	 */
	if (checkcmap(img) == 16)
	    cvtcmap(img);
	else
	    TIFFWarning(TIFFFileName(img->tif), "Assuming 8-bit colormap");
	/*
	 * Use mapping table and colormap to construct
	 * unpacking tables for samples < 8 bits.
	 */
	if (img->bitspersample <= 8 && !makecmap(img))
	    return (0);
	break;
    }
    return (1);
}

/*
 * Select the appropriate conversion routine for packed data.
 */
static int
pickTileContigCase(TIFFRGBAImage* img)
{
    tileContigRoutine put = 0;

    if (buildMap(img)) {
	switch (img->photometric) {
	case PHOTOMETRIC_RGB:
	    switch (img->bitspersample) {
			case 2: // + VK +
			if (!img->Map) 
				put = putRGBcontig2bittile; 
			else
				put = putRGBcontig2bitMaptile; // VK: todo putRGBcontig2bitMaptile
			break;
			case 4: // + VK +
			if (!img->Map) 
				put = putRGBcontig4bittile; 
			else
				put = putRGBcontig4bitMaptile; 
			break;
			case 8:
			if (!img->Map) {
				if (img->alpha == EXTRASAMPLE_ASSOCALPHA)
				put = putRGBAAcontig8bittile;
				else if (img->alpha == EXTRASAMPLE_UNASSALPHA)
				put = putRGBUAcontig8bittile;
				else
				put = putRGBcontig8bittile;
			} else
				put = putRGBcontig8bitMaptile;
			break;
			case 16:
			put = putRGBcontig16bittile;
			if (!img->Map) {
				if (img->alpha == EXTRASAMPLE_ASSOCALPHA)
				put = putRGBAAcontig16bittile;
				else if (img->alpha == EXTRASAMPLE_UNASSALPHA)
				put = putRGBUAcontig16bittile;
			}
			break;
			default: // + VK + 
			if (!img->Map) 
				put = putRGBcontigAllbittile; 
			else
				put = putRGBcontigAllbittile;  // todo Map
			break;
	    }
	    break;
	case PHOTOMETRIC_SEPARATED:
	    if (img->bitspersample == 8) {
		if (!img->Map)
		    put = putRGBcontig8bitCMYKtile;
		else
		    put = putRGBcontig8bitCMYKMaptile;
		} else if( img->bitspersample == 16) { // + VK +
			if (!img->Map)
				put = put_CMYK_contig16bittile;
			else
				put = put_CMYKmap_contig16bittile;
	    }
	    break;
	case PHOTOMETRIC_PALETTE:
	    switch (img->bitspersample) {
	    case 8:	put = put8bitcmaptile; break;
	    case 4: put = put4bitcmaptile; break;
	    case 2: put = put2bitcmaptile; break;
	    case 1: put = put1bitcmaptile; break;
	    }
	    break;
	case PHOTOMETRIC_MINISWHITE:
	case PHOTOMETRIC_MINISBLACK:
	    switch (img->bitspersample) {
            case 16: put = put16bitbwtile; break;
	    case 8:  put = putgreytile; break;
	    case 4:  put = put4bitbwtile; break;
	    case 2:  put = put2bitbwtile; break;
	    case 1:  put = put1bitbwtile; break;
	    }
	    break;
	case PHOTOMETRIC_YCBCR:
	    if (img->bitspersample == 8)
		put = initYCbCrConversion(img);
	    break;
	}
    }
    return ((img->put.contig = put) != 0);
}

/*
 * Select the appropriate conversion routine for unpacked data.
 *
 * NB: we assume that unpacked single channel data is directed
 *	 to the "packed routines.
 */
static int
pickTileSeparateCase(TIFFRGBAImage* img)
{
    tileSeparateRoutine put = 0;

    if (buildMap(img)) {
		switch (img->photometric) {
		case PHOTOMETRIC_RGB:
			switch (img->bitspersample) {
				case 8:
					if (!img->Map) {
						if (img->alpha == EXTRASAMPLE_ASSOCALPHA)
						put = putRGBAAseparate8bittile;
						else if (img->alpha == EXTRASAMPLE_UNASSALPHA)
						put = putRGBUAseparate8bittile;
						else
						put = putRGBseparate8bittile;
					} else
						put = putRGBseparate8bitMaptile;
					break;
				case 16:
					put = putRGBseparate16bittile;
					if (!img->Map) {
						if (img->alpha == EXTRASAMPLE_ASSOCALPHA)
						put = putRGBAAseparate16bittile;
						else if (img->alpha == EXTRASAMPLE_UNASSALPHA)
						put = putRGBUAseparate16bittile;
					}
					break;
				default:
					put = putRGBseparateAllbittile;
				}
				break;
		case PHOTOMETRIC_SEPARATED:
			put = putRGBplanarseparateAllbittile;
			break;
		}
	}
    return ((img->put.separate = put) != 0);
}

/*
 * Read a whole strip off data from the file, and convert to RGBA form.
 * If this is the last strip, then it will only contain the portion of
 * the strip that is actually within the image space.  The result is
 * organized in bottom to top form.
 */


int
TIFFReadRGBAStrip(TIFF* tif, uint32 row, uint32 * raster )

{
    char 	emsg[1024];
    TIFFRGBAImage img;
    int 	ok;
    uint32	rowsperstrip, rows_to_read;

    if( TIFFIsTiled( tif ) )
    {
        TIFFError(TIFFFileName(tif),
                  "Can't use TIFFReadRGBAStrip() with tiled file.");
	return (0);
    }
    
    TIFFGetFieldDefaulted(tif, TIFFTAG_ROWSPERSTRIP, &rowsperstrip);
    if( (row % rowsperstrip) != 0 )
    {
        TIFFError(TIFFFileName(tif),
                "Row passed to TIFFReadRGBAStrip() must be first in a strip.");
	return (0);
    }

    if (TIFFRGBAImageBegin(&img, tif, 0, emsg)) {

        img.row_offset = row;
        img.col_offset = 0;

        if( row + rowsperstrip > img.height )
            rows_to_read = img.height - row;
        else
            rows_to_read = rowsperstrip;
        
	ok = TIFFRGBAImageGet(&img, raster, img.width, rows_to_read );
        
	TIFFRGBAImageEnd(&img);
    } else {
	TIFFError(TIFFFileName(tif), emsg);
	ok = 0;
    }
    
    return (ok);
}

/*
 * Read a whole tile off data from the file, and convert to RGBA form.
 * The returned RGBA data is organized from bottom to top of tile,
 * and may include zeroed areas if the tile extends off the image.
 */

int
TIFFReadRGBATile(TIFF* tif, uint32 col, uint32 row, uint32 * raster)

{
    char 	emsg[1024];
    TIFFRGBAImage img;
    int 	ok;
    uint32	tile_xsize, tile_ysize;
    uint32	read_xsize, read_ysize;
    uint32	i_row;

    /*
     * Verify that our request is legal - on a tile file, and on a
     * tile boundary.
     */
    
    if( !TIFFIsTiled( tif ) )
    {
        TIFFError(TIFFFileName(tif),
                  "Can't use TIFFReadRGBATile() with stripped file.");
	return (0);
    }
    
    TIFFGetFieldDefaulted(tif, TIFFTAG_TILEWIDTH, &tile_xsize);
    TIFFGetFieldDefaulted(tif, TIFFTAG_TILELENGTH, &tile_ysize);
    if( (col % tile_xsize) != 0 || (row % tile_ysize) != 0 )
    {
        TIFFError(TIFFFileName(tif),
                  "Row/col passed to TIFFReadRGBATile() must be top"
                  "left corner of a tile.");
	return (0);
    }

    /*
     * Setup the RGBA reader.
     */
    
    if ( !TIFFRGBAImageBegin(&img, tif, 0, emsg)) {
	TIFFError(TIFFFileName(tif), emsg);
        return( 0 );
    }

    /*
     * The TIFFRGBAImageGet() function doesn't allow us to get off the
     * edge of the image, even to fill an otherwise valid tile.  So we
     * figure out how much we can read, and fix up the tile buffer to
     * a full tile configuration afterwards.
     */

    if( row + tile_ysize > img.height )
        read_ysize = img.height - row;
    else
        read_ysize = tile_ysize;
    
    if( col + tile_xsize > img.width )
        read_xsize = img.width - col;
    else
        read_xsize = tile_xsize;

    /*
     * Read the chunk of imagery.
     */
    
    img.row_offset = row;
    img.col_offset = col;

    ok = TIFFRGBAImageGet(&img, raster, read_xsize, read_ysize );
        
    TIFFRGBAImageEnd(&img);

    /*
     * If our read was incomplete we will need to fix up the tile by
     * shifting the data around as if a full tile of data is being returned.
     *
     * This is all the more complicated because the image is organized in
     * bottom to top format. 
     */

    if( read_xsize == tile_xsize && read_ysize == tile_ysize )
        return( ok );

    for( i_row = 0; i_row < read_ysize; i_row++ )
    {
        memmove( raster + (tile_ysize - i_row - 1) * tile_xsize,
                 raster + (read_ysize - i_row - 1) * read_xsize,
                 read_xsize * sizeof(uint32) );
        _TIFFmemset( raster + (tile_ysize - i_row - 1) * tile_xsize+read_xsize,
                     0, sizeof(uint32) * (tile_xsize - read_xsize) );
    }

    for( i_row = read_ysize; i_row < tile_ysize; i_row++ )
    {
        _TIFFmemset( raster + (tile_ysize - i_row - 1) * tile_xsize,
                     0, sizeof(uint32) * tile_xsize );
    }

    return (ok);
}
