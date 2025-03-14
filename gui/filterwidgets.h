// -*- C++ -*-
// $Id: filterwidgets.h,v 1.2 2009-11-02 20:38:02 robertl Exp $
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
#ifndef FILTERWIDGETS_H
#define FILTERWIDGETS_H

#include <QAbstractButton>   // for QAbstractButton
#include <QComboBox>         // for QComboBox
#include <QDateTime>         // for QDateTime
#include <QDateTimeEdit>     // for QDateTimeEdit
#include <QDoubleValidator>  // for QDoubleValidator
#include <QFunctionPointer>  // for qMax, qMin
#include <QLineEdit>         // for QLineEdit
#include <QList>             // for QList
#include <QObject>           // for QObject, Q_OBJECT, slots
#include <QSpinBox>          // for QSpinBox
#include <QString>           // for QString
#include <QWidget>           // for QWidget

#include <memory>            // for unique_ptr
#include <utility>           // for move
#include <vector>            // for vector

#include "filterdata.h"      // for MiscFltFilterData, RtTrkFilterData, TrackFilterData, WayPtsFilterData
#include "ui_miscfltui.h"    // for Ui_MiscFltWidget
#include "ui_rttrkui.h"      // for Ui_RtTrkWidget
#include "ui_trackui.h"      // for Ui_TrackWidget
#include "ui_wayptsui.h"     // for Ui_WayPtsWidget


//------------------------------------------------------------------------
class CheckEnabler: public QObject
{
  Q_OBJECT
public:
  CheckEnabler(QObject* parent, QAbstractButton* ck, QWidget* w): QObject(parent), checkBox(ck)
  {
    widgetList << w;
    connect(ck, &QAbstractButton::clicked, this, &CheckEnabler::checkStatusChanged);
    checkStatusChanged();
    fixWhatsThis();
  }
  CheckEnabler(QObject* parent, QAbstractButton* ck, QList<QWidget*>& wl):
    QObject(parent), checkBox(ck), widgetList(wl)
  {
    connect(ck, &QAbstractButton::clicked, this, &CheckEnabler::checkStatusChanged);
    checkStatusChanged();
    fixWhatsThis();
  }

public slots:
  void checkStatusChanged()
  {
    bool b = checkBox->isChecked();
    for (int i=0; i<widgetList.size(); i++) {
      widgetList[i]->setEnabled(b);
    }
  }

private:
  QAbstractButton* checkBox;
  QList<QWidget*> widgetList;
  void fixWhatsThis()
  {
    QString wts = checkBox->whatsThis();
    if (wts.length() != 0) {
      for (int i=0; i<widgetList.size(); i++) {
        QString s = widgetList[i]->whatsThis();
        if (s.length() == 0) {
          widgetList[i]->setWhatsThis(wts);
        }
      }
    }
    QString wtf = checkBox->toolTip();
    if (wtf.length() != 0) {
      for (int i=0; i<widgetList.size(); i++) {
        QString s = widgetList[i]->toolTip();
        if (s.length() == 0) {
          widgetList[i]->setToolTip(wtf);
        }
      }
    }
  }

};

//------------------------------------------------------------------------
class FilterOption
{
public:
  FilterOption() = default;
  /* Reference data members C.12 */
  FilterOption(const FilterOption &) = delete;
  FilterOption &operator=(const FilterOption &) = delete;
  FilterOption(FilterOption &&) = delete;
  FilterOption &operator=(FilterOption &&) = delete;
  virtual ~FilterOption() = default;

  virtual void setWidgetValue() = 0;
  virtual void getWidgetValue() = 0;
};

//------------------------------------------------------------------------
class BoolFilterOption: public FilterOption
{
public:
  BoolFilterOption(bool& b, QAbstractButton* ck):  b(b), checkBox(ck)
  {
  }
  void setWidgetValue() override
  {
    checkBox->setChecked(b);
  }
  void getWidgetValue() override
  {
    b = checkBox->isChecked();
  }

private:
  bool& b;
  QAbstractButton* checkBox;
};

//------------------------------------------------------------------------
class IntSpinFilterOption: public FilterOption
{
public:
  IntSpinFilterOption(int& val, QSpinBox* sb, int bottom = -100, int top = 100):  val(val), spinBox(sb)
  {
    sb->setRange(bottom, top);
  }
  void setWidgetValue() override
  {
    spinBox->setValue(val);
  }
  void getWidgetValue() override
  {
    val = spinBox->value();
  }

private:
  int& val;
  QSpinBox* spinBox;
};

//------------------------------------------------------------------------
class StringFilterOption: public FilterOption
{
public:
  StringFilterOption(QString& val, QLineEdit* le):  val(val), lineEdit(le)
  {
  }
  void setWidgetValue() override
  {
    lineEdit->setText(val);
  }
  void getWidgetValue() override
  {
    val = lineEdit->text();
  }

private:
  QString& val;
  QLineEdit* lineEdit;
};

//------------------------------------------------------------------------
class DoubleFilterOption: public FilterOption
{
public:
  DoubleFilterOption(double& val, QLineEdit* le,
                     double minVal = -1.E308,
                     double maxVal = 1.0E308,
                     int decimals = -1,
                     char format = 'g'
                    ):  val(val), lineEdit(le), minVal(minVal),
    maxVal(maxVal), decimals(decimals), format(format)
  {
    le->setValidator(new QDoubleValidator(minVal, maxVal, decimals, le));
  }
  void setWidgetValue() override
  {
    lineEdit->setText(QString("%1").arg(val, 0, format, decimals));
  }
  void getWidgetValue() override
  {
    val = lineEdit->text().toDouble();
    val = qMin(val, maxVal);
    val = qMax(val, minVal);
  }

private:
  double& val;
  QLineEdit* lineEdit;
  double minVal, maxVal;
  int decimals;
  char format;
};

//------------------------------------------------------------------------
class DateTimeFilterOption: public FilterOption
{
public:
  DateTimeFilterOption(QDateTime& val, QDateTimeEdit* w):  val(val), w(w)
  {
  }
  void setWidgetValue() override
  {
    w->setDateTime(val);
  }
  void getWidgetValue() override
  {
    val = w->dateTime();
  }

private:
  QDateTime& val;
  QDateTimeEdit* w;
};

//------------------------------------------------------------------------
class ComboFilterOption: public FilterOption
{
public:
  ComboFilterOption(int& val, QComboBox* w):  val(val), w(w)
  {
  }
  void setWidgetValue() override
  {
    w->setCurrentIndex(val);
  }
  void getWidgetValue() override
  {
    val = w->currentIndex();
  }

private:
  int& val;
  QComboBox* w;
};


//------------------------------------------------------------------------
class FilterWidget: public QWidget
{
public:
  FilterWidget(QWidget* parent) : QWidget(parent) {}

  void getWidgetValues()
  {
    for (const auto& fopt : fopts) {
      fopt->getWidgetValue();
    }
  }
  void setWidgetValues()
  {
    for (const auto& fopt : fopts) {
      fopt->setWidgetValue();
    }
  }

  void addFilterOption(std::unique_ptr<FilterOption> fo)
  {
    fopts.push_back(std::move(fo));
  }

  void addCheckEnabler(QAbstractButton* ck, QWidget* w)
  {
    enbls << new CheckEnabler(this, ck, w);
  }
  void addCheckEnabler(QAbstractButton* ck, QList<QWidget*>& wl)
  {
    enbls << new CheckEnabler(this, ck, wl);
  }
  virtual void checkChecks()
  {
    for (int i=0; i<enbls.size(); i++) {
      enbls[i]->checkStatusChanged();
    }
  }

private:
  std::vector<std::unique_ptr<FilterOption>> fopts;
  QList <CheckEnabler*> enbls;
};

//------------------------------------------------------------------------

class TrackWidget final: public FilterWidget
{
  Q_OBJECT
public:
  TrackWidget(QWidget* parent, TrackFilterData& tf);

  /* Since the TrackWidget ctor calls virtual function checkChecks() we must
   * not derive a class from TrackWidget. To prevent this possiblity
   * we mark TrackWidget final.
   * "warning: Call to virtual method 'TrackWidget::checkChecks' during
   * construction bypasses virtual dispatch
   * [clang-analyzer-optin.cplusplus.VirtualCall]"
   * https://www.artima.com/articles/never-call-virtual-functions-during-construction-or-destruction
   */
  void checkChecks() override
  {
    otherCheckX();
    FilterWidget::checkChecks();
  }

private:
  Ui_TrackWidget ui;
  TrackFilterData& tfd;

private slots:
  void mergeCheckX();
  void otherCheckX() const;
  void splitDateX();
  void splitTimeX();
  void splitDistanceX();
  void TZX() const;
  void packCheckX();
};

//------------------------------------------------------------------------
class WayPtsWidget: public FilterWidget
{
  Q_OBJECT
public:
  WayPtsWidget(QWidget* parent, WayPtsFilterData& wf);

private:
  Ui_WayPtsWidget ui;
  WayPtsFilterData& wfd;

private slots:
  void locationsCkX() const;
  void shortNamesCkX() const;
};

//------------------------------------------------------------------------
class RtTrkWidget: public FilterWidget
{
  Q_OBJECT
public:
  RtTrkWidget(QWidget* parent, RtTrkFilterData& rfd);

private:
  Ui_RtTrkWidget ui;
  RtTrkFilterData& rfd;
};
//------------------------------------------------------------------------
class MiscFltWidget: public FilterWidget
{
  Q_OBJECT
public:
  MiscFltWidget(QWidget* parent, MiscFltFilterData& mfd);

private:
  Ui_MiscFltWidget ui;
  MiscFltFilterData& mfd;
};
#endif
