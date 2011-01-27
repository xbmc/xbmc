/*
    $Id: iso9660_fs.c,v 1.4 2005/01/20 01:00:52 rocky Exp $

    Copyright (C) 2001 Herbert Valerio Riedel <hvr@gnu.org>
    Copyright (C) 2003, 2004 Rocky Bernstein <rocky@panix.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_ERRNO_H
# include <errno.h>
#endif

#ifdef HAVE_ICONV
# include <iconv.h>
#endif

#ifdef HAVE_LANGINFO_CODESET
#include <langinfo.h>
#endif

#include <cdio/cdio.h>
#include <cdio/bytesex.h>
#include <cdio/iso9660.h>
#include <cdio/util.h>

/* Private headers */
#include "cdio_assert.h"
#include "_cdio_stdio.h"
#include "cdio_private.h"

#include <stdio.h>
#include "portable.h"

static const char _rcsid[] = "$Id: iso9660_fs.c,v 1.4 2005/01/20 01:00:52 rocky Exp $";

/* Implementation of iso9660_t type */
struct _iso9660 {
  CdioDataSource_t *stream; /* Stream pointer */
  bool b_xa;              /* true if has XA attributes. */
  uint8_t  i_joliet_level;/* 0 = no Joliet extensions.
			     1-3: Joliet level. */
  iso9660_pvd_t pvd;      
  iso9660_svd_t svd;      
  iso_extension_mask_t iso_extension_mask; /* What extensions we
					      tolerate. */
};

/*!
  Open an ISO 9660 image for reading. Maybe in the future we will have
  a mode. NULL is returned on error.
*/
iso9660_t *
iso9660_open (const char *pathname /*, mode*/)
{
  return iso9660_open_ext(pathname, ISO_EXTENSION_NONE);
}

/*!
  Open an ISO 9660 image for reading. Maybe in the future we will have
  a mode. NULL is returned on error.
*/
iso9660_t *
iso9660_open_ext (const char *pathname,
		  iso_extension_mask_t iso_extension_mask)
{
  iso9660_t *p_iso = (iso9660_t *) _cdio_malloc(sizeof(struct _iso9660)) ;

  if (NULL == p_iso) return NULL;
  
  p_iso->stream = cdio_stdio_new( pathname );
  if (NULL == p_iso->stream) 
    goto error;
  
  if ( !iso9660_ifs_read_superblock(p_iso, iso_extension_mask) )
    goto error;
  
  /* Determine if image has XA attributes. */
  
  p_iso->b_xa = !strncmp ((char *) &(p_iso->pvd) + ISO_XA_MARKER_OFFSET, 
			  ISO_XA_MARKER_STRING, 
			  strlen (ISO_XA_MARKER_STRING));
  p_iso->iso_extension_mask = iso_extension_mask;
  return p_iso;

 error:
  free(p_iso);
  return NULL;
}



/*!
  Close previously opened ISO 9660 image.
  True is unconditionally returned. If there was an error false would
  be returned.
*/
bool 
iso9660_close (iso9660_t *p_iso)
{
  if (NULL != p_iso) {
    cdio_stdio_destroy(p_iso->stream);
    free(p_iso);
  }
  return true;
}

static bool
check_pvd (const iso9660_pvd_t *p_pvd) 
{
  if ( ISO_VD_PRIMARY != from_711(p_pvd->type) ) {
    cdio_warn ("unexpected PVD type %d", p_pvd->type);
    return false;
  }
  
  if (strncmp (p_pvd->id, ISO_STANDARD_ID, strlen (ISO_STANDARD_ID)))
    {
      cdio_warn ("unexpected ID encountered (expected `"
		ISO_STANDARD_ID "', got `%.5s'", p_pvd->id);
      return false;
    }
  return true;
}

#ifdef HAVE_JOLIET
static bool
ucs2be_to_locale(ICONV_CONST char *psz_ucs2be,  size_t i_inlen, 
		 char **p_psz_out,  size_t i_outlen)
{

  iconv_t ic = 
#if defined(HAVE_LANGINFO_CODESET)
    iconv_open(nl_langinfo(CODESET), "UCS-2BE");
#else 
    iconv_open("ASCII", "UCS-2BE");
#endif

  int rc;
  char *psz_buf = NULL;
  char *psz_buf2;
  int i_outlen_max = i_outlen;
  int i_outlen_actual;

  if (-1 == (size_t) ic) {
#if defined(HAVE_LANGINFO_CODESET)
    cdio_info("Failed to get conversion table for locale, trying ASCII");
    ic = iconv_open("ASCII", "UCS-2BE");
    if (-1 == (size_t) ic) {
      cdio_info("Failed to get conversion table for ASCII too");
      return false;
    }
#else 
    cdio_info("Failed to get conversion table for locale");
    return false;
#endif
  }
  
  psz_buf = (char *) realloc(psz_buf, i_outlen);
  psz_buf2 = psz_buf;
  if (!psz_buf) {
    /* XXX: report out of memory error */
    goto error;
  }
  rc = iconv(ic, &psz_ucs2be, &i_inlen, &psz_buf2, &i_outlen);
  iconv_close(ic);
  if ((rc == -1) && (errno != E2BIG)) {
    /* conversion failed */
    goto error;
  }
  i_outlen_actual = i_outlen_max - i_outlen;
  *p_psz_out = malloc(i_outlen_actual + 1);
  memcpy(*p_psz_out, psz_buf, i_outlen_actual);
  *(*p_psz_out + i_outlen_actual) = '\0';
  free(psz_buf);
  return true;
 error:
  free(psz_buf);
  *p_psz_out = NULL; 
  return false;
}
#endif /*HAVE_JOLIET*/

/*!  
  Return the application ID.  NULL is returned in psz_app_id if there
  is some problem in getting this.
*/
bool
iso9660_ifs_get_application_id(iso9660_t *p_iso, 
			       /*out*/ char **p_psz_app_id)
{
  if (!p_iso) {
    *p_psz_app_id = NULL;
    return false;
  }

#ifdef HAVE_JOLIET  
  if (p_iso->i_joliet_level) {
    /* TODO: check that we haven't reached the maximum size.
       If we have, perhaps we've truncated and if we can get 
       longer results *and* have the same character using
       the PVD, do that.
     */
    if ( ucs2be_to_locale(p_iso->svd.application_id, 
			  ISO_MAX_APPLICATION_ID, 
			  p_psz_app_id, 
			  ISO_MAX_APPLICATION_ID))
      return true;
  }
#endif /*HAVE_JOLIET*/ 
  *p_psz_app_id = iso9660_get_application_id( &(p_iso->pvd) );
  return *p_psz_app_id != NULL && strlen(*p_psz_app_id);
}

/*!  
  Return the Joliet level recognaized for p_iso.
*/
uint8_t iso9660_ifs_get_joliet_level(iso9660_t *p_iso)
{
  if (!p_iso) return 0;
  return p_iso->i_joliet_level;
}

/*!
   Return a string containing the preparer id with trailing
   blanks removed.
*/
bool
iso9660_ifs_get_preparer_id(iso9660_t *p_iso,
			/*out*/ char **p_psz_preparer_id)
{
  if (!p_iso) {
    *p_psz_preparer_id = NULL;
    return false;
  }

#ifdef HAVE_JOLIET  
  if (p_iso->i_joliet_level) {
    /* TODO: check that we haven't reached the maximum size.
       If we have, perhaps we've truncated and if we can get 
       longer results *and* have the same character using
       the PVD, do that.
     */
    if ( ucs2be_to_locale(p_iso->svd.preparer_id, ISO_MAX_PREPARER_ID, 
			  p_psz_preparer_id, ISO_MAX_PREPARER_ID) )
      return true;
  }
#endif /*HAVE_JOLIET*/
  *p_psz_preparer_id = iso9660_get_preparer_id( &(p_iso->pvd) );
  return *p_psz_preparer_id != NULL && strlen(*p_psz_preparer_id);
}

/*!
   Return a string containing the PVD's publisher id with trailing
   blanks removed.
*/
bool iso9660_ifs_get_publisher_id(iso9660_t *p_iso,
                                  /*out*/ char **p_psz_publisher_id)
{
  if (!p_iso) {
    *p_psz_publisher_id = NULL;
    return false;
  }

#ifdef HAVE_JOLIET  
  if (p_iso->i_joliet_level) {
    /* TODO: check that we haven't reached the maximum size.
       If we have, perhaps we've truncated and if we can get 
       longer results *and* have the same character using
       the PVD, do that.
     */
    if( ucs2be_to_locale(p_iso->svd.publisher_id, ISO_MAX_PUBLISHER_ID, 
			 p_psz_publisher_id, ISO_MAX_PUBLISHER_ID) )
      return true;
  }
#endif /*HAVE_JOLIET*/
  *p_psz_publisher_id = iso9660_get_publisher_id( &(p_iso->pvd) );
  return *p_psz_publisher_id != NULL && strlen(*p_psz_publisher_id);
}


/*!
   Return a string containing the PVD's publisher id with trailing
   blanks removed.
*/
bool iso9660_ifs_get_system_id(iso9660_t *p_iso,
			       /*out*/ char **p_psz_system_id)
{
  if (!p_iso) {
    *p_psz_system_id = NULL;
    return false;
  }

#ifdef HAVE_JOLIET  
  if (p_iso->i_joliet_level) {
    /* TODO: check that we haven't reached the maximum size.
       If we have, perhaps we've truncated and if we can get 
       longer results *and* have the same character using
       the PVD, do that.
     */
    if ( ucs2be_to_locale(p_iso->svd.system_id, ISO_MAX_SYSTEM_ID, 
			  p_psz_system_id, ISO_MAX_SYSTEM_ID) )
      return true;
  }
#endif /*HAVE_JOLIET*/
  *p_psz_system_id = iso9660_get_system_id( &(p_iso->pvd) );
  return *p_psz_system_id != NULL && strlen(*p_psz_system_id);
}


/*!
   Return a string containing the PVD's publisher id with trailing
   blanks removed.
*/
bool iso9660_ifs_get_volume_id(iso9660_t *p_iso,
			       /*out*/ char **p_psz_volume_id)
{
  if (!p_iso) {
    *p_psz_volume_id = NULL;
    return false;
  }

#ifdef HAVE_JOLIET  
  if (p_iso->i_joliet_level) {
    /* TODO: check that we haven't reached the maximum size.
       If we have, perhaps we've truncated and if we can get 
       longer results *and* have the same character using
       the PVD, do that.
     */
    if ( ucs2be_to_locale(p_iso->svd.volume_id, ISO_MAX_VOLUME_ID, 
			  p_psz_volume_id, ISO_MAX_VOLUME_ID) )
      return true;
  }
#endif /* HAVE_JOLIET */
  *p_psz_volume_id = iso9660_get_volume_id( &(p_iso->pvd) );
  return *p_psz_volume_id != NULL && strlen(*p_psz_volume_id);
}


/*!
   Return a string containing the PVD's publisher id with trailing
   blanks removed.
*/
bool iso9660_ifs_get_volumeset_id(iso9660_t *p_iso,
				  /*out*/ char **p_psz_volumeset_id)
{
  if (!p_iso) {
    *p_psz_volumeset_id = NULL;
    return false;
  }

#ifdef HAVE_JOLIET  
  if (p_iso->i_joliet_level) {
    /* TODO: check that we haven't reached the maximum size.
       If we have, perhaps we've truncated and if we can get 
       longer results *and* have the same character using
       the PVD, do that.
     */
    if ( ucs2be_to_locale(p_iso->svd.volume_set_id, 
			  ISO_MAX_VOLUMESET_ID, 
			  p_psz_volumeset_id, 
			  ISO_MAX_VOLUMESET_ID) )
      return true;
  }
#endif /*HAVE_JOLIET*/
  *p_psz_volumeset_id = iso9660_get_volume_id( &(p_iso->pvd) );
  return *p_psz_volumeset_id != NULL && strlen(*p_psz_volumeset_id);
}


/*!
  Read the Primary Volume Descriptor for an ISO 9660 image.
  True is returned if read, and false if there was an error.
*/
bool 
iso9660_ifs_read_pvd (const iso9660_t *p_iso, /*out*/ iso9660_pvd_t *p_pvd)
{
  if (0 == iso9660_iso_seek_read (p_iso, p_pvd, ISO_PVD_SECTOR, 1)) {
    cdio_warn ("error reading PVD sector (%d)", ISO_PVD_SECTOR);
    return false;
  }
  return check_pvd(p_pvd);
}


/*!
  Read the Super block of an ISO 9660 image. This is the 
  Primary Volume Descriptor (PVD) and perhaps a Supplemental Volume 
  Descriptor if (Joliet) extensions are acceptable.
*/
bool 
iso9660_ifs_read_superblock (iso9660_t *p_iso, 
			     iso_extension_mask_t iso_extension_mask)
{
  iso9660_svd_t *p_svd;  /* Secondary volume descriptor. */
  
  if (!p_iso || !iso9660_ifs_read_pvd(p_iso, &(p_iso->pvd)))
    return false;

  p_svd = &(p_iso->svd);
  p_iso->i_joliet_level = 0;

  if (0 != iso9660_iso_seek_read (p_iso, p_svd, ISO_PVD_SECTOR+1, 1)) {
    if ( ISO_VD_SUPPLEMENTARY == from_711(p_svd->type) ) {
      if (p_svd->escape_sequences[0] == 0x25 
	  && p_svd->escape_sequences[1] == 0x2f) {
	switch (p_svd->escape_sequences[2]) {
	case 0x40:
	  if (iso_extension_mask & ISO_EXTENSION_JOLIET_LEVEL1) 
	    p_iso->i_joliet_level = 1;
	  break;
	case 0x43:
	  if (iso_extension_mask & ISO_EXTENSION_JOLIET_LEVEL2) 
	    p_iso->i_joliet_level = 2;
	  break;
	case 0x45:
	  if (iso_extension_mask & ISO_EXTENSION_JOLIET_LEVEL3) 
	    p_iso->i_joliet_level = 3;
	  break;
	default:
	  cdio_info("Supplementary Volume Descriptor found, but not Joliet");
	}
	if (p_iso->i_joliet_level > 0) {
	  cdio_info("Found Extension: Joliet Level %d", p_iso->i_joliet_level);
	}
      }
    }
  }
  
  return true;
}


/*!
  Read the Primary Volume Descriptor for of CD.
*/
bool 
iso9660_fs_read_pvd(const CdIo_t *p_cdio, /*out*/ iso9660_pvd_t *p_pvd)
{
  /* A bit of a hack, we'll assume track 1 contains ISO_PVD_SECTOR.*/
  bool b_mode2;
  char buf[CDIO_CD_FRAMESIZE_RAW] = { 0, };
  int i_rc;

  switch(cdio_get_track_format(p_cdio, 1)) {
  case TRACK_FORMAT_CDI:
  case TRACK_FORMAT_XA:
    b_mode2 = true;
    break;
  case TRACK_FORMAT_DATA:
    b_mode2 = false;
    break;
  case TRACK_FORMAT_AUDIO: 
  case TRACK_FORMAT_PSX: 
  case TRACK_FORMAT_ERROR: 
  default:
    return false;
  }
  
  i_rc = b_mode2 
      ? cdio_read_mode2_sector (p_cdio, buf, ISO_PVD_SECTOR, false)
      : cdio_read_mode1_sector (p_cdio, buf, ISO_PVD_SECTOR, false);
  
  if (i_rc) {
    cdio_warn ("error reading PVD sector (%d)", ISO_PVD_SECTOR);
    return false;
  }
  
  /* The size of a PVD or SVD is smaller than a sector. So we
     allocated a bigger block above (buf) and now we'll copy just
     the part we need to save.
   */
  cdio_assert (sizeof(buf) >= sizeof (iso9660_pvd_t));
  memcpy(p_pvd, buf, sizeof(iso9660_pvd_t));
  
  return check_pvd(p_pvd);
}


/*!
  Read the Super block of an ISO 9660 image. This is the 
  Primary Volume Descriptor (PVD) and perhaps a Supplemental Volume 
  Descriptor if (Joliet) extensions are acceptable.
*/
bool 
iso9660_fs_read_superblock (CdIo_t *p_cdio, 
			    iso_extension_mask_t iso_extension_mask)
{
  if (!p_cdio) return false;
  
  {
    generic_img_private_t *p_env = (generic_img_private_t *) p_cdio->env;
    iso9660_pvd_t         *p_pvd = &(p_env->pvd);
    iso9660_svd_t         *p_svd = &(p_env->svd);
    char buf[CDIO_CD_FRAMESIZE_RAW] = { 0, };
    bool                   b_mode2;
    int i_rc;

    /* A bit of a hack, we'll assume track 1 contains ISO_PVD_SECTOR.*/
    switch(cdio_get_track_format(p_cdio, 1)) {
    case TRACK_FORMAT_CDI:
    case TRACK_FORMAT_XA:
      b_mode2 = true;
      break;
    case TRACK_FORMAT_DATA:
      b_mode2 = false;
      break;
    case TRACK_FORMAT_AUDIO: 
    case TRACK_FORMAT_PSX: 
    case TRACK_FORMAT_ERROR: 
    default:
      return false;
    }
  
    if ( !iso9660_fs_read_pvd(p_cdio, p_pvd) )
      return false;
    
    p_env->i_joliet_level = 0;
    
    i_rc = (b_mode2)
      ? cdio_read_mode2_sector (p_cdio, buf, ISO_PVD_SECTOR+1, false)
      : cdio_read_mode1_sector (p_cdio, buf, ISO_PVD_SECTOR+1, false);

    if (0 == i_rc) {
      /* The size of a PVD or SVD is smaller than a sector. So we
	 allocated a bigger block above (buf) and now we'll copy just
	 the part we need to save.
      */
      cdio_assert (sizeof(buf) >= sizeof (iso9660_svd_t));
      memcpy(p_svd, buf, sizeof(iso9660_svd_t));
  
      if ( ISO_VD_SUPPLEMENTARY == from_711(p_svd->type) ) {
	if (p_svd->escape_sequences[0] == 0x25 
	    && p_svd->escape_sequences[1] == 0x2f) {
	  switch (p_svd->escape_sequences[2]) {
	  case 0x40:
	    if (iso_extension_mask & ISO_EXTENSION_JOLIET_LEVEL1) 
	      p_env->i_joliet_level = 1;
	    break;
	  case 0x43:
	    if (iso_extension_mask & ISO_EXTENSION_JOLIET_LEVEL2) 
	      p_env->i_joliet_level = 2;
	    break;
	  case 0x45:
	    if (iso_extension_mask & ISO_EXTENSION_JOLIET_LEVEL3) 
	      p_env->i_joliet_level = 3;
	    break;
	  default:
	    cdio_info("Supplementary Volume Descriptor found, but not Joliet");
	  }
	  if (p_env->i_joliet_level > 0) {
	    cdio_info("Found Extension: Joliet Level %d", 
		      p_env->i_joliet_level);
	  }
	}
      }
    }
  }

  return true;
}

/*!
  Seek to a position and then read n blocks. Size read is returned.
*/
long int 
iso9660_iso_seek_read (const iso9660_t *p_iso, void *ptr, lsn_t start, 
		       long int size)
{
  long int ret;
  if (NULL == p_iso) return 0;
  ret = cdio_stream_seek (p_iso->stream, start * ISO_BLOCKSIZE, SEEK_SET);
  if (ret!=0) return 0;
  return cdio_stream_read (p_iso->stream, ptr, ISO_BLOCKSIZE, size);
}


static iso9660_stat_t *
_iso9660_dir_to_statbuf (iso9660_dir_t *p_iso9660_dir, 
			 bool b_mode2, uint8_t i_joliet_level)
{
  iso9660_xa_t *xa_data = NULL;
  uint8_t dir_len= iso9660_get_dir_len(p_iso9660_dir);
  unsigned int filename_len;
  unsigned int stat_len;
  iso9660_stat_t *stat;

  if (!dir_len) return NULL;

  filename_len  = from_711(p_iso9660_dir->filename_len);

  /* .. string in statbuf is one longer than in p_iso9660_dir's listing '\1' */
  stat_len      = sizeof(iso9660_stat_t)+filename_len+2;

  stat          = _cdio_malloc(stat_len);
  stat->type    = (p_iso9660_dir->file_flags & ISO_DIRECTORY) 
    ? _STAT_DIR : _STAT_FILE;
  stat->lsn     = from_733 (p_iso9660_dir->extent);
  stat->size    = from_733 (p_iso9660_dir->size);
  stat->secsize = _cdio_len2blocks (stat->size, ISO_BLOCKSIZE);

  if ('\0' == p_iso9660_dir->filename[0] && 1 == filename_len)
    strcpy (stat->filename, ".");
	else if ('\1' == p_iso9660_dir->filename[0] && 1 == filename_len)
    strcpy (stat->filename, "..");
  else {
#ifdef HAVE_JOLIET
    if (i_joliet_level) {
      int i_inlen = filename_len;
      int i_outlen = (i_inlen / 2);
      char *p_psz_out = NULL;
      ucs2be_to_locale(p_iso9660_dir->filename, i_inlen, 
		       &p_psz_out, i_outlen);
      strncpy(stat->filename, p_psz_out, filename_len);
      free(p_psz_out);
    } else
#endif /*HAVE_JOLIET*/
      strncpy (stat->filename, p_iso9660_dir->filename, filename_len);
  }


  iso9660_get_dtime(&(p_iso9660_dir->recording_time), true, &(stat->tm));

  cdio_assert (dir_len >= sizeof (iso9660_dir_t));

  if (b_mode2) {
    int su_length = iso9660_get_dir_len(p_iso9660_dir) 
      - sizeof (iso9660_dir_t);
    su_length -= filename_len;
    
    if (su_length % 2)
      su_length--;
    
    if (su_length < 0 || su_length < sizeof (iso9660_xa_t))
      return stat;
    
    xa_data = (void *) (((char *) p_iso9660_dir) 
			+ (iso9660_get_dir_len(p_iso9660_dir) - su_length));
    
    if (xa_data->signature[0] != 'X' 
	|| xa_data->signature[1] != 'A')
    {
      cdio_warn ("XA signature not found in ISO9660's system use area;"
		 " ignoring XA attributes for this file entry.");
      cdio_debug ("%d %d %d, '%c%c' (%d, %d)", 
		  iso9660_get_dir_len(p_iso9660_dir), 
		  filename_len,
		  su_length,
		  xa_data->signature[0], xa_data->signature[1],
		  xa_data->signature[0], xa_data->signature[1]);
      return stat;
    }
    stat->xa = *xa_data;
  }
  return stat;
    
}

/*!
  Return the directory name stored in the iso9660_dir_t

  A string is allocated: the caller must deallocate.
 */
char *
iso9660_dir_to_name (const iso9660_dir_t *iso9660_dir)
{
  char namebuf[256] = { 0, };
  uint8_t len=iso9660_get_dir_len(iso9660_dir);

  if (!len) return NULL;

  cdio_assert (len >= sizeof (iso9660_dir_t));

  /* (iso9660_dir->file_flags & ISO_DIRECTORY) */
  
  if (iso9660_dir->filename[0] == '\0')
    strcpy (namebuf, ".");
  else if (iso9660_dir->filename[0] == '\1')
    strcpy (namebuf, "..");
  else
    strncpy (namebuf, iso9660_dir->filename, iso9660_dir->filename_len);

  return strdup (namebuf);
}

/* 
   Return a pointer to a ISO 9660 stat buffer or NULL if there's an error
*/
static iso9660_stat_t *
_fs_stat_root (CdIo_t *p_cdio)
{

  if (!p_cdio) return NULL;
  
  {
    iso_extension_mask_t iso_extension_mask = ISO_EXTENSION_ALL;
    generic_img_private_t *p_env = (generic_img_private_t *) p_cdio->env;
    bool b_mode2 = cdio_get_track_green(p_cdio, 1);
    iso9660_dir_t *p_iso9660_dir;
    iso9660_stat_t *p_stat;

    if (!p_env->i_joliet_level)
      iso_extension_mask &= ~ISO_EXTENSION_JOLIET;
    
    /* FIXME try also with Joliet.*/
    if ( !iso9660_fs_read_superblock (p_cdio, iso_extension_mask) ) {
      cdio_warn("Could not read ISO-9660 Superblock.");
      return NULL;
    }

#ifdef HAVE_JOLIET    
    p_iso9660_dir = p_env->i_joliet_level 
      ? &(p_env->svd.root_directory_record) 
      : &(p_env->pvd.root_directory_record) ;
#else
    p_iso9660_dir = &(p_env->pvd.root_directory_record) ;
#endif
    
    p_stat = _iso9660_dir_to_statbuf (p_iso9660_dir, b_mode2, 
				      p_env->i_joliet_level);
    return p_stat;
  }

}

static iso9660_stat_t *
_fs_stat_iso_root (iso9660_t *p_iso)
{
  iso9660_stat_t *p_stat;
  iso9660_dir_t *p_iso9660_dir;

#ifdef HAVE_JOLIET
  p_iso9660_dir = p_iso->i_joliet_level 
    ? &(p_iso->svd.root_directory_record)
    : &(p_iso->pvd.root_directory_record) ;
#else 
  p_iso9660_dir = &(p_iso->pvd.root_directory_record) ;
#endif

  p_stat = _iso9660_dir_to_statbuf (p_iso9660_dir, true, 
				    p_iso->i_joliet_level);
  return p_stat;
}

static iso9660_stat_t *
_fs_stat_traverse (const CdIo_t *p_cdio, const iso9660_stat_t *_root, 
		   char **splitpath, bool b_mode2, bool translate)
{
  unsigned offset = 0;
  uint8_t *_dirbuf = NULL;
  iso9660_stat_t *p_stat;
  generic_img_private_t *p_env = (generic_img_private_t *) p_cdio->env;

  if (!splitpath[0])
    {
      unsigned int len=sizeof(iso9660_stat_t) + strlen(_root->filename)+1;
      p_stat = _cdio_malloc(len);
      memcpy(p_stat, _root, len);
      return p_stat;
    }

  if (_root->type == _STAT_FILE)
    return NULL;

  cdio_assert (_root->type == _STAT_DIR);

  if (_root->size != ISO_BLOCKSIZE * _root->secsize)
    {
      cdio_warn ("bad size for ISO9660 directory (%ud) should be (%lu)!",
		 (unsigned) _root->size, 
		 (unsigned long int) ISO_BLOCKSIZE * _root->secsize);
    }
  
  _dirbuf = _cdio_malloc (_root->secsize * ISO_BLOCKSIZE);

  if (b_mode2) {
    if (cdio_read_mode2_sectors (p_cdio, _dirbuf, _root->lsn, false, 
				 _root->secsize))
      return NULL;
  } else {
    if (cdio_read_mode1_sectors (p_cdio, _dirbuf, _root->lsn, false,
				 _root->secsize))
      return NULL;
  }
  
  while (offset < (_root->secsize * ISO_BLOCKSIZE))
    {
      iso9660_dir_t *p_iso9660_dir = (void *) &_dirbuf[offset];
      iso9660_stat_t *p_stat;
      int cmp;

      if (!iso9660_get_dir_len(p_iso9660_dir))
	{
	  offset++;
	  continue;
	}
      
      p_stat = _iso9660_dir_to_statbuf (p_iso9660_dir, b_mode2, 
					p_env->i_joliet_level);

      if (translate) {
	char *trans_fname = malloc(strlen(p_stat->filename));
	int trans_len;
	
	if (trans_fname == NULL) {
	  cdio_warn("can't allocate %lu bytes", 
		    (long unsigned int) strlen(p_stat->filename));
	  return NULL;
	}
	trans_len = iso9660_name_translate_ext(p_stat->filename, trans_fname, 
					       p_env->i_joliet_level);
	cmp = strcmp(splitpath[0], trans_fname);
	free(trans_fname);
      } else {
	cmp = strcmp(splitpath[0], p_stat->filename);
      }
      
      if (!cmp) {
	iso9660_stat_t *ret_stat 
	  = _fs_stat_traverse (p_cdio, p_stat, &splitpath[1], b_mode2, 
			       translate);
	free(p_stat);
	free (_dirbuf);
	return ret_stat;
      }

      free(p_stat);
	  
      offset += iso9660_get_dir_len(p_iso9660_dir);
    }

  cdio_assert (offset == (_root->secsize * ISO_BLOCKSIZE));
  
  /* not found */
  free (_dirbuf);
  return NULL;
}

static iso9660_stat_t *
_fs_iso_stat_traverse (iso9660_t *p_iso, const iso9660_stat_t *_root, 
		       char **splitpath, bool translate)
{
  unsigned offset = 0;
  uint8_t *_dirbuf = NULL;
  int ret;

  if (!splitpath[0])
    {
      iso9660_stat_t *p_stat;
      unsigned int len=sizeof(iso9660_stat_t) + strlen(_root->filename)+1;
      p_stat = _cdio_malloc(len);
      memcpy(p_stat, _root, len);
      return p_stat;
    }

  if (_root->type == _STAT_FILE)
    return NULL;

  cdio_assert (_root->type == _STAT_DIR);

  if (_root->size != ISO_BLOCKSIZE * _root->secsize)
    {
      cdio_warn ("bad size for ISO9660 directory (%ud) should be (%lu)!",
		 (unsigned) _root->size, 
		 (unsigned long int) ISO_BLOCKSIZE * _root->secsize);
    }
  
  _dirbuf = _cdio_malloc (_root->secsize * ISO_BLOCKSIZE);

  ret = iso9660_iso_seek_read (p_iso, _dirbuf, _root->lsn, _root->secsize);
  if (ret!=ISO_BLOCKSIZE*_root->secsize) return NULL;
  
  while (offset < (_root->secsize * ISO_BLOCKSIZE))
    {
      iso9660_dir_t *p_iso9660_dir = (void *) &_dirbuf[offset];
      iso9660_stat_t *p_stat;
      int cmp;

      if (!iso9660_get_dir_len(p_iso9660_dir))
	{
	  offset++;
	  continue;
	}
      
      p_stat = _iso9660_dir_to_statbuf (p_iso9660_dir, true, 
					p_iso->i_joliet_level);

      if (translate) {
	char *trans_fname = malloc(strlen(p_stat->filename)+1);
	int trans_len;
	
	if (trans_fname == NULL) {
	  cdio_warn("can't allocate %lu bytes", 
		    (long unsigned int) strlen(p_stat->filename));
	  return NULL;
	}
	trans_len = iso9660_name_translate_ext(p_stat->filename, trans_fname, 
					       p_iso->i_joliet_level);
	cmp = strcmp(splitpath[0], trans_fname);
	free(trans_fname);
      } else {
	cmp = strcmp(splitpath[0], p_stat->filename);
      }
      
      if (!cmp) {
	iso9660_stat_t *ret_stat 
	  = _fs_iso_stat_traverse (p_iso, p_stat, &splitpath[1], translate);
	free(p_stat);
	free (_dirbuf);
	return ret_stat;
      }

      free(p_stat);
	  
      offset += iso9660_get_dir_len(p_iso9660_dir);
    }

  cdio_assert (offset == (_root->secsize * ISO_BLOCKSIZE));
  
  /* not found */
  free (_dirbuf);
  return NULL;
}

/*!
  Get file status for pathname into stat. NULL is returned on error.
 */
iso9660_stat_t *
iso9660_fs_stat (CdIo_t *p_cdio, const char pathname[])
{
  iso9660_stat_t *p_root;
  char **p_psz_splitpath;
  iso9660_stat_t *p_stat;
  /* A bit of a hack, we'll assume track 1 contains ISO_PVD_SECTOR.*/
  bool b_mode2;

  if (!p_cdio)   return NULL;
  if (!pathname) return NULL;

  p_root = _fs_stat_root (p_cdio);
  if (!p_root) return NULL;

  b_mode2 = cdio_get_track_green(p_cdio, 1);
  p_psz_splitpath = _cdio_strsplit (pathname, '/');
  p_stat = _fs_stat_traverse (p_cdio, p_root, p_psz_splitpath, b_mode2, false);
  free(p_root);
  _cdio_strfreev (p_psz_splitpath);

  return p_stat;
}

/*!
  Get file status for pathname into stat. NULL is returned on error.
  pathname version numbers in the ISO 9660
  name are dropped, i.e. ;1 is removed and if level 1 ISO-9660 names
  are lowercased.
 */
iso9660_stat_t *
iso9660_fs_stat_translate (CdIo_t *p_cdio, const char pathname[], 
			   bool b_mode2)
{
  iso9660_stat_t *p_root;
  char **p_psz_splitpath;
  iso9660_stat_t *p_stat;

  if (!p_cdio)  return NULL;
  if (pathname) return NULL;

  p_root = _fs_stat_root (p_cdio);
  if (!p_root) return NULL;

  p_psz_splitpath = _cdio_strsplit (pathname, '/');
  p_stat = _fs_stat_traverse (p_cdio, p_root, p_psz_splitpath, b_mode2, true);
  free(p_root);
  _cdio_strfreev (p_psz_splitpath);

  return p_stat;
}

/*!
  Get file status for pathname into stat. NULL is returned on error.
 */
iso9660_stat_t *
iso9660_ifs_stat (iso9660_t *p_iso, const char pathname[])
{
  iso9660_stat_t *p_root;
  char **splitpath;
  iso9660_stat_t *stat;

  if (!p_iso)    return NULL;
  if (!pathname) return NULL;

  p_root = _fs_stat_iso_root (p_iso);
  if (!p_root) return NULL;

  splitpath = _cdio_strsplit (pathname, '/');
  stat = _fs_iso_stat_traverse (p_iso, p_root, splitpath, false);
  free(p_root);
  /*** FIXME _cdio_strfreev (splitpath); ***/

  return stat;
}

/*!
  Get file status for pathname into stat. NULL is returned on error.
  pathname version numbers in the ISO 9660
  name are dropped, i.e. ;1 is removed and if level 1 ISO-9660 names
  are lowercased.
 */
iso9660_stat_t *
iso9660_ifs_stat_translate (iso9660_t *p_iso, const char pathname[])
{
  iso9660_stat_t *p_root;
  char **p_psz_splitpath;
  iso9660_stat_t *p_stat;

  if (!p_iso)    return NULL;
  if (!pathname) return NULL;

  p_root = _fs_stat_iso_root (p_iso);
  if (NULL == p_root) return NULL;

  p_psz_splitpath = _cdio_strsplit (pathname, '/');
  p_stat = _fs_iso_stat_traverse (p_iso, p_root, p_psz_splitpath, true);
  free(p_root);
  _cdio_strfreev (p_psz_splitpath);

  return p_stat;
}

/*! 
  Read pathname (a directory) and return a list of iso9660_stat_t
  of the files inside that. The caller must free the returned result.
*/
CdioList_t * 
iso9660_fs_readdir (CdIo_t *p_cdio, const char pathname[], bool b_mode2)
{
  iso9660_stat_t *p_stat;
  generic_img_private_t *p_env = (generic_img_private_t *) p_cdio->env;

  if (!p_cdio)   return NULL;
  if (!pathname) return NULL;

  p_stat = iso9660_fs_stat (p_cdio, pathname);
  if (!p_stat) return NULL;

  if (p_stat->type != _STAT_DIR) {
    free(p_stat);
    return NULL;
  }

  {
    unsigned offset = 0;
    uint8_t *_dirbuf = NULL;
    CdioList_t *retval = _cdio_list_new ();

    if (p_stat->size != ISO_BLOCKSIZE * p_stat->secsize)
      {
	cdio_warn ("bad size for ISO9660 directory (%ud) should be (%lu)!",
		   (unsigned) p_stat->size, 
		   (unsigned long int) ISO_BLOCKSIZE * p_stat->secsize);
      }

    _dirbuf = _cdio_malloc (p_stat->secsize * ISO_BLOCKSIZE);

    if (b_mode2) {
      if (cdio_read_mode2_sectors (p_cdio, _dirbuf, p_stat->lsn, false, 
				   p_stat->secsize))
	cdio_assert_not_reached ();
    } else {
      if (cdio_read_mode1_sectors (p_cdio, _dirbuf, p_stat->lsn, false,
				   p_stat->secsize))
	cdio_assert_not_reached ();
    }

    while (offset < (p_stat->secsize * ISO_BLOCKSIZE))
      {
	iso9660_dir_t *p_iso9660_dir = (void *) &_dirbuf[offset];
	iso9660_stat_t *p_iso9660_stat;
	
	if (!iso9660_get_dir_len(p_iso9660_dir))
	  {
	    offset++;
	    continue;
	  }

	p_iso9660_stat = _iso9660_dir_to_statbuf(p_iso9660_dir, b_mode2, 
						 p_env->i_joliet_level);
	_cdio_list_append (retval, p_iso9660_stat);

	offset += iso9660_get_dir_len(p_iso9660_dir);
      }

    cdio_assert (offset == (p_stat->secsize * ISO_BLOCKSIZE));

    free (_dirbuf);
    free (p_stat);
    return retval;
  }
}

/*! 
  Read pathname (a directory) and return a list of iso9660_stat_t
  of the files inside that. The caller must free the returned result.
*/
CdioList_t * 
iso9660_ifs_readdir (iso9660_t *p_iso, const char pathname[])
{
  iso9660_stat_t *p_stat;

  if (!p_iso)    return NULL;
  if (!pathname) return NULL;

  p_stat = iso9660_ifs_stat (p_iso, pathname);
  if (!p_stat)   return NULL;

  if (p_stat->type != _STAT_DIR) {
    free(p_stat);
    return NULL;
  }

  {
    long int ret;
    unsigned offset = 0;
    uint8_t *_dirbuf = NULL;
    CdioList_t *retval = _cdio_list_new ();

    if (p_stat->size != ISO_BLOCKSIZE * p_stat->secsize)
      {
	cdio_warn ("bad size for ISO9660 directory (%ud) should be (%lu)!",
		   (unsigned int) p_stat->size, 
		   (unsigned long int) ISO_BLOCKSIZE * p_stat->secsize);
      }

    _dirbuf = _cdio_malloc (p_stat->secsize * ISO_BLOCKSIZE);

    ret = iso9660_iso_seek_read (p_iso, _dirbuf, p_stat->lsn, p_stat->secsize);
    if (ret != ISO_BLOCKSIZE*p_stat->secsize) return NULL;
    
    while (offset < (p_stat->secsize * ISO_BLOCKSIZE))
      {
	iso9660_dir_t *p_iso9660_dir = (void *) &_dirbuf[offset];
	iso9660_stat_t *p_iso9660_stat;
	
	if (!iso9660_get_dir_len(p_iso9660_dir))
	  {
	    offset++;
	    continue;
	  }

	p_iso9660_stat = _iso9660_dir_to_statbuf(p_iso9660_dir, true,
						 p_iso->i_joliet_level);
	_cdio_list_append (retval, p_iso9660_stat);

	offset += iso9660_get_dir_len(p_iso9660_dir);
      }

    cdio_assert (offset == (p_stat->secsize * ISO_BLOCKSIZE));

    free (_dirbuf);
    free (p_stat);
    return retval;
  }
}

static iso9660_stat_t *
find_fs_lsn_recurse (CdIo_t *p_cdio, const char pathname[], lsn_t lsn)
{
  CdioList_t *entlist = iso9660_fs_readdir (p_cdio, pathname, true);
  CdioList_t *dirlist =  _cdio_list_new ();
  CdioListNode_t *entnode;
    
  cdio_assert (entlist != NULL);

  /* iterate over each entry in the directory */
  
  _CDIO_LIST_FOREACH (entnode, entlist)
    {
      iso9660_stat_t *statbuf = _cdio_list_node_data (entnode);
      char _fullname[4096] = { 0, };
      char *filename = (char *) statbuf->filename;

      snprintf (_fullname, sizeof (_fullname), "%s%s", pathname, filename);
  
      strncat (_fullname, "/", sizeof (_fullname));

      if (statbuf->type == _STAT_DIR
          && strcmp ((char *) statbuf->filename, ".") 
          && strcmp ((char *) statbuf->filename, ".."))
        _cdio_list_append (dirlist, strdup (_fullname));

      if (statbuf->lsn == lsn) {
	unsigned int len=sizeof(iso9660_stat_t)+strlen(statbuf->filename)+1;
	iso9660_stat_t *ret_stat = _cdio_malloc(len);
	memcpy(ret_stat, statbuf, len);
        _cdio_list_free (entlist, true);
        _cdio_list_free (dirlist, true);
        return ret_stat;
      }
      
    }

  _cdio_list_free (entlist, true);

  /* now recurse/descend over directories encountered */

  _CDIO_LIST_FOREACH (entnode, dirlist)
    {
      char *_fullname = _cdio_list_node_data (entnode);
      iso9660_stat_t *ret_stat = find_fs_lsn_recurse (p_cdio, _fullname, lsn);

      if (NULL != ret_stat) {
        _cdio_list_free (dirlist, true);
        return ret_stat;
      }
    }

  _cdio_list_free (dirlist, true);
  return NULL;
}

/*!
   Given a directory pointer, find the filesystem entry that contains
   lsn and return information about it.

   Returns stat_t of entry if we found lsn, or NULL otherwise.
 */
iso9660_stat_t *
iso9660_find_fs_lsn(CdIo_t *p_cdio, lsn_t i_lsn)
{
  return find_fs_lsn_recurse (p_cdio, "/", i_lsn);
}

/*!
  Return true if ISO 9660 image has extended attrributes (XA).
*/
bool 
iso9660_ifs_is_xa (const iso9660_t * p_iso) 
{
  if (!p_iso) return false;
  return p_iso->b_xa;
}
