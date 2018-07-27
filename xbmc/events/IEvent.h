/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <string>

#include "utils/ISortable.h"

class CDateTime;

enum class EventLevel
{
  Basic = 0,
  Information = 1,
  Warning = 2,
  Error = 3,
};

class IEvent : public ISortable
{
public:
  virtual ~IEvent() = default;

  virtual const char* GetType() const = 0;
  virtual std::string GetIdentifier() const = 0;
  virtual EventLevel GetLevel() const = 0;
  virtual std::string GetLabel() const = 0;
  virtual std::string GetIcon() const = 0;
  virtual std::string GetDescription() const = 0;
  virtual std::string GetDetails() const = 0;
  virtual std::string GetExecutionLabel() const = 0;
  virtual CDateTime GetDateTime() const = 0;

  virtual bool CanExecute() const = 0;
  virtual bool Execute() const = 0;

  void ToSortable(SortItem& sortable, Field field) const override = 0;
};

typedef std::shared_ptr<const IEvent> EventPtr;
