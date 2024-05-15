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

#include "jeeps/gps.h"
#include "jeeps/garminusb.h"
#include "jeeps/gpsdevice.h"

class gpsusbdevh;  // Opaque

class GpsUsbDevice : public GpsDevice
{
public:
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
    return GPS_Packet_Read_usb(fd, packet, 1);
  }
  int32_t Packet_Read_bulk(GPS_Packet* packet)
  {
    return GPS_Packet_Read_usb(fd, packet, 0);
  }

private:
  int32_t GPS_Packet_Read_usb(gpsusbdevh* fd, GPS_Packet* packet, int eatbulk);
  int32_t GPS_Write_Packet_usb(gpsusbdevh* fd, const GPS_Packet& packet);

  gpsusbdevh* fd;
};
