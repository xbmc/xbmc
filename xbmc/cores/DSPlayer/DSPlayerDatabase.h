#pragma once
/* 
 *  Copyright (C) 2010-2013 Eduard Kytmanov
 *  http://www.avmedia.su
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef HAS_DS_PLAYER
#error DSPlayer's header file included without HAS_DS_PLAYER defined
#endif

#include "dbwrappers/Database.h"
#include "FileItem.h"

class CEdition 
{
public:
	CEdition();

	CStdString editionName;
	int editionNumber;
	bool IsSet() const;
};

typedef std::vector<CEdition> VECEDITIONS;

class CDSPlayerDatabase : public CDatabase
{
public:
	CDSPlayerDatabase(void);
	~CDSPlayerDatabase(void);

	virtual bool Open();
	bool GetResumeEdition(const CFileItem *item, CEdition &edition);
	bool GetResumeEdition(const CStdString& strFilenameAndPath, CEdition &edition);
	void GetEditionForFile(const CStdString& strFilenameAndPath, VECEDITIONS &ditions);
	void AddEdition(const CStdString& strFilenameAndPath, const CEdition &edition);
	void ClearEditionOfFile(const CStdString& strFilenameAndPath);

protected:
	virtual void CreateTables();
	virtual void CreateAnalytics();
	virtual int GetSchemaVersion() const;
	const char *GetBaseDBName() const { return "DSPlayer"; };

};
