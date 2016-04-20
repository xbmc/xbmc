%module KodiAPI_PVR

%{
#include "General.hpp"
#include "StreamUtils.hpp"
#include "Transfer.hpp"
#include "Trigger.hpp"
#include "definitions_pvr.hpp"

using namespace V2:KodiAPI::PVR;
using namespace V2:KodiAPI;

%}

%include "General.hpp"
%include "StreamUtils.hpp"
%include "Transfer.hpp"
%include "Trigger.hpp"
%include "definitions_pvr.hpp"
