/*

	Support for "OpenStreetMap" data files (.xml)

	Copyright (C) 2008 Olaf Klein, o.b.klein@gpsbabel.org

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

#include <cstring>                      // for strlen, strchr, strcmp

#include <QtCore/QByteArray>            // for QByteArray
#include <QtCore/QHash>                 // for QHash
#include <QtCore/QLatin1String>         // for QLatin1String
#include <QtCore/QPair>                 // for QPair, operator==
#include <QtCore/QString>               // for QString, operator==, operator+
#include <QtCore/QStringRef>            // for QStringRef
#include <QtCore/QXmlStreamAttributes>  // for QXmlStreamAttributes
#include <QtCore/QtGlobal>              // for qPrintable, QAddConst<>::Type

#include "defs.h"
#include "osm.h"
#include "gbfile.h"                     // for gbfprintf, gbfclose, gbfopen
#include "src/core/datetime.h"          // for DateTime
#include "xmlgeneric.h"                 // for xg_string, build_xg_tag_map, xml_deinit, xml_init, xml_read


#define MYNAME "osm"

// Until c++17 we have to define odr-used constexpr static data members at namespace scope.
#if __cplusplus < 201703L
// MSVC 2015 will error with C2373 if the array length isn't explicitly included.
#if !defined(_MSC_VER) || (_MSC_VER >= 1910) /* !MSVC or MSVC 2017 or newer */
constexpr const char* OsmFormat::osm_features[];
constexpr OsmFormat::osm_icon_mapping_t OsmFormat::osm_icon_mappings[];
#else
constexpr const char* OsmFormat::osm_features[21];
constexpr OsmFormat::osm_icon_mapping_t OsmFormat::osm_icon_mappings[49];
#endif
#endif
/*******************************************************************************/
/*                                   READER                                    */
/*-----------------------------------------------------------------------------*/

void
OsmFormat::osm_features_init()
{
  /* the first of osm_features is a place holder */
  for (int i = 1; osm_features[i]; ++i) {
    keys.insert(osm_features[i], i);
  }

  for (int i = 0; osm_icon_mappings[i].value; ++i) {
    QPair<int, QString> key(osm_icon_mappings[i].key, osm_icon_mappings[i].value);
    values.insert(key, &osm_icon_mappings[i]);
  }
}

char
OsmFormat::osm_feature_ikey(const QString& key) const
{
  return keys.value(key, -1);
}

QString
OsmFormat::osm_feature_symbol(const int ikey, const char* value) const
{
  QPair<int, QString> key(ikey, value);

  QString result;
  if (values.contains(key)) {
    result = values.value(key)->icon;
  } else {
    result = QString("%1:%2").arg(osm_features[ikey], value);
  }
  return result;
}

char*
OsmFormat::osm_strip_html(const char* str)
{
  utf_string utf(true, str);
  return strip_html(&utf);	// util.cc
}

QString
OsmFormat::osm_strip_html(const QString& str) const
{
  char* r = osm_strip_html(CSTR(str));
  QString rv(r);
  xfree(r);
  return rv;
}

void
OsmFormat::osm_node_end(xg_string /*unused*/, const QXmlStreamAttributes* /*unused*/)
{
  if (wpt) {
    if (wpt->wpt_flags.fmt_use) {
      waypt_add(wpt);
    } else {
      delete wpt;
    }
    wpt = nullptr;
  }
}

void
OsmFormat::osm_node(xg_string /*unused*/, const QXmlStreamAttributes* attrv)
{
  wpt = new Waypoint;

  if (attrv->hasAttribute("id")) {
    QString atstr = attrv->value("id").toString();
    wpt->description =  "osm-id " + atstr;
    if (waypoints.contains(atstr)) {
      warning(MYNAME ": Duplicate osm-id %s!\n", qPrintable(atstr));
    } else {
      waypoints.insert(atstr, wpt);
      wpt->wpt_flags.fmt_use = 1;
    }
  }

  // if (attrv->hasAttribute("user")) ; // ignored

  if (attrv->hasAttribute("lat")) {
    wpt->latitude = attrv->value("lat").toString().toDouble();
  }
  if (attrv->hasAttribute("lon")) {
    wpt->longitude = attrv->value("lon").toString().toDouble();
  }

  if (attrv->hasAttribute("timestamp")) {
    QString ts = attrv->value("timestamp").toString();
    wpt->creation_time = xml_parse_time(ts);
  }
}

void
OsmFormat::osm_node_tag(xg_string /*unused*/, const QXmlStreamAttributes* attrv)
{
  QString key, value;
  signed char ikey;

  if (attrv->hasAttribute("k")) {
    key = attrv->value("k").toString();
  }
  if (attrv->hasAttribute("v")) {
    value = attrv->value("v").toString();
  }

  QString str = osm_strip_html(value);

  if (key == QLatin1String("name")) {
    if (wpt->shortname.isEmpty()) {
      wpt->shortname = str;
    }
  } else if (key == QLatin1String("name:en")) {
    wpt->shortname = str;
  } else if ((ikey = osm_feature_ikey(key)) >= 0) {
    wpt->icon_descr = osm_feature_symbol(ikey, CSTR(value));
  } else if (key == QLatin1String("note")) {
    if (wpt->notes.isEmpty()) {
      wpt->notes = str;
    } else {
      wpt->notes += "; ";
      wpt->notes += str;
    }
  } else if (key == QLatin1String("gps:hdop")) {
    wpt->hdop = str.toDouble();
  } else if (key == QLatin1String("gps:vdop")) {
    wpt->vdop = str.toDouble();
  } else if (key == QLatin1String("gps:pdop")) {
    wpt->pdop = str.toDouble();
  } else if (key == QLatin1String("gps:sat")) {
    wpt->sat = str.toDouble();
  } else if (key == QLatin1String("gps:fix")) {
    if (str == QLatin1String("2d")) {
      wpt->fix = fix_2d;
    } else if (str == QLatin1String("3d")) {
      wpt->fix = fix_3d;
    } else if (str == QLatin1String("dgps")) {
      wpt->fix = fix_dgps;
    } else if (str == QLatin1String("pps")) {
      wpt->fix = fix_pps;
    } else if (str == QLatin1String("none")) {
      wpt->fix = fix_none;
    }
  }
}

void
OsmFormat::osm_way(xg_string /*unused*/, const QXmlStreamAttributes* attrv)
{
  rte = new route_head;
  // create a wpt to represent the route center if it has a center tag
  wpt = new Waypoint;
  if (attrv->hasAttribute("id")) {
    rte->rte_desc =  "osm-id " + attrv->value("id").toString();
  }
}

void
OsmFormat::osm_way_nd(xg_string /*unused*/, const QXmlStreamAttributes* attrv)
{
  if (attrv->hasAttribute("ref")) {
    QString atstr = attrv->value("ref").toString();

    if (waypoints.contains(atstr)) {
      const Waypoint* ctmp = waypoints.value(atstr);
      auto* tmp = new Waypoint(*ctmp);
      route_add_wpt(rte, tmp);
    } else {
      warning(MYNAME ": Way reference id \"%s\" wasn't listed under nodes!\n", qPrintable(atstr));
    }
  }
}

void
OsmFormat::osm_way_tag(xg_string /*unused*/, const QXmlStreamAttributes* attrv)
{
  QString key, value;
  signed char ikey;

  if (attrv->hasAttribute("k")) {
    key = attrv->value("k").toString();
  }
  if (attrv->hasAttribute("v")) {
    value = attrv->value("v").toString();
  }

  QString str = osm_strip_html(value);

  if (key == QLatin1String("name")) {
    if (rte->rte_name.isEmpty()) {
      rte->rte_name = str;
      wpt->shortname = str;
    }
  } else if (key == QLatin1String("name:en")) {
    rte->rte_name = str;

    wpt->shortname = str;
    // The remaining cases only apply to the center node
  } else if ((ikey = osm_feature_ikey(key)) >= 0) {
    wpt->icon_descr = osm_feature_symbol(ikey, CSTR(value));
  } else if (key == "note") {
    if (wpt->notes.isEmpty()) {
      wpt->notes = str;
    } else {
      wpt->notes += "; ";
      wpt->notes += str;
    }
  }
}

void
OsmFormat::osm_way_center(xg_string /*unused*/, const QXmlStreamAttributes* attrv)
{
  wpt->wpt_flags.fmt_use = 1;

  if (attrv->hasAttribute("lat")) {
    wpt->latitude = attrv->value("lat").toString().toDouble();
  }
  if (attrv->hasAttribute("lon")) {
    wpt->longitude = attrv->value("lon").toString().toDouble();
  }
}

void
OsmFormat::osm_way_end(xg_string /*unused*/, const QXmlStreamAttributes* /*unused*/)
{
  if (rte) {
    route_add_head(rte);
    rte = nullptr;
  }

  if (wpt) {
    if (wpt->wpt_flags.fmt_use) {
      waypt_add(wpt);
    } else {
      delete wpt;
      wpt = nullptr;
    }
  }
}

void
OsmFormat::rd_init(const QString& fname)
{
  wpt = nullptr;
  rte = nullptr;

  waypoints.clear();
  if (keys.isEmpty()) {
    osm_features_init();
  }

  xml_init(fname, build_xg_tag_map(this, osm_map), nullptr, nullptr, nullptr, true);
}

void
OsmFormat::read()
{
  xml_read();
}

void
OsmFormat::rd_deinit()
{
  xml_deinit();
  waypoints.clear();
}

/*******************************************************************************/
/*                                   WRITER                                    */
/*-----------------------------------------------------------------------------*/

void
OsmFormat::osm_init_icons()
{
  if (!icons.isEmpty()) {
    return;
  }

  for (int i = 0; osm_icon_mappings[i].value; ++i) {
    icons.insert(osm_icon_mappings[i].icon, &osm_icon_mappings[i]);
  }
}

void
OsmFormat::osm_write_tag(const QString& key, const QString& value) const
{
  if (!value.isEmpty()) {
    char* str = xml_entitize(CSTR(value));
    gbfprintf(fout, "    <tag k='%s' v='%s'/>\n", CSTR(key), str);
    xfree(str);
  }
}

void
OsmFormat::osm_disp_feature(const Waypoint* waypoint) const
{
  if (icons.contains(waypoint->icon_descr)) {
    const osm_icon_mapping_t* map = icons.value(waypoint->icon_descr);
    osm_write_tag(osm_features[map->key], map->value);
  }
}

void
OsmFormat::osm_write_opt_tag(const char* atag)
{
  char* cin;

  if (!atag) {
    return;
  }

  char* tag = cin = xstrdup(atag);
  char* ce = cin + strlen(cin);

  while (cin < ce) {
    char* sc, *dp;

    if ((sc = strchr(cin, ';'))) {
      *sc = '\0';
    }

    if ((dp = strchr(cin, ':'))) {
      *dp++ = '\0';
      osm_write_tag(cin, dp);
    }
    cin += strlen(cin) + 1;
  }

  xfree(tag);
}

void
OsmFormat::osm_release_ids(const Waypoint* waypoint)
{
  if (waypoint && waypoint->extra_data) {
    delete static_cast<int*>(waypoint->extra_data);
    const_cast<Waypoint*>(waypoint)->extra_data = nullptr;
  }
}

QString
OsmFormat::osm_name_from_wpt(const Waypoint* waypoint)
{
  QString name = QString("%1\01%2\01%3")
                 .arg(waypoint->shortname)
                 .arg(waypoint->latitude)
                 .arg(waypoint->longitude);
  return name;
}

void
OsmFormat::osm_waypt_disp(const Waypoint* waypoint)
{
  QString name = osm_name_from_wpt(waypoint);

  if (waypoints.contains(name)) {
    return;
  }

  waypoints.insert(name, waypoint);

  auto* id = new int;
  *id = --node_id;
  const_cast<Waypoint*>(waypoint)->extra_data = id;

  gbfprintf(fout, "  <node id='%d' visible='true' lat='%0.7f' lon='%0.7f'", *id, waypoint->latitude, waypoint->longitude);
  if (waypoint->creation_time.isValid()) {
    QString time_string = waypoint->CreationTimeXML();
    gbfprintf(fout, " timestamp='%s'", qPrintable(time_string));
  }
  gbfprintf(fout, ">\n");

  if (waypoint->hdop) {
    gbfprintf(fout, "    <tag k='gps:hdop' v='%f' />\n", waypoint->hdop);
  }
  if (waypoint->vdop) {
    gbfprintf(fout, "    <tag k='gps:vdop' v='%f' />\n", waypoint->vdop);
  }
  if (waypoint->pdop) {
    gbfprintf(fout, "    <tag k='gps:pdop' v='%f' />\n", waypoint->pdop);
  }
  if (waypoint->sat > 0) {
    gbfprintf(fout, "    <tag k='gps:sat' v='%d' />\n", waypoint->sat);
  }

  switch (waypoint->fix) {
  case fix_2d:
    gbfprintf(fout, "    <tag k='gps:fix' v='2d' />\n");
    break;
  case fix_3d:
    gbfprintf(fout, "    <tag k='gps:fix' v='3d' />\n");
    break;
  case fix_dgps:
    gbfprintf(fout, "    <tag k='gps:fix' v='dgps' />\n");
    break;
  case fix_pps:
    gbfprintf(fout, "    <tag k='gps:fix' v='pps' />\n");
    break;
  case fix_none:
    gbfprintf(fout, "    <tag k='gps:fix' v='none' />\n");
    break;
  case fix_unknown:
  default:
    break;
  }

  if (strlen(created_by) !=0) {
    gbfprintf(fout, "    <tag k='created_by' v='%s",created_by);
    if (!gpsbabel_testmode())
      if (strcmp("GPSBabel",created_by)==0) {
        gbfprintf(fout, "-%s", gpsbabel_version);
      }
    gbfprintf(fout, "'/>\n");
  }

  osm_write_tag("name", waypoint->shortname);
  osm_write_tag("note", waypoint->notes.isEmpty() ? waypoint->description : waypoint->notes);
  if (!waypoint->icon_descr.isNull()) {
    osm_disp_feature(waypoint);
  }

  osm_write_opt_tag(opt_tagnd);

  gbfprintf(fout, "  </node>\n");
}

void
OsmFormat::osm_rte_disp_head(const route_head* route)
{
  skip_rte = route->rte_waypt_ct <= 0;

  if (skip_rte) {
    return;
  }

  gbfprintf(fout, "  <way id='%d' visible='true'>\n", --node_id);
}

void
OsmFormat::osm_rtept_disp(const Waypoint* wpt_ref) const
{
  QString name = osm_name_from_wpt(wpt_ref);

  if (skip_rte) {
    return;
  }

  if (waypoints.contains(name)) {
    const Waypoint* waypoint = waypoints.value(name);
    auto* id = static_cast<int*>(waypoint->extra_data);
    gbfprintf(fout, "    <nd ref='%d'/>\n", *id);
  }
}

void
OsmFormat::osm_rte_disp_trail(const route_head* route)
{
  if (skip_rte) {
    return;
  }

  if (strlen(created_by) !=0) {
    gbfprintf(fout, "    <tag k='created_by' v='%s",created_by);
    if (!gpsbabel_testmode())
      if (strcmp("GPSBabel",created_by)==0) {
        gbfprintf(fout, "-%s", gpsbabel_version);
      }
    gbfprintf(fout, "'/>\n");
  }

  osm_write_tag("name", route->rte_name);
  osm_write_tag("note", route->rte_desc);

  if (opt_tag && (case_ignore_strncmp(opt_tag, "tagnd", 5) != 0)) {
    osm_write_opt_tag(opt_tag);
  }

  gbfprintf(fout, "  </way>\n");
}

/*-----------------------------------------------------------------------------*/

void
OsmFormat::wr_init(const QString& fname)
{
  fout = gbfopen(fname, "w", MYNAME);

  osm_init_icons();
  waypoints.clear();
  node_id = 0;
}

void
OsmFormat::write()
{
  gbfprintf(fout, "<?xml version='1.0' encoding='UTF-8'?>\n");
  gbfprintf(fout, "<osm version='0.6' generator='GPSBabel");
  if (!gpsbabel_testmode()) {
    gbfprintf(fout, "-%s", gpsbabel_version);
  }
  gbfprintf(fout, "'>\n");

  auto osm_waypt_disp_lambda = [this](const Waypoint* waypointp)->void {
    osm_waypt_disp(waypointp);
  };
  waypt_disp_all(osm_waypt_disp_lambda);
  route_disp_all(nullptr, nullptr, osm_waypt_disp_lambda);
  track_disp_all(nullptr, nullptr, osm_waypt_disp_lambda);

  auto osm_rte_disp_head_lambda = [this](const route_head* rte)->void {
    osm_rte_disp_head(rte);
  };
  auto osm_rte_disp_trail_lambda = [this](const route_head* rte)->void {
    osm_rte_disp_trail(rte);
  };
  auto osm_rtept_disp_lambda = [this](const Waypoint* waypointp)->void {
    osm_rtept_disp(waypointp);
  };
  route_disp_all(osm_rte_disp_head_lambda, osm_rte_disp_trail_lambda, osm_rtept_disp_lambda);
  track_disp_all(osm_rte_disp_head_lambda, osm_rte_disp_trail_lambda, osm_rtept_disp_lambda);

  gbfprintf(fout, "</osm>\n");
}

void
OsmFormat::wr_deinit()
{
  gbfclose(fout);

  auto osm_release_ids_lambda = [](const Waypoint* waypointp)->void {
    osm_release_ids(waypointp);
  };
  waypt_disp_all(osm_release_ids_lambda);
  route_disp_all(nullptr, nullptr, osm_release_ids_lambda);
  track_disp_all(nullptr, nullptr, osm_release_ids_lambda);

  waypoints.clear();
}

void
OsmFormat::exit()
{
  keys.clear();
  values.clear();
  icons.clear();
}

/*-----------------------------------------------------------------------------*/
