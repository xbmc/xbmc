#if (defined(_LINUX) || ! defined(__APPLE__))

#import "ZeroconfAvahi.h"

#import <string>

CZeroconfAvahi::CZeroconfAvahi():
{
}

CZeroconfAvahi::~CZeroconfAvahi(){
}

void CZeroconfAvahi::doPublishWebserver(int f_port){
}

void  CZeroconfAvahi::doRemoveWebserver(){
}

void CZeroconfAvahi::doStop(){
}

#endif