#pragma once

#include "IDirectory.h"

namespace DIRECTORY
{
/*!
 \ingroup filesystem 
 \brief Get access to a directory of a file system.

 The Factory can be used to create a directory object
 for every file system accessable. \n
 \n
 Example:

 \verbatim
 CStdString strShare="iso9660://";

 IDirectory* pDir=CFactoryDirectory::Create(strShare);
 \endverbatim
 The \e pDir pointer can be used to access a directory and retrieve it's content.

 When different types of shares have to be accessed use CVirtualDirectory.
 \sa IDirectory
 */
class CFactoryDirectory
{
public:
  static IDirectory* Create(const CStdString& strPath);
};
}
