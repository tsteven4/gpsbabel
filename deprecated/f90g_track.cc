/*
  Map file reader for F90G Automobile DVR.

    Copyright (C) 2014 Jim Keeler
    Copyright (C) 2001-2013 Robert Lipe

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

    Read the map file contents picking out the defined record types.

    The map file contains a constant 30 byte header record followed by a variable number of
    TT records.  The TT records start with the two characters "TT" and are 251 bytes long.
    The TT records contain values for time, position and velocity.

 */

#include "f90g_track.h"

#include <QDate>      // for QDate
#include <QDateTime>  // for QDateTime
#include <QFileInfo>  // for QFileInfo
#include <QTime>      // for QTime
#include <Qt>         // for UTC

#include <cstdio>     // for sscanf, snprintf, SEEK_SET
#include <cstring>    // for memcmp

#include "defs.h"     // for fatal, Waypoint, track_add_head, track_add_wpt, route_head
#include "gbfile.h"   // for gbfread, gbfclose, gbfopen, gbfseek


#define MYNAME "f90g_track"
#define TTRECORDSIZE      249
#define HEADERRECORDSIZE  30
#define FLIPEDBITS        0xaa
#define VALIDHEADER "MEDIA 1."

/*******************************************************************************
* %%%        global callbacks called by gpsbabel main process              %%% *
*******************************************************************************/
void
F90gTrackFormat::rd_init(const QString& fname)
{
  char header[HEADERRECORDSIZE];

  fin = gbfopen(fname, "r", MYNAME);
  gbfseek(fin, 0, SEEK_SET);
  if (gbfread(header, 1, HEADERRECORDSIZE, fin) != HEADERRECORDSIZE) {
    fatal(MYNAME ": read error");
  } else {
    // flip bits and check for valid header
    for (char &i : header) {
        i ^= FLIPEDBITS;
    }
    if (memcmp(header, VALIDHEADER, sizeof(VALIDHEADER)-1)) {
      fatal(MYNAME ": bad header");
    }
    // start the track list
    track = new route_head;
    track->rte_name = QFileInfo(fname).fileName();
    track_add_head(track);
  }
}

void
F90gTrackFormat::rd_deinit()
{
  gbfclose(fin);
}

void
F90gTrackFormat::read()
{
  char northSouth, eastWest, velocityMark, ttRec[TTRECORDSIZE], tempBuf[20];
  int year, mon, mday, hour, min, sec, latitudeDeg, latitudeMin, longitudeDeg, longitudeMin, velocity;

  if (track == nullptr) {
    fatal(MYNAME "Track setup error");
  }
  for (;;) {
    if ((gbfread((void*)ttRec, 1, 2, fin) != 2)
        || (memcmp(ttRec,"TT",2))) {
      break;
    }
    if (gbfread((void*)ttRec, 1, TTRECORDSIZE, fin) != TTRECORDSIZE) {
      break;
    }
    for (char &i : ttRec) {
        i ^= FLIPEDBITS;
    }

    // Pick the TT record apart and if it is good, fill in a new Waypoint
    year = mon = mday = hour = min = sec = latitudeDeg = latitudeMin = longitudeDeg = longitudeMin = velocity= 0;
    // Get the time stamp
    sscanf(&ttRec[15],"%4d%2d%2d%2d%2d%2d" ,&year, &mon, &mday, &hour, &min, &sec);
    // Get latitude and longitude
    sscanf(&ttRec[30],"%1c%2d%6d%1c%3d%6d", &northSouth, &latitudeDeg, &latitudeMin,
           &eastWest, &longitudeDeg, &longitudeMin);
    // Get velocity (KPH)
    sscanf(&ttRec[53],"%1c%3d", &velocityMark, &velocity);

    // sanity check the data before committing to the Waypoint
    if (year != 0 && (northSouth == 'N' || northSouth == 'S') && (eastWest == 'E' || eastWest == 'W')
        && velocityMark == 'M') {

      // create the Waypoint and fill it in
      auto* readWaypoint = new Waypoint;
      QDateTime dt = QDateTime(QDate(year, mon, mday), QTime(hour, min, sec), Qt::UTC);

      readWaypoint->SetCreationTime(dt);
      readWaypoint->latitude = (double(latitudeDeg) + double(latitudeMin)/MIN_PER_DEGREE)
                               * ((northSouth == 'N')? 1.0f : -1.0f);
      readWaypoint->longitude = (double(longitudeDeg) + double(longitudeMin)/MIN_PER_DEGREE)
                                * ((eastWest == 'E')? 1.0f : -1.0f);
//       qDebug() << dt.toString() << latitudeDeg << latitudeMin << readWaypoint->latitude;
      readWaypoint->speed = float(velocity) * SPEED_CONVERSION;
      // Name the Waypoint
      snprintf(tempBuf, sizeof(tempBuf), "%2.2dM%2.2dS-%3.3dKPH", min, sec, velocity);
      readWaypoint->shortname = QString(tempBuf);

      // Add the Waypoint to the current track
      track_add_wpt(track, readWaypoint);
    }
  }
}
