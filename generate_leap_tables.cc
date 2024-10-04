/*
    Copyright (C) 2024 Robert Lipe, gpsbabel.org

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

#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QIODevice>
#include <QList>

/* This data is from leap-seconds.list from https://data.iana.org/time-zones/releases/tzdata2024b.tar.gz
 * Note that the entry
 * "2272060800      10      # 1 Jan 1972"
 * is not the insertion of a leap second, it is the establishment of the offset at the given time.
 */

struct leap_second_data {
  qint64 ntpTime;
  int dtai;
};

const QList<leap_second_data> leap_info = {
  {2287785600, 11}, // 1 Jul 1972
  {2303683200, 12}, // 1 Jan 1973
  {2335219200, 13}, // 1 Jan 1974
  {2366755200, 14}, // 1 Jan 1975
  {2398291200, 15}, // 1 Jan 1976
  {2429913600, 16}, // 1 Jan 1977
  {2461449600, 17}, // 1 Jan 1978
  {2492985600, 18}, // 1 Jan 1979
  {2524521600, 19}, // 1 Jan 1980
  {2571782400, 20}, // 1 Jul 1981
  {2603318400, 21}, // 1 Jul 1982
  {2634854400, 22}, // 1 Jul 1983
  {2698012800, 23}, // 1 Jul 1985
  {2776982400, 24}, // 1 Jan 1988
  {2840140800, 25}, // 1 Jan 1990
  {2871676800, 26}, // 1 Jan 1991
  {2918937600, 27}, // 1 Jul 1992
  {2950473600, 28}, // 1 Jul 1993
  {2982009600, 29}, // 1 Jul 1994
  {3029443200, 30}, // 1 Jan 1996
  {3076704000, 31}, // 1 Jul 1997
  {3124137600, 32}, // 1 Jan 1999
  {3345062400, 33}, // 1 Jan 2006
  {3439756800, 34}, // 1 Jan 2009
  {3550089600, 35}, // 1 Jul 2012
  {3644697600, 36}, // 1 Jul 2015
  {3692217600, 37}, // 1 Jan 2017
};

/* The generated unix times using secsFromUnixEpoch can be compared with those in libsdtc++-v3:
 * https://github.com/gcc-mirror/gcc/blob/d77f073ce66cedbcbb22357c49b9ef19e1b61a43/libstdc%2B%2B-v3/src/c%2B%2B20/tzdb.cc#L1200-L1230
 */


const QString license = \
"/*\n" \
"    Copyright (C) 2024  Robert Lipe, robertlipe+source@gpsbabel.org\n" \
"\n" \
"    This program is free software; you can redistribute it and/or modify\n" \
"    it under the terms of the GNU General Public License as published by\n" \
"    the Free Software Foundation; either version 2 of the License, or\n" \
"    (at your option) any later version.\n" \
"\n" \
"    This program is distributed in the hope that it will be useful,\n" \
"    but WITHOUT ANY WARRANTY; without even the implied warranty of\n" \
"    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n" \
"    GNU General Public License for more details.\n" \
"\n" \
"    You should have received a copy of the GNU General Public License\n" \
"    along with this program; if not, write to the Free Software\n" \
"    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.\n" \
"\n" \
" */\n\n" \
;

int main()
{
  const QDateTime ntpEpoch(QDate(1900,01,01), QTime(0,0), Qt::UTC);
  const auto ntpOffset = ntpEpoch.toSecsSinceEpoch();
  assert(ntpOffset == -2208988800);
  qDebug() << ntpEpoch << ntpOffset;
  const QDateTime gpsEpoch(QDate(1980,01,06), QTime(0,0), Qt::UTC);
  const auto gpsOffset = gpsEpoch.toSecsSinceEpoch();
  constexpr qint64 unixLeapOffset = 10;
  constexpr qint64 gpsLeapOffset = 19;
  qDebug() << gpsEpoch << gpsOffset;

  QFile unixFile = QFile("leap_tables.h");
  if (!unixFile.open(QFile::WriteOnly)) {
    return 1;
  }
  QDebug unixStream(&unixFile);

  unixStream.nospace().noquote() << license;
  unixStream.nospace() << "struct leap_t {" << Qt::endl;
  unixStream.nospace() << "  qint64 secsSinceEpoch;" << Qt::endl;
  unixStream.nospace() << "  int secsOffset;" << Qt::endl;
  unixStream.nospace() << "};" << Qt::endl << Qt::endl;

  unixStream.nospace() << "const QVector<leap_t> unix_leap_table = {" << Qt::endl;
  for (const auto entry : leap_info) {
    qint64 secsFromUnixEpoch = entry.ntpTime + ntpOffset;
    unixStream.nospace().noquote() << "  {" <<
                                   secsFromUnixEpoch <<
                                   ", " <<
                                   entry.dtai - unixLeapOffset <<
                                   "}, // " <<
                                   QDateTime::fromSecsSinceEpoch(secsFromUnixEpoch).toUTC().toString(Qt::ISODate) <<
                                   Qt::endl;
  }
  unixStream.nospace() << "};" << Qt::endl << Qt::endl;

  unixStream.nospace() << "const QVector<leap_t> gps_leap_table = {" << Qt::endl;
  for (const auto entry : leap_info) {
    qint64 secsFromUnixEpoch = entry.ntpTime + ntpOffset;
    qint64 secsFromGpsEpoch = entry.ntpTime + ntpOffset - gpsOffset;
    if (secsFromGpsEpoch > 0) {
      unixStream.nospace().noquote() << "  {" <<
                                     secsFromGpsEpoch <<
                                     ", " <<
                                     entry.dtai - gpsLeapOffset <<
                                     "}, // " <<
                                     QDateTime::fromSecsSinceEpoch(secsFromUnixEpoch).toUTC().toString(Qt::ISODate) <<
                                     Qt::endl;
    }
  }

  unixStream.nospace() << "};" << Qt::endl;

  unixFile.close();
}

