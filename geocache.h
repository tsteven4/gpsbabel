/*
    Copyright (C) 2002-2012 Robert Lipe, robertlipe+source@gpsbabel.org

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

#ifndef GEOCACHE_H_INCLUDED_
#define GEOCACHE_H_INCLUDED_

#include <QString>              // for QString
#include <QVector>              // for QVector

#include "defs.h"               // for strip_html
#include "src/core/datetime.h"  // for DateTime


class Geocache
{
  /*
   * Extended data if waypoint happens to represent a geocache.  This is
   * totally voluntary data...
   */

public:

  /* Types */

  enum geocache_type {
    gt_unknown = 0,
    gt_traditional,
    gt_multi,
    gt_virtual,
    gt_letterbox,
    gt_event,
    gt_surprise,
    gt_webcam,
    gt_earth,
    gt_locationless,
    gt_benchmark, /* Extension to Groundspeak for GSAK */
    gt_cito,
    gt_ape,
    gt_mega,
    gt_wherigo
  };

  enum geocache_container {
    gc_unknown = 0,
    gc_micro,
    gc_other,
    gc_regular,
    gc_large,
    gc_virtual,
    gc_small
  };

  enum geocache_status {
    gs_unknown = 0,
    gs_true,
    gs_false
  };

  class UtfString
  {
  public:

    QString strip_html() const
    {
      return is_html? ::strip_html(utf_string) : utf_string;
    }

    bool is_html{false};
    QString utf_string;
  };

  /* Special Member Functions */

  Geocache() :
    id(0),
    type(gt_unknown),
    container(gc_unknown),
    diff(0),
    terr(0),
    is_archived(gs_unknown),
    is_available(gs_unknown),
    is_memberonly(gs_unknown),
    has_customcoords(gs_unknown),
    placer_id(0),
    favorite_points(0)
  {}

  /* Member Functions */

  void set_type(const QString& type_name);
  QString get_type() const;
  void set_container(const QString& container_name);
  QString get_container() const;
  QString get_icon() const;

  /* Data Members */

  long long id; /* The decimal cache number */
  geocache_type type:5;
  geocache_container container:4;
  unsigned int diff:6; /* (multiplied by ten internally) */
  unsigned int terr:6; /* (likewise) */
  geocache_status is_archived:2;
  geocache_status is_available:2;
  geocache_status is_memberonly:2;
  geocache_status has_customcoords:2;
  gpsbabel::DateTime exported;
  gpsbabel::DateTime last_found;
  QString placer; /* Placer name */
  int placer_id; /* Placer id */
  QString hint; /* all these UTF8, XML entities removed, May be not HTML. */
  UtfString desc_short;
  UtfString desc_long;
  int favorite_points;
  QString personal_note;

private:

  /* Types */

  struct type_mapping {
    geocache_type type;
    QString name;
  };

  struct container_mapping {
    geocache_container container;
    QString name;
  };

  /* Constants */

  static const QVector<type_mapping> type_map;
  static const QVector<container_mapping> container_map;
};
#endif // GEOCACHE_H_INCLUDED_
