/*

        Support for Swiss Map # (.xol) format

        Copyright (C) 2007 Olaf Klein

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
#include "garmin_tables.h"
#include "jeeps/gpsmath.h"
#include "src/core/file.h"
#include "src/core/xmlstreamwriter.h"
#include "xmlgeneric.h"

#include <QXmlStreamAttributes>
#include <QXmlStreamWriter>

static Waypoint* wpt_;
static route_head* trk_;
static bounds all_bounds;
static short_handle short_h;

static gpsbabel::File* oqfile;
static gpsbabel::XmlStreamWriter* writer;

static QVector<arglist_t> xol_args = {};

#define MYNAME "xol"

static xg_callback xol_shape, xol_shape_end;
static xg_callback xol_waypt, xol_overlay;

#define XOL "/overlays/overlay"

static xg_tag_mapping xol_map[] = {
    { xol_overlay, cb_start, XOL },
    { xol_shape, cb_start, XOL "/shapes/*shape" },
    { xol_shape_end, cb_end, XOL "/shapes/*shape" },
    { xol_waypt, cb_start, XOL "/shapes/shape/*points/point" },
    { nullptr, (xg_cb_type)0, nullptr} };

static void xol_overlay(xg_string, const QXmlStreamAttributes* attrv) {
  if (attrv->hasAttribute("version")) {
    if (attrv->value("version") != u"1.0") {
      fatal(MYNAME ": Unsupported version %s.\n",
            qPrintable(attrv->value("version").toString()));
    }
  }
}

static void xol_shape(xg_string, const QXmlStreamAttributes* attrv) {
  if (attrv->hasAttribute("type")) {
    if (attrv->value("type") == u"waypoint") {
      wpt_ = new Waypoint;
    } else if (attrv->value("type") == u"polyline") {
      trk_ = new route_head;
      track_add_head(trk_);
    }
  }

  if (attrv->hasAttribute("name")) {
    if (wpt_) {
      wpt_->shortname = attrv->value("name").toString();
    } else if (trk_) {
      trk_->rte_name = attrv->value("name").toString();
    }
  }

  if (wpt_) {
    if (attrv->hasAttribute("comment")) {
      wpt_->notes = attrv->value("comment").toString();
    }

    if (attrv->hasAttribute("alt")) {
      wpt_->altitude = attrv->value("alt").toDouble();
    }

    if (attrv->hasAttribute("timestamp")) {
      wpt_->creation_time = xml_parse_time(
          attrv->value("timestamp").toString());
    }

    if (attrv->hasAttribute("icon")) {
      wpt_->icon_descr = attrv->value("icon").toString();
    }
  }
}

static void xol_shape_end(xg_string, const QXmlStreamAttributes*) {
  if (wpt_) {
    if (trk_) {
      track_add_wpt(trk_, wpt_);
    } else {
      waypt_add(wpt_);
    }
    wpt_ = nullptr;
  } else if (trk_) {
    if (trk_->rte_waypt_ct() == 0) {
      track_del_head(trk_);
    }
    trk_ = nullptr;
  }
}

static void xol_waypt(xg_string, const QXmlStreamAttributes* attrv) {
  int x = 0, y = 0;

  if (attrv->hasAttribute("y")) {
    y = attrv->value("y").toInt();
  }

  if (attrv->hasAttribute("x")) {
    x = attrv->value("x").toInt();
  }

  GPS_Math_Swiss_EN_To_WGS84(x, y, &wpt_->latitude, &wpt_->longitude);
}

static void xol_rd_init(const QString& fname) {
  trk_ = nullptr;
  wpt_ = nullptr;

  xml_init(fname, xol_map, nullptr);
}

static void xol_read() { xml_read(); }

static void xol_rd_deinit() { xml_deinit(); }

/* writer */

static void xol_fatal_outside(const Waypoint* waypoint) {
  fatal(MYNAME ": %s (%s) is outside of convertible area \"%s\"!\n",
        waypoint->shortname.isEmpty() ? "Waypoint" : qPrintable(waypoint->shortname),
        pretty_deg_format(waypoint->latitude, waypoint->longitude, 'd', nullptr, 0),
        gt_get_mps_grid_longname(grid_swiss, MYNAME));
}

static void xol_waypt_bound_calc(const Waypoint* waypoint) {
  waypt_add_to_bounds(&all_bounds, waypoint);
}

static void xol_wr_init(const QString& fname) {
  oqfile = new gpsbabel::File(fname);
  oqfile->open(QIODevice::WriteOnly | QIODevice::Text);

  writer = new gpsbabel::XmlStreamWriter(oqfile);
  writer->setAutoFormattingIndent(2);
  writer->writeStartDocument();

  waypt_init_bounds(&all_bounds);
  short_h = mkshort_new_handle();

  setshort_length(short_h, 1024); /* ??? */
  setshort_badchars(short_h, "\r\n\t");
  setshort_mustupper(short_h, 0);
  setshort_mustuniq(short_h, 1);
  setshort_whitespace_ok(short_h, 1);
  setshort_repeating_whitespace_ok(short_h, 1);
  setshort_defname(short_h, "Waypoint");
}

static void xol_wr_deinit() {
  mkshort_del_handle(&short_h);
  writer->writeEndDocument();
  delete writer;
  writer = nullptr;

  oqfile->close();
  delete oqfile;
  oqfile = nullptr;
}

static void xol_waypt_disp_cb(const Waypoint* wpt) {
  double x, y;

  QString name = wpt->shortname;
  if (name.isEmpty() || global_opts.synthesize_shortnames) {
    name = mkshort_from_wpt(short_h, wpt);
  } else {
    name = mkshort(short_h, name);
  }

  if (!GPS_Math_WGS84_To_Swiss_EN(wpt->latitude, wpt->longitude, &x, &y)) {
    xol_fatal_outside(wpt);
  }
  writer->writeStartElement(QStringLiteral("shape"));
  writer->writeAttribute(QStringLiteral("type"), QStringLiteral("waypoint"));
  writer->writeAttribute(QStringLiteral("name"), name);
  writer->writeAttribute(QStringLiteral("comment"), wpt->notes);
  writer->writeAttribute(QStringLiteral("icon"), wpt->icon_descr);

  if (wpt->creation_time.isValid()) {
    writer->writeAttribute(QStringLiteral("timestamp"), wpt->CreationTimeXML());
  }
  if (wpt->altitude != unknown_alt) {
    writer->writeAttribute(QStringLiteral("alt"), QString::number(wpt->altitude, 'f', 6));
  }
  writer->writeStartElement(QStringLiteral("points"));
  writer->writeStartElement(QStringLiteral("point"));
  writer->writeAttribute(QStringLiteral("x"), QString::number(x));
  writer->writeAttribute(QStringLiteral("y"), QString::number(y));
  writer->writeEndElement();  // point
  writer->writeEndElement();  // points
  writer->writeEndElement();  // shape
}

static void xol_track_hdr_disp_cb(const route_head*) {
  writer->writeStartElement(QStringLiteral("shape"));
  writer->writeAttribute(QStringLiteral("type"), QStringLiteral("polyline"));
  writer->writeAttribute(QStringLiteral("lineSize"), QStringLiteral("3"));
  writer->writeAttribute(QStringLiteral("lineColor"), QStringLiteral("#e60000"));
  writer->writeAttribute(QStringLiteral("lineStyle"), QStringLiteral("solid"));
  writer->writeStartElement(QStringLiteral("waypoints"));
}

static void xol_track_tlr_disp_cb(const route_head*) {
  writer->writeEndElement();  // waypoints
  writer->writeEndElement();  // shape
}

static void xol_trkpt_disp_cb(const Waypoint* wpt) {
  double x, y;

  if (!GPS_Math_WGS84_To_Swiss_EN(wpt->latitude, wpt->longitude, &x, &y)) {
    xol_fatal_outside(wpt);
  }

  writer->writeStartElement(QStringLiteral("shape"));
  writer->writeAttribute(QStringLiteral("type"), QStringLiteral("waypoint"));
  if (wpt->creation_time.isValid()) {
    writer->writeAttribute(QStringLiteral("timestamp"), wpt->CreationTimeXML());
  }
  if (wpt->altitude != unknown_alt) {
    writer->writeAttribute(QStringLiteral("alt"), QString::number(wpt->altitude, 'f'));
  }

  writer->writeStartElement(QStringLiteral("points"));
  writer->writeStartElement(QStringLiteral("point"));
  writer->writeAttribute(QStringLiteral("x"), QString::number(x));
  writer->writeAttribute(QStringLiteral("y"), QString::number(y));
  writer->writeEndElement();  // point
  writer->writeEndElement();  // points
  writer->writeEndElement();  // shape
}

static void xol_write() {
  double x, y;

  waypt_disp_all(xol_waypt_bound_calc);
  track_disp_all(nullptr, nullptr, xol_waypt_bound_calc);

  if (!waypt_bounds_valid(&all_bounds)) {
    fatal(MYNAME ": No data available!\n");
  }

  if (!GPS_Math_WGS84_To_Swiss_EN((all_bounds.min_lat + all_bounds.max_lat) / 2,
                                  (all_bounds.min_lon + all_bounds.max_lon) / 2,
                                  &x, &y)) {
    fatal(MYNAME
          ": At least one point is outside of convertible area \"%s\"!\n",
          gt_get_mps_grid_longname(grid_swiss, MYNAME));
  }

  writer->setAutoFormatting(true);
  writer->writeStartElement(QStringLiteral("overlays"));
  writer->writeStartElement(QStringLiteral("overlay"));
  writer->writeAttribute(QStringLiteral("version"), QStringLiteral("1.0"));
  writer->writeStartElement(QStringLiteral("center"));
  writer->writeAttribute(QStringLiteral("x"), QString::number(x));
  writer->writeAttribute(QStringLiteral("y"), QString::number(y));
  writer->writeEndElement();  // center
  writer->writeStartElement(QStringLiteral("shapes"));

  waypt_disp_all(xol_waypt_disp_cb);
  track_disp_all(xol_track_hdr_disp_cb, xol_track_tlr_disp_cb,
                 xol_trkpt_disp_cb);

  writer->writeEndElement();  // shapes
  writer->writeEndElement();  // overlay
}

ff_vecs_t xol_vecs = {ff_type_file,
                      {(ff_cap)(ff_cap_read | ff_cap_write), /* waypoints */
                       (ff_cap)(ff_cap_read | ff_cap_write), /* tracks */
                       ff_cap_none},                         /* routes */
                      xol_rd_init,
                      xol_wr_init,
                      xol_rd_deinit,
                      xol_wr_deinit,
                      xol_read,
                      xol_write,
                      nullptr,
                      &xol_args,
                      CET_CHARSET_UTF8,
                      0  , NULL_POS_OPS
};
