#pragma once

#include "musicInfoTag.h"

using namespace MUSIC_INFO;

namespace PLAYLIST
{
	class CPlayList
	{
	public:
		class CPlayListItem : public CFileItem
		{
			public:
				CPlayListItem();
				CPlayListItem(const CStdString& strDescription, const CStdString& strFileName, long lDuration=0, long lStartOffset=0, long lEndOffset=0);

				virtual ~CPlayListItem();

				void							SetFileName(const CStdString& strFileName);
				const CStdString& GetFileName() const;

				void							SetDescription(const CStdString& strDescription);
				const CStdString& GetDescription() const;

				void						  SetDuration(long lDuration);
				long							GetDuration() const;

				void						  SetStartOffset(long lStartOffset);
				long							GetStartOffset() const;

				void						  SetEndOffset(long lEndOffset);
				long							GetEndOffset() const;

				void						SetMusicTag(const CMusicInfoTag &tag);
				CMusicInfoTag				GetMusicTag() const;

			protected:
				long			 m_lDuration;
		};
		CPlayList(void);
		virtual ~CPlayList(void);
		virtual bool 				Load(const CStdString& strFileName){return false;};
		virtual void 				Save(const CStdString& strFileName) const  {};
		void 								Add(const CPlayListItem& item);
		const CStdString&		GetName() const;
		void								Remove(const CStdString& strFileName);
		void								Remove(int position);
    bool								Swap(int position1, int position2);
		void 								Clear();
		virtual void 				Shuffle();
		int									size() const;
		int									RemoveDVDItems();
		const CPlayList::CPlayListItem& operator[] (int iItem)  const;
		CPlayList::CPlayListItem& operator[] (int iItem);

	protected:
		CStdString		m_strPlayListName;
		vector <CPlayListItem> m_vecItems;
		typedef vector <CPlayListItem>::iterator ivecItems;
	};
};
