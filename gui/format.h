// -*- C++ -*-
// $Id: format.h,v 1.6 2010-02-15 02:57:00 robertl Exp $
//------------------------------------------------------------------------
//
//  Copyright (C) 2009  S. Khai Mong <khai@mangrai.com>.
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License as
//  published by the Free Software Foundation; either version 2 of the
//  License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
//  USA.
//
//------------------------------------------------------------------------
#ifndef FORMAT_H
#define FORMAT_H

#include <QList>        // for QList
#include <QSettings>    // for QSettings
#include <QString>      // for QString
#include <QStringList>  // for QStringList
#include <QVariant>     // for QVariant
#include <vector>       // for vector

class FormatOption
{
public:

  /* Types */

  enum optionType {
    OPTstring,
    OPTbool,
    OPTint,
    OPTboundedInt,
    OPTfloat,
    OPTinFile,
    OPToutFile,
  };

  /* Special Member Functions */

  FormatOption() = default;
  FormatOption(const QString& name,
               const QString& description,
               optionType type,
               const QVariant& defaultValue = QVariant(),
               const QVariant& minValue = QVariant(),
               const QVariant& maxValue = QVariant(),
               const QString& html = QString()
              ):
    name_(name),
    description_(description),
    type_(type),
    defaultValue_(defaultValue),
    minValue_(minValue),
    maxValue_(maxValue),
    html_(html)
  {
    // Boolean values pay more attention to 'selected' than value.  Make
    // them match here. For non-bools, just make them unchecked.
    if ((type_ == OPTbool) && defaultValue_.toBool()) {
      isSelected_ = true;
    } else {
      isSelected_ = false;
    }
  }

  /* Member Functions */

  QString  getName() const
  {
    return name_;
  }
  QString  getDescription() const
  {
    return description_;
  }
  optionType getType() const
  {
    return type_;
  }
  QVariant getValue() const
  {
    return value_;
  }
  bool     getSelected() const
  {
    return isSelected_;
  }
  QVariant getMinValue() const
  {
    return minValue_;
  }
  QVariant getMaxValue() const
  {
    return maxValue_;
  }
  QVariant getDefaultValue() const
  {
    return defaultValue_;
  }

  void setValue(const QVariant& v)
  {
    value_ = v;
  }

  void setSelected(bool v)
  {
    isSelected_ = v;
  }

  QString getHtml() const
  {
    return html_;
  }

private:

  /* Data Members */

  QString name_;
  QString description_;
  optionType type_{OPTbool};
  QVariant defaultValue_;
  QVariant minValue_;
  QVariant maxValue_;
  QString  html_;
  QVariant value_;
  bool isSelected_{false};
};


//------------------------------------------------------------------------
class FormatList;
class Format
{
public:

  /* Special Member Functions */

  Format(const QString& name,
         const QString& description,
         bool readWaypoints,
         bool readTracks,
         bool readRoutes,
         bool writeWaypoints,
         bool writeTracks,
         bool writeRoutes,
         bool fileFormat,
         bool deviceFormat,
         const QStringList& extensions,
         QList<FormatOption>& inputOptions,
         QList<FormatOption>& outputOptions,
         const QString& html):
    name_(name),
    description_(description),
    readWaypoints_(readWaypoints),
    readTracks_(readTracks),
    readRoutes_(readRoutes),
    writeWaypoints_(writeWaypoints),
    writeTracks_(writeTracks),
    writeRoutes_(writeRoutes),
    fileFormat_(fileFormat),
    deviceFormat_(deviceFormat),

    extensions_(extensions),
    inputOptions_(inputOptions),
    outputOptions_(outputOptions)
  {
    (void)html; // suppress 'unused' warning.
  }

  /* Member Functions */

  bool isReadWaypoints() const
  {
    return readWaypoints_;
  }

  bool isReadTracks() const
  {
    return readTracks_;
  }

  bool isReadRoutes() const
  {
    return readRoutes_;
  }

  bool isReadSomething() const
  {
    return isReadWaypoints() || isReadTracks() || isReadRoutes();
  }

  bool isWriteWaypoints() const
  {
    return writeWaypoints_;
  }

  bool isWriteTracks() const
  {
    return writeTracks_;
  }

  bool isWriteRoutes() const
  {
    return writeRoutes_;
  }

  bool isWriteSomething() const
  {
    return isWriteWaypoints() || isWriteTracks() || isWriteRoutes();
  }

  QString getName() const
  {
    return name_;
  }

  QString getDescription() const
  {
    return description_;
  }

  QString getHtml() const
  {
    return html_;
  }

  QStringList getExtensions() const
  {
    return extensions_;
  }

  const QList<FormatOption>& getInputOptions()  const
  {
    return inputOptions_;
  }

  const QList<FormatOption>& getOutputOptions() const
  {
    return outputOptions_;
  }

  QList<FormatOption>* getInputOptionsRef()
  {
    return &inputOptions_;
  }

  QList<FormatOption>* getOutputOptionsRef()
  {
    return &outputOptions_;
  }

  bool isDeviceFormat() const
  {
    return deviceFormat_;
  }

  bool isFileFormat() const
  {
    return   fileFormat_;
  }

  bool isHidden() const
  {
    return hidden_;
  }

  void setHidden(bool state)
  {
    hidden_ = state;
  }

  void saveSettings(QSettings& settings);
  void restoreSettings(QSettings& settings);
  void setToDefault();
  static QString getHtmlBase()
  {
    return htmlBase_;
  }
  static void setHtmlBase(const QString& s)
  {
    htmlBase_ = s;
  }

  void bumpReadUseCount(int v)
  {
    readUseCount_ += v;
  }
  void bumpWriteUseCount(int v)
  {
    writeUseCount_ += v;
  }
  int getReadUseCount()  const
  {
    return readUseCount_;
  }
  int getWriteUseCount() const
  {
    return writeUseCount_;
  }
  void zeroUseCounts()
  {
    readUseCount_  = 0;
    writeUseCount_ = 0;
  }
  int getIndex() const
  {
    return idx_;
  }

private:

  /* Data Members */
  friend FormatList;

  int idx_{0};
  QString name_;
  QString description_;
  bool readWaypoints_{false};
  bool readTracks_{false};
  bool readRoutes_{false};
  bool writeWaypoints_{false};
  bool writeTracks_{false};
  bool writeRoutes_{false};
  bool fileFormat_{false};
  bool deviceFormat_{false};
  bool hidden_{false};
  QStringList extensions_;
  QList<FormatOption>inputOptions_;
  QList<FormatOption>outputOptions_;
  QString html_;
  static QString htmlBase_;
  int readUseCount_{0};
  int writeUseCount_{0};
};

/*
 * A container
 * 1) with a fixed number of elements
 * 2) with elements that are modifiable
 * 3) that is movable
 * 4) that is not copyable
 * 5) where each member contains it's own index
 *    into then list
 * 6) with iterators that are never invalidated
 *    by use of operator[]
 */

class FormatList : private std::vector<Format>
{
public:
  FormatList() : std::vector<Format>() {};
  FormatList(const FormatList&) = delete;
  FormatList& operator=(const FormatList&) = delete;
  FormatList(FormatList&&) = default;
  FormatList& operator=(FormatList&&) = default;

  FormatList(const std::vector<Format>&& qlist) : std::vector<Format>(qlist) // converting ctor
  {
    int idx = 0;
    for (auto& element : *this) {
      element.idx_ = idx++;
    }
  }

  // Expose limited methods for portability.
  // public types
  using typename std::vector<Format>::value_type;
  using typename std::vector<Format>::allocator_type;
  using typename std::vector<Format>::size_type;
  using typename std::vector<Format>::difference_type;
  using typename std::vector<Format>::reference;
  using typename std::vector<Format>::const_reference;
  using typename std::vector<Format>::pointer;
  using typename std::vector<Format>::const_pointer;
  using typename std::vector<Format>::iterator;
  using typename std::vector<Format>::const_iterator;
  using typename std::vector<Format>::reverse_iterator;
  using typename std::vector<Format>::const_reverse_iterator;
  // public functions
  using std::vector<Format>::back;
  using std::vector<Format>::begin;
  using std::vector<Format>::cbegin;
  using std::vector<Format>::cend;
  using std::vector<Format>::crbegin;
  using std::vector<Format>::crend;
  using std::vector<Format>::end;
  using std::vector<Format>::empty;
  using std::vector<Format>::front;
  using std::vector<Format>::rbegin;
  using std::vector<Format>::rend;
  using std::vector<Format>::size;
  using std::vector<Format>::operator[];
};
#endif
