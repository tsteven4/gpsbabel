/*
    Copyright (C) 2002-2022 Robert Lipe, robertlipe+source@gpsbabel.org

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
#include <QString>               // for QString
#include <QVector>               // for QVector
#include <Qt>                    // for CaseInsensitive

#include "geocache.h"            // for geocache_data, gc_other, gt_earth


const QVector<geocache_data::gs_type_mapping> geocache_data::gs_type_map = {
  { gt_traditional, "Traditional Cache" },
  { gt_traditional, "Traditional" }, /* opencaching.de */
  { gt_multi, "Multi-cache" },
  { gt_multi, "Multi" }, /* opencaching.de */
  { gt_virtual, "Virtual Cache" },
  { gt_virtual, "Virtual" }, /* opencaching.de */
  { gt_event, "Event Cache" },
  { gt_event, "Event" }, /* opencaching.de */
  { gt_webcam, "Webcam Cache" },
  { gt_webcam, "Webcam" }, /* opencaching.de */
  { gt_surprise, "Unknown Cache" },
  { gt_earth, "Earthcache" },
  { gt_earth, "Earth" }, /* opencaching.de */
  { gt_cito, "Cache In Trash Out Event" },
  { gt_letterbox, "Letterbox Hybrid" },
  { gt_locationless, "Locationless (Reverse) Cache" },
  { gt_ape, "Project APE Cache" },
  { gt_mega, "Mega-Event Cache" },
  { gt_wherigo, "Wherigo Cache" },

  { gt_benchmark, "Benchmark" } /* Not Groundspeak; for GSAK  */
};

const QVector<geocache_data::gs_container_mapping> geocache_data::gs_container_map = {
  { gc_other, "Unknown" },
  { gc_other, "Other" }, /* Synonym on read. */
  { gc_micro, "Micro" },
  { gc_regular, "Regular" },
  { gc_large, "Large" },
  { gc_small, "Small" },
  { gc_virtual, "Virtual" }
};

void geocache_data::gs_set_cachetype(const QString& type_name)
{
  for (const auto& map_entry : gs_type_map) {
    if (!type_name.compare(map_entry.name,Qt::CaseInsensitive)) {
      type = map_entry.type;
      return;
    }
  }
  type = gt_unknown;
}

QString geocache_data::gs_get_cachetype() const
{
  for (const auto& map_entry : gs_type_map) {
    if (type == map_entry.type) {
      return map_entry.name;
    }
  }
  return "Unknown";
}

void geocache_data::gs_set_container(const QString& container_name)
{
  for (const auto& map_entry : gs_container_map) {
    if (!container_name.compare(map_entry.name,Qt::CaseInsensitive)) {
      container =  map_entry.container;
      return;
    }
  }
  container = gc_unknown;
}

QString geocache_data::gs_get_container() const
{
  for (const auto& map_entry : gs_container_map) {
    if (container == map_entry.container) {
      return map_entry.name;
    }
  }
  return "Unknown";
}

/*
 * Return a QString that is suitable for icon lookup based on geocache
 * attributes.  The strings used are those present in a GPX file from
 * geocaching.com.  Thus we sort of make all the other formats do lookups
 * based on these strings.
 */
QString geocache_data::gs_get_icon() const
{
  if (!global_opts.smart_icons) {
    return nullptr;
  }

  /*
   * For icons, type overwrites container.  So a multi-micro will
   * get the icons for "multi".
   */
  switch (type) {
  case gt_virtual:
    return "Virtual cache";
  case gt_multi:
    return "Multi-Cache";
  case gt_event:
    return "Event Cache";
  case gt_surprise:
    return "Unknown Cache";
  case gt_webcam:
    return "Webcam Cache";
  default:
    break;
  }

  switch (container) {
  case gc_micro:
    return "Micro-Cache";
  default:
    break;
  }

  if (diff > 1) {
    return "Geocache";
  }

  return nullptr;
}
