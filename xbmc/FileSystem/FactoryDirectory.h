#pragma once

#include "directory.h"
using namespace DIRECTORY;
namespace DIRECTORY
{
	/*!
		\ingroup windows 
		\brief Get access to a directory of a file system.

		The Factory can be used to create a directory object
		for every file system accessable. \n
		\n
		Example:

		\verbatim
		CStdString strShare="iso9660://";

		CFactoryDirectory factory;
		CDirectory* pDir=factory.Create(strShare);
		\endverbatim
		The \e pDir pointer can be used to access a directory and retrieve it's content.

		When different types of shares have to be accessed use CVirtualDirectory.
		\sa CDirectory
		*/
  class CFactoryDirectory
  {
  public:
    CFactoryDirectory(void);
    virtual ~CFactoryDirectory(void);
    CDirectory* Create(const CStdString& strPath);
  };
}
