#ifndef CShoutcastRipFile_H
#define CShoutcastRipFile_H

#include "../lib/libshout/rip_manager.h"

#include "../lib/libid3/id3.h"
#include "../lib/libid3/misc_support.h"

typedef struct RecStateSt
{
	bool		bRecording;
	bool		bCanRecord;
  bool		bTrackChanged;
	bool	  bFilenameSet;
	bool	  bStreamSet;
	bool	  bHasMetaData;
} RecState;


//if one would like to make other IFiles recordable, it would be
//great, to define a superclass for this one with differend useful 
//functions for all, like FilterFileName(), Record(),...
class CShoutcastRipFile
{
	public:
		
										CShoutcastRipFile();
		virtual					~CShoutcastRipFile();
		

		void						SetRipManagerInfo( const RIP_MANAGER_INFO* ripInfo );
		void						SetTrackname( const char* trackname );


		bool						Record();
		bool						CanRecord();
		void						StopRecording();
		bool						IsRecording();
		void						Write( char *buf, unsigned long size );
		void						Reset();
		void					  GetID3Tag(ID3_Tag &tag);
	protected:


  private:

		void			PrepareRecording( );
		void			RemoveIllegalChars( char *szRemoveIllegal );
		void			GetDirectoryName( char* directoryName  );
		void      SetFilename( const char* filePath, const char* fileName );
		void			CutAfterLastChar( char* szToBeCutted, int where );
		void	    RemoveLastSpace( char* szToBeRemoved );


		RecState		m_recState;
  	int					m_iTrackCount;
		char				m_szFileName[1024]; //FIXME: i think, that length could be optimized here
		char				m_szFilteredFileName[1024];
		char				m_szStreamName[1024];
	  FILE*				m_ripFile;
		FILE*				m_logFile;
		ID3_Tag			m_id3Tag;
};


#endif
