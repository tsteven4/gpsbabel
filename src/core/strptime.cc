/*
    Copyright (C) 2023 Robert Lipe, robertlipe+source@gpsbabel.org

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

#include <cstring>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace gpsbabel
{
char* strptime(const char* buf, const char* format, struct tm* tm)
{
  // For efficiency assume the locale is constant.
  static auto current_locale = std::locale();

  std::istringstream bufstream(buf);
  bufstream.imbue(current_locale);
  bufstream >> std::get_time(tm, format);
  if (bufstream.fail()) {
    return nullptr;
  }
  if (bufstream.eof()) {
    return const_cast<char*>(buf + strlen(buf));
  }
  return const_cast<char*>(buf + bufstream.tellg());
}
} // namespace gpsbabel
