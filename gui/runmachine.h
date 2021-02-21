/*
    Copyright (C) 2021 Robert Lipe, gpsbabel.org

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

#ifndef RUNMACHINE_H
#define RUNMACHINE_H

#include <QtCore/qglobal.h>    // for qDebug
#include <QtCore/QDebug>       // for QDebug, operator<<
#include <QtCore/QObject>      // for QObject
#include <QtCore/QProcess>     // for QProcess, QProcess::ExitStatus, QProcess::ProcessError, qt_getEnumName
#include <QtCore/QString>      // for QString
#include <QtCore/QStringList>  // for QStringList
#include <QtWidgets/QWidget>   // for QWidget

#include <optional>            // for optional, nullopt

#include "processwait.h"       // for ProcessWaitDialog

namespace runMachine
{

static constexpr bool debug = false;

class SignalForwarder : public QObject
{
  Q_OBJECT

public:

  enum SignalId {
    start,
    processErrorOccurred,
    processStarted,
    processFinished,
    abortRequested
  };
  Q_ENUM(SignalId)

  explicit SignalForwarder(QObject* parent): QObject(parent) {}

Q_SIGNALS:
  void forwardedSignal(SignalForwarder::SignalId id,
                       std::optional<QProcess::ProcessError>,
                       std::optional<int> exitCode,
                       std::optional<QProcess::ExitStatus> exitStatus);

public Q_SLOTS:
  void errorOccurredX(QProcess::ProcessError error)
  {
    if constexpr(debug) {
      qDebug() << "emit error occurred" << error;
    }
    emit forwardedSignal(processErrorOccurred,
                         std::optional<QProcess::ProcessError>(error),
                         std::nullopt,
                         std::nullopt);
  }
  void startedX()
  {
    if constexpr(debug) {
      qDebug() << "emit started";
    }
    emit forwardedSignal(processStarted,
                         std::nullopt,
                         std::nullopt,
                         std::nullopt);
  }
  void finishedX(int exitCode, QProcess::ExitStatus exitStatus)
  {
    if constexpr(debug) {
      qDebug() << "emit finished";
    }
    emit forwardedSignal(processFinished,
                         std::nullopt,
                         std::optional<int>(exitCode),
                         std::optional<QProcess::ExitStatus>(exitStatus));
  }
  void abortX()
  {
    if constexpr(debug) {
      qDebug() << "emit abort";
    }
    emit forwardedSignal(abortRequested,
                         std::nullopt,
                         std::nullopt,
                         std::nullopt);
  }

};

class RunMachine : public QWidget
{
  Q_OBJECT

public:

  /* Types */

  enum State {
    init,
    starting,
    running,
    done
  };
  Q_ENUM(State)

  /* Special Member Functions */

  RunMachine(QWidget* parent, const QString& program, const QStringList& args);

  /* Member Functions */

  static QString decodeProcessError(QProcess::ProcessError err);
  int exec();
  void open();
  QString getOutputString()
  {
    return progress_->getOutputString();
  }
  QString getErrorString()
  {
    return errorString_;
  }
  bool getRetStatus() const
  {
    return errorString_.isEmpty();
  }

Q_SIGNALS:
  void finished();

private Q_SLOTS:
  void execute(SignalForwarder::SignalId id,
               std::optional<QProcess::ProcessError> error,
               std::optional<int> exitCode,
               std::optional<QProcess::ExitStatus> exitStatus);

private:

  static constexpr bool finishOnAbort = false;
  static constexpr bool finishOnRunningError = false;

  /* Data Members */

  QObject* parent_{nullptr};
  QProcess* process_{nullptr};
  ProcessWaitDialog* progress_{nullptr};
  SignalForwarder* forwarder_{nullptr};
  State state_{init};
  QString program_;
  QStringList args_;
  QString errorString_;

};

} // namespace runMachine
#endif
