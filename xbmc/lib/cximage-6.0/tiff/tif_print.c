/* $Header: /cvsroot/osrs/libtiff/libtiff/tif_print.c,v 1.5 2001/03/02 04:59:52 warmerda Exp $ */

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
 * Directory Printing Support
 */
#include "tiffiop.h"
#include <stdio.h>

#include <ctype.h>

static const char *photoNames[] = {
    "min-is-white",				/* PHOTOMETRIC_MINISWHITE */
    "min-is-black",				/* PHOTOMETRIC_MINISBLACK */
    "RGB color",				/* PHOTOMETRIC_RGB */
    "palette color (RGB from colormap)",	/* PHOTOMETRIC_PALETTE */
    "transparency mask",			/* PHOTOMETRIC_MASK */
    "separated",				/* PHOTOMETRIC_SEPARATED */
    "YCbCr",					/* PHOTOMETRIC_YCBCR */
    "7 (0x7)",
    "CIE L*a*b*",				/* PHOTOMETRIC_CIELAB */
};
#define	NPHOTONAMES	(sizeof (photoNames) / sizeof (photoNames[0]))

static const char *orientNames[] = {
    "0 (0x0)",
    "row 0 top, col 0 lhs",			/* ORIENTATION_TOPLEFT */
    "row 0 top, col 0 rhs",			/* ORIENTATION_TOPRIGHT */
    "row 0 bottom, col 0 rhs",			/* ORIENTATION_BOTRIGHT */
    "row 0 bottom, col 0 lhs",			/* ORIENTATION_BOTLEFT */
    "row 0 lhs, col 0 top",			/* ORIENTATION_LEFTTOP */
    "row 0 rhs, col 0 top",			/* ORIENTATION_RIGHTTOP */
    "row 0 rhs, col 0 bottom",			/* ORIENTATION_RIGHTBOT */
    "row 0 lhs, col 0 bottom",			/* ORIENTATION_LEFTBOT */
};
#define	NORIENTNAMES	(sizeof (orientNames) / sizeof (orientNames[0]))

/*
 * Print the contents of the current directory
 * to the specified stdio file stream.
 */
void
TIFFPrintDirectory(TIFF* tif, FILE* fd, long flags)
{
	register TIFFDirectory *td;
	char *sep;
	uint16 i;
	long l, n;

	fprintf(fd, "TIFF Directory at offset 0x%lx\n", tif->tif_diroff);
	td = &tif->tif_dir;
	if (TIFFFieldSet(tif,FIELD_SUBFILETYPE)) {
		fprintf(fd, "  Subfile Type:");
		sep = " ";
		if (td->td_subfiletype & FILETYPE_REDUCEDIMAGE) {
			fprintf(fd, "%sreduced-resolution image", sep);
			sep = "/";
		}
		if (td->td_subfiletype & FILETYPE_PAGE) {
			fprintf(fd, "%smulti-page document", sep);
			sep = "/";
		}
		if (td->td_subfiletype & FILETYPE_MASK)
			fprintf(fd, "%stransparency mask", sep);
		fprintf(fd, " (%lu = 0x%lx)\n",
		    (long) td->td_subfiletype, (long) td->td_subfiletype);
	}
	if (TIFFFieldSet(tif,FIELD_IMAGEDIMENSIONS)) {
		fprintf(fd, "  Image Width: %lu Image Length: %lu",
		    (u_long) td->td_imagewidth, (u_long) td->td_imagelength);
		if (TIFFFieldSet(tif,FIELD_IMAGEDEPTH))
			fprintf(fd, " Image Depth: %lu",
			    (u_long) td->td_imagedepth);
		fprintf(fd, "\n");
	}

 	/* Begin Pixar */
 	if (TIFFFieldSet(tif,FIELD_IMAGEFULLWIDTH) ||
 	    TIFFFieldSet(tif,FIELD_IMAGEFULLLENGTH)) {
	  fprintf(fd, "  Pixar Full Image Width: %lu Full Image Length: %lu\n",
		  (u_long) td->td_imagefullwidth,
		  (u_long) td->td_imagefulllength);
 	}
 	if (TIFFFieldSet(tif,FIELD_TEXTUREFORMAT))
	  _TIFFprintAsciiTag(fd, "Texture Format", td->td_textureformat);
 	if (TIFFFieldSet(tif,FIELD_WRAPMODES))
	  _TIFFprintAsciiTag(fd, "Texture Wrap Modes", td->td_wrapmodes);
 	if (TIFFFieldSet(tif,FIELD_FOVCOT))
	  fprintf(fd, "  Field of View Cotangent: %g\n", td->td_fovcot);
	if (TIFFFieldSet(tif,FIELD_MATRIX_WORLDTOSCREEN)) {
	  typedef float	Matrix[4][4];
	  Matrix*		m = (Matrix*)td->td_matrixWorldToScreen;
	  
	  fprintf(fd, "  Matrix NP:\n\t%g %g %g %g\n\t%g %g %g %g\n\t%g %g %g %g\n\t%g %g %g %g\n",
		  (*m)[0][0], (*m)[0][1], (*m)[0][2], (*m)[0][3],
		  (*m)[1][0], (*m)[1][1], (*m)[1][2], (*m)[1][3],
		  (*m)[2][0], (*m)[2][1], (*m)[2][2], (*m)[2][3],
		  (*m)[3][0], (*m)[3][1], (*m)[3][2], (*m)[3][3]);
 	}
 	if (TIFFFieldSet(tif,FIELD_MATRIX_WORLDTOCAMERA)) {
	  typedef float	Matrix[4][4];
	  Matrix*		m = (Matrix*)td->td_matrixWorldToCamera;
	  
	  fprintf(fd, "  Matrix Nl:\n\t%g %g %g %g\n\t%g %g %g %g\n\t%g %g %g %g\n\t%g %g %g %g\n",
		  (*m)[0][0], (*m)[0][1], (*m)[0][2], (*m)[0][3],
		  (*m)[1][0], (*m)[1][1], (*m)[1][2], (*m)[1][3],
		  (*m)[2][0], (*m)[2][1], (*m)[2][2], (*m)[2][3],
		  (*m)[3][0], (*m)[3][1], (*m)[3][2], (*m)[3][3]);
 	}
 	/* End Pixar */
	
	if (TIFFFieldSet(tif,FIELD_TILEDIMENSIONS)) {
		fprintf(fd, "  Tile Width: %lu Tile Length: %lu",
		    (u_long) td->td_tilewidth, (u_long) td->td_tilelength);
		if (TIFFFieldSet(tif,FIELD_TILEDEPTH))
			fprintf(fd, " Tile Depth: %lu",
			    (u_long) td->td_tiledepth);
		fprintf(fd, "\n");
	}
	if (TIFFFieldSet(tif,FIELD_RESOLUTION)) {
		fprintf(fd, "  Resolution: %g, %g",
		    td->td_xresolution, td->td_yresolution);
		if (TIFFFieldSet(tif,FIELD_RESOLUTIONUNIT)) {
			switch (td->td_resolutionunit) {
			case RESUNIT_NONE:
				fprintf(fd, " (unitless)");
				break;
			case RESUNIT_INCH:
				fprintf(fd, " pixels/inch");
				break;
			case RESUNIT_CENTIMETER:
				fprintf(fd, " pixels/cm");
				break;
			default:
				fprintf(fd, " (unit %u = 0x%x)",
				    td->td_resolutionunit,
				    td->td_resolutionunit);
				break;
			}
		}
		fprintf(fd, "\n");
	}
	if (TIFFFieldSet(tif,FIELD_POSITION))
		fprintf(fd, "  Position: %g, %g\n",
		    td->td_xposition, td->td_yposition);
	if (TIFFFieldSet(tif,FIELD_BITSPERSAMPLE))
		fprintf(fd, "  Bits/Sample: %u\n", td->td_bitspersample);
	if (TIFFFieldSet(tif,FIELD_SAMPLEFORMAT)) {
		fprintf(fd, "  Sample Format: ");
		switch (td->td_sampleformat) {
		case SAMPLEFORMAT_VOID:
			fprintf(fd, "void\n");
			break;
		case SAMPLEFORMAT_INT:
			fprintf(fd, "signed integer\n");
			break;
		case SAMPLEFORMAT_UINT:
			fprintf(fd, "unsigned integer\n");
			break;
		case SAMPLEFORMAT_IEEEFP:
			fprintf(fd, "IEEE floating point\n");
			break;
		case SAMPLEFORMAT_COMPLEXINT:
			fprintf(fd, "complex signed integer\n");
			break;
		case SAMPLEFORMAT_COMPLEXIEEEFP:
			fprintf(fd, "complex IEEE floating point\n");
			break;
		default:
			fprintf(fd, "%u (0x%x)\n",
			    td->td_sampleformat, td->td_sampleformat);
			break;
		}
	}
	if (TIFFFieldSet(tif,FIELD_COMPRESSION)) {
		const TIFFCodec* c = TIFFFindCODEC(td->td_compression);
		fprintf(fd, "  Compression Scheme: ");
		if (c)
			fprintf(fd, "%s\n", c->name);
		else
			fprintf(fd, "%u (0x%x)\n",
			    td->td_compression, td->td_compression);
	}
	if (TIFFFieldSet(tif,FIELD_PHOTOMETRIC)) {
		fprintf(fd, "  Photometric Interpretation: ");
		if (td->td_photometric < NPHOTONAMES)
			fprintf(fd, "%s\n", photoNames[td->td_photometric]);
		else {
			switch (td->td_photometric) {
			case PHOTOMETRIC_LOGL:
				fprintf(fd, "CIE Log2(L)\n");
				break;
			case PHOTOMETRIC_LOGLUV:
				fprintf(fd, "CIE Log2(L) (u',v')\n");
				break;
			default:
				fprintf(fd, "%u (0x%x)\n",
				    td->td_photometric, td->td_photometric);
				break;
			}
		}
	}
	if (TIFFFieldSet(tif,FIELD_EXTRASAMPLES) && td->td_extrasamples) {
		fprintf(fd, "  Extra Samples: %u<", td->td_extrasamples);
		sep = "";
		for (i = 0; i < td->td_extrasamples; i++) {
			switch (td->td_sampleinfo[i]) {
			case EXTRASAMPLE_UNSPECIFIED:
				fprintf(fd, "%sunspecified", sep);
				break;
			case EXTRASAMPLE_ASSOCALPHA:
				fprintf(fd, "%sassoc-alpha", sep);
				break;
			case EXTRASAMPLE_UNASSALPHA:
				fprintf(fd, "%sunassoc-alpha", sep);
				break;
			default:
				fprintf(fd, "%s%u (0x%x)", sep,
				    td->td_sampleinfo[i], td->td_sampleinfo[i]);
				break;
			}
			sep = ", ";
		}
		fprintf(fd, ">\n");
	}
	if (TIFFFieldSet(tif,FIELD_STONITS)) {
		fprintf(fd, "  Sample to Nits conversion factor: %.4e\n",
				td->td_stonits);
	}
#ifdef CMYK_SUPPORT
	if (TIFFFieldSet(tif,FIELD_INKSET)) {
		fprintf(fd, "  Ink Set: ");
		switch (td->td_inkset) {
		case INKSET_CMYK:
			fprintf(fd, "CMYK\n");
			break;
		default:
			fprintf(fd, "%u (0x%x)\n",
			    td->td_inkset, td->td_inkset);
			break;
		}
	}
	if (TIFFFieldSet(tif,FIELD_INKNAMES)) {
		char* cp;
		fprintf(fd, "  Ink Names: ");
		i = td->td_samplesperpixel;
		sep = "";
		for (cp = td->td_inknames; i > 0; cp = strchr(cp,'\0')+1, i--) {
			fprintf(fd, "%s", sep);
			_TIFFprintAscii(fd, cp);
			sep = ", ";
		}
	}
	if (TIFFFieldSet(tif,FIELD_NUMBEROFINKS))
		fprintf(fd, " Number of Inks: %u\n", td->td_ninks);
	if (TIFFFieldSet(tif,FIELD_DOTRANGE))
		fprintf(fd, "  Dot Range: %u-%u\n",
		    td->td_dotrange[0], td->td_dotrange[1]);
	if (TIFFFieldSet(tif,FIELD_TARGETPRINTER))
		_TIFFprintAsciiTag(fd, "Target Printer", td->td_targetprinter);
#endif
	if (TIFFFieldSet(tif,FIELD_THRESHHOLDING)) {
		fprintf(fd, "  Thresholding: ");
		switch (td->td_threshholding) {
		case THRESHHOLD_BILEVEL:
			fprintf(fd, "bilevel art scan\n");
			break;
		case THRESHHOLD_HALFTONE:
			fprintf(fd, "halftone or dithered scan\n");
			break;
		case THRESHHOLD_ERRORDIFFUSE:
			fprintf(fd, "error diffused\n");
			break;
		default:
			fprintf(fd, "%u (0x%x)\n",
			    td->td_threshholding, td->td_threshholding);
			break;
		}
	}
	if (TIFFFieldSet(tif,FIELD_FILLORDER)) {
		fprintf(fd, "  FillOrder: ");
		switch (td->td_fillorder) {
		case FILLORDER_MSB2LSB:
			fprintf(fd, "msb-to-lsb\n");
			break;
		case FILLORDER_LSB2MSB:
			fprintf(fd, "lsb-to-msb\n");
			break;
		default:
			fprintf(fd, "%u (0x%x)\n",
			    td->td_fillorder, td->td_fillorder);
			break;
		}
	}
#ifdef YCBCR_SUPPORT
	if (TIFFFieldSet(tif,FIELD_YCBCRSUBSAMPLING))
		fprintf(fd, "  YCbCr Subsampling: %u, %u\n",
		    td->td_ycbcrsubsampling[0], td->td_ycbcrsubsampling[1]);
	if (TIFFFieldSet(tif,FIELD_YCBCRPOSITIONING)) {
		fprintf(fd, "  YCbCr Positioning: ");
		switch (td->td_ycbcrpositioning) {
		case YCBCRPOSITION_CENTERED:
			fprintf(fd, "centered\n");
			break;
		case YCBCRPOSITION_COSITED:
			fprintf(fd, "cosited\n");
			break;
		default:
			fprintf(fd, "%u (0x%x)\n",
			    td->td_ycbcrpositioning, td->td_ycbcrpositioning);
			break;
		}
	}
	if (TIFFFieldSet(tif,FIELD_YCBCRCOEFFICIENTS))
		fprintf(fd, "  YCbCr Coefficients: %g, %g, %g\n",
		    td->td_ycbcrcoeffs[0],
		    td->td_ycbcrcoeffs[1],
		    td->td_ycbcrcoeffs[2]);
#endif
	if (TIFFFieldSet(tif,FIELD_HALFTONEHINTS))
		fprintf(fd, "  Halftone Hints: light %u dark %u\n",
		    td->td_halftonehints[0], td->td_halftonehints[1]);
	if (TIFFFieldSet(tif,FIELD_ARTIST))
		_TIFFprintAsciiTag(fd, "Artist", td->td_artist);
	if (TIFFFieldSet(tif,FIELD_DATETIME))
		_TIFFprintAsciiTag(fd, "Date & Time", td->td_datetime);
	if (TIFFFieldSet(tif,FIELD_HOSTCOMPUTER))
		_TIFFprintAsciiTag(fd, "Host Computer", td->td_hostcomputer);
	if (TIFFFieldSet(tif,FIELD_SOFTWARE))
		_TIFFprintAsciiTag(fd, "Software", td->td_software);
	if (TIFFFieldSet(tif,FIELD_COPYRIGHT))
		_TIFFprintAsciiTag(fd, "Copyright", td->td_copyright);
	if (TIFFFieldSet(tif,FIELD_DOCUMENTNAME))
		_TIFFprintAsciiTag(fd, "Document Name", td->td_documentname);
	if (TIFFFieldSet(tif,FIELD_IMAGEDESCRIPTION))
		_TIFFprintAsciiTag(fd, "Image Description", td->td_imagedescription);
	if (TIFFFieldSet(tif,FIELD_MAKE))
		_TIFFprintAsciiTag(fd, "Make", td->td_make);
	if (TIFFFieldSet(tif,FIELD_MODEL))
		_TIFFprintAsciiTag(fd, "Model", td->td_model);
	if (TIFFFieldSet(tif,FIELD_ORIENTATION)) {
		fprintf(fd, "  Orientation: ");
		if (td->td_orientation < NORIENTNAMES)
			fprintf(fd, "%s\n", orientNames[td->td_orientation]);
		else
			fprintf(fd, "%u (0x%x)\n",
			    td->td_orientation, td->td_orientation);
	}
	if (TIFFFieldSet(tif,FIELD_SAMPLESPERPIXEL))
		fprintf(fd, "  Samples/Pixel: %u\n", td->td_samplesperpixel);
	if (TIFFFieldSet(tif,FIELD_ROWSPERSTRIP)) {
		fprintf(fd, "  Rows/Strip: ");
		if (td->td_rowsperstrip == (uint32) -1)
			fprintf(fd, "(infinite)\n");
		else
			fprintf(fd, "%lu\n", (u_long) td->td_rowsperstrip);
	}
	if (TIFFFieldSet(tif,FIELD_MINSAMPLEVALUE))
		fprintf(fd, "  Min Sample Value: %u\n", td->td_minsamplevalue);
	if (TIFFFieldSet(tif,FIELD_MAXSAMPLEVALUE))
		fprintf(fd, "  Max Sample Value: %u\n", td->td_maxsamplevalue);
	if (TIFFFieldSet(tif,FIELD_SMINSAMPLEVALUE))
		fprintf(fd, "  SMin Sample Value: %g\n",
		    td->td_sminsamplevalue);
	if (TIFFFieldSet(tif,FIELD_SMAXSAMPLEVALUE))
		fprintf(fd, "  SMax Sample Value: %g\n",
		    td->td_smaxsamplevalue);
	if (TIFFFieldSet(tif,FIELD_PLANARCONFIG)) {
		fprintf(fd, "  Planar Configuration: ");
		switch (td->td_planarconfig) {
		case PLANARCONFIG_CONTIG:
			fprintf(fd, "single image plane\n");
			break;
		case PLANARCONFIG_SEPARATE:
			fprintf(fd, "separate image planes\n");
			break;
		default:
			fprintf(fd, "%u (0x%x)\n",
			    td->td_planarconfig, td->td_planarconfig);
			break;
		}
	}
	if (TIFFFieldSet(tif,FIELD_PAGENAME))
		_TIFFprintAsciiTag(fd, "Page Name", td->td_pagename);
	if (TIFFFieldSet(tif,FIELD_PAGENUMBER))
		fprintf(fd, "  Page Number: %u-%u\n",
		    td->td_pagenumber[0], td->td_pagenumber[1]);
	if (TIFFFieldSet(tif,FIELD_COLORMAP)) {
		fprintf(fd, "  Color Map: ");
		if (flags & TIFFPRINT_COLORMAP) {
			fprintf(fd, "\n");
			n = 1L<<td->td_bitspersample;
			for (l = 0; l < n; l++)
				fprintf(fd, "   %5lu: %5u %5u %5u\n",
				    l,
				    td->td_colormap[0][l],
				    td->td_colormap[1][l],
				    td->td_colormap[2][l]);
		} else
			fprintf(fd, "(present)\n");
	}
#ifdef COLORIMETRY_SUPPORT
	if (TIFFFieldSet(tif,FIELD_WHITEPOINT))
		fprintf(fd, "  White Point: %g-%g\n",
		    td->td_whitepoint[0], td->td_whitepoint[1]);
	if (TIFFFieldSet(tif,FIELD_PRIMARYCHROMAS))
		fprintf(fd, "  Primary Chromaticities: %g,%g %g,%g %g,%g\n",
		    td->td_primarychromas[0], td->td_primarychromas[1],
		    td->td_primarychromas[2], td->td_primarychromas[3],
		    td->td_primarychromas[4], td->td_primarychromas[5]);
	if (TIFFFieldSet(tif,FIELD_REFBLACKWHITE)) {
		fprintf(fd, "  Reference Black/White:\n");
		for (i = 0; i < td->td_samplesperpixel; i++)
			fprintf(fd, "    %2d: %5g %5g\n",
			    i,
			    td->td_refblackwhite[2*i+0],
			    td->td_refblackwhite[2*i+1]);
	}
	if (TIFFFieldSet(tif,FIELD_TRANSFERFUNCTION)) {
		fprintf(fd, "  Transfer Function: ");
		if (flags & TIFFPRINT_CURVES) {
			fprintf(fd, "\n");
			n = 1L<<td->td_bitspersample;
			for (l = 0; l < n; l++) {
				fprintf(fd, "    %2lu: %5u",
				    l, td->td_transferfunction[0][l]);
				for (i = 1; i < td->td_samplesperpixel; i++)
					fprintf(fd, " %5u",
					    td->td_transferfunction[i][l]);
				fputc('\n', fd);
			}
		} else
			fprintf(fd, "(present)\n");
	}
#endif
#ifdef ICC_SUPPORT
	if (TIFFFieldSet(tif,FIELD_ICCPROFILE))
		fprintf(fd, "  ICC Profile: <present>, %lu bytes\n",
		    (u_long) td->td_profileLength);
#endif
#ifdef PHOTOSHOP_SUPPORT
 	if (TIFFFieldSet(tif,FIELD_PHOTOSHOP))
 		fprintf(fd, "  Photoshop Data: <present>, %lu bytes\n",
 		    (u_long) td->td_photoshopLength);
#endif
#ifdef IPTC_SUPPORT
 	if (TIFFFieldSet(tif,FIELD_RICHTIFFIPTC))
 		fprintf(fd, "  RichTIFFIPTC Data: <present>, %lu bytes\n",
 		    (u_long) td->td_richtiffiptcLength);
#endif
#if SUBIFD_SUPPORT
	if (TIFFFieldSet(tif, FIELD_SUBIFD)) {
		fprintf(fd, "  SubIFD Offsets:");
		for (i = 0; i < td->td_nsubifd; i++)
			fprintf(fd, " %5lu", (long) td->td_subifd[i]);
		fputc('\n', fd);
	}
#endif
	if (tif->tif_printdir)
		(*tif->tif_printdir)(tif, fd, flags);
	if ((flags & TIFFPRINT_STRIPS) &&
	    TIFFFieldSet(tif,FIELD_STRIPOFFSETS)) {
		tstrip_t s;

		fprintf(fd, "  %lu %s:\n",
		    (long) td->td_nstrips,
		    isTiled(tif) ? "Tiles" : "Strips");
		for (s = 0; s < td->td_nstrips; s++)
			fprintf(fd, "    %3lu: [%8lu, %8lu]\n",
			    (u_long) s,
			    (u_long) td->td_stripoffset[s],
			    (u_long) td->td_stripbytecount[s]);
	}
}

void
_TIFFprintAscii(FILE* fd, const char* cp)
{
	for (; *cp != '\0'; cp++) {
		const char* tp;

		if (isprint(*cp)) {
			fputc(*cp, fd);
			continue;
		}
		for (tp = "\tt\bb\rr\nn\vv"; *tp; tp++)
			if (*tp++ == *cp)
				break;
		if (*tp)
			fprintf(fd, "\\%c", *tp);
		else
			fprintf(fd, "\\%03o", *cp & 0xff);
	}
}

void
_TIFFprintAsciiTag(FILE* fd, const char* name, const char* value)
{
	fprintf(fd, "  %s: \"", name);
	_TIFFprintAscii(fd, value);
	fprintf(fd, "\"\n");
}
