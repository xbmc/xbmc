/*****************************************************************
|
|   Platinum - Metadata Handler
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
 ****************************************************************/

#ifndef _PLT_METADATA_HANDLER_H_
#define _PLT_METADATA_HANDLER_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Neptune.h"

/*----------------------------------------------------------------------
|   PLT_MetadataHandler class
+---------------------------------------------------------------------*/
class PLT_MetadataHandler
{
public:
    virtual ~PLT_MetadataHandler() {}

    // metadata overridables
    virtual bool HandleExtension(const char* extension) = 0;
    virtual NPT_Result  Load(NPT_InputStream&  stream, 
                             NPT_TimeInterval  sleeptime = NPT_TimeInterval(0, 10000), 
                             NPT_TimeInterval  timeout = NPT_TimeInterval(30, 0)) = 0;
    virtual NPT_Result  Save(NPT_OutputStream& stream,
                             NPT_TimeInterval  sleeptime = NPT_TimeInterval(0, 10000), 
                             NPT_TimeInterval  timeout = NPT_TimeInterval(30, 0)) = 0;

    virtual const char* GetLicenseData(NPT_String& licenseData) = 0;
    virtual NPT_Result  GetCoverArtData(char*& caData, int& len) = 0;
    virtual const char* GetContentID(NPT_String& value) = 0;
    virtual const char* GetTitle(NPT_String& value) = 0;
    virtual const char* GetDescription(NPT_String& value) = 0;
    virtual NPT_Result  GetDuration(NPT_UInt32& seconds) = 0;
    virtual const char* GetProtection(NPT_String& protection) = 0;
    virtual NPT_Result  GetYear(NPT_Size& year) = 0;
    
    // helper functions
    virtual NPT_Result  LoadFile(const char* filename);
    virtual NPT_Result  SaveFile(const char* filename);
};

/*----------------------------------------------------------------------
|   PLT_MetadataHandlerFinder
+---------------------------------------------------------------------*/
class PLT_MetadataHandlerFinder
{
public:
    // methods
    PLT_MetadataHandlerFinder(const char* extension) : m_Extension(extension) {}
    bool operator()(PLT_MetadataHandler* const & handler) const {
        return handler->HandleExtension(m_Extension) ? true : false;
    }

private:
    // members
    NPT_String m_Extension;
};

#endif /* _PLT_METADATA_HANDLER_H_ */
