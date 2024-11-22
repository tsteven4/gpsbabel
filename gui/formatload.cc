// -*- c++ -*-
// $Id: formatload.cpp,v 1.3 2009-11-02 20:38:02 robertl Exp $
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

#include "formatload.h"
#include <QApplication>        // for QApplication
#include <QByteArray>          // for QByteArray
#include <QChar>               // for QChar, operator==
#include <QCoreApplication>    // for QCoreApplication
#include <QMessageBox>         // for QMessageBox
#include <QObject>             // for QObject
#include <QProcess>            // for QProcess
#include <QRegularExpression>  // for QRegularExpression, QRegularExpressionMatch
#include <QString>             // for QString, operator+
#include <QTextStream>         // for QTextStream
#include <QVariant>            // for QVariant
#include <utility>             // for move
#include "appname.h"           // for appNam


#ifdef GENERATE_CORE_STRINGS
extern QTextStream* generate_output_stream;
#endif

//------------------------------------------------------------------------
static QString xlt(const QString& f)
{
#ifdef GENERATE_CORE_STRINGS
  *generate_output_stream << "QT_TRANSLATE_NOOP(\"core\",\"" << f << "\")" << Qt::endl;
#endif
  return QCoreApplication::translate("core", f.toUtf8().constData());
}

//------------------------------------------------------------------------
bool FormatLoad::skipToValidLine()
{
  static const QRegularExpression regex("^file|serial");
  while ((currentLine_ < lines_.size()) && !regex.match(lines_[currentLine_]).hasMatch()) {
    currentLine_++;
  }
  return (currentLine_<lines_.size());
}

//------------------------------------------------------------------------
Format FormatLoad::processFormat()
{
  QStringList hfields = lines_[currentLine_++].split("\t");
  if (hfields.size() < 5) {
    throw ProcessFormatException("malformed response from gpsbabel");
  }
  QString htmlPage = lines_[currentLine_++].trimmed();

  QList <FormatOption> optionList;
  while ((currentLine_ < lines_.size()) && lines_[currentLine_].startsWith("option")) {
    QStringList ofields = lines_[currentLine_].split("\t");
    if (ofields.size() < 9) {
      throw ProcessFormatException("malformed response from gpsbabel");
    }
    QString name        = ofields[2];
    QString description = ofields[3];
    QString optionType  = ofields[4];
    QString optionDef   = ofields[5];
    QString optionMin   = ofields[6];
    QString optionMax   = ofields[7];
    QString optionHtml  = ofields[8];
    FormatOption::optionType type = FormatOption::OPTbool;
    if (optionType == "boolean") {
      type = FormatOption::OPTbool;
    } else if (optionType == "string") {
      type = FormatOption::OPTstring;
    } else if (optionType == "integer") {
      type = (optionMax != "" && optionMin != "") ? FormatOption::OPTboundedInt : FormatOption::OPTint;
      if (optionMax == "") {
        optionMax = "2147483647";
      }
      if (optionMin == "") {
        optionMin = "-2147483647";
      }
    } else if (optionType == "float") {
      type = FormatOption::OPTfloat;
      if (optionMax == "") {
        optionMax = "1.0E308";
      }
      if (optionMin == "") {
        optionMin = "-1.0E308";
      }
    } else if (optionType == "file") {
      type = FormatOption::OPTinFile;
    } else if (optionType == "outfile") {
      type = FormatOption::OPToutFile;
    } else {
      type = FormatOption::OPTstring;
    }
    optionList << FormatOption(name, xlt(description),
                               type, QVariant(optionDef), QVariant(optionMin),
                               QVariant(optionMax), optionHtml);
    currentLine_++;
  }
  QList <FormatOption> optionList2 = optionList;

  Format format = Format(hfields[2], xlt(hfields[4]),
                  hfields[1][0] == QChar('r'),  hfields[1][2] == QChar('r'),  hfields[1][4] == QChar('r'),
                  hfields[1][1] == QChar('w'),  hfields[1][3] == QChar('w'),  hfields[1][5] == QChar('w'),
                  hfields[0] == "file",
                  hfields[0] == "serial",
                  hfields[3].split('/'),
                  optionList,
                  optionList2, htmlPage);
#ifndef GENERATE_CORE_STRINGS
  if (htmlPage.length() > 0 && Format::getHtmlBase().length() == 0) {
    QString base = htmlPage;
    static const QRegularExpression re("/[^/]+$");
    base.replace(re, "/");
    Format::setHtmlBase(base);
  }
#endif
  return format;
}

//------------------------------------------------------------------------
QList<Format> FormatLoad::getFormats()
{
  QList<Format> formatList;

  QProcess babel;
  babel.start(QApplication::applicationDirPath() + "/gpsbabel", QStringList() << "-^3");
  if (!babel.waitForStarted()) {
    throw FormatLoadException("gpsbabel did not start");
  }
  babel.closeWriteChannel();
  if (!babel.waitForFinished()) {
    throw FormatLoadException("gpsbabel did not finish");
  }
  if (babel.exitCode() != 0) {
    throw FormatLoadException("gpsbabel returned an error");
  }

  QTextStream tstream(babel.readAll());
  QList<int>lineList;
  int k=0;
  while (!tstream.atEnd()) {
    QString l = tstream.readLine();
    k++;
    if (!l.trimmed().isEmpty()) {
      lines_ << l;
      lineList<<k;
    }
  }
  currentLine_ = 0;

  for (bool dataPresent = skipToValidLine(); dataPresent; dataPresent=skipToValidLine()) {
    try {
      Format format = processFormat();
      formatList.push_back(format);
    } catch (ProcessFormatException& /* e */) {
      QMessageBox::information
      (nullptr, appName,
       QObject::tr("Error processing formats from running process \"gpsbabel -^3\" at line %1").arg(lineList[currentLine_]));
    }
  }
  return std::move(formatList);
}
