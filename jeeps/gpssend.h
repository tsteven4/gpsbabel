#ifndef JEEPS_GPSSEND_H_INCLUDED_
#define JEEPS_GPSSEND_H_INCLUDED_


#include "jeeps/gps.h"

#define GPS_ARB_LEN 1024

void   GPS_Make_Packet(GPS_Packet* packet, US type, UC* data, uint32_t n);


#endif // JEEPS_GPSSEND_H_INCLUDED_
