/* $Header: /cvsroot/osrs/libtiff/libtiff/tif_dirinfo.c,v 1.8 2001/05/09 01:33:16 warmerda Exp $ */

/*
 * Copyright (c) 1988-1997 Sam Leffler
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
 * TIFF Library.
 *
 * Core Directory Tag Support.
 */
#include "tiffiop.h"
#include <stdlib.h>

/*
 * NB: NB: THIS ARRAY IS ASSUMED TO BE SORTED BY TAG.
 *     If a tag can have both LONG and SHORT types
 *     then the LONG must be placed before the SHORT for
 *     writing to work properly.
 *
 * NOTE: The second field (field_readcount) and third field (field_writecount)
 *       sometimes use the values TIFF_VARIABLE (-1), TIFF_VARIABLE2 (-3)
 *       and TIFFTAG_SPP (-2). The macros should be used but would throw off 
 *       the formatting of the code, so please interprete the -1, -2 and -3 
 *       values accordingly.
 */
#ifndef VMS
static 
#endif
const TIFFFieldInfo tiffFieldInfo[] = {
    { TIFFTAG_SUBFILETYPE,	 1, 1, TIFF_LONG,	FIELD_SUBFILETYPE,
      TRUE,	FALSE,	"SubfileType" },
/* XXX SHORT for compatibility w/ old versions of the library */
    { TIFFTAG_SUBFILETYPE,	 1, 1, TIFF_SHORT,	FIELD_SUBFILETYPE,
      TRUE,	FALSE,	"SubfileType" },
    { TIFFTAG_OSUBFILETYPE,	 1, 1, TIFF_SHORT,	FIELD_SUBFILETYPE,
      TRUE,	FALSE,	"OldSubfileType" },
    { TIFFTAG_IMAGEWIDTH,	 1, 1, TIFF_LONG,	FIELD_IMAGEDIMENSIONS,
      FALSE,	FALSE,	"ImageWidth" },
    { TIFFTAG_IMAGEWIDTH,	 1, 1, TIFF_SHORT,	FIELD_IMAGEDIMENSIONS,
      FALSE,	FALSE,	"ImageWidth" },
    { TIFFTAG_IMAGELENGTH,	 1, 1, TIFF_LONG,	FIELD_IMAGEDIMENSIONS,
      TRUE,	FALSE,	"ImageLength" },
    { TIFFTAG_IMAGELENGTH,	 1, 1, TIFF_SHORT,	FIELD_IMAGEDIMENSIONS,
      TRUE,	FALSE,	"ImageLength" },
    { TIFFTAG_BITSPERSAMPLE,	-1,-1, TIFF_SHORT,	FIELD_BITSPERSAMPLE,
      FALSE,	FALSE,	"BitsPerSample" },
    { TIFFTAG_COMPRESSION,	-1, 1, TIFF_SHORT,	FIELD_COMPRESSION,
      FALSE,	FALSE,	"Compression" },
    { TIFFTAG_PHOTOMETRIC,	 1, 1, TIFF_SHORT,	FIELD_PHOTOMETRIC,
      FALSE,	FALSE,	"PhotometricInterpretation" },
    { TIFFTAG_THRESHHOLDING,	 1, 1, TIFF_SHORT,	FIELD_THRESHHOLDING,
      TRUE,	FALSE,	"Threshholding" },
    { TIFFTAG_CELLWIDTH,	 1, 1, TIFF_SHORT,	FIELD_IGNORE,
      TRUE,	FALSE,	"CellWidth" },
    { TIFFTAG_CELLLENGTH,	 1, 1, TIFF_SHORT,	FIELD_IGNORE,
      TRUE,	FALSE,	"CellLength" },
    { TIFFTAG_FILLORDER,	 1, 1, TIFF_SHORT,	FIELD_FILLORDER,
      FALSE,	FALSE,	"FillOrder" },
    { TIFFTAG_DOCUMENTNAME,	-1,-1, TIFF_ASCII,	FIELD_DOCUMENTNAME,
      TRUE,	FALSE,	"DocumentName" },
    { TIFFTAG_IMAGEDESCRIPTION,	-1,-1, TIFF_ASCII,	FIELD_IMAGEDESCRIPTION,
      TRUE,	FALSE,	"ImageDescription" },
    { TIFFTAG_MAKE,		-1,-1, TIFF_ASCII,	FIELD_MAKE,
      TRUE,	FALSE,	"Make" },
    { TIFFTAG_MODEL,		-1,-1, TIFF_ASCII,	FIELD_MODEL,
      TRUE,	FALSE,	"Model" },
    { TIFFTAG_STRIPOFFSETS,	-1,-1, TIFF_LONG,	FIELD_STRIPOFFSETS,
      FALSE,	FALSE,	"StripOffsets" },
    { TIFFTAG_STRIPOFFSETS,	-1,-1, TIFF_SHORT,	FIELD_STRIPOFFSETS,
      FALSE,	FALSE,	"StripOffsets" },
    { TIFFTAG_ORIENTATION,	 1, 1, TIFF_SHORT,	FIELD_ORIENTATION,
      FALSE,	FALSE,	"Orientation" },
    { TIFFTAG_SAMPLESPERPIXEL,	 1, 1, TIFF_SHORT,	FIELD_SAMPLESPERPIXEL,
      FALSE,	FALSE,	"SamplesPerPixel" },
    { TIFFTAG_ROWSPERSTRIP,	 1, 1, TIFF_LONG,	FIELD_ROWSPERSTRIP,
      FALSE,	FALSE,	"RowsPerStrip" },
    { TIFFTAG_ROWSPERSTRIP,	 1, 1, TIFF_SHORT,	FIELD_ROWSPERSTRIP,
      FALSE,	FALSE,	"RowsPerStrip" },
    { TIFFTAG_STRIPBYTECOUNTS,	-1,-1, TIFF_LONG,	FIELD_STRIPBYTECOUNTS,
      FALSE,	FALSE,	"StripByteCounts" },
    { TIFFTAG_STRIPBYTECOUNTS,	-1,-1, TIFF_SHORT,	FIELD_STRIPBYTECOUNTS,
      FALSE,	FALSE,	"StripByteCounts" },
    { TIFFTAG_MINSAMPLEVALUE,	-2,-1, TIFF_SHORT,	FIELD_MINSAMPLEVALUE,
      TRUE,	FALSE,	"MinSampleValue" },
    { TIFFTAG_MAXSAMPLEVALUE,	-2,-1, TIFF_SHORT,	FIELD_MAXSAMPLEVALUE,
      TRUE,	FALSE,	"MaxSampleValue" },
    { TIFFTAG_XRESOLUTION,	 1, 1, TIFF_RATIONAL,	FIELD_RESOLUTION,
      FALSE,	FALSE,	"XResolution" },
    { TIFFTAG_YRESOLUTION,	 1, 1, TIFF_RATIONAL,	FIELD_RESOLUTION,
      FALSE,	FALSE,	"YResolution" },
    { TIFFTAG_PLANARCONFIG,	 1, 1, TIFF_SHORT,	FIELD_PLANARCONFIG,
      FALSE,	FALSE,	"PlanarConfiguration" },
    { TIFFTAG_PAGENAME,		-1,-1, TIFF_ASCII,	FIELD_PAGENAME,
      TRUE,	FALSE,	"PageName" },
    { TIFFTAG_XPOSITION,	 1, 1, TIFF_RATIONAL,	FIELD_POSITION,
      TRUE,	FALSE,	"XPosition" },
    { TIFFTAG_YPOSITION,	 1, 1, TIFF_RATIONAL,	FIELD_POSITION,
      TRUE,	FALSE,	"YPosition" },
    { TIFFTAG_FREEOFFSETS,	-1,-1, TIFF_LONG,	FIELD_IGNORE,
      FALSE,	FALSE,	"FreeOffsets" },
    { TIFFTAG_FREEBYTECOUNTS,	-1,-1, TIFF_LONG,	FIELD_IGNORE,
      FALSE,	FALSE,	"FreeByteCounts" },
    { TIFFTAG_GRAYRESPONSEUNIT,	 1, 1, TIFF_SHORT,	FIELD_IGNORE,
      TRUE,	FALSE,	"GrayResponseUnit" },
    { TIFFTAG_GRAYRESPONSECURVE,-1,-1, TIFF_SHORT,	FIELD_IGNORE,
      TRUE,	FALSE,	"GrayResponseCurve" },
    { TIFFTAG_RESOLUTIONUNIT,	 1, 1, TIFF_SHORT,	FIELD_RESOLUTIONUNIT,
      FALSE,	FALSE,	"ResolutionUnit" },
    { TIFFTAG_PAGENUMBER,	 2, 2, TIFF_SHORT,	FIELD_PAGENUMBER,
      TRUE,	FALSE,	"PageNumber" },
    { TIFFTAG_COLORRESPONSEUNIT, 1, 1, TIFF_SHORT,	FIELD_IGNORE,
      TRUE,	FALSE,	"ColorResponseUnit" },
#ifdef COLORIMETRY_SUPPORT
    { TIFFTAG_TRANSFERFUNCTION,	-1,-1, TIFF_SHORT,	FIELD_TRANSFERFUNCTION,
      TRUE,	FALSE,	"TransferFunction" },
#endif
    { TIFFTAG_SOFTWARE,		-1,-1, TIFF_ASCII,	FIELD_SOFTWARE,
      TRUE,	FALSE,	"Software" },
    { TIFFTAG_DATETIME,		20,20, TIFF_ASCII,	FIELD_DATETIME,
      TRUE,	FALSE,	"DateTime" },
    { TIFFTAG_ARTIST,		-1,-1, TIFF_ASCII,	FIELD_ARTIST,
      TRUE,	FALSE,	"Artist" },
    { TIFFTAG_HOSTCOMPUTER,	-1,-1, TIFF_ASCII,	FIELD_HOSTCOMPUTER,
      TRUE,	FALSE,	"HostComputer" },
#ifdef COLORIMETRY_SUPPORT
    { TIFFTAG_WHITEPOINT,	 2, 2, TIFF_RATIONAL,FIELD_WHITEPOINT,
      TRUE,	FALSE,	"WhitePoint" },
    { TIFFTAG_PRIMARYCHROMATICITIES,6,6,TIFF_RATIONAL,FIELD_PRIMARYCHROMAS,
      TRUE,	FALSE,	"PrimaryChromaticities" },
#endif
    { TIFFTAG_COLORMAP,		-1,-1, TIFF_SHORT,	FIELD_COLORMAP,
      TRUE,	FALSE,	"ColorMap" },
    { TIFFTAG_HALFTONEHINTS,	 2, 2, TIFF_SHORT,	FIELD_HALFTONEHINTS,
      TRUE,	FALSE,	"HalftoneHints" },
    { TIFFTAG_TILEWIDTH,	 1, 1, TIFF_LONG,	FIELD_TILEDIMENSIONS,
      FALSE,	FALSE,	"TileWidth" },
    { TIFFTAG_TILEWIDTH,	 1, 1, TIFF_SHORT,	FIELD_TILEDIMENSIONS,
      FALSE,	FALSE,	"TileWidth" },
    { TIFFTAG_TILELENGTH,	 1, 1, TIFF_LONG,	FIELD_TILEDIMENSIONS,
      FALSE,	FALSE,	"TileLength" },
    { TIFFTAG_TILELENGTH,	 1, 1, TIFF_SHORT,	FIELD_TILEDIMENSIONS,
      FALSE,	FALSE,	"TileLength" },
    { TIFFTAG_TILEOFFSETS,	-1, 1, TIFF_LONG,	FIELD_STRIPOFFSETS,
      FALSE,	FALSE,	"TileOffsets" },
    { TIFFTAG_TILEBYTECOUNTS,	-1, 1, TIFF_LONG,	FIELD_STRIPBYTECOUNTS,
      FALSE,	FALSE,	"TileByteCounts" },
    { TIFFTAG_TILEBYTECOUNTS,	-1, 1, TIFF_SHORT,	FIELD_STRIPBYTECOUNTS,
      FALSE,	FALSE,	"TileByteCounts" },
#ifdef TIFFTAG_SUBIFD
    { TIFFTAG_SUBIFD,		-1,-1, TIFF_LONG,	FIELD_SUBIFD,
      TRUE,	TRUE,	"SubIFD" },
#endif
#ifdef CMYK_SUPPORT		/* 6.0 CMYK tags */
    { TIFFTAG_INKSET,		 1, 1, TIFF_SHORT,	FIELD_INKSET,
      FALSE,	FALSE,	"InkSet" },
    { TIFFTAG_INKNAMES,		-1,-1, TIFF_ASCII,	FIELD_INKNAMES,
      TRUE,	TRUE,	"InkNames" },
    { TIFFTAG_NUMBEROFINKS,	 1, 1, TIFF_SHORT,	FIELD_NUMBEROFINKS,
      TRUE,	FALSE,	"NumberOfInks" },
    { TIFFTAG_DOTRANGE,		 2, 2, TIFF_SHORT,	FIELD_DOTRANGE,
      FALSE,	FALSE,	"DotRange" },
    { TIFFTAG_DOTRANGE,		 2, 2, TIFF_BYTE,	FIELD_DOTRANGE,
      FALSE,	FALSE,	"DotRange" },
    { TIFFTAG_TARGETPRINTER,	-1,-1, TIFF_ASCII,	FIELD_TARGETPRINTER,
      TRUE,	FALSE,	"TargetPrinter" },
#endif
    { TIFFTAG_EXTRASAMPLES,	-1,-1, TIFF_SHORT,	FIELD_EXTRASAMPLES,
      FALSE,	FALSE,	"ExtraSamples" },
/* XXX for bogus Adobe Photoshop v2.5 files */
    { TIFFTAG_EXTRASAMPLES,	-1,-1, TIFF_BYTE,	FIELD_EXTRASAMPLES,
      FALSE,	FALSE,	"ExtraSamples" },
    { TIFFTAG_SAMPLEFORMAT,	-1,-1, TIFF_SHORT,	FIELD_SAMPLEFORMAT,
      FALSE,	FALSE,	"SampleFormat" },
    { TIFFTAG_SMINSAMPLEVALUE,	-2,-1, TIFF_ANY,	FIELD_SMINSAMPLEVALUE,
      TRUE,	FALSE,	"SMinSampleValue" },
    { TIFFTAG_SMAXSAMPLEVALUE,	-2,-1, TIFF_ANY,	FIELD_SMAXSAMPLEVALUE,
      TRUE,	FALSE,	"SMaxSampleValue" },
#ifdef YCBCR_SUPPORT		/* 6.0 YCbCr tags */
    { TIFFTAG_YCBCRCOEFFICIENTS, 3, 3, TIFF_RATIONAL,	FIELD_YCBCRCOEFFICIENTS,
      FALSE,	FALSE,	"YCbCrCoefficients" },
    { TIFFTAG_YCBCRSUBSAMPLING,	 2, 2, TIFF_SHORT,	FIELD_YCBCRSUBSAMPLING,
      FALSE,	FALSE,	"YCbCrSubsampling" },
    { TIFFTAG_YCBCRPOSITIONING,	 1, 1, TIFF_SHORT,	FIELD_YCBCRPOSITIONING,
      FALSE,	FALSE,	"YCbCrPositioning" },
#endif
#ifdef COLORIMETRY_SUPPORT
    { TIFFTAG_REFERENCEBLACKWHITE,6,6,TIFF_RATIONAL,	FIELD_REFBLACKWHITE,
      TRUE,	FALSE,	"ReferenceBlackWhite" },
/* XXX temporarily accept LONG for backwards compatibility */
    { TIFFTAG_REFERENCEBLACKWHITE,6,6,TIFF_LONG,	FIELD_REFBLACKWHITE,
      TRUE,	FALSE,	"ReferenceBlackWhite" },
#endif
/* begin SGI tags */
    { TIFFTAG_MATTEING,		 1, 1, TIFF_SHORT,	FIELD_EXTRASAMPLES,
      FALSE,	FALSE,	"Matteing" },
    { TIFFTAG_DATATYPE,		-2,-1, TIFF_SHORT,	FIELD_SAMPLEFORMAT,
      FALSE,	FALSE,	"DataType" },
    { TIFFTAG_IMAGEDEPTH,	 1, 1, TIFF_LONG,	FIELD_IMAGEDEPTH,
      FALSE,	FALSE,	"ImageDepth" },
    { TIFFTAG_IMAGEDEPTH,	 1, 1, TIFF_SHORT,	FIELD_IMAGEDEPTH,
      FALSE,	FALSE,	"ImageDepth" },
    { TIFFTAG_TILEDEPTH,	 1, 1, TIFF_LONG,	FIELD_TILEDEPTH,
      FALSE,	FALSE,	"TileDepth" },
    { TIFFTAG_TILEDEPTH,	 1, 1, TIFF_SHORT,	FIELD_TILEDEPTH,
      FALSE,	FALSE,	"TileDepth" },
/* end SGI tags */
/* begin Pixar tags */
    { TIFFTAG_PIXAR_IMAGEFULLWIDTH,  1, 1, TIFF_LONG,	FIELD_IMAGEFULLWIDTH,
      TRUE,	FALSE,	"ImageFullWidth" },
    { TIFFTAG_PIXAR_IMAGEFULLLENGTH, 1, 1, TIFF_LONG,	FIELD_IMAGEFULLLENGTH,
      TRUE,	FALSE,	"ImageFullLength" },
    { TIFFTAG_PIXAR_TEXTUREFORMAT,  -1,-1, TIFF_ASCII,	FIELD_TEXTUREFORMAT,
      TRUE,	FALSE,	"TextureFormat" },
    { TIFFTAG_PIXAR_WRAPMODES,	    -1,-1, TIFF_ASCII,	FIELD_WRAPMODES,
      TRUE,	FALSE,	"TextureWrapModes" },
    { TIFFTAG_PIXAR_FOVCOT,	     1, 1, TIFF_FLOAT,	FIELD_FOVCOT,
      TRUE,	FALSE,	"FieldOfViewCotan" },
    { TIFFTAG_PIXAR_MATRIX_WORLDTOSCREEN,	16,16,	TIFF_FLOAT,
      FIELD_MATRIX_WORLDTOSCREEN,	TRUE,	FALSE,	"MatrixWorldToScreen" },
    { TIFFTAG_PIXAR_MATRIX_WORLDTOCAMERA,	16,16,	TIFF_FLOAT,
       FIELD_MATRIX_WORLDTOCAMERA,	TRUE,	FALSE,	"MatrixWorldToCamera" },
    { TIFFTAG_COPYRIGHT,	-1,-1, TIFF_ASCII,	FIELD_COPYRIGHT,
      TRUE,	FALSE,	"Copyright" },
/* end Pixar tags */
#ifdef IPTC_SUPPORT
#ifdef PHOTOSHOP_SUPPORT
    { TIFFTAG_RICHTIFFIPTC, -1,-1, TIFF_LONG,   FIELD_RICHTIFFIPTC, 
      FALSE,    TRUE,   "RichTIFFIPTC" },
#else
    { TIFFTAG_RICHTIFFIPTC, -1,-3, TIFF_UNDEFINED, FIELD_RICHTIFFIPTC, 
      FALSE,    TRUE,   "RichTIFFIPTC" },
#endif
#endif
#ifdef PHOTOSHOP_SUPPORT
    { TIFFTAG_PHOTOSHOP,    -1,-3, TIFF_BYTE,   FIELD_PHOTOSHOP, 
      FALSE,    TRUE,   "Photoshop" },
#endif
#ifdef ICC_SUPPORT
    { TIFFTAG_ICCPROFILE,	-1,-3, TIFF_UNDEFINED,	FIELD_ICCPROFILE,
      FALSE,	TRUE,	"ICC Profile" },
#endif
    { TIFFTAG_STONITS,		 1, 1, TIFF_DOUBLE,	FIELD_STONITS,
      FALSE,	FALSE,	"StoNits" },
};
#define	N(a)	(sizeof (a) / sizeof (a[0]))

void
_TIFFSetupFieldInfo(TIFF* tif)
{
	if (tif->tif_fieldinfo) {
		_TIFFfree(tif->tif_fieldinfo);
		tif->tif_nfields = 0;
	}
	_TIFFMergeFieldInfo(tif, tiffFieldInfo, N(tiffFieldInfo));
}

static int
tagCompare(const void* a, const void* b)
{
	const TIFFFieldInfo* ta = *(const TIFFFieldInfo**) a;
	const TIFFFieldInfo* tb = *(const TIFFFieldInfo**) b;
	/* NB: be careful of return values for 16-bit platforms */
	if (ta->field_tag != tb->field_tag)
		return (ta->field_tag < tb->field_tag ? -1 : 1);
	else
		return (tb->field_type < ta->field_type ? -1 : 1);
}

void
_TIFFMergeFieldInfo(TIFF* tif, const TIFFFieldInfo info[], int n)
{
	TIFFFieldInfo** tp;
	int i;

	if (tif->tif_nfields > 0) {
		tif->tif_fieldinfo = (TIFFFieldInfo**)
		    _TIFFrealloc(tif->tif_fieldinfo,
			(tif->tif_nfields+n) * sizeof (TIFFFieldInfo*));
	} else {
		tif->tif_fieldinfo = (TIFFFieldInfo**)
		    _TIFFmalloc(n * sizeof (TIFFFieldInfo*));
	}
	tp = &tif->tif_fieldinfo[tif->tif_nfields];
	for (i = 0; i < n; i++)
		tp[i] = (TIFFFieldInfo*) &info[i];	/* XXX */
	/*
	 * NB: the core tags are presumed sorted correctly.
	 */
	if (tif->tif_nfields > 0)
		qsort(tif->tif_fieldinfo, (size_t) (tif->tif_nfields += n),
		    sizeof (TIFFFieldInfo*), tagCompare);
	else
		tif->tif_nfields += n;
}

void
_TIFFPrintFieldInfo(TIFF* tif, FILE* fd)
{
	int i;

	fprintf(fd, "%s: \n", tif->tif_name);
	for (i = 0; i < tif->tif_nfields; i++) {
		const TIFFFieldInfo* fip = tif->tif_fieldinfo[i];
		fprintf(fd, "field[%2d] %5lu, %2d, %2d, %d, %2d, %5s, %5s, %s\n"
			, i
			, (unsigned long) fip->field_tag
			, fip->field_readcount, fip->field_writecount
			, fip->field_type
			, fip->field_bit
			, fip->field_oktochange ? "TRUE" : "FALSE"
			, fip->field_passcount ? "TRUE" : "FALSE"
			, fip->field_name
		);
	}
}

const int tiffDataWidth[] = {
    1,	/* nothing */
    1,	/* TIFF_BYTE */
    1,	/* TIFF_ASCII */
    2,	/* TIFF_SHORT */
    4,	/* TIFF_LONG */
    8,	/* TIFF_RATIONAL */
    1,	/* TIFF_SBYTE */
    1,	/* TIFF_UNDEFINED */
    2,	/* TIFF_SSHORT */
    4,	/* TIFF_SLONG */
    8,	/* TIFF_SRATIONAL */
    4,	/* TIFF_FLOAT */
    8,	/* TIFF_DOUBLE */
};

/*
 * Return nearest TIFFDataType to the sample type of an image.
 */
TIFFDataType
_TIFFSampleToTagType(TIFF* tif)
{
	int bps = (int) TIFFhowmany(tif->tif_dir.td_bitspersample, 8);

	switch (tif->tif_dir.td_sampleformat) {
	case SAMPLEFORMAT_IEEEFP:
		return (bps == 4 ? TIFF_FLOAT : TIFF_DOUBLE);
	case SAMPLEFORMAT_INT:
		return (bps <= 1 ? TIFF_SBYTE :
		    bps <= 2 ? TIFF_SSHORT : TIFF_SLONG);
	case SAMPLEFORMAT_UINT:
		return (bps <= 1 ? TIFF_BYTE :
		    bps <= 2 ? TIFF_SHORT : TIFF_LONG);
	case SAMPLEFORMAT_VOID:
		return (TIFF_UNDEFINED);
	}
	/*NOTREACHED*/
	return (TIFF_UNDEFINED);
}

const TIFFFieldInfo*
_TIFFFindFieldInfo(TIFF* tif, ttag_t tag, TIFFDataType dt)
{
	static const TIFFFieldInfo *last = NULL;
	int i, n;

	if (last && last->field_tag == tag &&
	    (dt == TIFF_ANY || dt == last->field_type))
		return (last);
	/* NB: if table gets big, use sorted search (e.g. binary search) */
	for (i = 0, n = tif->tif_nfields; i < n; i++) {
		const TIFFFieldInfo* fip = tif->tif_fieldinfo[i];
		if (fip->field_tag == tag &&
		    (dt == TIFF_ANY || fip->field_type == dt))
			return (last = fip);
	}
	return ((const TIFFFieldInfo *)0);
}

#include <assert.h>
#include <stdio.h>

const TIFFFieldInfo*
_TIFFFieldWithTag(TIFF* tif, ttag_t tag)
{
	const TIFFFieldInfo* fip = _TIFFFindFieldInfo(tif, tag, TIFF_ANY);
	if (!fip) {
		TIFFError("TIFFFieldWithTag",
		    "Internal error, unknown tag 0x%x", (u_int) tag);
		assert(fip != NULL);
		/*NOTREACHED*/
	}
	return (fip);
}
