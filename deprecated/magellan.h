/*
    Copyright (C) 2002-2005 Robert Lipe

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

#ifndef MAGELLAN_H_INCLUDED_
#define MAGELLAN_H_INCLUDED_

#include <QString>
#include "defs.h"

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

QString mag_find_descr_from_token(const char* token);
QString mag_find_token_from_descr(const QString& icon);

unsigned int mag_checksum(const char*buf);
QString m330_cleanse(const char* istring);

Waypoint* mag_trkparse(char* trkmsg);
void mag_rteparse(char* rtemsg);

#endif  // MAGELLAN_H_INCLUDED_
