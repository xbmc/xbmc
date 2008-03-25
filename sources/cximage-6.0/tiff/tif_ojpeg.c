#include "tiffiop.h"
#ifdef OJPEG_SUPPORT

/* JPEG Compression support, as per the original TIFF 6.0 specification.

   WARNING: KLUDGE ALERT!  The type of JPEG encapsulation defined by the TIFF
                           Version 6.0 specification is now totally obsolete and
   deprecated for new applications and images.  This file is an unsupported hack
   that was created solely in order to read (but NOT write!) a few old,
   unconverted images still present on some users' computer systems.  The code
   isn't pretty or robust, and it won't read every "old format" JPEG-in-TIFF
   file (see Samuel Leffler's draft "TIFF Technical Note No. 2" for a long and
   incomplete list of known problems), but it seems to work well enough in the
   few cases of practical interest to the author; so, "caveat emptor"!  This
   file should NEVER be enhanced to write new images using anything other than
   the latest approved JPEG-in-TIFF encapsulation method, implemented by the
   "tif_jpeg.c" file elsewhere in this library.

   This file interfaces with Release 6B of the JPEG Library written by theu
   Independent JPEG Group, which you can find on the Internet at:
   ftp.uu.net:/graphics/jpeg/.

   Contributed by Scott Marovich <marovich@hpl.hp.com> with considerable help
   from Charles Auer <Bumble731@msn.com> to unravel the mysteries of image files
   created by Microsoft's Wang Imaging application.
*/
#include <setjmp.h>
#include <stdio.h>
#ifdef FAR
#undef FAR /* Undefine FAR to avoid conflict with JPEG definition */
#endif
#define JPEG_INTERNALS /* Include "jpegint.h" for "DSTATE_*" symbols */
#undef INLINE
#include "../jpeg/jpeglib.h"
#undef JPEG_INTERNALS

/* Hack for Microsoft's Wang Imaging for Windows output files */
extern void jpeg_reset_huff_decode(j_decompress_ptr,float *);

/* On some machines, it may be worthwhile to use "_setjmp()" or "sigsetjmp()"
   instead of "setjmp()".  These macros make it easier:
*/
#define SETJMP(jbuf)setjmp(jbuf)
#define LONGJMP(jbuf,code)longjmp(jbuf,code)
#define JMP_BUF jmp_buf

#define TIFFTAG_WANG_PAGECONTROL 32934

/* Bit-vector offsets for keeping track of TIFF records that we've parsed. */

#define FIELD_JPEGPROC FIELD_CODEC
#define FIELD_JPEGIFOFFSET (FIELD_CODEC+1)
#define FIELD_JPEGIFBYTECOUNT (FIELD_CODEC+2)
#define FIELD_JPEGRESTARTINTERVAL (FIELD_CODEC+3)
#define FIELD_JPEGTABLES (FIELD_CODEC+4) /* New, post-6.0 JPEG-in-TIFF tag! */
#define FIELD_JPEGLOSSLESSPREDICTORS (FIELD_CODEC+5)
#define FIELD_JPEGPOINTTRANSFORM (FIELD_CODEC+6)
#define FIELD_JPEGQTABLES (FIELD_CODEC+7)
#define FIELD_JPEGDCTABLES (FIELD_CODEC+8)
#define FIELD_JPEGACTABLES (FIELD_CODEC+9)
#define FIELD_WANG_PAGECONTROL (FIELD_CODEC+10)
#define FIELD_JPEGCOLORMODE (FIELD_CODEC+11)

typedef struct jpeg_destination_mgr jpeg_destination_mgr;
typedef struct jpeg_source_mgr jpeg_source_mgr;
typedef struct jpeg_error_mgr jpeg_error_mgr;

/* State variable for each open TIFF file that uses "libjpeg" for JPEG
   decompression.  (Note:  This file should NEVER perform JPEG compression
   except in the manner implemented by the "tif_jpeg.c" file, elsewhere in this
   library; see comments above.)  JPEG Library internal state is recorded in a
   "jpeg_{de}compress_struct", while a "jpeg_common_struct" records a few items
   common to both compression and expansion.  The "cinfo" field containing JPEG
   Library state MUST be the 1st member of our own state variable, so that we
   can safely "cast" pointers back and forth.
*/
typedef struct             /* This module's private, per-image state variable */
  {
    union         /* JPEG Library state variable; this MUST be our 1st field! */
      {
     /* struct jpeg_compress_struct c; */
        struct jpeg_decompress_struct d;
        struct jpeg_common_struct comm;
      } cinfo;
    jpeg_error_mgr err;                         /* JPEG Library error manager */
    JMP_BUF exit_jmpbuf;             /* ...for catching JPEG Library failures */
#   ifdef never

 /* (The following two fields could be a "union", but they're small enough that
    it's not worth the effort.)
 */
    jpeg_destination_mgr dest;             /* Destination for compressed data */
#   endif
    jpeg_source_mgr src;                           /* Source of expanded data */
    JSAMPARRAY ds_buffer[MAX_COMPONENTS]; /* ->Temporary downsampling buffers */
    TIFF *tif;                        /* Reverse pointer, needed by some code */
    TIFFVGetMethod vgetparent;                    /* "Super class" methods... */
    TIFFVSetMethod vsetparent;
    TIFFStripMethod defsparent;
    TIFFTileMethod deftparent;
    void *jpegtables;           /* ->"New" JPEG tables, if we synthesized any */
    uint32 is_WANG,    /* <=> Microsoft Wang Imaging for Windows output file? */
           jpegtables_length;   /* Length of "new" JPEG tables, if they exist */
    tsize_t bytesperline;          /* No. of decompressed Bytes per scan line */
    int jpegquality,                             /* Compression quality level */
        jpegtablesmode,                          /* What to put in JPEGTables */
        samplesperclump,
        scancount;                           /* No. of scan lines accumulated */
    uint16 h_sampling,                          /* Luminance sampling factors */
           v_sampling,
           photometric;      /* Copy of "PhotometricInterpretation" tag value */
    u_char jpegcolormode;           /* Who performs RGB <-> YCbCr conversion? */
        /* JPEGCOLORMODE_RAW <=> TIFF Library does conversion */
        /* JPEGCOLORMODE_RGB <=> JPEG Library does conversion */
  } OJPEGState;
#define OJState(tif)((OJPEGState*)(tif)->tif_data)

static const TIFFFieldInfo ojpegFieldInfo[]=/* JPEG-specific TIFF-record tags */
  {

 /* This is the current JPEG-in-TIFF metadata-encapsulation tag, and its
    treatment in this file is idiosyncratic.  It should never appear in a
    "source" image conforming to the TIFF Version 6.0 specification, so we
    arrange to report an error if it appears.  But in order to support possible
    future conversion of "old" JPEG-in-TIFF encapsulations to "new" ones, we
    might wish to synthesize an equivalent value to be returned by the TIFF
    Library's "getfield" method.  So, this table tells the TIFF Library to pass
    these records to us in order to filter them below.
 */
    {
      TIFFTAG_JPEGTABLES            ,TIFF_VARIABLE,TIFF_VARIABLE,
      TIFF_UNDEFINED,FIELD_JPEGTABLES            ,FALSE,TRUE ,"JPEGTables"
    },

 /* These tags are defined by the TIFF Version 6.0 specification and are now
    obsolete.  This module reads them from an old "source" image, but it never
    writes them to a new "destination" image.
 */
    {
      TIFFTAG_JPEGPROC              ,1            ,1            ,
      TIFF_SHORT    ,FIELD_JPEGPROC              ,FALSE,FALSE,"JPEGProc"
    },
    {
      TIFFTAG_JPEGIFOFFSET          ,1            ,1            ,
      TIFF_LONG     ,FIELD_JPEGIFOFFSET          ,FALSE,FALSE,"JPEGInterchangeFormat"
    },
    {
      TIFFTAG_JPEGIFBYTECOUNT       ,1            ,1            ,
      TIFF_LONG     ,FIELD_JPEGIFBYTECOUNT       ,FALSE,FALSE,"JPEGInterchangeFormatLength"
    },
    {
      TIFFTAG_JPEGRESTARTINTERVAL   ,1            ,1            ,
      TIFF_SHORT    ,FIELD_JPEGRESTARTINTERVAL   ,FALSE,FALSE,"JPEGRestartInterval"
    },
    {
      TIFFTAG_JPEGLOSSLESSPREDICTORS,TIFF_VARIABLE,TIFF_VARIABLE,
      TIFF_SHORT    ,FIELD_JPEGLOSSLESSPREDICTORS,FALSE,TRUE ,"JPEGLosslessPredictors"
    },
    {
      TIFFTAG_JPEGPOINTTRANSFORM    ,TIFF_VARIABLE,TIFF_VARIABLE,
      TIFF_SHORT    ,FIELD_JPEGPOINTTRANSFORM    ,FALSE,TRUE ,"JPEGPointTransforms"
    },
    {
      TIFFTAG_JPEGQTABLES           ,TIFF_VARIABLE,TIFF_VARIABLE,
      TIFF_LONG     ,FIELD_JPEGQTABLES           ,FALSE,TRUE ,"JPEGQTables"
    },
    {
      TIFFTAG_JPEGDCTABLES          ,TIFF_VARIABLE,TIFF_VARIABLE,
      TIFF_LONG     ,FIELD_JPEGDCTABLES          ,FALSE,TRUE ,"JPEGDCTables"
    },
    {
      TIFFTAG_JPEGACTABLES          ,TIFF_VARIABLE,TIFF_VARIABLE,
      TIFF_LONG     ,FIELD_JPEGACTABLES          ,FALSE,TRUE ,"JPEGACTables"
    },
    {
      TIFFTAG_WANG_PAGECONTROL      ,TIFF_VARIABLE,1            ,
      TIFF_LONG     ,FIELD_WANG_PAGECONTROL      ,FALSE,FALSE,"WANG PageControl"
    },

 /* This is a pseudo tag intended for internal use only by the TIFF Library and
    its clients, which should never appear in an input/output image file.  It
    specifies whether the TIFF Library will perform YCbCr<->RGB color-space
    conversion (JPEGCOLORMODE_RAW <=> 0) or ask the JPEG Library to do it
    (JPEGCOLORMODE_RGB <=> 1).
 */
    {
      TIFFTAG_JPEGCOLORMODE         ,0            ,0            ,
      TIFF_ANY      ,FIELD_PSEUDO                ,FALSE,FALSE,"JPEGColorMode"
    }
  };
static const char JPEGLib_name[]={"JPEG Library"},
                  bad_bps[]={"%u BitsPerSample not allowed for JPEG"},
#                 ifdef never
                  no_write_frac[]={"fractional scan line discarded"},
#                 endif
                  no_read_frac[]={"fractional scan line not read"},
                  no_jtable_space[]={"No space for JPEGTables"};

/* The following diagnostic subroutines interface with and replace default
   subroutines in the JPEG Library.  Our basic strategy is to use "setjmp()"/
   "longjmp()" in order to return control to the TIFF Library when the JPEG
   library detects an error, and to use TIFF Library subroutines for displaying
   diagnostic messages to a client application.
*/
static void
TIFFojpeg_error_exit(register j_common_ptr cinfo)
  { char buffer[JMSG_LENGTH_MAX];

    (*cinfo->err->format_message)(cinfo,buffer);
    TIFFError(JPEGLib_name,buffer); /* Display error message */
    jpeg_abort(cinfo); /* Clean up JPEG Library state */
    LONGJMP(((OJPEGState *)cinfo)->exit_jmpbuf,1); /* Return to TIFF client */
  }

static void
TIFFojpeg_output_message(register j_common_ptr cinfo)
  { char buffer[JMSG_LENGTH_MAX];

 /* This subroutine is invoked only for warning messages, since the JPEG
    Library's "error_exit" method does its own thing and "trace_level" is never
    set > 0.
 */
    (*cinfo->err->format_message)(cinfo,buffer);
    TIFFWarning(JPEGLib_name,buffer);
  }

/* The following subroutines, which also interface with the JPEG Library, exist
   mainly in limit the side effects of "setjmp()" and convert JPEG normal/error
   conditions into TIFF Library return codes.
*/
#define CALLJPEG(sp,fail,op)(SETJMP((sp)->exit_jmpbuf)?(fail):(op))
#define CALLVJPEG(sp,op)CALLJPEG(sp,0,((op),1))
#ifdef never

static int
TIFFojpeg_create_compress(register OJPEGState *sp)
  {
    sp->cinfo.c.err = jpeg_std_error(&sp->err); /* Initialize error handling */
    sp->err.error_exit = TIFFojpeg_error_exit;
    sp->err.output_message = TIFFojpeg_output_message;
    return CALLVJPEG(sp,jpeg_create_compress(&sp->cinfo.c));
  }

static int
TIFFojpeg_finish_compress(register OJPEGState *sp)
  {return CALLVJPEG(sp,jpeg_finish_compress(&sp->cinfo.c));}

static int
TIFFojpeg_set_colorspace(register OJPEGState *sp,J_COLOR_SPACE colorspace)
  {return CALLVJPEG(sp,jpeg_set_colorspace(&sp->cinfo.c,colorspace));}

static int
TIFFojpeg_set_defaults(register OJPEGState *sp)
  {return CALLVJPEG(sp,jpeg_set_defaults(&sp->cinfo.c));}

static int
TIFFojpeg_set_quality(register OJPEGState *sp,int quality,boolean force_baseline)
  {return CALLVJPEG(sp,jpeg_set_quality(&sp->cinfo.c,quality,force_baseline));}

static int
TIFFojpeg_start_compress(register OJPEGState *sp,boolean write_all_tables)
  {return CALLVJPEG(sp,jpeg_start_compress(&sp->cinfo.c,write_all_tables));}

static int
TIFFojpeg_suppress_tables(register OJPEGState *sp,boolean suppress)
  {return CALLVJPEG(sp,jpeg_suppress_tables(&sp->cinfo.c,suppress));}

static int
TIFFojpeg_write_raw_data(register OJPEGState *sp,JSAMPIMAGE data,int num_lines)
  { return
      CALLJPEG(sp,-1,(int)jpeg_write_raw_data(&sp->cinfo.c,data,(JDIMENSION)num_lines));
  }

static int
TIFFojpeg_write_scanlines(register OJPEGState *sp,JSAMPARRAY scanlines,
                         int num_lines)
  { return
      CALLJPEG(sp,-1,(int)jpeg_write_scanlines(&sp->cinfo.c,scanlines,(JDIMENSION)num_lines));
  }

static int
TIFFojpeg_write_tables(register OJPEGState *sp)
  {return CALLVJPEG(sp,jpeg_write_tables(&sp->cinfo.c));}
#else /* well, hardly ever */

static int
_notSupported(register TIFF *tif)
  { const TIFFCodec *c = TIFFFindCODEC(tif->tif_dir.td_compression);

    TIFFError(tif->tif_name,"%s compression is not supported",c->name);
    return 0;
  }
#endif /* never */

static int
TIFFojpeg_abort(register OJPEGState *sp)
  {return CALLVJPEG(sp,jpeg_abort(&sp->cinfo.comm));}

static JSAMPARRAY
TIFFojpeg_alloc_sarray(register OJPEGState *sp,int pool_id,
                      JDIMENSION samplesperrow,JDIMENSION numrows)
  { return
      CALLJPEG(sp,0,(*sp->cinfo.comm.mem->alloc_sarray)(&sp->cinfo.comm,pool_id,samplesperrow, numrows));
  }

static int
TIFFojpeg_create_decompress(register OJPEGState *sp)
  {
    sp->cinfo.d.err = jpeg_std_error(&sp->err); /* Initialize error handling */
    sp->err.error_exit = TIFFojpeg_error_exit;
    sp->err.output_message = TIFFojpeg_output_message;
    return CALLVJPEG(sp,jpeg_create_decompress(&sp->cinfo.d));
  }

static int
TIFFojpeg_destroy(register OJPEGState *sp)
  {return CALLVJPEG(sp,jpeg_destroy(&sp->cinfo.comm));}

static int
TIFFojpeg_finish_decompress(register OJPEGState *sp)
  {return CALLJPEG(sp,-1,(int)jpeg_finish_decompress(&sp->cinfo.d));}

static int
TIFFojpeg_read_header(register OJPEGState *sp,boolean require_image)
  {return CALLJPEG(sp,-1,jpeg_read_header(&sp->cinfo.d,require_image));}

static int
TIFFojpeg_read_raw_data(register OJPEGState *sp,JSAMPIMAGE data,int max_lines)
  {
    return
      CALLJPEG(sp,-1,(int)jpeg_read_raw_data(&sp->cinfo.d,data,(JDIMENSION)max_lines));
  }

static int
TIFFojpeg_read_scanlines(register OJPEGState *sp,JSAMPARRAY scanlines,
                        int max_lines)
  { return
      CALLJPEG(sp,-1,(int)jpeg_read_scanlines(&sp->cinfo.d,scanlines,(JDIMENSION)max_lines));
  }

static int
TIFFojpeg_start_decompress(register OJPEGState *sp)
  {return CALLVJPEG(sp,jpeg_start_decompress(&sp->cinfo.d));}
#ifdef never

/* The following subroutines comprise a JPEG Library "destination" data manager
   by directing compressed data from the JPEG Library to a TIFF Library output
   buffer.
*/
static void
std_init_destination(register j_compress_ptr cinfo){} /* "Dummy" stub */

static boolean
std_empty_output_buffer(register j_compress_ptr cinfo)
  {
#   define sp ((OJPEGState *)cinfo)
    register TIFF *tif = sp->tif;

    tif->tif_rawcc = tif->tif_rawdatasize; /* Entire buffer has been filled */
    TIFFFlushData1(tif);
    sp->dest.next_output_byte = (JOCTET *)tif->tif_rawdata;
    sp->dest.free_in_buffer = (size_t)tif->tif_rawdatasize;
    return TRUE;
#   undef sp
  }

static void
std_term_destination(register j_compress_ptr cinfo)
  {
#   define sp ((OJPEGState *)cinfo)
    register TIFF *tif = sp->tif;

 /* NB: The TIFF Library does the final buffer flush. */
    tif->tif_rawcp = (tidata_t)sp->dest.next_output_byte;
    tif->tif_rawcc = tif->tif_rawdatasize - (tsize_t)sp->dest.free_in_buffer;
#   undef sp
  }

/*ARGSUSED*/ static void
TIFFojpeg_data_dest(register OJPEGState *sp,TIFF *tif)
  {
    sp->cinfo.c.dest = &sp->dest;
    sp->dest.init_destination = std_init_destination;
    sp->dest.empty_output_buffer = std_empty_output_buffer;
    sp->dest.term_destination = std_term_destination;
  }


/* Alternate destination manager to output JPEGTables field: */

static void
tables_init_destination(register j_compress_ptr cinfo)
  {
#   define sp ((OJPEGState *)cinfo)
 /* The "jpegtables_length" field is the allocated buffer size while building */
    sp->dest.next_output_byte = (JOCTET *)sp->jpegtables;
    sp->dest.free_in_buffer = (size_t)sp->jpegtables_length;
#   undef sp
  }

static boolean
tables_empty_output_buffer(register j_compress_ptr cinfo)
  { void *newbuf;
#   define sp ((OJPEGState *)cinfo)

 /* The entire buffer has been filled, so enlarge it by 1000 bytes. */
    if (!( newbuf = _TIFFrealloc( (tdata_t)sp->jpegtables
                                , (tsize_t)(sp->jpegtables_length + 1000)
                                )
         )
       ) ERREXIT1(cinfo,JERR_OUT_OF_MEMORY,100);
    sp->dest.next_output_byte = (JOCTET *)newbuf + sp->jpegtables_length;
    sp->dest.free_in_buffer = (size_t)1000;
    sp->jpegtables = newbuf;
    sp->jpegtables_length += 1000;
    return TRUE;
#   undef sp
  }

static void
tables_term_destination(register j_compress_ptr cinfo)
  {
#   define sp ((OJPEGState *)cinfo)
 /* Set tables length to no. of Bytes actually emitted. */
    sp->jpegtables_length -= sp->dest.free_in_buffer;
#   undef sp
  }

/*ARGSUSED*/ static int
TIFFojpeg_tables_dest(register OJPEGState *sp, TIFF *tif)
  {

 /* Allocate a working buffer for building tables.  The initial size is 1000
    Bytes, which is usually adequate.
 */
    if (sp->jpegtables) _TIFFfree(sp->jpegtables);
    if (!(sp->jpegtables = (void*)
                           _TIFFmalloc((tsize_t)(sp->jpegtables_length = 1000))
         )
       )
      {
        sp->jpegtables_length = 0;
        TIFFError("TIFFojpeg_tables_dest",no_jtable_space);
        return 0;
      };
    sp->cinfo.c.dest = &sp->dest;
    sp->dest.init_destination = tables_init_destination;
    sp->dest.empty_output_buffer = tables_empty_output_buffer;
    sp->dest.term_destination = tables_term_destination;
    return 1;
  }
#endif /* never */

/* The following subroutines comprise a JPEG Library "source" data manager by
   by directing compressed data to the JPEG Library from a TIFF Library input
   buffer.
*/
static void
std_init_source(register j_decompress_ptr cinfo)
  {
#   define sp ((OJPEGState *)cinfo)
    register TIFF *tif = sp->tif;

    if (sp->src.bytes_in_buffer == 0)
      {
        sp->src.next_input_byte = (const JOCTET *)tif->tif_rawdata;
        sp->src.bytes_in_buffer = (size_t)tif->tif_rawcc;
      };
#   undef sp
  }

static boolean
std_fill_input_buffer(register j_decompress_ptr cinfo)
  { static const JOCTET dummy_EOI[2]={0xFF,JPEG_EOI};
#   define sp ((OJPEGState *)cinfo)

 /* Control should never get here, since an entire strip/tile is read into
    memory before the decompressor is called; thus, data should have been
    supplied by the "init_source" method.  ...But, sometimes things fail.
 */
    WARNMS(cinfo,JWRN_JPEG_EOF);
    sp->src.next_input_byte = dummy_EOI; /* Insert a fake EOI marker */
    sp->src.bytes_in_buffer = sizeof dummy_EOI;
    return TRUE;
#   undef sp
  }

static void
std_skip_input_data(register j_decompress_ptr cinfo,long num_bytes)
  {
#   define sp ((OJPEGState *)cinfo)

    if (num_bytes > 0)
      if (num_bytes > (long)sp->src.bytes_in_buffer) /* oops: buffer overrun */
        (void)std_fill_input_buffer(cinfo);
      else
        {
          sp->src.next_input_byte += (size_t)num_bytes;
          sp->src.bytes_in_buffer -= (size_t)num_bytes;
        }
#   undef sp
  }

/*ARGSUSED*/ static void
std_term_source(register j_decompress_ptr cinfo){} /* "Dummy" stub */

/* Allocate temporary I/O buffers for downsampled data, using values computed in
   "jpeg_start_{de}compress()".  We use the JPEG Library's allocator so that
   buffers will be released automatically when done with a strip/tile.  This is
   also a handy place to compute samplesperclump, bytesperline, etc.
*/
static int
alloc_downsampled_buffers(TIFF *tif,jpeg_component_info *comp_info,
                          int num_components)
  { register OJPEGState *sp = OJState(tif);

    sp->samplesperclump = 0;
    if (num_components > 0)
      { int ci = 0;
        register jpeg_component_info *compptr = comp_info;

        do
          { JSAMPARRAY buf;

            sp->samplesperclump +=
              compptr->h_samp_factor * compptr->v_samp_factor;
            if (!(buf = TIFFojpeg_alloc_sarray( sp
                                              , JPOOL_IMAGE
                                              , compptr->width_in_blocks*DCTSIZE
                                              , compptr->v_samp_factor  *DCTSIZE
                                              )
                 )
               ) return 0;
            sp->ds_buffer[ci] = buf;
          }
        while (++compptr,++ci < num_components);
      };
    return 1;
  }
#ifdef never

/* JPEG Encoding begins here. */

static void
unsuppress_quant_table(register OJPEGState *sp,int tblno)
  { register JQUANT_TBL *qtbl;

    if (qtbl = sp->cinfo.c.quant_tbl_ptrs[tblno]) qtbl->sent_table = FALSE;
  }

static void
unsuppress_huff_table(register OJPEGState *sp,register int tblno)
  { register JHUFF_TBL *htbl;

    if (   (htbl = sp->cinfo.c.dc_huff_tbl_ptrs[tblno])
        || (htbl = sp->cinfo.c.ac_huff_tbl_ptrs[tblno])
       ) htbl->sent_table = FALSE;
  }

static int
prepare_JPEGTables(register TIFF *tif)
  { register OJPEGState *sp = OJState(tif);

 /* Initialize quantization tables for the current quality setting, and mark for
    output only the tables that we want.  Note that chrominance tables are
    currently used only with YCbCr.
 */
    if (   !TIFFojpeg_set_quality(sp,sp->jpegquality,FALSE);
        || !TIFFojpeg_suppress_tables(sp,TRUE)
       ) return 0;
    if (sp->jpegtablesmode & JPEGTABLESMODE_QUANT)
      {
        unsuppress_quant_table(sp,0);
        if (sp->photometric == PHOTOMETRIC_YCBCR) unsuppress_quant_table(sp,1);
      }
    if (sp->jpegtablesmode & JPEGTABLESMODE_HUFF)
      {
        unsuppress_huff_table(sp,0);
        if (sp->photometric == PHOTOMETRIC_YCBCR) unsuppress_huff_table(sp,1);
      };
    return TIFFojpeg_tables_dest(sp,tif) && TIFFojpeg_write_tables(sp);
  }

static int
OJPEGSetupEncode(register TIFF *tif)
  { static const char module[]={"OJPEGSetupEncode"};
    register OJPEGState *sp = OJState(tif);
#   define td (&tif->tif_dir)

 /* Verify miscellaneous parameters.  This will need work if the TIFF Library
    ever supports different depths for different components, or if the JPEG
    Library ever supports run-time depth selection.  Neither seems imminent.
 */
    if (td->td_bitspersample != BITS_IN_JSAMPLE)
      {
        TIFFError(module,bad_bps,td->td_bitspersample);
        return 0;
      };

 /* Initialize all JPEG parameters to default values.  Note that the JPEG
    Library's "jpeg_set_defaults()" method needs legal values for the
    "in_color_space" and "input_components" fields.
 */
    sp->cinfo.c.in_color_space = JCS_UNKNOWN;
    sp->cinfo.c.input_components = 1;
    if (!TIFFojpeg_set_defaults(sp)) return 0;
    switch (sp->photometric = td->td_photometric) /* set per-file parameters */
      {
        case PHOTOMETRIC_YCBCR:
          sp->h_sampling = td->td_ycbcrsubsampling[0];
          sp->v_sampling = td->td_ycbcrsubsampling[1];
#         ifdef COLORIMETRY_SUPPORT

       /* A ReferenceBlackWhite field MUST be present, since the default value
          is inapproriate for YCbCr.  Fill in the proper value if application
          didn't set it.
       */
          if (!TIFFFieldSet(tif,FIELD_REFBLACKWHITE))
            { float refbw[6];
              long top = 1L << td->td_bitspersample;

              refbw[0] = 0;
              refbw[1] = (float)(top-1L);
              refbw[2] = (float)(top>>1);
              refbw[3] = refbw[1];
              refbw[4] = refbw[2];
              refbw[5] = refbw[1];
              TIFFSetField(tif,TIFFTAG_REFERENCEBLACKWHITE,refbw);
            };
#         endif /* COLORIMETRY_SUPPORT */
          break;
        case PHOTOMETRIC_PALETTE: /* disallowed by Tech Note */
        case PHOTOMETRIC_MASK:
          TIFFError(module,"PhotometricInterpretation %d not allowed for JPEG",
            (int)sp->photometric);
          return 0;

     /* TIFF 6.0 forbids subsampling of all other color spaces */

        default: sp->h_sampling = sp->v_sampling = 1;
      };
    sp->cinfo.c.data_precision = td->td_bitspersample;
    if (isTiled(tif))
      {
        if (td->td_tilelength % (sp->v_sampling*DCTSIZE))
          {
            TIFFError(module,"JPEG tile height must be multiple of %d",
              sp->v_sampling*DCTSIZE);
            return 0;
          };
        if (td->td_tilewidth % (sp->h_sampling*DCTSIZE))
          {
            TIFFError(module,"JPEG tile width must be multiple of %d",
              sp->h_sampling*DCTSIZE);
            return 0;
          }
      }
    else
      if (   td->td_rowsperstrip < td->td_imagelength
          && (td->td_rowsperstrip % (sp->v_sampling*DCTSIZE))
         )
        {
          TIFFError(module,"RowsPerStrip must be multiple of %d for JPEG",
            sp->v_sampling*DCTSIZE);
          return 0;
        };
    if (sp->jpegtablesmode & (JPEGTABLESMODE_QUANT|JPEGTABLESMODE_HUFF))
      { /* create a JPEGTables field */

        if (!prepare_JPEGTables(tif)) return 0;

     /* Mark the field "present".  We can't use "TIFFSetField()" because
        "BEENWRITING" is already set!
     */
        TIFFSetFieldBit(tif,FIELD_JPEGTABLES);
        tif->tif_flags |= TIFF_DIRTYDIRECT;
      }
    else
   /* We do not support application-supplied JPEG tables, so mark the field
      "not present".
   */
      TIFFClrFieldBit(tif,FIELD_JPEGTABLES);
    TIFFojpeg_data_dest(sp,tif); /* send JPEG output to TIFF Library's buffer */
    return 1;
#   undef td
  }

/*ARGSUSED*/ static int
OJPEGEncode(register TIFF *tif,tidata_t buf,tsize_t cc,tsample_t s)
  { register OJPEGState *sp = OJState(tif);

 /* Encode a chunk of pixels, where returned data is NOT down-sampled (the
    standard case).  The data is expected to be written in scan-line multiples.
 */
    if (cc % sp->bytesperline) TIFFWarning(tif->tif_name,no_write_frac);
    cc /= sp->bytesperline;
    while (--cc >= 0)
      { JSAMPROW bufptr = (JSAMPROW)buf;

        if (TIFFojpeg_write_scanlines(sp,&bufptr,1) != 1) return 0;
        ++tif->tif_row;
        buf += sp->bytesperline;
      };
    return 1;
  }

/*ARGSUSED*/ static int
OJPEGEncodeRaw(register TIFF *tif,tidata_t buf,tsize_t cc,tsample_t s)
  { register OJPEGState *sp = OJState(tif);

 /* Encode a chunk of pixels, where returned data is down-sampled as per the
    sampling factors.  The data is expected to be written in scan-line
    multiples.
 */
    if (cc % sp->bytesperline) TIFFWarning(tif->tif_name,no_write_frac);
    cc /= sp->bytesperline;
    while (--cc >= 0)
      {
        if (sp->cinfo.c.num_components > 0)
          { int ci = 0, clumpoffset = 0;
            register jpeg_component_info *compptr = sp->cinfo.c.comp_info;

         /* The fastest way to separate the data is to make 1 pass over the scan
            line for each row of each component.
         */
            do
              { int ypos = 0;

                do
                  { int padding;
                    register JSAMPLE *inptr = (JSAMPLE*)buf + clumpoffset,
                                     *outptr =
                      sp->ds_buffer[ci][sp->scancount*compptr->v_samp_factor+ypos];
                 /* Cb,Cr both have sampling factors 1, so this is correct */
                    register int clumps_per_line =
                      sp->cinfo.c.comp_info[1].downsampled_width,
                                 xpos;

                    padding = (int)
                              ( compptr->width_in_blocks * DCTSIZE
                              - clumps_per_line * compptr->h_samp_factor
                              );
                    if (compptr->h_samp_factor == 1) /* Cb & Cr fast path */
                      do
                        {
                          *outptr++ = inptr[0];
                          inptr += sp->samplesperclump;
                        }
                      while (--clumps_per_line > 0);
                    else /* general case */
                      do
                        {
                          xpos = 0;
                          do *outptr++ = inptr[xpos];
                          while (++xpos < compptr->h_samp_factor);
                          inptr += sp->samplesperclump;
                        }
                      while (--clumps_per_line > 0);
                    xpos = 0; /* Pad each scan line as needed */
                    do outptr[0]=outptr[-1]; while (++outptr,++xpos < padding);
                    clumpoffset += compptr->h_samp_factor;
                  }
                while (++ypos < compptr->v_samp_factor);
              }
            while (++compptr,++ci < sp->cinfo.c.num_components);
          };
        if (++sp->scancount >= DCTSIZE)
          { int n = sp->cinfo.c.max_v_samp_factor*DCTSIZE;

            if (TIFFojpeg_write_raw_data(sp,sp->ds_buffer,n) != n) return 0;
            sp->scancount = 0;
          };
        ++tif->tif_row++
        buf += sp->bytesperline;
      };
    return 1;
  }

static int
OJPEGPreEncode(register TIFF *tif,tsample_t s)
  { static const char module[]={"OJPEGPreEncode"};
    uint32 segment_width, segment_height;
    int downsampled_input = FALSE;
    register OJPEGState *sp = OJState(tif);
#   define td (&tif->tif_dir)

 /* Set encoding state at the start of a strip or tile. */

    if (td->td_planarconfig == PLANARCONFIG_CONTIG)
      {
        sp->cinfo.c.input_components = td->td_samplesperpixel;
        if (sp->photometric == PHOTOMETRIC_YCBCR)
          {
            if (sp->jpegcolormode == JPEGCOLORMODE_RGB)
              sp->cinfo.c.in_color_space = JCS_RGB;
            else
              {
                sp->cinfo.c.in_color_space = JCS_YCbCr;
                if (sp->h_sampling != 1 || sp->v_sampling != 1)
                  downsampled_input = TRUE;
              };
            if (!TIFFojpeg_set_colorspace(sp,JCS_YCbCr)) return 0;

         /* Set Y sampling factors; we assume "jpeg_set_colorspace()" set the
            rest to 1.
         */
            sp->cinfo.c.comp_info[0].h_samp_factor = sp->h_sampling;
            sp->cinfo.c.comp_info[0].v_samp_factor = sp->v_sampling;
          }
        else
          {
            sp->cinfo.c.in_color_space = JCS_UNKNOWN;
            if (!TIFFojpeg_set_colorspace(sp,JCS_UNKNOWN)) return 0;
         /* "jpeg_set_colorspace()" set all sampling factors to 1. */
          }
      }
    else
      {
        sp->cinfo.c.input_components = 1;
        sp->cinfo.c.in_color_space = JCS_UNKNOWN;
        if (!TIFFojpeg_set_colorspace(sp,JCS_UNKNOWN)) return 0;
        sp->cinfo.c.comp_info[0].component_id = s;
     /* "jpeg_set_colorspace()" set all sampling factors to 1. */
        if (sp->photometric == PHOTOMETRIC_YCBCR && s > 0)
          sp->cinfo.c.comp_info[0].quant_tbl_no =
          sp->cinfo.c.comp_info[0].dc_tbl_no =
          sp->cinfo.c.comp_info[0].ac_tbl_no = 1;
      };
    if (isTiled(tif))
      {
        segment_width = td->td_tilewidth;
        segment_height = td->td_tilelength;
        sp->bytesperline = TIFFTileRowSize(tif);
      }
    else
      {
        segment_width = td->td_imagewidth;
        segment_height = td->td_imagelength - tif->tif_row;
        if (segment_height > td->td_rowsperstrip)
          segment_height = td->td_rowsperstrip;
        sp->bytesperline = TIFFScanlineSize(tif);
      };
    if (td->td_planarconfig == PLANARCONFIG_SEPARATE && s > 0)
      {

     /* Scale the expected strip/tile size to match a downsampled component. */

        segment_width = TIFFhowmany(segment_width,sp->h_sampling);
        segment_height = TIFFhowmany(segment_height,sp->v_sampling);
      };
    if (segment_width > 65535 || segment_height > 65535)
      {
        TIFFError(module,"Strip/tile too large for JPEG");
        return 0;
      };
    sp->cinfo.c.image_width = segment_width;
    sp->cinfo.c.image_height = segment_height;
    sp->cinfo.c.write_JFIF_header = /* Don't write extraneous markers */
    sp->cinfo.c.write_Adobe_marker = FALSE;
    if (!(sp->jpegtablesmode & JPEGTABLESMODE_QUANT)) /* setup table handling */
      {
        if (!TIFFojpeg_set_quality(sp,sp->jpegquality,FALSE)) return 0;
        unsuppress_quant_table(sp,0);
        unsuppress_quant_table(sp,1);
      };
    sp->cinfo.c.optimize_coding = !(sp->jpegtablesmode & JPEGTABLESMODE_HUFF);
    tif->tif_encoderow = tif->tif_encodestrip = tif->tif_encodetile =
      (sp->cinfo.c.raw_data_in = downsampled_input)
      ? OJPEGEncodeRaw : OJPEGEncode;
    if (   !TIFFojpeg_start_compress(sp,FALSE) /* start JPEG compressor */
        ||     downsampled_input /* allocate downsampled-data buffers */
           && !alloc_downsampled_buffers(tif,sp->cinfo.c.comp_info,
                                         sp->cinfo.c.num_components)
       ) return 0;
    sp->scancount = 0;
    return 1;
#   undef td
  }

static int
OJPEGPostEncode(register TIFF *tif)
  { register OJPEGState *sp = OJState(tif);

 /* Finish up at the end of a strip or tile. */

    if (sp->scancount > 0) /* emit partial buffer of down-sampled data */
      {
        if (sp->scancount < DCTSIZE && sp->cinfo.c.num_components > 0)
          { int ci = 0, n;                         /* Pad the data vertically */
            register jpeg_component_info *compptr = sp->cinfo.c.comp_info;

            do
               { tsize_t row_width =
                   compptr->width_in_blocks*DCTSIZE*sizeof(JSAMPLE);
                 int ypos = sp->scancount*compptr->v_samp_factor;

                 do _TIFFmemcpy( (tdata_t)sp->ds_buffer[ci][ypos]
                               , (tdata_t)sp->ds_buffer[ci][ypos-1]
                               , row_width
                               );
                 while (++ypos < compptr->v_samp_factor*DCTSIZE);
               }
            while (++compptr,++ci < sp->cinfo.c.num_components);
          };
        n = sp->cinfo.c.max_v_samp_factor*DCTSIZE;
        if (TIFFojpeg_write_raw_data(sp,sp->ds_buffer,n) != n) return 0;
      };
    return TIFFojpeg_finish_compress(sp);
  }
#endif /* never */

/* JPEG Decoding begins here. */

static int
OJPEGSetupDecode(register TIFF *tif)
  { static const char module[]={"OJPEGSetupDecode"};
    register OJPEGState *sp = OJState(tif);
#   define td (&tif->tif_dir)

 /* Verify miscellaneous parameters.  This will need work if the TIFF Library
    ever supports different depths for different components, or if the JPEG
    Library ever supports run-time depth selection.  Neither seems imminent.
 */
    if (td->td_bitspersample != BITS_IN_JSAMPLE)
      {
        TIFFError(module,bad_bps,td->td_bitspersample);
        return 0;
      };

 /* Almost all old JPEG-in-TIFF encapsulations use 8 bits per sample, but the
    following is just a "sanity check", since "OJPEGPreDecode()" actually
    depends upon this assumption in certain cases.
 */
    if (td->td_bitspersample != 8)
      {
        TIFFError(module,"Cannot decompress %u bits per sample");
        return 0;
      };

 /* Grab parameters that are same for all strips/tiles. */

    if ((sp->photometric = td->td_photometric) == PHOTOMETRIC_YCBCR)
      {
        sp->h_sampling = td->td_ycbcrsubsampling[0];
        sp->v_sampling = td->td_ycbcrsubsampling[1];
      }
    else /* TIFF 6.0 forbids subsampling of all other color spaces */
      sp->h_sampling = sp->v_sampling = 1;
    sp->cinfo.d.src = &sp->src;
    sp->src.init_source = std_init_source;
    sp->src.fill_input_buffer = std_fill_input_buffer;
    sp->src.skip_input_data = std_skip_input_data;
    sp->src.resync_to_restart = jpeg_resync_to_restart;
    sp->src.term_source = std_term_source;
    tif->tif_postdecode = _TIFFNoPostDecode; /* Override Byte-swapping */
    return 1;
#   undef td
  }

/*ARGSUSED*/ static int
OJPEGDecode(register TIFF *tif,tidata_t buf,tsize_t cc,tsample_t s)
  { static float zeroes[6];
    tsize_t nrows;
    register OJPEGState *sp = OJState(tif);

 /* BEWARE OF KLUDGE:  If our input file was produced by Microsoft's Wang
                       Imaging for Windows application, the DC coefficients of
    each JPEG image component (Y,Cb,Cr) must be reset at the beginning of each
    TIFF "strip", and any JPEG data bits remaining in the decoder's input buffer
    must be discarded, up to the next input-Byte storage boundary.  To do so, we
    create an "ad hoc" interface in the "jdhuff.c" module of IJG JPEG Library
    Version 6, and we invoke that interface here before decoding each "strip".
 */
    if (sp->is_WANG) jpeg_reset_huff_decode(&sp->cinfo.d,zeroes);

 /* Decode a chunk of pixels, where returned data is NOT down-sampled (the
    standard case).  The data is expected to be read in scan-line multiples.
 */
    if (nrows = sp->cinfo.d.image_height)
      { unsigned int bytesperline = isTiled(tif)
                                  ? TIFFTileRowSize(tif)
                                  : TIFFScanlineSize(tif);

     /* WARNING:  Unlike "OJPEGDecodeRaw()", below, the no. of Bytes in each
                  decoded row is calculated here as "bytesperline" instead of
        using "sp->bytesperline", which might be a little smaller.  This can
        occur for an old tiled image whose width isn't a multiple of 8 pixels.
        That's illegal according to the TIFF Version 6 specification, but some
        test files, like "zackthecat.tif", were built that way.  In those cases,
        we want to embed the image's true width in our caller's buffer (which is
        presumably allocated according to the expected tile width) by
        effectively "padding" it with unused Bytes at the end of each row.
     */
        do
          { JSAMPROW bufptr = (JSAMPROW)buf;

            if (TIFFojpeg_read_scanlines(sp,&bufptr,1) != 1) return 0;
            buf += bytesperline;
            ++tif->tif_row;
          }
        while ((cc -= bytesperline) > 0 && --nrows > 0);
      };
    return sp->cinfo.d.output_scanline < sp->cinfo.d.output_height
        || TIFFojpeg_finish_decompress(sp);
  }

/*ARGSUSED*/ static int
OJPEGDecodeRaw(register TIFF *tif,tidata_t buf,tsize_t cc,tsample_t s)
  { static float zeroes[6];
    tsize_t nrows;
    register OJPEGState *sp = OJState(tif);

 /* BEWARE OF KLUDGE:  If our input file was produced by Microsoft's Wang
                       Imaging for Windows application, the DC coefficients of
    each JPEG image component (Y,Cb,Cr) must be reset at the beginning of each
    TIFF "strip", and any JPEG data bits remaining in the decoder's input buffer
    must be discarded, up to the next input-Byte storage boundary.  To do so, we
    create an "ad hoc" interface in the "jdhuff.c" module of IJG JPEG Library
    Version 6, and we invoke that interface here before decoding each "strip".
 */
    if (sp->is_WANG) jpeg_reset_huff_decode(&sp->cinfo.d,zeroes);

 /* Decode a chunk of pixels, where returned data is down-sampled as per the
    sampling factors.  The data is expected to be read in scan-line multiples.
 */
    if (nrows = sp->cinfo.d.image_height)
      do
        {
          if (sp->scancount >= DCTSIZE) /* reload downsampled-data buffer */
            { int n = sp->cinfo.d.max_v_samp_factor*DCTSIZE;

              if (TIFFojpeg_read_raw_data(sp,sp->ds_buffer,n) != n) return 0;
              sp->scancount = 0;
            };
          if (sp->cinfo.d.num_components > 0)
            { int ci = 0, clumpoffset = 0;
              register jpeg_component_info *compptr = sp->cinfo.d.comp_info;

           /* The fastest way to separate the data is: make 1 pass over the scan
              line for each row of each component.
           */
              do
                { int ypos = 0;

                  if (compptr->h_samp_factor == 1) /* Cb & Cr fast path */
                    do
                      { register JSAMPLE *inptr =
                          sp->ds_buffer[ci][sp->scancount*compptr->v_samp_factor+ypos],
                                         *outptr = (JSAMPLE *)buf + clumpoffset;
                     /* Cb & Cr have sampling factors = 1, so this is correct */
                        register int clumps_per_line =
                          sp->cinfo.d.comp_info[1].downsampled_width;

                        do *outptr = *inptr++;
                        while ( (outptr += sp->samplesperclump)
                              , --clumps_per_line > 0
                              );
                      }
                    while ( (clumpoffset += compptr->h_samp_factor)
                          , ++ypos < compptr->v_samp_factor
                          );
                  else /* general case */
                    do
                      { register JSAMPLE *inptr =
                          sp->ds_buffer[ci][sp->scancount*compptr->v_samp_factor+ypos],
                                         *outptr = (JSAMPLE *)buf + clumpoffset;
                     /* Cb & Cr have sampling factors = 1, so this is correct */
                        register int clumps_per_line =
                          sp->cinfo.d.comp_info[1].downsampled_width;

                        do
                          { register int xpos = 0;

                            do outptr[xpos] = *inptr++;
                            while (++xpos < compptr->h_samp_factor);
                          }
                        while ( (outptr += sp->samplesperclump)
                              , --clumps_per_line > 0
                              );
                      }
                    while ( (clumpoffset += compptr->h_samp_factor)
                          , ++ypos < compptr->v_samp_factor
                          );
                }
              while (++compptr,++ci < sp->cinfo.d.num_components);
            };
          ++sp->scancount;
          buf += sp->bytesperline;
          ++tif->tif_row;
        }
      while ((cc -= sp->bytesperline) > 0 && --nrows > 0);
    return sp->cinfo.d.output_scanline < sp->cinfo.d.output_height
        || TIFFojpeg_finish_decompress(sp);
  }

/* "OJPEGPreDecode()" temporarily forces the JPEG Library to use the following
   subroutine as a "dummy" input reader, to fool it into thinking that it has
   read the image's 1st "Start of Scan" (SOS) marker and initialize accordingly.
*/
/*ARGSUSED*/ METHODDEF(int)
fake_SOS_marker(j_decompress_ptr cinfo){return JPEG_REACHED_SOS;}

/*ARGSUSED*/ METHODDEF(int)
suspend(j_decompress_ptr cinfo){return JPEG_SUSPENDED;}

/*ARGSUSED*/ static int
OJPEGPreDecode(register TIFF *tif,tsample_t s)
  { static const char bad_factors[]={"Improper JPEG sampling factors"},
                      module[]={"OJPEGPreDecode"};
    uint32 segment_width, segment_height;
    int downsampled_output = FALSE,
        is_JFIF;                                           /* <=> JFIF image? */
    J_COLOR_SPACE in_color_space = JCS_UNKNOWN;  /* Image's input color space */
    register OJPEGState *sp = OJState(tif);
#   define td (&tif->tif_dir)

    tif->tif_predecode = _TIFFNoPreCode; /* Don't call us again */

 /* BOGOSITY ALERT!  MicroSoft's Wang Imaging for Windows application produces
                     images containing "JPEGInterchangeFormat[Length]" TIFF
    records that resemble JFIF-in-TIFF encapsulations but, in fact, violate the
    TIFF Version 6 specification in several ways; nevertheless, we try to handle
    them gracefully because there are apparently a lot of them around.  The
    purported "JFIF" data stream in one of these files vaguely resembles a JPEG
    "tables only" data stream, except that there's no trailing EOI marker.  The
    rest of the JPEG data stream lies in a discontiguous file region, identified
    by the 0th Strip offset (which is *also* illegal!), where it begins with an
    SOS marker and apparently continues to the end of the file.  There is no
    trailing EOI marker here, either.
 */
    is_JFIF = !sp->is_WANG && TIFFFieldSet(tif,FIELD_JPEGIFOFFSET);

 /* Set up to decode a strip or tile.  Start by resetting decoder state left
    over from any previous strip/tile, in case our client application didn't
    read all of that data.  Then read the JPEG header data.
 */
    if (!TIFFojpeg_abort(sp)) return 0;

 /* Do a preliminary translation of the image's (input) color space from its
    TIFF representation to JPEG Library representation.  We might have to fix
    this up after calling "TIFFojpeg_read_header()", which tries to establish
    its own JPEG Library defaults.  While we're here, initialize some other
    decompression parameters that won't be overridden.
 */
    if (td->td_planarconfig == PLANARCONFIG_CONTIG)
      {
        if (sp->h_sampling != 1 || sp->v_sampling != 1)
          downsampled_output = TRUE; /* Tentative default */
        switch (sp->photometric) /* default color-space translation */
          {
            case PHOTOMETRIC_MINISBLACK: in_color_space = JCS_GRAYSCALE;
                                         break;
            case PHOTOMETRIC_RGB       : in_color_space = JCS_RGB;
                                         break;
            case PHOTOMETRIC_SEPARATED : in_color_space = JCS_CMYK;
                                         break;
            case PHOTOMETRIC_YCBCR     : in_color_space = JCS_YCbCr;
                                      /* JPEG Library converts YCbCr to RGB? */
                                         if (   sp->jpegcolormode
                                             == JPEGCOLORMODE_RGB
                                            ) downsampled_output = FALSE;
          }
      };
    segment_width = td->td_imagewidth;
    segment_height = td->td_imagelength - tif->tif_row;
    if (isTiled(tif))
      {
        if (sp->is_WANG) /* we don't know how to handle it */
          {
            TIFFError(module,"Tiled Wang image not supported");
            return 0;
          };

     /* BOGOSITY ALERT!  "TIFFTileRowSize()" seems to work fine for modern JPEG-
                         in-TIFF encapsulations where the image width--like the
        tile width--is a multiple of 8 or 16 pixels.  But image widths and
        heights are aren't restricted to 8- or 16-bit multiples, and we need
        the exact Byte count of decompressed scan lines when we call the JPEG
        Library.  At least one old file ("zackthecat.tif") in the TIFF Library
        test suite has widths and heights slightly less than the tile sizes, and
        it apparently used the bogus computation below to determine the number
        of Bytes per scan line (was this due to an old, broken version of
        "TIFFhowmany()"?).  Before we get here, "OJPEGSetupDecode()" verified
        that our image uses 8-bit samples, so the following check appears to
        return the correct answer in all known cases tested to date.
     */
        if (is_JFIF || (segment_width & 7) == 0)
          sp->bytesperline = TIFFTileRowSize(tif); /* Normal case */
        else
          {
            /* Was the file-encoder's segment-width calculation bogus? */
            segment_width = (segment_width/sp->h_sampling + 1) * sp->h_sampling;
            sp->bytesperline = segment_width * td->td_samplesperpixel;
          }
      }
    else sp->bytesperline = TIFFVStripSize(tif,1);
    if (td->td_planarconfig == PLANARCONFIG_SEPARATE && s > 0)
      {

     /* Scale the expected strip/tile size to match a downsampled component. */

        segment_width = TIFFhowmany(segment_width,sp->h_sampling);
        segment_height = TIFFhowmany(segment_height,sp->v_sampling);
      };

 /* BEWARE OF KLUDGE:  If we have JPEG Interchange File Format (JFIF) image,
                       then we want to read "metadata" in the bit-stream's
    header and validate it against corresponding information in TIFF records.
    But if we have a *really old* JPEG file that's not JFIF, then we simply
    assign TIFF-record values to JPEG Library variables without checking.
 */
    if (is_JFIF) /* JFIF image */
      { unsigned char *end_of_data;
        register unsigned char *p;

     /* WARNING:  Although the image file contains a JFIF bit stream, it might
                  also contain some old TIFF records causing "OJPEGVSetField()"
        to have allocated quantization or Huffman decoding tables.  But when the
        JPEG Library reads and parses the JFIF header below, it reallocate these
        tables anew without checking for "dangling" pointers, thereby causing a
        memory "leak".  We have enough information to potentially deallocate the
        old tables here, but unfortunately JPEG Library Version 6B uses a "pool"
        allocator for small objects, with no deallocation procedure; instead, it
        reclaims a whole pool when an image is closed/destroyed, so well-behaved
        TIFF client applications (i.e., those which close their JPEG images as
        soon as they're no longer needed) will waste memory for a short time but
        recover it eventually.  But ill-behaved TIFF clients (i.e., those which
        keep many JPEG images open gratuitously) can exhaust memory prematurely.
        If the JPEG Library ever implements a deallocation procedure, insert
        this clean-up code:
     */
#       ifdef someday
        if (sp->jpegtablesmode & JPEGTABLESMODE_QUANT) /* free quant. tables */
          { register int i = 0;

            do
              { register JQUANT_TBL *q;

                if (q = sp->cinfo.d.quant_tbl_ptrs[i])
                  {
                    jpeg_free_small(&sp->cinfo.comm,q,sizeof *q);
                    sp->cinfo.d.quant_tbl_ptrs[i] = 0;
                  }
              }
            while (++i < NUM_QUANT_TBLS);
          };
        if (sp->jpegtablesmode & JPEGTABLESMODE_HUFF) /* free Huffman tables */
          { register int i = 0;

            do
              { register JHUFF_TBL *h;

                if (h = sp->cinfo.d.dc_huff_tbl_ptrs[i])
                  {
                    jpeg_free_small(&sp->cinfo.comm,h,sizeof *h);
                    sp->cinfo.d.dc_huff_tbl_ptrs[i] = 0;
                  };
                if (h = sp->cinfo.d.ac_huff_tbl_ptrs[i])
                  {
                    jpeg_free_small(&sp->cinfo.comm,h,sizeof *h);
                    sp->cinfo.d.ac_huff_tbl_ptrs[i] = 0;
                  }
              }
            while (++i < NUM_HUFF_TBLS);
          };
#       endif /* someday */

     /* Since we might someday wish to try rewriting "old format" JPEG-in-TIFF
        encapsulations in "new format" files, try to synthesize the value of a
        modern "JPEGTables" TIFF record by scanning the JPEG data from just past
        the "Start of Information" (SOI) marker until something other than a
        legitimate "table" marker is found, as defined in ISO DIS 10918-1
        Appending B.2.4; namely:

        -- Define Quantization Table (DQT)
        -- Define Huffman Table (DHT)
        -- Define Arithmetic Coding table (DAC)
        -- Define Restart Interval (DRI)
        -- Comment (COM)
        -- Application data (APPn)

        For convenience, we also accept "Expansion" (EXP) markers, although they
        are apparently not a part of normal "table" data.
     */
        sp->jpegtables = p = (unsigned char *)sp->src.next_input_byte;
        end_of_data = p + sp->src.bytes_in_buffer;
        p += 2;
        while (p < end_of_data && p[0] == 0xFF)
          switch (p[1])
            {
              default  : goto L;
              case 0xC0: /* SOF0  */
              case 0xC1: /* SOF1  */
              case 0xC2: /* SOF2  */
              case 0xC3: /* SOF3  */
              case 0xC4: /* DHT   */
              case 0xC5: /* SOF5  */
              case 0xC6: /* SOF6  */
              case 0xC7: /* SOF7  */
              case 0xC9: /* SOF9  */
              case 0xCA: /* SOF10 */
              case 0xCB: /* SOF11 */
              case 0xCC: /* DAC   */
              case 0xCD: /* SOF13 */
              case 0xCE: /* SOF14 */
              case 0xCF: /* SOF15 */
              case 0xDB: /* DQT   */
              case 0xDD: /* DRI   */
              case 0xDF: /* EXP   */
              case 0xE0: /* APP0  */
              case 0xE1: /* APP1  */
              case 0xE2: /* APP2  */
              case 0xE3: /* APP3  */
              case 0xE4: /* APP4  */
              case 0xE5: /* APP5  */
              case 0xE6: /* APP6  */
              case 0xE7: /* APP7  */
              case 0xE8: /* APP8  */
              case 0xE9: /* APP9  */
              case 0xEA: /* APP10 */
              case 0xEB: /* APP11 */
              case 0xEC: /* APP12 */
              case 0xED: /* APP13 */
              case 0xEE: /* APP14 */
              case 0xEF: /* APP15 */
              case 0xFE: /* COM   */
                         p += (p[2] << 8 | p[3]) + 2;
            };
     L: if (p - (unsigned char *)sp->jpegtables > 2) /* fake "JPEGTables" */
          {

         /* In case our client application asks, pretend that this image file
            contains a modern "JPEGTables" TIFF record by copying to a buffer
            the initial part of the JFIF bit-stream that we just scanned, from
            the SOI marker through the "metadata" tables, then append an EOI
            marker and flag the "JPEGTables" TIFF record as "present".
         */
            sp->jpegtables_length = p - (unsigned char*)sp->jpegtables + 2;
            p = sp->jpegtables;
            if (!(sp->jpegtables = _TIFFmalloc(sp->jpegtables_length)))
              {
                TIFFError(module,no_jtable_space);
                return 0;
              };
            _TIFFmemcpy(sp->jpegtables,p,sp->jpegtables_length-2);
            p = (unsigned char *)sp->jpegtables + sp->jpegtables_length;
            p[-2] = 0xFF; p[-1] = JPEG_EOI; /* Append EOI marker */
            TIFFSetFieldBit(tif,FIELD_JPEGTABLES);
            tif->tif_flags |= TIFF_DIRTYDIRECT;
          }
        else sp->jpegtables = 0; /* Don't simulate "JPEGTables" */
        if (TIFFojpeg_read_header(sp,TRUE) != JPEG_HEADER_OK) return 0;
        if (   sp->cinfo.d.image_width  != segment_width
            || sp->cinfo.d.image_height != segment_height
           )
          {
            TIFFError(module,"Improper JPEG strip/tile size");
            return 0;
          };
        if ( ( td->td_planarconfig == PLANARCONFIG_CONTIG
             ? td->td_samplesperpixel
             : 1
             ) != sp->cinfo.d.num_components
           )
          {
            TIFFError(module,"Improper JPEG component count");
            return 0;
          };
        if (sp->cinfo.d.data_precision != td->td_bitspersample)
          {
            TIFFError(module,"Improper JPEG data precision");
            return 0;
          };
        /*if (td->td_planarconfig == PLANARCONFIG_CONTIG)
          { int ci;

         // Component 0 should have expected sampling factors.

            if (   sp->cinfo.d.comp_info[0].h_samp_factor != sp->h_sampling
                || sp->cinfo.d.comp_info[0].v_samp_factor != sp->v_sampling
               )
              {
                TIFFError(module,bad_factors);
                return 0;
              };
            ci = 1; // The rest should have sampling factors 1,1
            do if (   sp->cinfo.d.comp_info[ci].h_samp_factor != 1
                   || sp->cinfo.d.comp_info[ci].v_samp_factor != 1
                  )
                 {
                   TIFFError(module,bad_factors);
                   return 0;
                 }
            while (++ci < sp->cinfo.d.num_components);
          }
        else

       // PLANARCONFIG_SEPARATE's single component should have sampling factors
       
          if (   sp->cinfo.d.comp_info[0].h_samp_factor != 1
              || sp->cinfo.d.comp_info[0].v_samp_factor != 1
             )
            {
              TIFFError(module,bad_factors);
              return 0;
            }
		  */
      }
    else /* not JFIF image */
      { int (*save)(j_decompress_ptr cinfo) = sp->cinfo.d.marker->read_markers;
        register int i;

     /* We're not assuming that this file's JPEG bit stream has any header
        "metadata", so fool the JPEG Library into thinking that we read a
        "Start of Input" (SOI) marker and a "Start of Frame" (SOFx) marker, then
        force it to read a simulated "Start of Scan" (SOS) marker when we call
        "TIFFojpeg_read_header()" below.  This should cause the JPEG Library to
        establish reasonable defaults.
     */
        sp->cinfo.d.marker->saw_SOI =       /* Pretend we saw SOI marker */
        sp->cinfo.d.marker->saw_SOF = TRUE; /* Pretend we saw SOF marker */
        sp->cinfo.d.marker->read_markers =
          sp->is_WANG ? suspend : fake_SOS_marker;
        sp->cinfo.d.global_state = DSTATE_INHEADER;
        sp->cinfo.d.Se = DCTSIZE2-1; /* Suppress JPEG Library warning */
        sp->cinfo.d.image_width  = segment_width;
        sp->cinfo.d.image_height = segment_height;
        sp->cinfo.d.data_precision = td->td_bitspersample;

     /* The following color-space initialization, including the complicated
        "switch"-statement below, essentially duplicates the logic used by the
        JPEG Library's "jpeg_init_colorspace()" subroutine during compression.
     */
        sp->cinfo.d.num_components = td->td_planarconfig == PLANARCONFIG_CONTIG
                                    ? td->td_samplesperpixel
                                    : 1;
        sp->cinfo.d.comp_info = (jpeg_component_info *)
          (*sp->cinfo.d.mem->alloc_small)
            ( &sp->cinfo.comm
            , JPOOL_IMAGE
            , sp->cinfo.d.num_components * sizeof *sp->cinfo.d.comp_info
            );
        i = 0;
        do
          {
            sp->cinfo.d.comp_info[i].component_index = i;
            sp->cinfo.d.comp_info[i].component_needed = TRUE;
            sp->cinfo.d.cur_comp_info[i] = &sp->cinfo.d.comp_info[i];
          }
        while (++i < sp->cinfo.d.num_components);
        switch (in_color_space)
          {
            case JCS_UNKNOWN  :
              i = 0;
              do
                {
                  sp->cinfo.d.comp_info[i].component_id = i;
                  sp->cinfo.d.comp_info[i].h_samp_factor =
                  sp->cinfo.d.comp_info[i].v_samp_factor = 1;
                }
              while (++i < sp->cinfo.d.num_components);
              break;
            case JCS_GRAYSCALE:
              sp->cinfo.d.comp_info[0].component_id =
              sp->cinfo.d.comp_info[0].h_samp_factor =
              sp->cinfo.d.comp_info[0].v_samp_factor = 1;
              break;
            case JCS_RGB      :
              sp->cinfo.d.comp_info[0].component_id = 'R';
              sp->cinfo.d.comp_info[1].component_id = 'G';
              sp->cinfo.d.comp_info[2].component_id = 'B';
              i = 0;
              do sp->cinfo.d.comp_info[i].h_samp_factor =
                 sp->cinfo.d.comp_info[i].v_samp_factor = 1;
              while (++i < sp->cinfo.d.num_components);
              break;
            case JCS_CMYK     :
              sp->cinfo.d.comp_info[0].component_id = 'C';
              sp->cinfo.d.comp_info[1].component_id = 'Y';
              sp->cinfo.d.comp_info[2].component_id = 'M';
              sp->cinfo.d.comp_info[3].component_id = 'K';
              i = 0;
              do sp->cinfo.d.comp_info[i].h_samp_factor =
                 sp->cinfo.d.comp_info[i].v_samp_factor = 1;
              while (++i < sp->cinfo.d.num_components);
              break;
            case JCS_YCbCr    :
              i = 0;
              do
                {
                  sp->cinfo.d.comp_info[i].component_id = i+1;
                  sp->cinfo.d.comp_info[i].h_samp_factor =
                  sp->cinfo.d.comp_info[i].v_samp_factor = 1;
                  sp->cinfo.d.comp_info[i].quant_tbl_no =
                  sp->cinfo.d.comp_info[i].dc_tbl_no =
                  sp->cinfo.d.comp_info[i].ac_tbl_no = i > 0;
                }
              while (++i < sp->cinfo.d.num_components);
              sp->cinfo.d.comp_info[0].h_samp_factor = sp->h_sampling;
              sp->cinfo.d.comp_info[0].v_samp_factor = sp->v_sampling;
          };
        sp->cinfo.d.comps_in_scan = sp->cinfo.d.num_components;
        i = TIFFojpeg_read_header(sp,(unsigned char)(!sp->is_WANG));
        sp->cinfo.d.marker->read_markers = save; /* Restore input method */
        if (sp->is_WANG) /* Microsoft Wang Imaging for Windows file */
          {
            if (i != JPEG_SUSPENDED) return 0;

         /* BOGOSITY ALERT!  Files generated by Microsoft's Wang Imaging
                             application are a special--and, technically
            illegal--case.  A JPEG SOS marker and rest of the data stream should
            be located at the end of the file, in a position identified by the
            0th Strip offset.
         */
            i = td->td_nstrips - 1;
            sp->src.next_input_byte = tif->tif_base + td->td_stripoffset[0];
            sp->src.bytes_in_buffer = td->td_stripoffset[i] -
              td->td_stripoffset[0] + td->td_stripbytecount[i];
            i = TIFFojpeg_read_header(sp,TRUE);
          };
        if (i != JPEG_HEADER_OK) return 0;
      };

 /* The JPEG Library doesn't seem to be as smart as we are about choosing
    suitable default input- and output color spaces for decompression, so fix
    things up here.
 */
    sp->cinfo.d.out_color_space =
      ((sp->cinfo.d.jpeg_color_space = in_color_space) == JCS_YCbCr)
      ? (sp->jpegcolormode == JPEGCOLORMODE_RGB ? JCS_RGB : JCS_YCbCr)
      : JCS_UNKNOWN; /* Suppress color-space handling */
    tif->tif_decoderow = tif->tif_decodestrip = tif->tif_decodetile =
      (sp->cinfo.d.raw_data_out = downsampled_output)
      ? OJPEGDecodeRaw : OJPEGDecode;
    if (!TIFFojpeg_start_decompress(sp)) return 0; /* Start JPEG decompressor */
    if (downsampled_output) /* allocate downsampled-data buffers */
      {
        if (!alloc_downsampled_buffers(tif,sp->cinfo.d.comp_info,
                                       sp->cinfo.d.num_components)
           ) return 0;
        sp->scancount = DCTSIZE; /* mark buffer empty */
      };
    return 1;
#   undef td
  }

static int
OJPEGVSetField(register TIFF *tif,ttag_t tag,va_list ap)
  { uint32 v32;
    register OJPEGState *sp = OJState(tif);
#   define td (&tif->tif_dir)

    switch (tag)
      {
#       ifdef COLORIMETRY_SUPPORT

     /* If a "ReferenceBlackWhite" TIFF tag appears in the file explicitly, undo
        any modified default definition that we might have installed below, then
        install the real one.
     */
        case TIFFTAG_REFERENCEBLACKWHITE   : if (td->td_refblackwhite)
                                               {
                                                 _TIFFfree(td->td_refblackwhite);
                                                 td->td_refblackwhite = 0;
                                               };
#       endif /* COLORIMETRY_SUPPORT */
        default                            : return
                                               (*sp->vsetparent)(tif,tag,ap);
#       ifdef COLORIMETRY_SUPPORT

     /* BEWARE OF KLUDGE:  Some old-format JPEG-in-TIFF files, including those
                           created by Microsoft's Wang Imaging application,
        illegally omit a "ReferenceBlackWhite" TIFF tag, even though the TIFF
        specification's default is intended for the RGB color space and is in-
        appropriate for the YCbCr color space ordinarily used for JPEG images.
        Since many TIFF client applications request the value of this tag
        immediately after a TIFF image directory is parsed, and before any other
        code in this module receives control, we are forced to fix this problem
        very early in image-file processing.  Fortunately, legal TIFF files are
        supposed to store their tags in numeric order, so a mandatory
        "PhotometricInterpretation" tag should always appear before an optional
        "ReferenceBlackWhite" tag.  So, we slyly peek ahead when we discover the
        desired photometry, by installing modified black and white reference
        levels.
     */
        case TIFFTAG_PHOTOMETRIC           :
          if (   (v32 = (*sp->vsetparent)(tif,tag,ap))
              && td->td_photometric == PHOTOMETRIC_YCBCR
             )
            if (td->td_refblackwhite = _TIFFmalloc(6*sizeof(float)))
              { register long top = 1 << td->td_bitspersample;

                td->td_refblackwhite[0] = 0;
                td->td_refblackwhite[1] = td->td_refblackwhite[3] =
                td->td_refblackwhite[5] = (float)(top - 1);
                td->td_refblackwhite[2] = td->td_refblackwhite[4] = (float)(top >> 1);
              }
            else
              {
                TIFFError(tif->tif_name,"Cannot define default reference black and white levels");
                v32 = 0;
              };
          return v32;
#       endif /* COLORIMETRY_SUPPORT */

     /* BEWARE OF KLUDGE:  According to Charles Auer <Bumble731@msn.com>, if our
                           input is a multi-image (multi-directory) JPEG-in-TIFF
        file created by Microsoft's Wang Imaging application, for some reason
        the first directory excludes the vendor-specific "WANG PageControl" tag
        (32934) that we check below, so the only other way to identify these
        directories is apparently to look for a software-identification tag with
        the substring, "Wang Labs".  Single-image files can apparently pass both
        tests, which causes no harm here, but what a mess this is!
     */
        case TIFFTAG_SOFTWARE              : if (   (v32 = (*sp->vsetparent)
                                                             (tif,tag,ap)
                                                    )
                                                 && strstr( td->td_software
                                                          , "Wang Labs"
                                                          )
                                                ) sp->is_WANG = 1;
                                             return v32;
        case TIFFTAG_JPEGPROC              :
        case TIFFTAG_JPEGIFOFFSET          :
        case TIFFTAG_JPEGIFBYTECOUNT       :
        case TIFFTAG_JPEGRESTARTINTERVAL   :
        case TIFFTAG_JPEGLOSSLESSPREDICTORS:
        case TIFFTAG_JPEGPOINTTRANSFORM    :
        case TIFFTAG_JPEGQTABLES           :
        case TIFFTAG_JPEGDCTABLES          :
        case TIFFTAG_JPEGACTABLES          :
        case TIFFTAG_WANG_PAGECONTROL      :
        case TIFFTAG_JPEGCOLORMODE         : ;
      };
    v32 = va_arg(ap,uint32); /* No. of values in this TIFF record */

 /* BEWARE:  The following actions apply only if we are reading a "source" TIFF
             image to be decompressed for a client application program.  If we
    ever enhance this file's CODEC to write "destination" JPEG-in-TIFF images,
    we'll need an "if"- and another "switch"-statement below, because we'll
    probably want to store these records' values in some different places.  Most
    of these need not be parsed here in order to decode JPEG bit stream, so we
    set boolean flags to note that they have been seen, but we otherwise ignore
    them.
 */
    switch (tag)
      { JHUFF_TBL **h;
        //float *refbw;

     /* Validate the JPEG-process code. */

        case TIFFTAG_JPEGPROC              :
          switch (v32)
            {
              default: TIFFError(tif->tif_name,"Unknown JPEG process");
                       return 0;
              case 14: TIFFError(JPEGLib_name,
                         "Does not support lossless Huffman coding");
                       return 0;
              case  1: ;
            };
          break;

     /* The TIFF Version 6.0 specification says that if the value of a TIFF
        "JPEGInterchangeFormat" record is 0, then we are to behave as if this
        record were absent; i.e., the data does *not* represent a JPEG Inter-
        change Format File (JFIF), so don't even set the boolean "I've been
        here" flag below.  Otherwise, the field's value represents the file
        offset of the JPEG SOI marker.
     */
        case TIFFTAG_JPEGIFOFFSET          :
          if (v32)
            {
              sp->src.next_input_byte = tif->tif_base + v32;
              break;
            };
          return 1;
        case TIFFTAG_JPEGIFBYTECOUNT       :
          sp->src.bytes_in_buffer = v32;
          break;

     /* The TIFF Version 6.0 specification says that if the JPEG "Restart"
        marker interval is 0, then the data has no "Restart" markers; i.e., we
        must behave as if this TIFF record were absent.  So, don't even set the
        boolean "I've been here" flag below.
     */
        case TIFFTAG_JPEGRESTARTINTERVAL   :
          if (v32)
            {
              sp->cinfo.d.restart_interval = v32;
              break;
            };
          return 1;

     /* We have a vector of offsets to quantization tables, so load 'em! */

        case TIFFTAG_JPEGQTABLES           :
          if (v32)
            { uint32 *v;
              int i;

              if (v32 > NUM_QUANT_TBLS)
                {
                  TIFFError(tif->tif_name,"Too many quantization tables");
                  return 0;
                };
              i = 0;
              v = va_arg(ap,uint32 *);
              do /* read quantization table */
                { register UINT8 *from = tif->tif_base + *v++;
                  register UINT16 *to = (sp->cinfo.d.quant_tbl_ptrs[i] =
                                          jpeg_alloc_quant_table(&sp->cinfo.comm)
                                        )->quantval;
                  register int j = DCTSIZE2;

                  do *to++ = *from++; while (--j > 0);
                }
              while (++i < (int)v32);
              sp->jpegtablesmode |= JPEGTABLESMODE_QUANT;
            };
          break;

     /* We have a vector of offsets to DC Huffman tables, so load 'em! */

        case TIFFTAG_JPEGDCTABLES          :
          h = sp->cinfo.d.dc_huff_tbl_ptrs;
          goto L;

     /* We have a vector of offsets to AC Huffman tables, so load 'em! */

        case TIFFTAG_JPEGACTABLES          :
          h = sp->cinfo.d.ac_huff_tbl_ptrs;
       L: if (v32)
            { uint32 *v;
              int i;

              if (v32 > NUM_HUFF_TBLS)
                {
                  TIFFError(tif->tif_name,"Too many Huffman tables");
                  return 0;
                };
              v = va_arg(ap,uint32 *);
              i = 0;
              do /* copy each Huffman table */
                { int size = 0;
                  register UINT8 *from = tif->tif_base + *v++,
                                 *to = (*h++ =
                                         jpeg_alloc_huff_table(&sp->cinfo.comm)
                                       )->bits;
                  register int j = sizeof (*h)->bits;

               /* WARNING:  This code relies on the fact that an image file not
                            "memory mapped" was read entirely into a single
                  buffer by "TIFFInitOJPEG()", so we can do a fast memory-to-
                  memory copy here.  Each table consists of 16 Bytes, which are
                  suffixed to a 0 Byte when copied, followed by a variable
                  number of Bytes whose length is the sum of the first 16.
               */
                  *to++ = 0;
                  while (--j > 0) size += *to++ = *from++; /* Copy 16 Bytes */
                  if (size > sizeof (*h)->huffval/sizeof *(*h)->huffval)
                    {
                      TIFFError(tif->tif_name,"Huffman table too big");
                      return 0;
                    };
                  if ((j = size) > 0) do *to++ = *from++; while (--j > 0);
                  while (++size <= sizeof (*h)->huffval/sizeof *(*h)->huffval)
                    *to++ = 0; /* Zero the rest of the table for cleanliness */
                }
              while (++i < (int)v32);
              sp->jpegtablesmode |= JPEGTABLESMODE_HUFF;
            };
          break;

     /* The following vendor-specific TIFF tag occurs in (highly illegal) files
        generated by MicroSoft's Wang Imaging for Windows application.  These
        can apparently have several "pages", in which case this tag specifies
        the offset of a "page control" structure, which we don't currently know
        how to handle.  0 indicates a 1-page image with no "page control", which
        we make a feeble effort to handle.
     */
        case TIFFTAG_WANG_PAGECONTROL      :
          if (v32 == 0) v32 = -1;
          sp->is_WANG = v32;
          tag = TIFFTAG_JPEGPROC+FIELD_WANG_PAGECONTROL-FIELD_JPEGPROC;
          break;

     /* This pseudo tag indicates whether we think that our caller is supposed
        to do YCbCr<->RGB color-space conversion (JPEGCOLORMODE_RAW <=> 0) or
        whether we must ask the JPEG Library to do it (JPEGCOLORMODE_RGB <=> 1).
     */
        case TIFFTAG_JPEGCOLORMODE         :
          sp->jpegcolormode = (unsigned char)v32;

       /* Mark the image to indicate whether returned data is up-sampled, so
          that "TIFF{Strip,Tile}Size()" reflect the true amount of data present.
       */
          v32 = tif->tif_flags; /* Save flags temporarily */
          tif->tif_flags &= ~TIFF_UPSAMPLED;
          if (td->td_planarconfig == PLANARCONFIG_CONTIG)
            if (   td->td_photometric == PHOTOMETRIC_YCBCR
                && sp->jpegcolormode == JPEGCOLORMODE_RGB
               ) tif->tif_flags |= TIFF_UPSAMPLED;
            else
              if (   (td->td_ycbcrsubsampling[1]<<16|td->td_ycbcrsubsampling[0])
                  != (1 << 16 | 1)
                 ) /* XXX what about up-sampling? */;

       /* If the up-sampling state changed, re-calculate tile size. */

          if ((tif->tif_flags ^ v32) & TIFF_UPSAMPLED)
            {
              tif->tif_tilesize = TIFFTileSize(tif);
              tif->tif_flags |= TIFF_DIRTYDIRECT;
            };
          return 1;
      };
    TIFFSetFieldBit(tif,tag-TIFFTAG_JPEGPROC+FIELD_JPEGPROC);
    return 1;
#   undef td
  }

static int
OJPEGVGetField(register TIFF *tif,ttag_t tag,va_list ap)
  { register OJPEGState *sp = OJState(tif);

    switch (tag)
      {

     /* If this file has managed to synthesize a set of consolidated "metadata"
        tables for the current (post-TIFF Version 6.0 specification) JPEG-in-
        TIFF encapsulation strategy, then tell our caller about them; otherwise,
        keep mum.
     */
        case TIFFTAG_JPEGTABLES            :
          if (sp->jpegtables_length) /* we have "new"-style JPEG tables */
            {
              *va_arg(ap,uint32 *) = sp->jpegtables_length;
              *va_arg(ap,char **) = sp->jpegtables;
              return 1;
            };

     /* This pseudo tag indicates whether we think that our caller is supposed
        to do YCbCr<->RGB color-space conversion (JPEGCOLORMODE_RAW <=> 0) or
        whether we must ask the JPEG Library to do it (JPEGCOLORMODE_RGB <=> 1).
     */
        case TIFFTAG_JPEGCOLORMODE         :
          *va_arg(ap,uint32 *) = sp->jpegcolormode;
          return 1;

     /* The following tags are defined by the TIFF Version 6.0 specification
        and are obsolete.  If our caller asks for information about them, do not
        return anything, even if we parsed them in an old-format "source" image.
     */
        case TIFFTAG_JPEGPROC              :
        case TIFFTAG_JPEGIFOFFSET          :
        case TIFFTAG_JPEGIFBYTECOUNT       :
        case TIFFTAG_JPEGRESTARTINTERVAL   :
        case TIFFTAG_JPEGLOSSLESSPREDICTORS:
        case TIFFTAG_JPEGPOINTTRANSFORM    :
        case TIFFTAG_JPEGQTABLES           :
        case TIFFTAG_JPEGDCTABLES          :
        case TIFFTAG_JPEGACTABLES          : return 0;
      };
    return (*sp->vgetparent)(tif,tag,ap);
  }

/*ARGSUSED*/ static void
OJPEGPrintDir(register TIFF *tif,FILE *fd,long flags)
  { register OJPEGState *sp = OJState(tif);

    if (sp->jpegtables_length)
      fprintf(fd,"  JPEG Table Data: <present>, %lu bytes\n",
        sp->jpegtables_length);
  }

static uint32
OJPEGDefaultStripSize(register TIFF *tif,register uint32 s)
  { register OJPEGState *sp = OJState(tif);
#   define td (&tif->tif_dir)

    if ((s = (*sp->defsparent)(tif,s)) < td->td_imagelength)
      s = TIFFroundup(s,td->td_ycbcrsubsampling[1]*DCTSIZE);
    return s;
#   undef td
  }

static void
OJPEGDefaultTileSize(register TIFF *tif,register uint32 *tw,register uint32 *th)
  { register OJPEGState *sp = OJState(tif);
#   define td (&tif->tif_dir)

    (*sp->deftparent)(tif,tw,th);
    *tw = TIFFroundup(*tw,td->td_ycbcrsubsampling[0]*DCTSIZE);
    *th = TIFFroundup(*th,td->td_ycbcrsubsampling[1]*DCTSIZE);
#   undef td
  }

static void
OJPEGCleanUp(register TIFF *tif)
  { register OJPEGState *sp;

    if (sp = OJState(tif))
      {
        TIFFojpeg_destroy(sp); /* Release JPEG Library variables */
        if (sp->jpegtables) _TIFFfree(sp->jpegtables);

     /* If the image file isn't "memory mapped" and we read it all into a
        single, large memory buffer, free the buffer now.
     */
        if (!isMapped(tif) && tif->tif_base) /* free whole-file buffer */
          {
            _TIFFfree(tif->tif_base);
            tif->tif_base = 0;
            tif->tif_size = 0;
          };
        _TIFFfree(sp); /* Release local variables */
        tif->tif_data = 0;
      }
  }

int
TIFFInitOJPEG(register TIFF *tif,int scheme)
  { register OJPEGState *sp;
#   define td (&tif->tif_dir)
#   ifndef never

 /* This module supports a decompression-only CODEC, which is intended strictly
    for viewing old image files using the obsolete JPEG-in-TIFF encapsulation
    specified by the TIFF Version 6.0 specification.  It does not, and never
    should, support compression for new images.  If a client application asks us
    to, refuse and complain loudly!
 */
    if (tif->tif_mode != O_RDONLY) return _notSupported(tif);
#   endif /* never */
    if (!isMapped(tif))
      {

     /* BEWARE OF KLUDGE:  If our host operating-system doesn't let an image
                           file be "memory mapped", then we want to read the
        entire file into a single (possibly large) memory buffer as if it had
        been "memory mapped".  Although this is likely to waste space, because
        analysis of the file's content might cause parts of it to be read into
        smaller buffers duplicatively, it appears to be the lesser of several
        evils.  Very old JPEG-in-TIFF encapsulations aren't guaranteed to be
        JFIF bit streams, or to have a TIFF "JPEGTables" record or much other
        "metadata" to help us locate the decoding tables and entropy-coded data,
        so we're likely do a lot of random-access grokking around, and we must
        ultimately tell the JPEG Library to sequentially scan much of the file
        anyway.  This is all likely to be easier if we use "brute force" to
        read the entire file, once, and don't use incremental disc I/O.  If our
        client application tries to process a file so big that we can't buffer
        it entirely, then tough shit: we'll give up and exit!
     */
        if (!(tif->tif_base = _TIFFmalloc(tif->tif_size=TIFFGetFileSize(tif))))
          {
            TIFFError(tif->tif_name,"Cannot allocate file buffer");
            return 0;
          };
        if (!SeekOK(tif,0) || !ReadOK(tif,tif->tif_base,tif->tif_size))
          {
            TIFFError(tif->tif_name,"Cannot read file");
            return 0;
          }
      };

 /* Allocate storage for this module's per-file variables. */

    if (!(tif->tif_data = (tidata_t)_TIFFmalloc(sizeof *sp)))
      {
        TIFFError("TIFFInitOJPEG","No space for JPEG state block");
        return 0;
      };
    (sp = OJState(tif))->tif = tif; /* Initialize reverse pointer */
    if (!TIFFojpeg_create_decompress(sp)) return 0; /* Init. JPEG Library */

 /* If the image file doesn't have "JPEGInterchangeFormat[Length]" TIFF records
    to guide us, we have few clues about where its encapsulated JPEG bit stream
    is located, so establish intelligent defaults:  If the Image File Directory
    doesn't immediately follow the TIFF header, assume that the JPEG data lies
    in between; otherwise, assume that it follows the Image File Directory.
 */
    sp->src.next_input_byte = tif->tif_base + tif->tif_diroff; /* Default */
    if (tif->tif_header.tiff_diroff > sizeof tif->tif_header)
      {
        sp->src.bytes_in_buffer = tif->tif_header.tiff_diroff
                                - sizeof tif->tif_header;
        sp->src.next_input_byte = sp->src.next_input_byte
                                - sp->src.bytes_in_buffer;
      }
    else /* this case is ugly! */
      { uint16 dircount;

        dircount = *(uint16 *)sp->src.next_input_byte;
        if (tif->tif_flags & TIFF_SWAB) TIFFSwabShort(&dircount);
        sp->src.next_input_byte += dircount * sizeof(TIFFDirEntry)
                                + sizeof dircount;
        sp->src.bytes_in_buffer = tif->tif_base + tif->tif_nextdiroff
                                - sp->src.next_input_byte;
      };

 /* Install CODEC-specific tag information and override default TIFF Library
    "method" subroutines with our own, CODEC-specific methods.  Like all good
    members of an object-class, we save some of these subroutine pointers for
    "fall back" in case our own methods fail.
 */
    _TIFFMergeFieldInfo(tif,ojpegFieldInfo,
      sizeof ojpegFieldInfo/sizeof *ojpegFieldInfo);
    sp->defsparent = tif->tif_defstripsize;
    sp->deftparent = tif->tif_deftilesize;
    sp->vgetparent = tif->tif_vgetfield;
    sp->vsetparent = tif->tif_vsetfield;
    tif->tif_defstripsize = OJPEGDefaultStripSize;
    tif->tif_deftilesize = OJPEGDefaultTileSize;
    tif->tif_vgetfield = OJPEGVGetField;
    tif->tif_vsetfield = OJPEGVSetField;
    tif->tif_printdir = OJPEGPrintDir;
#   ifdef never
    tif->tif_setupencode = OJPEGSetupEncode;
    tif->tif_preencode = OJPEGPreEncode;
    tif->tif_postencode = OJPEGPostEncode;
#   else /* well, hardly ever */
    tif->tif_setupencode = tif->tif_postencode = _notSupported;
    tif->tif_preencode = (TIFFPreMethod)_notSupported;
#   endif /* never */
    tif->tif_setupdecode = OJPEGSetupDecode;
    tif->tif_predecode = OJPEGPreDecode;
    tif->tif_cleanup = OJPEGCleanUp;

 /* Initialize other CODEC-specific variables requiring default values. */

    tif->tif_flags |= TIFF_NOBITREV; /* No bit-reversal within data bytes */
    sp->is_WANG = 0; /* Assume not a Microsoft Wang Imaging file by default */
    sp->jpegtables = 0; /* No "new"-style JPEG tables synthesized yet */
    sp->jpegtables_length = 0;
    sp->jpegquality = 75; /* Default IJG quality */
    sp->jpegcolormode = JPEGCOLORMODE_RAW;
    sp->jpegtablesmode = 0; /* No tables found yet */
    return 1;
#   undef td
  }
#endif /* OJPEG_SUPPORT */
