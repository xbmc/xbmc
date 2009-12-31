#include "PlayerActions.h"
#include "Application.h"

using namespace Json;

JSON_STATUS CPlayerActions::GetActivePlayers(const CStdString &method, const Value& parameterObject, Value &result)
{
  PLAYERCOREID playerCore = g_application.GetCurrentPlayer();

  switch (playerCore)
  {
    case EPC_DVDPLAYER:
      result["players"].append("dvdplayer");
      break;
    case EPC_MPLAYER:
      result["players"].append("mplayer");
      break;
    case EPC_PAPLAYER:
      result["players"].append("papplayer");
      break;
    case EPC_EXTPLAYER:
      result["players"].append("extplayer");
      break;
    default:
      break;
  }

  return OK;
}

JSON_STATUS CPlayerActions::Pause(const CStdString &method, const Value& parameterObject, Value &result)
{
  if (parameterObject.isMember("player"))
  {
    printf("%s\n", __FUNCTION__);

    return OK;
  }

  return InvalidParams;
}

JSON_STATUS CPlayerActions::SkipNext(const CStdString &method, const Value& parameterObject, Value &result)
{
  if (parameterObject.isMember("player"))
  {
    printf("%s\n", __FUNCTION__);

    return OK;
  }

  return InvalidParams;
}

JSON_STATUS CPlayerActions::SkipPrevious(const CStdString &method, const Value& parameterObject, Value &result)
{
  if (parameterObject.isMember("player"))
  {
    printf("%s\n", __FUNCTION__);

    return OK;
  }

  return InvalidParams;
}
