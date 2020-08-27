/*
    Copyright (C) 2013 Robert Lipe, gpsbabel.org

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

#include "src/core/xmlstreamwriter.h"

#include <QtCore/QtGlobal>

#include <QtCore/QFile>
#include <QtCore/QString>
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
#  include <QtCore/QTextCodec>
#endif
#include <QtCore/QXmlStreamWriter>

// As this code began in C, we have several hundred places that write
// c strings.  Add a test that the string contains anything useful
// before serializing an empty tag.
// We also strip out characters that are illegal in xml.  These can creep
// into our structures from other formats where they are legal.

namespace gpsbabel
{

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  XmlTextCodec* XmlTextCodec::instance = new XmlTextCodec();
 
  XmlTextCodec::XmlTextCodec() : QTextCodec()
  {
    utf8Codec = QTextCodec::codecForName("UTF-8");
  }
 
  QByteArray XmlTextCodec::convertFromUnicode(const QChar* chars, int len, QTextCodec::ConverterState* state) const
  {
  // Qt 4.7.4, 4.6.2 don't have IgnoreHeader set on the first call, which can
  // result in a BOM being output by utf8Codec.
    state->flags |= QTextCodec::IgnoreHeader;
    QByteArray r = utf8Codec->fromUnicode(chars, len, state);
    char* data = r.data();
    for (int i = 0; i < r.size(); i++) {
      if ((0x00 <= data[i] && data[i] <= 0x08) ||
          (0x0b <= data[i] && data[i] <= 0x0c) ||
          (0x0e <= data[i] && data[i] <= 0x1f)) {
        data[i] = ' ';
      }
    }
    return r;
  }
 
  QString XmlTextCodec::convertToUnicode(const char* chars, int len, QTextCodec::ConverterState* state) const
  {
    return utf8Codec->toUnicode(chars, len, state);
  }
 
  int XmlTextCodec::mibEnum() const
  {
    return UTF8_FOR_XML_MIB;
  }
 
  // Our name must not overlap with UTF-8 or it may be returned by QTextCodec::codecForName("UTF-8")
  QByteArray XmlTextCodec::name() const
  {
    return QByteArray("UTF-8-XML");
  }
#endif
 
XmlStreamWriter::XmlStreamWriter(QString* string) : QXmlStreamWriter(string)
{
}
 
XmlStreamWriter::XmlStreamWriter(QFile* f) : QXmlStreamWriter(f)
{
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    setCodec(XmlTextCodec::instance);
#endif
  }

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  // We must override the encoding, we don't want to use XmlTextCode::name().
  void XmlStreamWriter::writeStartDocument()
  {
    writeProcessingInstruction(QStringLiteral("xml version=\"1.0\" encoding=\"UTF-8\""));
  }
#endif

// Dont emit the element if there's nothing interesting in it.
void XmlStreamWriter::writeOptionalTextElement(const QString& qualifiedName, const QString& text)
{
  if (!text.isEmpty()) {
    QXmlStreamWriter::writeTextElement(qualifiedName, text);
  }
}

} // namespace gpsbabel
