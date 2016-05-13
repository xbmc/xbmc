%module KodiAPI_Player

%{
#include "Player.hpp"
#include "PlayList.hpp"
#include "InfoTagMusic.hpp"
#include "InfoTagVideo.hpp"
#include "definitions_player.hpp"

using namespace V2:KodiAPI::Player;
using namespace V2:KodiAPI;

%}

%include "Player.hpp"
%include "PlayList.hpp"
%include "InfoTagMusic.hpp"
%include "InfoTagVideo.hpp"
%include "definitions_player.hpp"
