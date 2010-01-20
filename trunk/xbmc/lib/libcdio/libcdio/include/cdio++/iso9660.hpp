/* -*- C++ -*-
    $Id: iso9660.hpp,v 1.11 2008/01/09 04:26:23 rocky Exp $

    Copyright (C) 2006, 2008 Rocky Bernstein <rocky@gnu.org>

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

/** \file iso9660.hpp
 *
 *  \brief C++ class for libcdio: the CD Input and Control
 *  library. Applications use this for anything regarding libcdio.
 */

#ifndef __ISO9660_HPP__
#define __ISO9660_HPP__

#include <cdio/iso9660.h>
#include <cdio++/cdio.hpp>
#include <string.h>
#include <cstdlib>
#include <vector>		// vector class library
#include <cstdlib>
using namespace std;

/** ISO 9660 class.
*/
class ISO9660
{

public:

  class PVD  // Primary Volume ID
  {
  public:

    iso9660_pvd_t pvd;  // Make private?

    PVD()
    {
      memset(&pvd, 0, sizeof(pvd));
    }
    
    PVD(iso9660_pvd_t *p_new_pvd)
    { 
      memcpy(&pvd, p_new_pvd, sizeof(pvd));
    };

    /*!
      Return the PVD's application ID.
      NULL is returned if there is some problem in getting this. 
    */
    char * get_application_id();
    
    int get_pvd_block_size();
    
    /*!
      Return the PVD's preparer ID.
      NULL is returned if there is some problem in getting this. 
    */
    char * get_preparer_id();
    
    /*!
      Return the PVD's publisher ID.
      NULL is returned if there is some problem in getting this. 
    */
    char * get_publisher_id();
    
    const char *get_pvd_id();
    
    int get_pvd_space_size();
    
    uint8_t get_pvd_type();
    
    /*! Return the primary volume id version number (of pvd).
      If there is an error 0 is returned. 
    */
    int get_pvd_version();
    
    /*! Return the LSN of the root directory for pvd.
      If there is an error CDIO_INVALID_LSN is returned. 
    */
    lsn_t get_root_lsn();
  
    /*!
      Return the PVD's system ID.
      NULL is returned if there is some problem in getting this. 
    */
    char * get_system_id();
    
    /*!
      Return the PVD's volume ID.
      NULL is returned if there is some problem in getting this. 
    */
    char * get_volume_id();
    
    /*!
      Return the PVD's volumeset ID.
      NULL is returned if there is some problem in getting this. 
    */
    char * get_volumeset_id();
    
  };
    
  class Stat  // ISO 9660 file information
  {
  public:

    iso9660_stat_t *p_stat;
    typedef vector< ISO9660::Stat *> stat_vector_t;

    Stat(iso9660_stat_t *p_new_stat)
    { 
      p_stat = p_new_stat;
    };

    Stat(const Stat& copy_in) 
    {
      free(p_stat);
      p_stat = (iso9660_stat_t *) 
	calloc( 1, sizeof(iso9660_stat_t) 
		+ strlen(copy_in.p_stat->filename)+1 );
      p_stat = copy_in.p_stat;
    }
      
    const Stat& operator= (const Stat& right) 
    {
      free(p_stat);
      this->p_stat = right.p_stat;
      return right;
    }
    
    ~Stat() 
    {
      free(p_stat);
      p_stat = NULL;
    }
    
  };

  class FS : public CdioDevice // ISO 9660 Filesystem on a CD or CD-image
  {
  public:

    typedef vector< ISO9660::Stat *> stat_vector_t;

    /*!
      Given a directory pointer, find the filesystem entry that contains
      lsn and return information about it.
      
      @return Stat * of entry if we found lsn, or NULL otherwise.
      Caller must free return value.
    */
    Stat *find_lsn(lsn_t i_lsn);

    /*! Read the Primary Volume Descriptor for a CD.  A
      PVD object is returned if read, and NULL if there was an error.
    */
    PVD *read_pvd ();

    /*!
      Read the Super block of an ISO 9660 image. This is the 
      Primary Volume Descriptor (PVD) and perhaps a Supplemental Volume 
      Descriptor if (Joliet) extensions are acceptable.
    */
    bool read_superblock (iso_extension_mask_t iso_extension_mask);

    /*! Read psz_path (a directory) and return a vector of iso9660_stat_t
      pointers for the files inside that directory. The caller must free the
      returned result.
    */
    bool readdir (const char psz_path[], stat_vector_t& stat_vector,
		  bool b_mode2=false);

    /*!
      Return file status for path name psz_path. NULL is returned on
      error.
      
      If translate is true, version numbers in the ISO 9660 name are
      dropped, i.e. ;1 is removed and if level 1 ISO-9660 names are
      lowercased.
      
      Mode2 is used only if translate is true and is a hack that
      really should go away in libcdio sometime. If set use mode 2
      reading, otherwise use mode 1 reading.
      
      @return file status object for psz_path. NULL is returned on
      error.
    */
    Stat *
    stat (const char psz_path[], bool b_translate=false, bool b_mode2=false)
    {
      if (b_translate) 
	return new Stat(iso9660_fs_stat_translate (p_cdio, psz_path, 
							    b_mode2));
      else 
	return new Stat(iso9660_fs_stat (p_cdio, psz_path));
    }
  };
  
  class IFS  // ISO 9660 filesystem image
  {
  public:

    typedef vector< ISO9660::Stat *> stat_vector_t;

    IFS()
    { 
      p_iso9660=NULL; 
    };

    ~IFS() 
    { 
      iso9660_close(p_iso9660); 
      p_iso9660 = (iso9660_t *) NULL;
    };
    
    /*! Close previously opened ISO 9660 image and free resources
      associated with the image. Call this when done using using an ISO
      9660 image.
      
      @return true is unconditionally returned. If there was an error
      false would be returned.
    */
    bool close();

    /*!
      Given a directory pointer, find the filesystem entry that contains
      lsn and return information about it.
      
      Returns Stat*  of entry if we found lsn, or NULL otherwise.
    */
    Stat *find_lsn(lsn_t i_lsn);

    /*!  
      Get the application ID.  psz_app_id is set to NULL if there
      is some problem in getting this and false is returned.
    */
    bool get_application_id(/*out*/ char * &psz_app_id) 
    {
      return iso9660_ifs_get_application_id(p_iso9660, &psz_app_id);
    }
    
    /*!  
      Return the Joliet level recognized. 
    */
    uint8_t get_joliet_level();
    
    /*!  
      Get the preparer ID.  psz_preparer_id is set to NULL if there
      is some problem in getting this and false is returned.
    */
    bool get_preparer_id(/*out*/ char * &psz_preparer_id) 
    {
      return iso9660_ifs_get_preparer_id(p_iso9660, &psz_preparer_id);
    }

    /*!  
      Get the publisher ID.  psz_publisher_id is set to NULL if there
      is some problem in getting this and false is returned.
    */
    bool get_publisher_id(/*out*/ char * &psz_publisher_id) 
    {
      return iso9660_ifs_get_publisher_id(p_iso9660, &psz_publisher_id);
    }
    
    /*!  
      Get the system ID.  psz_system_id is set to NULL if there
      is some problem in getting this and false is returned.
    */
    bool get_system_id(/*out*/ char * &psz_system_id)
    {
      return iso9660_ifs_get_system_id(p_iso9660, &psz_system_id);
    }
    
    /*! Return the volume ID in the PVD. psz_volume_id is set to
      NULL if there is some problem in getting this and false is
      returned.
    */
    bool get_volume_id(/*out*/ char * &psz_volume_id) 
    {
      return iso9660_ifs_get_volume_id(p_iso9660, &psz_volume_id);
    }
    
    /*! Return the volumeset ID in the PVD. psz_volumeset_id is set to
      NULL if there is some problem in getting this and false is
      returned.
    */
    bool get_volumeset_id(/*out*/ char * &psz_volumeset_id) 
    {
      return iso9660_ifs_get_volumeset_id(p_iso9660, &psz_volumeset_id);
    }

    /*!
      Return true if ISO 9660 image has extended attrributes (XA).
    */
    bool is_xa ();

    /*! Open an ISO 9660 image for reading. Maybe in the future we will
      have a mode. NULL is returned on error. An open routine should be
      called before using any read routine. If device object was
      previously opened it is closed first.
      
      @param psz_path location of ISO 9660 image
      @param iso_extension_mask the kinds of ISO 9660 extensions will be
      considered on access.
      
      @return true if open succeeded or false if error.
      
      @see open_fuzzy
    */
    bool open(const char *psz_path, 
	      iso_extension_mask_t iso_extension_mask=ISO_EXTENSION_NONE)
    {
      if (p_iso9660) iso9660_close(p_iso9660);
      p_iso9660 = iso9660_open_ext(psz_path, iso_extension_mask);
      return NULL != (iso9660_t *) p_iso9660 ;
    }

    /*! Open an ISO 9660 image for "fuzzy" reading. This means that we
      will try to guess various internal offset based on internal
      checks. This may be useful when trying to read an ISO 9660 image
      contained in a file format that libiso9660 doesn't know natively
      (or knows imperfectly.)
      
      Maybe in the future we will have a mode. NULL is returned on
      error.
      
      @see open
    */
    bool open_fuzzy (const char *psz_path,
		     iso_extension_mask_t iso_extension_mask
		     =ISO_EXTENSION_NONE,
		     uint16_t i_fuzz=20);

    /*! Read the Primary Volume Descriptor for an ISO 9660 image.  A
      PVD object is returned if read, and NULL if there was an error.
    */
    PVD *read_pvd ();
  
    /*!
      Read the Super block of an ISO 9660 image but determine framesize
      and datastart and a possible additional offset. Generally here we are
      not reading an ISO 9660 image but a CD-Image which contains an ISO 9660
      filesystem.

      @see read_superblock
    */
    bool read_superblock (iso_extension_mask_t iso_extension_mask
			  =ISO_EXTENSION_NONE,
			  uint16_t i_fuzz=20);

    /*!
      Read the Super block of an ISO 9660 image but determine framesize
      and datastart and a possible additional offset. Generally here we are
      not reading an ISO 9660 image but a CD-Image which contains an ISO 9660
      filesystem.

      @see read_superblock
    */
    bool 
    read_superblock_fuzzy (iso_extension_mask_t iso_extension_mask
			   =ISO_EXTENSION_NONE,
			   uint16_t i_fuzz=20);

    /*! Read psz_path (a directory) and return a list of iso9660_stat_t
      pointers for the files inside that directory. The caller must free
      the returned result.
    */
    bool readdir (const char psz_path[], stat_vector_t& stat_vector) 
    {
      CdioList_t *p_stat_list = iso9660_ifs_readdir (p_iso9660, psz_path);
      
      if (p_stat_list) {
	CdioListNode_t *p_entnode;
	_CDIO_LIST_FOREACH (p_entnode, p_stat_list) {
	  iso9660_stat_t *p_statbuf = 
	    (iso9660_stat_t *) _cdio_list_node_data (p_entnode);
	  stat_vector.push_back(new ISO9660::Stat(p_statbuf));
	}
	_cdio_list_free (p_stat_list, false);
	return true;
      } else {
	return false;
      }
    }

    /*!
      Seek to a position and then read n bytes. Size read is returned.
    */
    long int 
    seek_read (void *ptr, lsn_t start, long int i_size=1) 
    {
      return iso9660_iso_seek_read (p_iso9660, ptr, start, i_size);
    }
    
    /*!  
      Return file status for pathname. NULL is returned on error.
    */
    Stat *
    stat (const char psz_path[], bool b_translate=false) 
    {
      if (b_translate) 
	return new Stat(iso9660_ifs_stat_translate (p_iso9660, psz_path));
      else 
	return new Stat(iso9660_ifs_stat (p_iso9660, psz_path));
    }
    
  private:
    iso9660_t *p_iso9660;
  };

};

typedef vector< ISO9660::Stat *> stat_vector_t;
typedef vector <ISO9660::Stat *>::iterator stat_vector_iterator_t;

#endif /* __ISO9660_HPP__ */
