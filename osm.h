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
#ifndef OSM_H_INCLUDED_
#define OSM_H_INCLUDED_

#include <QtCore/QHash>                 // for QHash
#include <QtCore/QList>                 // for QList
#include <QtCore/QPair>                 // for QPair
#include <QtCore/QString>               // for QString
#include <QtCore/QVector>               // for QVector
#include <QtCore/QXmlStreamAttributes>  // for QXmlStreamAttributes

#include "defs.h"
#include "format.h"                     // for Format
#include "gbfile.h"                     // for gbfile
#include "xmlgeneric.h"                 // for xg_functor_map_entry, cb_start, cb_end, xg_string


class OsmFormat : public Format
{
public:
  /* Member Functions */

  QVector<arglist_t>* get_args() override
  {
    return &osm_args;
  }

  ff_type get_type() const override
  {
    return ff_type_file;
  }

  QVector<ff_cap> get_cap() const override
  {
    return {
      (ff_cap)(ff_cap_read | ff_cap_write)	/* waypoints */,
      ff_cap_write 			/* tracks */,
      (ff_cap)(ff_cap_read | ff_cap_write) 	/* routes */,
    };
  }

  QString get_encode() const override
  {
    return CET_CHARSET_UTF8;
  }

  int get_fixed_encode() const override
  {
    return 0;
  }

  void rd_init(const QString& fname) override;
  void read() override;
  void rd_deinit() override;
  void wr_init(const QString& fname) override;
  void write() override;
  void wr_deinit() override;
  void exit() override;

private:
  /* Types */

  struct osm_icon_mapping_t {
    int key;
    const char* value;
    const char* icon;
  };

  /* Constants */

  // MSVC 2015 will error with C2373 if the array length isn't explicitly included.
#if !defined(_MSC_VER) || (_MSC_VER >= 1910) /* !MSVC or MSVC 2017 or newer */
  static constexpr const char* osm_features[] = {
#else
  static constexpr const char* osm_features[21] = {
#endif
    "- dummy -",	/*  0 */
    "aeroway",	/*  1 */
    "amenity",	/*  2 */
    "building",	/*  3 */
    "cycleway",	/*  4 */
    "railway",	/*  5 */
    "highway",	/*  6 */
    "historic",	/*  7 */
    "landuse",	/*  8 */
    "leisure",	/*  9 */
    "man_made",	/* 10 */
    "military",	/* 11 */
    "natural",	/* 12 */
    "place",	/* 13 */
    "power",	/* 14 */
    "shop",		/* 15 */
    "sport",	/* 16 */
    "tourism",	/* 17 */
    "waterway",	/* 18 */
    "aerialway",	/* 19 */
    nullptr
  };

  /* based on <http://wiki.openstreetmap.org/index.php/Map_Features> */

  // MSVC 2015 will error with C2373 if the array length isn't explicitly included.
#if !defined(_MSC_VER) || (_MSC_VER >= 1910) /* !MSVC or MSVC 2017 or newer */	
  static constexpr osm_icon_mapping_t osm_icon_mappings[] = {
#else
  static constexpr osm_icon_mapping_t osm_icon_mappings[49] = {
#endif

    /* cycleway ...*/

    /* highway ...*/

//	{ 6, "mini_roundabout",		"?" },
//	{ 6, "stop",			"?" },
//	{ 6, "traffic_signals",		"?" },
//	{ 6, "crossing",		"?" },
//	{ 6, "gate",			"?" },
//	{ 6, "stile",			"?" },
//	{ 6, "cattle_grid",		"?" },
//	{ 6, "toll_booth",		"?" },
//	{ 6, "incline",			"?" },
//	{ 6, "incline_steep",		"?" },
//	{ 6, "viaduct",			"?" },
//	{ 6, "motorway_junction",	"?" },
//	{ 6, "services",		"?" },
//	{ 6, "ford",			"?" },
//	{ 6, "bus_stop",		"?" },
//	{ 6, "turning_circle",		"?" },
//	{ 6, "User Defined",		"?" },

    /* waterway ... */

    { 18, "dock",			"Dock" },
//	{ 18, "lock_gate",		"?" },
//	{ 18, "turning_point",		"?" },
//	{ 18, "aqueduct",		"?" },
//	{ 18, "boatyard",		"?" },
//	{ 18, "water_point",		"?" },
//	{ 18, "waste_disposal",		"?" },
//	{ 18, "mooring",		"?" },
//	{ 18, "weir",			"?" },
//	{ 18, "User Defined",		"?" },

    /* railway ... */

//	{ 5, "station",			"?" },
//	{ 5, "halt",			"?" },
//	{ 5, "tram_stop",		"?" },
//	{ 5, "viaduct",			"?" },
    { 5, "crossing",		"Crossing" },
//	{ 5, "level_crossing",		"?" },
//	{ 5, "subway_entrance",		"?" },
//	{ 5, "turntable",		"?" },
//	{ 5, "User Defined",		"?" },

    /* aeroway ... */

    { 1, "aerodrome",		"Airport" },
    { 1, "terminal",		"Airport" },
    { 1, "helipad",			"Heliport" },
//	{ 1, "User Defined",		"?" },

    /* aerialway ... */

//	{ 19, "User Defined",		"?" },

    /* power ... */

//	{ 14, "tower",			"?" },
//	{ 14, "sub_station",		"?" },
//	{ 14, "generator",		"?" },

    /* man_made ... */

//	{ 10, "works",			"?" },
//	{ 10, "beacon",			"?" },
//	{ 10, "survey_point",		"?" },
//	{ 10, "power_wind",		"?" },
//	{ 10, "power_hydro",		"?" },
//	{ 10, "power_fossil",		"?" },
//	{ 10, "power_nuclear",		"?" },
//	{ 10, "tower",			"?" },
//	{ 10, "water_tower",		"?" },
//	{ 10, "gasometer",		"?" },
//	{ 10, "reservoir_covered",	"?" },
//	{ 10, "lighthouse",		"?" },
//	{ 10, "windmill",		"?" },
//	{ 10, "wastewater_plant",	"?" },
//	{ 10, "crane",			"?" },
//	{ 10, "User Defined",		"?" },

    /* building ... */

    { 3, "yes",			"Building" },
//	{ 3, "User Defined",		"?" },

    /* leisure ... */

//	{ 9, "sports_centre",		"?" },
    { 9, "golf_course",		"Golf Course" },
    { 9, "stadium",			"Stadium" },
//	{ 9, "track",			"?" },
//	{ 9, "pitch",			"?" },
//	{ 9, "water_park",		"?" },
    { 9, "marina",			"Marina" },
//	{ 9, "slipway",			"?" },
    { 9, "fishing",			"Fishing Area" },
//	{ 9, "nature_reserve",		"?" },
    { 9, "park",			"Park" },
//	{ 9, "playground",		"?" },
//	{ 9, "garden",			"?" },
//	{ 9, "common",			"?" },
//	{ 9, "User Defined",		"?" },

    /* amenity ... */

    { 2, "pub",			"Bar" },
//	{ 2, "biergarten",		"?" },
    { 2, "nightclub",		"Bar" },
//	{ 2, "cafe",			"?" },
    { 2, "restaurant",		"Restaurant" },
    { 2, "fast_food",		"Fast Food" },
    { 2, "parking",			"Parking Area" },
//	{ 2, "bicycle_parking",		"?" },
//	{ 2, "bicycle_rental",		"?" },
    { 2, "car_rental",		"Car Rental" },
//	{ 2, "car_sharing",		"?" },
//	{ 2, "taxi",			"?" },
    { 2, "fuel",			"Gas Station" },
    { 2, "telephone",		"Telephone" },
    { 2, "toilets",			"Restroom" },
//	{ 2, "recycling",		"?" },
//	{ 2, "public_building",		"?" },
    { 2, "townhall",		"City Hall" },
//	{ 2, "place_of_worship",	"?" },
//	{ 2, "grave_yard",		"?" },
    { 2, "post_office",		"Post Office" },
//	{ 2, "post_box",		"?" },
    { 2, "school",			"School" },
//	{ 2, "university",		"?" },
//	{ 2, "college",			"?" },
    { 2, "pharmacy",		"Pharmacy" },
    { 2, "hospital",		"Medical Facility" },
//	{ 2, "library",			"?" },
    { 2, "police",			"Police Station" },
//	{ 2, "fire_station",		"?" },
//	{ 2, "bus_station",		"?" },
//	{ 2, "theatre",			"?" },
//	{ 2, "cinema",			"?" },
//	{ 2, "arts_centre",		"?" },
//	{ 2, "courthouse",		"?" },
//	{ 2, "prison",			"?" },
    { 2, "bank",			"Bank" },
//	{ 2, "bureau_de_change",	"?" },
//	{ 2, "atm",			"?" },
//	{ 2, "fountain",		"?" },
//	{ 2, "User Defined",		"?" },

    /* shop ... */

//	{ 15, "supermarket",		"?" },
    { 15, "convenience",		"Convenience Store" },
//	{ 15, "butcher",		"?" },
//	{ 15, "bicycle",		"?" },
//	{ 15, "doityourself",		"?" },
//	{ 15, "dry_cleaning",		"?" },
//	{ 15, "laundry",		"?" },
//	{ 15, "outdoor",		"?" },
//	{ 15, "kiosk",			"?" },
//	{ 15, "User Defined",		"?" },

    /* tourism ... */

    { 17, "information",		"Information" },
    { 17, "hotel",			"Hotel" },
    { 17, "motel",			"Lodging" },
    { 17, "guest_house",		"Lodging" },
    { 17, "hostel",			"Lodging" },
    { 17, "camp_site",		"Campground" },
    { 17, "caravan_site",		"RV Park" },
    { 17, "picnic_site",		"Picnic Area" },
    { 17, "viewpoint",		"Scenic Area" },
//	{ 17, "theme_park",		"?" },
//	{ 17, "attraction",		"?" },
    { 17, "zoo",			"Zoo" },
//	{ 17, "artwork",		"?" },
    { 17, "museum",			"Museum" },
//	{ 17, "User Defined",		"?" },

    /* historic ... */

//	{ 7, "castle",			"?" },
//	{ 7, "monument",		"?" },
//	{ 7, "memorial",		"?" },
//	{ 7, "archaeological_site",	"?" },
//	{ 7, "ruins",			"?" },
//	{ 7, "battlefield",		"?" },
//	{ 7, "User Defined",		"?" },

    /* landuse ... */

//	{ 8, "farm",			"?" },
//	{ 8, "quarry",			"?" },
//	{ 8, "landfill",		"?" },
//	{ 8, "basin",			"?" },
//	{ 8, "reservoir",		"?" },
    { 8, "forest",			"Forest" },
//	{ 8, "allotments",		"?" },
//	{ 8, "residential",		"?" },
//	{ 8, "retail",			"?" },
//	{ 8, "commercial",		"?" },
//	{ 8, "industrial",		"?" },
//	{ 8, "brownfield",		"?" },
//	{ 8, "greenfield",		"?" },
//	{ 8, "railway",			"?" },
//	{ 8, "construction",		"?" },
    { 8, "military",		"Military" },
    { 8, "cemetery",		"Cemetery" },
//	{ 8, "village_green",		"?" },
//	{ 8, "recreation_ground",	"?" },
//	{ 8, "User Defined",		"?" },

    /* military ... */

//	{ 11, "airfield",		"?" },
//	{ 11, "bunker",			"?" },
//	{ 11, "barracks",		"?" },
//	{ 11, "danger_area",		"?" },
//	{ 11, "range",			"?" },
//	{ 11, "naval_base",		"?" },
//	{ 11, "User Defined",		"?" },

    /* natural ... */

//	{ 12, "spring",			"?" },
//	{ 12, "peak",			"?" },
//	{ 12, "glacier",		"?" },
//	{ 12, "volcano",		"?" },
//	{ 12, "cliff",			"?" },
//	{ 12, "scree",			"?" },
//	{ 12, "scrub",			"?" },
//	{ 12, "fell",			"?" },
//	{ 12, "heath",			"?" },
//	{ 12, "wood",			"?" },
//	{ 12, "marsh",			"?" },
//	{ 12, "water",			"?" },
//	{ 12, "coastline",		"?" },
//	{ 12, "mud",			"?" },
    { 12, "beach",			"Beach" },
//	{ 12, "bay",			"?" },
//	{ 12, "land",			"?" },
//	{ 12, "cave_entrance",		"?" },
//	{ 12, "User Defined",		"?" },

    /* sport ... */

//	{ 16, "10pin",			"?" },
//	{ 16, "athletics",		"?" },
//	{ 16, "australian_football",	"?" },
//	{ 16, "baseball",		"?" },
//	{ 16, "basketball",		"?" },
//	{ 16, "boules",			"?" },
//	{ 16, "bowls",			"?" },
//	{ 16, "climbing",		"?" },
//	{ 16, "cricket",		"?" },
//	{ 16, "cricket_nets",		"?" },
//	{ 16, "croquet",		"?" },
//	{ 16, "cycling",		"?" },
//	{ 16, "dog_racing",		"?" },
//	{ 16, "equestrian",		"?" },
//	{ 16, "football",		"?" },
//	{ 16, "golf",			"?" },
//	{ 16, "gymnastics",		"?" },
//	{ 16, "hockey",			"?" },
//	{ 16, "horse_racing",		"?" },
//	{ 16, "motor",			"?" },
//	{ 16, "multi",			"?" },
//	{ 16, "pelota",			"?" },
//	{ 16, "racquet",		"?" },
//	{ 16, "rugby",			"?" },
//	{ 16, "skating",		"?" },
//	{ 16, "skateboard",		"?" },
//	{ 16, "soccer",			"?" },
    { 16, "swimming",		"Swimming Area" },
    { 16, "skiing",			"Skiing Area" },
//	{ 16, "table_tennis",		"?" },
//	{ 16, "tennis",			"?" },
//	{ 16, "orienteering",		"?" },
//	{ 16, "User Defined",		"?" },

    /* place ... */

//	{ 13, "continent",		"?" },
//	{ 13, "country",		"?" },
//	{ 13, "state",			"?" },
//	{ 13, "region",			"?" },
//	{ 13, "county",			"?" },
    { 13, "city",			"City (Large)" },
    { 13, "town",			"City (Medium)" },
    { 13, "village",		"City (Small)" },
//	{ 13, "hamlet",			"?" },
//	{ 13, "suburb",			"?" },
//	{ 13, "locality",		"?" },
//	{ 13, "island",			"?" },
//	{ 13, "User Defined",		"?" },

    { -1, nullptr, nullptr }
  };

  /* Member Functions */

  void osm_features_init();
  char osm_feature_ikey(const QString& key) const;
  QString osm_feature_symbol(int ikey, const char* value) const;
  static char* osm_strip_html(const char* str);
  QString osm_strip_html(const QString& str) const;
  void osm_node_end(xg_string /* unused */, const QXmlStreamAttributes* /* unused */);
  void osm_node(xg_string /* unused */, const QXmlStreamAttributes* attrv);
  void osm_node_tag(xg_string /* unused */, const QXmlStreamAttributes* attrv);
  void osm_way(xg_string /* unused */, const QXmlStreamAttributes* attrv);
  void osm_way_nd(xg_string /* unused */, const QXmlStreamAttributes* attrv);
  void osm_way_tag(xg_string /* unused */, const QXmlStreamAttributes* attrv);
  void osm_way_center(xg_string /* unused */, const QXmlStreamAttributes* attrv);
  void osm_way_end(xg_string /* unused */, const QXmlStreamAttributes* /* unused */);
  void osm_init_icons();
  void osm_write_tag(const QString& key, const QString& value) const;
  void osm_disp_feature(const Waypoint* waypoint) const;
  void osm_write_opt_tag(const char* atag);
  static void osm_release_ids(const Waypoint* waypoint);
  static QString osm_name_from_wpt(const Waypoint* waypoint);
  void osm_waypt_disp(const Waypoint* waypoint);
  void osm_rte_disp_head(const route_head* route);
  void osm_rtept_disp(const Waypoint* wpt_ref) const;
  void osm_rte_disp_trail(const route_head* route);

  /* Data Members */

  char* opt_tag{};
  char* opt_tagnd{};
  char* created_by{};

  QVector<arglist_t> osm_args = {
    { "tag", &opt_tag, 	"Write additional way tag key/value pairs", nullptr, ARGTYPE_STRING, ARG_NOMINMAX, nullptr},
    { "tagnd", &opt_tagnd,	"Write additional node tag key/value pairs", nullptr, ARGTYPE_STRING, ARG_NOMINMAX, nullptr },
    { "created_by", &created_by, "Use this value as custom created_by value","GPSBabel", ARGTYPE_STRING, ARG_NOMINMAX, nullptr },
  };

  QHash<QString, const Waypoint*> waypoints;

  QHash<QString, int> keys;
  QHash<QPair<int, QString>, const osm_icon_mapping_t*> values;
  QHash<QString, const osm_icon_mapping_t*> icons;

  gbfile* fout{};
  int node_id{};
  int skip_rte{};

  route_head* rte{};
  Waypoint* wpt{};

  QList<xg_functor_map_entry<OsmFormat>> osm_map = {
    {&OsmFormat::osm_node,	cb_start,	"/osm/node"},
    {&OsmFormat::osm_node_tag,	cb_start,	"/osm/node/tag"},
    {&OsmFormat::osm_node_end,	cb_end,		"/osm/node"},
    {&OsmFormat::osm_way,	cb_start,	"/osm/way"},
    {&OsmFormat::osm_way_nd,	cb_start,	"/osm/way/nd"},
    {&OsmFormat::osm_way_tag,	cb_start,	"/osm/way/tag"},
    {&OsmFormat::osm_way_center,	cb_start,	"/osm/way/center"},
    {&OsmFormat::osm_way_end,	cb_end,		"/osm/way"}
  };
};
#endif // OSM_H_INCLUDED_
