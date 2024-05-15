#ifndef JEEPS_GPSSERIAL_H_INCLUDED_
#define JEEPS_GPSSERIAL_H_INCLUDED_

#include <QSerialPort>

#include "jeeps/gps.h"

class GpsSerialDevice : public GpsDevice
{
public:
  static constexpr int DEFAULT_BAUD = 9600;

  int32_t Chars_Ready() override;
  int32_t On(const char* port) override;
  int32_t Off() override;
  int32_t Wait() override;
  int32_t Flush() override;
  int32_t Set_Baud_Rate(int br);
  int32_t Write_Packet(const GPS_Packet& packet) override;
  bool Send_Ack(GPS_Packet* tra, GPS_Packet* rec) override;
  int32_t Packet_Read(GPS_Packet* packet) override;
  bool Get_Ack(GPS_Packet *tra, GPS_Packet *rec) override;

private:
  static constexpr int msecDELAY = 180;	/* Milliseconds before GPS sends A001 */

  [[gnu::format(printf, 2, 3)]] void Error(const char* fmt, ...);
  int32_t Chars_Ready_After(int msec_timeout);
  int32_t Read(void* ibuf, int size);
  int32_t Write(const void* obuf, int size);
  static US Build_Serial_Packet(const GPS_Packet& in, GPS_Serial_Packet* out);
  static void Diag(void* buf, size_t sz);
  static void DiagS(void* buf, size_t sz);

  QSerialPort sp;
};
#endif // JEEPS_GPSSERIAL_H_INCLUDED_
