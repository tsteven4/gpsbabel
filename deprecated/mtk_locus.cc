/*

Format converter for MediaTek Locus-capable devices.

Copyright (C) 2012 Jeremy Mortis

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

-------------------------------------------------------------------

This module will download logged track data (a.k.a. Locus) from a GPS
devices based on the MediaTek MT3339 GPS chipset, such as:
- GlobalTop PA6H Module
- Fastrax IT530

The MT3339 also emits standard NMEA packets including
$GPGGA, $GPGSA, $GPGSV, $GPRMC, and $GPVTG.  This module ignores those.
If you have an MT3339 and you want to process NMEA packets, simply use
the nmea format instead of this one.

Example usage::
# Read from USB port, output trackpoints in GPX format
./gpsbabel -t -i mtk_locus -f /dev/ttyUSB0 -o gpx -F out.gpx

*/


#include "defs.h"
#include "gbser.h"
#include <cerrno>
#include <cstdio>
#include <cstdlib>

static route_head* track;

static char* opt_baudrate;
static char* opt_download;
static char* opt_erase;
static char* opt_status;
static char* opt_enable;

static QVector<arglist_t> mtk_locus_args = {
  {"baudrate", &opt_baudrate, "Speed in bits per second of serial port (autodetect=0)", "0", ARGTYPE_INT, ARG_NOMINMAX , nullptr},
  {"download", &opt_download, "Download logged fixes", "1", ARGTYPE_BOOL, ARG_NOMINMAX, nullptr },
  {"erase", &opt_erase, "Erase device data after download", "0", ARGTYPE_BOOL, ARG_NOMINMAX, nullptr },
  {"status", &opt_status, "Show device status", "0", ARGTYPE_BOOL, ARG_NOMINMAX, nullptr },
  {"enable", &opt_enable, "Enable logging after download", "0", ARGTYPE_BOOL, ARG_NOMINMAX, nullptr },
};

static void mtk_locus_rd_init(const QString& fname);
static void mtk_locus_rd_deinit();
static void mtk_locus_read();

ff_vecs_t mtk_locus_vecs = {
  ff_type_file,
  {
    ff_cap_read /* waypoints */,
    ff_cap_read /* tracks */,
    ff_cap_none /* routes */
  },
  mtk_locus_rd_init,
  nullptr,  // write init
  mtk_locus_rd_deinit,
  nullptr,  // write deinit
  mtk_locus_read,
  nullptr,  // write
  nullptr, // exit
  &mtk_locus_args,
  CET_CHARSET_ASCII, 0 /* ascii is the expected character set */
  , NULL_POS_OPS
};

#define MYNAME "mtk_locus"
#define TIMEOUT 1500

#define PMTK_ACK                "$PMTK001"
#define PMTK_LOCUS_STOP_LOGGER  "$PMTK185"
#define PMTK_LOCUS_QUERY_STATUS "$PMTK183"
#define PMTK_LOCUS_ERASE_FLASH  "$PMTK184"
#define PMTK_Q_LOCUS_DATA       "$PMTK622"
#define PMTK_Q_RELEASE          "$PMTK605"
#define PMTK_DT_RELEASE         "$PMTK705"

static gbfile* ffd; // File access.
static void* sfd;
static enum {rm_serial, rm_file} read_mode;
static int packetnum;
static char line[1000];
static int download_complete;
static int valid_packet_found;
static int fixes_found;
static int first_loxsequence;
static int last_loxsequence;
static char waiting_for[20];


static void set_baudrate();
static void read_line();
static void process_packet();
static void process_pmtklox();
static void process_pmtklog();
static void process_pmtk001();
static void process_pmtk705();
static void send_command(const char* s, const char*wait_for);
static int calculate_checksum(const char* s, int length);
static void dbg(int l, const char* msg, ...);

static void
mtk_locus_rd_init(const QString& fname)
{
  dbg(1, "Opening file: %s\n", qPrintable(fname));

  if (gbser_is_serial(qPrintable(fname))) {

    dbg(1, "Input is a serial port\n");
    read_mode = rm_serial;
    if ((sfd = gbser_init(qPrintable(fname))) == nullptr) {
      fatal(MYNAME ": Can't initialise port \"%s\" (%s)\n", qPrintable(fname), strerror(errno));
    }
    set_baudrate();
    gbser_flush(sfd);

  } else {

    dbg(1, "Input is a normal file\n");
    read_mode = rm_file;
    if ((ffd = gbfopen(fname, "rb", MYNAME)) == nullptr) {
      fatal(MYNAME ": Can't initialise port \"%s\" (%s)\n", qPrintable(fname), strerror(errno));
    }
  }

  dbg(1, "File opened\n");
}

static void
mtk_locus_rd_deinit()
{
  if (read_mode == rm_serial) {
    gbser_deinit(sfd);
  } else {
    gbfclose(ffd);
  }
}

static void
mtk_locus_read()
{
  track = new route_head;
  track_add_head(track);
  dbg(1, "Track initialized\n");

  packetnum = 0;
  valid_packet_found = 0;
  fixes_found = 0;
  download_complete = 0;
  first_loxsequence = -1;
  last_loxsequence = -1;

  read_line();
  // initial serial buffer may contain garbage, so read until valid packet found
  for (int i = 0; i<10; i++) {
    process_packet();
    read_line();
    if (valid_packet_found) {
      break;
    }
  }

  if (! valid_packet_found) {
    fatal(MYNAME "No valid input data found");
  }

  if (strcmp(opt_download, "1") == 0) {
    send_command(PMTK_Q_LOCUS_DATA ",1", nullptr);

    while (! download_complete) {
      process_packet();
      read_line();
    }
  }

  if (read_mode == rm_serial) {
    if (strcmp(opt_erase, "1") == 0) {
      send_command(PMTK_LOCUS_ERASE_FLASH ",1", PMTK_ACK);
      printf("Flash erased\n");
    }

    if (strcmp(opt_enable, "1") == 0) {
      send_command(PMTK_LOCUS_STOP_LOGGER ",0", PMTK_ACK);
      printf("Logging enabled\n");
    } else {
      send_command(PMTK_LOCUS_STOP_LOGGER ",1", PMTK_ACK);
      printf("Logging disabled\n");
    }

    if (strcmp(opt_status, "1") == 0) {
      printf("Device status:\n");
      send_command(PMTK_Q_RELEASE, PMTK_DT_RELEASE);
      send_command(PMTK_LOCUS_QUERY_STATUS, PMTK_ACK);
    }
  }
}

void
set_baudrate()
{
  int rc;
  int baudrates[] = { 4800, 9600, 14400, 19200, 38400, 57600, 115200, 0 };
  int baudrate;

  if (strcmp(opt_baudrate, "0") != 0) {

    baudrate = atoi(opt_baudrate);
    rc = gbser_set_speed(sfd, baudrate);
    if (rc != gbser_OK) {
      fatal(MYNAME ": Set baud rate to %i failed (%i)\n", baudrate, rc);
    }

  } else {

    dbg(1, "Probing for baudrate...\n");
    for (int i=0;; i++) {
      baudrate = baudrates[i];
      if (baudrate == 0) {
        fatal(MYNAME ": Autobaud connection failed\n");
      }
      dbg(1, MYNAME ": Probing at %i baud...\n", baudrate);
      rc = gbser_set_speed(sfd, baudrate);

      if (rc != gbser_OK) {
        dbg(1, "Set speed failed\n");
        continue;
      }

      rc = gbser_read_line(sfd, line, sizeof(line)-1, TIMEOUT, 0x0A, 0x0D);
      if (rc != gbser_OK) {
        dbg(1, "Read test failed\n");
        continue;
      }

      dbg(1, "Port successfully opened\n");
      break;
    }
  }
}

void
read_line()
{
  line[0] = '\0';

  if (read_mode == rm_file) {
    char* s = gbfgetstr(ffd);
    if (s == nullptr) {
      dbg(1, "EOF reached\n");
      download_complete = 1;
      return;
    }
    strncat(line, s, sizeof(line)-1);
  } else {
    int rc = gbser_read_line(sfd, line, sizeof(line)-1, TIMEOUT, 0x0A, 0x0D);
    if (rc != gbser_OK) {
      fatal(MYNAME "Serial read failed: %i\n", rc);
    }
  }

  packetnum++;
  dbg(1, "Line %i: %s\n", packetnum, line);
}

void
process_packet()
{
  int given_checksum;

  if ((strlen(line) < 3) || (line[0] != '$') || (line[strlen(line)-3] != '*')) {
    dbg(1, "Line %i: Malformed packet\n", packetnum);
    return;
  }

  int calculated_checksum = calculate_checksum(&line[1], strlen(line) - 1 - 3);
  sscanf(&line[strlen(line) - 2], "%02x", &given_checksum);
  if (calculated_checksum != given_checksum) {
    dbg(1, "Line %i: NMEA Checksum incorrect, expecting %02X\n", packetnum, calculated_checksum);
    return;
  }

  if (strncmp(line, waiting_for, strlen(waiting_for)) == 0) {
    waiting_for[0] = '\0';
  }

  valid_packet_found = 1;
  line[strlen(line) - 3] = '\0';  // remove checksum

  // note that use of strtok in following routines corrupts the value of line

  if (strncmp(line, "$PMTKLOX", 8) == 0) {
    process_pmtklox();
  } else if (strncmp(line, "$PMTKLOG", 8) == 0) {
    process_pmtklog();
  } else if (strncmp(line, "$PMTK001", 8) == 0) {
    process_pmtk001();
  } else if (strncmp(line, "$PMTK705", 8) == 0) {
    process_pmtk705();
  } else {
    dbg(1, "Unknown packet type\n");
  }

}

void
process_pmtklox()
{
  uint8_t fixbytes[16];
  static Waypoint* trkpt;
  static Waypoint* waypt;

  char* token = strtok(line, ",");
  if ((token == nullptr) || (strcmp(token, "$PMTKLOX") != 0)) {
    warning("Line %i: Invalid packet id\n", packetnum);
    return;
  }

  char* loxtype = strtok(nullptr, ",");
  if (loxtype == nullptr) {
    warning("Line %i: Missing lox type\n", packetnum);
    return;
  }

  if (strcmp(loxtype, "0") == 0) {
    last_loxsequence = atoi(strtok(nullptr, "*")) - 1;
    dbg(1, "Line %i: last sequence will be %i\n", packetnum, last_loxsequence);
    return;
  }

  if (strcmp(loxtype, "2") == 0) {
    printf("Found %i fixes\n", fixes_found);
    download_complete = 1;
    return;
  }

  if (strcmp(loxtype, "1") != 0) {
    dbg(1, "Line %i: Invalid lox type\n", packetnum);
    return;
  }

  int loxsequence = atoi(strtok(nullptr, ","));

  if (first_loxsequence == -1) {
    first_loxsequence = loxsequence;
    if (first_loxsequence != 0) {
      fatal(MYNAME "Dump already in progress (first $PMTKLOX has sequence %i)\n", first_loxsequence);
    }
  }

  if (read_mode == rm_serial) {
    printf("Downloading packet %i of %i\r", loxsequence, last_loxsequence);
  }

  token = strtok(nullptr, ",");
  int fixnum = 0;
  while (token != nullptr) {
    fixnum++;
    int bytenum = 0;
    uint8_t calculated_checksum = 0;
    for (int wordnum = 0; wordnum<4; wordnum++) {  // 4 8-byte hex strings per fix
      if (token == nullptr) {
        dbg(1, "Line %i: Fix %i incomplete data\n", packetnum, fixnum);
        return;
      }
      for (int i = 0; i<4; i++) {
        unsigned int hexval;
        sscanf(&token[i * 2], "%2x", &hexval);
        fixbytes[bytenum++] = hexval;
        calculated_checksum ^= hexval;
      }
      token = strtok(nullptr, ",");
    }

    if (calculated_checksum != 0) {
      dbg(1, "Line %i: Fix %i failed checksum\n", packetnum, fixnum);
      continue;
    }

    uint32_t timestamp = le_read32(&fixbytes[0]);
    char fixtype = fixbytes[4];
    float latitude = le_read_float(&fixbytes[5]);
    float longitude = le_read_float(&fixbytes[9]);
    int height = le_read16(&fixbytes[13]);

    if (fixtype != '\x02') {
      dbg(1, "line %i: Fix %i Invalid fix type: %02X\n", packetnum, fixnum, fixtype);
      continue;
    }

    if ((latitude < -180.0) || (latitude > 180.0)
        || (longitude < -180.0) || (longitude > 180.0)
        || (height < -1000) || (height > 100000)) {
      dbg(1, "line %i: Fix %i data out of range\n", packetnum, fixnum);
      continue;
    }

    if (global_opts.masked_objective & TRKDATAMASK) {
      trkpt  = new Waypoint;
      trkpt->SetCreationTime(timestamp);
      trkpt->latitude = latitude;
      trkpt->longitude = longitude;
      trkpt->altitude = height;
      trkpt->sat = 0;
      trkpt->hdop = 0;
      trkpt->fix = fix_3d;
      track_add_wpt(track, trkpt);
    }

    if (global_opts.masked_objective & WPTDATAMASK) {
      waypt  = new Waypoint;
      waypt->SetCreationTime(timestamp);
      waypt->latitude = latitude;
      waypt->longitude = longitude;
      waypt->altitude = height;
      waypt->sat = 0;
      waypt->hdop = 0;
      waypt->fix = fix_3d;
      waypt_add(waypt);
    }

    dbg(1, "Time: %li Type: %02x Lat: %f Long: %f height: %i\n", (long int)timestamp, fixtype, latitude, longitude, height);

    fixes_found++;
  }
}

void
process_pmtklog()
{
  strtok(line, ",");

  printf("Serial#:  %s\n", strtok(nullptr, ","));

  int type = atoi(strtok(nullptr, ","));
  if (type == 0) {
    printf("Type:     %i (wrap around when full)\n", type);
  } else {
    printf("Type:     %i (stop when full)\n", type);
  }

  printf("Mode:     0x%02X\n", atoi(strtok(nullptr, ",")));
  printf("Content:  %s\n", strtok(nullptr, ","));
  printf("Interval: %s seconds\n", strtok(nullptr, ","));
  printf("Distance: %s\n", strtok(nullptr, ","));
  printf("Speed:    %s\n", strtok(nullptr, ","));

  int status = atoi(strtok(nullptr, ","));
  if (status == 0) {
    printf("Status:   %i (enabled)\n", status);
  } else {
    printf("Status:   %i (disabled)\n", status);
  }

  printf("Number:   %s fixes available\n", strtok(nullptr, ","));
  printf("Percent:  %s%% used\n", strtok(nullptr, ","));
}

void
process_pmtk001()
{
  strtok(line, ",");
  char* cmd = strtok(nullptr,",");
  char* flag = strtok(nullptr,",");

  switch (atoi(flag)) {
  case 0:
    dbg(1, "Ack: %s %s (Invalid command)\n", cmd, flag);
    break;
  case 1:
    dbg(1, "Ack: %s %s (Unsupported command)\n", cmd, flag);
    break;
  case 2:
    dbg(1, "Ack: %s %s (Action failed)\n", cmd, flag);
    break;
  case 3:
    dbg(1, "Ack: %s %s (Success)\n", cmd, flag);
    break;
  default:
    dbg(1, "Ack: %s %s (Unknown error)\n", cmd, flag);
    break;
  }
}

void
process_pmtk705()
{
  char* token = strtok(line, ",");
  token = strtok(nullptr,",");

  printf("Firmware: %s\n", token);
}

void
send_command(const char* s, const char* wait_for)
{
  time_t starttime;
  time_t currtime;
  char cmd[100];

  if (read_mode == rm_file) {
    dbg(1, "Sending device commands ignored when using file input: %s\n", s);
    return;
  }

  int checksum = calculate_checksum(&s[1], strlen(s)-1);
  snprintf(cmd, sizeof(cmd)-1, "%s*%02X\r\n", s, checksum);

  int rc = gbser_print(sfd, cmd);
  if (rc != gbser_OK) {
    fatal(MYNAME ": Write error (%d)\n", rc);
  }

  dbg(1, "Sent command: %s\n", cmd);

  if (wait_for == nullptr) {
    waiting_for[0] = '\0';
    return;
  }

  time(&starttime);
  cmd[0] = '\0';
  strncat(cmd, &s[5], 3);
  waiting_for[0] = '\0';
  strncat(waiting_for, wait_for, sizeof(waiting_for)-1);
  dbg(1, "Waiting for: %s\n", waiting_for);

  read_line();
  while (strlen(waiting_for) > 0) {
    time(&currtime);
    if (currtime > starttime + 5) {
      fatal(MYNAME "Ack not received: %s\n", s);
    }
    process_packet();
    read_line();
  }
}

int
calculate_checksum(const char* s, int length)
{
  int sum = 0;

  for (int i = 0; i<length; i++) {
    sum ^= *s++;
  }
  return sum;
}

void
dbg(int l, const char* msg, ...)
{
  va_list ap;
  va_start(ap, msg);
  if (global_opts.debug_level >= l) {
    vfprintf(stderr,msg, ap);
    fflush(stderr);
  }
  va_end(ap);
}


