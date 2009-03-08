#pragma once

#include <memory>
#include "Zeroconf.h"

class CZeroconfOSX : public CZeroconf{
public:
    CZeroconfOSX();
    ~CZeroconfOSX();
protected:
    //implement base CZeroConf interface
    virtual void doPublishWebserver(int f_port);
    virtual void doRemoveWebserver();
    virtual void doStop();
    
private:
    //another indirection with pimpl
    //CZeroconfOSXData stores the actual (objective-c-) data 
    class CZeroconfOSXData; 
    std::auto_ptr<CZeroconfOSXData> mp_data;
};
