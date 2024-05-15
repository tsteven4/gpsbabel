/********************************************************************
** @source JEEPS serial port low level functions
**
** @author Copyright (C) 1999,2000 Alan Bleasby
** @version 1.0
** @modified December 28th 1999 Alan Bleasby. First version
** @modified June 29th 2000 Alan Bleasby. NMEA additions
** @modified Copyright (C) 2006 Robert Lipe
** @@
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Library General Public
** License as published by the Free Software Foundation; either
** version 2 of the License, or (at your option) any later version.
**
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Library General Public License for more details.
**
** You should have received a copy of the GNU Library General Public
** License along with this library; if not, write to the
** Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
** Boston, MA  02110-1301, USA.
********************************************************************/
#include "jeeps/gpsserial.h"

#include <cstdarg>          // for va_end, va_list, va_start
#include <cstdio>           // for fprintf, vsnprintf, stderr, va_list

#include <QByteArray>       // for QByteArray
#include <QIODeviceBase>    // for QIODeviceBase, QIODeviceBase::ReadWrite
#include <QSerialPort>      // for QSerialPort, QSerialPort::AllDirections
#include <QString>          // for QString
#include <QThread>          // for QThread
#include <QtGlobal>         // for qint64

#include "jeeps/gps.h"

int gps_baud_rate = GpsSerialDevice::DEFAULT_BAUD;

/*
 * Display an error from the serial subsystem.
 */
[[gnu::format(printf, 2, 3)]] void GpsSerialDevice::Error(const char* fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);

  QString msg = QString::vasprintf(fmt, ap);
  msg.append(": ");
  msg.append(sp.errorString());

  Error("%s", qPrintable(msg));

  va_end(ap);
}

int32_t GpsSerialDevice::On(const char* port)
{
  GPS_Diag("Opening %s\n", port);
  sp.setPortName(port);
  bool ok = sp.open(QIODeviceBase::ReadWrite);
  if (!ok) {
   Error("Cannot open serial port '%s'", port);
    gps_errno = SERIAL_ERROR;
    return 0;
  }
  ok = sp.setDataTerminalReady(true);
  if (!ok) {
    Error("Couldn't set DTR");
  }
  ok = sp.setRequestToSend(true);
  if (!ok) {
    Error("Couldn't set RTS");
  }
#if 0
  GPS_Diag("baudRate %d\n", sp.baudRate());
  GPS_Diag("parity %d\n", sp.parity());
  GPS_Diag("stopBits %d\n", sp.stopBits());
  GPS_Diag("flow control %d\n", sp.flowControl());
  GPS_Diag("break enabled %d\n", sp.isBreakEnabled());
  GPS_Diag("pinout signals 0x%0x\n", sp.pinoutSignals());
  GPS_Diag("read buffer %d\n", sp.readBufferSize());
  GPS_Diag("dtr %d\n", sp.isDataTerminalReady());
  GPS_Diag("rts %d\n", sp.isRequestToSend());

  GPS_Diag("GPS_Serial_On error", sp.error());
#endif

  return 1;
}

int32_t GpsSerialDevice::Off()
{
  sp.close();
  return 1;
}

int32_t GpsSerialDevice::Chars_Ready_After(int msec_timeout)
{
  /* If no bytes are available call waitForRead()
   * in order to process IO in blocking mode.
   * This may result in bytes becoming available.
   * Don't process IO unless necessary as it negatively
   * impacts performance.
   */
  if (sp.bytesAvailable() <= 0) {
    bool ok = sp.waitForReadyRead(msec_timeout);
    if (!ok) {
      sp.clearError();
    }
  }
  return sp.bytesAvailable() > 0;
}

int32_t GpsSerialDevice::Chars_Ready()
{
  return Chars_Ready_After(1);
}

int32_t GpsSerialDevice::Wait()
{
  /* Wait a short time before testing if data is ready.
   * The GPS II, in particular, has a noticable time responding
   * with a response to the device inquiry and if we give up on this
   * too soon, we fail to read the response to the A001 packet and
   * blow our state machines when it starts streaming the capabiilties
   * response packet.
   */
  return Chars_Ready_After(msecDELAY);
}

int32_t GpsSerialDevice::Flush()
{
  bool ok = sp.clear(QSerialPort::AllDirections);
  if (!ok) {
    Error("SERIAL: flush error");
    gps_errno = SERIAL_ERROR;
  }

  return ok;
}

int32_t GpsSerialDevice::Write(const void* obuf, int size)
{
  /*
   * Unbelievably, the Keyspan PDA serial driver 3.2, a "Windows
   * Certified driver", will crash the OS on a write of zero bytes.
   * We get such writes from upstream when there are zero payload
   * bytes.  SO we trap those here to stop Keyspan & Windows from
   * nuking the system.
   */
  if (size == 0) {
    return 0;
  }

  qint64 len = sp.write(static_cast<const char*>(obuf), size);

  if (len != size) {
    fatal("Write error.  Wrote %d of %d bytes.\n", static_cast<int>(len), size);
  }

  /* Call waitForBytesWritten to process IO in blocking mode.
   */
  bool ok = sp.waitForBytesWritten(10);
  if (!ok) {
    sp.clearError();
  }
  return len;
}

int32_t GpsSerialDevice::Read(void* ibuf, int size)
{
  /* Process IO */
  (void) Chars_Ready();

  qint64 cnt = sp.read(static_cast<char*>(ibuf), size);
  return cnt;
}

// Based on information by Kolesár András from
// http://www.manualslib.com/manual/413938/Garmin-Gps-18x.html?page=32
int32_t GpsSerialDevice::Set_Baud_Rate(int br)
{
  static UC data[4];
  GPS_Packet tra;
  GPS_Packet rec;

  // Turn off all requests by transmitting packet
  GPS_Util_Put_Short(data, 0);
  GPS_Make_Packet(&tra, 0x1c, data, 2);
  if (!Write_Packet(tra)) {
    return gps_errno;
  }
  if (!Get_Ack(&tra, &rec)) {
    return gps_errno;
  }

  GPS_Util_Put_Int(data, br);
  GPS_Make_Packet(&tra, 0x30, data, 4);
  if (!Write_Packet(tra)) {
    return gps_errno;
  }
  if (!Get_Ack(&tra, &rec)) {
    return gps_errno;
  }

  // Receive IOP_BAUD_ACPT_DATA
  if (!Packet_Read(&rec)) {
    return gps_errno;
  }

  // Acknowledge new speed
  if (!Send_Ack(&tra, &rec)) {
    return gps_errno;
  }
  Flush();
  Wait();

  // Sleep for a small amount of time, about 100 milliseconds,
  // to make sure the packet was successfully transmitted to the GPS unit.
  QThread::usleep(100000);

  // Change port speed
  bool ok = sp.setBaudRate(br);
  if (!ok) {
    Error("setBaudRate failed");
    sp.close();
    return 0;
  }

  GPS_Util_Put_Short(data, 0x3a);
  GPS_Make_Packet(&tra, 0x0a, data, 2);
  if (!Write_Packet(tra)) {
    return gps_errno;
  }
  if (!Get_Ack(&tra, &rec)) {
    return gps_errno;
  }

  GPS_Util_Put_Short(data, 0x3a);
  GPS_Make_Packet(&tra, 0x0a, data, 2);
  if (!Write_Packet(tra)) {
    return gps_errno;
  }
  if (!Get_Ack(&tra, &rec)) {
    return gps_errno;
  }

  if (global_opts.debug_level >= 1) {
    fprintf(stderr, "Serial port speed set to %d\n", br);
  }
  return 0;

}
