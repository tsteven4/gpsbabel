/*
    Track resampling filter

    Copyright (C) 2021 Robert Lipe, robertlipe+source@gpsbabel.org

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

#include "resample.h"

#include <cmath>                // for round
#include <optional>             // for optional

#include <QDebug>               // for QDebug
#include <QList>                // for QList<>::const_iterator
#include <QString>              // for QString
#include <QTextStream>          // for qSetRealNumberPrecision
#include <QtGlobal>             // for qDebug, qAsConst, qint64

#include "defs.h"               // for Waypoint, route_head, fatal, WaypointList, track_add_wpt, track_disp_all, RouteList, track_add_head, track_del_wpt, track_swap, UrlList, gb_color, global_options, global_opts
#include "formspec.h"           // for FormatSpecificDataList
#include "src/core/datetime.h"  // for DateTime
#include "src/core/logging.h"   // for FatalMsg
#include "src/core/nvector.h"   // for NVector
#include "src/core/vector3d.h"  // for operator<<, Vector3D


#if FILTERS_ENABLED
#define MYNAME "resample"


void ResampleFilter::average_waypoint(Waypoint* wpt)
{
  // We filter in the n-vector coordinate system.
  // This removes difficulties at the discontinuity at longitude = +/- 180 degrees,
  // as well as at the singularities at the poles.
  // Our filter is from Gade, 5.3.6. Horizontal geographical mean, equation 17.
  if (location_history.isEmpty()) {
    gpsbabel::NVector current_position = gpsbabel::NVector(wpt->latitude, wpt->longitude);
    location_history.fill(current_position, average_count);
    accumulated_position = current_position * average_count;
  } else {
    accumulated_position -= location_history.at(counter); // subtract off the oldest position
    if (wpt->extra_data) {
      location_history[counter] = gpsbabel::Vector3D(0.0, 0.0, 0.0);
      wpt->extra_data = nullptr;
    } else {
      location_history[counter] = gpsbabel::NVector(wpt->latitude, wpt->longitude);
    }
    accumulated_position += location_history.at(counter); // and add in the newest position;
    if (global_opts.debug_level >= 5) {
      qDebug() << qSetRealNumberPrecision(12) << location_history.at(counter) << accumulated_position << accumulated_position.norm();
    }

    gpsbabel::NVector normalized_position = accumulated_position / accumulated_position.norm();
    wpt->latitude = normalized_position.latitude();
    wpt->longitude = normalized_position.longitude();

    counter = (counter + 1) % average_count;
  }
}

void ResampleFilter::process()
{
  if (interpolateopt) {
    RouteList backuproute;
    track_swap(backuproute);

    if (backuproute.empty()) {
      fatal(FatalMsg() << MYNAME ": Found no tracks to operate on.");
    }

    for (const auto* rte_old : qAsConst(backuproute)) {
      // FIXME: Allocating a new route_head and copying the members one at a
      // time is not maintainable.  When new members are added it is likely
      // they will not be copied here!
      // We want a deep copy of everything but with an empty WaypointList.
      auto* rte_new = new route_head;
      rte_new->rte_name = rte_old->rte_name;
      rte_new->rte_desc = rte_old->rte_desc;
      rte_new->rte_urls = rte_old->rte_urls;
      rte_new->rte_num = rte_old->rte_num;
      rte_new->fs = rte_old->fs.FsChainCopy();
      rte_new->line_color = rte_old->line_color;
      rte_new->line_width = rte_old->line_width;
      rte_new->session = rte_old->session;
      track_add_head(rte_new);

      bool first = true;
      const Waypoint* prevwpt;
      for (const Waypoint* wpt : rte_old->waypoint_list) {
        if (first) {
          first = false;
        } else {
          std::optional<qint64> timespan;
          if (prevwpt->creation_time.isValid() && wpt->creation_time.isValid()) {
            timespan = wpt->creation_time.toMSecsSinceEpoch() -
                       prevwpt->creation_time.toMSecsSinceEpoch();
          }

          track_add_wpt(rte_new, new Waypoint(*prevwpt));
          // Insert the required points
          for (int n = 0; n < interpolate_count - 1; ++n) {
            double frac = static_cast<double>(n + 1) /
                          static_cast<double>(interpolate_count);
            // We create the inserted point from the Waypoint at the
            // beginning of the span.  We clear some fields but use a
            // copy of the rest or the interpolated value.
            auto* wpt_new = new Waypoint(*prevwpt);
            wpt_new->shortname = QString();
            wpt_new->description = QString();
            if (timespan.has_value()) {
              wpt_new->SetCreationTime(0, prevwpt->creation_time.toMSecsSinceEpoch() +
                                       round(frac * *timespan));
            } else {
              wpt_new->creation_time = gpsbabel::DateTime();
            }
            wpt_new->latitude = 0.0;
            wpt_new->longitude = 0.0;
            wpt_new->extra_data = &wpt_zero_stuffed;
            track_add_wpt(rte_new, wpt_new);
          }

          if (wpt == rte_old->waypoint_list.back()) {
            track_add_wpt(rte_new, new Waypoint(*wpt));
          }
        }


        prevwpt = wpt;
      }
    }
    backuproute.flush();
  }

  if (averageopt) {
    auto route_hdr = [this](const route_head* rte)->void {
      // Filter in the forward direction
      counter = 0;
      location_history.clear();

      for (auto it = rte->waypoint_list.cbegin(); it != rte->waypoint_list.cend(); ++it)
      {
        Waypoint* wpt = *it;
        average_waypoint(wpt);
      }

      // Filter in the reverse direction
      counter = 0;
      location_history.clear();

      for (auto it = rte->waypoint_list.crbegin(); it != rte->waypoint_list.crend(); ++it)
      {
        Waypoint* wpt = *it;
        average_waypoint(wpt);
      }
    };

    track_disp_all(route_hdr, nullptr, nullptr);
  }

  if (decimateopt) {
    auto route_hdr =[this](const route_head* rte)->void {
      counter = 0;
      current_rte = rte;
    };

    auto waypt_cb = [this](const Waypoint* wpt)->void{
      if (counter != 0)
      {
        track_del_wpt(const_cast<route_head*>(current_rte), const_cast<Waypoint*>(wpt));
        delete wpt;
      }
      counter = (counter + 1) % decimate_count;
    };

    track_disp_all(route_hdr, nullptr, waypt_cb);
  }
}

void ResampleFilter::init()
{

  if (averageopt) {
    bool ok;
    average_count = QString(averageopt).toInt(&ok);
    if (!ok || average_count < 2) {
      fatal(FatalMsg() << MYNAME ": the average count must be greater than one.");
    }
  }

  if (decimateopt) {
    bool ok;
    decimate_count = QString(decimateopt).toInt(&ok);
    if (!ok || decimate_count < 2) {
      fatal(FatalMsg() << MYNAME ": the decimate count must be greater than one.");
    }
  }

  if (interpolateopt) {
    bool ok;
    interpolate_count = QString(interpolateopt).toInt(&ok);
    if (!ok || interpolate_count < 2) {
      fatal(FatalMsg() << MYNAME ": the interpolate count must be greater than one.");
    }
  }
}

void ResampleFilter::deinit()
{
  location_history.clear();
  location_history.squeeze();
}

#endif // FILTERS_ENABLED
