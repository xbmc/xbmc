/*****************************************************************
|
|   Platinum - Light Sample Device
|
|   Copyright (c) 2004-2008, Plutinosoft, LLC.
|   Author: Sylvain Rebaud (sylvain@plutinosoft.com)
|
 ****************************************************************/

#ifndef _PLT_LIGHT_SAMPLE_H_
#define _PLT_LIGHT_SAMPLE_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "PltDeviceHost.h"

/*----------------------------------------------------------------------
|   PLT_LightSampleDevice class
+---------------------------------------------------------------------*/
class PLT_LightSampleDevice : public PLT_DeviceHost
{
public:
    PLT_LightSampleDevice(const char* FriendlyName,
                          const char* UUID = "");
    virtual ~PLT_LightSampleDevice();

    virtual NPT_Result OnAction(PLT_ActionReference&          action, 
                                const NPT_HttpRequestContext& context);
};

#endif /* _PLT_LIGHT_SAMPLE_H_ */
