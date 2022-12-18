/*
    Copyright (C) 2002-2014 Robert Lipe, robertlipe+source@gpsbabel.org

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
#ifndef WAYPT_H_INCLUDED_
#define WAYPT_H_INCLUDED_

#include <algorithm>                 // for sort, stable_sort
#include <cmath>                     // for M_PI
#include <cstdarg>                   // for va_list
#include <cstddef>                   // for NULL, nullptr_t, size_t
#include <cstdint>                   // for int32_t, uint32_t
#include <cstdio>                    // for NULL, fprintf, FILE, stdout
#include <ctime>                     // for time_t
#include <optional>                  // for optional
#include <utility>                   // for move

#if HAVE_LIBZ
#include <zlib.h>                    // doesn't really belong here, but is missing elsewhere.
#elif !ZLIB_INHIBITED
#include "zlib.h"                    // doesn't really belong here, but is missing elsewhere.
#endif

#include <QDateTime>                 // for QDateTime
#include <QDebug>                    // for QDebug
#include <QList>                     // for QList, QList<>::const_iterator, QList<>::const_reverse_iterator, QList<>::count, QList<>::reverse_iterator
#include <QScopedPointer>            // for QScopedPointer
#include <QScopedPointerPodDeleter>  // for QScopedPointerPodDeleter
#include <QString>                   // for QString
#include <QStringView>               // for QStringView
#include <QTextCodec>                // for QTextCodec
#include <QVector>                   // for QVector
#include <Qt>                        // for CaseInsensitive
#include <QtGlobal>                  // for QForeachContainer, qMakeForeachContainer, foreach, qint64

#include "defs.h"
#include "formspec.h"                // for FormatSpecificData
#include "inifile.h"                 // for inifile_t
#include "session.h"                 // for session_t
#include "src/core/datetime.h"       // for DateTime

#if 0
class geocache_data;
class UrlLink;
class UrlList;
class wp_flags;
enum fix_type;
#endif

/*
 * This is a waypoint, as stored in the GPSR.   It tries to not
 * cater to any specific model or protocol.  Anything that needs to
 * be truncated, edited, or otherwise trimmed should be done on the
 * way to the target.
 */
class Waypoint
{
public:
  /* Special Member Funtions */

  Waypoint();
  ~Waypoint();
  Waypoint(const Waypoint& other);
  Waypoint& operator=(const Waypoint& other);

  /* Member Functions */

  bool HasUrlLink() const;
  const UrlLink& GetUrlLink() const;
  void AddUrlLink(const UrlLink& l);
  QString CreationTimeXML() const;
  gpsbabel::DateTime GetCreationTime() const;
  void SetCreationTime(const gpsbabel::DateTime& t);
  void SetCreationTime(qint64 t, qint64 ms = 0);
  geocache_data* AllocGCData();
  int EmptyGCData() const;

  /* Data Members */

  double latitude;		/* Degrees */
  double longitude; 		/* Degrees */
  double altitude; 		/* Meters. */
  double geoidheight;	/* Height (in meters) of geoid (mean sea level) above WGS84 earth ellipsoid. */

  /*
   * The "thickness" of a waypoint; adds an element of 3D.  Can be
   * used to construct rudimentary polygons for, say, airspace
   * definitions.   The units are meters.
   */
  double depth;

  /*
   * An alarm trigger value that can be considered to be a circle
   * surrounding a waypoint (or cylinder if depth is also defined).
   * The units are meters.
   */
  double proximity;

  /* shortname is a waypoint name as stored in receiver.  It should
   * strive to be, well, short, and unique.   Enforcing length and
   * character restrictions is the job of the output.   A typical
   * minimum length for shortname is 6 characters for NMEA units,
   * 8 for Magellan and 10 for Vista.   These are only guidelines.
   */
  QString shortname;
  /*
   * description is typically a human readable description of the
   * waypoint.   It may be used as a comment field in some receivers.
   * These are probably under 40 bytes, but that's only a guideline.
   */
  QString description;
  /*
   * notes are relatively long - over 100 characters - prose associated
   * with the above.   Unlike shortname and description, these are never
   * used to compute anything else and are strictly "passed through".
   * Few formats support this.
   */
  QString notes;

  UrlList urls;

  wp_flags wpt_flags;
  QString icon_descr;

  gpsbabel::DateTime creation_time;

  /*
   * route priority is for use by the simplify filter.  If we have
   * some reason to believe that the route point is more important,
   * we can give it a higher (numerically; 0 is the lowest) priority.
   * This causes it to be removed last.
   * This is currently used by the saroute input filter to give named
   * waypoints (representing turns) a higher priority.
   * This is also used by the google input filter because they were
   * nice enough to use exactly the same priority scheme.
   */
  int route_priority;

  /* Optional dilution of precision:  positional, horizontal, vertical.
   * 1 <= dop <= 50
   */
  float hdop;
  float vdop;
  float pdop;
  float course;	/* Optional: degrees true */
  float speed;   	/* Optional: meters per second. */
  fix_type fix;	/* Optional: 3d, 2d, etc. */
  int  sat;	/* Optional: number of sats used for fix */

  unsigned char heartrate; /* Beats/min. likely to get moved to fs. */
  unsigned char cadence;	 /* revolutions per minute */
  float power; /* watts, as measured by cyclists */
  float temperature; /* Degrees celsius */
  float odometer_distance; /* Meters */
  geocache_data* gc_data;
  FormatSpecificDataList fs;
  const session_t* session;	/* pointer to a session struct */
  void* extra_data;	/* Extra data added by, say, a filter. */

private:
  /* Data Members */

  static geocache_data empty_gc_data;
};
#endif // WAYPT_H_INCLUDED_
