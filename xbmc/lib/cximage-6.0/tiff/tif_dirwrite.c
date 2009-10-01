/* $Header: /cvsroot/osrs/libtiff/libtiff/tif_dirwrite.c,v 1.9 2001/09/26 17:42:18 warmerda Exp $ */

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
 * Directory Write Support Routines.
 */
#include "tiffiop.h"

#if HAVE_IEEEFP
#define	TIFFCvtNativeToIEEEFloat(tif, n, fp)
#define	TIFFCvtNativeToIEEEDouble(tif, n, dp)
#else
extern	void TIFFCvtNativeToIEEEFloat(TIFF*, uint32, float*);
extern	void TIFFCvtNativeToIEEEDouble(TIFF*, uint32, double*);
#endif

static	int TIFFWriteNormalTag(TIFF*, TIFFDirEntry*, const TIFFFieldInfo*);
static	void TIFFSetupShortLong(TIFF*, ttag_t, TIFFDirEntry*, uint32);
static	int TIFFSetupShortPair(TIFF*, ttag_t, TIFFDirEntry*);
static	int TIFFWritePerSampleShorts(TIFF*, ttag_t, TIFFDirEntry*);
static	int TIFFWritePerSampleAnys(TIFF*, TIFFDataType, ttag_t, TIFFDirEntry*);
static	int TIFFWriteShortTable(TIFF*, ttag_t, TIFFDirEntry*, uint32, uint16**);
static	int TIFFWriteShortArray(TIFF*,
	    TIFFDataType, ttag_t, TIFFDirEntry*, uint32, uint16*);
static	int TIFFWriteLongArray(TIFF *,
	    TIFFDataType, ttag_t, TIFFDirEntry*, uint32, uint32*);
static	int TIFFWriteRationalArray(TIFF *,
	    TIFFDataType, ttag_t, TIFFDirEntry*, uint32, float*);
static	int TIFFWriteFloatArray(TIFF *,
	    TIFFDataType, ttag_t, TIFFDirEntry*, uint32, float*);
static	int TIFFWriteDoubleArray(TIFF *,
	    TIFFDataType, ttag_t, TIFFDirEntry*, uint32, double*);
static	int TIFFWriteByteArray(TIFF*, TIFFDirEntry*, char*);
static	int TIFFWriteAnyArray(TIFF*,
	    TIFFDataType, ttag_t, TIFFDirEntry*, uint32, double*);
#ifdef COLORIMETRY_SUPPORT
static	int TIFFWriteTransferFunction(TIFF*, TIFFDirEntry*);
#endif
#ifdef CMYK_SUPPORT
static	int TIFFWriteInkNames(TIFF*, TIFFDirEntry*);
#endif
static	int TIFFWriteData(TIFF*, TIFFDirEntry*, char*);
static	int TIFFLinkDirectory(TIFF*);

#define	WriteRationalPair(type, tag1, v1, tag2, v2) {		\
	if (!TIFFWriteRational(tif, type, tag1, dir, v1))	\
		goto bad;					\
	if (!TIFFWriteRational(tif, type, tag2, dir+1, v2))	\
		goto bad;					\
	dir++;							\
}
#define	TIFFWriteRational(tif, type, tag, dir, v) \
	TIFFWriteRationalArray((tif), (type), (tag), (dir), 1, &(v))
#ifndef TIFFWriteRational
static	int TIFFWriteRational(TIFF*,
	    TIFFDataType, ttag_t, TIFFDirEntry*, float);
#endif

/*
 * Write the contents of the current directory
 * to the specified file.  This routine doesn't
 * handle overwriting a directory with auxiliary
 * storage that's been changed.
 */
int
TIFFWriteDirectory(TIFF* tif)
{
	uint16 dircount;
	toff_t diroff;
	ttag_t tag;
	uint32 nfields;
	tsize_t dirsize;
	char* data;
	TIFFDirEntry* dir;
	TIFFDirectory* td;
	u_long b, fields[FIELD_SETLONGS];
	int fi, nfi;

	if (tif->tif_mode == O_RDONLY)
		return (1);
	/*
	 * Clear write state so that subsequent images with
	 * different characteristics get the right buffers
	 * setup for them.
	 */
	if (tif->tif_flags & TIFF_POSTENCODE) {
		tif->tif_flags &= ~TIFF_POSTENCODE;
		if (!(*tif->tif_postencode)(tif)) {
			TIFFError(tif->tif_name,
			    "Error post-encoding before directory write");
			return (0);
		}
	}
	(*tif->tif_close)(tif);			/* shutdown encoder */
	/*
	 * Flush any data that might have been written
	 * by the compression close+cleanup routines.
	 */
	if (tif->tif_rawcc > 0 && !TIFFFlushData1(tif)) {
		TIFFError(tif->tif_name,
		    "Error flushing data before directory write");
		return (0);
	}
	if ((tif->tif_flags & TIFF_MYBUFFER) && tif->tif_rawdata) {
		_TIFFfree(tif->tif_rawdata);
		tif->tif_rawdata = NULL;
		tif->tif_rawcc = 0;
                tif->tif_rawdatasize = 0;
	}
	tif->tif_flags &= ~(TIFF_BEENWRITING|TIFF_BUFFERSETUP);

	td = &tif->tif_dir;
	/*
	 * Size the directory so that we can calculate
	 * offsets for the data items that aren't kept
	 * in-place in each field.
	 */
	nfields = 0;
	for (b = 0; b <= FIELD_LAST; b++)
		if (TIFFFieldSet(tif, b))
			nfields += (b < FIELD_SUBFILETYPE ? 2 : 1);
	dirsize = nfields * sizeof (TIFFDirEntry);
	data = (char*) _TIFFmalloc(dirsize);
	if (data == NULL) {
		TIFFError(tif->tif_name,
		    "Cannot write directory, out of space");
		return (0);
	}
	/*
	 * Directory hasn't been placed yet, put
	 * it at the end of the file and link it
	 * into the existing directory structure.
	 */
	if (tif->tif_diroff == 0 && !TIFFLinkDirectory(tif))
		goto bad;
	tif->tif_dataoff = (toff_t)(
	    tif->tif_diroff + sizeof (uint16) + dirsize + sizeof (toff_t));
	if (tif->tif_dataoff & 1)
		tif->tif_dataoff++;
	(void) TIFFSeekFile(tif, tif->tif_dataoff, SEEK_SET);
	tif->tif_curdir++;
	dir = (TIFFDirEntry*) data;
	/*
	 * Setup external form of directory
	 * entries and write data items.
	 */
	_TIFFmemcpy(fields, td->td_fieldsset, sizeof (fields));
	/*
	 * Write out ExtraSamples tag only if
	 * extra samples are present in the data.
	 */
	if (FieldSet(fields, FIELD_EXTRASAMPLES) && !td->td_extrasamples) {
		ResetFieldBit(fields, FIELD_EXTRASAMPLES);
		nfields--;
		dirsize -= sizeof (TIFFDirEntry);
	}								/*XXX*/
	for (fi = 0, nfi = tif->tif_nfields; nfi > 0; nfi--, fi++) {
		const TIFFFieldInfo* fip = tif->tif_fieldinfo[fi];
		if (!FieldSet(fields, fip->field_bit))
			continue;
		switch (fip->field_bit) {
		case FIELD_STRIPOFFSETS:
			/*
			 * We use one field bit for both strip and tile
			 * offsets, and so must be careful in selecting
			 * the appropriate field descriptor (so that tags
			 * are written in sorted order).
			 */
			tag = isTiled(tif) ?
			    TIFFTAG_TILEOFFSETS : TIFFTAG_STRIPOFFSETS;
			if (tag != fip->field_tag)
				continue;
			if (!TIFFWriteLongArray(tif, TIFF_LONG, tag, dir,
			    (uint32) td->td_nstrips, td->td_stripoffset))
				goto bad;
			break;
		case FIELD_STRIPBYTECOUNTS:
			/*
			 * We use one field bit for both strip and tile
			 * byte counts, and so must be careful in selecting
			 * the appropriate field descriptor (so that tags
			 * are written in sorted order).
			 */
			tag = isTiled(tif) ?
			    TIFFTAG_TILEBYTECOUNTS : TIFFTAG_STRIPBYTECOUNTS;
			if (tag != fip->field_tag)
				continue;
			if (!TIFFWriteLongArray(tif, TIFF_LONG, tag, dir,
			    (uint32) td->td_nstrips, td->td_stripbytecount))
				goto bad;
			break;
		case FIELD_ROWSPERSTRIP:
			TIFFSetupShortLong(tif, TIFFTAG_ROWSPERSTRIP,
			    dir, td->td_rowsperstrip);
			break;
		case FIELD_COLORMAP:
			if (!TIFFWriteShortTable(tif, TIFFTAG_COLORMAP, dir,
			    3, td->td_colormap))
				goto bad;
			break;
		case FIELD_IMAGEDIMENSIONS:
			TIFFSetupShortLong(tif, TIFFTAG_IMAGEWIDTH,
			    dir++, td->td_imagewidth);
			TIFFSetupShortLong(tif, TIFFTAG_IMAGELENGTH,
			    dir, td->td_imagelength);
			break;
		case FIELD_TILEDIMENSIONS:
			TIFFSetupShortLong(tif, TIFFTAG_TILEWIDTH,
			    dir++, td->td_tilewidth);
			TIFFSetupShortLong(tif, TIFFTAG_TILELENGTH,
			    dir, td->td_tilelength);
			break;
		case FIELD_POSITION:
			WriteRationalPair(TIFF_RATIONAL,
			    TIFFTAG_XPOSITION, td->td_xposition,
			    TIFFTAG_YPOSITION, td->td_yposition);
			break;
		case FIELD_RESOLUTION:
			WriteRationalPair(TIFF_RATIONAL,
			    TIFFTAG_XRESOLUTION, td->td_xresolution,
			    TIFFTAG_YRESOLUTION, td->td_yresolution);
			break;
		case FIELD_BITSPERSAMPLE:
		case FIELD_MINSAMPLEVALUE:
		case FIELD_MAXSAMPLEVALUE:
		case FIELD_SAMPLEFORMAT:
			if (!TIFFWritePerSampleShorts(tif, fip->field_tag, dir))
				goto bad;
			break;
		case FIELD_SMINSAMPLEVALUE:
		case FIELD_SMAXSAMPLEVALUE:
			if (!TIFFWritePerSampleAnys(tif,
			    _TIFFSampleToTagType(tif), fip->field_tag, dir))
				goto bad;
			break;
		case FIELD_PAGENUMBER:
		case FIELD_HALFTONEHINTS:
#ifdef YCBCR_SUPPORT
		case FIELD_YCBCRSUBSAMPLING:
#endif
#ifdef CMYK_SUPPORT
		case FIELD_DOTRANGE:
#endif
			if (!TIFFSetupShortPair(tif, fip->field_tag, dir))
				goto bad;
			break;
#ifdef CMYK_SUPPORT
		case FIELD_INKNAMES:
			if (!TIFFWriteInkNames(tif, dir))
				goto bad;
			break;
#endif
#ifdef COLORIMETRY_SUPPORT
		case FIELD_TRANSFERFUNCTION:
			if (!TIFFWriteTransferFunction(tif, dir))
				goto bad;
			break;
#endif
#if SUBIFD_SUPPORT
		case FIELD_SUBIFD:
			if (!TIFFWriteNormalTag(tif, dir, fip))
				goto bad;
			/*
			 * Total hack: if this directory includes a SubIFD
			 * tag then force the next <n> directories to be
			 * written as ``sub directories'' of this one.  This
			 * is used to write things like thumbnails and
			 * image masks that one wants to keep out of the
			 * normal directory linkage access mechanism.
			 */
			if (dir->tdir_count > 0) {
				tif->tif_flags |= TIFF_INSUBIFD;
				tif->tif_nsubifd = (uint16) dir->tdir_count;
				if (dir->tdir_count > 1)
					tif->tif_subifdoff = dir->tdir_offset;
				else
					tif->tif_subifdoff = (uint32)(
					      tif->tif_diroff
					    + sizeof (uint16)
					    + ((char*)&dir->tdir_offset-data));
			}
			break;
#endif
		default:
			if (!TIFFWriteNormalTag(tif, dir, fip))
				goto bad;
			break;
		}
		dir++;
		ResetFieldBit(fields, fip->field_bit);
	}
	/*
	 * Write directory.
	 */
	dircount = (uint16) nfields;
	diroff = (uint32) tif->tif_nextdiroff;
	if (tif->tif_flags & TIFF_SWAB) {
		/*
		 * The file's byte order is opposite to the
		 * native machine architecture.  We overwrite
		 * the directory information with impunity
		 * because it'll be released below after we
		 * write it to the file.  Note that all the
		 * other tag construction routines assume that
		 * we do this byte-swapping; i.e. they only
		 * byte-swap indirect data.
		 */
		for (dir = (TIFFDirEntry*) data; dircount; dir++, dircount--) {
			TIFFSwabArrayOfShort(&dir->tdir_tag, 2);
			TIFFSwabArrayOfLong(&dir->tdir_count, 2);
		}
		dircount = (uint16) nfields;
		TIFFSwabShort(&dircount);
		TIFFSwabLong(&diroff);
	}
	(void) TIFFSeekFile(tif, tif->tif_diroff, SEEK_SET);
	if (!WriteOK(tif, &dircount, sizeof (dircount))) {
		TIFFError(tif->tif_name, "Error writing directory count");
		goto bad;
	}
	if (!WriteOK(tif, data, dirsize)) {
		TIFFError(tif->tif_name, "Error writing directory contents");
		goto bad;
	}
	if (!WriteOK(tif, &diroff, sizeof (diroff))) {
		TIFFError(tif->tif_name, "Error writing directory link");
		goto bad;
	}
	TIFFFreeDirectory(tif);
	_TIFFfree(data);
	tif->tif_flags &= ~TIFF_DIRTYDIRECT;
	(*tif->tif_cleanup)(tif);

	/*
	 * Reset directory-related state for subsequent
	 * directories.
	 */
        TIFFCreateDirectory(tif);
	return (1);
bad:
	_TIFFfree(data);
	return (0);
}
#undef WriteRationalPair

/*
 * Process tags that are not special cased.
 */
static int
TIFFWriteNormalTag(TIFF* tif, TIFFDirEntry* dir, const TIFFFieldInfo* fip)
{
	u_short wc = (u_short) fip->field_writecount;
	uint32 wc2;

	dir->tdir_tag = (uint16) fip->field_tag;
	dir->tdir_type = (u_short) fip->field_type;
	dir->tdir_count = wc;
#define	WRITEF(x,y)	x(tif, fip->field_type, fip->field_tag, dir, wc, y)
	switch (fip->field_type) {
	case TIFF_SHORT:
	case TIFF_SSHORT:
		if (wc > 1) {
			uint16* wp;
			if (wc == (u_short) TIFF_VARIABLE)
				TIFFGetField(tif, fip->field_tag, &wc, &wp);
			else
				TIFFGetField(tif, fip->field_tag, &wp);
			if (!WRITEF(TIFFWriteShortArray, wp))
				return (0);
		} else {
			uint16 sv;
			TIFFGetField(tif, fip->field_tag, &sv);
			dir->tdir_offset =
			    TIFFInsertData(tif, dir->tdir_type, sv);
		}
		break;
	case TIFF_LONG:
	case TIFF_SLONG:
		if (wc > 1) {
			uint32* lp;
			if (wc == (u_short) TIFF_VARIABLE)
				TIFFGetField(tif, fip->field_tag, &wc, &lp);
			else
				TIFFGetField(tif, fip->field_tag, &lp);
			if (!WRITEF(TIFFWriteLongArray, lp))
				return (0);
		} else {
			/* XXX handle LONG->SHORT conversion */
			TIFFGetField(tif, fip->field_tag, &dir->tdir_offset);
		}
		break;
	case TIFF_RATIONAL:
	case TIFF_SRATIONAL:
		if (wc > 1) {
			float* fp;
			if (wc == (u_short) TIFF_VARIABLE)
				TIFFGetField(tif, fip->field_tag, &wc, &fp);
			else
				TIFFGetField(tif, fip->field_tag, &fp);
			if (!WRITEF(TIFFWriteRationalArray, fp))
				return (0);
		} else {
			float fv;
			TIFFGetField(tif, fip->field_tag, &fv);
			if (!WRITEF(TIFFWriteRationalArray, &fv))
				return (0);
		}
		break;
	case TIFF_FLOAT:
		if (wc > 1) {
			float* fp;
			if (wc == (u_short) TIFF_VARIABLE)
				TIFFGetField(tif, fip->field_tag, &wc, &fp);
			else
				TIFFGetField(tif, fip->field_tag, &fp);
			if (!WRITEF(TIFFWriteFloatArray, fp))
				return (0);
		} else {
			float fv;
			TIFFGetField(tif, fip->field_tag, &fv);
			if (!WRITEF(TIFFWriteFloatArray, &fv))
				return (0);
		}
		break;
	case TIFF_DOUBLE:
		if (wc > 1) {
			double* dp;
			if (wc == (u_short) TIFF_VARIABLE)
				TIFFGetField(tif, fip->field_tag, &wc, &dp);
			else
				TIFFGetField(tif, fip->field_tag, &dp);
			if (!WRITEF(TIFFWriteDoubleArray, dp))
				return (0);
		} else {
			double dv;
			TIFFGetField(tif, fip->field_tag, &dv);
			if (!WRITEF(TIFFWriteDoubleArray, &dv))
				return (0);
		}
		break;
	case TIFF_ASCII:
		{ char* cp;
		  TIFFGetField(tif, fip->field_tag, &cp);
		  dir->tdir_count = (uint32) (strlen(cp) + 1);
		  if (!TIFFWriteByteArray(tif, dir, cp))
			return (0);
		}
		break;

        /* added based on patch request from MARTIN.MCBRIDE.MM@agfa.co.uk,
           correctness not verified (FW, 99/08) */
        case TIFF_BYTE:
        case TIFF_SBYTE:          
                if (wc > 1) {
                    char* cp;
                    if (wc == (u_short) TIFF_VARIABLE) {
                        TIFFGetField(tif, fip->field_tag, &wc, &cp);
                        dir->tdir_count = wc;
                    } else if (wc == (u_short) TIFF_VARIABLE2) {
			TIFFGetField(tif, fip->field_tag, &wc2, &cp);
			dir->tdir_count = wc2;
                    } else
                        TIFFGetField(tif, fip->field_tag, &cp);
                    if (!TIFFWriteByteArray(tif, dir, cp))
                        return (0);
                } else {
                    char cv;
                    TIFFGetField(tif, fip->field_tag, &cv);
                    if (!TIFFWriteByteArray(tif, dir, &cv))
                        return (0);
                }
                break;

	case TIFF_UNDEFINED:
		{ char* cp;
		  if (wc == (u_short) TIFF_VARIABLE) {
			TIFFGetField(tif, fip->field_tag, &wc, &cp);
			dir->tdir_count = wc;
		  } else if (wc == (u_short) TIFF_VARIABLE2) {
			TIFFGetField(tif, fip->field_tag, &wc2, &cp);
			dir->tdir_count = wc2;
		  } else 
			TIFFGetField(tif, fip->field_tag, &cp);
		  if (!TIFFWriteByteArray(tif, dir, cp))
			return (0);
		}
		break;

        case TIFF_NOTYPE:
                break;
	}
	return (1);
}
#undef WRITEF

/*
 * Setup a directory entry with either a SHORT
 * or LONG type according to the value.
 */
static void
TIFFSetupShortLong(TIFF* tif, ttag_t tag, TIFFDirEntry* dir, uint32 v)
{
	dir->tdir_tag = (uint16) tag;
	dir->tdir_count = 1;
	if (v > 0xffffL) {
		dir->tdir_type = (short) TIFF_LONG;
		dir->tdir_offset = v;
	} else {
		dir->tdir_type = (short) TIFF_SHORT;
		dir->tdir_offset = TIFFInsertData(tif, (int) TIFF_SHORT, v);
	}
}
#undef MakeShortDirent

#ifndef TIFFWriteRational
/*
 * Setup a RATIONAL directory entry and
 * write the associated indirect value.
 */
static int
TIFFWriteRational(TIFF* tif,
    TIFFDataType type, ttag_t tag, TIFFDirEntry* dir, float v)
{
	return (TIFFWriteRationalArray(tif, type, tag, dir, 1, &v));
}
#endif

#define	NITEMS(x)	(sizeof (x) / sizeof (x[0]))
/*
 * Setup a directory entry that references a
 * samples/pixel array of SHORT values and
 * (potentially) write the associated indirect
 * values.
 */
static int
TIFFWritePerSampleShorts(TIFF* tif, ttag_t tag, TIFFDirEntry* dir)
{
	uint16 buf[10], v;
	uint16* w = buf;
	int i, status, samples = tif->tif_dir.td_samplesperpixel;

	if (samples > NITEMS(buf))
		w = (uint16*) _TIFFmalloc(samples * sizeof (uint16));
	TIFFGetField(tif, tag, &v);
	for (i = 0; i < samples; i++)
		w[i] = v;
	status = TIFFWriteShortArray(tif, TIFF_SHORT, tag, dir, samples, w);
	if (w != buf)
		_TIFFfree((char*) w);
	return (status);
}

/*
 * Setup a directory entry that references a samples/pixel array of ``type''
 * values and (potentially) write the associated indirect values.  The source
 * data from TIFFGetField() for the specified tag must be returned as double.
 */
static int
TIFFWritePerSampleAnys(TIFF* tif,
    TIFFDataType type, ttag_t tag, TIFFDirEntry* dir)
{
	double buf[10], v;
	double* w = buf;
	int i, status;
	int samples = (int) tif->tif_dir.td_samplesperpixel;

	if (samples > NITEMS(buf))
		w = (double*) _TIFFmalloc(samples * sizeof (double));
	TIFFGetField(tif, tag, &v);
	for (i = 0; i < samples; i++)
		w[i] = v;
	status = TIFFWriteAnyArray(tif, type, tag, dir, samples, w);
	if (w != buf)
		_TIFFfree(w);
	return (status);
}
#undef NITEMS

/*
 * Setup a pair of shorts that are returned by
 * value, rather than as a reference to an array.
 */
static int
TIFFSetupShortPair(TIFF* tif, ttag_t tag, TIFFDirEntry* dir)
{
	uint16 v[2];

	TIFFGetField(tif, tag, &v[0], &v[1]);
	return (TIFFWriteShortArray(tif, TIFF_SHORT, tag, dir, 2, v));
}

/*
 * Setup a directory entry for an NxM table of shorts,
 * where M is known to be 2**bitspersample, and write
 * the associated indirect data.
 */
static int
TIFFWriteShortTable(TIFF* tif,
    ttag_t tag, TIFFDirEntry* dir, uint32 n, uint16** table)
{
	uint32 i, off;

	dir->tdir_tag = (uint16) tag;
	dir->tdir_type = (short) TIFF_SHORT;
	/* XXX -- yech, fool TIFFWriteData */
	dir->tdir_count = (uint32) (1L<<tif->tif_dir.td_bitspersample);
	off = tif->tif_dataoff;
	for (i = 0; i < n; i++)
		if (!TIFFWriteData(tif, dir, (char *)table[i]))
			return (0);
	dir->tdir_count *= n;
	dir->tdir_offset = off;
	return (1);
}

/*
 * Write/copy data associated with an ASCII or opaque tag value.
 */
static int
TIFFWriteByteArray(TIFF* tif, TIFFDirEntry* dir, char* cp)
{
	if (dir->tdir_count > 4) {
		if (!TIFFWriteData(tif, dir, cp))
			return (0);
	} else
		_TIFFmemcpy(&dir->tdir_offset, cp, dir->tdir_count);
	return (1);
}

/*
 * Setup a directory entry of an array of SHORT
 * or SSHORT and write the associated indirect values.
 */
static int
TIFFWriteShortArray(TIFF* tif,
    TIFFDataType type, ttag_t tag, TIFFDirEntry* dir, uint32 n, uint16* v)
{
	dir->tdir_tag = (uint16) tag;
	dir->tdir_type = (short) type;
	dir->tdir_count = n;
	if (n <= 2) {
		if (tif->tif_header.tiff_magic == TIFF_BIGENDIAN) {
			dir->tdir_offset = (uint32) ((long) v[0] << 16);
			if (n == 2)
				dir->tdir_offset |= v[1] & 0xffff;
		} else {
			dir->tdir_offset = v[0] & 0xffff;
			if (n == 2)
				dir->tdir_offset |= (long) v[1] << 16;
		}
		return (1);
	} else
		return (TIFFWriteData(tif, dir, (char*) v));
}

/*
 * Setup a directory entry of an array of LONG
 * or SLONG and write the associated indirect values.
 */
static int
TIFFWriteLongArray(TIFF* tif,
    TIFFDataType type, ttag_t tag, TIFFDirEntry* dir, uint32 n, uint32* v)
{
	dir->tdir_tag = (uint16) tag;
	dir->tdir_type = (short) type;
	dir->tdir_count = n;
	if (n == 1) {
		dir->tdir_offset = v[0];
		return (1);
	} else
		return (TIFFWriteData(tif, dir, (char*) v));
}

/*
 * Setup a directory entry of an array of RATIONAL
 * or SRATIONAL and write the associated indirect values.
 */
static int
TIFFWriteRationalArray(TIFF* tif,
    TIFFDataType type, ttag_t tag, TIFFDirEntry* dir, uint32 n, float* v)
{
	uint32 i;
	uint32* t;
	int status;

	dir->tdir_tag = (uint16) tag;
	dir->tdir_type = (short) type;
	dir->tdir_count = n;
	t = (uint32*) _TIFFmalloc(2*n * sizeof (uint32));
	for (i = 0; i < n; i++) {
		float fv = v[i];
		int sign = 1;
		uint32 den;

		if (fv < 0) {
			if (type == TIFF_RATIONAL) {
				TIFFWarning(tif->tif_name,
	"\"%s\": Information lost writing value (%g) as (unsigned) RATIONAL",
				_TIFFFieldWithTag(tif,tag)->field_name, fv);
				fv = 0;
			} else
				fv = -fv, sign = -1;
		}
		den = 1L;
		if (fv > 0) {
			while (fv < 1L<<(31-3) && den < 1L<<(31-3))
				fv *= 1<<3, den *= 1L<<3;
		}
		t[2*i+0] = (uint32) (sign * (fv + 0.5));
		t[2*i+1] = den;
	}
	status = TIFFWriteData(tif, dir, (char *)t);
	_TIFFfree((char*) t);
	return (status);
}

static int
TIFFWriteFloatArray(TIFF* tif,
    TIFFDataType type, ttag_t tag, TIFFDirEntry* dir, uint32 n, float* v)
{
	dir->tdir_tag = (uint16) tag;
	dir->tdir_type = (short) type;
	dir->tdir_count = n;
	TIFFCvtNativeToIEEEFloat(tif, n, v);
	if (n == 1) {
		dir->tdir_offset = *(uint32*) &v[0];
		return (1);
	} else
		return (TIFFWriteData(tif, dir, (char*) v));
}

static int
TIFFWriteDoubleArray(TIFF* tif,
    TIFFDataType type, ttag_t tag, TIFFDirEntry* dir, uint32 n, double* v)
{
	dir->tdir_tag = (uint16) tag;
	dir->tdir_type = (short) type;
	dir->tdir_count = n;
	TIFFCvtNativeToIEEEDouble(tif, n, v);
	return (TIFFWriteData(tif, dir, (char*) v));
}

/*
 * Write an array of ``type'' values for a specified tag (i.e. this is a tag
 * which is allowed to have different types, e.g. SMaxSampleType).
 * Internally the data values are represented as double since a double can
 * hold any of the TIFF tag types (yes, this should really be an abstract
 * type tany_t for portability).  The data is converted into the specified
 * type in a temporary buffer and then handed off to the appropriate array
 * writer.
 */
static int
TIFFWriteAnyArray(TIFF* tif,
    TIFFDataType type, ttag_t tag, TIFFDirEntry* dir, uint32 n, double* v)
{
	char buf[10 * sizeof(double)];
	char* w = buf;
	int i, status = 0;

	if (n * tiffDataWidth[type] > sizeof buf)
		w = (char*) _TIFFmalloc(n * tiffDataWidth[type]);
	switch (type) {
	case TIFF_BYTE:
		{ uint8* bp = (uint8*) w;
		  for (i = 0; i < (int) n; i++)
			bp[i] = (uint8) v[i];
		  dir->tdir_tag = (uint16) tag;
		  dir->tdir_type = (short) type;
		  dir->tdir_count = n;
		  if (!TIFFWriteByteArray(tif, dir, (char*) bp))
			goto out;
		}
		break;
	case TIFF_SBYTE:
		{ int8* bp = (int8*) w;
		  for (i = 0; i < (int) n; i++)
			bp[i] = (int8) v[i];
		  dir->tdir_tag = (uint16) tag;
		  dir->tdir_type = (short) type;
		  dir->tdir_count = n;
		  if (!TIFFWriteByteArray(tif, dir, (char*) bp))
			goto out;
		}
		break;
	case TIFF_SHORT:
		{ uint16* bp = (uint16*) w;
		  for (i = 0; i < (int) n; i++)
			bp[i] = (uint16) v[i];
		  if (!TIFFWriteShortArray(tif, type, tag, dir, n, (uint16*)bp))
				goto out;
		}
		break;
	case TIFF_SSHORT:
		{ int16* bp = (int16*) w;
		  for (i = 0; i < (int) n; i++)
			bp[i] = (int16) v[i];
		  if (!TIFFWriteShortArray(tif, type, tag, dir, n, (uint16*)bp))
			goto out;
		}
		break;
	case TIFF_LONG:
		{ uint32* bp = (uint32*) w;
		  for (i = 0; i < (int) n; i++)
			bp[i] = (uint32) v[i];
		  if (!TIFFWriteLongArray(tif, type, tag, dir, n, bp))
			goto out;
		}
		break;
	case TIFF_SLONG:
		{ int32* bp = (int32*) w;
		  for (i = 0; i < (int) n; i++)
			bp[i] = (int32) v[i];
		  if (!TIFFWriteLongArray(tif, type, tag, dir, n, (uint32*) bp))
			goto out;
		}
		break;
	case TIFF_FLOAT:
		{ float* bp = (float*) w;
		  for (i = 0; i < (int) n; i++)
			bp[i] = (float) v[i];
		  if (!TIFFWriteFloatArray(tif, type, tag, dir, n, bp))
			goto out;
		}
		break;
	case TIFF_DOUBLE:
		return (TIFFWriteDoubleArray(tif, type, tag, dir, n, v));
	default:
		/* TIFF_NOTYPE */
		/* TIFF_ASCII */
		/* TIFF_UNDEFINED */
		/* TIFF_RATIONAL */
		/* TIFF_SRATIONAL */
		goto out;
	}
	status = 1;
 out:
	if (w != buf)
		_TIFFfree(w);
	return (status);
}

#ifdef COLORIMETRY_SUPPORT
static int
TIFFWriteTransferFunction(TIFF* tif, TIFFDirEntry* dir)
{
	TIFFDirectory* td = &tif->tif_dir;
	tsize_t n = (1L<<td->td_bitspersample) * sizeof (uint16);
	uint16** tf = td->td_transferfunction;
	int ncols;

	/*
	 * Check if the table can be written as a single column,
	 * or if it must be written as 3 columns.  Note that we
	 * write a 3-column tag if there are 2 samples/pixel and
	 * a single column of data won't suffice--hmm.
	 */
	switch (td->td_samplesperpixel - td->td_extrasamples) {
	default:	if (_TIFFmemcmp(tf[0], tf[2], n)) { ncols = 3; break; }
	case 2:		if (_TIFFmemcmp(tf[0], tf[1], n)) { ncols = 3; break; }
	case 1: case 0:	ncols = 1;
	}
	return (TIFFWriteShortTable(tif,
	    TIFFTAG_TRANSFERFUNCTION, dir, ncols, tf));
}
#endif

#ifdef CMYK_SUPPORT
static int
TIFFWriteInkNames(TIFF* tif, TIFFDirEntry* dir)
{
	TIFFDirectory* td = &tif->tif_dir;

	dir->tdir_tag = TIFFTAG_INKNAMES;
	dir->tdir_type = (short) TIFF_ASCII;
	dir->tdir_count = td->td_inknameslen;
	return (TIFFWriteByteArray(tif, dir, td->td_inknames));
}
#endif

/*
 * Write a contiguous directory item.
 */
static int
TIFFWriteData(TIFF* tif, TIFFDirEntry* dir, char* cp)
{
	tsize_t cc;

	if (tif->tif_flags & TIFF_SWAB) {
		switch (dir->tdir_type) {
		case TIFF_SHORT:
		case TIFF_SSHORT:
			TIFFSwabArrayOfShort((uint16*) cp, dir->tdir_count);
			break;
		case TIFF_LONG:
		case TIFF_SLONG:
		case TIFF_FLOAT:
			TIFFSwabArrayOfLong((uint32*) cp, dir->tdir_count);
			break;
		case TIFF_RATIONAL:
		case TIFF_SRATIONAL:
			TIFFSwabArrayOfLong((uint32*) cp, 2*dir->tdir_count);
			break;
		case TIFF_DOUBLE:
			TIFFSwabArrayOfDouble((double*) cp, dir->tdir_count);
			break;
		}
	}
	dir->tdir_offset = tif->tif_dataoff;
	cc = dir->tdir_count * tiffDataWidth[dir->tdir_type];
	if (SeekOK(tif, dir->tdir_offset) &&
	    WriteOK(tif, cp, cc)) {
		tif->tif_dataoff += (cc + 1) & ~1;
		return (1);
	}
	TIFFError(tif->tif_name, "Error writing data for field \"%s\"",
	    _TIFFFieldWithTag(tif, dir->tdir_tag)->field_name);
	return (0);
}

/*
 * Similar to TIFFWriteDirectory(), but if the directory has already
 * been written once, it is relocated to the end of the file, in case it
 * has changed in size.  Note that this will result in the loss of the 
 * previously used directory space. 
 */ 

int 
TIFFRewriteDirectory( TIFF *tif )
{
    static const char module[] = "TIFFRewriteDirectory";

    /* We don't need to do anything special if it hasn't been written. */
    if( tif->tif_diroff == 0 )
        return TIFFWriteDirectory( tif );

    /*
    ** Find and zero the pointer to this directory, so that TIFFLinkDirectory
    ** will cause it to be added after this directories current pre-link.
    */
    
    /* Is it the first directory in the file? */
    if (tif->tif_header.tiff_diroff == tif->tif_diroff) 
    {
        tif->tif_header.tiff_diroff = 0;
        tif->tif_diroff = 0;

#define	HDROFF(f)	((toff_t) &(((TIFFHeader*) 0)->f))
        TIFFSeekFile(tif, HDROFF(tiff_diroff), SEEK_SET);
        if (!WriteOK(tif, &(tif->tif_header.tiff_diroff), 
                     sizeof (tif->tif_diroff))) 
        {
            TIFFError(tif->tif_name, "Error updating TIFF header");
            return (0);
        }
    }
    else
    {
        toff_t  nextdir, off;

	nextdir = tif->tif_header.tiff_diroff;
	do {
		uint16 dircount;

		if (!SeekOK(tif, nextdir) ||
		    !ReadOK(tif, &dircount, sizeof (dircount))) {
			TIFFError(module, "Error fetching directory count");
			return (0);
		}
		if (tif->tif_flags & TIFF_SWAB)
			TIFFSwabShort(&dircount);
		(void) TIFFSeekFile(tif,
		    dircount * sizeof (TIFFDirEntry), SEEK_CUR);
		if (!ReadOK(tif, &nextdir, sizeof (nextdir))) {
			TIFFError(module, "Error fetching directory link");
			return (0);
		}
		if (tif->tif_flags & TIFF_SWAB)
			TIFFSwabLong(&nextdir);
	} while (nextdir != tif->tif_diroff && nextdir != 0);
        off = TIFFSeekFile(tif, 0, SEEK_CUR); /* get current offset */
        (void) TIFFSeekFile(tif, off - (toff_t)sizeof(nextdir), SEEK_SET);
        tif->tif_diroff = 0;
	if (!WriteOK(tif, &(tif->tif_diroff), sizeof (nextdir))) {
		TIFFError(module, "Error writing directory link");
		return (0);
	}
    }

    /*
    ** Now use TIFFWriteDirectory() normally.
    */

    return TIFFWriteDirectory( tif );
}


/*
 * Link the current directory into the
 * directory chain for the file.
 */
static int
TIFFLinkDirectory(TIFF* tif)
{
	static const char module[] = "TIFFLinkDirectory";
	toff_t nextdir;
	toff_t diroff, off;

	tif->tif_diroff = (TIFFSeekFile(tif, (toff_t) 0, SEEK_END)+1) &~ 1;
	diroff = tif->tif_diroff;
	if (tif->tif_flags & TIFF_SWAB)
		TIFFSwabLong(&diroff);
#if SUBIFD_SUPPORT
	if (tif->tif_flags & TIFF_INSUBIFD) {
		(void) TIFFSeekFile(tif, tif->tif_subifdoff, SEEK_SET);
		if (!WriteOK(tif, &diroff, sizeof (diroff))) {
			TIFFError(module,
			    "%s: Error writing SubIFD directory link",
			    tif->tif_name);
			return (0);
		}
		/*
		 * Advance to the next SubIFD or, if this is
		 * the last one configured, revert back to the
		 * normal directory linkage.
		 */
		if (--tif->tif_nsubifd)
			tif->tif_subifdoff += sizeof (diroff);
		else
			tif->tif_flags &= ~TIFF_INSUBIFD;
		return (1);
	}
#endif
	if (tif->tif_header.tiff_diroff == 0) {
		/*
		 * First directory, overwrite offset in header.
		 */
		tif->tif_header.tiff_diroff = tif->tif_diroff;
#define	HDROFF(f)	((toff_t) &(((TIFFHeader*) 0)->f))
		(void) TIFFSeekFile(tif, HDROFF(tiff_diroff), SEEK_SET);
		if (!WriteOK(tif, &diroff, sizeof (diroff))) {
			TIFFError(tif->tif_name, "Error writing TIFF header");
			return (0);
		}
		return (1);
	}
	/*
	 * Not the first directory, search to the last and append.
	 */
	nextdir = tif->tif_header.tiff_diroff;
	do {
		uint16 dircount;

		if (!SeekOK(tif, nextdir) ||
		    !ReadOK(tif, &dircount, sizeof (dircount))) {
			TIFFError(module, "Error fetching directory count");
			return (0);
		}
		if (tif->tif_flags & TIFF_SWAB)
			TIFFSwabShort(&dircount);
		(void) TIFFSeekFile(tif,
		    dircount * sizeof (TIFFDirEntry), SEEK_CUR);
		if (!ReadOK(tif, &nextdir, sizeof (nextdir))) {
			TIFFError(module, "Error fetching directory link");
			return (0);
		}
		if (tif->tif_flags & TIFF_SWAB)
			TIFFSwabLong(&nextdir);
	} while (nextdir != 0);
        off = TIFFSeekFile(tif, 0, SEEK_CUR); /* get current offset */
        (void) TIFFSeekFile(tif, off - (toff_t)sizeof(nextdir), SEEK_SET);
	if (!WriteOK(tif, &diroff, sizeof (diroff))) {
		TIFFError(module, "Error writing directory link");
		return (0);
	}
	return (1);
}
