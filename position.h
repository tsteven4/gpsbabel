/*
    Distance Between Points Filter(s)

    Copyright (C) 2002 Robert Lipe, robertlipe+source@gpsbabel.org

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

 */

#ifndef POSITION_H_INCLUDED_
#define POSITION_H_INCLUDED_

#include <QList>     // for QList
#include <QString>    // for QString
#include <QVector>    // for QVector
#include <QtGlobal>   // for qint64

#include "defs.h"     // for arglist_t, route_head (ptr only), ARG_NOMINMAX, ARGTYPE_FLOAT, ARGTYPE_REQUIRED, ARGTYPE_BOOL, Waypoint, WaypointList (ptr only)
#include "filter.h"   // for Filter
#include "option.h"  // for OptionString, OptionBool


#if FILTERS_ENABLED

class PositionFilter:public Filter
{
public:
  QVector<arglist_t>* get_args() override
  {
    return &args;
  }
  void init() override;
  void process() override;

private:
  /* Types */

  class WptRecord
  {
  public:
    explicit WptRecord(Waypoint* w) : wpt(w) {}

    Waypoint* wpt{nullptr};
    bool deleted{false};
  };

  /* Member Functions */

  void position_runqueue(const WaypointList& waypt_list, int qtype);

  /* Data Members */

  double pos_dist{};
  qint64 max_diff_time{};
  OptionDouble distopt{true};
  OptionDouble timeopt;
  OptionBool purge_duplicates;
  bool check_time{};

  QVector<arglist_t> args = {
    {
      "distance", &distopt, "Maximum positional distance",
      nullptr, ARGTYPE_STRING | ARGTYPE_REQUIRED, ARG_NOMINMAX, nullptr
    },
    {
      "all", &purge_duplicates,
      "Suppress all points close to other points",
      nullptr, ARGTYPE_BOOL, ARG_NOMINMAX, nullptr
    },
    {
      "time", &timeopt, "Maximum time in seconds between two points",
      nullptr, ARGTYPE_FLOAT | ARGTYPE_REQUIRED, ARG_NOMINMAX, nullptr
    },
  };

};
#endif // FILTERS_ENABLED
#endif // POSITION_H_INCLUDED_
