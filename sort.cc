/*
    Arbitrary Sorting Filter(s)

    Copyright (C) 2004 Robert Lipe, robertlipe+source@gpsbabel.org

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

#include "sort.h"

#include <QDateTime>            // for QDateTime
#include <QString>              // for operator<, QString

#include "defs.h"
#include "geocache.h"           // for Geocache
#include "src/core/datetime.h"  // for DateTime


#if FILTERS_ENABLED

bool SortFilter::sort_comp_wpt_by_description(const Waypoint* a, const Waypoint* b)
{
  return a->description < b->description;
}

bool SortFilter::sort_comp_wpt_by_gcid(const Waypoint* a, const Waypoint* b)
{
  return a->gc_data->id < b->gc_data->id;
}

bool SortFilter::sort_comp_wpt_by_shortname(const Waypoint* a, const Waypoint* b)
{
  return a->shortname < b->shortname;
}

bool SortFilter::sort_comp_wpt_by_time(const Waypoint* a, const Waypoint* b)
{
  return a->GetCreationTime() < b->GetCreationTime();
}

bool SortFilter::sort_comp_rh_by_description(const route_head* a, const route_head* b)
{
  return a->rte_desc < b->rte_desc;
}

bool SortFilter::sort_comp_rh_by_name(const route_head* a, const route_head* b)
{
  return a->rte_name < b->rte_name;
}

bool SortFilter::sort_comp_rh_by_number(const route_head* a, const route_head* b)
{
  return a->rte_num < b->rte_num;
}

void SortFilter::process()
{
  switch (wpt_sort_mode) {
  case SortModeWpt::none:
    break;
  case SortModeWpt::description:
    waypt_sort(sort_comp_wpt_by_description);
    break;
  case SortModeWpt::gcid:
    waypt_sort(sort_comp_wpt_by_gcid);
    break;
  case SortModeWpt::shortname:
    waypt_sort(sort_comp_wpt_by_shortname);
    break;
  case SortModeWpt::time:
    waypt_sort(sort_comp_wpt_by_time);
    break;
  default:
    gbFatal("unknown waypoint sort mode.\n");
  }

  switch (rte_sort_mode)  {
  case SortModeRteHd::none:
    break;
  case SortModeRteHd::description:
    route_sort(sort_comp_rh_by_description);
    break;
  case SortModeRteHd::name:
    route_sort(sort_comp_rh_by_name);
    break;
  case SortModeRteHd::number:
    route_sort(sort_comp_rh_by_number);
    break;
  default:
    gbFatal("unknown route sort mode.\n");
  }

  switch (trk_sort_mode)  {
  case SortModeRteHd::none:
    break;
  case SortModeRteHd::description:
    track_sort(sort_comp_rh_by_description);
    break;
  case SortModeRteHd::name:
    track_sort(sort_comp_rh_by_name);
    break;
  case SortModeRteHd::number:
    track_sort(sort_comp_rh_by_number);
    break;
  default:
    gbFatal("unknown track sort mode.\n");
  }
}

void SortFilter::init()
{
  // sort waypts by
  if (!opt_sm_description && !opt_sm_gcid && !opt_sm_shortname && !opt_sm_time) {
    wpt_sort_mode = SortModeWpt::none;
  } else if (opt_sm_description && !opt_sm_gcid && !opt_sm_shortname && !opt_sm_time) {
    wpt_sort_mode = SortModeWpt::description;
  } else if (!opt_sm_description && opt_sm_gcid && !opt_sm_shortname && !opt_sm_time) {
    wpt_sort_mode = SortModeWpt::gcid;
  } else if (!opt_sm_description && !opt_sm_gcid && opt_sm_shortname && !opt_sm_time) {
    wpt_sort_mode = SortModeWpt::shortname;
  } else if (!opt_sm_description && !opt_sm_gcid && !opt_sm_shortname && opt_sm_time) {
    wpt_sort_mode = SortModeWpt::time;
  } else {
    gbFatal("At most one of the options description, gcid, shortname and time may be selected.\n");
  }

  // sort routes by
  if (!opt_sm_rtedesc && !opt_sm_rtename && !opt_sm_rtenum) {
    rte_sort_mode = SortModeRteHd::none;
  } else if (opt_sm_rtedesc && !opt_sm_rtename && !opt_sm_rtenum) {
    rte_sort_mode = SortModeRteHd::description;
  } else if (!opt_sm_rtedesc && opt_sm_rtename && !opt_sm_rtenum) {
    rte_sort_mode = SortModeRteHd::name;
  } else if (!opt_sm_rtedesc && !opt_sm_rtename && opt_sm_rtenum) {
    rte_sort_mode = SortModeRteHd::number;
  } else {
    gbFatal("At most one of the options rtedesc, rtename and rtenum may be selected.\n");
  }

  // sort tracks by
  if (!opt_sm_trkdesc && !opt_sm_trkname && !opt_sm_trknum) {
    trk_sort_mode = SortModeRteHd::none;
  } else if (opt_sm_trkdesc && !opt_sm_trkname && !opt_sm_trknum) {
    trk_sort_mode = SortModeRteHd::description;
  } else if (!opt_sm_trkdesc && opt_sm_trkname && !opt_sm_trknum) {
    trk_sort_mode = SortModeRteHd::name;
  } else if (!opt_sm_trkdesc && !opt_sm_trkname && opt_sm_trknum) {
    trk_sort_mode = SortModeRteHd::number;
  } else {
    gbFatal("At most one of the options trkdesc, trkname and trknum may be selected.\n");
  }
}

#endif // FILTERS_ENABLED
