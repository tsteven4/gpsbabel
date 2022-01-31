/*
    Communicate Thales/Magellan serial protocol.

    Copyright (C) 2002, 2003, 2004, 2005, 2006, 2007,
      2008, 2010  Robert Lipe, robertlipe+source@gpsbabel.org

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

#include "magproto.h"

#include <QByteArray>                // for QByteArray
#include <QDate>                     // for QDate
#include <QDateTime>                 // for QDateTime
#include <QDir>                      // for QDir, operator|, QDir::Files, QDir::Name, QDir::Readable
#include <QFileInfo>                 // for QFileInfo
#include <QFileInfoList>             // for QFileInfoList
#include <QLatin1String>             // for QLatin1String
#include <QList>                     // for QList
#include <QScopedPointer>            // for QScopedPointer
#include <QScopedPointerPodDeleter>  // for QScopedPointerPodDeleter
#include <QString>                   // for QString, operator+, operator==
#include <QStringList>               // for QStringList
#include <QTime>                     // for QTime
#include <Qt>                        // for CaseInsensitive, UTC
#include <QtGlobal>                  // for qAsConst, qPrintable, foreach

#include <cassert>                   // for assert
#include <cctype>                    // for isprint, toupper
#include <cmath>                     // for fabs, lround
#include <cstdio>                    // for sscanf, size_t
#include <cstdlib>                   // for atoi, atof, strtoul
#include <cstring>                   // for strchr, strncmp, strlen, memmove, strrchr, memset
#include <ctime>                     // for time_t, tm
#include <type_traits>               // for add_const<>::type

#include "defs.h"                    // for Waypoint, fatal, xasprintf, warning, xfree, global_options, global_opts, route_head, ddmm2degrees, gstrsub, setshort_length, xmalloc, xstrdup, CSTRc, current_time, get_cache_icon, get_filename, mkshort_new_handle, setshort_mustupper, setshort_whitespac...
#include "format.h"                  // for Format
#include "gbfile.h"                  // for gbfclose, gbfeof, gbfgets, gbfopen, gbfwrite
#include "gbser.h"                   // for gbser_deinit, gbser_init, gbser_is_serial, gbser_read_line, gbser_set_port, gbser_write, gbser_OK
#include "src/core/datetime.h"       // for DateTime
#include "vecs.h"                    // for Vecs


#define MYNAME "MAGPROTO"
#define MAXCMTCT 200

#define debug_serial  (global_opts.debug_level > 1)

/*
 *   For each receiver type, return a "cleansed" version of the string
 *   that's valid for a waypoint name or comment.   The string should be
 *   freed when you're done with it.
 */
QString
MagprotoBase::m315_cleanse(const char* istring)
{
  char* rstring = (char*) xmalloc(strlen(istring)+1);
  char* o;
  const char* i;
  static constexpr char m315_valid_chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789";
  for (o=rstring,i=istring; *i; i++) {
    if (strchr(m315_valid_chars, toupper(*i))) {
      *o++ = toupper(*i);
    }
  }
  *o = 0;
  QString rv(rstring);
  xfree(rstring);
  return rv;
}

/*
 * Do same for 330, Meridian, and SportTrak.
 */
QString
Magellan::m330_cleanse(const char* istring)
{
  static constexpr char m330_valid_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ "
      "abcdefghijklmnopqrstuvwxyz"
      "0123456789+-.'/!@#<%^&>()=:\\";
  char* rstring = (char*) xmalloc(strlen(istring)+1);
  char* o;
  const char* i;

  for (o=rstring,i=istring; *i; i++) {
    if (strchr(m330_valid_chars, *i)) {
      *o++ = *i;
    }
  }
  *o = 0;
  QString rv(rstring);
  xfree(rstring);
  return rv;
}

/*
 * Given a protocol message, compute the checksum as needed by
 * the Magellan protocol.
 */
unsigned int
Magellan::mag_checksum(const char* const buf)
{
  int csum = 0;

  for (const char* p = buf; *p; p++) {
    csum  ^= *p;
  }

  return csum;
}
unsigned int
MagprotoBase::mag_pchecksum(const char* const buf, int len)
{
  int csum = 0;
  const char* p = buf;
  for (; len ; len--) {
    csum ^= *p++;
  }
  return csum;
}

void
MagprotoBase::mag_writemsg(const char* const buf)
{
  unsigned int osum = mag_checksum(buf);
  int retry_cnt = 5;
  QScopedPointer<char, QScopedPointerPodDeleter> obuf;

  if (debug_serial) {
    warning("WRITE: $%s*%02X\r\n",buf, osum);
  }

retry:

  int i = xasprintf(obuf, "$%s*%02X\r\n",buf, osum);
  termwrite(obuf.data(), i);
  if (magrxstate == mrs_handon || magrxstate == mrs_awaiting_ack) {
    magrxstate = mrs_awaiting_ack;
    mag_readmsg(trkdata);
    if (last_rx_csum != osum) {
      if (debug_serial) {
        warning("COMM ERROR: Expected %02x, got %02x",
                osum, last_rx_csum);
      }
      if (retry_cnt--) {
        goto retry;
      } else {
        mag_handoff();
        fatal(MYNAME
              ": Too many communication errors.\n");
      }
    }
  }
}

void
MagprotoBase::mag_writeack(int osum)
{
  QScopedPointer<char, QScopedPointerPodDeleter> nbuf;
  QScopedPointer<char, QScopedPointerPodDeleter> obuf;

  if (is_file) {
    return;
  }

  (void) xasprintf(nbuf, "PMGNCSM,%02X", osum);
  unsigned int nsum = mag_checksum(nbuf.data());
  int i = xasprintf(obuf, "$%s*%02X\r\n",nbuf.data(), nsum);

  if (debug_serial) {
    warning("ACK WRITE: %s",obuf.data());
  }
  /*
   * Don't call mag_writemsg here so we don't get into ack feedback
   * loops.
   */
  termwrite(obuf.data(), i);
}

void
MagprotoBase::mag_handon()
{
  if (!is_file) {
    mag_writemsg("PMGNCMD,HANDON");
  }
  magrxstate = mrs_handon;

}

void
MagprotoBase::mag_handoff()
{
  if (!is_file) {
    mag_writemsg("PMGNCMD,HANDOFF");
  }
  magrxstate = mrs_handoff;
}

void
MagprotoBase::mag_verparse(char* ibuf)
{
  int prodid = mm_unknown;
  char version[1024];
  const pid_to_model_t* pp = pid_to_model;

  got_version = 1;
  sscanf(ibuf,"$PMGNVER,%d,%[^,]", &prodid, version);

  for (pp = pid_to_model; pp->model != mm_unknown; pp++) {
    if (pp->pid == prodid) {
      break;
    }
  }

  if (prodid == 37) {
    broken_sportrak = 1;
  }

  switch (pp->model) {
  case mm_gps315320:
  case mm_map410:
    icon_mapping = gps315_icon_table;
    setshort_length(mkshort_handle, 6);
    setshort_mustupper(mkshort_handle, 1);
    mag_cleanse = m315_cleanse;
    break;
  case mm_map330:
  case mm_meridian:
  case mm_sportrak:
    icon_mapping = map330_icon_table;
    setshort_length(mkshort_handle, wpt_len);
    setshort_mustupper(mkshort_handle, 0);
    mag_cleanse = m330_cleanse;
    break;
  default:
    fatal(MYNAME ": Unknown receiver type %d, model version '%s'.\n", prodid, version);
  }
}

#define IS_TKN(x) (strncmp(ibuf,x, sizeof(x)-1) == 0)

void
MagprotoBase::mag_readmsg(gpsdata_type objective)
{
  char ibuf[512];	/* oliskoli: corrupted data (I've seen descr with a lot
				     of escaped FFFFFFFF) may need more size  */
  int retrycnt = 20;

retry:
  QString gr = termread(ibuf, sizeof(ibuf));

  if (gr.isEmpty()) {
    if (!got_version) {
      /*
       * The 315 can take up to six seconds to respond to
       * a VERSION command.   Since this is on startup,
       * we'll be fairly persistent in retrying.
       */
      if (retrycnt--) {
        goto retry;
      } else {
        fatal(MYNAME ": No data received from GPS.\n");
      }
    } else {
      if (is_file)  {
        found_done = 1;
      }
      return;
    }
  }

  /* If column zero isn't a dollar sign, it's not for us */
  if (ibuf[0] != '$') {
    fatal(MYNAME ": line doesn't start with '$'.\n");
  }


  int isz = strlen(ibuf);

  if (isz < 5) {
    if (debug_serial) {
      warning("SHORT READ %d\n", isz);
    }
    return;
  }
  mag_error = 0;
  while (!isprint(ibuf[isz])) {
    isz--;
  }
  char* isump = &ibuf[isz-1];
  unsigned int isum = strtoul(isump, nullptr,16);
  if (isum != mag_pchecksum(&ibuf[1], isz-3)) {
    if (debug_serial) {
      warning("RXERR %02x/%02x: '%s'\n", isum, mag_pchecksum(&ibuf[1],isz-5), ibuf);
    }
    /* Special case receive errors early on. */
    if (!got_version) {
      fatal(MYNAME ": bad communication.  Check bit rate.\n");
    }
  }
  if (debug_serial) {
    warning("READ: %s\n", ibuf);
  }
  if (IS_TKN("$PMGNCSM,")) {
    last_rx_csum = strtoul(&ibuf[9], nullptr, 16);
    magrxstate = mrs_handon;
    return;
  }
  if (strncmp(ibuf, "$PMGNWPL,", 7) == 0) {
    Waypoint* wpt = mag_wptparse(ibuf);
    waypoint_read_count++;
    if (global_opts.verbose_status) {
      waypt_status_disp(waypoint_read_count,
                        waypoint_read_count);
    }

    if (extension_hint) {
      if (extension_hint == WPTDATAMASK) {
        waypt_add(wpt);
      } else if (extension_hint == RTEDATAMASK) {
        rte_wpt_tmp.append(wpt);
      }
    } else {
      switch (objective) {
      case wptdata:
        waypt_add(wpt);
        break;
      case rtedata:
        rte_wpt_tmp.append(wpt);
        break;
      default:
        break;
      }
    }
  }
  if (strncmp(ibuf, "$PMGNTRK,", 7) == 0) {
    Waypoint* wpt = mag_trkparse(ibuf);
    /*
     * Allow lazy allocation of track head.
     */
    if (trk_head == nullptr) {
      /* These tracks don't have names, so derive one
       * from input filename.
       */

      trk_head = new route_head;

      /* Whack trailing extension if present. */
      QString s = get_filename(curfname);
      int idx = s.indexOf('.');
      if (idx > 0) {
        s.truncate(idx);
      }

      trk_head->rte_name = s;
      track_add_head(trk_head);
    }

    track_add_wpt(trk_head, wpt);
  }
  if (strncmp(ibuf, "$PMGNRTE,", 7) == 0) {
    mag_rteparse(ibuf);
  }
  if (IS_TKN("$PMGNVER,")) {
    mag_verparse(ibuf);
  }
  mag_error = 0;
  if (!ignore_unable && IS_TKN("$PMGNCMD,UNABLE")) {
    warning("Unable to send\n");
    found_done = 1;
    mag_error = 1;
    ignore_unable = 0;
    return;
  }
  if (IS_TKN("$PMGNCMD,END") || (is_file && (gbfeof(magfile_h)))) {
    found_done = 1;
    return;
  }

  if (magrxstate != mrs_handoff) {
    mag_writeack(isum);
  }
}

int
MagprotoBase::terminit(const QString& portname, int create_ok)
{
  if (gbser_is_serial(qPrintable(portname))) {
    if (serial_handle = gbser_init(qPrintable(portname)), nullptr != serial_handle) {
      int rc;
      if (rc = gbser_set_port(serial_handle, bitrate, 8, 0, 1), gbser_OK != rc) {
        fatal(MYNAME ": Can't configure port\n");
      }
    }
    is_file = 0;
    if (serial_handle == nullptr) {
      fatal(MYNAME ": Could not open serial port %s\n", qPrintable(portname));
    }
    return 1;
  } else {
    /* Does this check for an error? */
    magfile_h = gbfopen(portname, create_ok ? "w+b" : "rb", MYNAME);
    is_file = 1;
    icon_mapping = map330_icon_table;
    mag_cleanse = m330_cleanse;
    got_version = 1;
    return 0;
  }
}

QString MagprotoBase::termread(char* ibuf, int size)
{
  if (is_file) {
    return gbfgets(ibuf, size, magfile_h);
  } else {
    int rc = gbser_read_line(serial_handle, ibuf, size, 2000, 0x0a, 0x0d);
    if (rc != gbser_OK) {
      fatal(MYNAME ": Read error\n");
    }
    return ibuf;
  }
}

/* Though not documented in the protocol spec, if the unit itself
 * wants to create a field containing a comma, it will encode it
 * as <escape>2C.  We extrapolate that any 2 digit hex encoding may
 * be valid.  We don't do this in termread() since we need to do it
 * after the scanf.  This means we have to do it field-by-field
 * basis.
 *
 * The buffer is modified in place and shortened by copying the remaining
 * string including the terminator.
 */
void
MagprotoBase::mag_dequote(char* ibuf)
{
  char* esc = nullptr;

  while ((esc = strchr(ibuf, 0x1b))) {
    int nremains = strlen(esc);
    if (nremains >= 3) {
      static constexpr char hex[] = "0123456789ABCDEF";
      const char* c1 = strchr(hex, esc[1]);
      const char* c2 = strchr(hex, esc[2]);
      if (c1 && c2) {
        int escv = (c1 - hex) * 16 + (c2 - hex);
        if (escv == 255) {	/* corrupted data */
          char* tmp = esc + 1;
          while (*tmp == 'F') {
            tmp++;
          }
          memmove(esc, tmp, strlen(tmp) + 1);
        } else {
          *esc++ = (isprint(escv)) ? escv : '$';
          /* buffers overlap */
          memmove(esc, esc+2, nremains - 2);
        }
      }
    } else {
      *esc = '\0';	/* trim corrupted data,
					   otherwise we get an endless loop */
    }
  }
}

void
MagprotoBase::termwrite(const char* obuf, int size)
{
  if (is_file) {
    size_t nw;
    if (nw = gbfwrite(obuf, 1, size, magfile_h), nw < (size_t) size) {
      fatal(MYNAME ": Write error");
    }
  } else {
    int rc;
    if (rc = gbser_write(serial_handle, obuf, size), rc < 0) {
      fatal(MYNAME ": Write error");
    }
  }
}

void MagprotoBase::termdeinit()
{
  if (is_file) {
    gbfclose(magfile_h);
    magfile_h = nullptr;
  } else {
    gbser_deinit(serial_handle);
    serial_handle = nullptr;
  }
}

/*
 * The part of the serial init that's common to read and write.
 */
void
MagprotoBase::mag_serial_init_common(const QString& portname)
{
  if (is_file) {
    return;
  }

  mag_handoff();
  if (!noack && !suppress_ack) {
    mag_handon();
  }

  time_t now = current_time().toTime_t();
  /*
   * The 315 can take up to 4.25 seconds to respond to initialization
   * commands.   Time out on the side of caution.
   */
  time_t later = now + 6;
  got_version = 0;
  mag_writemsg("PMGNCMD,VERSION");

  while (!got_version) {
    mag_readmsg(trkdata);
    if (current_time().toTime_t() > later) {
      fatal(MYNAME ": No acknowledgment from GPS on %s\n",
            qPrintable(portname));
    }
  }

  if ((icon_mapping != gps315_icon_table)) {
    /*
     * The 315 can't handle this command, so we set a global
     * to ignore the NAK on it.
     */
    ignore_unable = 1;
    mag_writemsg("PMGNCMD,NMEAOFF");
    ignore_unable = 0;
  }

  if (nukewpt) {
    /* The unit will send us an "end" message upon completion */
    mag_writemsg("PMGNCMD,DELETE,WAYPOINT");
    mag_readmsg(trkdata);
    if (!found_done) {
      fatal(MYNAME ": Unexpected response to waypoint delete command.\n");
    }
    found_done = 0;
  }

}

void
MagprotoBase::mag_rd_init_common(const QString& portname)
{
  waypoint_read_count = 0;
  // For Explorist GC, intercept the device access and redirect to GPX.
  // We actually do the rd_init() inside read as we may have multiple
  // files that we have to read.
  if (portname == "usb:") {
    const char** dlist = os_get_magellan_mountpoints();
    explorist_info = explorist_ini_get(dlist);
    if (explorist_info) {
      gpx_vec = Vecs::Instance().find_vec("gpx");
    }
    return;
  }

  if (bs) {
    bitrate=atoi(bs);
  }

  if (!mkshort_handle) {
    mkshort_handle = mkshort_new_handle();
  }

  terminit(portname, 0);
  mag_serial_init_common(portname);

  rte_wpt_tmp.clear();

  /* find the location of the tail of the path name,
   * make a copy of it, then lop off the file extension
   */

  curfname = get_filename(portname);

  /*
   * I'd rather not derive behaviour from filenames but since
   * we can't otherwise tell if we should put a WPT on the route
   * queue or the WPT queue in the presence of (-w -r -t) we
   * divine a hint from the filename extension when we can.
   */
  QString exten = QFileInfo(curfname).suffix();
  if (exten.length() > 0) {
    if (0 == exten.compare(QLatin1String("upt"), Qt::CaseInsensitive)) {
      extension_hint = WPTDATAMASK;
    } else if (0 == exten.compare(QLatin1String("log"), Qt::CaseInsensitive)) {
      extension_hint = TRKDATAMASK;
    } else if (0 == exten.compare(QLatin1String("rte"), Qt::CaseInsensitive)) {
      extension_hint = RTEDATAMASK;
    }
  }

}

void
MagprotoBase::mag_rd_init(const QString& portname)
{
  explorist = 0;
  suppress_ack = 1;
  mag_rd_init_common(portname);
}

void
MagprotoXFileFormat::rd_init(const QString& portname)
{
  explorist = 1;
  mag_rd_init_common(portname);
}

void
MagprotoBase::mag_wr_init_common(const QString& portname)
{
  suppress_ack = 0;
  if (bs) {
    bitrate=atoi(bs);
  }

  if (waypt_count() > 500) {
    fatal(MYNAME ": Meridian/Explorist does not support more than 500 waypoints in one file. Only\n200 waypoints may have comments.\nDecrease the number of waypoints sent.\n");
  }

  if (cmts) {
    wptcmtcnt_max = atoi(cmts);
  } else {
    wptcmtcnt_max = MAXCMTCT ;
  }

  if (!mkshort_handle) {
    mkshort_handle = mkshort_new_handle();
  }

  terminit(portname, 1);
  mag_serial_init_common(portname);

  rte_wpt_tmp.clear();
}

/*
 * Entry point for extended (explorist) points.
 */
void
MagprotoXFileFormat::wr_init(const QString& portname)
{
  wpt_len = 20;
  explorist = 1;
  mag_wr_init_common(portname);
  setshort_length(mkshort_handle, wpt_len);
  setshort_whitespace_ok(mkshort_handle, 1);
}

void
MagprotoBase::mag_wr_init(const QString& portname)
{
  explorist = 0;
  wpt_len = 8;
  mag_wr_init_common(portname);
  /*
   * Whitespace is actually legal, but since waypoint name length is
   * only 8 bytes, we'll conserve them.
   */

  setshort_whitespace_ok(mkshort_handle, 0);
}

void
MagprotoBase::mag_deinit()
{
  if (explorist_info) {
    explorist_ini_done(explorist_info);
    return;
  }
  mag_handoff();
  termdeinit();
  if (mkshort_handle) {
    mkshort_del_handle(&mkshort_handle);
  }

  while (!rte_wpt_tmp.isEmpty()) {
    delete rte_wpt_tmp.takeFirst();
  }

  trk_head = nullptr;

  curfname.clear();
}

void
MagprotoBase::mag_wr_deinit()
{
  if (explorist) {
    mag_writemsg("PMGNCMD,END");
  }
  mag_deinit();
}

/*
 * I'm tired of arguing with scanf about optional fields .  Detokenize
 * an incoming string that may contain empty fields.
 *
 * Probably should be cleaned up and moved to common code, but
 * making it deal with an arbitrary number of fields of arbitrary
 * size is icky.  We don't have to solve the general case here...
 */

void MagprotoBase::parse_istring(char* istring)
{
  int f = 0;
  int n;
  while (istring[0]) {
    char* fp = ifield[f];
    int x = sscanf(istring, "%[^,]%n", fp, &n);
    f++;
    if (x) {
      istring += n;
      /* IF more in this string, skip delim */
      if (istring[0]) {
        istring++;
      }
    } else {
      istring ++;
    }
  }
}

/*
 * Given an incoming track messages of the form:
 * $PMGNTRK,3605.259,N,08644.389,W,00151,M,201444.61,A,,020302*66
 * create and return a populated waypoint.
 */
Waypoint*
MagprotoBase::mag_trkparse(char* trkmsg)
{
  int hms;
  int fracsecs;
  struct tm tm;

  auto* waypt = new Waypoint;

  memset(&tm, 0, sizeof(tm));

  /*
   * As some of the fields are optional, sscanf works badly
   * for us.
   */
  parse_istring(trkmsg);
  double latdeg = atof(ifield[1]);
  char latdir = ifield[2][0];
  double lngdeg = atof(ifield[3]);
  char lngdir = ifield[4][0];
  int alt = atof(ifield[5]);
  char altunits = ifield[6][0];
  (void)altunits;
  sscanf(ifield[7], "%d.%d", &hms, &fracsecs);
  /* Field 8 is constant */
  /* Field nine is optional track name */
  int dmy = atoi(ifield[10]);
  int sec = hms % 100;
  hms = hms / 100;
  int min = hms % 100;
  hms = hms / 100;
  int hour = hms % 100;

  int year = 100 + dmy % 100 + 1900;
  dmy = dmy / 100;
  int mon =  dmy % 100;
  dmy = dmy / 100;
  int day = dmy % 100;
  QDateTime dt(QDate(year, mon, day), QTime(hour, min, sec, fracsecs * 10), Qt::UTC);
  waypt->SetCreationTime(dt);

  if (latdir == 'S') {
    latdeg = -latdeg;
  }
  waypt->latitude = ddmm2degrees(latdeg);

  if (lngdir == 'W') {
    lngdeg = -lngdeg;
  }
  waypt->longitude = ddmm2degrees(lngdeg);

  waypt->altitude = alt;

  return waypt;

}

/*
 * Given an incoming route messages of the form:
 * $PMGNRTE,4,1,c,1,DAD,a,Anna,a*61
 * generate a route.
 */
void
MagprotoBase::mag_rteparse(char* rtemsg)
{
  int n;
  int frags,frag,rtenum;
  char xbuf[100],next_stop[100],abuf[100];
  char* currtemsg;
  static mag_rte_head_t* mag_rte_head;

#if 0
  sscanf(rtemsg,"$PMGNRTE,%d,%d,%c,%d%n",
         &frags,&frag,xbuf,&rtenum,&n);
#else
  sscanf(rtemsg,"$PMGNRTE,%d,%d,%c,%d%n",
         &frags,&frag,xbuf,&rtenum,&n);

  /* Explorist has a route name here */
  QString rte_name;
  if (explorist) {
    char* ca = rtemsg + n;
    if (*ca++ != ',') {
      fatal(MYNAME ": Incorrectly formatted route line '%s'", rtemsg);
    }

    char* ce = strchr(ca, ',');
    if (ce == nullptr) {
      fatal(MYNAME ": Incorrectly formatted route line '%s'", rtemsg);
    }

    if (ca == ce) {
      rte_name = "Route";
      rte_name += QString::number(rtenum);
    } else {
      rte_name = ca;
      rte_name.truncate(ce-ca);
    }

    n += ((ce - ca) + 1);
  }

#endif

  /*
   * This is the first component of a route.  Allocate a new
   * head.
   */
  if (frag == 1) {
    mag_rte_head = new mag_rte_head_t;
    mag_rte_head->nelems = frags;
  }

  currtemsg = rtemsg + n;

  /*
   * The individual line may contain several route elements.
   * loop and pick those up.
   */
  while (sscanf(currtemsg,",%[^,],%[^,]%n",next_stop, abuf,&n)) {
    if ((next_stop[0] == 0) || (next_stop[0] == '*')) {
      break;
    }

    /* trim CRC from waypoint icon string */
    if (char* p = strchr(abuf, '*'); p != nullptr) {
      *p = '\0';
    }

    auto* rte_elem = new mag_rte_elem;

    rte_elem->wpt_name = next_stop;
    rte_elem->wpt_icon = abuf;

    mag_rte_head->elem_list.append(rte_elem);

    /* Sportrak (the non-mapping unit) creates malformed
     * RTE sentence with no icon info after the routepoint
     * name.  So if we saw an "icon" treat that as new
     * routepoint.
     */
    if (broken_sportrak && abuf[0]) {
      rte_elem = new mag_rte_elem;
      rte_elem->wpt_name = abuf;

      mag_rte_head->elem_list.append(rte_elem);
    }

    next_stop[0] = 0;
    currtemsg += n;
  }

  /*
   * If this was the last fragment of the route, add it to the
   * gpsbabel internal structs now.
   */
  if (frag == mag_rte_head->nelems) {

    auto* rte_head = new route_head;
    route_add_head(rte_head);
    rte_head->rte_num = rtenum;
    rte_head->rte_name = rte_name;

    /*
     * It is quite feasible that we have 200 waypoints,
     * 3 of which are used in the route.  We'll need to find
     * those in the queue for SD routes...
     */

    while (!mag_rte_head->elem_list.isEmpty()) {
      mag_rte_elem* re = mag_rte_head->elem_list.takeFirst();

      /*
       * Copy route points from temp wpt queue.
       */
      foreach (const Waypoint* waypt, rte_wpt_tmp) {
        if (waypt->shortname == re->wpt_name) {
          auto* wpt = new Waypoint(*waypt);
          route_add_wpt(rte_head, wpt);
          break;
        }
      }

      delete re;
    }
    delete mag_rte_head;
  }
}

QString
Magellan::mag_find_descr_from_token(const char* token)
{
  if (icon_mapping == nullptr) {
    return "unknown";
  }

  for (const magellan_icon_mapping_t* i = icon_mapping; i->token; i++) {
    if (token[0] == 0) {
      break;
    }
    if (case_ignore_strcmp(token, i->token) == 0) {
      return i->icon;
    }
  }
  return icon_mapping[0].icon;
}

QString
Magellan::mag_find_token_from_descr(const QString& icon)
{
  const magellan_icon_mapping_t* i = icon_mapping;

  if (i == nullptr || icon == nullptr) {
    return "a";
  }

  for (i = icon_mapping; i->token; i++) {
    if (icon.compare(i->icon, Qt::CaseInsensitive) == 0) {
      return i->token;
    }
  }
  return icon_mapping[0].token;
}

/*
 * Given an incoming waypoint messages of the form:
 * $PMGNWPL,3549.499,N,08650.827,W,0000257,M,HOME,HOME,c*4D
 * create and return a populated waypoint.
 */
Waypoint*
MagprotoBase::mag_wptparse(char* trkmsg)
{
  double latdeg, lngdeg;
  char latdir;
  char lngdir;
  int alt;
  char altunits;
  char shortname[100];
  char descr[256];
  char icon_token[100];
  int i = 0;

  descr[0] = 0;
  icon_token[0] = 0;

  auto* waypt = new Waypoint;

  sscanf(trkmsg,"$PMGNWPL,%lf,%c,%lf,%c,%d,%c,%[^,],%[^,]",
         &latdeg,&latdir,
         &lngdeg,&lngdir,
         &alt,&altunits,shortname,descr);
  char* icone = strrchr(trkmsg, '*');
  char* icons = strrchr(trkmsg, ',')+1;

  mag_dequote(descr);

  for (char* blah = icons ; blah < icone; blah++) {
    icon_token[i++] = *blah;
  }
  icon_token[i++] = '\0';

  if (latdir == 'S') {
    latdeg = -latdeg;
  }
  waypt->latitude = ddmm2degrees(latdeg);

  if (lngdir == 'W') {
    lngdeg = -lngdeg;
  }
  waypt->longitude = ddmm2degrees(lngdeg);

  waypt->altitude = alt;
  waypt->shortname = shortname;
  waypt->description = descr;
  waypt->icon_descr = mag_find_descr_from_token(icon_token);

  return waypt;
}

void
MagprotoBase::mag_read()
{
  if (gpx_vec) {
    QStringList f = os_gpx_files(explorist_info->track_path);
    for (const auto& file : qAsConst(f)) {
      gpx_vec->rd_init(file);
      gpx_vec->read();
      gpx_vec->rd_deinit();
    }

    f = os_gpx_files(explorist_info->waypoint_path);
    for (const auto& file : qAsConst(f)) {
      gpx_vec->rd_init(file);
      gpx_vec->read();
      gpx_vec->rd_deinit();
    }
#if 0
    f = os_gpx_files(explorist_info->geo_path);
    for (const auto& file : qAsConst(f)) {
      gpx_vec->rd_init(file);
      gpx_vec->read();
      gpx_vec->rd_deinit();
    }
#endif
    return;
  }

  found_done = 0;
  if (global_opts.masked_objective & TRKDATAMASK) {
    magrxstate = mrs_handoff;
    if (!is_file) {
      mag_writemsg("PMGNCMD,TRACK,2");
    }

    while (!found_done) {
      mag_readmsg(trkdata);
    }
  }

  found_done = 0;
  if (global_opts.masked_objective & WPTDATAMASK) {
    magrxstate = mrs_handoff;
    if (!is_file) {
      mag_writemsg("PMGNCMD,WAYPOINT");
    }

    while (!found_done) {
      mag_readmsg(wptdata);
    }
  }

  found_done = 0;
  if (global_opts.masked_objective & RTEDATAMASK) {
    magrxstate = mrs_handoff;
    if (!is_file) {
      /*
       * serial routes require waypoint & routes
       * messages commands.
       */
      mag_writemsg("PMGNCMD,WAYPOINT");

      while (!found_done) {
        mag_readmsg(rtedata);
      }

      mag_writemsg("PMGNCMD,ROUTE");

      found_done = 0;
      while (!found_done) {
        mag_readmsg(rtedata);
      }
    } else {
      /*
       * SD routes are a stream of PMGNWPL and
       * PMGNRTE messages, in that order.
       */
      while (!found_done) {
        mag_readmsg(rtedata);
      }
    }
  }
}

void
MagprotoBase::mag_waypt_pr(const Waypoint* waypointp)
{
  QScopedPointer<char, QScopedPointerPodDeleter> obuf;
  QScopedPointer<char, QScopedPointerPodDeleter> ofmtdesc;
  QString icon_token;

  double ilat = waypointp->latitude;
  double ilon = waypointp->longitude;

  double lon = fabs(ilon);
  double lat = fabs(ilat);

  int lon_deg = lon;
  int lat_deg = lat;

  lon = (lon - lon_deg) * 60.0;
  lat = (lat - lat_deg) * 60.0;

  lon = (lon_deg * 100.0 + lon);
  lat = (lat_deg * 100.0 + lat);

  if (deficon)  {
    icon_token = mag_find_token_from_descr(deficon);
  } else {
    icon_token = mag_find_token_from_descr(waypointp->icon_descr);
  }

  if (get_cache_icon(waypointp)) {
    icon_token = mag_find_token_from_descr(get_cache_icon(waypointp));
  }

  QString isrc = waypointp->notes.isEmpty() ? waypointp->description : waypointp->notes;
  QString owpt = global_opts.synthesize_shortnames ?
                 mkshort_from_wpt(mkshort_handle, waypointp) : waypointp->shortname;
  QString odesc = isrc;
  owpt = mag_cleanse(CSTRc(owpt));

  if (global_opts.smart_icons &&
      waypointp->gc_data->diff && waypointp->gc_data->terr) {
    // It's a string and compactness counts, so "1.0" is OK to be "10".
    xasprintf(ofmtdesc, "%ud/%ud %s", waypointp->gc_data->diff,
              waypointp->gc_data->terr, CSTRc(odesc));
    odesc = mag_cleanse(ofmtdesc.data());
  } else {
    odesc = mag_cleanse(CSTRc(odesc));
  }

  /*
   * For the benefit of DirectRoute (which uses waypoint comments
   * to deliver turn-by-turn popups for street routing) allow a
   * cap on the comments delivered so we leave space for it to route.
   */
  if (!odesc.isEmpty() && (wptcmtcnt++ >= wptcmtcnt_max)) {
    odesc.clear();
  }

  xasprintf(obuf, "PMGNWPL,%4.3f,%c,%09.3f,%c,%07.0f,M,%-.*s,%-.46s,%s",
            lat, ilat < 0 ? 'S' : 'N',
            lon, ilon < 0 ? 'W' : 'E',
            waypointp->altitude == unknown_alt ?
            0 : waypointp->altitude,
            wpt_len,
            CSTRc(owpt),
            CSTRc(odesc),
            CSTR(icon_token));
  mag_writemsg(obuf.data());

  if (!is_file) {
    if (mag_error) {
      warning("Protocol error Writing '%s'\n", obuf.data());
    }
  }
}

void MagprotoBase::mag_track_disp(const Waypoint* waypointp)
{
  QScopedPointer<char, QScopedPointerPodDeleter> obuf;

  double ilat = waypointp->latitude;
  double ilon = waypointp->longitude;

  QByteArray dmy("");
  QByteArray hms("");
  if (waypointp->creation_time.isValid()) {
    // Round to hundredths of seconds before conversion to string.
    // Rounding can ripple all the way from the msec to the year.
    QDateTime dt = waypointp->GetCreationTime().toUTC();
    dt = dt.addMSecs(10 * lround(dt.time().msec()/10.0) - dt.time().msec());
    assert((dt.time().msec() % 10) == 0);
    dmy = dt.toString("ddMMyy").toUtf8();
    hms = dt.toString("hhmmss.zzz").left(9).toUtf8();
  }

  double lon = fabs(ilon);
  double lat = fabs(ilat);

  int lon_deg = lon;
  int lat_deg = lat;

  lon = (lon - lon_deg) * 60.0;
  lat = (lat - lat_deg) * 60.0;

  lon = (lon_deg * 100.0 + lon);
  lat = (lat_deg * 100.0 + lat);

  xasprintf(obuf,"PMGNTRK,%4.3f,%c,%09.3f,%c,%05.0f,%c,%s,A,,%s",
            lat, ilat < 0 ? 'S' : 'N',
            lon, ilon < 0 ? 'W' : 'E',
            waypointp->altitude == unknown_alt ?
            0 : waypointp->altitude,
            'M', hms.constData(), dmy.constData());
  mag_writemsg(obuf.data());
}

void MagprotoBase::mag_track_pr()
{
  auto mag_track_disp_lambda = [this](const Waypoint* waypointp)->void {
    mag_track_disp(waypointp);
  };
  track_disp_all(nullptr, nullptr, mag_track_disp_lambda);
}

/*
The spec says to stack points:
	$PMGNRTE,2,1,c,1,FOO,POINT1,b,POINT2,c,POINT3,d*6C<CR><LF>

Meridian SD card and serial (at least) writes in pairs:
	$PMGNRTE,4,1,c,1,HOME,c,I49X73,a*15
	...
	$PMGNRTE,4,4,c,1,RON273,a,MYCF93,a*7B

The spec also says that some units don't like single-legged pairs,
and to replace the 2nd name with "<<>>", but I haven't seen one of those.
*/

void
MagprotoBase::mag_route_trl(const route_head* rte)
{
  QScopedPointer<char, QScopedPointerPodDeleter> obuff;
  QString buff1;
  QString buff2;
  QString icon_token;

  /* count waypoints for this route */
  int i = rte->rte_waypt_ct();

  /* number of output PMGNRTE messages at 2 points per line */
  int numlines = (i / 2) + (i % 2);

  /* increment the route counter. */
  route_out_count++;

  int thisline = i = 0;
  foreach (const Waypoint* waypointp, rte->waypoint_list) {
    i++;

    if (deficon) {
      icon_token = mag_find_token_from_descr(deficon);
    } else {
      icon_token = mag_find_token_from_descr(waypointp->icon_descr);
    }

    QString* pbuff;
    if (i == 1) {
      pbuff = &buff1;
    } else {
      pbuff = &buff2;
    }
    // Write name, icon tuple into alternating buff1/buff2 buffer.
    *pbuff = waypointp->shortname + ',' + icon_token;

    if ((waypointp == rte->waypoint_list.back()) || ((i % 2) == 0)) {
      QString expbuf;
      thisline++;
      if (explorist) {
        expbuf = rte->rte_name + ',';
      }

      xasprintf(obuff, "PMGNRTE,%d,%d,c,%d,%s%s,%s",
                numlines, thisline,
                rte->rte_num ? rte->rte_num : route_out_count,
                CSTRc(expbuf),
                CSTR(buff1), CSTR(buff2));

      mag_writemsg(obuff.data());
      buff1.clear();
      buff2.clear();
      i = 0;
    }
  }
}

void
MagprotoBase::mag_route_pr()
{
  route_out_count = 0;
  auto mag_route_trl_lambda = [this](const route_head* rte)->void {
    mag_route_trl(rte);
  };
  auto mag_waypt_pr_lambda = [this](const Waypoint* waypointp)->void {
    mag_waypt_pr(waypointp);
  };
  route_disp_all(nullptr, mag_route_trl_lambda, mag_waypt_pr_lambda);

}

void
MagprotoBase::mag_write()
{

  wptcmtcnt = 0;

  switch (global_opts.objective) {
  case trkdata:
    mag_track_pr();
    break;
  case wptdata: {
    auto mag_waypt_pr_lambda = [this](const Waypoint* waypointp)->void {
      mag_waypt_pr(waypointp);
    };
    waypt_disp_all(mag_waypt_pr_lambda);
    break;
  }
  case rtedata:
    mag_route_pr();
    break;
  default:
    fatal(MYNAME ": Unknown objective.\n");
  }
}

const char** MagprotoBase::os_get_magellan_mountpoints()
{
#if __APPLE__
  const char** dlist = (const char**) xcalloc(2, sizeof *dlist);
  dlist[0] = xstrdup("/Volumes/Magellan");
  dlist[1] = nullptr;
  return dlist;
#else
  fatal("Not implemented");
  return nullptr;
#endif
}

QStringList
MagprotoBase::os_gpx_files(const char* dirname)
{
  QDir dir(dirname);

  const QFileInfoList filist = dir.entryInfoList(QStringList("*.gpx"), QDir::Files | QDir::Readable, QDir::Name);
  QStringList rv;
  for (const auto& fi : filist) {
    rv.append(fi.absoluteFilePath());
  }
  return rv;
}

// Explorist

#ifdef DEAD_CODE_IS_REBORN
const char*
Explorist::explorist_read_value(const char* section, const char* key)
{
  return inifile_readstr(inifile, section, key);
}
#endif

Explorist::mag_info*
Explorist::explorist_ini_try(const char* path)
{
  char* inipath;

  xasprintf(&inipath, "%s/%s", path, "APP/Atlas.ini");
  inifile = inifile_init(QString::fromUtf8(inipath), myname);
  if (!inifile) {
    xfree(inipath);
    return nullptr;
  }

  auto* info = (mag_info*) xmalloc(sizeof(mag_info));
  info->geo_path = nullptr;
  info->track_path = nullptr;
  info->waypoint_path = nullptr;

  char* s = xstrdup(inifile_readstr(inifile,  "UGDS", "WpFolder"));
  if (s) {
    s = gstrsub(s, "\\", "/");
    xasprintf(&info->waypoint_path, "%s/%s", path, s);
  }
  s = xstrdup(inifile_readstr(inifile,  "UGDS", "GcFolder"));
  if (s) {
    s = gstrsub(s, "\\", "/");
    xasprintf(&info->geo_path, "%s/%s", path, s);
  }
  s = xstrdup(inifile_readstr(inifile,  "UGDS", "TrkFolder"));
  if (s) {
    s = gstrsub(s, "\\", "/");
    xasprintf(&info->track_path, "%s/%s", path, s);
  }

  inifile_done(inifile);
  xfree(inipath);
  return info;
}

Explorist::mag_info*
Explorist::explorist_ini_get(const char** dirlist)
{
  mag_info* r = nullptr;
  while (dirlist && *dirlist) {
    r = explorist_ini_try(*dirlist);
    if (r) {
      return r;
    }
  }
  return r;
}

void
Explorist::explorist_ini_done(mag_info* info)
{
  xfree(info->geo_path);
  xfree(info->track_path);
  xfree(info->waypoint_path);
  xfree(info);
}
