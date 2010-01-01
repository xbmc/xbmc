#include "PlayerActions.h"
#include "Application.h"
#include "Builtins.h"
#include "Util.h"

using namespace Json;

JSON_STATUS CPlayerActions::GetActivePlayers(const CStdString &method, const Value& parameterObject, Value &result)
{
  PLAYERCOREID playerCore = g_application.GetCurrentPlayer();

  if (playerCore != EPC_NONE)
    result["players"].append(CPlayerCoreFactory::GetPlayerName(playerCore).c_str());

  return OK;
}

JSON_STATUS CPlayerActions::GetAvailablePlayers(const CStdString &method, const Value& parameterObject, Value &result)
{
/*  static void GetPlayers( const CFileItem& item, VECPLAYERCORES &vecCores);   //Players supporting the specified file
  static void GetPlayers( VECPLAYERCORES &vecCores, bool audio, bool video ); //All audio players and/or video players
  static void GetPlayers( VECPLAYERCORES &vecCores );                         //All players*/
  VECPLAYERCORES vecCores;

  /*if (parameterObject.isMember("fileitem"))
  {
    CStdString path = parameterObject["fileitem"].asString().c_str();
    CFileItem fileitem(path, CUtil::HasSlashAtEnd(path));
    CPlayerCoreFactory::GetPlayers(fileitem, vecCores);
  }
  else */if (parameterObject.isMember("audio") || parameterObject.isMember("video"))
    CPlayerCoreFactory::GetPlayers(vecCores, parameterObject["audio"].asBool(), parameterObject["video"].asBool());
  else
    CPlayerCoreFactory::GetPlayers(vecCores);

  for (unsigned int i = 0; i < vecCores.size(); i++)
    result["players"].append(CPlayerCoreFactory::GetPlayerName(vecCores[i]).c_str());

  return OK;
}

JSON_STATUS CPlayerActions::PlayPause(const CStdString &method, const Value& parameterObject, Value &result)
{
  CBuiltins::Execute("playercontrol(play)");
  result["playing"] = g_application.IsPlaying();
  result["paused"] = g_application.IsPaused();
  return OK;
}

JSON_STATUS CPlayerActions::Stop(const CStdString &method, const Value& parameterObject, Value &result)
{
  CBuiltins::Execute("playercontrol(stop)");
  return OK;
}

JSON_STATUS CPlayerActions::SkipPrevious(const CStdString &method, const Value& parameterObject, Value &result)
{
  CBuiltins::Execute("playercontrol(previous)");
  return OK;
}

JSON_STATUS CPlayerActions::SkipNext(const CStdString &method, const Value& parameterObject, Value &result)
{
  CBuiltins::Execute("playercontrol(next)");
  return OK;
}

JSON_STATUS CPlayerActions::BigSkipBackward(const CStdString &method, const Value& parameterObject, Value &result)
{
  CBuiltins::Execute("playercontrol(bigskipbackward)");
  return OK;
}

JSON_STATUS CPlayerActions::BigSkipForward(const CStdString &method, const Value& parameterObject, Value &result)
{
  CBuiltins::Execute("playercontrol(bigskipforward)");
  return OK;
}

JSON_STATUS CPlayerActions::SmallSkipBackward(const CStdString &method, const Value& parameterObject, Value &result)
{
  CBuiltins::Execute("playercontrol(smallskipbackward)");
  return OK;
}

JSON_STATUS CPlayerActions::SmallSkipForward(const CStdString &method, const Value& parameterObject, Value &result)
{
  CBuiltins::Execute("playercontrol(smallskipforward)");
  return OK;
}

JSON_STATUS CPlayerActions::Rewind(const CStdString &method, const Value& parameterObject, Value &result)
{
  CBuiltins::Execute("playercontrol(rewind)");
  return OK;
}

JSON_STATUS CPlayerActions::Forward(const CStdString &method, const Value& parameterObject, Value &result)
{
  CBuiltins::Execute("playercontrol(forward)");
  return OK;
}

JSON_STATUS CPlayerActions::Record(const CStdString &method, const Value& parameterObject, Value &result)
{
  CBuiltins::Execute("playercontrol(record)");
  return OK;
}
