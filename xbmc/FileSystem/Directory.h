#pragma once
#include "../fileitem.h"

namespace DIRECTORY
{
	/*!
		\ingroup windows 
		\brief Interface to the directory on a file system.

		This Interface is retrieved from CFactoryDirectory and can be used to 
		access the directories on a filesystem.
		\sa CFactoryDirectory
		*/
  class CDirectory
  {
  public:
    CDirectory(void);
    virtual ~CDirectory(void);
		/*!
			\brief Get the \e items of the directory \e strPath.
			\param strPath Directory to read.
			\param items Retrieves the directory entries.
			\return Returns \e true, if successfull.
			\sa CFactoryDirectory
			*/
    virtual bool  GetDirectory(const CStdString& strPath,VECFILEITEMS &items)=0;
				bool	IsAllowed(const CStdString& strFile);
		void  SetMask(const CStdString& strMask);
	protected:
		CStdString	m_strFileMask;		///< Holds the file mask specified by SetMask()
  };

}