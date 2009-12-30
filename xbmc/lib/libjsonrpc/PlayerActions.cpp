#include "PlayerActions.h"
#include "Application.h"

using namespace Json;

JSON_STATUS CPlayerActions::GetActivePlayers(const CStdString &method, const Value& parameterObject, Value &result)
{
/*  printf("%s\n", __FUNCTION__);

  PLAYERCOREID playerCore = g_application.GetCurrentPlayer();
  vector<CStdString> players;

  switch (playerCore)
  {
    case EPC_DVDPLAYER:
      players.push_back("dvdplayer");
      break;
    case EPC_MPLAYER:
      players.push_back("mplayer");
      break;
    case EPC_PAPLAYER:
      players.push_back("papplayer");
      break;
    case EPC_EXTPLAYER:
      players.push_back("extplayer");
      break;
    default:
      break;
  }

  result.Add("player", players);*/

  return OK;
}

JSON_STATUS CPlayerActions::Pause(const CStdString &method, const Value& parameterObject, Value &result)
{
 // if (parameterObject.has<string>("player"))
  {
    printf("%s\n", __FUNCTION__);

    return OK;
  }

  return InvalidParams;
}

JSON_STATUS CPlayerActions::SkipNext(const CStdString &method, const Value& parameterObject, Value &result)
{
  printf("%s\n", __FUNCTION__);
  return OK;
}

JSON_STATUS CPlayerActions::SkipPrevious(const CStdString &method, const Value& parameterObject, Value &result)
{
  printf("%s\n", __FUNCTION__);
  return OK;
}
