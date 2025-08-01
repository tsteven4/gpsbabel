/*

    Support for TrackLogs digital mapping (.trl) files,

    Copyright (C) 2006,2007 Olaf Klein

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

#include <cstdio>                       // for SEEK_CUR, size_t
#include <cstdint>                      // int32_t, int16_t, uint32_t
#include <cstdlib>                      // for atoi
#include <cstring>                      // for strncmp, memcpy, strcmp, strlen

#include <QByteArray>                   // for QByteArray
#include <QString>                      // for QString, operator+
#include <QXmlStreamAttributes>         // for QXmlStreamAttributes
#include <QtGlobal>                     // for qPrintable

#include "defs.h"
#include "gbfile.h"                     // for gbfgetdbl, gbfgetint32, gbfputint32, gbfgetint16, gbfputdbl, gbfputc, gbfread, gbfseek, gbfgetc, gbfile, gbfclose, gbfungetc, gbfeof, gbfputs, gbfwrite, gbfopen_le, gbfgetuint32, gbfputuint16, gbfputuint32
#include "jeeps/gpsmath.h"              // for GPS_Lookup_Datum_Index, GPS_Math_Known_Datum_To_WGS84_C, GPS_Math_NGENToAiry1830LatLon
#include "src/core/logging.h"           // for FatalMsg
#include "xmlgeneric.h"                 // for cb_cdata, xg_callback, xg_string, xml_deinit, xml_init, cb_end, cb_start, xg_cb_type, xml_read, xml_readstring, xg_tag_mapping


#define MYNAME "dmtlog"

#define DEFLATE_BUFF_SIZE 16384

static gbfile* fin, *fout;

static char* xmlbin;
static Waypoint* xmlwpt;
static route_head* xmltrk;
static QString xmlgrid;
static int xmldatum;
static double xmlEasting, xmlNorthing;
static double xmlLatitude, xmlLongitude;
static double xmlAltitude;

#if !ZLIB_INHIBITED
static int xmlbinsize;
#endif

static char header_written;
static char* opt_index;
static int track_index, this_index;

static
QVector<arglist_t> dmtlog_args = {
  {
    "index", &opt_index,
    "Index of track (if more than one in source)", "1", ARGTYPE_INT, "1", nullptr, nullptr
  },
};


#if !ZLIB_INHIBITED
static xg_callback tlog3a_xgcb_version, tlog3a_xgcb_length, tlog3a_xgcb_data;

static xg_tag_mapping tlog3a_xgcb_map[] = {
  { tlog3a_xgcb_version, 	cb_cdata, "/CXMLSafe/Version" },
  { tlog3a_xgcb_length, 	cb_cdata, "/CXMLSafe/Length" },
  { tlog3a_xgcb_data, 	cb_cdata, "/CXMLSafe/Data" },
  { nullptr,	(xg_cb_type)0,         nullptr}
};
#endif

static xg_callback tlog3b_xgcb_tfna, tlog3b_xgcb_tfdes;
static xg_callback tlog3b_xgcb_wptst, tlog3b_xgcb_tptst;
static xg_callback tlog3b_xgcb_tpten, tlog3b_xgcb_wpten;
static xg_callback tlog3b_xgcb_wptid, tlog3b_xgcb_wptdt;
static xg_callback tlog3b_xgcb_wptgr, tlog3b_xgcb_wptea;
static xg_callback tlog3b_xgcb_wptno, tlog3b_xgcb_wptal;
static xg_callback tlog3b_xgcb_tptdt;

static xg_tag_mapping tlog3b_xgcb_map[] = {
  { tlog3b_xgcb_tfna,	cb_cdata, "/CTrackFile/Name" },
  { tlog3b_xgcb_tfdes,	cb_cdata, "/CTrackFile/Description" },
  { tlog3b_xgcb_wptst,	cb_start, "/CTrackFile/CWayPoint" },
  { tlog3b_xgcb_wptid,	cb_cdata, "/CTrackFile/CWayPoint/Id" },
  { tlog3b_xgcb_wptdt,	cb_cdata, "/CTrackFile/CWayPoint/Datum" },
  { tlog3b_xgcb_wptgr,	cb_cdata, "/CTrackFile/CWayPoint/Grid" },
  { tlog3b_xgcb_wptea,	cb_cdata, "/CTrackFile/CWayPoint/Easting" },
  { tlog3b_xgcb_wptno,	cb_cdata, "/CTrackFile/CWayPoint/Northing" },
  { tlog3b_xgcb_wptal,	cb_cdata, "/CTrackFile/CWayPoint/Altitude" },
  { tlog3b_xgcb_wpten,	cb_end,   "/CTrackFile/CWayPoint" },
  { tlog3b_xgcb_tptst,	cb_start, "/CTrackFile/CTrackPoint" },
  { tlog3b_xgcb_wptid,	cb_cdata, "/CTrackFile/CTrackPoint/Id" },
  { tlog3b_xgcb_tptdt,	cb_cdata, "/CTrackFile/CTrackPoint/Datum" },
  { tlog3b_xgcb_wptgr,	cb_cdata, "/CTrackFile/CTrackPoint/Grid" },
  { tlog3b_xgcb_wptea,	cb_cdata, "/CTrackFile/CTrackPoint/Easting" },
  { tlog3b_xgcb_wptno,	cb_cdata, "/CTrackFile/CTrackPoint/Northing" },
  { tlog3b_xgcb_wptal,	cb_cdata, "/CTrackFile/CTrackPoint/Altitude" },
  { tlog3b_xgcb_tpten,	cb_end,   "/CTrackFile/CTrackPoint" },
  { nullptr,	(xg_cb_type)0,         nullptr}
};

/* helpers */

static void
convert_datum(Waypoint* wpt, int datum)
{
  if (datum != DATUM_WGS84) {
    double lat = wpt->latitude;
    double lon = wpt->longitude;
    double alt = wpt->altitude;
    GPS_Math_Known_Datum_To_WGS84_C(lat, lon, alt,
                                    &wpt->latitude, &wpt->longitude, &wpt->altitude,
                                    datum);
  }
}


static void
finalize_pt(Waypoint* wpt)
{
  if (xmlgrid == "BNG") {
    GPS_Math_NGENToAiry1830LatLon(xmlEasting, xmlNorthing,
                                  &wpt->latitude, &wpt->longitude);
    xmldatum = DATUM_OSGB36;
  } else {
    wpt->latitude = xmlLatitude;
    wpt->longitude = xmlLongitude;
  }
  /* NOTE:
   * Alan White reports this program actually subtracts a number
   * of meters ranging between 46 and 50 meters.  It appears to be
   * constant for each location, but different without an obvious
   * correlation to ground altitude.  We considered offsetting this
   * in GPSBabel, but concluded it wasn't worth the bother.
   * If we get complaints, probably all of our alt reading and writing
   * should offset an average of 46m or so.
   */
  wpt->altitude = xmlAltitude;
  convert_datum(wpt, xmldatum);
}

/* xml-reader callbacks */

#if !ZLIB_INHIBITED
static void
tlog3a_xgcb_version(xg_string args, const QXmlStreamAttributes*)
{
  if (args != "1") {
    fatal(MYNAME ": Unsupported file version '%s'!\n", qPrintable(args));
  }
}

static void
tlog3a_xgcb_length(xg_string, const QXmlStreamAttributes*)
{
}

static void
tlog3a_xgcb_data(xg_string args, const QXmlStreamAttributes*)
{
  int len;
  char* bin;
  char* cin, *cout;
  char cl, ch;
  //TODO This function needs rethinking.
  len = args.length();
  bin = (char*) xmalloc((len >> 1) + 1);

  char* cincopy  = xstrdup(args);
  cin = cincopy;
  cout = bin;

  cl = 0x10;
  while (*cin) {
    char c = *cin++;

    if (c == '\0') {
      break;
    } else if ((c >= 'A') && (c <= 'F')) {
      c -= 'A' - 10;
    } else if ((c >= 'a') && (c <= 'f')) {
      c -= 'a' - 10;
    } else if ((c >= '0') && (c <= '9')) {
      c -= '0';
    } else {
      continue;
    }

    if (cl == 0x10) {
      cl = c;
    } else {
      ch = (cl << 4) | c;
      *cout++ = ch;
      cl = 0x10;
    }
  }
  xmlbin = bin;
  xmlbinsize = (cout - bin);
  xfree(cincopy);
}
#endif


static void
tlog3b_xgcb_tfna(xg_string args, const QXmlStreamAttributes*)
{
  if (xmltrk == nullptr) {
    xmltrk = new route_head;
    track_add_head(xmltrk);
  }
  xmltrk->rte_name = args;
}


static void
tlog3b_xgcb_tfdes(xg_string args, const QXmlStreamAttributes*)
{
  if (xmltrk == nullptr) {
    xmltrk = new route_head;
    track_add_head(xmltrk);
  }
  xmltrk->rte_desc = args;
}


static void
tlog3b_xgcb_wptst(xg_string, const QXmlStreamAttributes*)
{
  xmlwpt = new Waypoint;
  xmldatum = DATUM_WGS84;
}


static void
tlog3b_xgcb_tptst(xg_string, const QXmlStreamAttributes*)
{
  xmlwpt = new Waypoint;
  xmldatum = DATUM_WGS84;
}


static void
tlog3b_xgcb_tpten(xg_string, const QXmlStreamAttributes*)
{
  finalize_pt(xmlwpt);

  if (xmltrk == nullptr) {
    xmltrk = new route_head;
    track_add_head(xmltrk);
  }
  track_add_wpt(xmltrk, xmlwpt);
  xmlwpt = nullptr;
}


static void
tlog3b_xgcb_wptid(xg_string args, const QXmlStreamAttributes*)
{
  xmlwpt->shortname = args;
}


static void
tlog3b_xgcb_wptdt(xg_string args, const QXmlStreamAttributes*)
{
  xmldatum = GPS_Lookup_Datum_Index(args);
}


static void
tlog3b_xgcb_wptgr(xg_string args, const QXmlStreamAttributes*)
{
  xmlgrid = args;
/*
  if (xmlgrid != NULL) {
    if (strcmp(xmlgrid, args) == 0) {
      return;
    }
    xfree(xmlgrid);
  }
  xmlgrid = xstrdup(args);
*/
}


static void
tlog3b_xgcb_wptno(xg_string args, const QXmlStreamAttributes*)
{
  xmlNorthing = args.toDouble();
}

static void
tlog3b_xgcb_wptea(xg_string args, const QXmlStreamAttributes*)
{
  xmlEasting = args.toDouble();
}


static void
tlog3b_xgcb_wptal(xg_string args, const QXmlStreamAttributes*)
{
  xmlAltitude = args.toDouble();
}


static void
tlog3b_xgcb_tptdt(xg_string args, const QXmlStreamAttributes*)
{
  xmldatum = GPS_Lookup_Datum_Index(args);
}


static void
tlog3b_xgcb_wpten(xg_string, const QXmlStreamAttributes*)
{
  finalize_pt(xmlwpt);
  waypt_add(xmlwpt);
  xmlwpt = nullptr;
}


static QString
read_str(gbfile* f)
{
  int i = gbfgetc(f);
  if (i == 0xff) {
    i = gbfgetint16(f);
  }

  return gbfreadbuf(i, f);
}

static void
write_str(const char* str, gbfile* f)
{
  if (str && *str) {
    int len = strlen(str);
    if (len > 0xfe) {
#if 0
      if (len > 0x7fff) {
        len = 0x7fff;
      }
      gbfputc((unsigned char) 0xff, f);
      gbfputint16(len, f);
#else
      len = 0xfe;
      gbfputc(len, f);
#endif
    } else {
      gbfputc(len, f);
    }
    gbfwrite(str, len, 1, f);
  } else {
    gbfputc(0, f);
  }
}

static void
write_str(const QString& str, gbfile* f)
{
  write_str(CSTR(str), f);
}

static int
read_datum(gbfile* f)
{
  auto d = read_str(f);
  auto g = read_str(f);

  int res = GPS_Lookup_Datum_Index(d);

  if (d.compare(g))  {
    fatal(FatalMsg() << MYNAME << ": Unsupported combination of datum '" << d << "' and grid '" << g << "''!\n");
  }

  return res;
}


static void
read_CTrackFile(const int version)
{
  int16_t u1 = gbfgetint16(fin);

  char buf[128];
  gbfread(buf, 1, 10, fin);
  if ((u1 != 0x0a) || (strncmp("CTrackFile", buf, 10) != 0)) {
    fatal(MYNAME ": Unknown or invalid track file.\n");
  }

  if (version == 8) {
    gbfseek(fin, 36, SEEK_CUR);  /* skip unknown 36 bytes */
  }

  int32_t ver = gbfgetint32(fin);
  if (ver != version) {
    fatal(MYNAME ": Unknown or invalid track file (%d).\n", ver);
  }

  (void) gbfgetint32(fin); // Unknown 2
  (void) gbfgetint32(fin); // Unknown 3
  (void) gbfgetint32(fin); // Unknown 4

  auto* track = new route_head;
  track_add_head(track);

  /* S1 .. S9: comments, hints, jokes, aso */
  for (int i = 0; i < 9; i++) {
    (void) read_str(fin);
  }

  int32_t tcount = gbfgetint32(fin);
  int datum = 118;
  if (tcount > 0) {
    datum = read_datum(fin);
    if (version == 8) {
      gbfread(buf, 1, 4, fin);
      int len = gbfgetint16(fin);
      gbfseek(fin, len, SEEK_CUR);
    }
  }

  while (tcount > 0) {
    tcount--;

    if (version == 8) {
      datum = read_datum(fin);
    }

    auto* wpt = new Waypoint;

    wpt->latitude = gbfgetdbl(fin);
    wpt->longitude = gbfgetdbl(fin);
    wpt->altitude = gbfgetdbl(fin);

    if (datum < 0) {
      fatal(MYNAME ": Invalid datum %d found", datum);
    }
    convert_datum(wpt, datum);

    track_add_wpt(track, wpt);

    if (version == 8) {
      gbfseek(fin, 34, SEEK_CUR);  /* skip unknown 34 bytes */
    }
  }

  if (version == 8) {

    int i = gbfgetint16(fin);
    i = gbfgetc(fin);
    if (i == 0) {
      return;
    }

    gbfungetc(i, fin);
    datum = read_datum(fin);

    (void) gbfgetint16(fin);
    (void) gbfgetint32(fin);

    gbfread(buf, 1, 9, fin);
    if (strncmp(buf, "CWayPoint", 9) != 0) {
      warning(MYNAME ": Unsupported waypoint structure!\n");
      return;
    }

    while (! gbfeof(fin)) {
      i = gbfgetc(fin);
      if (i == 0) {
        break;
      }

      gbfungetc(i, fin);
      datum = read_datum(fin);

      auto* wpt = new Waypoint;

      wpt->latitude = gbfgetdbl(fin);
      wpt->longitude = gbfgetdbl(fin);
      wpt->altitude = gbfgetdbl(fin);

      gbfseek(fin, 36, SEEK_CUR);	/* skip unknown 36 bytes */

      wpt->notes = read_str(fin);
      wpt->description = read_str(fin);
      (void) gbfgetint16(fin);

      waypt_add(wpt);
    }
    return;
  }

  int32_t wcount = gbfgetint32(fin);
  if (wcount == 0) {
    return;
  }

  datum = read_datum(fin);

  while (wcount > 0) {
    wcount--;

    auto* wpt = new Waypoint;

    wpt->latitude = gbfgetdbl(fin);
    wpt->longitude = gbfgetdbl(fin);
    wpt->altitude = gbfgetdbl(fin);

    convert_datum(wpt, datum);

    int32_t namect = gbfgetint32(fin);

    // variants of shortname

    for (int32_t i = 0; i < namect; i++) {
      auto name = read_str(fin);
      if (!name.isEmpty()) {
        switch (i) {
        case 0:
          wpt->description = name;
          break;
        case 1:
          wpt->shortname = name;
          break;
        }
      }
    }

    waypt_add(wpt);
  }
}


#if !ZLIB_INHIBITED

static int
inflate_buff(const char* buff, const size_t size, char** out_buff)
{
  z_stream strm;
  char out[DEFLATE_BUFF_SIZE];
  char* cout = nullptr;
  uint32_t bytes = 0;

  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  strm.avail_in = 0;
  strm.next_in = Z_NULL;

  int res = inflateInit(&strm);
  if (res != Z_OK) {
    return res;
  }

  strm.avail_in = size;
  strm.next_in = (Bytef*)buff;

  do {
    strm.avail_out = DEFLATE_BUFF_SIZE;
    strm.next_out = (Bytef*)out;
    res = inflate(&strm, Z_NO_FLUSH);

    switch (res) {
    case Z_NEED_DICT:
      res = Z_DATA_ERROR;
      [[fallthrough]];
    case Z_DATA_ERROR:
    case Z_MEM_ERROR:
      (void)inflateEnd(&strm);
      return res;
    }
    uint32_t have = DEFLATE_BUFF_SIZE - strm.avail_out;
    if (have > 0) {
      cout = (char*) xrealloc(cout, bytes + have);
      memcpy(cout+bytes, out, have);
      bytes+=have;
    }
  } while (strm.avail_out == 0);

  *out_buff = cout;
  return res;
}


static void
read_CXMLSafe()
{
  char* xmlstr = nullptr;

  xmlbin = nullptr;
  xmlbinsize = 0;

  xml_init(fin->name, tlog3a_xgcb_map, nullptr);
  xml_read();
  xml_deinit();

  if (xmlbin != nullptr) {
    inflate_buff(xmlbin, xmlbinsize, &xmlstr);
    xfree(xmlbin);

    xml_init(nullptr, tlog3b_xgcb_map, nullptr);
    xml_readstring(xmlstr);
    xml_deinit();

    xfree(xmlstr);
  }
}

#endif

static void
read_XML()
{
  xml_init(fin->name, tlog3b_xgcb_map, nullptr);
  xml_read();
  xml_deinit();
}

/*******************************************************************************
* %%%        global callbacks called by gpsbabel main process              %%% *
*******************************************************************************/

static void
dmtlog_rd_init(const QString& fname)
{
  fin = gbfopen_le(fname, "rb", MYNAME);

  xmlbin = nullptr;
  xmltrk = nullptr;
  xmlwpt = nullptr;
  xmlgrid = QString();
}

static void
dmtlog_rd_deinit()
{
  gbfclose(fin);
}

static void
dmtlog_read()
{
  switch (gbfgetuint32(fin)) {

  case 0x4FFFF:
    read_CTrackFile(4);
    break;

  case 0x8FFFF:
    read_CTrackFile(8);
    break;

  case 0x4d58433c:
#if !ZLIB_INHIBITED
    read_CXMLSafe();
#else
    fatal(MYNAME ": Zlib was not included in this build.\n");
#endif
    break;
  case 0x7254433c:
    read_XML();
    break;

  default:
    fatal(MYNAME ": Unknown or unsupported file type.\n");
  }
}

static void
dmtlog_wr_init(const QString& fname)
{
  fout = gbfopen_le(fname, "wb", MYNAME);
}

static void
dmtlog_wr_deinit()
{
  gbfclose(fout);
}

static void
write_header(const route_head* trk)
{
  const char ZERO = '\0';

  header_written = 1;

  int count = 0;
  if (trk != nullptr) {
    count = trk->rte_waypt_ct();
  }
  if (!trk || trk->rte_name.isEmpty()) {
    write_str("Name", fout);
  } else {
    write_str(trk->rte_name, fout);
  }

  // This fails for internationalization, but as this text is in the
  // file itself, it shouldn't be localized.
  QString cout = QString::number(count) + " trackpoints and " +
                 QString::number(waypt_count()) + " waypoints";
  write_str(cout, fout);

  for (int i = 3; i <= 8; i++) {
    gbfputc(ZERO, fout);
  }
  write_str("GPSBabel", fout);
  gbfputint32(count, fout);
  if (count > 0) {
    write_str("WGS84", fout);
    write_str("WGS84", fout);
  }
}

static void
track_hdr_cb(const route_head* trk)
{

  this_index++;
  if (this_index != track_index) {
    return;
  }
  write_header(trk);
}

static void
track_wpt_cb(const Waypoint* wpt)
{
  if (this_index != track_index) {
    return;
  }

  gbfputdbl(wpt->latitude, fout);
  gbfputdbl(wpt->longitude, fout);
  gbfputdbl(wpt->altitude != unknown_alt ? wpt->altitude : 0, fout);
}

static void
wpt_cb(const Waypoint* wpt)
{
  gbfputdbl(wpt->latitude, fout);
  gbfputdbl(wpt->longitude, fout);
  gbfputdbl(wpt->altitude != unknown_alt ? wpt->altitude : 0, fout);

  int names = 1;
  if (!wpt->description.isEmpty()) {
    names = 2;
  }
  gbfputint32(names, fout);
  if (names > 1) {
    write_str(wpt->description, fout);
  }
  write_str(wpt->shortname.isEmpty() ? "Name" : wpt->shortname, fout);
}

static void
dmtlog_write()
{
  track_index = atoi(opt_index);
  /* ... validate index */

  gbfputint32(0x4FFFF, fout);
  gbfputuint16(0x0A, fout);
  gbfputs("CTrackFile", fout);
  gbfputint32(4, fout);
  gbfputint32(1, fout);
  gbfputint32(0x100001, fout);
  gbfputuint32((uint32_t)gpsbabel_time, fout);

  header_written = 0;
  this_index = 0;
  track_disp_all(track_hdr_cb, nullptr, track_wpt_cb);
  if (!header_written) {
    write_header(nullptr);
  }
  gbfputint32(waypt_count(), fout);
  if (waypt_count() > 0) {
    write_str("WGS84", fout);
    write_str("WGS84", fout);
    waypt_disp_all(wpt_cb);
  }
}

/**************************************************************************/

ff_vecs_t dmtlog_vecs = {
  ff_type_file,
  {
    (ff_cap)(ff_cap_read | ff_cap_write)	/* waypoints */,
    (ff_cap)(ff_cap_read | ff_cap_write)	/* tracks */,
    ff_cap_none			/* routes */
  },
  dmtlog_rd_init,
  dmtlog_wr_init,
  dmtlog_rd_deinit,
  dmtlog_wr_deinit,
  dmtlog_read,
  dmtlog_write,
  nullptr,
  &dmtlog_args,
  CET_CHARSET_ASCII, 0
  , NULL_POS_OPS
};

/**************************************************************************/
