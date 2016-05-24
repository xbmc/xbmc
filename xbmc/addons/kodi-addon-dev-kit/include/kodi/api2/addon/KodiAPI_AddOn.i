%module KodiAPI_AddOn

%{
#include "General.hpp"
#include "Network.hpp"
#include "SoundPlay.hpp"
#include "VFSUtils.hpp"
#include "definitions_addon.hpp"

using namespace V2:KodiAPI::AddOn;
using namespace V2:KodiAPI;

%}

%include "General.hpp"
%include "Network.hpp"
%include "SoundPlay.hpp"
%include "VFSUtils.hpp"
%include "definitions_addon.hpp"
