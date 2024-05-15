/*
    Abstraction of underlying device types.

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

#ifndef JEEPS_GPSDEVICE_H_INCLUDED_
#define JEEPS_GPSDEVICE_H_INCLUDED_

class GpsDevice;

#include "jeeps/gps.h"

int32_t GPS_Device_Chars_Ready(GpsDevice* fd);
int32_t GPS_Device_On(const char* port, GpsDevice** fd);
int32_t GPS_Device_Off(GpsDevice* fd);
int32_t GPS_Device_Wait(GpsDevice* fd);
int32_t GPS_Device_Flush(GpsDevice* fd);
int32_t GPS_Device_Read(int32_t ignored, void* ibuf, int size);
int32_t GPS_Device_Write(int32_t ignored, const void* obuf, int size);
void    GPS_Device_Error(char* hdr, ...);
int32_t GPS_Write_Packet(GpsDevice* fd, const GPS_Packet& packet);
bool    GPS_Send_Ack(GpsDevice* fd, GPS_Packet* tra, GPS_Packet* rec);
int32_t GPS_Packet_Read(GpsDevice* fd, GPS_Packet* packet);
bool    GPS_Get_Ack(GpsDevice* fd, GPS_Packet* tra, GPS_Packet* rec);

class GpsDevice
{
public:
  GpsDevice() = default;
  // Provide virtual public destructor to avoid undefined behavior when
  // an object of derived class type is deleted through a pointer to
  // its base class type.
  // https://wiki.sei.cmu.edu/confluence/display/cplusplus/OOP52-CPP.+Do+not+delete+a+polymorphic+object+without+a+virtual+destructor
  virtual ~GpsDevice() = default;
  // And that requires us to explicitly default or delete the move and copy operations.
  // To prevent slicing we delete them.
  // https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#c21-if-you-define-or-delete-any-default-operation-define-or-delete-them-all.
  GpsDevice(const GpsDevice&) = delete;
  GpsDevice& operator=(const GpsDevice&) = delete;
  GpsDevice(GpsDevice&&) = delete;
  GpsDevice& operator=(GpsDevice&&) = delete;

  virtual int32_t On(const char* port) = 0;
  virtual int32_t Off() = 0;
  virtual int32_t Chars_Ready()
  {
    return true;
  }
  virtual int32_t Wait()
  {
    return true;
  }
  virtual int32_t Flush()
  {
    return true;
  }
  virtual bool Send_Ack(GPS_Packet* /* tra */, GPS_Packet* /* rec */)
  {
    return true;
  }
  virtual bool Get_Ack(GPS_Packet* /* tra */, GPS_Packet* /* rec */)
  {
    return true;
  }
  virtual int32_t Packet_Read(GPS_Packet* packet) = 0;
  virtual int32_t Write_Packet(const GPS_Packet& packet) = 0;
};

#endif /* JEEPS_GPSDEVICE_H_INCLUDED_ */
