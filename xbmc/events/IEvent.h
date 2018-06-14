/*
 *      Copyright (C) 2015 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
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
