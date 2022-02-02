/*
 * FAI/IGC data format translation.
 *
 * Refer to Appendix 1 of
 * http://www.fai.org:81/gliding/gnss/tech_spec_gnss.asp for the
 * specification of the IGC data format.  This translation code was
 * written when the latest amendment list for the specification was AL6.
 *
 * Copyright (C) 2004 Chris Jones
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#ifndef IGC_H_INCLUDED_
#define IGC_H_INCLUDED_

#include <QString>                 // for QString
#include <QVector>                 // for QVector

#include <ctime>                   // for time_t, tm

#include "defs.h"                  // for arglist_t, ff_cap, route_head, Waypoint, ff_cap_read, ff_cap_write, ARGTYPE_STRING, ARG_NOMINMAX, CET_CHARSET_ASCII, ff_cap_none, ff_type, ff_type_file
#include "format.h"                // for Format
#include "gbfile.h"                // for gbfile


class IgcFormat : public Format
{
public:
  QVector<arglist_t>* get_args() override
  {
    return &igc_args;
  }

  ff_type get_type() const override
  {
    return ff_type_file;
  }

  QVector<ff_cap> get_cap() const override
  {
    return {
      ff_cap_none,                          // waypoints
      (ff_cap)(ff_cap_read | ff_cap_write), // tracks
      (ff_cap)(ff_cap_read | ff_cap_write)  // routes
    };
  }

  QString get_encode() const override
  {
    return CET_CHARSET_ASCII;
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

private:
  /* Types */

  /*
   * IGC record types.
   * These appear as the first char in each record.
   */
  enum igc_rec_type_t {
    rec_manuf_id = 'A',		// FR manufacturer and identification
    rec_fix = 'B',		// Fix
    rec_task = 'C',		// Task/declaration
    rec_diff_gps = 'D',		// Differential GPS
    rec_event = 'E',		// Event
    rec_constel = 'F',		// Constellation
    rec_security = 'G',		// Security
    rec_header = 'H',		// File header
    rec_fix_defn = 'I',		// List of extension data included at end of each fix (B) record
    rec_extn_defn = 'J',	// List of data included in each extension (K) record
    rec_extn_data = 'K',	// Extension data
    rec_log_book = 'L',		// Logbook/comments

    // M..Z are spare

    rec_none = 0,		// No record
    rec_bad = 1,		// Bad record
  };

  enum state_t { id, takeoff, start, turnpoint, finish, landing };

  /* Member Functions */

  static unsigned char coords_match(double lat1, double lon1, double lat2, double lon2);
  igc_rec_type_t get_record(char** rec);
  friend inline state_t& operator++(state_t& s);
  friend inline state_t operator++(state_t& s, int);
  static void igc_task_rec(const char* rec);
  void detect_pres_track(const route_head* rh);
  void detect_gnss_track(const route_head* rh);
  void detect_other_track(const route_head* rh);
  void get_tracks(const route_head** pres_track, const route_head** gnss_track);
  static char* latlon2str(const Waypoint* wpt);
  static char* date2str(tm* dt);
  static char* tod2str(tm* tod);
  void wr_header();
  void wr_task_wpt_name(const Waypoint* wpt, const char* alt_name) const;
  void wr_task_hdr(const route_head* rte) const;
  void wr_task_wpt(const Waypoint* wpt) const;
  void wr_task_tlr(const route_head* rte) const;
  void wr_tasks();
  void wr_fix_record(const Waypoint* wpt, int pres_alt, int gnss_alt) const;
  static int correlate_tracks(const route_head* pres_track, const route_head* gnss_track);
  static double interpolate_alt(const route_head* track, time_t time);
  void wr_track();

  /* Data Members */

  gbfile* file_in{}, *file_out{};
  char manufacturer[4]{};
  const route_head* head{};
  char* timeadj = nullptr;
  int lineno{};

  QVector<arglist_t> igc_args = {
    {
      "timeadj", &timeadj,
      "(integer sec or 'auto') Barograph to GPS time diff",
      nullptr, ARGTYPE_STRING, ARG_NOMINMAX, nullptr
    }
  };
};
#endif // IGC_H_INCLUDED_
