
#include "DirectoryHistory.h"
#include "../util.h"


CDirectoryHistory::CDirectoryHistory()
{
	m_strNull="";
}

CDirectoryHistory::~CDirectoryHistory()
{
}

void	CDirectoryHistory::Set(const CStdString&  strSelectedItem, const CStdString& strDirectory)
{
	if (strSelectedItem.size()==0) return;
	if (strDirectory.size()==0) return;
	OutputDebugString("Set:");
	OutputDebugString(strDirectory.c_str());
	OutputDebugString("=");
	OutputDebugString(strSelectedItem.c_str());
	OutputDebugString("\n");
	CStdString strDir=strDirectory;
	strDir.ToLower();
	while (CUtil::HasSlashAtEnd(strDir) )
	{
		strDir=strDir.Left(strDir.size()-1);
	}

	CStdString strItem=strSelectedItem;
	while (CUtil::HasSlashAtEnd(strItem) )
	{
		strItem=strItem.Left(strItem.size()-1);
	}


	for (int i=0; i < (int)m_vecHistory.size(); ++i)
	{
		CHistoryItem& item=m_vecHistory[i];
		if (  strDir==item.m_strDirectory) 
		{
			item.m_strItem=strItem;
			return;
		}
	}

	CHistoryItem item;
	item.m_strItem=strItem;
	item.m_strDirectory=strDir;
	m_vecHistory.push_back(item);
}

const CStdString& CDirectoryHistory::Get(const CStdString& strDirectory) const
{
	OutputDebugString("Get:");
	OutputDebugString(strDirectory.c_str());
	OutputDebugString("=");
	CStdString strDir=strDirectory;
	strDir.ToLower();
	while (CUtil::HasSlashAtEnd(strDir) )
	{
		strDir=strDir.Left(strDir.size()-1);
	}
	for (int i=0; i < (int)m_vecHistory.size(); ++i)
	{
		const CHistoryItem& item=m_vecHistory[i];
		if (  strDir==item.m_strDirectory) 
		{
			OutputDebugString(item.m_strItem.c_str());
			OutputDebugString("\n");

			return item.m_strItem;
		}
	}
	OutputDebugString("?\n");
	return m_strNull;
}
