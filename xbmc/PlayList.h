#pragma once

#include "StdString.h"
#include <vector>

using namespace std;

namespace PLAYLIST
{
	class CPlayList
	{
	public:
		class CPlayListItem
		{
			public:
				CPlayListItem();
				CPlayListItem(const CStdString& strDescription, const CStdString& strFileName, long lDuration=0);
				virtual ~CPlayListItem();

				void							SetFileName(const CStdString& strFileName);
				const CStdString& GetFileName() const;

				void							SetDescription(const CStdString& strDescription);
				const CStdString& GetDescription() const;

				void						  SetDuration(long lDuration);
				long							GetDuration() const;

			protected:
				CStdString m_strFilename;
				CStdString m_strDescription;
				long			 m_lDuration;
		};
		CPlayList(void);
		virtual ~CPlayList(void);
		virtual bool 				Load(const CStdString& strFileName){return false;};
		virtual void 				Save(const CStdString& strFileName) const  {};
		void 								Add(const CPlayListItem& item);
		const CStdString&		GetName() const;
		void								Remove(const CStdString& strFileName);
		void 								Clear();
		virtual void 				Shuffle();
		int									size() const;
		int									RemoveDVDItems();
		const CPlayList::CPlayListItem& operator[] (int iItem)  const;

	protected:
		CStdString		m_strPlayListName;
		vector <CPlayListItem> m_vecItems;
		typedef vector <CPlayListItem>::iterator ivecItems;
	};
};
