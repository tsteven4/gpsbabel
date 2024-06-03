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
#ifndef JEEPS_GPSUSBWIN_H_INCLUDED_
#define JEEPS_GPSUSBWIN_H_INCLUDED_

#include <cstdio>                 // for size_t

#include <windows.h>              // Boost this out of alpha order.
#include <setupapi.h>             // for HDEVINFO, SP_DEVICE_INTERFACE_DATA

#include "jeeps/gpsdevice_usb.h"  // for gpsusbdevh, GpsUsbDevice

class GpsWinusb : public GpsUsbDevice
{
public:
  int llop_get_intr(garmin_usb_packet* ibuf, size_t sz) override
  {
    return gusb_win_get(ibuf, sz);
  }
  int llop_get_bulk(garmin_usb_packet* ibuf, size_t sz) override
  {
    return gusb_win_get_bulk(ibuf, sz);
  }
  int llop_send(const garmin_usb_packet* opkt, size_t sz) override
  {
    return gusb_win_send(opkt, sz);
  }
  int llop_close(gpsusbdevh* dh, bool exit_lib) override
  {
    return gusb_win_close(dh, exit_lib);
  }

private:
  /* Types */

  struct winusb_unit_data {
    int booger;
  };

  /* Constants */


  /* Member Functions */

  int gusb_win_get(garmin_usb_packet* ibuf, size_t sz);
  int gusb_win_get_bulk(garmin_usb_packet* ibuf, size_t sz);
  int gusb_win_close(gpsusbdevh* dh, bool exit_lib);
  int gusb_win_send(const garmin_usb_packet* opkt, size_t sz);
  int gusb_init(const char* portname, gpsusbdevh** dh) override;
  HANDLE garmin_usb_start(HDEVINFO hdevinfo, SP_DEVICE_INTERFACE_DATA* infodata);
  [[gnu::format(printf, 1, 2)]] static void GPS_Serial_Error(const char* fmt, ...);

  /* Data Members */

  HANDLE usb_handle = INVALID_HANDLE_VALUE;
  int usb_tx_packet_size ;
};
#endif // JEEPS_GPSUSBWIN_H_INCLUDED_
