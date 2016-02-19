%module KodiAPI_AddOn

%{
#include "General.hpp"
#include "Codec.hpp"
#include "Network.hpp"
#include "SoundPlay.hpp"
#include "VFSUtils.hpp"

using namespace V2:KodiAPI::AddOn;
using namespace V2:KodiAPI;

%}

%include "General.hpp"
%include "Codec.hpp"
%include "Network.hpp"
%include "SoundPlay.hpp"
%include "VFSUtils.hpp"
