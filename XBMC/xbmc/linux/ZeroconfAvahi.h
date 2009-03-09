#pragma once

#if (defined(_LINUX) || ! defined(__APPLE__))

#include <memory>
#include "Zeroconf.h"


class CZeroconfAvahi : public CZeroconf{
public:
    CZeroconfAvahi();
    ~CZeroconfAvahi();
    
protected:
    //implement base CZeroConf interface
    virtual void doPublishWebserver(int f_port);
    virtual void doRemoveWebserver();
    virtual void doStop();
    
private:
};

#endifs