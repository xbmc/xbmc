#pragma once

#include <memory>
#include "Zeroconf.h"

class CZeroconfOSX : public CZeroconf{
public:
    CZeroconfOSX();
    ~CZeroconfOSX();
protected:
    //implement base CZeroConf interface
    bool doPublishService(const std::string& fcr_identifier,
                          const std::string& fcr_type,
                          const std::string& fcr_name,
                          unsigned int f_port);
    
    bool doRemoveService(const std::string& fcr_ident);
    
    //doHas is ugly ...
    bool doHasService(const std::string& fcr_ident);
    
    virtual void doStop();    
    
private:
    //another indirection with pimpl
    //CZeroconfOSXData stores the actual (objective-c-) data 
    class CZeroconfOSXData; 
    std::auto_ptr<CZeroconfOSXData> mp_data;
};
