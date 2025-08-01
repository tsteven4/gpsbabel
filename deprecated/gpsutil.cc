/*
    Access gpsutil files.

    Copyright (C) 2002, 2003, 2004 Robert Lipe

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


#include "defs.h"
#include "cet_util.h"
#include "magellan.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>

static gbfile* file_in, *file_out;
static short_handle mkshort_handle;

#define MYNAME "GPSUTIL"

static void
rd_init(const QString& fname)
{
  file_in = gbfopen(fname, "rb", MYNAME);
}

static void
rd_deinit()
{
  gbfclose(file_in);
}

static void
wr_init(const QString& fname)
{
  file_out = gbfopen(fname, "w", MYNAME);
  mkshort_handle = mkshort_new_handle();
}

static void
wr_deinit()
{
  gbfclose(file_out);
  mkshort_del_handle(&mkshort_handle);
}

static void
data_read()
{
  char* ibuf;
  char desc[31];
  double lat,lon;
  char latdir, londir;
  long alt;
  char alttype;
  char icon[3];
  int line = 0;
  /*
   * Make sure that all waypoints in single read have same
   * timestamp.
   */
  time_t now = current_time().toTime_t();
  icon[0] = 0;

  while ((ibuf = gbfgetstr(file_in))) {
    char* sn;

    if ((line++ == 0) && file_in->unicode) {
      cet_convert_init(CET_CHARSET_UTF8, 1);
    }

    /*  A sharp in column zero or an blank line is a comment */
    ibuf = lrtrim(ibuf);
    int len = strlen(ibuf);
    if ((len == 0) || (*ibuf == '#')) {
      continue;
    }

    if (len > 71) {
      int offs = len - 71;
      sn = xstrndup(ibuf, offs + 8);
      ibuf += (offs + 9);
    } else {
      sn = xstrndup(ibuf, 8);
      ibuf += 9;
    }

    int n = sscanf(ibuf, "%lf%c %lf%c %ld%c %30[^,] %2s",
                   &lat, &latdir, &lon, &londir,
                   &alt, &alttype, desc, icon);
    /* Require at least first three fields, otherwise ignore */
    if (n < 2) {
      xfree(sn);
      continue;
    }
    rtrim(sn);
    rtrim(desc);
    rtrim(icon);
    auto* wpt_tmp = new Waypoint;
    wpt_tmp->altitude = alt;
    wpt_tmp->shortname = sn;
    xfree(sn);
    wpt_tmp->description = desc;
    wpt_tmp->SetCreationTime(now);

    if (latdir == 'S') {
      lat = -lat;
    }
    if (londir == 'W') {
      lon = -lon;
    }

    lat /= 100.0;
    lon /= 100.0;
    int ilon = (int)(lon);
    wpt_tmp->longitude = ilon + (lon - ilon)*(100.0/60.0);
    int ilat = (int)(lat);
    wpt_tmp->latitude = ilat + (lat - ilat) * (100.0/60.0);
    wpt_tmp->icon_descr = mag_find_descr_from_token(icon);
    waypt_add(wpt_tmp);
  }
}

static void
gpsutil_disp(const Waypoint* wpt)
{
  char* tdesc = xstrdup(wpt->description);

  QString icon_token = mag_find_token_from_descr(wpt->icon_descr);

  double lon = degrees2ddmm(wpt->longitude);
  double lat = degrees2ddmm(wpt->latitude);

  gbfprintf(file_out, "%-8.8s %08.3f%c %09.3f%c %07.0f%c %-30.30s %s\n",
            global_opts.synthesize_shortnames ?
            CSTRc(mkshort_from_wpt(mkshort_handle, wpt)) :
            CSTRc(wpt->shortname),
            fabs(lat),
            lat < 0.0 ? 'S' : 'N',
            fabs(lon),
            lon < 0.0 ? 'W' : 'E',
            ((wpt->altitude == unknown_alt) ||
             (wpt->altitude < 0.0)) ? 0 : wpt->altitude,
            'm',
            CSTRc(wpt->description) ? tdesc : "",
            CSTR(icon_token));

  xfree(tdesc);
}

static void
data_write()
{
  waypt_disp_all(gpsutil_disp);
}


ff_vecs_t gpsutil_vecs = {
  ff_type_file,
  FF_CAP_RW_WPT,
  rd_init,
  wr_init,
  rd_deinit,
  wr_deinit,
  data_read,
  data_write,
  nullptr,
  nullptr,
  CET_CHARSET_ASCII, 0	/* CET-REVIEW */
  , NULL_POS_OPS,
  nullptr
};
