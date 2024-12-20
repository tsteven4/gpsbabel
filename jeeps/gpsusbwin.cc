/*
    Windows layer of Garmin/USB protocol.

    Copyright (C) 2004, 2006, 2006 Robert Lipe, robertlipe@usa.net

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

#include <cctype>
#include <cstdio>
#include <windows.h> // Boost this out of alpha order.
#include <initguid.h>
#include <malloc.h>
#include <setupapi.h>
#include <winioctl.h>

#include "jeeps/garminusb.h"
#include "jeeps/gps.h"
#include "jeeps/gpsapp.h"
#include "jeeps/gpsusbcommon.h"

/* Constants from Garmin doc. */

// {2C9C45C2-8E7D-4C08-A12D-816BBAE722C0}
DEFINE_GUID(GARMIN_GUID, 0x2c9c45c2L, 0x8e7d, 0x4c08, 0xa1, 0x2d, 0x81, 0x6b, 0xba, 0xe7, 0x22, 0xc0);

#define GARMIN_USB_API_VERSION 1
#define GARMIN_USB_MAX_BUFFER_SIZE 4096
#define GARMIN_USB_INTERRUPT_DATA_SIZE 64

#define IOCTL_GARMIN_USB_API_VERSION CTL_CODE \
	(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GARMIN_USB_INTERRUPT_IN CTL_CODE \
	(FILE_DEVICE_UNKNOWN, 0x850, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GARMIN_USB_BULK_OUT_PACKET_SIZE CTL_CODE \
	(FILE_DEVICE_UNKNOWN, 0x851, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct {
  int booger;
} winusb_unit_data;

static HANDLE usb_handle = INVALID_HANDLE_VALUE;
static int usb_tx_packet_size ;

static int
gusb_win_close(gpsdevh* /* handle */, bool /* exit_lib */)
{
  if (usb_handle != INVALID_HANDLE_VALUE) {
    CloseHandle(usb_handle);
    usb_handle = INVALID_HANDLE_VALUE;
  }

  return 0;
}

static int
gusb_win_get(garmin_usb_packet* ibuf, size_t sz)
{
  DWORD rxed = GARMIN_USB_INTERRUPT_DATA_SIZE;
  unsigned char* buf = (unsigned char*) &ibuf->dbuf;
  int tsz=0;

  while (sz) {
    /* The driver wrongly (IMO) rejects reads smaller than
     * GARMIN_USB_INTERRUPT_DATA_SIZE
     */
    if (!DeviceIoControl(usb_handle, IOCTL_GARMIN_USB_INTERRUPT_IN, NULL, 0,
                         buf, GARMIN_USB_INTERRUPT_DATA_SIZE, &rxed, NULL)) {
      GPS_Serial_Error("Ioctl");
      gbFatal("ioctl\n");
    }
    buf += rxed;
    sz  -= rxed;
    tsz += rxed;
    if (rxed < GARMIN_USB_INTERRUPT_DATA_SIZE) {
      break;
    }
  }
  return tsz;
}

static int
gusb_win_get_bulk(garmin_usb_packet* ibuf, size_t sz)
{
  int n;
  DWORD rsz;
  unsigned char* buf = (unsigned char*) &ibuf->dbuf;

  n = ReadFile(usb_handle, buf, sz, &rsz, NULL);

  return rsz;
}

static int
gusb_win_send(const garmin_usb_packet* opkt, size_t sz)
{
  DWORD rsz;
  const auto* obuf = opkt->dbuf;

  /* The spec warns us about making writes an exact multiple
   * of the packet size, but isn't clear whether we can issue
   * data in a single call to WriteFile if it spans buffers.
   */
  WriteFile(usb_handle, obuf, sz, &rsz, NULL);

  if (rsz != sz) {
    gbFatal("Error sending %zu bytes.  Successfully sent %ld\n", sz, rsz);
  }

  return rsz;
}

static gusb_llops_t win_llops = {
  gusb_win_get,
  gusb_win_get_bulk,
  gusb_win_send,
  gusb_win_close
};

static
HANDLE garmin_usb_start(HDEVINFO hdevinfo, SP_DEVICE_INTERFACE_DATA* infodata)
{
  DWORD size;
  PSP_INTERFACE_DEVICE_DETAIL_DATA pdd = NULL;
  SP_DEVINFO_DATA devinfo;

  SetupDiGetDeviceInterfaceDetail(hdevinfo, infodata,
                                  NULL, 0, &size, NULL);

  pdd = (PSP_INTERFACE_DEVICE_DETAIL_DATA) xmalloc(size);
  pdd->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);

  devinfo.cbSize = sizeof(SP_DEVINFO_DATA);
  if (!SetupDiGetDeviceInterfaceDetail(hdevinfo, infodata,
                                       pdd, size, NULL, &devinfo)) {
    GPS_Serial_Error("SetupDiGetDeviceInterfaceDetail");
    return NULL;
  }

  /* Whew.  All that just to get something we can open... */
  GPS_Diag("Windows GUID for interface is \n\t%s\n",
           pdd->DevicePath);

  if (usb_handle != INVALID_HANDLE_VALUE) {
    gbFatal("garmin_usb_start called while device already started.\n");
  }

  usb_handle = CreateFile(pdd->DevicePath, GENERIC_READ|GENERIC_WRITE,
                          0, NULL, OPEN_EXISTING, 0, NULL);
  if (usb_handle == INVALID_HANDLE_VALUE) {
    if (GetLastError() == ERROR_ACCESS_DENIED) {
      gbWarning(
        "Exclusive access is denied.  It's likely that something else such as\n"
        "Garmin Lifetime Updater, Communicator, Basecamp, Nroute, Spanner,\n"
        "Google Earth, or GPSGate already has control of the device\n");
    }
    GPS_Serial_Error("(usb) CreateFile on '%s' failed", pdd->DevicePath);
    return NULL;
  }

  if (!DeviceIoControl(usb_handle, IOCTL_GARMIN_USB_BULK_OUT_PACKET_SIZE,
                       NULL, 0, &usb_tx_packet_size, GARMIN_USB_INTERRUPT_DATA_SIZE,
                       &size, NULL)) {
    gbFatal("Couldn't get USB packet size.\n");
  }
  win_llops.max_tx_size = usb_tx_packet_size;

  gusb_syncup();

  return usb_handle;
}

/*
 * Main entry point from the upper layer.   Walk the device tree, find our
 * device, and light it up.
 */
int
gusb_init(const char* pname, gpsdevh** dh)
{
  int req_unit_number = 0;
  int un = 0;
  int match;

  HDEVINFO hdevinfo;
  SP_DEVICE_INTERFACE_DATA devinterface;

  winusb_unit_data* wud = (winusb_unit_data*) xcalloc(sizeof(winusb_unit_data), 1);
  *dh = (gpsdevh*) wud;

  gusb_register_ll(&win_llops);

  if (strlen(pname) > 4) {
    if (0 == strcmp(pname+4, "list")) {
      req_unit_number = -1;
    } else {
      req_unit_number = xstrtoi(pname+4, nullptr, 10);
    }
  }

  hdevinfo = SetupDiGetClassDevs(&GARMIN_GUID, NULL, NULL,
                                 DIGCF_PRESENT | DIGCF_INTERFACEDEVICE);

  if (hdevinfo == INVALID_HANDLE_VALUE) {
    GPS_Serial_Error("SetupDiGetClassDevs failed");
    gbWarning("Is the Garmin USB driver installed?\n");
    return 0;
  }

  devinterface.cbSize = sizeof(devinterface);

  if (req_unit_number >= 0) {
    if (!SetupDiEnumDeviceInterfaces(hdevinfo, NULL,
                                     &GARMIN_GUID,
                                     req_unit_number, &devinterface)) {
      GPS_Serial_Error("SetupDiEnumDeviceInterfaces");
      gbWarning("Is the Garmin USB unit number %d powered up and connected?\nIs it really a USB unit?  If it's serial, don't choose USB, choose serial.\nAre the Garmin USB drivers installed and functioning with other programs?\nIs it a storage based device like Nuvi, CO, or OR?\n  If so, send GPX files to it, don't use this module.\n", un);
      return 0;
    }
    /* We've matched.  Now start the specific unit. */
    garmin_usb_start(hdevinfo, &devinterface);
    return 1;
  }

  /*
   * Out unit nunber  is less than zero, so loop over all units
   * and display them.
   */
  for (match = 0;; match++) {
    if (!SetupDiEnumDeviceInterfaces(hdevinfo, NULL,
                                     &GARMIN_GUID, match, &devinterface)) {
      if (GetLastError() == ERROR_NO_MORE_ITEMS) {

        break;
      } else {

        GPS_Serial_Error("SetupDiEnumDeviceInterfaces");
        gbWarning("Is the Garmin USB unit number %d powered up and connected?\n", un);
        return 0;
      }
    }
    /* We've matched.  Now start the specific unit. */
    garmin_usb_start(hdevinfo, &devinterface);
    gusb_close(nullptr);
  }
  gusb_list_units();
  exit(0);
}
