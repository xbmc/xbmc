/*****************************************************************
|
|   Platinum - Light Sample Device
|
|   (c) 2004 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
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
    PLT_LightSampleDevice(char* FriendlyName,
                          char* UUID = "");
    virtual ~PLT_LightSampleDevice();

    virtual NPT_Result OnAction(PLT_ActionReference& action, NPT_SocketInfo* info = NULL);
};

#endif /* _PLT_LIGHT_SAMPLE_H_ */
