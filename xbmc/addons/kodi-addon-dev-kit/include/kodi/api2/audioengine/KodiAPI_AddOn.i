%module KodiAPI_AudioEngine

%{
#include "General.hpp"
#include "Stream.hpp"
#include "Network.hpp"
#include "SoundPlay.hpp"
#include "VFSUtils.hpp"

using namespace V2:KodiAPI::AudioEngine;
using namespace V2:KodiAPI;

%}

%include "General.hpp"
%include "Stream.hpp"
