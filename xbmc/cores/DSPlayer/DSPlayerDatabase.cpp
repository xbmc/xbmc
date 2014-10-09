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
#ifdef HAS_DS_PLAYER
#include "DSPlayerDatabase.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "dbwrappers/dataset.h"
#include "filesystem/StackDirectory.h"
#include "video/VideoInfoTag.h "
#include "settings/AdvancedSettings.h"

using namespace XFILE;

CEdition::CEdition()
	:editionNumber(0)
	, editionName("")
{
}

bool CEdition::IsSet() const
{
	return (!editionName.empty() && editionNumber >= 0);
}

CDSPlayerDatabase::CDSPlayerDatabase(void)
{
}


CDSPlayerDatabase::~CDSPlayerDatabase(void)
{
}

bool CDSPlayerDatabase::Open()
{
	return CDatabase::Open(g_advancedSettings.m_databaseDSPlayer);
}

int CDSPlayerDatabase::GetSchemaVersion() const
{
	return 1;
}

void CDSPlayerDatabase::CreateTables() 
{
	CLog::Log(LOGINFO, "create edition table");
	m_pDS->exec("CREATE TABLE edition (idEdition integer primary key, file text, editionName text, editionNumber integer)\n");
}

void CDSPlayerDatabase::CreateAnalytics()
{

	m_pDS->exec("CREATE INDEX idxEdition ON edition (file)");
}

bool CDSPlayerDatabase::GetResumeEdition(const CStdString& strFilenameAndPath, CEdition &edition)
{
	VECEDITIONS editions;
	GetEditionForFile(strFilenameAndPath, editions);
	if (editions.size() > 0)
	{
		edition = editions[0];
		return true;
	}
	return false;
}

bool CDSPlayerDatabase::GetResumeEdition(const CFileItem *item, CEdition &edition)
{
	CStdString strPath = item->GetPath();
	if ((item->IsVideoDb() || item->IsDVD()) && item->HasVideoInfoTag())
		strPath = item->GetVideoInfoTag()->m_strFileNameAndPath;

	return GetResumeEdition(strPath, edition);
}

void CDSPlayerDatabase::GetEditionForFile(const CStdString& strFilenameAndPath, VECEDITIONS& editions)
{
	try
	{
		if (NULL == m_pDB.get()) return ;
		if (NULL == m_pDS.get()) return ;

		CStdString strSQL=PrepareSQL("select * from edition where file='%s' order by editionNumber", strFilenameAndPath.c_str());
		m_pDS->query( strSQL.c_str() );
		while (!m_pDS->eof())
		{
			CEdition edition;
			edition.editionName = m_pDS->fv("editionName").get_asString();
			edition.editionNumber = m_pDS->fv("editionNumber").get_asInt();

			editions.push_back(edition);
			m_pDS->next();
		}

		m_pDS->close();
	}
	catch (...)
	{
		CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
	}
}

void CDSPlayerDatabase::AddEdition(const CStdString& strFilenameAndPath, const CEdition &edition)
{
	try
	{
		if(!edition.IsSet())		return ;
		if (NULL == m_pDB.get())    return ;
		if (NULL == m_pDS.get())    return ;

		CStdString strSQL;
		int idEdition= -1;

		strSQL=PrepareSQL("select idEdition from edition where file='%s'", strFilenameAndPath.c_str());

		m_pDS->query( strSQL.c_str() );
		if (m_pDS->num_rows() != 0)
			idEdition = m_pDS->get_field_value("idEdition").get_asInt();
		m_pDS->close();

		if (idEdition >= 0 )
			strSQL=PrepareSQL("update edition set  editionName = '%s', editionNumber = '%i' where idEdition = %i", edition.editionName.c_str(), edition.editionNumber, idEdition);
		else
			strSQL=PrepareSQL("insert into edition (idEdition, file, editionName, editionNumber) values(NULL, '%s', '%s', %i)", strFilenameAndPath.c_str(), edition.editionName.c_str(), edition.editionNumber);


		m_pDS->exec(strSQL.c_str());
	}
	catch (...)
	{
		CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
	}
}

void CDSPlayerDatabase::ClearEditionOfFile(const CStdString& strFilenameAndPath)
{
	try
	{
		if (NULL == m_pDB.get()) return ;
		if (NULL == m_pDS.get()) return ;

		CStdString strSQL=PrepareSQL("delete from edition where file='%s'", strFilenameAndPath.c_str());
		m_pDS->exec(strSQL.c_str());
	}
	catch (...)
	{
		CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, strFilenameAndPath.c_str());
	}
}
#endif