/*
    Physical/OS USB layer to talk to libusb.

    Copyright (C) 2004, 2005, 2006, 2007, 2008  Robert Lipe, robertlipe@usa.net

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

#ifndef JEEPS_GPSLIBUSB_H_INCLUDED_
#define JEEPS_GPSLIBUSB_H_INCLUDED_

#if HAVE_LIBUSB_1_0

#include <cstdint>                // for uint16_t
#include <cstdio>                 // for size_t
#ifdef LIBUSB_H_INCLUDE
// Warning: LIBUSB_H_INCLUDE necessarily includes bracket or double quote
//          characters.
//          qmake -tp vc doesn't properly quote these characters, and the
//          produced project file is invalid.  However, we don't use libusb
//          at all on windows, so this isn't an issue in this application.
#  include LIBUSB_H_INCLUDE
#endif
#include "jeeps/gpsdevice_usb.h"  // for gpsusbdevh, GpsUsbDevice


class GpsLibusb : public GpsUsbDevice
{
public:
  int llop_get_intr(garmin_usb_packet* ibuf, size_t sz) override
  {
    return gusb_libusb_get(ibuf, sz);
  }
  int llop_get_bulk(garmin_usb_packet* ibuf, size_t sz) override
  {
    return gusb_libusb_get_bulk(ibuf, sz);
  }
  int llop_send(const garmin_usb_packet* opkt, size_t sz) override
  {
    return gusb_libusb_send(opkt, sz);
  }
  int llop_close(gpsusbdevh* dh, bool exit_lib) override
  {
    return gusb_teardown(dh, exit_lib);
  }

private:
  /* Types */

  struct libusb_unit_data {
    unsigned product_id;
  };

  /* Constants */

  static constexpr uint16_t GARMIN_VID = 0x91e;

  /* This is very sensitive to timing; libusb and/or the unit is kind of
   * sloppy about not obeying packet boundaries.  If this is too high, the
   * multiple packets responding to the device inquiry will be glommed into
   * one packet and we'll misparse them.  If it's too low, we'll get partially
   * satisfied reads.  It turns out this isn't terrible because we still end
   * up with DLE boundings and the upper layers (which are used to doing frame
   * coalescion into packets anyway because of their serial background) will
   * compensate.
   */
  static constexpr unsigned int TMOUT_I = 5000; /*  Milliseconds to timeout intr pipe access. */
  static constexpr unsigned int TMOUT_B = 5000; /*  Milliseconds to timeout bulk pipe access. */

  /* Member Functions */

  int gusb_libusb_get(garmin_usb_packet* ibuf, size_t sz);
  int gusb_libusb_get_bulk(garmin_usb_packet* ibuf, size_t sz);
  int gusb_teardown(gpsusbdevh* dh, bool exit_lib);
  int gusb_libusb_send(const garmin_usb_packet* opkt, size_t sz);
  int gusb_init(const char* portname, gpsusbdevh** dh) override;
  int garmin_usb_scan(libusb_unit_data* lud, int req_unit_number);
  void garmin_usb_start(struct libusb_device* dev,
                        struct libusb_device_descriptor* desc,
                        libusb_unit_data* lud);
  unsigned gusb_reset_toggles();

  /* Data Members */

  unsigned char gusb_intr_in_ep;
  unsigned char gusb_bulk_out_ep;
  unsigned char gusb_bulk_in_ep;

  bool libusb_successfully_initialized{false};
  libusb_device_handle* udev{nullptr};
};

#endif /* HAVE_LIBUSB_1_0 */
#endif /* JEEPS_GPSLIBUSB_H_INCLUDED_ */

