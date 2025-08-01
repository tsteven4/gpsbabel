/*

    Support for CompeGPS waypoint (.wpt), route (.rte) and track (.trk) files,

    Copyright (C) 2005 Olaf Klein

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

/*
    History:
    		10/23/2005: First release; only a reader
    		10/25/2005: becomes a writer too
		10/26/2005: received documentation from CompeGPS team
		            added fatals for "G" and "U" if not WGS84 and lat/lon
		08/13/2006: switch to gbfile api
*/

/*

    the meaning of leading characters in CompeGPS data lines (enhanced PCX):

    header lines:

	"G": WGS 84			- Datum of the map
	"N": Anybody			- Name of the user
	"L": -02:00:00			- Difference to UTC
	"M": ...			- Any comments
	"R": 16711680 , xxxx , 1 	- Route header
	"U": 1				- System of coordinates (0=UTM 1=Latitude/Longitude)

	"C":  0 0 255 2 -1.000000	- ???
	"V":  0.0 0.0 0 0 0 0 0.0	- ???
	"E": 0|1|00-NUL-00 00:00:00|00:00:00|0 - ???

    data lines:

	"W": if(route) routepoint; else waypoint
	"T": trackpoint
    	"t": if(track) additionally track info
	     if(!track) additionally trackpoint info
	"a": link to ...
	"w": waypoint additional info

*/

#include "defs.h"
#include "cet_util.h"
#include "csv_util.h"

#if CSVFMTS_ENABLED
#include <cmath>
#include "jeeps/gpsmath.h"
#include <cstdlib>
#include <cstdio>

#define MYNAME "CompeGPS"

#define SHORT_NAME_LENGTH 16

static gbfile* fin, *fout;
static int target_index, curr_index;
static int track_info_flag;
static short_handle sh;
static int snlen;
static int radius;
static int input_datum;

static const route_head* curr_track;

/* placeholders for options */

static char* option_icon;
static char* option_index;
static char* option_radius;
static char* option_snlen;

static
QVector<arglist_t> compegps_args = {
  {
    "deficon", &option_icon, "Default icon name",
    nullptr, ARGTYPE_STRING, ARG_NOMINMAX, nullptr
  },
  {
    "index", &option_index, "Index of route/track to write (if more than one in source)",
    nullptr, ARGTYPE_INT, "1", nullptr, nullptr
  },
  {
    "radius", &option_radius, "Give points (waypoints/route points) a default radius (proximity)",
    nullptr, ARGTYPE_FLOAT, "0", nullptr, nullptr
  },
  {
    "snlen", &option_snlen, "Length of generated shortnames (default 16)",
    "16", ARGTYPE_INT, "1", nullptr, nullptr
  },
};

static
void fix_datum(double* lat, double* lon)
{
  double amt;

  /*
   * Avoid FP jitter in the common case.
   */
  if (input_datum != DATUM_WGS84) {
    GPS_Math_Known_Datum_To_WGS84_M(*lat, *lon, 0.0, lat, lon,
                                    &amt, input_datum);
  }
}

static void
compegps_parse_date(const char* c, QDate& date)
{
  date = QDate::fromString(c, "dd-MMM-yy");
  // If that worked, fix 1900 bias to 2000 bias. Some have full 4 digits.
  if (date.isValid()) {
    date = date.addYears(100);
  } else {
    date = QDate::fromString(c, "dd-MMM-yyyy");
  }
}

static void
compegps_parse_time(const char* c, QTime& time)
{
  time = QTime::fromString(c, "hh:mm:ss");
}

/* specialized readers */

static Waypoint*
parse_wpt(char* buff)
{
  int col = -1;
  char* cx;
  auto* wpt = new Waypoint;
  int has_time = 0;
  QDate date;
  QTime time;

  char* c = strstr(buff, "A ");
  if (c == buff) {
    col++;
  }

  c = csv_lineparse(buff, " ", "", col++);
  while (c != nullptr) {
    c = lrtrim(c);
    if (*c != '\0') {
#if 0
      printf(MYNAME "_read_wpt: col(%d)=%s\n", col, c);
#endif
      switch (col) {
      case 0:

        cx = c + strlen(c) - 1;		/* trim trailing underscores */
        while ((cx >= c) && (*cx == '_')) {
          *cx-- = '\0';
        }
        if (*c != '\0') {
          wpt->shortname = c;
        }
        break;
      case 2:
        human_to_dec(c, &wpt->latitude, nullptr, 1);
        break;
      case 3:
        human_to_dec(c, nullptr, &wpt->longitude, 2);
        break;
        // Older compegps used a dumb constant.
        // Report are that 2010-era writes a sensible
        // value here.
        /* always "27-MAR-62 00:00:00" */
      case 4:
        if (strcmp(c, "27-MAR-62")) {
          has_time = 1;
          compegps_parse_date(c, date);
        }
        break;
      case 5:
        if (has_time) {
          compegps_parse_time(c, time);
          wpt->SetCreationTime(QDateTime(date, time, Qt::UTC));
        }
        break;
      case 6:
        wpt->altitude = atof(c);
        break;
      case 7:
        wpt->description = c;
        break;
      default:
        if (col > 7) {
          wpt->description += " ";
          wpt->description += c;
        }
      }
    }
    c = csv_lineparse(nullptr, " ", "", col++);
  }
  fix_datum(&wpt->latitude, &wpt->longitude);
  return wpt;
}

static void
parse_wpt_info(const char* buff, Waypoint* wpt)		/* "w" */
{
  int col = -1;
  double fx;

  char* c = csv_lineparse(buff, ",", "", col++);
  while (c != nullptr) {
    c = lrtrim(c);
    if (*c != '\0') {
#if 0
      printf(MYNAME "_read_wpt_info: col(%d)=%s\n", col, c);
#endif
      switch (col) {
      case 0:
        wpt->icon_descr = c;
        break;
      case 1:
        break;			/* Text postion */
      case 2:
        break;			/* Lens zoom level */
      case 3:
        break;			/* Text colour */
      case 4:
        break;			/* Background colour */
      case 5:
        break;			/* Transparent text  (0=transparent, 1=no transparent) */
      case 6:
        break;			/* ??? */
      case 7:
        break;			/* ??? */
      case 8: 			/* radius */
        fx = atof(c);
        if (fx > 0) {
          WAYPT_SET(wpt, proximity, fx);
        }
        break;
      }
    }
    c = csv_lineparse(nullptr, ",", "", col++);
  }
}

static Waypoint*
parse_trkpt(char* buff)
{
  int col = -1;
  auto* wpt = new Waypoint;
  QDate date;
  QTime time;

  char* c = strstr(buff, "A ");
  if (c == buff) {
    col++;
  }

  c = csv_lineparse(buff, " ", "", col++);
  while (c != nullptr) {
    c = lrtrim(c);
    if (*c != '\0') {
#if 0
      printf(MYNAME "_read_trkpt: col(%d)=%s\n", col, c);
#endif
      switch (col) {
      case 2:
        human_to_dec(c, &wpt->latitude, nullptr, 1);
        break;
      case 3:
        human_to_dec(c, nullptr, &wpt->longitude, 2);
        break;
      case 4:
        compegps_parse_date(c, date);
        break;
      case 5:
        compegps_parse_time(c, time);
        wpt->SetCreationTime(QDateTime(date, time, Qt::UTC));
        break;
      case 7:
        wpt->altitude = atof(c);
        break;
      }
    }
    c = csv_lineparse(nullptr, " ", "", col++);
  }
  fix_datum(&wpt->latitude, &wpt->longitude);
  return wpt;
}

static void
parse_track_info(const char* buff, route_head* track)	/* "t" */
{
  int col = -1;

  char* c = csv_lineparse(buff, "|", "", col++);
  while (c != nullptr) {
    c = lrtrim(c);
    if (*c != '\0') {
#if 0
      printf(MYNAME "_read_track_info: col(%d)=%s\n", col, c);
#endif
      switch (col) {
      case 0:
        break;	/* unknown field */
      case 1:
        track->rte_name = c;
        break;
      case 2:
        break;	/* unknown field */
      case 3:
        break;	/* unknown field */
      }
    }
    c = csv_lineparse(nullptr, "|", "", col++);
  }
}

static void
parse_rte_info(const char* buff, route_head* route)	/* "R" */
{
  int col = -1;

  char* c = csv_lineparse(buff, ",", "", col++);
  while (c != nullptr) {
    c = lrtrim(c);
    if (*c != '\0') {
#if 0
      printf(MYNAME "_read_rte_info: col(%d)=%s\n", col, c);
#endif
      switch (col) {
      case 0:
        break;				/* unknown field (colour?) */
      case 1:
        route->rte_name = c;
        break;
      case 2:
        break;				/* unknown field */

      }
    }
    c = csv_lineparse(nullptr, ",", "", col++);
  }
}

/* main functions */

static void
compegps_rd_init(const QString& fname)
{
  fin = gbfopen(fname, "rb", MYNAME);
  input_datum = DATUM_WGS84;
}

static void
compegps_rd_deinit()
{
  gbfclose(fin);
}

static void
compegps_data_read()
{
  char* buff;
  int line = 0;
  Waypoint* wpt = nullptr;
  route_head* route = nullptr;
  route_head* track = nullptr;

  while ((buff = gbfgetstr(fin))) {
    if ((line++ == 0) && fin->unicode) {
      cet_convert_init(CET_CHARSET_UTF8, 1);
    }
    char* cin = lrtrim(buff);
    if (strlen(cin) == 0) {
      continue;
    }

    char* ctail = strchr(cin, ' ');
    if (ctail == nullptr) {
      continue;
    }
    ctail = lrtrim(ctail);

    switch (*cin) {
    case 'G':
      input_datum = GPS_Lookup_Datum_Index(ctail);
      if (input_datum < 0) {
        fatal(MYNAME ": Unsupported datum \"%s\"!", ctail);
      }
      break;
    case 'U':
      switch (*ctail) {
      case '1': /* lat/lon, that's we want to see */
        break;
      case '0': /* UTM not supported yet */
        fatal(MYNAME "Sorry, UTM is not supported yet!\n");
      default:
        fatal(MYNAME "Invalid system of coordinates (%s)!\n", cin);
      }
      break;
    case 'R':
      route = new route_head;
      route_add_head(route);
      parse_rte_info(ctail, route);
      break;
    case 'M':
      break;
    case 'W':
      wpt = parse_wpt(ctail);
      if (wpt != nullptr) {
        if (route != nullptr) {
          route_add_wpt(route, wpt);
        } else {
          waypt_add(wpt);
        }
      }
      break;
    case 'w':
      is_fatal((wpt == nullptr), MYNAME ": No waypoint data before \"%s\"!", cin);
      parse_wpt_info(ctail, wpt);
      break;
    case 'T':
      wpt = parse_trkpt(ctail);
      if (wpt != nullptr) {
        if (track == nullptr) {
          track = new route_head;
          track_add_head(track);
        }
        track_add_wpt(track, wpt);
      }
      break;
    case 't':
      if (track != nullptr) {
        parse_track_info(ctail, track);
      }
      break;
    }
  }
}

/* ----------------------------------------------------------- */

static void
write_waypt_cb(const Waypoint* wpt)
{
  if (curr_index != target_index) {
    return;
  }

  // Our only output cleansing is to replace
  QString cleaned_name(wpt->shortname);
  cleaned_name.replace(' ', '_');

  QString name = (snlen > 0) ? mkshort(sh, cleaned_name) : cleaned_name;

  gbfprintf(fout, "W  %s A ", CSTR(name));
  gbfprintf(fout, "%.10f%c%c ",
            fabs(wpt->latitude), 0xBA, (wpt->latitude >= 0) ? 'N' : 'S');
  gbfprintf(fout, "%.10f%c%c ",
            fabs(wpt->longitude), 0xBA, (wpt->longitude >= 0) ? 'E' : 'W');
  gbfprintf(fout, "27-MAR-62 00:00:00 %.6f",
            (wpt->altitude != unknown_alt) ? wpt->altitude : 0.0);
  if (wpt->description != nullptr) {
    gbfprintf(fout, " %s", CSTRc(wpt->description));
  }
  gbfprintf(fout, "\n");

  if ((!wpt->icon_descr.isNull()) || (wpt->wpt_flags.proximity) || \
      (option_icon != nullptr)) {
    gbfprintf(fout, "w  %s,0,0.0,16777215,255,1,7,,%.1f\n",
              wpt->icon_descr.isNull() ? "Waypoint" : CSTR(wpt->icon_descr),
              WAYPT_GET(wpt, proximity, 0));
  }
}

static void
write_route_hdr_cb(const route_head* rte)
{
  curr_index++;
  if (curr_index != target_index) {
    return;
  }

  QString name = rte->rte_name;
  if (name != nullptr) {
    name = csv_stringclean(name, ",");
  } else {
    name = " ";
  }
  gbfprintf(fout, "R  16711680,%s,1,-1\n", CSTR(name));
}

static void
write_route()
{
  curr_index = 0;
  route_disp_all(write_route_hdr_cb, nullptr, write_waypt_cb);
}

static void
write_track_hdr_cb(const route_head* trk)
{
  track_info_flag = 0;
  curr_track = trk;

  curr_index++;
  if (curr_index != target_index) {
    return;
  }

  track_info_flag = 1;
}

static void
write_trkpt_cb(const Waypoint* wpt)
{
  if ((curr_index != target_index) || (wpt == nullptr)) {
    return;
  }

  QDateTime default_dt(QDate(1970,1,1), QTime(0,0,0), Qt::UTC);
  auto dt = wpt->creation_time.isValid() ? wpt->GetCreationTime() : default_dt;
  QString buff = dt.toString("dd-MMM-yy hh:mm:ss").toUpper();

  gbfprintf(fout, "T  A %.10f%c%c %.10f%c%c ",
            fabs(wpt->latitude), 0xBA, (wpt->latitude >= 0) ? 'N' : 'S',
            fabs(wpt->longitude), 0xBA, (wpt->longitude >= 0) ? 'E' : 'W');
  gbfprintf(fout, "%s s %.1f %.1f %.1f %.1f %d ",
            CSTR(buff),
            wpt->altitude,
            0.0,
            0.0,
            0.0,
            0);
  gbfprintf(fout, "%.1f %.1f %.1f %.1f %.1f\n",
            -1000.0,
            -1.0,
            -1.0,
            -1.0,
            -1.0);
  if (track_info_flag != 0) {
    track_info_flag = 0;
    if (curr_track->rte_name != nullptr) {
      QString name = csv_stringclean(curr_track->rte_name, "|");
      gbfprintf(fout, "t 4294967295|%s|-1|-1\n", CSTR(name));
    }
  }
}

static void
write_track()
{
  curr_index = 0;

//	gbfprintf(fout, "L  +02:00:00\n");
  track_disp_all(write_track_hdr_cb, nullptr, write_trkpt_cb);
  gbfprintf(fout, "F  1234\n");
}

static void
write_waypoints()
{
  waypt_disp_all(write_waypt_cb);
}

/* --------------------------------------------------------------------------- */

static void
compegps_wr_init(const QString& fname)
{
  fout = gbfopen(fname, "w", MYNAME);
  sh = mkshort_new_handle();
}

static void
compegps_wr_deinit()
{
  mkshort_del_handle(&sh);
  gbfclose(fout);
}

static void
compegps_data_write()
{
  /* because of different file extensions we can only write one GPS data type at time */

  gbfprintf(fout, "G  WGS 84\n");
  gbfprintf(fout, "U  1\n");

  /* process options */

  target_index = 1;
  if (option_index != nullptr) {
    target_index = atoi(option_index);
  }

  snlen = 0;
  if (global_opts.synthesize_shortnames != 0) {
    if (option_snlen != nullptr) {
      snlen = atoi(option_snlen);
    } else {
      snlen = SHORT_NAME_LENGTH;
    }

    is_fatal((snlen < 1), MYNAME "Invalid length for generated shortnames!");

    setshort_whitespace_ok(sh, 0);
    setshort_length(sh, snlen);
  }

  radius = -1;
  if (option_radius != nullptr) {
    radius = atof(option_radius);
    is_fatal((radius <= 0.0), MYNAME "Invalid value for radius!");
  }

  if (option_icon != nullptr) {
    if (*option_icon == '\0') {
      option_icon = nullptr;
    } else if (case_ignore_strcmp(option_icon, "deficon") == 0) {
      option_icon = nullptr;
    }
  }

  switch (global_opts.objective) {
  case wptdata:
  case unknown_gpsdata:
    curr_index = target_index = 0;
    write_waypoints();
    break;
  case trkdata:
    write_track();
    break;
  case rtedata:
    write_route();
    break;
  case posndata:
    fatal(MYNAME ": Realtime positioning not supported.\n");
    break;
  }
}

/* --------------------------------------------------------------------------- */

ff_vecs_t compegps_vecs = {
  ff_type_file,
  FF_CAP_RW_ALL,
  compegps_rd_init,
  compegps_wr_init,
  compegps_rd_deinit,
  compegps_wr_deinit,
  compegps_data_read,
  compegps_data_write,
  nullptr,
  &compegps_args,
  CET_CHARSET_MS_ANSI, 1
  , NULL_POS_OPS,
  nullptr
};
#endif /* CSVFMTS_ENABLED */
