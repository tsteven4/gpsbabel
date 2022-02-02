/*

    Support for MapSource Text Export (Tab delimited) files.

    Copyright (C) 2006 Olaf Klein, o.b.klein@gpsbabel.org

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
#ifndef GARMIN_TXT_H_INCLUDED_
#define GARMIN_TXT_H_INCLUDED_

#if CSVFMTS_ENABLED

#include <QString>                // for QString
#include <QStringList>            // for QStringList
#include <QVector>                // for QVector

#include <cstdint>                // for uint16_t
#include <ctime>                  // for time_t

#include "defs.h"                 // for arglist_t, Waypoint, route_head, ARG_NOMINMAX, ARGTYPE_STRING, ARGTYPE_INT, CET_CHARSET_UTF8, FF_CAP_RW_ALL, ff_cap, ff_type, ff_type_file, grid_type
#include "format.h"               // for Format
#include "src/core/textstream.h"  // for TextStream


class GarminTxtFormat : public Format
{
public:
  QVector<arglist_t>* get_args() override
  {
    return &garmin_txt_args;
  }

  ff_type get_type() const override
  {
    return ff_type_file;
  }

  QVector<ff_cap> get_cap() const override
  {
    return FF_CAP_RW_ALL;
  }

  QString get_encode() const override
  {
    /*
     * The file encoding is Windows-1252, a.k.a CET_CHARSET_MS_ANSI.
     * Conversion between Windows-1252 and utf-16 is handled by the stream.
     * Conversion between utf-16 and utf-8 is handled by this format.
     * Let main know char strings have already been converted to utf-8
     * so it doesn't attempt to re-convert any char strings including gmsd data.
     */
    return CET_CHARSET_UTF8;
  }

  int get_fixed_encode() const override
  {
    return 0;
  }

  void rd_init(const QString& fname) override;
  void read() override;
  void rd_deinit() override;
  void wr_init(const QString& fname) override;
  void write() override;
  void wr_deinit() override;

  enum header_type {
    waypt_header = 0,
    rtept_header,
    trkpt_header,
    route_header,
    track_header,
    unknown_header
  };

private:
  /* Types */
  struct gtxt_flags_t {
    unsigned int metric:1;
    unsigned int celsius:1;
    unsigned int utc:1;
    unsigned int enum_waypoints:1;
    unsigned int route_header_written:1;
    unsigned int track_header_written:1;
  };

  struct info_t {
    double length;
    time_t start;
    time_t time;
    double speed;
    double total;
    int count;
    const Waypoint* prev_wpt;
    const Waypoint* first_wpt;
    const Waypoint* last_wpt;
  };

  /* Constants */

  static constexpr int kMaxHeaderFields = 36;

  static constexpr const char* headers[] = {
    "Name\tDescription\tType\tPosition\tAltitude\tDepth\tProximity\tTemperature\t"
    "Display Mode\tColor\tSymbol\tFacility\tCity\tState\tCountry\t"
    "Date Modified\tLink\tCategories",
    "Waypoint Name\tDistance\tLeg Length\tCourse",
    "Position\tTime\tAltitude\tDepth\tTemperature\tLeg Length\tLeg Time\tLeg Speed\tLeg Course",
    "Name\tLength\tCourse\tWaypoints\tLink",
    "Name\tStart Time\tElapsed Time\tLength\tAverage Speed\tLink",
    nullptr
  };

  /* Member Functions */

  static const char* get_option_val(const char* option, const char* def);
  void init_date_and_time_format();
  void convert_datum(const Waypoint* wpt, double* dest_lat, double* dest_lon);
  void enum_waypt_cb(const Waypoint* wpt);
  static int sort_waypt_cb(const void* a, const void* b);
  void prework_hdr_cb(const route_head* /*unused*/);
  void prework_tlr_cb(const route_head* /*unused*/);
  void prework_wpt_cb(const Waypoint* wpt);
  void print_position(const Waypoint* wpt);
  void print_date_and_time(time_t time, int time_only);
  void print_categories(uint16_t categories);
  void print_course(const Waypoint* A, const Waypoint* B);
  void print_distance(double distance, int no_scale, int with_tab, int decis);
  void print_speed(const double* distance, const time_t* time);
  void print_temperature(float temperature);
  void print_string(const char* fmt, const QString& string);
  void write_waypt(const Waypoint* wpt);
  void route_disp_hdr_cb(const route_head* rte);
  void route_disp_tlr_cb(const route_head* /*unused*/);
  void route_disp_wpt_cb(const Waypoint* wpt);
  void track_disp_hdr_cb(const route_head* track);
  void track_disp_tlr_cb(const route_head* /*unused*/);
  void track_disp_wpt_cb(const Waypoint* wpt);
  void free_header(header_type ht);
  bool parse_date_and_time(const QString& str, time_t* value);
  uint16_t parse_categories(const QString& str);
  bool parse_temperature(const QString& str, double* temperature);
  void parse_header(const QStringList& lineparts);
  bool parse_display(const QString& str, int* val);
  void bind_fields(header_type ht);
  void parse_grid(const QStringList& lineparts);
  void parse_datum(const QStringList& lineparts);
  void parse_waypoint(const QStringList& lineparts);
  void parse_route_header(const QStringList& lineparts);
  void parse_track_header(const QStringList& lineparts);
  void parse_route_waypoint(const QStringList& lineparts);
  void parse_track_waypoint(const QStringList& lineparts);

  /* Data Members */

  gpsbabel::TextStream* fin = nullptr;
  gpsbabel::TextStream* fout = nullptr;
  route_head* current_trk{}, *current_rte{};
  int waypoints{};
  int routepoints{};
  const Waypoint** wpt_a{};
  int wpt_a_ct{};
  grid_type grid_index;
  int datum_index{};
  const char* datum_str{};
  int current_line{};
  char* date_time_format = nullptr;
  int precision = 3;
  time_t utc_offs = 0;

  gtxt_flags_t gtxt_flags{};

  char* header_lines[unknown_header + 1][kMaxHeaderFields] {};
  int header_fields[unknown_header + 1][kMaxHeaderFields] {};
  int header_ct[unknown_header + 1] {};

  char* opt_datum = nullptr;
  char* opt_dist = nullptr;
  char* opt_temp = nullptr;
  char* opt_date_format = nullptr;
  char* opt_time_format = nullptr;
  char* opt_precision = nullptr;
  char* opt_utc = nullptr;
  char* opt_grid = nullptr;

  QVector<arglist_t> garmin_txt_args = {
    {"date",  &opt_date_format, "Read/Write date format (i.e. yyyy/mm/dd)", nullptr, ARGTYPE_STRING, ARG_NOMINMAX, nullptr},
    {"datum", &opt_datum, 	    "GPS datum (def. WGS 84)", "WGS 84", ARGTYPE_STRING, ARG_NOMINMAX, nullptr},
    {"dist",  &opt_dist,        "Distance unit [m=metric, s=statute]", "m", ARGTYPE_STRING, ARG_NOMINMAX, nullptr},
    {"grid",  &opt_grid,        "Write position using this grid.", nullptr, ARGTYPE_STRING, ARG_NOMINMAX, nullptr},
    {"prec",  &opt_precision,   "Precision of coordinates", "3", ARGTYPE_INT, ARG_NOMINMAX, nullptr},
    {"temp",  &opt_temp,        "Temperature unit [c=Celsius, f=Fahrenheit]", "c", ARGTYPE_STRING, ARG_NOMINMAX, nullptr},
    {"time",  &opt_time_format, "Read/Write time format (i.e. HH:mm:ss xx)", nullptr, ARGTYPE_STRING, ARG_NOMINMAX, nullptr},
    {"utc",   &opt_utc,         "Write timestamps with offset x to UTC time", nullptr, ARGTYPE_INT, "-23", "+23", nullptr}
  };

  info_t* route_info{};
  int route_idx{};
  info_t* cur_info{};
};

#if 0
inline GarminTxtFormat::header_type& operator++(GarminTxtFormat::header_type& s) // prefix
{
  return s = static_cast<GarminTxtFormat::header_type>(s + 1);
}
inline GarminTxtFormat::header_type operator++(GarminTxtFormat::header_type& s, int) // postfix
{
  GarminTxtFormat::header_type ret(s);
  ++s;
  return ret;
}

inline gt_display_modes_e& operator++(gt_display_modes_e& s) // prefix
{
  return s = static_cast<gt_display_modes_e>(s + 1);
}
inline gt_display_modes_e operator++(gt_display_modes_e& s, int) // postfix
{
  gt_display_modes_e ret(s);
  ++s;
  return ret;
}
#endif

#endif // CSVFMTS_ENABLED
#endif // GARMIN_TXT_H_INCLUDED_
