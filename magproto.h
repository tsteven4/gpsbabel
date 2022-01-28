/*
    Communicate Thales/Magellan serial protocol.

    Copyright (C) 2002, 2003, 2004, 2005, 2006, 2007,
      2008, 2010  Robert Lipe, robertlipe+source@gpsbabel.org

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
#ifndef MAGPROTO_H_INCLUDED_
#define MAGPROTO_H_INCLUDED_

#include <QList>            // for QList
#include <QString>          // for QString
#include <QStringList>      // for QStringList
#include <QVector>          // for QVector

#include "defs.h"           // for arglist_t, ARG_NOMINMAX, Waypoint, ARGTYPE_INT, CET_CHARSET_ASCII, FF_CAP_RW_ALL, ff_cap, ff_type, ARGTYPE_BOOL, ARGTYPE_STRING, ff_type_file, route_head, ff_type_serial, gpsdata_type, short_handle
#include "explorist_ini.h"  // for mag_info
#include "format.h"         // for Format
#include "gbfile.h"         // for gbfile
#include "inifile.h"        // for inifile_t

class Explorist
{
public:
  /* Types */

  /*
   * Interesting traits of the device from the *.ini files.
   */
  struct mag_info {
    char* geo_path;
    char* track_path;
    char* waypoint_path;
  };
  
  /* Member Functions */

  mag_info* explorist_ini_get(const char** dirlist);
  static void explorist_ini_done(mag_info* info);

private:
  /* Constants */

  static constexpr char myname[] = "explorist";

  /* Member Functions */

  mag_info* explorist_ini_try(const char*);

  /* Data Members */

  inifile_t* inifile{};
};

class MagprotoBase : private Explorist
{
protected:
  /*
   * Table of "interesting" Magellan models.
   * Selfishly, if I haven't heard of it, it's not in the table.
   * This doesn't mean I actually have TRIED all models listed below.
   * (Donations welcome. :-)
   */
  enum meridian_model {
    mm_unknown = 0 ,
    mm_gps315320,
    mm_map410,
    mm_map330,
    mm_gps310,
    mm_meridian,
    mm_sportrak
  };
  
  struct pid_to_model_t {
    meridian_model model;
    int pid;
    const char* model_n;
  };
  
  struct magellan_icon_mapping_t {
    const char* token;
    const char* icon;
  };

  /* Types */

  enum mag_rxstate {
    mrs_handoff = 0,
    mrs_handon,
    mrs_awaiting_ack
  };

  /*
   *   An individual element of a route.
   */
  struct mag_rte_elem {
    QString wpt_name;
    QString wpt_icon;
  };

  /*
   *  A header of a route.  Related elements of a route belong to this.
   */
  struct mag_rte_head_t {
    QList<mag_rte_elem*> elem_list; /* list of child rte_elems */
    char* rte_name{nullptr};
    int nelems{0};
  };


  /* Constants */

  static constexpr const magellan_icon_mapping_t gps315_icon_table[] = {
    { "a", "filled circle" },
    { "b", "box" },
    { "c", "red buoy" },
    { "d", "green buoy" },
    { "e", "buoy" },
    { "f", "rocks" },
    { "g", "red daymark" },
    { "h", "green daymark" },
    { "i", "bell" },
    { "j", "danger" },
    { "k", "diver down" },
    { "l", "fish" },
    { "m", "house" },
    { "n", "mark" },
    { "o", "car" },
    { "p", "tent" },
    { "q", "boat" },
    { "r", "food" },
    { "s", "fuel" },
    { "t", "tree" },
    { nullptr, nullptr }
  };

  static constexpr const magellan_icon_mapping_t map330_icon_table[] = {
    { "a", "crossed square" },
    { "b", "box" },
    { "c", "house" },
    { "d", "aerial" },
    { "e", "airport" },
    { "f", "amusement park" },
    { "g", "ATM" },
    { "g", "Bank" },
    { "h", "auto repair" },
    { "i", "boating" },
    { "j", "camping" },
    { "k", "exit ramp" },
    { "l", "first aid" },
    { "m", "nav aid" },
    { "n", "buoy" },
    { "o", "fuel" },
    { "p", "garden" },
    { "q", "golf" },
    { "r", "hotel" },
    { "s", "hunting/fishing" },
    { "t", "large city" },
    { "u", "lighthouse" },
    { "v", "major city" },
    { "w", "marina" },
    { "x", "medium city" },
    { "y", "museum" },
    { "z", "obstruction" },
    { "aa", "park" },
    { "ab", "resort" },
    { "ac", "restaurant" },
    { "ad", "rock" },
    { "ae", "scuba" },
    { "af", "RV service" },
    { "ag", "shooting" },
    { "ah", "sight seeing" },
    { "ai", "small city" },
    { "aj", "sounding" },
    { "ak", "sports arena" },
    { "al", "tourist info" },
    { "am", "truck service" },
    { "an", "winery" },
    { "ao", "wreck" },
    { "ap", "zoo" },
    { "ah", "Virtual cache"}, 	/* Binos: because you "see" them. */
    { "ak", "Micro-Cache" },	/* Looks like a film canister. */
    { "an", "Multi-Cache"}, 	/* Winery: grapes 'coz they "bunch" */
    { "s",  "Unknown Cache"}, 	/* 'Surprise' cache: use a target. */
    { "ac",  "Event Cache"}, 	/* Event caches.  May be food. */
    { nullptr, nullptr }
  };

  static constexpr pid_to_model_t pid_to_model[] = {
    { mm_gps315320, 19, "ColorTrak" },
    { mm_gps315320, 24, "GPS 315/320" },
    { mm_map410, 25, "Map 410" },
    { mm_map330, 30, "Map 330" },
    { mm_gps310, 31, "GPS 310" },
    { mm_meridian, 33, "Meridian" },
    { mm_meridian, 35, "ProMark 2" },
    { mm_sportrak, 36, "SporTrak Map/Pro" },
    { mm_sportrak, 37, "SporTrak" },
    { mm_meridian, 38, "FX324 Plotter" },
    { mm_meridian, 39, "Meridian Color" },
    { mm_meridian, 40, "FX324C Plotter" },
    { mm_sportrak, 41, "Sportrak Color" },
    { mm_sportrak, 42, "Sportrak Marine" },
    { mm_meridian, 43, "Meridian Marine" },
    { mm_sportrak, 44, "Sportrak Topo" },
    { mm_sportrak, 45, "Mystic" },
    { mm_meridian, 46, "MobileMapper" },
    { mm_meridian, 110, "Explorist 100" },
    { mm_meridian, 111, "Explorist 200" },
    { mm_unknown, 0, nullptr }
  };

  /* Member Functions */

  static QString m315_cleanse(const char* istring);
  static QString m330_cleanse(const char* istring);
  static unsigned int mag_checksum(const char* buf);
  static unsigned int mag_pchecksum(const char* buf, int len);
  void mag_writemsg(const char* buf);
  void mag_writeack(int osum);
  void mag_handon();
  void mag_handoff();
  void mag_verparse(char* ibuf);
  void mag_readmsg(gpsdata_type objective);
  int terminit(const QString& portname, int create_ok);
  QString termread(char* ibuf, int size);
  static void mag_dequote(char* ibuf);
  void termwrite(const char* obuf, int size);
  void termdeinit();
  void mag_serial_init_common(const QString& portname);
  void mag_rd_init_common(const QString& portname);
  void mag_rd_init(const QString& portname);
  void mag_wr_init_common(const QString& portname);
  void mag_wr_init(const QString& portname);
  void mag_deinit();
  void mag_wr_deinit();
  void parse_istring(char* istring);
  Waypoint* mag_trkparse(char* trkmsg);
  void mag_rteparse(char* rtemsg);
  QString mag_find_descr_from_token(const char* token);
  QString mag_find_token_from_descr(const QString& icon);
  Waypoint* mag_wptparse(char* /*trkmsg*/);
  void mag_read();
  void mag_waypt_pr(const Waypoint* waypointp);
  void mag_track_disp(const Waypoint* waypointp);
  void mag_track_pr();
  void mag_route_trl(const route_head* rte);
  void mag_route_pr();
  void mag_write();
  static const char** os_get_magellan_mountpoints();
  static QStringList os_gpx_files(const char* dirname);

  /* Data Members */

   int bitrate = 4800;
   int wptcmtcnt{};
   int wptcmtcnt_max{};
   int explorist{};
   int broken_sportrak{};

   short_handle mkshort_handle = nullptr;
   char* deficon = nullptr;
   char* bs = nullptr;
   char* cmts = nullptr;
   char* noack = nullptr;
   char* nukewpt = nullptr;
   int route_out_count{};
   int waypoint_read_count{};
   int wpt_len = 8;
   QString curfname;
   int extension_hint{};
   // For Explorist GC/510/610/710 families, bludgeon in GPX support.
   // (This has nothing to do with the Explorist 100...600 products.)
   Format* gpx_vec{};
   mag_info* explorist_info{};

  /*
   * Magellan's firmware is *horribly* slow to send the next packet after
   * we turn around an ack while we are reading from the device.  It's
   * quite spiffy when we're writing to the device.   Since we're *way*
   * less likely to lose data while reading from it than it is to lose data
   * when we write to it, we turn off the acks when we are predominantly
   * reading.
   */
  int suppress_ack{};

  QList<Waypoint*> rte_wpt_tmp; /* temporary PGMNWPL msgs for routes */

  gbfile* magfile_h{};
  mag_rxstate magrxstate;
  int mag_error{};
  unsigned int last_rx_csum{};
  int found_done{};
  int got_version{};
  int is_file = 0;
  route_head* trk_head{};
  int ignore_unable{};

  using cleanse_fn = QString(const char*);
  cleanse_fn* mag_cleanse{};

  const magellan_icon_mapping_t* icon_mapping = map330_icon_table;

  void* serial_handle = nullptr;

  char ifield[20][100]{};

  QVector<arglist_t> mag_fargs = {
    {
      "deficon", &deficon, "Default icon name", nullptr, ARGTYPE_STRING,
      ARG_NOMINMAX, nullptr
    },
    {
      "maxcmts", &cmts, "Max number of comments to write (maxcmts=200)",
      nullptr, ARGTYPE_INT, ARG_NOMINMAX, nullptr
    }
  };
};

class MagprotoSerialFormat : public Format, private MagprotoBase
{
public:
  QVector<arglist_t>* get_args() override
  {
    return &mag_sargs;
  }

  ff_type get_type() const override
  {
    return ff_type_serial;
  }

  QVector<ff_cap> get_cap() const override
  {
    return FF_CAP_RW_ALL;
  }

  QString get_encode() const override
  {
    return CET_CHARSET_ASCII;
  }

  int get_fixed_encode() const override
  {
    return 0;
  }

  void rd_init(const QString& fname) override {mag_rd_init(fname);}
  void read() override {mag_read();}
  void rd_deinit() override {mag_deinit();}
  void wr_init(const QString& fname) override {mag_wr_init(fname);}
  void write() override {mag_write();}
  void wr_deinit() override {mag_deinit();}

private:
  QVector<arglist_t> mag_sargs = {
    {
      "deficon", &deficon, "Default icon name", nullptr, ARGTYPE_STRING,
      ARG_NOMINMAX, nullptr
    },
    {
      "maxcmts", &cmts, "Max number of comments to write (maxcmts=200)",
      "200", ARGTYPE_INT, ARG_NOMINMAX, nullptr
    },
    {
      "baud", &bs, "Numeric value of bitrate (baud=4800)", "4800",
      ARGTYPE_INT, ARG_NOMINMAX, nullptr
    },
    {
      "noack", &noack, "Suppress use of handshaking in name of speed",
      nullptr, ARGTYPE_BOOL, ARG_NOMINMAX, nullptr
    },
    {
      "nukewpt", &nukewpt, "Delete all waypoints", nullptr, ARGTYPE_BOOL,
      ARG_NOMINMAX, nullptr
    }
  };
};

class MagprotoFileFormat : public Format, private MagprotoBase
{
public:
  QVector<arglist_t>* get_args() override
  {
    return &mag_fargs;
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
    return CET_CHARSET_ASCII;
  }

  int get_fixed_encode() const override
  {
    return 0;
  }

  void rd_init(const QString& fname) override {mag_rd_init(fname);}
  void read() override {mag_read();}
  void rd_deinit() override {mag_deinit();}
  void wr_init(const QString& fname) override {mag_wr_init(fname);}
  void write() override {mag_write();}
  void wr_deinit() override {mag_deinit();}
};

class MagprotoXFileFormat : public Format, private MagprotoBase
{
public:
  QVector<arglist_t>* get_args() override
  {
    return &mag_fargs;
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
    return CET_CHARSET_ASCII;
  }

  int get_fixed_encode() const override
  {
    return 0;
  }

  void rd_init(const QString& fname) override;
  void read() override {mag_read();}
  void rd_deinit() override {mag_deinit();}
  void wr_init(const QString& fname) override;
  void write() override {mag_write();}
  void wr_deinit() override {mag_wr_deinit();}
};
#endif // MAGPROTO_H_INCLUDED_
