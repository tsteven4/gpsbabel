/*

    Support for MapSource Text Export (Tab delimited) files.

    Copyright (C) 2006 Olaf Klein, o.b.klein@gpsbabel.org
    Copyright (C) 2004-2022 Robert Lipe, robertlipe+source@gpsbabel.org

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
#include "garmin_txt.h"

#include <algorithm>               // for for_each, sort
#include <array>                   // for array, array<>::iterator
#include <cctype>                  // for toupper
#include <cmath>                   // for fabs, floor
#include <cstdint>                 // for uint16_t
#include <cstdio>                  // for snprintf, sscanf
#include <cstdlib>                 // for abs
#include <cstring>                 // for strstr, strlen
#include <ctime>                   // for gmtime, time_t, localtime, strftime, tm

#include <optional>                // for optional
#include <utility>                 // for pair, as_const, make_pair
#include <QByteArray>              // for QByteArray
#include <QChar>                   // for QChar, QChar::Other_Control
#include <QDateTime>               // for QDateTime
#include <QDebug>                  // for QDebug
#include <QIODevice>               // for QIODevice, QIODeviceBase::ReadOnly, QIODeviceBase::WriteOnly
#include <QList>                   // for QList, QList<>::const_iterator
#include <QString>                 // for QString, operator!=
#include <QStringList>             // for QStringList
#include <QStringLiteral>          // for qMakeStringPrivate, QStringLiteral
#include <QTextStream>             // for QTextStream
#include <QVector>                 // for QVector
#include <Qt>                      // for CaseInsensitive
#include <QtGlobal>                // for qRound, qPrintable

#include "defs.h"
#include "csv_util.h"              // for csv_linesplit
#include "formspec.h"              // for FormatSpecificDataList
#include "garmin_fs.h"             // for garmin_fs_t
#include "garmin_tables.h"         // for gt_display_modes_e, gt_find_desc_from_icon_number, gt_find_icon_number_from_desc, gt_get_mps_grid_longname, gt_lookup_datum_index, gt_lookup_grid_type, GDB, gt_get_icao_cc, gt_get_icao_country, gt_get_mps_datum_name, gt_waypt_class_names, GT_DISPLAY_MODE...
#include "jeeps/gpsmath.h"         // for GPS_Math_Known_Datum_To_UTM_EN, GPS_Math_WGS84_To_Known_Datum_M, GPS_Math_WGS84_To_Swiss_EN, GPS_Math_WGS84_To_UKOSMap_H
#include "src/core/datetime.h"     // for DateTime
#include "src/core/logging.h"      // for FatalMsg
#include "src/core/textstream.h"   // for TextStream


const QVector<QString> GarminTxtFormat::headers = {
  "Name\tDescription\tType\tPosition\tAltitude\tDepth\tProximity\tTemperature\t"
  "Display Mode\tColor\tSymbol\tFacility\tCity\tState\tCountry\t"
  "Date Modified\tLink\tCategories",
  "Waypoint Name\tDistance\tLeg Length\tCourse",
  "Position\tTime\tAltitude\tDepth\tTemperature\tLeg Length\tLeg Time\tLeg Speed\tLeg Course",
  "Name\tLength\tCourse\tWaypoints\tLink",
  "Name\tStart Time\tElapsed Time\tLength\tAverage Speed\tLink"
};


bool GarminTxtFormat::is_valid_alt(double alt)
{
  return (alt != unknown_alt) && (alt < kGarminUnknownAlt);
}

/* helpers */

void
GarminTxtFormat::init_date_and_time_format()
{
  // This is old, and weird, code.. date_time_format is a global that's
  // explicitly malloced and freed elsewhere. This isn't very C++ at all,
  // but this format is on its deathbead for deprecation.
  QString d1 = convert_human_date_format(opt_date_format);

  QString t1 = convert_human_time_format(opt_time_format);

  date_time_format = QStringLiteral("%1 %2").arg(d1, t1);
}

void
GarminTxtFormat::convert_datum(const Waypoint* wpt, double* dest_lat, double* dest_lon) const
{
  double alt;

  if (datum_index == kDatumWGS84) {
    *dest_lat = wpt->latitude;
    *dest_lon = wpt->longitude;
  } else GPS_Math_WGS84_To_Known_Datum_M(wpt->latitude, wpt->longitude, 0.0,
                                           dest_lat, dest_lon, &alt, datum_index);
}

/* WRITER *****************************************************************/

/* Waypoint preparation */

void
GarminTxtFormat::enum_waypt_cb(const Waypoint* wpt)
{
  const garmin_fs_t* gmsd = garmin_fs_t::find(wpt);
  int wpt_class = garmin_fs_t::get_wpt_class(gmsd, 0);
  if (wpt_class < 0x80) {
    if (gtxt_flags.enum_waypoints) {		/* enumerate only */
      waypoints++;
      return;
    }
    for (int i = 0; i < wpt_a_ct; i++) {		/* check for duplicates */
      const Waypoint* tmp = wpt_a[i];
      if (tmp->shortname.compare(wpt->shortname, Qt::CaseInsensitive) == 0) {
        wpt_a[i] = wpt;
        waypoints--;
        return;

      }
    }
    wpt_a[wpt_a_ct++] = wpt;
  }

}

/* common route and track pre-work */

void
GarminTxtFormat::prework_hdr_cb(const route_head* /*unused*/)
{
  cur_info = &route_info[route_idx];
}

void
GarminTxtFormat::prework_tlr_cb(const route_head* /*unused*/)
{
  cur_info->last_wpt = cur_info->prev_wpt;
  route_idx++;
}

void
GarminTxtFormat::prework_wpt_cb(const Waypoint* wpt)
{
  const Waypoint* prev = cur_info->prev_wpt;

  if (prev != nullptr) {
    cur_info->time += (wpt->GetCreationTime().toTime_t() - prev->GetCreationTime().toTime_t());
    cur_info->length += waypt_distance_ex(prev, wpt);
  } else {
    cur_info->first_wpt = wpt;
    cur_info->start = wpt->GetCreationTime().toTime_t();
  }
  cur_info->prev_wpt = wpt;
  cur_info->count++;
  routepoints++;
}


/* output helpers */

void
GarminTxtFormat::print_position(const Waypoint* wpt)
{
  int valid = 1;
  double lat;
  double lon;
  double north;
  double east;
  int zone;
  char map[3];
  char zonec;

  convert_datum(wpt, &lat, &lon);

  /* ----------------------------------------------------------------------------*/
  /*            the following code is from pretty_deg_format (util.c)            */
  /* ----------------------------------------------------------------------------*/
  /* !ToDo! generate common code for calculating of degrees, minutes and seconds */
  /* ----------------------------------------------------------------------------*/

  char latsig = lat < 0 ? 'S':'N';
  char lonsig = lon < 0 ? 'W':'E';
  int latint = abs((int) lat);
  int lonint = abs((int) lon);
  double latmin = 60.0 * (fabs(lat) - latint);
  double lonmin = 60.0 * (fabs(lon) - lonint);
  double latsec = 60.0 * (latmin - floor(latmin));
  double lonsec = 60.0 * (lonmin - floor(lonmin));

  switch (grid_index) {

  case grid_lat_lon_ddd:

    *fout << QString::asprintf("%c%0.*f %c%0.*f\t",
                               latsig, precision, fabs(lat),
                               lonsig, precision, fabs(lon));
    break;

  case grid_lat_lon_dmm:

    *fout << QString::asprintf("%c%d %0*.*f %c%d %0*.*f\t",
                               latsig, latint, precision + 3, precision, latmin,
                               lonsig, lonint, precision + 3, precision, lonmin);
    break;

  case grid_lat_lon_dms:

    *fout << QString::asprintf("%c%d %d %.*f %c%d %d %.*f\t",
                               latsig, latint, (int)latmin, precision, latsec,
                               lonsig, lonint, (int)lonmin, precision, lonsec);
    break;

  case grid_bng:

    valid = GPS_Math_WGS84_To_UKOSMap_H(wpt->latitude, wpt->longitude, &east, &north, map);
    if (valid) {
      *fout << QString::asprintf("%s %5.0f %5.0f\t", map, east, north);
    }
    break;

  case grid_utm:

    valid = GPS_Math_Known_Datum_To_UTM_EN(lat, lon,
                                           &east, &north, &zone, &zonec, datum_index);
    if (valid) {
      *fout << QString::asprintf("%02d %c %.0f %.0f\t", zone, zonec, east, north);
    }
    break;

  case grid_swiss:

    valid = GPS_Math_WGS84_To_Swiss_EN(wpt->latitude, wpt->longitude, &east, &north);
    if (valid) {
      *fout << QString::asprintf("%.f %.f\t", east, north);
    }
    break;

  default:
    gbFatal("ToDo\n");
  }

  if (! valid) {
    *fout << "#####\n";
    gbFatal("%s (%s) is outside of convertible area \"%s\"!\n",
          wpt->shortname.isEmpty() ? "Waypoint" : gbLogCStr(wpt->shortname),
          gbLogCStr(pretty_deg_format(wpt->latitude, wpt->longitude, 'd', nullptr, false)),
          gbLogCStr(gt_get_mps_grid_longname(grid_index)));
  }
}

void
GarminTxtFormat::print_date_and_time(const time_t time, const bool time_only)
{
  std::tm tm{};
  char tbuf[32];

  if (time < 0) {
    *fout << "\t";
    return;
  }
  if (time_only) {
    tm = *gmtime(&time);
    *fout << QString::asprintf("%d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
  } else if (time != 0) {
    if (gtxt_flags.utc) {
      time_t t = time + utc_offs;
      tm = *gmtime(&t);
    } else {
      tm = *localtime(&time);
    }
    strftime(tbuf, sizeof(tbuf), CSTR(date_time_format), &tm);
    *fout << QString::asprintf("%s ", tbuf);
  }
  *fout << "\t";
}

void
GarminTxtFormat::print_categories(uint16_t categories)
{
  const QStringList categoryList = garmin_fs_t::print_categories(categories);
  if (!categoryList.isEmpty()) {
    *fout << categoryList.join(',');
  }
}

void
GarminTxtFormat::print_course(const Waypoint* A, const Waypoint* B)		/* seems to be okay */
{
  if ((A != nullptr) && (B != nullptr) && (A != B)) {
    int course = qRound(waypt_course(A, B));
    *fout << QString::asprintf("%d° true", course);
  }
}

void
GarminTxtFormat::print_distance(const double distance, const bool no_scale, const bool with_tab, const int decis)
{
  double dist = distance;

  if (gtxt_flags.metric == 0) {
    dist = METERS_TO_FEET(dist);

    if ((dist < 5280) || no_scale) {
      *fout << QString::asprintf("%.*f ft", decis, dist);
    } else {
      dist = METERS_TO_MILES(distance);
      if (dist < 100.0) {
        *fout << QString::asprintf("%.1f mi", dist);
      } else {
        *fout << QString::asprintf("%d mi", qRound(dist));
      }
    }
  } else {
    if ((dist < 1000) || no_scale) {
      *fout << QString::asprintf("%.*f m", decis, dist);
    } else {
      dist = dist / 1000.0;
      if (dist < 100.0) {
        *fout << QString::asprintf("%.1f km", dist);
      } else {
        *fout << QString::asprintf("%d km", qRound(dist));
      }
    }
  }
  if (with_tab) {
    *fout << "\t";
  }
}

void
GarminTxtFormat::print_speed(const double distance, const time_t time)
{
  double dist = distance;
  const char* unit;

  if (!gtxt_flags.metric) {
    dist = METERS_TO_MILES(dist) * 1000.0;
    unit = "mph";
  } else {
    unit = "kph";
  }
  int idist = qRound(dist);

  if ((time != 0) && (idist > 0)) {
    double speed = MPS_TO_KPH(dist / (double)time);
    int ispeed = qRound(speed);

    if (speed < 0.01) {
      *fout << QString::asprintf("0 %s", unit);
    } else if (ispeed < 2) {
      *fout << QString::asprintf("%.1f %s", speed, unit);
    } else {
      *fout << QString::asprintf("%d %s", ispeed, unit);
    }
  } else {
    *fout << QString::asprintf("0 %s", unit);
  }
  *fout << "\t";
}

void
GarminTxtFormat::print_temperature(const float temperature)
{
  if (gtxt_flags.celsius) {
    *fout << QString::asprintf("%.f C", temperature);
  } else {
    *fout << QString::asprintf("%.f F", (temperature * 1.8) + 32);
  }
}

void
GarminTxtFormat::print_string(const char* fmt, const QString& string)
{
  /* remove unwanted characters from source string */
  QString cleanstring;
  for (const auto& chr : string) {
    if (chr.category() != QChar::Other_Control) {
      cleanstring.append(chr);
    } else {
      cleanstring.append(' ');
    }
  }
  *fout << QString::asprintf(fmt, CSTR(cleanstring));
}


/* main cb's */

void
GarminTxtFormat::write_waypt(const Waypoint* wpt)
{
  QString wpt_type;

  const garmin_fs_t* gmsd = garmin_fs_t::find(wpt);

  int i = garmin_fs_t::get_display(gmsd, 0);
  if (!((i >= 0) && (i < gt_display_mode_names.size()))) {
    i = 0;
  }
  QString dspl_mode = gt_display_mode_names[i];

  int wpt_class = garmin_fs_t::get_wpt_class(gmsd, 0);
  if ((wpt_class >= 0) && (wpt_class < gt_waypt_class_names.size())) {
    wpt_type = gt_waypt_class_names[wpt_class];
  } else {
    wpt_type = gt_waypt_class_names[0];
  }

  *fout << "Waypoint\t" << wpt->shortname << "\t";
  if (wpt_class <= gt_waypt_class_airport_ndb) {
    QString temp = wpt->notes;
    if (temp.isEmpty()) {
      if (wpt->description != wpt->shortname) {
        temp = wpt->description;
      } else {
        temp = "";
      }
    }
    print_string("%s\t", temp);
  } else {
    *fout << "\t";
  }
  *fout << wpt_type << "\t";

  print_position(wpt);

  if (is_valid_alt(wpt->altitude)) {
    print_distance(wpt->altitude, true, false, 0);
  }
  *fout << "\t";

  double x = wpt->depth_value_or(unknown_alt);
  if (x != unknown_alt) {
    print_distance(x, true, false, 1);
  }
  *fout << "\t";

  x = wpt->proximity_value_or(unknown_alt);
  if (x != unknown_alt) {
    print_distance(x, false, false, 0);
  }
  *fout << "\t";

  x = wpt->temperature_value_or(-999);
  if (x != -999) {
    print_temperature(x);
  }
  *fout << "\t" << dspl_mode << "\t";

  *fout << "Unknown\t"; 				/* Color is fixed: Unknown */

  int icon = garmin_fs_t::get_icon(gmsd, -1);
  if (icon == -1) {
    icon = gt_find_icon_number_from_desc(wpt->icon_descr, GDB);
  }
  print_string("%s\t", gt_find_desc_from_icon_number(icon, GDB));

  print_string("%s\t", garmin_fs_t::get_facility(gmsd, ""));
  print_string("%s\t", garmin_fs_t::get_city(gmsd, ""));
  print_string("%s\t", garmin_fs_t::get_state(gmsd, ""));
  const char* country = gt_get_icao_country(garmin_fs_t::get_cc(gmsd, ""));
  print_string("%s\t", (country != nullptr) ? country : "");
  print_date_and_time(wpt->GetCreationTime().toTime_t(), false);
  if (wpt->HasUrlLink()) {
    const UrlLink& l = wpt->GetUrlLink();
    print_string("%s\t", l.url_);
  } else {
    print_string("%s\t", "");
  }
  print_categories(garmin_fs_t::get_category(gmsd, 0));

  *fout << "\r\n";
}

void
GarminTxtFormat::route_disp_hdr_cb(const route_head* rte)
{
  cur_info = &route_info[route_idx];
  cur_info->prev_wpt = nullptr;
  cur_info->total = 0;
  if (rte->rte_waypt_empty()) {
    return;
  }

  if (!gtxt_flags.route_header_written) {
    gtxt_flags.route_header_written = 1;
    *fout << QStringLiteral("\r\n\r\nHeader\t%1\r\n").arg(headers[route_header]);
  }
  print_string("\r\nRoute\t%s\t", rte->rte_name);
  print_distance(cur_info->length, false, true, 0);
  print_course(cur_info->first_wpt, cur_info->last_wpt);
  *fout << QString::asprintf("\t%d waypoints\t", cur_info->count);
  if (rte->rte_urls.HasUrlLink()) {
    print_string("%s\r\n", rte->rte_urls.GetUrlLink().url_);
  } else {
    print_string("%s\r\n", "");
  }
  *fout << QStringLiteral("\r\nHeader\t%1\r\n\r\n").arg(headers[rtept_header]);
}

void
GarminTxtFormat::route_disp_tlr_cb(const route_head* /*unused*/)
{
  route_idx++;
}

void
GarminTxtFormat::route_disp_wpt_cb(const Waypoint* wpt)
{
  const Waypoint* prev = cur_info->prev_wpt;

  *fout << "Route Waypoint\t" << wpt->shortname << "\t";

  if (prev != nullptr) {
    double dist = waypt_distance_ex(prev, wpt);
    cur_info->total += dist;
    print_distance(cur_info->total, false, true, 0);
    print_distance(dist, false, true, 0);
    print_course(prev, wpt);
  } else {
    print_distance(0, true, false, 0);
  }

  *fout << "\r\n";

  cur_info->prev_wpt = wpt;
}

void
GarminTxtFormat::track_disp_hdr_cb(const route_head* track)
{
  cur_info = &route_info[route_idx];
  cur_info->prev_wpt = nullptr;
  cur_info->total = 0;
  if (track->rte_waypt_empty()) {
    return;
  }

  if (!gtxt_flags.track_header_written) {
    gtxt_flags.track_header_written = 1;
    *fout << QStringLiteral("\r\n\r\nHeader\t%1\r\n").arg(headers[track_header]);
  }
  print_string("\r\nTrack\t%s\t", track->rte_name);
  print_date_and_time(cur_info->start, false);
  print_date_and_time(cur_info->time, true);
  print_distance(cur_info->length, false, true, 0);
  print_speed(cur_info->length, cur_info->time);
  if (track->rte_urls.HasUrlLink()) {
    print_string("%s", track->rte_urls.GetUrlLink().url_);
  } else {
    print_string("%s", "");
  }
  *fout << QStringLiteral("\r\n\r\nHeader\t%1\r\n\r\n").arg(headers[trkpt_header]);
}

void
GarminTxtFormat::track_disp_tlr_cb(const route_head* /*unused*/)
{
  route_idx++;
}

void
GarminTxtFormat::track_disp_wpt_cb(const Waypoint* wpt)
{
  const Waypoint* prev = cur_info->prev_wpt;
  time_t delta;
  double dist;

  *fout << "Trackpoint\t";

  print_position(wpt);
  print_date_and_time(wpt->GetCreationTime().toTime_t(), false);
  if (is_valid_alt(wpt->altitude)) {
    print_distance(wpt->altitude, true, false, 0);
  }

  *fout << "\t";
  double depth = wpt->depth_value_or(unknown_alt);
  if (depth != unknown_alt) {
    print_distance(depth, true, false, 1);
  }

  if (prev != nullptr) {
    *fout << "\t";
    delta = wpt->GetCreationTime().toTime_t() - prev->GetCreationTime().toTime_t();
    float temp = wpt->temperature_value_or(-999);
    if (temp != -999) {
      print_temperature(temp);
    }
    *fout << "\t";
    dist = waypt_distance_ex(prev, wpt);
    print_distance(dist, false, true, 0);
    print_date_and_time(delta, true);
    print_speed(dist, delta);
    print_course(prev, wpt);
  }
  *fout << "\r\n";

  cur_info->prev_wpt = wpt;
}

/*******************************************************************************
* %%%        global callbacks called by gpsbabel main process              %%% *
*******************************************************************************/

void
GarminTxtFormat::garmin_txt_utc_option()
{
  if (opt_utc) {
    utc_offs = opt_utc.get_result() * 60 * 60;
    gtxt_flags.utc = 1;
  }
}

void
GarminTxtFormat::garmin_txt_adjust_time(QDateTime& dt) const
{
  if (gtxt_flags.utc) {
    dt = dt.toUTC().addSecs(dt.offsetFromUtc() - utc_offs);
  }
}

void
GarminTxtFormat::wr_init(const QString& fname)
{
  gtxt_flags = {};

  fout = new gpsbabel::TextStream;
  fout->open(fname, QIODevice::WriteOnly, "windows-1252");

  gtxt_flags.metric = opt_dist.get().startsWith("m", Qt::CaseInsensitive);
  gtxt_flags.celsius = opt_temp.get().startsWith("c", Qt::CaseInsensitive);
  init_date_and_time_format();
  if (opt_precision) {
    precision = opt_precision.get_result();
    if (precision < 0) {
      gbFatal("Invalid precision (%s)!\n", gbLogCStr(opt_precision));
    }
  }

  grid_index = grid_lat_lon_dmm;
  if (!opt_grid.isEmpty()) {
    bool ok;

    if (int i = opt_grid.toInt(&ok); ok) {
      grid_index = (grid_type) i;
      if ((grid_index < GRID_INDEX_MIN) || (grid_index > GRID_INDEX_MAX))
        gbFatal("Grid index out of range (%d..%d)!\n",
              (int)GRID_INDEX_MIN, (int)GRID_INDEX_MAX);
    } else {
      grid_index = gt_lookup_grid_type(opt_grid);
    }
  }

  switch (grid_index) {
  case grid_bng: /* force datum to "Ord Srvy Grt Britn" */
    datum_index = kDatumOSGB36;
    break;
  case grid_swiss: /* force datum to WGS84 */
    datum_index = kDatumWGS84;
    break;
  default:
    datum_index = gt_lookup_datum_index(opt_datum);
  }

  garmin_txt_utc_option();
}

void
GarminTxtFormat::wr_deinit()
{
  fout->close();
  delete fout;
  fout = nullptr;
  date_time_format.clear();
  date_time_format.squeeze();
}

void
GarminTxtFormat::write()
{
  auto enum_waypt_cb_lambda = [this](const Waypoint* waypointp)->void {
    enum_waypt_cb(waypointp);
  };
  auto prework_hdr_cb_lambda = [this](const route_head* rte)->void {
    prework_hdr_cb(rte);
  };
  auto prework_tlr_cb_lambda = [this](const route_head* rte)->void {
    prework_tlr_cb(rte);
  };
  auto prework_wpt_cb_lambda = [this](const Waypoint* waypointp)->void {
    prework_wpt_cb(waypointp);
  };
  auto route_disp_hdr_cb_lambda = [this](const route_head* rte)->void {
    route_disp_hdr_cb(rte);
  };
  auto route_disp_tlr_cb_lambda = [this](const route_head* rte)->void {
    route_disp_tlr_cb(rte);
  };
  auto route_disp_wpt_cb_lambda = [this](const Waypoint* waypointp)->void {
    route_disp_wpt_cb(waypointp);
  };
  auto track_disp_hdr_cb_lambda = [this](const route_head* rte)->void {
    track_disp_hdr_cb(rte);
  };
  auto track_disp_tlr_cb_lambda = [this](const route_head* rte)->void {
    track_disp_tlr_cb(rte);
  };
  auto track_disp_wpt_cb_lambda = [this](const Waypoint* waypointp)->void {
    track_disp_wpt_cb(waypointp);
  };

  QString grid_str = gt_get_mps_grid_longname(grid_index);
  grid_str = grid_str.replace('*', u'°');
  *fout << "Grid\t" << grid_str << "\r\n";

  QString datum_str = gt_get_mps_datum_name(datum_index);
  *fout << "Datum\t" << datum_str << "\r\n\r\n";

  waypoints = 0;
  gtxt_flags.enum_waypoints = 1;			/* enum all waypoints */
  waypt_disp_all(enum_waypt_cb_lambda);
  route_disp_all(nullptr, nullptr, enum_waypt_cb_lambda);
  gtxt_flags.enum_waypoints = 0;

  if (waypoints > 0) {
    wpt_a_ct = 0;
    wpt_a = new const Waypoint*[waypoints] {};
    waypt_disp_all(enum_waypt_cb_lambda);
    route_disp_all(nullptr, nullptr, enum_waypt_cb_lambda);
    auto sort_waypt_lambda = [](const Waypoint* wa, const Waypoint* wb)->bool {
      return wa->shortname.compare(wb->shortname, Qt::CaseInsensitive) < 0;
    };
    std::sort(wpt_a, wpt_a + waypoints, sort_waypt_lambda);

    *fout << QStringLiteral("Header\t%1\r\n\r\n").arg(headers[waypt_header]);
    for (int i = 0; i < waypoints; i++) {
      write_waypt(wpt_a[i]);
    }
    delete[] wpt_a;

    route_idx = 0;
    route_info = new PathInfo[route_count()];
    routepoints = 0;
    route_disp_all(prework_hdr_cb_lambda, prework_tlr_cb_lambda, prework_wpt_cb_lambda);

    if (routepoints > 0) {
      route_idx = 0;
      route_disp_all(route_disp_hdr_cb_lambda, route_disp_tlr_cb_lambda, route_disp_wpt_cb_lambda);
    }
    delete[] route_info;
    route_info = nullptr;
  }

  route_idx = 0;
  route_info = new PathInfo[track_count()];
  routepoints = 0;
  track_disp_all(prework_hdr_cb_lambda, prework_tlr_cb_lambda, prework_wpt_cb_lambda);

  if (routepoints > 0) {
    route_idx = 0;
    track_disp_all(track_disp_hdr_cb_lambda, track_disp_tlr_cb_lambda, track_disp_wpt_cb_lambda);
  }
  delete[] route_info;
}

/* READER *****************************************************************/

/* helpers */

void
GarminTxtFormat::free_headers()
{
  std::for_each(header_mapping_info.begin(), header_mapping_info.end(),
                [](auto& list)->void { list.clear(); });
}

// Super simple attempt to convert strftime/strptime spec to Qt spec.
// This misses a LOT of cases and vagaries, but the reality is that we
// see very few date formats here.
QString
GarminTxtFormat::strftime_to_timespec(const char* s)
{
  QString q;
  int l = strlen(s);
  q.reserve(l * 2); // no penalty if our guess is wrong.

  for (int i = 0; i < l; i++) {
    switch (s[i]) {
    case '%':
      if (i < l-1) {
        switch (s[++i]) {
        case 'd':
          q += "dd";
          continue;
        case 'm':
          q += "MM";
          continue;
        case 'y':
          q += "yy";
          continue;
        case 'Y':
          q += "yyyy";
          continue;
        case 'H':
          q += "HH";
          continue;
        case 'M':
          q += "mm";
          continue;
        case 'S':
          q += "ss";
          continue;
        case 'A':
          q += "dddd";
          continue;
        case 'a':
          q += "ddd";
          continue;
        case 'B':
          q += "MMMM";
          continue;
        case 'C':
          q += "yy";
          continue;
        case 'D':
          q += "MM/dd/yyyy";
          continue;
        case 'T':
          q += "hh:mm:ss";
          continue;
        case 'F':
          q += "yyyy-MM-dd";
          continue;
        case 'p':
          q += "AP";
          continue;
        default:
          gbWarning("omitting unknown strptime conversion \"%%%c\" in \"%s\"\n", s[i], s);
          break;
        }
      }
      break;
    default:
      q += s[i];
      break;
    }
  }
  return q;
}


/* data parsers */

QDateTime
GarminTxtFormat::parse_date_and_time(const QString& str)
{
  QString timespec = strftime_to_timespec(CSTR(date_time_format));
  return QDateTime::fromString(QString(str).trimmed(), timespec);
}

uint16_t
GarminTxtFormat::parse_categories(const QString& str) const
{
  uint16_t res = 0;

  const QStringList catstrings = str.split(',');
  for (const auto& catstring : catstrings) {
    QString cin = catstring.trimmed();
    if (!cin.isEmpty()) {
      if (std::optional<uint16_t> cat = garmin_fs_t::convert_category(cin); !cat.has_value()) {
        gbWarning("Unable to convert category \"%s\" at line %d!\n", gbLogCStr(cin), current_line);
      } else {
        res = res | *cat;
      }
    }
  }
  return res;
}

bool
GarminTxtFormat::parse_temperature(const QString& str, double* temperature) const
{
  double value;
  unsigned char unit;

  if (str.isEmpty()) {
    return false;
  }

  if (sscanf(CSTR(str), "%lf %c", &value, &unit) == 2) {
    unit = toupper(unit);
    switch (unit) {
    case 'C':
      *temperature = value;
      break;
    case 'F':
      *temperature = FAHRENHEIT_TO_CELSIUS(value);
      break;
    default:
      gbFatal("Unknown temperature unit \"%c\" at line %d!\n", unit, current_line);
    }
    return true;
  } else {
    gbFatal("Invalid temperature \"%s\" at line %d!\n", gbLogCStr(str), current_line);
  }
  return false;
}

void
GarminTxtFormat::parse_header(const QStringList& lineparts)
{
  header_column_names.clear();
  for (const auto& name : lineparts) {
    header_column_names.append(name.toUpper());
  }
}

bool
GarminTxtFormat::parse_display(const QString& str, int* val) const
{
  if (str.isEmpty()) {
    return false;
  }

  for (int i = 0; i < gt_display_mode_names.size(); ++i) {
    if (case_ignore_strcmp(str, gt_display_mode_names[i]) == 0) {
      *val = i;
      return true;
    }
  }
  gbWarning("Unknown display mode \"%s\" at line %d.\n", gbLogCStr(str), current_line);
  return false;
}

void
GarminTxtFormat::bind_fields(const header_type ht)
{
  if ((grid_index < 0) || (datum_index < 0)) {
    gbFatal("Incomplete or invalid file header!\n");
  }

  if (header_column_names.isEmpty()) {
    return;
  }
  header_mapping_info[ht].clear();

  /* make a copy of headers[ht], uppercase, split on "\t" */
  const QStringList altheader = headers.at(ht).toUpper().split('\t');

  int i = -1;
  for (const auto& name : std::as_const(header_column_names)) {
    i++;

    int field_idx = altheader.indexOf(name);
    if (field_idx >= 0) {
      int field_no = field_idx + 1;
      header_mapping_info[ht].append(std::make_pair(name, field_no));
      if (global_opts.debug_level >= 2) {
        gbDebug("Binding field \"%s\" to internal number %d (%d,%d)\n", gbLogCStr(name), field_no, ht, i);
      }
    } else {
      gbWarning("Field %s not recognized!\n", gbLogCStr(name));
    }
  }
  header_column_names.clear();
}

void
GarminTxtFormat::parse_grid(const QStringList& lineparts)
{
  if (lineparts.empty()) {
    gbFatal("Missing grid headline!\n");
  }

  const QString str = lineparts.at(0);
  if (str.contains("dd.ddddd")) {
    grid_index = grid_lat_lon_ddd;
  } else if (str.contains("mm.mmm")) {
    grid_index = grid_lat_lon_dmm;
  } else if (str.contains("mm'ss.s")) {
    grid_index = grid_lat_lon_dms;
  } else {
    grid_index = gt_lookup_grid_type(str);
  }
}

void
GarminTxtFormat::parse_datum(const QStringList& lineparts)
{
  if (lineparts.empty()) {
    gbFatal("Missing GPS datum headline!\n");
  }

  const auto& str = lineparts.at(0);
  datum_index = gt_lookup_datum_index(str);
}

void
GarminTxtFormat::parse_waypoint(const QStringList& lineparts)
{
  int column = -1;

  bind_fields(waypt_header);

  auto* wpt = new Waypoint;
  auto* gmsd = new garmin_fs_t(-1);
  wpt->fs.FsChainAdd(gmsd);

  for (const auto& str : lineparts) {
    if (++column >= header_mapping_info[waypt_header].size()) {
      gbWarning("too many fields in Waypoint record!\n");
      break;
    }
    int i;
    double d;
    const auto& [name, field_no] = header_mapping_info[waypt_header].at(column);

    switch (field_no) {
    case  1:
      if (!str.isEmpty()) {
        wpt->shortname = str;
      }
      break;
    case  2:
      if (!str.isEmpty()) {
        wpt->notes = str;
      }
      break;
    case  3:
      for (i = 0; i < gt_waypt_class_names.size(); i++) {
        if (case_ignore_strcmp(str, gt_waypt_class_names[i]) == 0) {
          garmin_fs_t::set_wpt_class(gmsd, i);
          break;
        }
      }
      break;
    case  4:
      parse_coordinates(str, datum_index, grid_index,
                        &wpt->latitude, &wpt->longitude);
      break;
    case  5:
      if (parse_distance(str, &d, 1)) {
        wpt->altitude = d;
      }
      break;
    case  6:
      if (parse_distance(str, &d, 1)) {
        wpt->set_depth(d);
      }
      break;
    case  7:
      if (parse_distance(str, &d, 1)) {
        wpt->set_proximity(d);
      }
      break;
    case  8:
      if (parse_temperature(str, &d)) {
        wpt->set_temperature(d);
      }
      break;
    case  9:
      if (parse_display(str, &i)) {
        garmin_fs_t::set_display(gmsd, i);
      }
      break;
    case 10:
      break;	/* skip color */
    case 11:
      i = gt_find_icon_number_from_desc(str, GDB);
      garmin_fs_t::set_icon(gmsd, i);
      wpt->icon_descr = gt_find_desc_from_icon_number(i, GDB);
      break;
    case 12:
      garmin_fs_t::set_facility(gmsd, str);
      break;
    case 13:
      garmin_fs_t::set_city(gmsd, str);
      break;
    case 14:
      garmin_fs_t::set_state(gmsd, str);
      break;
    case 15:
      garmin_fs_t::set_country(gmsd, str);
      garmin_fs_t::set_cc(gmsd, gt_get_icao_cc(str, wpt->shortname));
      break;
    case 16:
      if (QDateTime dt = parse_date_and_time(str); dt.isValid()) {
        garmin_txt_adjust_time(dt);
        wpt->SetCreationTime(dt);
      }
    break;
    case 17: {
      wpt->AddUrlLink(str);
    }
    break;
    case 18:
      garmin_fs_t::set_category(gmsd, parse_categories(str));
      break;
    default:
      break;
    }
  }
  waypt_add(wpt);
}

void
GarminTxtFormat::parse_route_header(const QStringList& lineparts)
{
  int column = -1;

  auto* rte = new route_head;

  bind_fields(route_header);
  for (const auto& str : lineparts) {
    if (++column >= header_mapping_info[route_header].size()) {
      gbWarning("too many fields in Route record!\n");
      break;
    }
    const auto& [name, field_no] = header_mapping_info[route_header].at(column);
    switch (field_no) {
    case 1:
      if (!str.isEmpty()) {
        rte->rte_name = str;
      }
      break;
    case 5:
      rte->rte_urls.AddUrlLink(UrlLink(str));
      break;
    }
  }
  route_add_head(rte);
  current_rte = rte;
}

void
GarminTxtFormat::parse_track_header(const QStringList& lineparts)
{
  int column = -1;

  bind_fields(track_header);
  auto* trk = new route_head;
  for (const auto& str : lineparts) {
    if (++column >= header_mapping_info[track_header].size()) {
      gbWarning("too many fields in Track record!\n");
      break;
    }
    const auto& [name, field_no] = header_mapping_info[track_header].at(column);
    switch (field_no) {
    case 1:
      if (!str.isEmpty()) {
        trk->rte_name = str;
      }
      break;
    case 6:
      trk->rte_urls.AddUrlLink(UrlLink(str));
      break;
    }
  }
  track_add_head(trk);
  current_trk = trk;
}


void
GarminTxtFormat::parse_route_waypoint(const QStringList& lineparts)
{
  int column = -1;
  Waypoint* wpt = nullptr;

  bind_fields(rtept_header);

  for (const auto& str : lineparts) {
    if (++column >= header_mapping_info[rtept_header].size()) {
      gbWarning("too many fields in Route Waypoint record!\n");
      break;
    }
    const auto& [name, field_no] = header_mapping_info[rtept_header].at(column);
    switch (field_no) {
    case 1:
      if (str.isEmpty()) {
        gbFatal("Route waypoint without name at line %d!\n", current_line);
      }
      wpt = find_waypt_by_name(str);
      if (wpt == nullptr) {
        gbFatal(FatalMsg() << "Route waypoint " << str << " not in waypoint list (line " << current_line<< ")!\n");
      }
      wpt = new Waypoint(*wpt);
      break;
    }
  }
  if (wpt != nullptr) {
    route_add_wpt(current_rte, wpt);
  }
}

void
GarminTxtFormat::parse_track_waypoint(const QStringList& lineparts)
{
  int column = -1;

  bind_fields(trkpt_header);
  auto* wpt = new Waypoint;

  for (const auto& str : lineparts) {
    if (++column >= header_mapping_info[trkpt_header].size()) {
      gbWarning("too many fields in Trackpoint record!\n");
      break;
    }
    double x;

    if (str.isEmpty()) {
      continue;
    }

    const auto& [name, field_no] = header_mapping_info[trkpt_header].at(column);

    switch (field_no) {
    case 1:
      parse_coordinates(str, datum_index, grid_index,
                        &wpt->latitude, &wpt->longitude);
      break;
    case 2:
      if (QDateTime dt = parse_date_and_time(str); dt.isValid()) {
        garmin_txt_adjust_time(dt);
        wpt->SetCreationTime(dt);
      }
    break;
    case 3:
      if (parse_distance(str, &x, 1)) {
        wpt->altitude = x;
      }
      break;
    case 4:
      if (parse_distance(str, &x, 1)) {
        wpt->set_depth(x);
      }
      break;
    case 5:
      if (parse_temperature(str, &x)) {
        wpt->set_temperature(x);
      }
      break;
    case 8:
      if (parse_speed(str, &x, 1)) {
        wpt->set_speed(x);
      }
      break;
    case 9:
      wpt->set_course(xstrtoi(CSTR(str), nullptr, 10));
      break;
    }
  }
  track_add_wpt(current_trk, wpt);
}

/***************************************************************/

void
GarminTxtFormat::rd_init(const QString& fname)
{
  gtxt_flags = {};

  fin = new gpsbabel::TextStream;
  fin->open(fname, QIODevice::ReadOnly, "windows-1252");

  free_headers();
  header_column_names.clear();

  datum_index = -1;
  grid_index = (grid_type) -1;

  init_date_and_time_format();
  garmin_txt_utc_option();
}

void
GarminTxtFormat::rd_deinit()
{
  free_headers();
  header_column_names.clear();
  fin->close();
  delete fin;
  fin = nullptr;
  date_time_format.clear();
  date_time_format.squeeze();
}

void
GarminTxtFormat::read()
{
  QString buff;

  current_line = 0;
  while ((buff = fin->readLine(), !buff.isNull())) {
    ++current_line;
    buff = buff.trimmed();

    if (buff.isEmpty()) {
      continue;
    }

    QStringList lineparts = csv_linesplit(buff, "\t", "", 0);

    if (lineparts.empty()) {
      continue;
    }
    auto linetype = lineparts.at(0);
    lineparts.removeFirst();

    if (linetype.compare(u"Header", Qt::CaseInsensitive) == 0) {
      parse_header(lineparts);
    } else if (linetype.compare(u"Grid", Qt::CaseInsensitive) == 0) {
      parse_grid(lineparts);
    } else if (linetype.compare(u"Datum", Qt::CaseInsensitive) == 0) {
      parse_datum(lineparts);
    } else if (linetype.compare(u"Waypoint", Qt::CaseInsensitive) == 0) {
      parse_waypoint(lineparts);
    } else if (linetype.compare(u"Route Waypoint", Qt::CaseInsensitive) == 0) {
      parse_route_waypoint(lineparts);
    } else if (linetype.compare(u"Trackpoint", Qt::CaseInsensitive) == 0) {
      parse_track_waypoint(lineparts);
    } else if (linetype.compare(u"Route", Qt::CaseInsensitive) == 0) {
      parse_route_header(lineparts);
    } else if (linetype.compare(u"Track", Qt::CaseInsensitive) == 0) {
      parse_track_header(lineparts);
    } else if (linetype.compare(u"Map", Qt::CaseInsensitive) == 0) /* do nothing */ ;
    else {
      gbFatal("Unknown identifier (%s) at line %d!\n", gbLogCStr(linetype), current_line);
    }

  }
}
