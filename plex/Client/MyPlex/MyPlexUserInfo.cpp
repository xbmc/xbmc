//
//  MyPlexUserInfo.cpp
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2013-04-12.
//  Copyright 2013 Plex Inc. All rights reserved.
//

#include "MyPlexUserInfo.h"
#include "utils/log.h"

bool CMyPlexUserInfo::SetFromXmlElement(TiXmlElement *root)
{
  if (root->QueryStringAttribute("authenticationToken", &authToken) != TIXML_SUCCESS)
  {
    CLog::Log(LOGERROR, "CMyPlexUserInfo::SetFromXmlElement return didn't contain authToken, don't know how to continue...");
    return false;
  }

  root->QueryIntAttribute("id", &id);
  root->QueryStringAttribute("email", &email);
  root->QueryStringAttribute("username", &username);
  root->QueryStringAttribute("queueEmail", &queueEmail);
  root->QueryStringAttribute("queueUid", &queueUID);
  root->QueryStringAttribute("cloudsyncdevice", &cloudSyncDevice);

  for (TiXmlElement* element = root->FirstChildElement(); element; element = element->NextSiblingElement())
  {
    if (element->ValueStr() == "subscription")
    {
      element->QueryBoolAttribute("active", &subscription);
      element->QueryStringAttribute("status", &subscriptionStatus);
      element->QueryStringAttribute("plan", &subscriptionPlan);

      for (TiXmlElement* feat = element->FirstChildElement(); feat; feat = feat->NextSiblingElement())
      {
        if (feat->ValueStr() == "feature")
        {
          std::string feature;
          feat->QueryStringAttribute("id", &feature);
          features.push_back(feature);
        }
      }
    }
    else if (element->ValueStr() == "joined-at")
    {
      CDateTime dateTime;
      dateTime.SetFromDBDateTime(std::string(element->GetText()));
      if (dateTime.IsValid())
        joinedAt = dateTime;
    }
  }

  return true;
}
