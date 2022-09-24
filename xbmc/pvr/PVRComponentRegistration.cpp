/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRComponentRegistration.h"

#include "pvr/guilib/PVRGUIActionsChannels.h"
#include "pvr/guilib/PVRGUIActionsClients.h"
#include "pvr/guilib/PVRGUIActionsDatabase.h"
#include "pvr/guilib/PVRGUIActionsEPG.h"
#include "pvr/guilib/PVRGUIActionsParentalControl.h"
#include "pvr/guilib/PVRGUIActionsPlayback.h"
#include "pvr/guilib/PVRGUIActionsPowerManagement.h"
#include "pvr/guilib/PVRGUIActionsRecordings.h"
#include "pvr/guilib/PVRGUIActionsTimers.h"
#include "pvr/guilib/PVRGUIActionsUtils.h"

#include <memory>

using namespace PVR;

CPVRComponentRegistration::CPVRComponentRegistration()
{
  RegisterComponent(std::make_shared<CPVRGUIActionsChannels>());
  RegisterComponent(std::make_shared<CPVRGUIActionsClients>());
  RegisterComponent(std::make_shared<CPVRGUIActionsDatabase>());
  RegisterComponent(std::make_shared<CPVRGUIActionsEPG>());
  RegisterComponent(std::make_shared<CPVRGUIActionsParentalControl>());
  RegisterComponent(std::make_shared<CPVRGUIActionsPlayback>());
  RegisterComponent(std::make_shared<CPVRGUIActionsPowerManagement>());
  RegisterComponent(std::make_shared<CPVRGUIActionsRecordings>());
  RegisterComponent(std::make_shared<CPVRGUIActionsTimers>());
  RegisterComponent(std::make_shared<CPVRGUIActionsUtils>());
}

CPVRComponentRegistration::~CPVRComponentRegistration()
{
  DeregisterComponent(typeid(CPVRGUIActionsUtils));
  DeregisterComponent(typeid(CPVRGUIActionsTimers));
  DeregisterComponent(typeid(CPVRGUIActionsRecordings));
  DeregisterComponent(typeid(CPVRGUIActionsPowerManagement));
  DeregisterComponent(typeid(CPVRGUIActionsPlayback));
  DeregisterComponent(typeid(CPVRGUIActionsParentalControl));
  DeregisterComponent(typeid(CPVRGUIActionsEPG));
  DeregisterComponent(typeid(CPVRGUIActionsDatabase));
  DeregisterComponent(typeid(CPVRGUIActionsClients));
  DeregisterComponent(typeid(CPVRGUIActionsChannels));
}
