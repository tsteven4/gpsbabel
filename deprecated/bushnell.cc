/*
    Read and write Bushnell files.

    Copyright (C) 2008, 2009  Robert Lipe

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
#include <QtCore/QFileInfo>
#include <cmath>
#define MYNAME "Bushnell"

static gbfile* file_in;
static QString ofname;
static short_handle mkshort_handle = nullptr;

static
QVector<arglist_t> bushnell_args = {
};

// Apparently, the icons are undocumented, so we made up names,
// preferring them to be consistent with other brands where possible.

struct bushnell_icon_mapping_t {
  signed int symbol;
  const char* icon;
};

static const bushnell_icon_mapping_t bushnell_icons[] = {
  { 0x00, "Yellow Square"},
  { 0x01, "Blue Grey Circle" },
  { 0x02, "Yellow Diamond" },
  { 0x03, "Blue Asterisk" },
  { 0x04, "Blue Bulls Eye pointing NE" },
  { 0x05, "Red =O= on a 45 degree." },
  { 0x06, "House" },
  { 0x06, "Residence" },
  { 0x07, "Lodging" },
  { 0x08, "Hospital" },
  { 0x09, "Auto Repair" },
  { 0x09, "Car Repair" },
  { 0x0a, "Tools" },
  { 0x0b, "Gas" },
  { 0x0c, "Hiking" },
  { 0x0d, "Camping" },
  { 0x0e, "Picnic Area" },
  { 0x0f, "Deer Stand" },
  { 0x10, "Deer" },
  { 0x11, "Park" },
  { 0x11, "Tree" },
  { 0x12, "Highway Exit" },
  { 0x13, "Fjord"}, // Looks like a road narrows.
  { 0x14, "Bridge" },
  { 0x15, "Waypoint" }, //  or golf hole/flag
  { 0x16, "Warning" }, //  Caution Triangle with ! in it.
  { 0x17, "Bicycle" },

  { 0x18, "Blue Circle" }, // ? in it, undocumented icon.
  { 0x19, "Blue Diamond Checkmark" }, //  undocumented.

  { 0x1a, "Camera" },
  { 0x1b, "Restaurant" }, // "Fork/Knife (meal place?)"
  { 0x1c, "Restroom" }, // (man & Woman icon)"
  { 0x1d, "RV Park" }, // "Bus or RV (RV campground?)"
  { 0x1e, "Potable Water" }, // (faucet/glass or bucket)"
  { 0x1f, "Fishing" },
  { 0x20, "Anchor in square" },
  { 0x21, "Boat ramp/launch" },
  { 0x22, "Anchor" },
  { 0x23, "Buoy" },
  { 0x24, "Man Overboard?" },
  { 0x25, "Snow Skiing" },
  { 0x26, "Mountain/Mountain Peak" },
  { 0x27, "Turkey Tracks/animal tracks" },

  { 0x28, "Bank" }, // "Cash (ATM Maybe)"
  { 0x29, "Bar" }, // "Martini undocumented"
  { 0x2a, "Lighthouse" },

  { 0x2b, "Tent" },

  { 0x2c, "Crescent Wrench or can opener" },

  { 0x2d, "School" }, //? White Building with tunnel looking door and flag on top."
  { 0x2f, "Information" }, // "i  (info/internet maybe?)"
  { 0x30, "Picnic" }, //"Picnic table & Tree, maybe forest picnic or day use area?"
  { 0x31, "Phone" },
  { 0x32, "Letter/Envelope" },
  { 0x33, "Forest/Park Ranger" },
  { 0x34, "Fire department" }, //? Red Square building with yellow flag."

  { 0x35, "Shopping" },
  { 0x36, "Looks like Cross+hurricane symbol, strange also undocumented." },

  { 0x37, "Tunnel" },
  { 0x38, "Mountain/Summit" },

  { 0x39, "Square split diagonally with lines between... magnet maybe? undocumented" },

  { 0x3a, "Swimmer/swimming" },
  { 0x3b, "Officer? Looks like man leaned over holding blue cube..." },
  { 0x3c, "Parking" }, //"Car Parked"
  { 0x3d, "Airport" },
  { 0x3e, "Bus Terminal" }, // (guess) Looks like Bus under canopy."
  { 0x3f, "Red Cross" },
  { 0x40, "Red Building with flag, Fire Station maybe." },
  { 0x41, "Bus" },
  { 0x42, "Officer" }, // "see 3b: duplicate"
  { 0x43, "Railroad" },
  { 0x44, "Auto Ferry" },
  {-1, nullptr}
};

static unsigned int
bushnell_get_icon_from_name(QString name)
{
  if (name.isNull()) {
    name = "Waypoint";
  }

  for (const bushnell_icon_mapping_t* t = bushnell_icons; t->icon != nullptr; t++) {
    if (0 == name.compare(t->icon, Qt::CaseInsensitive)) {
      return t->symbol;
    }
  }
  return 0;
}

static const char*
bushnell_get_name_from_symbol(signed int s)
{
  for (const bushnell_icon_mapping_t* t = bushnell_icons; t->icon != nullptr; t++) {
    if (s == t->symbol) {
      return t->icon;
    }
  }
  return "Waypoint";
}

static void
rd_init(const QString& fname)
{
  file_in = gbfopen_le(fname, "rb", MYNAME);
}

static void
rd_deinit()
{
  gbfclose(file_in);
}

static void
wr_init(const QString& fname)
{
  static char valid_chars [] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789"
                               ".-/\\~@#$%^&*()_+=<>"
                               "abcdefghijklmnopqrstuvwxyz";

  // If user provided an extension in the pathname, whack it.
  ofname = fname;
  int suffix_len = QFileInfo(fname).suffix().length();
  if (suffix_len > 0) {
    /* drop the suffix and the period */
    ofname.chop(suffix_len + 1);
  }

  mkshort_handle = mkshort_new_handle();
  setshort_length(mkshort_handle, 19);
  setshort_goodchars(mkshort_handle, valid_chars);
}

static void
wr_deinit()
{
  mkshort_del_handle(&mkshort_handle);
  ofname.clear();
}

/*
 * Each file contains a single waypoint.
 */
static void
bushnell_read()
{
  auto* wpt_tmp = new Waypoint;

  int32_t lat_tmp = gbfgetint32(file_in);
  int32_t lon_tmp = gbfgetint32(file_in);

  unsigned int icon = gbfgetc(file_in);
  wpt_tmp->icon_descr = bushnell_get_name_from_symbol(icon);
  unsigned int proximity = gbfgetc(file_in); // 1 = off, 3 = proximity alarm.
  (void) proximity;
  wpt_tmp->latitude = lat_tmp /  10000000.0;
  wpt_tmp->longitude = lon_tmp / 10000000.0;

  // Apparently this is always zero terminated, though it's never been
  // observed to be longer than 19 bytes + a null terminator.
  wpt_tmp->shortname = gbfgetstr(file_in);

  waypt_add(wpt_tmp);
}

static void
bushnell_write_one(const Waypoint* wpt)
{
  char tbuf[20]; // 19 text bytes + null terminator.
  char padding[2] = {0, 0};
  static int wpt_count;
  QString fname(ofname);
  fname += "-";
  fname += QString::number(wpt_count++);
  fname += ".wpt";

  gbfile* file_out = gbfopen_le(fname, "wb", MYNAME);
  gbfputint32(round(wpt->latitude  * 10000000), file_out);
  gbfputint32(round(wpt->longitude * 10000000), file_out);
  gbfputc(bushnell_get_icon_from_name(wpt->icon_descr), file_out);
  gbfputc(0x01, file_out);  // Proximity alarm.  1 == "off", 3 == armed.

  strncpy(tbuf, CSTRc(wpt->shortname), sizeof(tbuf) - 1);
  tbuf[sizeof(tbuf)-1] = 0;
  gbfwrite(tbuf, sizeof(tbuf), 1, file_out);

  // two padding bytes follow name.
  gbfwrite(padding, sizeof(padding), 1, file_out);

  gbfclose(file_out);
}

static void
bushnell_write()
{
  waypt_disp_all(bushnell_write_one);
}

ff_vecs_t bushnell_vecs = {
  ff_type_file,
  FF_CAP_RW_WPT,
  rd_init,
  wr_init,
  rd_deinit,
  wr_deinit,
  bushnell_read,
  bushnell_write,
  nullptr,
  &bushnell_args,
  CET_CHARSET_MS_ANSI, 0,  /* Not really sure... */
  NULL_POS_OPS,
  nullptr
};
