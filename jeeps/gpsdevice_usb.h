/*
    USB operations.

    Copyright (C) 2006 Robert Lipe, robertlipe@usa.net

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
#ifndef JEEPS_GPSDEVICE_USB_H_INCLUDED_
#define JEEPS_GPSDEVICE_USB_H_INCLUDED_

#include <stddef.h>           // for size_t
#include <stdint.h>           // for int32_t
#include "jeeps/gpsdevice.h"  // for GpsDevice
#include "jeeps/gpssend.h"    // for GPS_Packet


class gpsusbdevh;  // Opaque

class GpsUsbDevice : public GpsDevice
{
public:
  /* Constants */

  /*
   * New packet types in USB.
   */
  static constexpr int GUSB_SESSION_START = 5;  /* We request units attention */
  static constexpr int GUSB_SESSION_ACK = 6;    /* Unit responds that we have its attention */
  static constexpr int GUSB_REQUEST_BULK = 2;   /* Unit requests we read from bulk pipe */

  /* Special Member Functions */

  GpsUsbDevice() = default;
  // Provide virtual public destructor to avoid undefined behavior when
  // an object of derived class type is deleted through a pointer to
  // its base class type.
  // https://wiki.sei.cmu.edu/confluence/display/cplusplus/OOP52-CPP.+Do+not+delete+a+polymorphic+object+without+a+virtual+destructor
  virtual ~GpsUsbDevice() = default;
  // And that requires us to explicitly default or delete the move and copy operations.
  // To prevent slicing we delete them.
  // https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#c21-if-you-define-or-delete-any-default-operation-define-or-delete-them-all.
  GpsUsbDevice(const GpsUsbDevice&) = delete;
  GpsUsbDevice& operator=(const GpsUsbDevice&) = delete;
  GpsUsbDevice(GpsUsbDevice&&) = delete;
  GpsUsbDevice& operator=(GpsUsbDevice&&) = delete;

  /* Member Functions */

  int32_t On(const char* port) override
  {
    return gusb_init(port, &fd);
  }
  int32_t Off() override
  {
    return gusb_close(fd);
  }
  int32_t Write_Packet(const GPS_Packet& packet) override
  {
    return GPS_Write_Packet_usb(fd, packet);
  }
  int32_t Packet_Read(GPS_Packet* packet) override
  {
    /* Default is to eat bulk request packets. */
    return GPS_Packet_Read_usb(fd, packet, true);
  }
  int32_t Packet_Read_bulk(GPS_Packet* packet)
  {
    return GPS_Packet_Read_usb(fd, packet, false);
  }

protected:
  /* Types */

  /* This structure is a bit funny looking to avoid variable length
   * arrays which aren't present in C89.   This contains the visible
   * fields in the USB packets of the Garmin USB receivers (60C, 76C, etc.)
   * All data are little endian.
   */
  union garmin_usb_packet {
    struct {
      unsigned char type;
      unsigned char reserved1;
      unsigned char reserved2;
      unsigned char reserved3;
      unsigned char pkt_id[2];
      unsigned char reserved6;
      unsigned char reserved7;
      unsigned char datasz[4];
      unsigned char databuf[1]; /* actually an variable length array... */
    } gusb_pkt;
    unsigned char dbuf[1024];
  };

  struct garmin_unit_info_t {
    unsigned long serial_number;
    unsigned long unit_id;
    unsigned long unit_version;
    char* os_identifier; /* In case the OS has another name for it. */
    char* product_identifier; /* From the hardware itself. */
  };

  /* Constants */
  static constexpr int GUSB_MAX_UNITS = 20;

  /* Member Functions */

  int gusb_close(gpsusbdevh* dh, bool exit_lib = true);
  int gusb_cmd_get(garmin_usb_packet* ibuf, size_t sz);
  int gusb_cmd_send(const garmin_usb_packet* obuf, size_t sz);
  void gusb_list_units();
  void gusb_id_unit(garmin_unit_info_t* gu);
  void gusb_syncup();

  virtual int gusb_init(const char* portname, gpsusbdevh** dh) = 0;
  virtual int llop_get_intr(garmin_usb_packet* ibuf, size_t sz) = 0;
  virtual int llop_get_bulk(garmin_usb_packet* ibuf, size_t sz) = 0;
  virtual int llop_send(const garmin_usb_packet* opkt, size_t sz) = 0;
  virtual int llop_close(gpsusbdevh* dh, bool exit_lib) = 0;

  /* Data Members */

  int max_tx_size{0};

private:
  /* Member Functions */

  int32_t GPS_Packet_Read_usb(gpsusbdevh* fd, GPS_Packet* packet, bool eat_bulk);
  int32_t GPS_Write_Packet_usb(gpsusbdevh* fd, const GPS_Packet& packet);

  /* Data Members */

  gpsusbdevh* fd{nullptr};
  garmin_unit_info_t garmin_unit_info[GUSB_MAX_UNITS];
};
#endif // JEEPS_GPSDEVICE_USB_H_INCLUDED_
