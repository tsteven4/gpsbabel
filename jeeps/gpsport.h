/*
 *  For portability any '32' type must be 32 bits
 *                  and '16' type must be 16 bits
 */

/* Since GPSBabel already has an integer size abstraction layer and
 * defs.h includes gbtypes.h before this file, just use that.
 */

#ifndef JEEPS_GPSPORT_H_INCLUDED_
#define JEEPS_GPSPORT_H_INCLUDED_

#include <cstdint>

using UC = unsigned char;
using US = uint16_t;
using uint16 = uint16_t;
using int16 = int16_t;
using uint32 = uint32_t;
using int32 = int32_t;

#endif // JEEPS_GPSPORT_H_INCLUDED_
