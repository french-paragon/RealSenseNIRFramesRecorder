#include "consolewatcher.h"

#include <QSocketNotifier>
#include <QDebug>

const QString ConsoleWatcher::exit_cmd = "exit";
const QString ConsoleWatcher::set_folder_cmd = "output";
const QString ConsoleWatcher::set_camera_cmd = "camera";
const QString ConsoleWatcher::start_record_cmd = "start";
const QString ConsoleWatcher::record_cmd = "save";
const QString ConsoleWatcher::stop_save_cmd = "stopsave";
const QString ConsoleWatcher::record_interval_cmd = "saveinterval";
const QString ConsoleWatcher::stop_record_cmd = "stop";
const QString ConsoleWatcher::export_record_cmd = "export";
const QString ConsoleWatcher::ir_toggle_cmd = "irpattern";
const QString ConsoleWatcher::list_cams_cmd = "list";
const QString ConsoleWatcher::list_connections_cmd = "remotes";
const QString ConsoleWatcher::remote_connect_cmd = "connect";
const QString ConsoleWatcher::remote_ping_cmd = "ping";
const QString ConsoleWatcher::remote_disconnect_cmd = "disconnect";
const QString ConsoleWatcher::time_cmd = "time";
const QString ConsoleWatcher::config_time_cmd = "cfgtime";
const QString ConsoleWatcher::help_cmd = "help";

ConsoleWatcher::ConsoleWatcher(QObject *parent) :
	QObject(parent),
	_out(stdout),
	_in(stdin)
{
	_notifier = new QSocketNotifier(fileno(stdin), QSocketNotifier::Read, this);
}

void ConsoleWatcher::run() {
	_out << ">" << flush;
	connect(_notifier, &QSocketNotifier::activated, this, &ConsoleWatcher::readCommand);
}
void ConsoleWatcher::stop() {
	disconnect(_notifier, &QSocketNotifier::activated, this, &ConsoleWatcher::readCommand);
}


void ConsoleWatcher::readCommand() {

	QString line = _in.readLine();

	QVector<QStringRef> values = line.splitRef(' ', QString::SkipEmptyParts);
	QString cmd = values[0].toString().trimmed().toLower();

	if(isExit(cmd)) {
		if (values.size() != 1) {
			Q_EMIT InvalidTriggered(line);
		} else {
			emit exitTriggered();
		}
	} else if (cmd == set_folder_cmd) {

		if (values.size() != 2) {
			Q_EMIT InvalidTriggered(line);
		} else {
			emit setFolderTriggered(values[1].toString());
		}

	} else if (cmd == set_camera_cmd) {

		if (values.size() != 2) {
			Q_EMIT InvalidTriggered(line);
		} else {

			bool ok = true;
			int cam = values[1].toInt(&ok);

			if (!ok) {
				cam = -1;
			}

			emit setCameraTriggered(cam);
		}

	} else if (cmd == start_record_cmd) {

		if (values.size() != 2) {
			Q_EMIT InvalidTriggered(line);
		} else {

			if (values[1] == "session") {
				emit startRecordSessionTriggered();
			} else {

				bool ok = true;
				int cam = values[1].toInt(&ok);

				if (!ok) {
					cam = -1;
				}

				emit startRecordTriggered(cam);
			}
		}

	} else if (cmd == record_cmd) {

		if (values.size() == 1) {

			emit saveImgsContinuousTriggered();

		} else if (values.size() != 2) {
			Q_EMIT InvalidTriggered(line);
		} else {

			bool ok = true;

			int nFrames = -1;

			nFrames = values[1].toInt(&ok);
			if (!ok) {
				nFrames = 0;
			}
			emit saveImgsTriggered(nFrames);
		}

	} else if (cmd == stop_save_cmd) {

		if (values.size() == 1) {

			emit stopSaveImgsTriggered();

		} else {
			Q_EMIT InvalidTriggered(line);
		}

	} else if (cmd == record_interval_cmd) {

		if (values.size() != 3) {
			Q_EMIT InvalidTriggered(line);
		} else {

			bool ok = true;

			int nFrames = -1;
			int msec = 0;

			nFrames = values[1].toInt(&ok);
			msec = values[2].toInt(&ok);

			if (!ok) {
				nFrames = 0;
				msec = 0;
			}

			emit saveImgsIntervalTriggered(nFrames, msec);
		}

	} else if (cmd == stop_record_cmd) {

		if (values.size() != 1) {
			Q_EMIT InvalidTriggered(line);
		} else {
			emit stopRecordTriggered();
		}

	} else if (cmd == export_record_cmd) {

		if (values.size() != 1) {
			Q_EMIT InvalidTriggered(line);
		} else {
			emit exportRecordTriggered();
		}

	} else if (cmd == ir_toggle_cmd) {

		if (values.size() != 2) {
			Q_EMIT InvalidTriggered(line);
		} else {
			if (values[1].toString().toLower() == "on") {
				setIrPatternTriggered(true);
			} else if (values[1].toString().toLower() == "off") {
				setIrPatternTriggered(false);
			} else {
				Q_EMIT InvalidTriggered(line);
			}
		}

	} else if (cmd == list_cams_cmd) {

		if (values.size() != 1) {
			Q_EMIT InvalidTriggered(line);
		} else {
			emit listCamerasTriggered();
		}

	} else if (cmd == list_connections_cmd) {

		if (values.size() != 1) {
			Q_EMIT InvalidTriggered(line);
		} else {
			emit listConnectionsTriggered();
		}

	} else if (cmd == remote_connect_cmd) {

		if (values.size() != 2) {
			Q_EMIT InvalidTriggered(line);
		} else {
			emit remoteConnectionTriggered(values[1].toString());
		}

	} else if (cmd == remote_ping_cmd) {

		if (values.size() != 2) {
			Q_EMIT InvalidTriggered(line);
		} else {

			bool ok = true;
			int row = values[1].toInt(&ok);

			if (!ok) {
				row = -1;
			}

			emit remotePingTriggered(row);
		}

	} else if (cmd == remote_disconnect_cmd) {

		if (values.size() != 2) {
			Q_EMIT InvalidTriggered(line);
		} else {
			emit remoteDisconnectionTriggered(values[1].toString());
		}

	} else if (cmd == time_cmd) {

		if (values.size() != 1) {
			Q_EMIT InvalidTriggered(line);
		} else {
			emit timeTriggered();
		}

	} else if (cmd == config_time_cmd) {

		if (values.size() != 3) {
			Q_EMIT InvalidTriggered(line);
		} else {

			bool ok;
			int port = values[2].toInt(&ok);

			if (ok) {
				emit configTimeTriggered(values[1].toString(), port);
			} else {
				Q_EMIT InvalidTriggered(line);
			}
		}

	} else if (cmd == help_cmd) {

		if (values.size() != 1) {
			Q_EMIT InvalidTriggered(line);
		} else {
			emit helpTriggered();
		}

	} else {
		emit InvalidTriggered(line);
	}

	_out << ">" << flush;
}

bool ConsoleWatcher::isExit(QString cmd) {
	return cmd.trimmed().toLower() == exit_cmd;
}
