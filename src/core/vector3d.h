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

#ifndef VECTOR3D_H
#define VECTOR3D_H

#include <QtCore/QDebug>

#include <iostream>

namespace gpsbabel
{

class Vector3D
{
public:
  Vector3D() {};
  Vector3D(double, double, double);

  double norm() const;
  double getx() const;
  double gety() const;
  double getz() const;
  Vector3D& normalize();

  Vector3D& operator=(const Vector3D&);
  Vector3D& operator+=(const Vector3D&);
  Vector3D& operator-=(const Vector3D&);
  Vector3D& operator*=(double);
  Vector3D& operator/=(double);
  Vector3D operator+ (const Vector3D&) const;
  Vector3D operator- (const Vector3D&) const;
  Vector3D operator* (double) const;
  Vector3D operator/ (double) const;
  Vector3D operator- () const;
  double operator* (const Vector3D&) const;

  static Vector3D crossProduct(const Vector3D&, const Vector3D&);
  static double dotProduct(const Vector3D&, const Vector3D&);

protected:
  double x;
  double y;
  double z;
};
Vector3D operator* (double, const Vector3D&);
std::ostream& operator<< (std::ostream& os, const Vector3D& v);
QDebug operator<<(QDebug debug, const Vector3D& v);

}
#endif // VECTOR3D_H
