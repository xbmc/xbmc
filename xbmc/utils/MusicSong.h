#pragma once

namespace MUSIC_GRABBER
{
	class CMusicSong
	{
	public:
		CMusicSong(void);
		CMusicSong(int iTrack, const CStdString& strName, int iDuration);
		virtual ~CMusicSong(void);

		const CStdString& GetSongName() const;
		int								GetTrack() const;
		int								GetDuration() const;
		bool							Parse(const CStdString& strHTML);
		void							Save(FILE* fd);
		void							Load(FILE* fd);
	protected:
		int					m_iTrack;
		CStdString	m_strSongName;
		int					m_iDuration;
	};
};