// -*- C++ -*-
// $Id: main.cpp,v 1.8 2010-06-06 00:49:08 robertl Exp $
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
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111
//  USA
//
//------------------------------------------------------------------------
#include <QtCore/QByteArray>        // for operator+, QByteArray
#include <QtCore/QCoreApplication>  // for QCoreApplication
#include <QtCore/QDir>              // for QDir
#include <QtCore/QString>           // for QString
#include <QtCore/QtGlobal>          // for qgetenv, qputenv, QT_VERSION, QT_VERSION_CHECK
#include <QtGui/QIcon>              // for QIcon
#include <QtWidgets/QApplication>   // for QApplication

#include "mainwindow.h"

//------------------------------------------------------------------------
int main(int argc, char**argv)
{
// MIN_QT_VERSION in configure.ac should correspond to the QT_VERSION_CHECK arguments in main.cc and gui/main.cc
#if (QT_VERSION < QT_VERSION_CHECK(5, 9, 0))
  #error this version of Qt is not supported.
#endif

  QApplication app(argc, argv);
  QApplication::setWindowIcon(QIcon(":/images/appicon.png"));

#ifdef _WIN32
  const QByteArray pathSeparator(";");
#else
  const QByteArray pathSeparator(":");
#endif
  qputenv("PATH",
          QDir::toNativeSeparators(QApplication::applicationDirPath()).toUtf8()
          + pathSeparator
          + qgetenv("PATH"));

  QCoreApplication::setOrganizationName("GPSBabel");
  QCoreApplication::setOrganizationDomain("gpsbabel.org");
  QCoreApplication::setApplicationName("GPSBabel");

  MainWindow mainWindow(nullptr);
  mainWindow.show();
  QApplication::exec();

  return 0;
}
