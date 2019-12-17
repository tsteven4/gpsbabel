/*
    Describe vectors containing file operations.

    Copyright (C) 2002, 2004, 2005, 2006, 2007 Robert Lipe, robertlipe+source@gpsbabel.org

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

#include <QtCore/QByteArray>    // for QByteArray
#include <QtCore/QString>       // for QString
#include <QtCore/QStringList>   // for QStringList
#include <QtCore/QVector>       // for QVector<>::iterator, QVector
#include <QtCore/Qt>            // for CaseInsensitive
#include <QtCore/QtGlobal>      // for qPrintable

#include <algorithm>            // for sort
#include <cassert>              // for assert
#include <cctype>               // for isdigit
#include <cstdio>               // for printf, putchar, sscanf, size_t
#include <cstdint>

#include "defs.h"
#include "format.h"             // for Format
#include "gbversion.h"          // for WEB_DOC_DIR
#include "gpx.h"                // for GpxFormat
#include "inifile.h"            // for inifile_readstr
#include "legacyformat.h"       // for LegacyFormat
#include "src/core/logging.h"   // for Warning
#include "xcsv.h"               // for XcsvFile, xcsv_file, xcsv_read_internal_style, xcsv_setup_internal_style


#define MYNAME "vecs"

#if CSVFMTS_ENABLED
extern ff_vecs_t xcsv_vecs;
#endif // CSVFMTS_ENABLED
extern ff_vecs_t geo_vecs;
extern ff_vecs_t mag_svecs;
extern ff_vecs_t mag_fvecs;
extern ff_vecs_t magX_fvecs;
extern ff_vecs_t garmin_vecs;
extern ff_vecs_t gdb_vecs;
extern ff_vecs_t mapsend_vecs;
extern ff_vecs_t mps_vecs;
extern ff_vecs_t nmea_vecs;
extern ff_vecs_t ozi_vecs;
extern ff_vecs_t pcx_vecs;
extern ff_vecs_t kml_vecs;
#if MAXIMAL_ENABLED
extern ff_vecs_t gpsutil_vecs;
extern ff_vecs_t lowranceusr_vecs;
extern ff_vecs_t holux_vecs;
extern ff_vecs_t tpg_vecs;
extern ff_vecs_t tpo2_vecs;
extern ff_vecs_t tpo3_vecs;
extern ff_vecs_t tmpro_vecs;
extern ff_vecs_t tiger_vecs;
extern ff_vecs_t easygps_vecs;
extern ff_vecs_t saroute_vecs;
extern ff_vecs_t navicache_vecs;
extern ff_vecs_t psit_vecs;
#if SHAPELIB_ENABLED
extern ff_vecs_t shape_vecs;
#endif
extern ff_vecs_t gpl_vecs;
extern ff_vecs_t text_vecs;
extern ff_vecs_t html_vecs;
extern ff_vecs_t netstumbler_vecs;
extern ff_vecs_t igc_vecs;
extern ff_vecs_t brauniger_iq_vecs;
extern ff_vecs_t mtk_vecs;
extern ff_vecs_t mtk_fvecs;
extern ff_vecs_t mtk_m241_vecs;
extern ff_vecs_t mtk_m241_fvecs;
extern ff_vecs_t mtk_locus_vecs;
#endif // MAXIMAL_ENABLED
extern ff_vecs_t wbt_svecs;
#if MAXIMAL_ENABLED
extern ff_vecs_t vpl_vecs;
extern ff_vecs_t wbt_fvecs;
//extern ff_vecs_t wbt_fvecs;
extern ff_vecs_t hiketech_vecs;
extern ff_vecs_t glogbook_vecs;
extern ff_vecs_t vcf_vecs;
extern ff_vecs_t google_dir_vecs;
extern ff_vecs_t maggeo_vecs;
extern ff_vecs_t an1_vecs;
extern ff_vecs_t tomtom_vecs;
extern ff_vecs_t tef_xml_vecs;
extern ff_vecs_t vitosmt_vecs;
extern ff_vecs_t wfff_xml_vecs;
extern ff_vecs_t bcr_vecs;
extern ff_vecs_t ignr_vecs;
#if CSVFMTS_ENABLED
extern ff_vecs_t stmsdf_vecs;
#endif // CSVFMTS_ENABLED
#if CSVFMTS_ENABLED
extern ff_vecs_t stmwpp_vecs;
#endif // CSVFMTS_ENABLED
extern ff_vecs_t cst_vecs;
extern ff_vecs_t nmn4_vecs;
#if CSVFMTS_ENABLED
extern ff_vecs_t compegps_vecs;
#endif // CSVFMTS_ENABLED
extern ff_vecs_t yahoo_vecs;
extern ff_vecs_t unicsv_vecs;
extern ff_vecs_t gtm_vecs;
extern ff_vecs_t gpssim_vecs;
#if CSVFMTS_ENABLED
extern ff_vecs_t garmin_txt_vecs;
#endif // CSVFMTS_ENABLED
extern ff_vecs_t gtc_vecs;
extern ff_vecs_t dmtlog_vecs;
extern ff_vecs_t raymarine_vecs;
extern ff_vecs_t alanwpr_vecs;
extern ff_vecs_t alantrl_vecs;
extern ff_vecs_t vitovtt_vecs;
extern ff_vecs_t ggv_log_vecs;
#if CSVFMTS_ENABLED
extern ff_vecs_t g7towin_vecs;
#endif // CSVFMTS_ENABLED
extern ff_vecs_t garmin_gpi_vecs;
extern ff_vecs_t lmx_vecs;
extern ff_vecs_t random_vecs;
extern ff_vecs_t xol_vecs;
extern ff_vecs_t dg100_vecs;
extern ff_vecs_t dg200_vecs;
extern ff_vecs_t navilink_vecs;
extern ff_vecs_t ik3d_vecs;
extern ff_vecs_t osm_vecs;
extern ff_vecs_t destinator_poi_vecs;
extern ff_vecs_t destinator_itn_vecs;
extern ff_vecs_t destinator_trl_vecs;
extern ff_vecs_t exif_vecs;
extern ff_vecs_t vidaone_vecs;
extern ff_vecs_t igo8_vecs;
extern ff_vecs_t gopal_vecs;
extern ff_vecs_t humminbird_vecs;
extern ff_vecs_t humminbird_ht_vecs;
extern ff_vecs_t mapasia_tr7_vecs;
extern ff_vecs_t gnav_trl_vecs;
extern ff_vecs_t navitel_trk_vecs;
extern ff_vecs_t ggv_ovl_vecs;
#if CSVFMTS_ENABLED
extern ff_vecs_t jtr_vecs;
#endif // CSVFMTS_ENABLED
extern ff_vecs_t itracku_vecs;
extern ff_vecs_t itracku_fvecs;
extern ff_vecs_t sbp_vecs;
extern ff_vecs_t sbn_vecs;
extern ff_vecs_t mmo_vecs;
extern ff_vecs_t bushnell_vecs;
extern ff_vecs_t bushnell_trl_vecs;
extern ff_vecs_t skyforce_vecs;
extern ff_vecs_t pocketfms_bc_vecs;
extern ff_vecs_t pocketfms_fp_vecs;
extern ff_vecs_t pocketfms_wp_vecs;
extern ff_vecs_t v900_vecs;
extern ff_vecs_t ng_vecs;
extern ff_vecs_t enigma_vecs;
extern ff_vecs_t skytraq_vecs;
extern ff_vecs_t teletype_vecs;
extern ff_vecs_t skytraq_fvecs;
extern ff_vecs_t miniHomer_vecs;
extern ff_vecs_t jogmap_vecs;
extern ff_vecs_t wintec_tes_vecs;
extern ff_vecs_t subrip_vecs;
extern ff_vecs_t format_garmin_xt_vecs;
extern ff_vecs_t format_fit_vecs;
extern ff_vecs_t mapbar_track_vecs;
extern ff_vecs_t f90g_track_vecs;
extern ff_vecs_t mapfactor_vecs;
extern ff_vecs_t energympro_vecs;
extern ff_vecs_t mynav_vecs;
extern ff_vecs_t geojson_vecs;
extern ff_vecs_t ggv_bin_vecs;
extern ff_vecs_t globalsat_sport_vecs;
#endif // MAXIMAL_ENABLED

struct vecs_t {
  Format* vec;
  QString name;
  QString desc;
  QString extensions; // list of possible extensions separated by '/', first is output default for GUI.
  QString parent;
};

/*
 * Having these LegacyFormat instances be non-static data members
 * prevents the static initialization order fiasco because
 * the static vec that is used to construct a legacy format
 * instance is guaranteed to have already constructed when an instance
 * of this class is constructed.
 */
#if CSVFMTS_ENABLED
static LegacyFormat xcsv_fmt {&xcsv_vecs};
#endif // CSVFMTS_ENABLED
static LegacyFormat geo_fmt {&geo_vecs};
static GpxFormat gpx_fmt;
static LegacyFormat mag_sfmt {&mag_svecs};
static LegacyFormat mag_ffmt {&mag_fvecs};
static LegacyFormat magX_ffmt {&magX_fvecs};
static LegacyFormat garmin_fmt {&garmin_vecs};
static LegacyFormat gdb_fmt {&gdb_vecs};
static LegacyFormat mapsend_fmt {&mapsend_vecs};
static LegacyFormat mps_fmt {&mps_vecs};
static LegacyFormat nmea_fmt {&nmea_vecs};
static LegacyFormat ozi_fmt {&ozi_vecs};
static LegacyFormat pcx_fmt {&pcx_vecs};
static LegacyFormat kml_fmt {&kml_vecs};
#if MAXIMAL_ENABLED
static LegacyFormat gpsutil_fmt {&gpsutil_vecs};
static LegacyFormat lowranceusr_fmt {&lowranceusr_vecs};
static LegacyFormat holux_fmt {&holux_vecs};
static LegacyFormat tpg_fmt {&tpg_vecs};
static LegacyFormat tpo2_fmt {&tpo2_vecs};
static LegacyFormat tpo3_fmt {&tpo3_vecs};
static LegacyFormat tmpro_fmt {&tmpro_vecs};
static LegacyFormat tiger_fmt {&tiger_vecs};
static LegacyFormat easygps_fmt {&easygps_vecs};
static LegacyFormat saroute_fmt {&saroute_vecs};
static LegacyFormat navicache_fmt {&navicache_vecs};
static LegacyFormat psit_fmt {&psit_vecs};
#if SHAPELIB_ENABLED
static LegacyFormat shape_fmt {&shape_vecs};
#endif
static LegacyFormat gpl_fmt {&gpl_vecs};
static LegacyFormat text_fmt {&text_vecs};
static LegacyFormat html_fmt {&html_vecs};
static LegacyFormat netstumbler_fmt {&netstumbler_vecs};
static LegacyFormat igc_fmt {&igc_vecs};
static LegacyFormat brauniger_iq_fmt {&brauniger_iq_vecs};
static LegacyFormat mtk_fmt {&mtk_vecs};
static LegacyFormat mtk_ffmt {&mtk_fvecs};
static LegacyFormat mtk_m241_fmt {&mtk_m241_vecs};
static LegacyFormat mtk_m241_ffmt {&mtk_m241_fvecs};
static LegacyFormat mtk_locus_fmt {&mtk_locus_vecs};
#endif // MAXIMAL_ENABLED
static LegacyFormat wbt_sfmt {&wbt_svecs};
#if MAXIMAL_ENABLED
static LegacyFormat vpl_fmt {&vpl_vecs};
static LegacyFormat wbt_ffmt {&wbt_fvecs};
//static LegacyFormat wbt_ffmt {&wbt_fvecs};
static LegacyFormat hiketech_fmt {&hiketech_vecs};
static LegacyFormat glogbook_fmt {&glogbook_vecs};
static LegacyFormat vcf_fmt {&vcf_vecs};
static LegacyFormat google_dir_fmt {&google_dir_vecs};
static LegacyFormat maggeo_fmt {&maggeo_vecs};
static LegacyFormat an1_fmt {&an1_vecs};
static LegacyFormat tomtom_fmt {&tomtom_vecs};
static LegacyFormat tef_xml_fmt {&tef_xml_vecs};
static LegacyFormat vitosmt_fmt {&vitosmt_vecs};
static LegacyFormat wfff_xml_fmt {&wfff_xml_vecs};
static LegacyFormat bcr_fmt {&bcr_vecs};
static LegacyFormat ignr_fmt {&ignr_vecs};
#if CSVFMTS_ENABLED
static LegacyFormat stmsdf_fmt {&stmsdf_vecs};
#endif // CSVFMTS_ENABLED
#if CSVFMTS_ENABLED
static LegacyFormat stmwpp_fmt {&stmwpp_vecs};
#endif // CSVFMTS_ENABLED
static LegacyFormat cst_fmt {&cst_vecs};
static LegacyFormat nmn4_fmt {&nmn4_vecs};
#if CSVFMTS_ENABLED
static LegacyFormat compegps_fmt {&compegps_vecs};
#endif // CSVFMTS_ENABLED
static LegacyFormat yahoo_fmt {&yahoo_vecs};
static LegacyFormat unicsv_fmt {&unicsv_vecs};
static LegacyFormat gtm_fmt {&gtm_vecs};
static LegacyFormat gpssim_fmt {&gpssim_vecs};
#if CSVFMTS_ENABLED
static LegacyFormat garmin_txt_fmt {&garmin_txt_vecs};
#endif // CSVFMTS_ENABLED
static LegacyFormat gtc_fmt {&gtc_vecs};
static LegacyFormat dmtlog_fmt {&dmtlog_vecs};
static LegacyFormat raymarine_fmt {&raymarine_vecs};
static LegacyFormat alanwpr_fmt {&alanwpr_vecs};
static LegacyFormat alantrl_fmt {&alantrl_vecs};
static LegacyFormat vitovtt_fmt {&vitovtt_vecs};
static LegacyFormat ggv_log_fmt {&ggv_log_vecs};
#if CSVFMTS_ENABLED
static LegacyFormat g7towin_fmt {&g7towin_vecs};
#endif // CSVFMTS_ENABLED
static LegacyFormat garmin_gpi_fmt {&garmin_gpi_vecs};
static LegacyFormat lmx_fmt {&lmx_vecs};
static LegacyFormat random_fmt {&random_vecs};
static LegacyFormat xol_fmt {&xol_vecs};
static LegacyFormat dg100_fmt {&dg100_vecs};
static LegacyFormat dg200_fmt {&dg200_vecs};
static LegacyFormat navilink_fmt {&navilink_vecs};
static LegacyFormat ik3d_fmt {&ik3d_vecs};
static LegacyFormat osm_fmt {&osm_vecs};
static LegacyFormat destinator_poi_fmt {&destinator_poi_vecs};
static LegacyFormat destinator_itn_fmt {&destinator_itn_vecs};
static LegacyFormat destinator_trl_fmt {&destinator_trl_vecs};
static LegacyFormat exif_fmt {&exif_vecs};
static LegacyFormat vidaone_fmt {&vidaone_vecs};
static LegacyFormat igo8_fmt {&igo8_vecs};
static LegacyFormat gopal_fmt {&gopal_vecs};
static LegacyFormat humminbird_fmt {&humminbird_vecs};
static LegacyFormat humminbird_ht_fmt {&humminbird_ht_vecs};
static LegacyFormat mapasia_tr7_fmt {&mapasia_tr7_vecs};
static LegacyFormat gnav_trl_fmt {&gnav_trl_vecs};
static LegacyFormat navitel_trk_fmt {&navitel_trk_vecs};
static LegacyFormat ggv_ovl_fmt {&ggv_ovl_vecs};
#if CSVFMTS_ENABLED
static LegacyFormat jtr_fmt {&jtr_vecs};
#endif // CSVFMTS_ENABLED
static LegacyFormat itracku_fmt {&itracku_vecs};
static LegacyFormat itracku_ffmt {&itracku_fvecs};
static LegacyFormat sbp_fmt {&sbp_vecs};
static LegacyFormat sbn_fmt {&sbn_vecs};
static LegacyFormat mmo_fmt {&mmo_vecs};
static LegacyFormat bushnell_fmt {&bushnell_vecs};
static LegacyFormat bushnell_trl_fmt {&bushnell_trl_vecs};
static LegacyFormat skyforce_fmt {&skyforce_vecs};
static LegacyFormat pocketfms_bc_fmt {&pocketfms_bc_vecs};
static LegacyFormat pocketfms_fp_fmt {&pocketfms_fp_vecs};
static LegacyFormat pocketfms_wp_fmt {&pocketfms_wp_vecs};
static LegacyFormat v900_fmt {&v900_vecs};
static LegacyFormat ng_fmt {&ng_vecs};
static LegacyFormat enigma_fmt {&enigma_vecs};
static LegacyFormat skytraq_fmt {&skytraq_vecs};
static LegacyFormat teletype_fmt {&teletype_vecs};
static LegacyFormat skytraq_ffmt {&skytraq_fvecs};
static LegacyFormat miniHomer_fmt {&miniHomer_vecs};
static LegacyFormat jogmap_fmt {&jogmap_vecs};
static LegacyFormat wintec_tes_fmt {&wintec_tes_vecs};
static LegacyFormat subrip_fmt {&subrip_vecs};
static LegacyFormat format_garmin_xt_fmt {&format_garmin_xt_vecs};
static LegacyFormat format_fit_fmt {&format_fit_vecs};
static LegacyFormat mapbar_track_fmt {&mapbar_track_vecs};
static LegacyFormat f90g_track_fmt {&f90g_track_vecs};
static LegacyFormat mapfactor_fmt {&mapfactor_vecs};
static LegacyFormat energympro_fmt {&energympro_vecs};
static LegacyFormat mynav_fmt {&mynav_vecs};
static LegacyFormat geojson_fmt {&geojson_vecs};
static LegacyFormat ggv_bin_fmt {&ggv_bin_vecs};
static LegacyFormat globalsat_sport_fmt {&globalsat_sport_vecs};
#endif // MAXIMAL_ENABLED

static const QVector<vecs_t> vec_list {
#if CSVFMTS_ENABLED
  /* XCSV must be the first entry in this table. */
  {
    &xcsv_fmt,
    "xcsv",
    "? Character Separated Values",
    nullptr,
    nullptr,
  },
#endif
  {
    &geo_fmt,
    "geo",
    "Geocaching.com .loc",
    "loc",
    nullptr,
  },
  {
    &gpx_fmt,
    "gpx",
    "GPX XML",
    "gpx",
    nullptr,
  },
  {
    &mag_sfmt,
    "magellan",
    "Magellan serial protocol",
    nullptr,
    nullptr,
  },
  {
    &mag_ffmt,
    "magellan",
    "Magellan SD files (as for Meridian)",
    nullptr,
    nullptr,
  },
  {
    &magX_ffmt,
    "magellanx",
    "Magellan SD files (as for eXplorist)",
    "upt",
    nullptr,
  },
  {
    &garmin_fmt,
    "garmin",
    "Garmin serial/USB protocol",
    nullptr,
    nullptr,
  },
  {
    &gdb_fmt,
    "gdb",
    "Garmin MapSource - gdb",
    "gdb",
    nullptr,
  },
  {
    &mapsend_fmt,
    "mapsend",
    "Magellan Mapsend",
    nullptr,
    nullptr,
  },
  {
    &mps_fmt,
    "mapsource",
    "Garmin MapSource - mps",
    "mps",
    nullptr,
  },
  {
    &nmea_fmt,
    "nmea",
    "NMEA 0183 sentences",
    nullptr,
    nullptr,
  },
  {
    &ozi_fmt,
    "ozi",
    "OziExplorer",
    nullptr,
    nullptr,
  },
  {
    &pcx_fmt,
    "pcx",
    "Garmin PCX5",
    "pcx",
    nullptr,
  },
  {
    &kml_fmt,
    "kml",
    "Google Earth (Keyhole) Markup Language",
    "kml",
    nullptr,
  },
#if MAXIMAL_ENABLED
  {
    &gpsutil_fmt,
    "gpsutil",
    "gpsutil",
    nullptr,
    nullptr,
  },
  {
    &lowranceusr_fmt,
    "lowranceusr",
    "Lowrance USR",
    "usr",
    nullptr,
  },
  {
    &holux_fmt,
    "holux",
    "Holux (gm-100) .wpo Format",
    "wpo",
    nullptr,
  },
  {
    &tpg_fmt,
    "tpg",
    "National Geographic Topo .tpg (waypoints)",
    "tpg",
    nullptr,
  },
  {
    &tpo2_fmt,
    "tpo2",
    "National Geographic Topo 2.x .tpo",
    "tpo",
    nullptr,
  },
  {
    &tpo3_fmt,
    "tpo3",
    "National Geographic Topo 3.x/4.x .tpo",
    "tpo",
    nullptr,
  },
  {
    &tmpro_fmt,
    "tmpro",
    "TopoMapPro Places File",
    "tmpro",
    nullptr,
  },
  {
    &tiger_fmt,
    "tiger",
    "U.S. Census Bureau Tiger Mapping Service",
    nullptr,
    nullptr,
  },
  {
    &easygps_fmt,
    "easygps",
    "EasyGPS binary format",
    "loc",
    nullptr,
  },
  {
    &saroute_fmt,
    "saroute",
    "DeLorme Street Atlas Route",
    "anr",
    nullptr,
  },
  {
    &navicache_fmt,
    "navicache",
    "Navicache.com XML",
    nullptr,
    nullptr,
  },
  {	/* MRCB */
    &psit_fmt,
    "psitrex",
    "KuDaTa PsiTrex text",
    nullptr,
    nullptr,
  },
#if SHAPELIB_ENABLED
  {
    &shape_fmt,
    "shape",
    "ESRI shapefile",
    "shp",
    nullptr,
  },
#endif
  {
    &gpl_fmt,
    "gpl",
    "DeLorme GPL",
    "gpl",
    nullptr,
  },
  {
    &text_fmt,
    "text",
    "Textual Output",
    "txt",
    nullptr,
  },
  {
    &html_fmt,
    "html",
    "HTML Output",
    "html",
    nullptr,
  },
  {
    &netstumbler_fmt,
    "netstumbler",
    "NetStumbler Summary File (text)",
    nullptr,
    nullptr,
  },
  {
    &igc_fmt,
    "igc",
    "FAI/IGC Flight Recorder Data Format",
    nullptr,
    nullptr,
  },
  {
    &brauniger_iq_fmt,
    "baroiq",
    "Brauniger IQ Series Barograph Download",
    nullptr,
    nullptr,
  },
  {
    &mtk_fmt,
    "mtk",
    "MTK Logger (iBlue 747,Qstarz BT-1000,...) download",
    nullptr,
    nullptr,
  },
  {
    &mtk_ffmt,
    "mtk-bin",
    "MTK Logger (iBlue 747,...) Binary File Format",
    "bin",
    nullptr,
  },
  {
    &mtk_m241_fmt,
    "m241",
    "Holux M-241 (MTK based) download",
    nullptr,
    nullptr,
  },
  {
    &mtk_m241_ffmt,
    "m241-bin",
    "Holux M-241 (MTK based) Binary File Format",
    "bin",
    nullptr,
  },
  {
    &mtk_locus_fmt,
    "mtk_locus",
    "MediaTek Locus",
    nullptr,
    nullptr,
  },
#endif // MAXIMAL_ENABLED
  {
    &wbt_sfmt,
    "wbt",
    "Wintec WBT-100/200 GPS Download",
    nullptr,
    nullptr,
  },
#if MAXIMAL_ENABLED
  {
    &vpl_fmt,
    "vpl",
    "Honda/Acura Navigation System VP Log File Format",
    nullptr,
    nullptr,
  },
  {
    &wbt_ffmt,
    "wbt-bin",
    "Wintec WBT-100/200 Binary File Format",
    "bin",
    nullptr,
  },
  {
    &wbt_ffmt,
    "wbt-tk1",
    "Wintec WBT-201/G-Rays 2 Binary File Format",
    "tk1",
    nullptr,
  },
  {
    &hiketech_fmt,
    "hiketech",
    "HikeTech",
    "gps",
    nullptr,
  },
  {
    &glogbook_fmt,
    "glogbook",
    "Garmin Logbook XML",
    "xml",
    nullptr,
  },
  {
    &vcf_fmt,
    "vcard",
    "Vcard Output (for iPod)",
    "vcf",
    nullptr,
  },
  {
    &google_dir_fmt,
    "googledir",
    "Google Directions XML",
    "xml",
    nullptr,
  },
  {
    &maggeo_fmt,
    "maggeo",
    "Magellan Explorist Geocaching",
    "gs",
    nullptr,
  },
  {
    &an1_fmt,
    "an1",
    "DeLorme .an1 (drawing) file",
    "an1",
    nullptr,
  },
  {
    &tomtom_fmt,
    "tomtom",
    "TomTom POI file (.ov2)",
    "ov2",
    nullptr,
  },
  {
    &tef_xml_fmt,
    "tef",
    "Map&Guide 'TourExchangeFormat' XML",
    "xml",
    nullptr,
  },
  {
    &vitosmt_fmt,
    "vitosmt",
    "Vito Navigator II tracks",
    "smt",
    nullptr,
  },
  {
    &wfff_xml_fmt,
    "wfff",
    "WiFiFoFum 2.0 for PocketPC XML",
    "xml",
    nullptr,
  },
  {
    &bcr_fmt,
    "bcr",
    "Motorrad Routenplaner (Map&Guide) .bcr files",
    "bcr",
    nullptr,
  },
  {
    &ignr_fmt,
    "ignrando",
    "IGN Rando track files",
    "rdn",
    nullptr,
  },
#if CSVFMTS_ENABLED
  {
    &stmsdf_fmt,
    "stmsdf",
    "Suunto Trek Manager (STM) .sdf files",
    "sdf",
    nullptr,
  },
#endif
#if CSVFMTS_ENABLED
  {
    &stmwpp_fmt,
    "stmwpp",
    "Suunto Trek Manager (STM) WaypointPlus files",
    "txt",
    nullptr,
  },
#endif //  CSVFMTS_ENABLED
  {
    &cst_fmt,
    "cst",
    "CarteSurTable data file",
    "cst",
    nullptr,
  },
  {
    &nmn4_fmt,
    "nmn4",
    "Navigon Mobile Navigator .rte files",
    "rte",
    nullptr,
  },
#if CSVFMTS_ENABLED
  {
    &compegps_fmt,
    "compegps",
    "CompeGPS data files (.wpt/.trk/.rte)",
    nullptr,
    nullptr,
  },
#endif //CSVFMTS_ENABLED
  {
    &yahoo_fmt,
    "yahoo",
    "Yahoo Geocode API data",
    nullptr,
    nullptr,
  },
  {
    &unicsv_fmt,
    "unicsv",
    "Universal csv with field structure in first line",
    nullptr,
    nullptr,
  },
  {
    &gtm_fmt,
    "gtm",
    "GPS TrackMaker",
    "gtm",
    nullptr,
  },
  {
    &gpssim_fmt,
    "gpssim",
    "Franson GPSGate Simulation",
    "gpssim",
    nullptr,
  },
#if CSVFMTS_ENABLED
  {
    &garmin_txt_fmt,
    "garmin_txt",
    "Garmin MapSource - txt (tab delimited)",
    "txt",
    nullptr,
  },
#endif // CSVFMTS_ENABLED
  {
    &gtc_fmt,
    "gtrnctr",
    "Garmin Training Center (.tcx/.crs/.hst/.xml)",
    "tcx/crs/hst/xml",
    nullptr,
  },
  {
    &dmtlog_fmt,
    "dmtlog",
    "TrackLogs digital mapping (.trl)",
    "trl",
    nullptr,
  },
  {
    &raymarine_fmt,
    "raymarine",
    "Raymarine Waypoint File (.rwf)",
    "rwf",
    nullptr,
  },
  {
    &alanwpr_fmt,
    "alanwpr",
    "Alan Map500 waypoints and routes (.wpr)",
    "wpr",
    nullptr,
  },
  {
    &alantrl_fmt,
    "alantrl",
    "Alan Map500 tracklogs (.trl)",
    "trl",
    nullptr,
  },
  {
    &vitovtt_fmt,
    "vitovtt",
    "Vito SmartMap tracks (.vtt)",
    "vtt",
    nullptr,
  },
  {
    &ggv_log_fmt,
    "ggv_log",
    "Geogrid-Viewer tracklogs (.log)",
    "log",
    nullptr,
  },
#if CSVFMTS_ENABLED
  {
    &g7towin_fmt,
    "g7towin",
    "G7ToWin data files (.g7t)",
    "g7t",
    nullptr,
  },
#endif
  {
    &garmin_gpi_fmt,
    "garmin_gpi",
    "Garmin Points of Interest (.gpi)",
    "gpi",
    nullptr,
  },
  {
    &lmx_fmt,
    "lmx",
    "Nokia Landmark Exchange",
    nullptr,
    nullptr,
  },
  {
    &random_fmt,
    "random",
    "Internal GPS data generator",
    nullptr,
    nullptr,
  },
  {
    &xol_fmt,
    "xol",
    "Swiss Map 25/50/100 (.xol)",
    "xol",
    nullptr,
  },
  {
    &dg100_fmt,
    "dg-100",
    "GlobalSat DG-100/BT-335 Download",
    nullptr,
    nullptr,
  },
  {
    &dg200_fmt,
    "dg-200",
    "GlobalSat DG-200 Download",
    nullptr,
    nullptr,
  },
  {
    &navilink_fmt,
    "navilink",
    "NaviGPS GT-11/BGT-11 Download",
    nullptr,
    nullptr,
  },
  {
    &ik3d_fmt,
    "ik3d",
    "MagicMaps IK3D project file (.ikt)",
    "ikt",
    nullptr,
  },
  {
    &osm_fmt,
    "osm",
    "OpenStreetMap data files",
    "osm",
    nullptr,
  },
  {
    &destinator_poi_fmt,
    "destinator_poi",
    "Destinator Points of Interest (.dat)",
    "dat",
    nullptr,
  },
  {
    &destinator_itn_fmt,
    "destinator_itn",
    "Destinator Itineraries (.dat)",
    "dat",
    nullptr,
  },
  {
    &destinator_trl_fmt,
    "destinator_trl",
    "Destinator TrackLogs (.dat)",
    "dat",
    nullptr,
  },
  {
    &exif_fmt,
    "exif",
    "Embedded Exif-GPS data (.jpg)",
    "jpg",
    nullptr,
  },
  {
    &vidaone_fmt,
    "vidaone",
    "VidaOne GPS for Pocket PC (.gpb)",
    "gpb",
    nullptr,
  },
  {
    &igo8_fmt,
    "igo8",
    "IGO8 .trk",
    "trk",
    nullptr,
  },
  {
    &gopal_fmt,
    "gopal",
    "GoPal GPS track log (.trk)",
    "trk",
    nullptr,
  },
  {
    &humminbird_fmt,
    "humminbird",
    "Humminbird waypoints and routes (.hwr)",
    "hwr",
    nullptr,
  },
  {
    &humminbird_ht_fmt,
    "humminbird_ht",
    "Humminbird tracks (.ht)",
    "ht",
    nullptr,
  },
  {
    &mapasia_tr7_fmt,
    "mapasia_tr7",
    "MapAsia track file (.tr7)",
    "tr7",
    nullptr,
  },
  {
    &gnav_trl_fmt,
    "gnav_trl",
    "Google Navigator Tracklines (.trl)",
    "trl",
    nullptr,
  },
  {
    &navitel_trk_fmt,
    "navitel_trk",
    "Navitel binary track (.bin)",
    "bin",
    nullptr,
  },
  {
    &ggv_ovl_fmt,
    "ggv_ovl",
    "Geogrid-Viewer ascii overlay file (.ovl)",
    "ovl",
    nullptr,
  },
#if CSVFMTS_ENABLED
  {
    &jtr_fmt,
    "jtr",
    "Jelbert GeoTagger data file",
    "jtr",
    nullptr,
  },
#endif
  {
    &itracku_fmt,
    "itracku",
    "XAiOX iTrackU Logger",
    nullptr,
    nullptr,
  },
  {
    &itracku_ffmt,
    "itracku-bin",
    "XAiOX iTrackU Logger Binary File Format",
    "bin",
    nullptr,
  },
  {
    &sbp_fmt,
    "sbp",
    "NaviGPS GT-31/BGT-31 datalogger (.sbp)",
    "sbp",
    nullptr,
  },
  {
    &sbn_fmt,
    "sbn",
    "NaviGPS GT-31/BGT-31 SiRF binary logfile (.sbn)",
    "sbn",
    nullptr,
  },
  {
    &mmo_fmt,
    "mmo",
    "Memory-Map Navigator overlay files (.mmo)",
    "mmo",
    nullptr,
  },
  {
    &bushnell_fmt,
    "bushnell",
    "Bushnell GPS Waypoint file",
    "wpt",
    nullptr,
  },
  {
    &bushnell_trl_fmt,
    "bushnell_trl",
    "Bushnell GPS Trail file",
    "trl",
    nullptr,
  },
  {
    &skyforce_fmt,
    "skyforce",
    "Skymap / KMD150 ascii files",
    nullptr,
    nullptr,
  },
  {
    &pocketfms_bc_fmt,
    "pocketfms_bc",
    "PocketFMS breadcrumbs",
    nullptr,
    nullptr,
  },
  {
    &pocketfms_fp_fmt,
    "pocketfms_fp",
    "PocketFMS flightplan (.xml)",
    "xml",
    nullptr,
  },
  {
    &pocketfms_wp_fmt,
    "pocketfms_wp",
    "PocketFMS waypoints (.txt)",
    "txt",
    nullptr,
  },
  {
    &v900_fmt,
    "v900",
    "Columbus/Visiontac V900 files (.csv)",
    nullptr,
    nullptr,
  },
  {
    &ng_fmt,
    "naviguide",
    "Naviguide binary route file (.twl)",
    "twl",
    nullptr,
  },
  {
    &enigma_fmt,
    "enigma",
    "Enigma binary waypoint file (.ert)",
    "ert",
    nullptr,
  },
  {
    &skytraq_fmt,
    "skytraq",
    "SkyTraq Venus based loggers (download)",
    nullptr,
    nullptr,
  },
  {
    &teletype_fmt,
    "teletype",
    "Teletype [ Get Jonathon Johnson to describe",
    nullptr,
    nullptr,
  },
  {
    &skytraq_ffmt,
    "skytraq-bin",
    "SkyTraq Venus based loggers Binary File Format",
    "bin",
    nullptr,
  },
  {
    &miniHomer_fmt,
    "miniHomer",
    "MiniHomer, a skyTraq Venus 6 based logger (download tracks, waypoints and get/set POI)",
    nullptr,
    nullptr,
  },
  {
    &jogmap_fmt,
    "jogmap",
    "Jogmap.de XML format",
    "xml",
    nullptr,
  },
  {
    &wintec_tes_fmt,
    "wintec_tes",
    "Wintec TES file",
    "tes",
    nullptr,
  },
  {
    &subrip_fmt,
    "subrip",
    "SubRip subtitles for video mapping (.srt)",
    "srt",
    nullptr,
  },
  {
    &format_garmin_xt_fmt,
    "garmin_xt",
    "Mobile Garmin XT Track files",
    nullptr,
    nullptr,
  },
  {
    &format_fit_fmt,
    "garmin_fit",
    "Flexible and Interoperable Data Transfer (FIT) Activity file",
    "fit",
    nullptr,
  },
  {
    &mapbar_track_fmt,
    "mapbar",
    "Mapbar (China) navigation track for Sonim Xp3300",
    "trk",
    nullptr,
  },
  {
    &f90g_track_fmt,
    "f90g",
    "F90G Automobile DVR GPS log file",
    "map",
    nullptr,
  },
  {
    &mapfactor_fmt,
    "mapfactor",
    "Mapfactor Navigator",
    "xml",
    nullptr,
  },
  {
    &energympro_fmt,
    "energympro",
    "Energympro GPS training watch",
    "cpo",
    nullptr,
  },
  {
    &mynav_fmt,
    "mynav",
    "MyNav TRC format",
    "trc",
    nullptr,
  },
  {
    &geojson_fmt,
    "geojson",
    "GeoJson",
    "json",
    nullptr,
  },
  {
    &ggv_bin_fmt,
    "ggv_bin",
    "Geogrid-Viewer binary overlay file (.ovl)",
    "ovl",
    nullptr,
  },
  {
    &globalsat_sport_fmt,
    "globalsat",
    "GlobalSat GH625XT GPS training watch",
    nullptr,
    nullptr,
  }
#endif // MAXIMAL_ENABLED
};

/*
 * When we modify an element on the list we need to be careful
 * that we are not modifying a Qt COW copy.
 * Qt has an undocumented but public member function isDetached().
 * If the list is detached it implies it is not shared, then functions
 * then might detach, like the iterator begin which is implcitly used
 * in the range based for loop, won't cause a copy to be created.
 * We can make sure this is true for at least our regression cases
 * with assertions.
 * There is an odd situation that an empty QVector is not detached,
 * so we have to exclude this from the check.
 * The possibility of detachement is also why the type of element
 * on the list must be default constructable. This is why we have
 * to supply a default for any const members of arglist_t.  Without
 * the default the default constructor would be implicitly deleted.
 */

void init_vecs()
{
  for (const auto& vec : vec_list) {
    QVector<arglist_t>* args = vec.vec->get_args();
    if (args && !args->isEmpty()) {
      assert(args->isDetached());
      for (auto& arg : *args) {
        arg.argvalptr = nullptr;
        if (arg.argval) {
          *arg.argval = nullptr;
        }
      }
    }
  }
}

static int is_integer(const char* c)
{
  return isdigit(c[0]) || ((c[0] == '+' || c[0] == '-') && isdigit(c[1]));
}

void exit_vecs()
{
  for (const auto& vec : vec_list) {
    (vec.vec->exit)();
    QVector<arglist_t>* args = vec.vec->get_args();
    if (args && !args->isEmpty()) {
      assert(args->isDetached());
      for (auto& arg : *args) {
        if (arg.argvalptr) {
          xfree(arg.argvalptr);
          *arg.argval = arg.argvalptr = nullptr;
        }
      }
    }
  }
}

void assign_option(const QString& module, arglist_t* arg, const char* val)
{
  const char* c;

  if (arg->argval == nullptr) {
    fatal("%s: No local variable defined for option \"%s\"!", qPrintable(module), arg->argstring);
  }

  if (arg->argvalptr != nullptr) {
    xfree(arg->argvalptr);
    arg->argvalptr = nullptr;
  }
  if (arg->argval) {
    *arg->argval = nullptr;
  }

  if (val == nullptr) {
    return;
  }

  // Fixme - this is probably somewhere between wrong and less than great.  If you have an option "foo"
  // and want to set it to the value "foo", this code will prevent that from happening, but we seem to have
  // code all over the place that relies on this. :-/
  if (case_ignore_strcmp(val, arg->argstring) == 0) {
    c = "";
  } else {
    c = val;
  }

  switch (arg->argtype & ARGTYPE_TYPEMASK) {
  case ARGTYPE_INT:
    if (*c == '\0') {
      c = "0";
    } else {
      int test;
      is_fatal(1 != sscanf(c, "%d", &test),
               "%s: Invalid parameter value %s for option %s", qPrintable(module), val, arg->argstring);
    }
    break;
  case ARGTYPE_FLOAT:
    if (*c == '\0') {
      c = "0";
    } else {
      double test;
      is_fatal(1 != sscanf(c, "%lf", &test),
               "%s: Invalid parameter value %s for option %s", qPrintable(module), val, arg->argstring);
    }
    break;
  case ARGTYPE_BOOL:
    if (*c == '\0') {
      c = "1";
    } else {
      switch (*c) {
      case 'Y':
      case 'y':
        c = "1";
        break;
      case 'N':
      case 'n':
        c = "0";
        break;
      default:
        if (isdigit(*c)) {
          if (*c == '0') {
            c = "0";
          } else {
            c = "1";
          }
        } else {
          warning(MYNAME ": Invalid logical value '%s' (%s)!\n", c, qPrintable(module));
          c = "0";
        }
        break;
      }
    }
    break;
  }

  /* for bool options without default: don't set argval if "FALSE" */

  if (((arg->argtype & ARGTYPE_TYPEMASK) == ARGTYPE_BOOL) &&
      (*c == '0') && (arg->defaultvalue == nullptr)) {
    return;
  }
  *arg->argval = arg->argvalptr = xstrdup(c);
}

void disp_vec_options(const QString& vecname, const QVector<arglist_t>* args)
{
  if (args) {
    for (const auto& arg : *args) {
      if (*arg.argval && arg.argval) {
        printf("options: module/option=value: %s/%s=\"%s\"",
               qPrintable(vecname), arg.argstring, *arg.argval);
        if (arg.defaultvalue && (case_ignore_strcmp(arg.defaultvalue, *arg.argval) == 0)) {
          printf(" (=default)");
        }
        printf("\n");
      }
    }
  }
}

void validate_options(const QStringList& options, const QVector<arglist_t>* args, const QString& name)
{
  for (const auto& option : options) {
    const QString option_name = option.left(option.indexOf('='));
    bool valid = false;
    if (args) {
      for (const auto& arg : *args) {
        if (option_name.compare(arg.argstring, Qt::CaseInsensitive) == 0) {
          valid = true;
          break;
        }
      }
    }
    if (!valid) {
      warning("'%s' is an unknown option to %s.\n", qPrintable(option_name), qPrintable(name));
    }
  }
}

Format* find_vec(const QString& vecname)
{
  QStringList options = vecname.split(',');
  if (options.isEmpty()) {
    fatal("A format name is required.\n");
  }
  const QString svecname = options.takeFirst();

  for (const auto& vec : vec_list) {
    if (svecname.compare(vec.name, Qt::CaseInsensitive) != 0) {
      continue;
    }

    QVector<arglist_t>* args = vec.vec->get_args();

    validate_options(options, args, vec.name);

    if (args && !args->isEmpty()) {
      assert(args->isDetached());
      for (auto& arg : *args) {
        if (!options.isEmpty()) {
          const QString opt = get_option(options, arg.argstring);
          if (!opt.isNull()) {
            assign_option(vec.name, &arg, CSTR(opt));
            continue;
          }
        }
        QString qopt = inifile_readstr(global_opts.inifile, vec.name, arg.argstring);
        if (qopt.isNull()) {
          qopt = inifile_readstr(global_opts.inifile, "Common format settings", arg.argstring);
        }
        if (qopt.isNull()) {
          assign_option(vec.name, &arg, arg.defaultvalue);
        } else {
          assign_option(vec.name, &arg, CSTR(qopt));
        }
      }
    }

    if (global_opts.debug_level >= 1) {
      disp_vec_options(vec.name, args);
    }

#if CSVFMTS_ENABLED
    // xcsv_setup_internal_style( nullptr );
#endif // CSVFMTS_ENABLED
    vec.vec->set_name(vec.name);	/* needed for session information */
    return vec.vec;

  }

  /*
   * Didn't find it in the table of "real" file types, so plan B
   * is to search the list of xcsv styles.
   */
  for (const auto& svec : style_list) {
    if (svecname.compare(svec.name,  Qt::CaseInsensitive) != 0) {
      continue;
    }

    QVector<arglist_t>* xcsv_args = vec_list.at(0).vec->get_args();

    validate_options(options, xcsv_args, svec.name);

    if (xcsv_args && !xcsv_args->isEmpty()) {
      assert(xcsv_args->isDetached());
      for (auto& arg : *xcsv_args) {
        if (!options.isEmpty()) {
          const QString opt = get_option(options, arg.argstring);
          if (!opt.isNull()) {
            assign_option(svec.name, &arg, CSTR(opt));
            continue;
          }
        }
        QString qopt = inifile_readstr(global_opts.inifile, svec.name, arg.argstring);
        if (qopt.isNull()) {
          qopt = inifile_readstr(global_opts.inifile, "Common format settings", arg.argstring);
        }
        if (qopt.isNull()) {
          assign_option(svec.name, &arg, arg.defaultvalue);
        } else {
          assign_option(svec.name, &arg, CSTR(qopt));
        }
      }
    }

    if (global_opts.debug_level >= 1) {
      disp_vec_options(svec.name, xcsv_args);
    }
#if CSVFMTS_ENABLED
    xcsv_setup_internal_style(svec.style_buf);
#endif // CSVFMTS_ENABLED

    vec_list[0].vec->set_name(svec.name);	/* needed for session information */
    return vec_list[0].vec;
  }

  /*
   * Not found.
   */
  return nullptr;
}

/*
 * Find and return a specific argument in an arg list.
 * Modelled approximately after getenv.
 */
QString get_option(const QStringList& options, const char* argname)
{
  QString rval;

  for (const auto& option : options) {
    int split = option.indexOf('=');
    const QString option_name = option.left(split);
    if (option_name.compare(argname, Qt::CaseInsensitive) == 0) {
      /*
       * If we have something of the form "foo=bar"
       * return "bar".   Otherwise, we assume we have
       * simply "foo" so we return that.
       */
      if (split >= 0) { // we found an '='s.
        rval = option.mid(split + 1); // not null, possibly empty
        assert(!rval.isNull());
        break;
      } else {
        rval = option_name; // not null, possibly empty.
        assert(!rval.isNull());
        break;
      }
    }
  }
  return rval;
}

/*
 * Smoosh the vecs list and style lists together and sort them
 * alphabetically.  Returns an allocated copy of a style_vecs_array
 * that's populated and sorted.
 */
static QVector<vecs_t> sort_and_unify_vecs()
{
  QVector<vecs_t> svp;
  svp.reserve(vec_list.size() + style_list.size());

  /* Normal vecs are easy; populate the first part of the array. */
  for (const auto& vec : vec_list) {
    vecs_t uvec = vec;
    if (uvec.parent == nullptr) {
      uvec.parent = uvec.name;
    }
    svp.append(uvec);
  }

#if CSVFMTS_ENABLED
  /* The style formats are based on the xcsv format,
   * Make sure we know which entry in the vector list that is.
   */
  assert(vec_list.at(0).name == "xcsv");
  /* The style formats use a modified xcsv argument list that doesn't include
   * the option to set the style file.  Make sure we know which entry in
   * the argument list that is.
   */
  assert(case_ignore_strcmp(vec_list.at(0).vec->get_args()->at(0).helpstring,
                            "Full path to XCSV style file") == 0);
  /* Prepare a modified argument list for the style formats. */
  auto xcsv_args = new QVector<arglist_t>(*vec_list.at(0).vec->get_args()); /* LEAK */
  xcsv_args->removeFirst();

  /* Walk the style list, parse the entries, dummy up a "normal" vec */
  for (const auto& svec : style_list) {
    xcsv_read_internal_style(svec.style_buf);
    vecs_t uvec;
    uvec.name = svec.name;
    uvec.extensions = xcsv_file.extension;
    /* TODO: This needs to be reworked when xcsv isn't a LegacyFormat and
     * xcsv_vecs disappear.
     */
    auto ffvec = new ff_vecs_t(xcsv_vecs); /* Inherits xcsv opts */ /* LEAK */
    /* Reset file type to inherit ff_type from xcsv. */
    ffvec->type = xcsv_file.type;
    /* Skip over the first help entry for all but the
     * actual 'xcsv' format - so we don't expose the
     * 'Full path to XCSV style file' argument to any
     * GUIs for an internal format.
     */
    ffvec->args = xcsv_args;
    ffvec->cap.fill(ff_cap_none);
    switch (xcsv_file.datatype) {
    case unknown_gpsdata:
    case wptdata:
      ffvec->cap[ff_cap_rw_wpt] = (ff_cap)(ff_cap_read | ff_cap_write);
      break;
    case trkdata:
      ffvec->cap[ff_cap_rw_trk] = (ff_cap)(ff_cap_read | ff_cap_write);
      break;
    case rtedata:
      ffvec->cap[ff_cap_rw_rte] = (ff_cap)(ff_cap_read | ff_cap_write);
      break;
    default:
      ;
    }
    uvec.vec = new LegacyFormat(ffvec); /* LEAK */
    uvec.desc = xcsv_file.description;
    uvec.parent = "xcsv";
    svp.append(uvec);
  }
#endif // CSVFMTS_ENABLED

  /*
   *  Display the available formats in a format that's easy for humans to
   *  parse for help on available command line options.
   */
  auto alpha = [](const vecs_t& a, const vecs_t& b)->bool {
    return QString::compare(a.desc, b.desc, Qt::CaseInsensitive) < 0;
  };

  /* Now that we have everything in an array, alphabetize them */
  std::sort(svp.begin(), svp.end(), alpha);

  return svp;
}

#define VEC_FMT "	%-20.20s  %-.50s\n"

void disp_vecs()
{
  const auto svp = sort_and_unify_vecs();
  for (const auto& vec : svp) {
    if (vec.vec->get_type() == ff_type_internal)  {
      continue;
    }
    printf(VEC_FMT, qPrintable(vec.name), qPrintable(vec.desc));
    const QVector<arglist_t>* args = vec.vec->get_args();
    if (args) {
      for (const auto& arg : *args) {
        if (!(arg.argtype & ARGTYPE_HIDDEN))
          printf("	  %-18.18s    %s%-.50s %s\n",
                 arg.argstring,
                 (arg.argtype & ARGTYPE_TYPEMASK) ==
                 ARGTYPE_BOOL ? "(0/1) " : "",
                 arg.helpstring,
                 (arg.argtype & ARGTYPE_REQUIRED) ? "(required)" : "");
      }
    }
  }
}

void disp_vec(const QString& vecname)
{
  const auto svp = sort_and_unify_vecs();
  for (const auto& vec : svp) {
    if (vecname.compare(vec.name, Qt::CaseInsensitive) != 0)  {
      continue;
    }

    printf(VEC_FMT, qPrintable(vec.name), qPrintable(vec.desc));
    const QVector<arglist_t>* args = vec.vec->get_args();
    if (args) {
      for (const auto& arg : *args) {
        if (!(arg.argtype & ARGTYPE_HIDDEN))
          printf("	  %-18.18s    %s%-.50s %s\n",
                 arg.argstring,
                 (arg.argtype & ARGTYPE_TYPEMASK) ==
                 ARGTYPE_BOOL ? "(0/1) " : "",
                 arg.helpstring,
                 (arg.argtype & ARGTYPE_REQUIRED) ? "(required)" : "");
      }
    }
  }
}

/*
 * Additional information for V1.
 * Output format type at front of line.
 */
static void disp_v1(ff_type t)
{
  const char* tstring;

  switch (t) {
  case ff_type_file:
    tstring = "file";
    break;
  case ff_type_serial:
    tstring = "serial";
    break;
  case ff_type_internal:
    tstring = "internal";
    break;
  default:
    tstring = "unknown";
    break;
  }
  printf("%s\t", tstring);
}

static void disp_v2(const Format* v)
{
  for (auto& i : v->get_cap()) {
    putchar((i & ff_cap_read) ? 'r' : '-');
    putchar((i & ff_cap_write) ? 'w' : '-');
  }
  putchar('\t');
}

const char* name_option(uint32_t type)
{
  const char* at[] = {
    "unknown",
    "integer",
    "float",
    "string",
    "boolean",
    "file",
    "outfile"
  };

  if ((type & ARGTYPE_TYPEMASK) <= 6) {
    return at[type & ARGTYPE_TYPEMASK];
  }
  return at[0];
}

static void disp_help_url(const vecs_t& vec, const arglist_t* arg)
{
  printf("\t" WEB_DOC_DIR "/fmt_%s.html", CSTR(vec.name));
  if (arg) {
    printf("#fmt_%s_o_%s", CSTR(vec.name), arg->argstring);
  }
  printf("\n");
}


static void disp_v3(const vecs_t& vec)
{
  disp_help_url(vec, nullptr);
  const QVector<arglist_t>* args = vec.vec->get_args();
  if (args) {
    for (const auto& arg : *args) {
      if (!(arg.argtype & ARGTYPE_HIDDEN)) {
        printf("option\t%s\t%s\t%s\t%s\t%s\t%s\t%s",
               CSTR(vec.name),
               arg.argstring,
               arg.helpstring,
               name_option(arg.argtype),
               arg.defaultvalue ? arg.defaultvalue : "",
               arg.minvalue ? arg.minvalue : "",
               arg.maxvalue ? arg.maxvalue : "");
      }
      disp_help_url(vec, &arg);
      printf("\n");
    }
  }
}

/*
 *  Display the available formats in a format that's easy to machine
 *  parse.   Typically invoked by programs like graphical wrappers to
 *  determine what formats are supported.
 */
void disp_formats(int version)
{
  const auto svp = sort_and_unify_vecs();
  switch (version) {
  case 0:
  case 1:
  case 2:
  case 3:
    for (const auto& vec : svp) {
      /* Version 1 displays type at front of all types.
       * Version 0 skips internal types.
       */
      if (version > 0) {
        disp_v1(vec.vec->get_type());
      } else {
        if (vec.vec->get_type() == ff_type_internal) {
          continue;
        }
      }
      if (version >= 2) {
        disp_v2(vec.vec);
      }
      printf("%s\t%s\t%s%s%s\n", CSTR(vec.name),
             !vec.extensions.isEmpty() ? CSTR(vec.extensions) : "",
             CSTR(vec.desc),
             version >= 3 ? "\t" : "",
             version >= 3 ? CSTR(vec.parent) : "");
      if (version >= 3) {
        disp_v3(vec);
      }
    }
    break;
  default:
    ;
  }
}

//#define FIND_ALL_NULLPTR_ARGUMENTS
//#define FIND_ALL_EMPTY_ARGUMENT_LISTS

bool validate_args(const QString& name, const QVector<arglist_t>* args)
{
  bool ok = true;

#ifdef FIND_ALL_NULLPTR_ARGUMENTS
  if (args == nullptr) {
    Warning() << name << "Is passing nullptr for arguments.";
  }
#endif

  if (args != nullptr) {
#ifdef FIND_ALL_EMPTY_ARGUMENT_LISTS
    if (args->isEmpty()) {
      Warning() << name << "It isn't necessary to use an empty argument list, you can pass nullptr.";
    }
#endif
    for (const auto& arg : *args) {
      if (arg.argtype == ARGTYPE_INT) {
        if (arg.defaultvalue &&
            ! is_integer(arg.defaultvalue)) {
          Warning() << name << "Int option" << arg.argstring << "default value" << arg.defaultvalue << "is not an integer.";
          ok = false;
        }
        if (arg.minvalue &&
            ! is_integer(arg.minvalue)) {
          Warning() << name << "Int option" << arg.argstring << "minimum value" << arg.minvalue << "is not an integer.";
          ok = false;
        }
        if (arg.maxvalue &&
            ! is_integer(arg.maxvalue)) {
          Warning() << name << "Int option" << arg.argstring << "maximum value" << arg.maxvalue << "is not an integer.";
          ok = false;
        }
      }
    }
  }

  return ok;
}

static bool validate_vec(const vecs_t& vec)
{
  bool ok = validate_args(vec.name, vec.vec->get_args());

  return ok;
}

bool validate_formats()
{
  bool ok = true;

  for (const auto& vec : vec_list) {
    ok = validate_vec(vec) && ok;
  }

  return ok;
}
