/*
    Form GarminUSB packets to send.

    Copyright (C) 2004, 2005, 2006 Robert Lipe, robertlipe@usa.net

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
#include "jeeps/gpsusbint.h"
#include <cerrno>
#include <cstdio>

int32_t
GPS_Write_Packet_usb(gpsdevh* /*unused*/, const GPS_Packet& packet)
{
  garmin_usb_packet gp;
  memset(&gp, 0, sizeof(gp));


  /*
   * Take the "portable" GPS_Packet data and put them into
   * the USB packet that we will put on the wire.
   */
  gp.gusb_pkt.type = 0x14;
  le_write16(&gp.gusb_pkt.pkt_id, packet.type);
  le_write32(&gp.gusb_pkt.datasz, packet.n);
  memcpy(&gp.gusb_pkt.databuf, packet.data, packet.n);

  return  gusb_cmd_send(&gp, packet.n + 12);
}
