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

#include "runmachine.h"

#include <QtCore/QEventLoop>         // for QEventLoop
#include <QtCore/QNonConstOverload>  // for QNonConstOverload
#include <QtCore/QtGlobal>           // for QOverload, qOverload
#include <QtWidgets/QDialog>         // for QDialog

#include "appname.h"                 // for appName


namespace runMachine
{

QString RunMachine::decodeProcessError(QProcess::ProcessError err)
{
  switch (err) {
  case QProcess::FailedToStart:
    return tr("Process failed to start");
    break;
  case QProcess::Crashed:
    return tr("Process crashed");
    break;
  case QProcess::Timedout:
    return tr("Process timedout");
    break;
  case QProcess::WriteError:
    return tr("Error while trying to write to process");
    break;
  case QProcess::ReadError:
    return tr("Error while trying to read from process");
    break;
  case QProcess::UnknownError:
  default:
    return tr("Unknown process error");
  }
}

RunMachine::RunMachine(QWidget* parent,
                       const QString& program,
                       const QStringList& args) :
  QWidget(parent), program_(program), args_(args)
{
  forwarder_ = new SignalForwarder(this);
  process_ = new QProcess(this);
  progress_ = new ProcessWaitDialog(this, process_);
  // It is important that at least some of the fowarded signals are
  // QueuedConnections to avoid reentrant use of RunMachine::execute which it
  // is not designed for.
  connect(process_, &QProcess::errorOccurred,
          forwarder_, &SignalForwarder::errorOccurredX, Qt::QueuedConnection);
  connect(process_, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
          forwarder_, &SignalForwarder::finishedX, Qt::QueuedConnection);
  connect(process_, &QProcess::started, forwarder_,
          &SignalForwarder::startedX, Qt::QueuedConnection);
  connect(progress_, &ProcessWaitDialog::rejected,
          forwarder_, &SignalForwarder::abortX, Qt::QueuedConnection);
  connect(forwarder_, &SignalForwarder::forwardedSignal,
          this, &RunMachine::execute);
}

int RunMachine::exec()
{
  open();

  // block until complete.
  QEventLoop loop;
  connect(this, &RunMachine::finished, &loop, &QEventLoop::quit);
  loop.exec();

  return getRetStatus();
}

void RunMachine::open()
{
  execute(SignalForwarder::start, std::nullopt, std::nullopt, std::nullopt);
}

void RunMachine::execute(SignalForwarder::SignalId id,
                         std::optional<QProcess::ProcessError> error,
                         std::optional<int> exitCode,
                         std::optional<QProcess::ExitStatus> exitStatus)
{
  if constexpr(debug) {
    QDebug debugStream = qDebug();
    debugStream << "exec entering" << state_ << id;
    if (error.has_value()) {
      debugStream << *error;
    }
    if (exitStatus.has_value()) {
      debugStream << *exitStatus;
    }
    if (exitCode.has_value()) {
      debugStream << *exitCode;
    }
  }

  switch (state_) {
  case init:
    process_->start(program_, args_);
    state_ = starting;
    break;

  case starting:
    switch (id) {
    case SignalForwarder::processErrorOccurred:
      errorString_ = QString(tr("Process \"%1\" did not start")).arg(appName);
      state_ = done;
      emit finished();
      break;
    case SignalForwarder::processStarted:
      //progress_ = new ProcessWaitDialog(nullptr, process_);
      progress_->show();
      progress_->open();
      state_ = running;
      break;
    default:
      if constexpr(debug) {
        qDebug() << "signal" << id << "UNEXPECTED in starting state!";
      }
      break;
    };
    break;

  case running:
    switch (id) {
    case SignalForwarder::processErrorOccurred:
      if constexpr(finishOnRunningError) {
        progress_->accept();
        errorString_ = decodeProcessError(*error);
        state_ = done;
        emit finished();
      }
      break;
    case SignalForwarder::processFinished:
      progress_->accept();
      if (*exitStatus == QProcess::NormalExit) {
        if (*exitCode != 0) {
          errorString_ =
            QString(tr("Process exited unsuccessfully with code %1"))
            .arg(*exitCode);
        }
      } else {
        errorString_ = tr("Process crashed while running");
      }
      state_ = done;
      emit finished();
      break;
    case SignalForwarder::abortRequested:
      if constexpr(finishOnAbort) {
        // To avoid a message from ~QProcess we need to close the process
        // instead of killing it.
        // QProcess: Destroyed while process (".../gpsbabel") is still running."
        process_->close();
        errorString_ = tr("Process crashed while running");
        state_ = done;
        emit finished();
      } else {
        // Console applications on Windows that do not run an event loop, or
        // whose event loop does not handle the WM_CLOSE message, can only be
        // terminated
        process_->kill();
      }
      break;
    default:
      if constexpr(debug) {
        qDebug() << "signal" << id << "UNEXPECTED in running state!";
      }
      break;
    };
    break;

  case done:
    break;

  default:
    if constexpr(debug) {
      qDebug() << "UNEXPECTED State!";
    }
    break;
  };
  if constexpr(debug) {
    qDebug() << "exec leaving" << state_ << id;
  }
}

} // namespace runMachine
