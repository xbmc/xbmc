/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
/*!
 \file MusicDatabase.h
 \brief
 */
#pragma once
#include "dbwrappers/Database.h"
#include "utils/SortUtils.h"

class CArtist;
class CFileItem;

namespace dbiplus
{
    class field_value;
    typedef std::vector<field_value> sql_record;
}

#include <set>

// return codes of Cleaning up the Database
// numbers are strings from strings.xml
#define ERROR_OK     317
#define ERROR_CANCEL    0
#define ERROR_DATABASE    315
#define ERROR_REORG_SONGS   319
#define ERROR_REORG_ARTIST   321
#define ERROR_REORG_GENRE   323
#define ERROR_REORG_PATH   325
#define ERROR_REORG_ALBUM   327
#define ERROR_WRITING_CHANGES  329
#define ERROR_COMPRESSING   332

#define NUM_SONGS_BEFORE_COMMIT 500

/*!
 \ingroup music
 \brief A set of CStdString objects, used for CMusicDatabase
 \sa ISETPATHES, CMusicDatabase
 */
typedef std::set<CStdString> SETPATHES;

/*!
 \ingroup music
 \brief The SETPATHES iterator
 \sa SETPATHES, CMusicDatabase
 */
typedef std::set<CStdString>::iterator ISETPATHES;

class CGUIDialogProgress;
class CFileItemList;

/*!
 \ingroup music
 \brief Class to store and read tag information
 
 CMusicDatabase can be used to read and store
 tag information for faster access. It is based on
 sqlite (http://www.sqlite.org).
 
 Here is the database layout:
 \image html musicdatabase.png
 
 \sa CAlbum, CSong, VECSONGS, CMapSong, VECARTISTS, VECALBUMS, VECGENRES
 */
class CPictureDatabase : public CDatabase
{
    friend class DatabaseUtils;
    friend class TestDatabaseUtilsHelper;
    
public:
    CPictureDatabase(void);
    virtual ~CPictureDatabase(void);
    
    virtual bool Open();
    virtual bool CommitTransaction();
    void EmptyCache();
    void Clean();
    int  Cleanup(CGUIDialogProgress *pDlgProgress=NULL);    
    
protected:
    virtual bool CreateTables();
    virtual int GetMinVersion() const;
    
    const char *GetBaseDBName() const { return "MyPictures"; };
};