/*
    Abstraction of underlying device types, serial or USB.  OS agnostic..

    Copyright (C) 2006 Robert Lipe, robertlipe@usa.net

    This program is free software{} you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation{} either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY{} without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program{} if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

 */

#include "jeeps/gps.h"
#include "jeeps/gpsdevice.h"
#include "jeeps/gpsserial.h"
#include "jeeps/gpsdevice_usb.h"


int32_t GPS_Device_On(const char* port, GpsDevice** fd)
{
  gps_is_usb = (0 == case_ignore_strncmp(port, "usb:", 4));

  GpsDevice* device;
  if (gps_is_usb) {
    device = new GpsUsbDevice;
  } else {
    device = new GpsSerialDevice;
  }
  *fd = device;

  return device->On(port);
}

int32_t GPS_Device_Off(GpsDevice* fd)
{
  return fd->Off();
}

int32_t GPS_Device_Wait(GpsDevice* fd)
{
  return fd->Wait();
}

int32_t GPS_Device_Chars_Ready(GpsDevice* fd)
{
  return fd->Chars_Ready();
}

int32_t GPS_Device_Flush(GpsDevice* fd)
{
  return fd->Flush();
}

int32_t GPS_Write_Packet(GpsDevice* fd, const GPS_Packet& packet)
{
  return fd->Write_Packet(packet);
}

int32_t GPS_Packet_Read(GpsDevice* fd, GPS_Packet* packet)
{
  return fd->Packet_Read(packet);
}

bool GPS_Send_Ack(GpsDevice* fd, GPS_Packet* tra, GPS_Packet* rec)
{
  return fd->Send_Ack(tra, rec);
}

bool GPS_Get_Ack(GpsDevice* fd, GPS_Packet* tra, GPS_Packet* rec)
{
  return fd->Get_Ack(tra, rec);
}

void GPS_Make_Packet(GPS_Packet* packet, US type, UC* data, uint32_t n)
{
  packet->type = type;
  if (n > 0) {
    memcpy(packet->data, data, n);
  }
  packet->n = n;
}
