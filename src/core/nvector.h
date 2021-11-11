/*
    Copyright (C) 2018 Robert Lipe, gpsbabel.org

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
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111 USA

 */

#ifndef NVECTOR_H
#define NVECTOR_H

#include "vector3d.h"

namespace gpsbabel
{

#define COMPARE_BEARING_TO_GRTCIRC
#ifdef COMPARE_BEARING_TO_GRTCIRC
constexpr double MEAN_EARTH_RADIUS_METERS = 6378137.0;
#else
constexpr double MEAN_EARTH_RADIUS_METERS = 6371000.0;
#endif
constexpr double WGS84_SEMI_MAJOR_AXIS_METERS = 6378137.0; // a
#ifdef COMPARE_BEARING_TO_GRTCIRC
constexpr double WGS84_FLATTENING = 0.0;
#else
constexpr double WGS84_FLATTENING = 1.0/298.257223563; // (a-b)/a
#endif
constexpr double WGS84_ASPECT_RATIO = 1.0 - WGS84_FLATTENING; // b/a
constexpr double WGS84_SEMI_MINOR_AXIS_METERS = WGS84_SEMI_MAJOR_AXIS_METERS * WGS84_ASPECT_RATIO; // b
constexpr double WGS84_ECCENTRICITY_SQUARED = 1.0 - (WGS84_ASPECT_RATIO * WGS84_ASPECT_RATIO);

class PVector;

class LatLon
{
public:
  LatLon(double, double);

  double lat;
  double lon;
};

class NVector : public Vector3D
{
//  friend class PVector;

public:
  NVector() {};
  NVector(double, double);
  NVector(const Vector3D&);
  NVector(const PVector&);

  double latitude() const;
  double longitude() const;

  static double distance_radians(const NVector&, const NVector&);
  static double distance(const NVector&, const  NVector&);
  static double distance(double, double, double, double);
  static double azimuth_radians(const NVector&, const NVector&, double height_a = 0.0, double height_b = 0.0);
  static double azimuth(const NVector&, const NVector&, double height_a = 0.0, double height_b = 0.0);
  static double azimuth(double, double, double, double, double height_a = 0.0, double height_b = 0.0);
  static double azimuth_true_degrees(const NVector&, const NVector&, double height_a = 0.0, double height_b = 0.0);
  static double azimuth_true_degrees(double, double, double, double, double height_a = 0.0, double height_b = 0.0);
  static NVector linepart(const NVector&, const NVector&, double);
  static LatLon linepart(double, double, double, double, double);
  static NVector crossTrackProjection(const NVector&, const NVector&, const NVector&);
  static LatLon crossTrackProjection(double, double, double, double, double, double);
  static double crossTrackDistance(const NVector&, const NVector&, const NVector&);
  static double crossTrackDistance(double, double, double, double, double, double);

private:
  double height;
};

class PVector : public Vector3D
{
public:
  PVector() {};
  PVector(const NVector&, double);
};

}
#endif // NVECTOR_H
